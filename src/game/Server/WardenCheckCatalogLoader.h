/**
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2026 MaNGOS <https://www.getmangos.eu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef MANGOS_WARDEN_CHECK_CATALOG_LOADER_H
#define MANGOS_WARDEN_CHECK_CATALOG_LOADER_H

#include "WardenCheckCatalog.h"
#include "WardenModuleCatalog.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <utility>

namespace warden
{
/** Startup failure categories safe to expose without catalogue payloads. */
enum class WardenCheckCatalogLoadFailure : uint8
{
    None,
    CatalogueQueryFailed,
    EmptyCatalogue,
    SourceCountOverflow,
    SourceCountInconsistent,
    SourceCountMismatch,
    InvalidRow,
    ProfileWithoutModule,
    ModuleWithoutProfile,
    InvalidPlan,
    PublicationFailed
};

/** Returns a stable operator-facing label for a startup failure category. */
char const* ToString(WardenCheckCatalogLoadFailure failure);

/** Ensures module and check profiles cover the same exact client identities. */
inline WardenCheckCatalogLoadFailure ValidateWardenCatalogCoverage(
    WardenCheckCatalog const& checks, WardenModuleCatalog const& modules)
{
    for (WardenCheckProfile const& profile : checks.Profiles())
    {
        ModuleProfile const* module =
            modules.Find(profile.key.build, profile.key.platform);
        if (!module || std::find(module->requiredCheckLocales.begin(),
            module->requiredCheckLocales.end(), profile.key.locale) ==
                module->requiredCheckLocales.end())
            return WardenCheckCatalogLoadFailure::ProfileWithoutModule;
    }

    for (ModuleProfile const* module : modules.Profiles())
    {
        for (std::string const& locale : module->requiredCheckLocales)
        {
            if (!checks.Find(module->build, module->platform, locale))
                return WardenCheckCatalogLoadFailure::ModuleWithoutProfile;
        }
    }
    return WardenCheckCatalogLoadFailure::None;
}

using WardenCatalogPreflight =
    std::function<bool(WardenCheckProfile const&)>;
using WardenCatalogPublisher = std::function<bool(
    std::shared_ptr<WardenCheckCatalog const> const&)>;

/**
 * Pure startup transaction shared by the SQL adapter and focused tests.
 * No catalogue can escape until source-count, row, profile, coverage, and
 * wire-plan validation all complete.
 */
class WardenCheckCatalogLoadTransaction
{
public:
    /** Starts one statement snapshot without reading any nullable row field. */
    WardenCheckCatalogLoadFailure Begin(uint64 sourceCount)
    {
        if (m_started || m_finished)
            return SetFailure(
                WardenCheckCatalogLoadFailure::SourceCountInconsistent);
        m_started = true;
        m_expectedRows = sourceCount;
        if (!sourceCount)
            return SetFailure(WardenCheckCatalogLoadFailure::EmptyCatalogue);
        if (sourceCount > std::numeric_limits<uint32>::max())
            return SetFailure(
                WardenCheckCatalogLoadFailure::SourceCountOverflow);
        return WardenCheckCatalogLoadFailure::None;
    }

    /** Validates the repeated scalar before the caller reads this SQL row. */
    WardenCheckCatalogLoadFailure ObserveSourceCount(uint64 sourceCount)
    {
        if (m_failure != WardenCheckCatalogLoadFailure::None)
            return m_failure;
        if (!m_started || m_finished || m_sourceCountObserved)
            return SetFailure(
                WardenCheckCatalogLoadFailure::CatalogueQueryFailed);
        if (sourceCount != m_expectedRows)
            return SetFailure(
                WardenCheckCatalogLoadFailure::SourceCountInconsistent);
        m_sourceCountObserved = true;
        return WardenCheckCatalogLoadFailure::None;
    }

    /** Validates and stages one complete row after its scalar was accepted. */
    WardenCheckCatalogLoadFailure Add(WardenCheckRowInput const& input,
        WardenCheckDiagnostic& diagnostic)
    {
        if (m_failure != WardenCheckCatalogLoadFailure::None)
            return m_failure;
        if (!m_started || m_finished || !m_sourceCountObserved)
            return SetFailure(
                WardenCheckCatalogLoadFailure::CatalogueQueryFailed);
        if (m_consumedRows >= m_expectedRows)
            return SetFailure(
                WardenCheckCatalogLoadFailure::SourceCountMismatch);
        if (m_builder.Add(input, diagnostic) !=
            CheckCatalogValidation::Valid)
            return SetFailure(WardenCheckCatalogLoadFailure::InvalidRow);
        ++m_consumedRows;
        m_sourceCountObserved = false;
        return WardenCheckCatalogLoadFailure::None;
    }

    /** Builds privately and calls the publisher exactly once on full success. */
    WardenCheckCatalogLoadFailure Finish(WardenModuleCatalog const& modules,
        WardenCatalogPreflight const& preflight,
        WardenCatalogPublisher const& publisher,
        WardenCheckDiagnostic& diagnostic)
    {
        if (m_failure != WardenCheckCatalogLoadFailure::None)
            return m_failure;
        if (!m_started || m_finished)
            return SetFailure(
                WardenCheckCatalogLoadFailure::CatalogueQueryFailed);
        if (m_sourceCountObserved)
            return SetFailure(
                WardenCheckCatalogLoadFailure::SourceCountMismatch);
        if (m_consumedRows != m_expectedRows)
            return SetFailure(
                WardenCheckCatalogLoadFailure::SourceCountMismatch);

        WardenCheckCatalog candidate;
        if (m_builder.Build(candidate, diagnostic) !=
            CheckCatalogValidation::Valid)
            return SetFailure(WardenCheckCatalogLoadFailure::InvalidRow);

        WardenCheckCatalogLoadFailure const coverage =
            ValidateWardenCatalogCoverage(candidate, modules);
        if (coverage != WardenCheckCatalogLoadFailure::None)
            return SetFailure(coverage);

        if (!preflight)
            return SetFailure(WardenCheckCatalogLoadFailure::InvalidPlan);
        for (WardenCheckProfile const& profile : candidate.Profiles())
        {
            if (!preflight(profile))
                return SetFailure(WardenCheckCatalogLoadFailure::InvalidPlan);
        }

        auto mutableSnapshot =
            std::make_shared<WardenCheckCatalog>(std::move(candidate));
        std::shared_ptr<WardenCheckCatalog const> snapshot = mutableSnapshot;
        if (!publisher || !publisher(snapshot))
            return SetFailure(
                WardenCheckCatalogLoadFailure::PublicationFailed);
        m_finished = true;
        return WardenCheckCatalogLoadFailure::None;
    }

private:
    WardenCheckCatalogLoadFailure SetFailure(
        WardenCheckCatalogLoadFailure failure)
    {
        m_failure = failure;
        return failure;
    }

    WardenCheckCatalogBuilder m_builder;
    uint64 m_expectedRows = 0;
    uint64 m_consumedRows = 0;
    WardenCheckCatalogLoadFailure m_failure =
        WardenCheckCatalogLoadFailure::None;
    bool m_started = false;
    bool m_finished = false;
    bool m_sourceCountObserved = false;
};

/** Required synchronous startup loader for the World check catalogue. */
class WardenCheckCatalogLoader
{
public:
    /** Validates the complete DB snapshot before atomically publishing it. */
    bool LoadAndPublish() const;
};
}

#endif

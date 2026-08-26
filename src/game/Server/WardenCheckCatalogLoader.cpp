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

#include "WardenCheckCatalogLoader.h"

#include "Database/DatabaseEnv.h"
#include "Log.h"
#include "WardenCheckPlanner.h"
#include "WardenManager.h"
#include "WardenPacketCodec.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace
{
std::string SafeToken(std::string const& value)
{
    if (value.empty())
        return "<unavailable>";
    for (unsigned char byte : value)
    {
        bool const alphaNumeric = (byte >= '0' && byte <= '9') ||
            (byte >= 'A' && byte <= 'Z') ||
            (byte >= 'a' && byte <= 'z');
        if (!alphaNumeric)
            return "<invalid>";
    }
    return value;
}

std::string SqlHex(Field const& field)
{
    char const* value = field.GetString();
    return value ? std::string(value) : std::string();
}

char const* PurposeName(warden::CheckPlanPurpose purpose)
{
    switch (purpose)
    {
        case warden::CheckPlanPurpose::Initial: return "Initial";
        case warden::CheckPlanPurpose::Recurring: return "Recurring";
        case warden::CheckPlanPurpose::AggressiveImmediate:
            return "AggressiveImmediate";
        case warden::CheckPlanPurpose::AggressiveRecurring:
            return "AggressiveRecurring";
        case warden::CheckPlanPurpose::Confirmation: return "Confirmation";
    }
    return "Unknown";
}

void LogLoadFailure(warden::WardenCheckCatalogLoadFailure failure)
{
    sLog.outError("Warden catalogue load failed: %s.",
        warden::ToString(failure));
}

void LogLoadFailure(warden::WardenCheckCatalogLoadFailure failure,
    warden::WardenCheckDiagnostic const& diagnostic)
{
    std::string const platform = SafeToken(diagnostic.profile.platform);
    std::string const locale = SafeToken(diagnostic.profile.locale);
    sLog.outError("Warden catalogue load failed: %s (build %u; platform %s; "
        "locale %s; check %u; validation %s).", warden::ToString(failure),
        diagnostic.profile.build, platform.c_str(), locale.c_str(),
        diagnostic.checkId, warden::ToString(diagnostic.validation));
}

bool PreflightProfile(warden::WardenCheckProfile const& profile)
{
    std::string const platform = SafeToken(profile.key.platform);
    std::string const locale = SafeToken(profile.key.locale);
    std::vector<warden::CheckPlan> const plans =
        warden::BuildWardenPreflightPlans(profile);
    if (plans.empty())
    {
        sLog.outError("Warden catalogue plan preflight failed (build %u; "
            "platform %s; locale %s; validation Empty).", profile.key.build,
            platform.c_str(), locale.c_str());
        return false;
    }

    for (warden::CheckPlan const& plan : plans)
    {
        warden::WardenCheckPlanBudget budget;
        warden::CheckPlanValidation const validation =
            warden::InspectCheckPlan(plan, budget);
        if (validation != warden::CheckPlanValidation::Valid)
        {
            sLog.outError("Warden catalogue plan preflight failed (build %u; "
                "platform %s; locale %s; purpose %s; validation %s).",
                profile.key.build, platform.c_str(), locale.c_str(),
                PurposeName(plan.purpose), warden::ToString(validation));
            return false;
        }
    }
    return true;
}

void LogSummary(std::shared_ptr<warden::WardenCheckCatalog const> const& snapshot)
{
    sLog.outString("Warden catalogue loaded: %u total rows, %u enabled rows, "
        "%u profiles.", snapshot->TotalRows(), snapshot->EnabledRows(),
        uint32(snapshot->Profiles().size()));
    for (warden::WardenCheckProfile const& profile : snapshot->Profiles())
    {
        std::string const platform = SafeToken(profile.key.platform);
        std::string const locale = SafeToken(profile.key.locale);
        std::array<uint32, 4> typeCounts{};
        std::array<uint32, 4> classCounts{};
        for (warden::WardenCheckDefinition const& definition : profile.checks)
        {
            switch (warden::GetWardenCheckType(definition))
            {
                case warden::WardenCheckType::Timing: ++typeCounts[0]; break;
                case warden::WardenCheckType::Mpq: ++typeCounts[1]; break;
                case warden::WardenCheckType::Lua: ++typeCounts[2]; break;
                case warden::WardenCheckType::Mem: ++typeCounts[3]; break;
            }
            ++classCounts[uint8(definition.evidenceClass)];
        }
        sLog.outString("Warden profile %u/%s/%s: Timing %u, MPQ %u, Lua %u, "
            "MEM %u; ProtocolHealth %u, IntegrityInvariant %u, "
            "ThreatSignature %u, Corroboration %u.", profile.key.build,
            platform.c_str(), locale.c_str(), typeCounts[0], typeCounts[1],
            typeCounts[2], typeCounts[3], classCounts[0], classCounts[1],
            classCounts[2], classCounts[3]);
    }
}
}

namespace warden
{
char const* ToString(WardenCheckCatalogLoadFailure failure)
{
    switch (failure)
    {
        case WardenCheckCatalogLoadFailure::None: return "None";
        case WardenCheckCatalogLoadFailure::CatalogueQueryFailed:
            return "CatalogueQueryFailed";
        case WardenCheckCatalogLoadFailure::EmptyCatalogue:
            return "EmptyCatalogue";
        case WardenCheckCatalogLoadFailure::SourceCountOverflow:
            return "SourceCountOverflow";
        case WardenCheckCatalogLoadFailure::SourceCountInconsistent:
            return "SourceCountInconsistent";
        case WardenCheckCatalogLoadFailure::SourceCountMismatch:
            return "SourceCountMismatch";
        case WardenCheckCatalogLoadFailure::InvalidRow: return "InvalidRow";
        case WardenCheckCatalogLoadFailure::ProfileWithoutModule:
            return "ProfileWithoutModule";
        case WardenCheckCatalogLoadFailure::ModuleWithoutProfile:
            return "ModuleWithoutProfile";
        case WardenCheckCatalogLoadFailure::InvalidPlan: return "InvalidPlan";
        case WardenCheckCatalogLoadFailure::PublicationFailed:
            return "PublicationFailed";
    }
    return "Unknown";
}

bool WardenCheckCatalogLoader::LoadAndPublish() const
{
    // The scalar and rows come from one statement snapshot. The LEFT JOIN
    // returns one count-only sentinel when the catalogue is empty.
    std::unique_ptr<QueryResult> result(WorldDatabase.Query(
        "SELECT `snapshot`.`snapshot_count`, `checks`.`build`, "
        "HEX(`checks`.`platform`), HEX(`checks`.`locale`), "
        "`checks`.`check_id`, `checks`.`type`, `checks`.`enabled`, "
        "`checks`.`sort_order`, `checks`.`evidence_class`, "
        "HEX(`checks`.`module`), `checks`.`address`, `checks`.`length`, "
        "HEX(`checks`.`request`), HEX(`checks`.`expected`) "
        "FROM (SELECT COUNT(*) AS `snapshot_count` FROM `warden_checks`) "
        "AS `snapshot` LEFT JOIN `warden_checks` AS `checks` ON TRUE "
        "ORDER BY `checks`.`build`, `checks`.`platform`, `checks`.`locale`, "
        "`checks`.`sort_order`, `checks`.`check_id`"));
    if (!result)
    {
        LogLoadFailure(WardenCheckCatalogLoadFailure::CatalogueQueryFailed);
        return false;
    }

    Field const* fields = result->Fetch();
    if (!fields)
    {
        LogLoadFailure(WardenCheckCatalogLoadFailure::CatalogueQueryFailed);
        return false;
    }

    WardenCheckCatalogLoadTransaction transaction;
    WardenCheckCatalogLoadFailure failure =
        transaction.Begin(fields[0].GetUInt64());
    if (failure != WardenCheckCatalogLoadFailure::None)
    {
        // Begin consumes only snapshot_count, never nullable sentinel fields.
        LogLoadFailure(failure);
        return false;
    }

    WardenCheckDiagnostic diagnostic;
    do
    {
        fields = result->Fetch();
        // Reject a torn/repeated scalar before reading any projected row field.
        failure = transaction.ObserveSourceCount(fields[0].GetUInt64());
        if (failure != WardenCheckCatalogLoadFailure::None)
        {
            LogLoadFailure(failure);
            return false;
        }

        // Keep SQL values at full schema width until the builder validates
        // every narrowing conversion and family-specific field contract.
        WardenCheckRowInput input;
        input.build = fields[1].GetUInt32();
        input.platformHex = SqlHex(fields[2]);
        input.localeHex = SqlHex(fields[3]);
        input.checkId = fields[4].GetUInt32();
        input.type = fields[5].GetUInt32();
        input.enabled = fields[6].GetUInt32();
        input.sortOrder = fields[7].GetUInt32();
        input.evidenceClass = fields[8].GetUInt32();
        input.moduleHex = SqlHex(fields[9]);
        input.address = fields[10].GetUInt32();
        input.length = fields[11].GetUInt32();
        input.requestHex = SqlHex(fields[12]);
        input.expectedHex = SqlHex(fields[13]);
        failure = transaction.Add(input, diagnostic);
        if (failure != WardenCheckCatalogLoadFailure::None)
        {
            if (failure == WardenCheckCatalogLoadFailure::InvalidRow)
                LogLoadFailure(failure, diagnostic);
            else
                LogLoadFailure(failure);
            return false;
        }
    }
    while (result->NextRow());

    std::shared_ptr<WardenCheckCatalog const> publishedSnapshot;
    WardenCatalogPublisher publisher =
        [&publishedSnapshot](
            std::shared_ptr<WardenCheckCatalog const> const& snapshot)
        {
            if (!WardenManager::Instance().PublishCheckCatalog(snapshot))
                return false;
            publishedSnapshot = snapshot;
            return true;
        };
    failure = transaction.Finish(WardenModuleCatalog{}, PreflightProfile,
        publisher, diagnostic);
    if (failure != WardenCheckCatalogLoadFailure::None)
    {
        if (failure == WardenCheckCatalogLoadFailure::InvalidRow &&
            diagnostic.validation != CheckCatalogValidation::Valid)
            LogLoadFailure(failure, diagnostic);
        else
            LogLoadFailure(failure);
        return false;
    }

    LogSummary(publishedSnapshot);
    return true;
}
}

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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "TestHarness.h"

#include "WardenEvidence.h"

namespace
{
warden::WardenEvidence Evidence(warden::WardenCheckType type,
    warden::WardenEvidenceClass evidenceClass,
    warden::WardenCheckOutcome outcome)
{
    warden::WardenEvidence evidence;
    evidence.requestId = 7;
    evidence.checkId = 3;
    evidence.checkType = type;
    evidence.evidenceClass = evidenceClass;
    evidence.outcome = outcome;
    return evidence;
}
}

TEST(WardenEvidence_confirmation_requires_a_typed_non_health_anomaly)
{
    auto mismatch = Evidence(warden::WardenCheckType::Mem,
        warden::WardenEvidenceClass::IntegrityInvariant,
        warden::WardenCheckOutcome::Mismatch);
    CHECK(warden::NeedsConfirmation(mismatch));

    mismatch.outcome = warden::WardenCheckOutcome::Unavailable;
    CHECK(warden::NeedsConfirmation(mismatch));
    mismatch.outcome = warden::WardenCheckOutcome::Match;
    CHECK(!warden::NeedsConfirmation(mismatch));
    mismatch.checkId = 0;
    mismatch.outcome = warden::WardenCheckOutcome::Mismatch;
    CHECK(!warden::NeedsConfirmation(mismatch));

    auto timing = Evidence(warden::WardenCheckType::Timing,
        warden::WardenEvidenceClass::ProtocolHealth,
        warden::WardenCheckOutcome::Unstable);
    CHECK(!warden::NeedsConfirmation(timing));
}

TEST(WardenEvidence_unavailable_and_corroboration_never_become_incidents)
{
    auto unavailable = Evidence(warden::WardenCheckType::Mem,
        warden::WardenEvidenceClass::IntegrityInvariant,
        warden::WardenCheckOutcome::Unavailable);
    CHECK(warden::ClassifyConfirmedEvidence(
        warden::WardenEnforcementMode::KickAndBan, unavailable) ==
        warden::WardenConfirmedDisposition::Audit);

    auto corroboration = Evidence(warden::WardenCheckType::Mpq,
        warden::WardenEvidenceClass::Corroboration,
        warden::WardenCheckOutcome::Mismatch);
    CHECK(warden::ClassifyConfirmedEvidence(
        warden::WardenEnforcementMode::KickAndBan, corroboration) ==
        warden::WardenConfirmedDisposition::Audit);
}

TEST(WardenEvidence_actionable_mismatch_respects_observation_mode)
{
    auto actionable = Evidence(warden::WardenCheckType::Mem,
        warden::WardenEvidenceClass::IntegrityInvariant,
        warden::WardenCheckOutcome::Mismatch);
    CHECK(warden::ClassifyConfirmedEvidence(
        warden::WardenEnforcementMode::Observe, actionable) ==
        warden::WardenConfirmedDisposition::Audit);
    CHECK(warden::ClassifyConfirmedEvidence(
        warden::WardenEnforcementMode::Kick, actionable) ==
        warden::WardenConfirmedDisposition::Incident);
}

TEST(WardenEvidence_labels_are_fixed_and_secret_free)
{
    CHECK_STR(warden::ToString(warden::WardenCheckType::Mem), "MEM");
    CHECK_STR(warden::ToString(
        warden::WardenEvidenceClass::IntegrityInvariant),
        "IntegrityInvariant");
    CHECK_STR(warden::ToString(warden::WardenCheckOutcome::Unavailable),
        "Unavailable");
}

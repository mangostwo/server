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

#include "TestHarness.h"

#include "Auth/BigNumber.h"
#include "Auth/HMACSHA1.h"
#include "WardenConfiguration.h"
#include "WardenIncidentStore.h"
#include "WardenManager.h"
#include "WardenProtocol.h"
#include "WorldGatewayAuth.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <memory>
#include <string>
#include <utility>

namespace
{
std::array<uint8, 40> LeadingZeroSessionKey()
{
    std::array<uint8, 40> key{};
    for (size_t index = 0; index < 39; ++index)
        key[index] = static_cast<uint8>(index + 1);
    return key;
}

std::array<uint8, 20> RedirectDigest(BigNumber& sessionKey)
{
    std::array<uint8, 4> const address = {{0xCB, 0x00, 0x71, 0x09}};
    std::array<uint8, 2> const port = {{0x8C, 0x0E}};
    HMACSHA1 hmac(40, sessionKey.AsByteArray(40));
    hmac.UpdateData(address.data(), address.size());
    hmac.UpdateData(port.data(), port.size());
    hmac.Finalize();
    std::array<uint8, 20> digest{};
    std::copy(hmac.GetDigest(), hmac.GetDigest() + digest.size(),
        digest.begin());
    return digest;
}
}

TEST(WardenAdmission_account_projection_indices_are_append_only)
{
    using Field = WorldGatewayAccountField;
    std::array<std::pair<Field, size_t>, 11> const expected =
    {{
        {Field::Id, 0}, {Field::Security, 1}, {Field::SessionKey, 2},
        {Field::LastIp, 3}, {Field::Locked, 4}, {Field::Expansion, 5},
        {Field::MuteTime, 6}, {Field::DbcLocale, 7}, {Field::ClientOS, 8},
        {Field::ClientLocale, 9}, {Field::Count, 10}
    }};
    for (auto const& entry : expected)
    {
        CHECK_EQ(WorldGatewayAccountFieldIndex(entry.first), entry.second);
        CHECK_EQ(static_cast<size_t>(entry.first), entry.second);
    }
}

TEST(WardenAdmission_key_extraction_preserves_identity_move_custody_and_redirect)
{
    std::array<uint8, 40> const expected = LeadingZeroSessionKey();
    BigNumber retained;
    retained.SetBinary(expected.data(), expected.size());

    std::array<uint8, 40> before{};
    uint8 const* first = retained.AsByteArray(40);
    std::copy(first, first + before.size(), before.begin());

    warden::AdmissionData admission = BuildWardenAdmissionData(
        12340, "Win", "enGB", retained);
    REQUIRE(admission.available);
    CHECK_EQ(admission.build, uint32(12340));
    CHECK_STR(admission.platform.c_str(), "Win");
    CHECK_STR(admission.clientLocale.c_str(), "enGB");
    CHECK(admission.sessionKey == expected);

    std::array<uint8, 40> after{};
    uint8 const* second = retained.AsByteArray(40);
    std::copy(second, second + after.size(), after.begin());
    CHECK(before == expected);
    CHECK(after == expected);

    std::array<uint8, 20> const digest = RedirectDigest(retained);
    CHECK_HEX(digest.data(), digest.size(),
        "bfb1038f369a90479c0c05144b374b1b4f5b9f03");

    warden::AdmissionData moved(std::move(admission));
    CHECK(!admission.available);
    CHECK(std::all_of(admission.sessionKey.begin(),
        admission.sessionKey.end(), [](uint8 byte) { return byte == 0; }));
    CHECK(moved.sessionKey == expected);
    moved.Clear();
    CHECK(!moved.available);
    CHECK(std::all_of(moved.sessionKey.begin(), moved.sessionKey.end(),
        [](uint8 byte) { return byte == 0; }));
}

TEST(WardenAdmission_rebases_database_deadlines_without_clock_aliasing)
{
    CHECK_EQ(warden::RebaseIncidentDeadline(130, 100, 1000), uint64(1030));
    CHECK_EQ(warden::RebaseIncidentDeadline(100, 100, 1000), uint64(0));
    CHECK_EQ(warden::RebaseIncidentDeadline(99, 100, 1000), uint64(0));
    CHECK_EQ(warden::RebaseIncidentDeadline(130, 0, 1000), uint64(0));
    uint64 const maximum = std::numeric_limits<uint64>::max();
    CHECK_EQ(warden::RebaseIncidentDeadline(120, 100, maximum - 5), maximum);
}

TEST(WardenAdmission_queue_time_consumes_aggressive_deadline)
{
    warden::WardenConfiguration configuration;
    warden::WardenAdmissionHistory history;
    history.recentIncidentCount = configuration.aggressiveThreshold;
    history.aggressiveUntilServer = 1100;
    history.incidentHistoryLoaded = true;

    CHECK(warden::ShouldUseAggressiveWardenAdmission(
        history, configuration, 1099));
    CHECK(!warden::ShouldUseAggressiveWardenAdmission(
        history, configuration, 1100));
    history.incidentHistoryLoaded = false;
    CHECK(!warden::ShouldUseAggressiveWardenAdmission(
        history, configuration, 1000));
}

TEST(WardenAdmission_observe_or_missing_history_never_selects_aggressive)
{
    warden::WardenAdmissionHistory skippedHistory;
    warden::WardenConfiguration currentEnforcement;
    currentEnforcement.enforcementMode = warden::WardenEnforcementMode::Kick;
    CHECK(!warden::ShouldUseAggressiveWardenAdmission(
        skippedHistory, currentEnforcement, 1000));

    warden::WardenAdmissionHistory carriedHistory;
    carriedHistory.recentIncidentCount = 10;
    carriedHistory.aggressiveUntilServer = 2000;
    carriedHistory.incidentHistoryLoaded = true;
    warden::WardenConfiguration currentObserve;
    currentObserve.enforcementMode = warden::WardenEnforcementMode::Observe;
    CHECK(!warden::ShouldUseAggressiveWardenAdmission(
        carriedHistory, currentObserve, 1000));
}

TEST(WardenAdmission_keeps_history_with_its_attach_time_configuration)
{
    warden::WardenManager manager;
    std::shared_ptr<warden::WardenConfiguration const> attachSnapshot =
        manager.GetConfigurationSnapshot();
    REQUIRE(attachSnapshot != nullptr);

    warden::WardenAdmissionContext context;
    context.configuration = *attachSnapshot;
    context.history.recentIncidentCount =
        context.configuration.aggressiveThreshold;
    context.history.aggressiveUntilServer = 2000;
    context.history.incidentHistoryLoaded = true;

    warden::WardenConfiguration reloaded;
    reloaded.enforcementMode = warden::WardenEnforcementMode::Kick;
    reloaded.aggressiveThreshold = 8;
    reloaded.banThreshold = 12;
    manager.PublishConfiguration(reloaded);
    std::shared_ptr<warden::WardenConfiguration const> reloadedSnapshot =
        manager.GetConfigurationSnapshot();
    REQUIRE(reloadedSnapshot != nullptr);

    CHECK(warden::ShouldUseAggressiveWardenAdmission(context.history,
        context.configuration, 1000));
    CHECK(!warden::ShouldUseAggressiveWardenAdmission(context.history,
        *reloadedSnapshot, 1000));
}

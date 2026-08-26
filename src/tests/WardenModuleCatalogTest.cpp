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

#include "WardenModuleCatalog.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

TEST(WardenCatalog_selects_only_the_exact_12340_windows_build)
{
    warden::WardenModuleCatalog catalog;

    CHECK(catalog.Find(12340, "Win") != nullptr);
    CHECK(catalog.Find(12340, "OSX") == nullptr);
    CHECK(catalog.Find(9999, "Win") == nullptr);
}

TEST(WardenCatalog_enumerates_one_validated_profile)
{
    warden::WardenModuleCatalog catalog;
    std::vector<warden::ModuleProfile const*> const profiles =
        catalog.Profiles();

    REQUIRE(profiles.size() == 1u);
    CHECK_EQ(profiles[0]->build, uint32(12340));
    CHECK_STR(profiles[0]->platform, "Win");
    CHECK(catalog.Validate(*profiles[0]) == warden::ModuleValidation::Valid);
}

TEST(WardenCatalog_requires_all_ten_evidenced_wrath_locales)
{
    warden::WardenModuleCatalog catalog;
    warden::ModuleProfile const* profile = catalog.Find(12340, "Win");
    REQUIRE(profile != nullptr);

    std::vector<std::string> const expected =
        {"enUS", "enGB", "deDE", "esES", "esMX", "frFR", "ruRU",
            "koKR", "zhCN", "zhTW"};
    CHECK(profile->requiredCheckLocales == expected);
}

TEST(WardenCatalog_exact_wrath_module_identity_and_keys_are_custody_pinned)
{
    warden::WardenModuleCatalog catalog;
    warden::ModuleProfile const* profile = catalog.Find(12340, "Win");

    REQUIRE(profile != nullptr);
    CHECK_EQ(profile->module.size, 18756u);
    CHECK_HEX(profile->moduleId.data(), profile->moduleId.size(),
        "79c0768d657977d697e10bad956cced1");
    CHECK_HEX(profile->moduleSha256.data(), profile->moduleSha256.size(),
        "6c68006a2f1fd31e7208204b3f7ceb94a6ce977876e13f2f703e9cd644482289");
    CHECK_HEX(profile->moduleKey.data(), profile->moduleKey.size(),
        "ae25bc51063b77bd363c3efe0fc173f9");
    CHECK_HEX(profile->hashSeed.data(), profile->hashSeed.size(),
        "4d808d2c77d905c41a6380ec08586afe");
    CHECK_HEX(profile->clientKeySeedHash.data(),
        profile->clientKeySeedHash.size(),
        "568c054c781a972a6037a2290c22b52571a06f4e");
    CHECK_HEX(profile->clientKeySeed.data(), profile->clientKeySeed.size(),
        "7f96eefda5b63d20a4df8e00cbf48304");
    CHECK_HEX(profile->serverKeySeed.data(), profile->serverKeySeed.size(),
        "c2b7adedfccca9c2bfb3f85602ba809b");
}

TEST(WardenCatalog_exact_12340_initialization_callbacks_are_custody_pinned)
{
    warden::WardenModuleCatalog catalog;
    warden::ModuleProfile const* profile = catalog.Find(12340, "Win");

    REQUIRE(profile != nullptr);
    CHECK_HEX(profile->initialization.archive.selectors.data(),
        profile->initialization.archive.selectors.size(), "01000200");
    CHECK_EQ(profile->initialization.archive.openRva, uint32(0x00024F80));
    CHECK_EQ(profile->initialization.archive.sizeRva, uint32(0x000218C0));
    CHECK_EQ(profile->initialization.archive.readRva, uint32(0x00022530));
    CHECK_EQ(profile->initialization.archive.closeRva, uint32(0x00022910));
    CHECK_HEX(profile->initialization.lua.prefix.data(),
        profile->initialization.lua.prefix.size(), "040000");
    CHECK_EQ(profile->initialization.lua.callbackRva, uint32(0x00419D40));
    CHECK_EQ(profile->initialization.lua.selector, uint8(1));
    CHECK_HEX(profile->initialization.timing.prefix.data(),
        profile->initialization.timing.prefix.size(), "010100");
    CHECK_EQ(profile->initialization.timing.callbackRva, uint32(0x0046AE20));
    CHECK_EQ(profile->initialization.timing.install, uint8(1));

    warden::ModuleProfile invalid = *profile;
    invalid.initialization.archive.closeRva = 0;
    CHECK(catalog.Validate(invalid) ==
        warden::ModuleValidation::InvalidInitialization);
}

TEST(WardenCatalog_rejects_a_corrupted_module_copy)
{
    warden::WardenModuleCatalog catalog;
    warden::ModuleProfile const* profile = catalog.Find(12340, "Win");

    REQUIRE(profile != nullptr);
    warden::ModuleProfile corrupted = *profile;
    std::vector<uint8> bytes(corrupted.module.data,
        corrupted.module.data + corrupted.module.size);
    bytes[801] ^= 0x80;
    corrupted.module = {bytes.data(), bytes.size()};

    CHECK(catalog.Validate(corrupted) ==
        warden::ModuleValidation::DigestMismatch);
}

TEST(WardenProtocol_admission_move_transfers_then_cleanses_the_source)
{
    warden::AdmissionData source;
    source.build = 12340;
    source.platform = "Win";
    source.clientLocale = "enGB";
    source.sessionKey.fill(0xA5);
    source.available = true;

    warden::AdmissionData moved(std::move(source));

    CHECK_EQ(moved.build, 12340u);
    CHECK_STR(moved.platform, "Win");
    CHECK_STR(moved.clientLocale, "enGB");
    CHECK(std::all_of(moved.sessionKey.begin(), moved.sessionKey.end(),
        [](uint8 value) { return value == 0xA5; }));
    CHECK(moved.available);
    CHECK_EQ(source.build, 0u);
    CHECK(source.platform.empty());
    CHECK(source.clientLocale.empty());
    CHECK(std::all_of(source.sessionKey.begin(), source.sessionKey.end(),
        [](uint8 value) { return value == 0; }));
    CHECK(!source.available);
}

TEST(WardenProtocol_clear_removes_pending_credentials)
{
    warden::AdmissionData admission;
    admission.build = 12340;
    admission.platform = "Win";
    admission.clientLocale = "enGB";
    admission.sessionKey.fill(0x5A);
    admission.available = true;

    admission.Clear();

    CHECK_EQ(admission.build, 0u);
    CHECK(admission.platform.empty());
    CHECK(admission.clientLocale.empty());
    CHECK(std::all_of(admission.sessionKey.begin(), admission.sessionKey.end(),
        [](uint8 value) { return value == 0; }));
    CHECK(!admission.available);
}

TEST(WardenProtocol_failure_names_are_secret_free_fixed_labels)
{
    CHECK_STR(warden::ToString(warden::WardenState::ModuleReady),
        "ModuleReady");
    CHECK_STR(warden::ToString(warden::WardenFailure::HashMismatch),
        "HashMismatch");
}

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

#include "WardenCheckCatalogLoader.h"
#include "WardenCheckCatalog.h"
#include "WardenCheckFixtures.h"

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace
{
warden::CheckCatalogValidation AddOne(
    warden::WardenCheckRowInput const& row)
{
    warden::WardenCheckCatalogBuilder builder;
    warden::WardenCheckDiagnostic diagnostic;
    return builder.Add(row, diagnostic);
}

warden::CheckCatalogValidation BuildRows(
    std::vector<warden::WardenCheckRowInput> const& rows,
    warden::WardenCheckCatalog& catalog)
{
    warden::WardenCheckCatalogBuilder builder;
    warden::WardenCheckDiagnostic diagnostic;
    for (warden::WardenCheckRowInput const& row : rows)
    {
        warden::CheckCatalogValidation const validation =
            builder.Add(row, diagnostic);
        if (validation != warden::CheckCatalogValidation::Valid)
            return validation;
    }
    return builder.Build(catalog, diagnostic);
}

warden::CheckCatalogValidation BuildRows(
    std::vector<warden::WardenCheckRowInput> const& rows)
{
    warden::WardenCheckCatalog catalog;
    return BuildRows(rows, catalog);
}

std::vector<warden::WardenCheckRowInput> FirstProfileRows()
{
    std::vector<warden::WardenCheckRowInput> rows =
        warden::test::InitialWardenRows();
    rows.resize(4);
    return rows;
}

warden::WardenCheckCatalogLoadFailure StageSnapshot(
    warden::WardenCheckCatalogLoadTransaction& transaction,
    std::vector<warden::WardenCheckRowInput> const& rows,
    uint64 sourceCount, warden::WardenCheckDiagnostic& diagnostic)
{
    warden::WardenCheckCatalogLoadFailure failure =
        transaction.Begin(sourceCount);
    if (failure != warden::WardenCheckCatalogLoadFailure::None)
        return failure;
    for (warden::WardenCheckRowInput const& row : rows)
    {
        failure = transaction.ObserveSourceCount(sourceCount);
        if (failure != warden::WardenCheckCatalogLoadFailure::None)
            return failure;
        failure = transaction.Add(row, diagnostic);
        if (failure != warden::WardenCheckCatalogLoadFailure::None)
            return failure;
    }
    return warden::WardenCheckCatalogLoadFailure::None;
}

warden::WardenCatalogPreflight AcceptPreflight()
{
    return [](warden::WardenCheckProfile const&) { return true; };
}
}

TEST(WardenCheckCatalog_type_evidence_class_contract_is_canonical)
{
    using warden::WardenCheckType;
    using warden::WardenEvidenceClass;

    CHECK(warden::IsLegalWardenEvidenceClass(WardenCheckType::Timing,
        WardenEvidenceClass::ProtocolHealth));
    CHECK(warden::IsLegalWardenEvidenceClass(WardenCheckType::Mpq,
        WardenEvidenceClass::IntegrityInvariant));
    CHECK(warden::IsLegalWardenEvidenceClass(WardenCheckType::Mpq,
        WardenEvidenceClass::Corroboration));
    CHECK(warden::IsLegalWardenEvidenceClass(WardenCheckType::Lua,
        WardenEvidenceClass::Corroboration));
    CHECK(warden::IsLegalWardenEvidenceClass(WardenCheckType::Mem,
        WardenEvidenceClass::IntegrityInvariant));
    CHECK(warden::IsLegalWardenEvidenceClass(WardenCheckType::Mem,
        WardenEvidenceClass::ThreatSignature));
    CHECK(warden::IsLegalWardenEvidenceClass(WardenCheckType::Mem,
        WardenEvidenceClass::Corroboration));

    CHECK(!warden::IsLegalWardenEvidenceClass(WardenCheckType::Timing,
        WardenEvidenceClass::IntegrityInvariant));
    CHECK(!warden::IsLegalWardenEvidenceClass(WardenCheckType::Mpq,
        WardenEvidenceClass::ThreatSignature));
    CHECK(!warden::IsLegalWardenEvidenceClass(WardenCheckType::Lua,
        WardenEvidenceClass::IntegrityInvariant));
    CHECK(!warden::IsLegalWardenEvidenceClass(WardenCheckType::Mem,
        WardenEvidenceClass::ProtocolHealth));
    CHECK(!warden::IsLegalWardenEvidenceClass(
        static_cast<WardenCheckType>(0xFF),
        WardenEvidenceClass::Corroboration));
    CHECK(!warden::IsLegalWardenEvidenceClass(WardenCheckType::Mem,
        static_cast<WardenEvidenceClass>(0xFF)));
}

TEST(WardenCheckCatalog_decodes_and_selects_ten_exact_12340_profiles)
{
    warden::WardenCheckCatalogBuilder builder;
    warden::WardenCheckDiagnostic diagnostic;
    for (warden::WardenCheckRowInput const& row :
        warden::test::InitialWardenRows())
    {
        REQUIRE(builder.Add(row, diagnostic) ==
            warden::CheckCatalogValidation::Valid);
    }

    warden::WardenCheckCatalog catalog;
    REQUIRE(builder.Build(catalog, diagnostic) ==
        warden::CheckCatalogValidation::Valid);
    CHECK_EQ(catalog.TotalRows(), uint32(40));
    CHECK_EQ(catalog.EnabledRows(), uint32(40));
    CHECK_EQ(catalog.Profiles().size(), size_t(10));

    struct ExpectedProfile
    {
        char const* locale;
        char const* mpqSha1;
        char const* luaText;
    };
    std::array<ExpectedProfile, 10> const expectedProfiles =
    {{
        {"enUS", "8c7ced99f8dddd48296551efe05a2cf27b26f818",
            "4f6b6179"},
        {"enGB", "8c7ced99f8dddd48296551efe05a2cf27b26f818",
            "4f6b6179"},
        {"deDE", "0b4d01bdeb4f47de030b57d81506093eb887ee0b",
            "4f4b"},
        {"esES", "20ec8371ec168b4723af6de3afe81d46843726f4",
            "41636570746172"},
        {"esMX", "0e39f4af09e3cf08925d41e61fbac8ee16478fc9",
            "41636570746172"},
        {"frFR", "e6f5a0c5c63056f63097420ae29b47aca2e4d496",
            "4f4b"},
        {"ruRU", "329bf203079002d36e05ebf54bd5746aa37e47c8",
            "d09ed09a"},
        {"koKR", "39bcde7e67f7da4a366d15007dbaf3d438338e00",
            "ed9995ec9db8"},
        {"zhCN", "53538853e7026786eb30fcb247d7e8179a3caaf8",
            "e7a1aee5ae9a"},
        {"zhTW", "ed14f2c71688b1de9660f9ce04a62d63a9eb297a",
            "e7a2bae5ae9a"}
    }};

    for (ExpectedProfile const& expected : expectedProfiles)
    {
        warden::WardenCheckProfile const* profile =
            catalog.Find(12340, "Win", expected.locale);
        REQUIRE(profile != nullptr);
        REQUIRE(profile->checks.size() == 4u);
        CHECK(profile->hasActionableChecks);
        CHECK_EQ(profile->totalRows, uint32(4));
        CHECK_EQ(warden::GetWardenCheckId(profile->checks[0]),
            uint32(65536));
        CHECK(warden::GetWardenCheckType(profile->checks[0]) ==
            warden::WardenCheckType::Timing);
        CHECK(!warden::IsConfirmationEligible(profile->checks[0]));

        REQUIRE(warden::GetWardenCheckId(profile->checks[1]) == 1u);
        CHECK(warden::IsConfirmationEligible(profile->checks[1]));
        warden::MpqCheckProfile const& mpq =
            std::get<warden::MpqCheckProfile>(profile->checks[1].payload);
        CHECK_STR(mpq.path.c_str(), "DBFilesClient\\AreaTable.dbc");
        CHECK_HEX(mpq.expectedSha1.data(), mpq.expectedSha1.size(),
            expected.mpqSha1);

        REQUIRE(warden::GetWardenCheckId(profile->checks[2]) == 2u);
        warden::LuaCheckProfile const& lua =
            std::get<warden::LuaCheckProfile>(profile->checks[2].payload);
        CHECK_STR(lua.query.c_str(), "OKAY");
        CHECK_HEX(reinterpret_cast<uint8 const*>(lua.expectedText.data()),
            lua.expectedText.size(), expected.luaText);

        REQUIRE(warden::GetWardenCheckId(profile->checks[3]) == 3u);
        warden::MemCheckProfile const& mem =
            std::get<warden::MemCheckProfile>(profile->checks[3].payload);
        CHECK(mem.moduleName.empty());
        CHECK_EQ(mem.addressOrRva, uint32(0x007DA8C0));
        CHECK_HEX(mem.expectedBytes.data(), mem.expectedBytes.size(),
            "b9601ad300e8769df9ffe851fbffff688c29af0068d816af00b8b3120000e82d"
            "fdffffa3441ad300");
    }

    CHECK(warden::IsActionableEvidenceClass(
        warden::WardenEvidenceClass::IntegrityInvariant));
    CHECK(warden::IsActionableEvidenceClass(
        warden::WardenEvidenceClass::ThreatSignature));
    CHECK(!warden::IsActionableEvidenceClass(
        warden::WardenEvidenceClass::ProtocolHealth));
    CHECK(!warden::IsActionableEvidenceClass(
        warden::WardenEvidenceClass::Corroboration));

    CHECK(catalog.Find(12340, "Win", "itIT") == nullptr);
    CHECK(catalog.Find(12340, "OSX", "enUS") == nullptr);
}

TEST(WardenCheckCatalog_preserves_embedded_zero_bytes)
{
    std::vector<warden::WardenCheckRowInput> rows = FirstProfileRows();
    rows.resize(2);
    rows[1] = warden::test::MakeRow(12340, "656E5553", 9001,
        warden::WardenCheckType::Mem, 20,
        warden::WardenEvidenceClass::IntegrityInvariant);
    rows[1].address = 0x00400000;
    rows[1].length = 3;
    rows[1].expectedHex = "A100B2";

    warden::WardenCheckCatalog catalog;
    REQUIRE(BuildRows(rows, catalog) ==
        warden::CheckCatalogValidation::Valid);
    warden::WardenCheckProfile const* profile =
        catalog.Find(12340, "Win", "enUS");
    REQUIRE(profile != nullptr);
    REQUIRE(profile->checks.size() == 2u);
    warden::MemCheckProfile const& decoded =
        std::get<warden::MemCheckProfile>(profile->checks[1].payload);
    REQUIRE(decoded.expectedBytes.size() == 3u);
    CHECK_HEX(decoded.expectedBytes.data(), decoded.expectedBytes.size(),
        "a100b2");
}

TEST(WardenCheckCatalog_rejects_noncanonical_hex)
{
    warden::WardenCheckRowInput row = FirstProfileRows()[3];
    row.expectedHex = "ABC";
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidHex);
    row.expectedHex = "GG";
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidHex);
    row.expectedHex = "AA BB";
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidHex);
}

TEST(WardenCheckCatalog_rejects_invalid_profile_identity)
{
    warden::WardenCheckRowInput row = FirstProfileRows()[3];
    row.build = 0;
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidBuild);
    row.build = 65536;
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidBuild);

    row = FirstProfileRows()[3];
    row.platformHex.clear();
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidPlatform);
    row.platformHex = "4142434445";
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidPlatform);
    row.platformHex = "5700696E";
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidPlatform);
    row.platformHex = "57696E20";
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidPlatform);

    row = FirstProfileRows()[3];
    row.localeHex = "656E55";
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidLocale);
    row.localeHex = "656E555300";
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidLocale);
    row.localeHex = "65005553";
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidLocale);
    row.localeHex = "656E2053";
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidLocale);
}

TEST(WardenCheckCatalog_rejects_invalid_scalar_fields_before_narrowing)
{
    warden::WardenCheckRowInput row = FirstProfileRows()[3];
    row.checkId = 0;
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidId);
    row = FirstProfileRows()[3];
    row.enabled = 2;
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidEnabled);
    row.enabled = 256;
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidEnabled);
    row = FirstProfileRows()[3];
    row.sortOrder = 65536;
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidSortOrder);
    row = FirstProfileRows()[3];
    row.type = 1;
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidType);
    row.type = 0x1F3;
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidType);
    row = FirstProfileRows()[3];
    row.evidenceClass = 4;
    CHECK(AddOne(row) ==
        warden::CheckCatalogValidation::InvalidEvidenceClass);
    row.evidenceClass = 259;
    CHECK(AddOne(row) ==
        warden::CheckCatalogValidation::InvalidEvidenceClass);
}

TEST(WardenCheckCatalog_rejects_every_phase_two_type_even_when_disabled)
{
    std::array<uint32, 5> const phaseTwoTypes =
        {{0x71, 0x7E, 0xB2, 0xBF, 0xD9}};
    for (uint32 type : phaseTwoTypes)
    {
        warden::WardenCheckRowInput row = FirstProfileRows()[3];
        row.type = type;
        row.enabled = 0;
        CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidType);
    }
}

TEST(WardenCheckCatalog_enforces_timing_contract_and_cardinality)
{
    warden::WardenCheckRowInput row = FirstProfileRows()[0];
    row.moduleHex = "41";
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidUnusedField);
    row = FirstProfileRows()[0];
    row.requestHex = "41";
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidUnusedField);
    row = FirstProfileRows()[0];
    row.expectedHex = "41";
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidUnusedField);
    row = FirstProfileRows()[0];
    row.address = 1;
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidUnusedField);
    row = FirstProfileRows()[0];
    row.length = 1;
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidUnusedField);
    row = FirstProfileRows()[0];
    row.evidenceClass = 1;
    CHECK(AddOne(row) ==
        warden::CheckCatalogValidation::IllegalTypeEvidenceClass);

    std::vector<warden::WardenCheckRowInput> rows = FirstProfileRows();
    rows[0].enabled = 0;
    CHECK(BuildRows(rows) == warden::CheckCatalogValidation::DisabledTiming);
    rows = FirstProfileRows();
    warden::WardenCheckRowInput secondTiming = rows[0];
    secondTiming.checkId = 65537;
    secondTiming.sortOrder = 11;
    rows.push_back(secondTiming);
    CHECK(BuildRows(rows) == warden::CheckCatalogValidation::MultipleTiming);
}

TEST(WardenCheckCatalog_enforces_mpq_contract)
{
    warden::WardenCheckRowInput row = FirstProfileRows()[1];
    row.requestHex.clear();
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidPath);
    row.requestHex = "410042";
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidPath);
    row.requestHex.assign(512, '4');
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidPath);
    row = FirstProfileRows()[1];
    row.expectedHex.assign(38, 'A');
    CHECK(AddOne(row) ==
        warden::CheckCatalogValidation::InvalidExpectedBytes);
    row = FirstProfileRows()[1];
    row.moduleHex = "41";
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidUnusedField);
    row = FirstProfileRows()[1];
    row.address = 1;
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidUnusedField);
    row = FirstProfileRows()[1];
    row.length = 1;
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidUnusedField);
}

TEST(WardenCheckCatalog_enforces_lua_contract)
{
    warden::WardenCheckRowInput row = FirstProfileRows()[2];
    row.requestHex.clear();
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidQuery);
    row.requestHex = "410042";
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidQuery);
    row.requestHex.assign(512, '5');
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidQuery);
    row = FirstProfileRows()[2];
    row.expectedHex.clear();
    CHECK(AddOne(row) ==
        warden::CheckCatalogValidation::InvalidExpectedText);
    row.expectedHex = "410042";
    CHECK(AddOne(row) ==
        warden::CheckCatalogValidation::InvalidExpectedText);
    row.expectedHex.assign(130, '6');
    CHECK(AddOne(row) ==
        warden::CheckCatalogValidation::InvalidExpectedText);
    row = FirstProfileRows()[2];
    row.moduleHex = "41";
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidUnusedField);
    row = FirstProfileRows()[2];
    row.address = 1;
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidUnusedField);
    row = FirstProfileRows()[2];
    row.length = 1;
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidUnusedField);
}

TEST(WardenCheckCatalog_enforces_mem_contract)
{
    warden::WardenCheckRowInput row = FirstProfileRows()[3];
    row.address = 0;
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidAddress);
    row = FirstProfileRows()[3];
    row.moduleHex = "410042";
    CHECK(AddOne(row) ==
        warden::CheckCatalogValidation::InvalidModuleName);
    row.moduleHex.assign(512, '4');
    CHECK(AddOne(row) ==
        warden::CheckCatalogValidation::InvalidModuleName);
    row = FirstProfileRows()[3];
    row.length = 0;
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidLength);
    row.length = 256;
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidLength);
    row = FirstProfileRows()[3];
    row.requestHex = "41";
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidUnusedField);
    row = FirstProfileRows()[3];
    row.expectedHex.resize(row.expectedHex.size() - 2);
    CHECK(AddOne(row) ==
        warden::CheckCatalogValidation::InvalidExpectedBytes);
    row = FirstProfileRows()[3];
    row.address = 0xFFFFFFF0u;
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidAddress);
}

TEST(WardenCheckCatalog_pins_legal_type_evidence_class_pairs)
{
    warden::WardenCheckRowInput timing = FirstProfileRows()[0];
    for (uint32 value = 0; value <= 3; ++value)
    {
        timing.evidenceClass = value;
        CHECK(AddOne(timing) == (value == 0 ?
            warden::CheckCatalogValidation::Valid :
            warden::CheckCatalogValidation::IllegalTypeEvidenceClass));
    }

    warden::WardenCheckRowInput mpq = FirstProfileRows()[1];
    for (uint32 value = 0; value <= 3; ++value)
    {
        mpq.evidenceClass = value;
        bool const legal = value == 1 || value == 3;
        CHECK(AddOne(mpq) == (legal ? warden::CheckCatalogValidation::Valid :
            warden::CheckCatalogValidation::IllegalTypeEvidenceClass));
    }

    warden::WardenCheckRowInput lua = FirstProfileRows()[2];
    for (uint32 value = 0; value <= 3; ++value)
    {
        lua.evidenceClass = value;
        CHECK(AddOne(lua) == (value == 3 ?
            warden::CheckCatalogValidation::Valid :
            warden::CheckCatalogValidation::IllegalTypeEvidenceClass));
    }

    warden::WardenCheckRowInput mem = FirstProfileRows()[3];
    for (uint32 value = 0; value <= 3; ++value)
    {
        mem.evidenceClass = value;
        CHECK(AddOne(mem) == (value == 0 ?
            warden::CheckCatalogValidation::IllegalTypeEvidenceClass :
            warden::CheckCatalogValidation::Valid));
    }
}

TEST(WardenCheckCatalog_rejects_duplicate_ids_and_sort_orders_when_disabled)
{
    std::vector<warden::WardenCheckRowInput> rows = FirstProfileRows();
    rows[2].checkId = rows[1].checkId;
    CHECK(BuildRows(rows) == warden::CheckCatalogValidation::DuplicateId);
    rows = FirstProfileRows();
    rows[2].sortOrder = rows[1].sortOrder;
    CHECK(BuildRows(rows) ==
        warden::CheckCatalogValidation::DuplicateSortOrder);
    rows = FirstProfileRows();
    rows[2].enabled = 0;
    rows[2].checkId = rows[1].checkId;
    CHECK(BuildRows(rows) == warden::CheckCatalogValidation::DuplicateId);
    rows = FirstProfileRows();
    rows[2].enabled = 0;
    rows[2].sortOrder = rows[1].sortOrder;
    CHECK(BuildRows(rows) ==
        warden::CheckCatalogValidation::DuplicateSortOrder);
}

TEST(WardenCheckCatalog_rejects_conflicting_expectations_for_one_request)
{
    std::vector<warden::WardenCheckRowInput> rows = FirstProfileRows();
    warden::WardenCheckRowInput duplicate = rows[1];
    duplicate.checkId = 9001;
    duplicate.sortOrder = 21;
    duplicate.expectedHex[0] = duplicate.expectedHex[0] == '0' ? '1' : '0';
    rows.push_back(duplicate);
    CHECK(BuildRows(rows) ==
        warden::CheckCatalogValidation::ConflictingRequestExpectation);

    rows = FirstProfileRows();
    duplicate = rows[2];
    duplicate.checkId = 9002;
    duplicate.sortOrder = 31;
    duplicate.expectedHex[0] = duplicate.expectedHex[0] == '0' ? '1' : '0';
    rows.push_back(duplicate);
    CHECK(BuildRows(rows) ==
        warden::CheckCatalogValidation::ConflictingRequestExpectation);

    rows = FirstProfileRows();
    duplicate = rows[3];
    duplicate.checkId = 9003;
    duplicate.sortOrder = 41;
    duplicate.expectedHex[0] = duplicate.expectedHex[0] == '0' ? '1' : '0';
    rows.push_back(duplicate);
    CHECK(BuildRows(rows) ==
        warden::CheckCatalogValidation::ConflictingRequestExpectation);

    rows = FirstProfileRows();
    duplicate = rows[3];
    duplicate.checkId = 9004;
    duplicate.sortOrder = 41;
    duplicate.length /= 2;
    duplicate.expectedHex.resize(duplicate.length * 2);
    rows.push_back(duplicate);
    CHECK(BuildRows(rows) == warden::CheckCatalogValidation::Valid);

    rows = FirstProfileRows();
    duplicate = rows[3];
    duplicate.checkId = 9005;
    duplicate.sortOrder = 41;
    duplicate.address += 2;
    duplicate.length = 4;
    duplicate.expectedHex = "FFFFFFFF";
    rows.push_back(duplicate);
    CHECK(BuildRows(rows) ==
        warden::CheckCatalogValidation::ConflictingRequestExpectation);

    rows = FirstProfileRows();
    duplicate = rows[3];
    duplicate.checkId = 9008;
    duplicate.sortOrder = 41;
    duplicate.address -= 2;
    duplicate.length = 4;
    duplicate.expectedHex = "0000" + rows[3].expectedHex.substr(0, 4);
    duplicate.expectedHex[4] =
        duplicate.expectedHex[4] == '0' ? '1' : '0';
    rows.push_back(duplicate);
    CHECK(BuildRows(rows) ==
        warden::CheckCatalogValidation::ConflictingRequestExpectation);

    rows = FirstProfileRows();
    duplicate = rows[3];
    duplicate.checkId = 9006;
    duplicate.sortOrder = 41;
    duplicate.address += 2;
    duplicate.length = 4;
    duplicate.expectedHex = rows[3].expectedHex.substr(4, 8);
    rows.push_back(duplicate);
    CHECK(BuildRows(rows) == warden::CheckCatalogValidation::Valid);

    rows = FirstProfileRows();
    warden::WardenCheckRowInput moduleName = rows[3];
    moduleName.checkId = 9009;
    moduleName.sortOrder = 41;
    moduleName.moduleHex = "576F772E657865";
    rows.push_back(moduleName);
    duplicate = moduleName;
    duplicate.checkId = 9010;
    duplicate.sortOrder = 42;
    duplicate.moduleHex = "776F772E657865";
    duplicate.expectedHex[0] =
        duplicate.expectedHex[0] == '0' ? '1' : '0';
    rows.push_back(duplicate);
    CHECK(BuildRows(rows) ==
        warden::CheckCatalogValidation::ConflictingRequestExpectation);

    rows = FirstProfileRows();
    duplicate = rows[1];
    duplicate.checkId = 9007;
    duplicate.sortOrder = 21;
    duplicate.enabled = 0;
    duplicate.expectedHex[0] = duplicate.expectedHex[0] == '0' ? '1' : '0';
    rows.push_back(duplicate);
    CHECK(BuildRows(rows) == warden::CheckCatalogValidation::Valid);
}

TEST(WardenCheckCatalog_enforces_complete_profiles_and_atomic_build)
{
    std::vector<warden::WardenCheckRowInput> rows;
    CHECK(BuildRows(rows) == warden::CheckCatalogValidation::EmptyCatalog);
    rows = FirstProfileRows();
    rows.erase(rows.begin());
    CHECK(BuildRows(rows) == warden::CheckCatalogValidation::MissingTiming);
    rows = FirstProfileRows();
    rows[0].enabled = 0;
    CHECK(BuildRows(rows) == warden::CheckCatalogValidation::DisabledTiming);
    rows = FirstProfileRows();
    for (size_t index = 1; index < rows.size(); ++index)
        rows[index].enabled = 0;
    CHECK(BuildRows(rows) ==
        warden::CheckCatalogValidation::MissingNonHealth);

    warden::WardenCheckRowInput malformed = FirstProfileRows()[3];
    malformed.enabled = 0;
    malformed.address = 0;
    CHECK(AddOne(malformed) ==
        warden::CheckCatalogValidation::InvalidAddress);

    rows = FirstProfileRows();
    rows.resize(3);
    rows[1].enabled = 0;
    warden::WardenCheckCatalog observationOnly;
    REQUIRE(BuildRows(rows, observationOnly) ==
        warden::CheckCatalogValidation::Valid);
    warden::WardenCheckProfile const* profile =
        observationOnly.Find(12340, "Win", "enUS");
    REQUIRE(profile != nullptr);
    CHECK(!profile->hasActionableChecks);
    CHECK_EQ(profile->totalRows, uint32(3));
    CHECK_EQ(profile->checks.size(), size_t(2));
    CHECK_EQ(observationOnly.EnabledRows(), uint32(2));

    warden::WardenCheckCatalog unchanged =
        warden::test::BuildInitialWardenCatalog();
    REQUIRE(unchanged.TotalRows() == 40u);
    rows = FirstProfileRows();
    rows[2].checkId = rows[1].checkId;
    CHECK(BuildRows(rows, unchanged) ==
        warden::CheckCatalogValidation::DuplicateId);
    CHECK_EQ(unchanged.TotalRows(), uint32(40));
    CHECK(unchanged.Find(12340, "Win", "ruRU") != nullptr);
}

TEST(WardenCheckCatalogLoader_rejects_invalid_snapshot_counts_before_rows)
{
    warden::WardenCheckCatalogLoadTransaction transaction;
    CHECK(transaction.Begin(0) ==
        warden::WardenCheckCatalogLoadFailure::EmptyCatalogue);

    warden::WardenCheckCatalogLoadTransaction overflow;
    CHECK(overflow.Begin(uint64(std::numeric_limits<uint32>::max()) + 1) ==
        warden::WardenCheckCatalogLoadFailure::SourceCountOverflow);

    std::vector<warden::WardenCheckRowInput> const rows =
        warden::test::InitialWardenRows();
    warden::WardenCheckDiagnostic diagnostic;
    warden::WardenCheckCatalogLoadTransaction unobserved;
    REQUIRE(unobserved.Begin(1) ==
        warden::WardenCheckCatalogLoadFailure::None);
    CHECK(unobserved.Add(rows[0], diagnostic) ==
        warden::WardenCheckCatalogLoadFailure::CatalogueQueryFailed);

    warden::WardenCheckCatalogLoadTransaction inconsistent;
    REQUIRE(inconsistent.Begin(rows.size()) ==
        warden::WardenCheckCatalogLoadFailure::None);
    REQUIRE(inconsistent.ObserveSourceCount(rows.size()) ==
        warden::WardenCheckCatalogLoadFailure::None);
    REQUIRE(inconsistent.Add(rows[0], diagnostic) ==
        warden::WardenCheckCatalogLoadFailure::None);
    CHECK(inconsistent.ObserveSourceCount(rows.size() - 1) ==
        warden::WardenCheckCatalogLoadFailure::SourceCountInconsistent);
}

TEST(WardenCheckCatalogLoader_rejects_consumed_row_count_mismatch)
{
    std::vector<warden::WardenCheckRowInput> const rows =
        warden::test::InitialWardenRows();
    warden::WardenCheckDiagnostic diagnostic;
    warden::WardenCheckCatalogLoadTransaction transaction;
    REQUIRE(StageSnapshot(transaction, rows, rows.size() + 1, diagnostic) ==
        warden::WardenCheckCatalogLoadFailure::None);

    uint32 publicationCalls = 0;
    warden::WardenCatalogPublisher publisher =
        [&publicationCalls](
            std::shared_ptr<warden::WardenCheckCatalog const> const&)
        {
            ++publicationCalls;
            return true;
        };
    CHECK(transaction.Finish(warden::WardenModuleCatalog{},
        AcceptPreflight(), publisher, diagnostic) ==
        warden::WardenCheckCatalogLoadFailure::SourceCountMismatch);
    CHECK_EQ(publicationCalls, uint32(0));
}

TEST(WardenCheckCatalogLoader_requires_bidirectional_exact_profile_coverage)
{
    std::vector<warden::WardenCheckRowInput> rows =
        warden::test::InitialWardenRows();
    rows.resize(rows.size() - 4);
    warden::WardenCheckDiagnostic diagnostic;
    warden::WardenCheckCatalogLoadTransaction missingProfile;
    REQUIRE(StageSnapshot(missingProfile, rows, rows.size(), diagnostic) ==
        warden::WardenCheckCatalogLoadFailure::None);
    uint32 publicationCalls = 0;
    warden::WardenCatalogPublisher publisher =
        [&publicationCalls](
            std::shared_ptr<warden::WardenCheckCatalog const> const&)
        {
            ++publicationCalls;
            return true;
        };
    CHECK(missingProfile.Finish(warden::WardenModuleCatalog{},
        AcceptPreflight(), publisher, diagnostic) ==
        warden::WardenCheckCatalogLoadFailure::ModuleWithoutProfile);
    CHECK_EQ(publicationCalls, uint32(0));

    rows = warden::test::InitialWardenRows();
    std::vector<warden::WardenCheckRowInput> extra = FirstProfileRows();
    for (warden::WardenCheckRowInput& row : extra)
        row.localeHex = "69744954";
    rows.insert(rows.end(), extra.begin(), extra.end());
    warden::WardenCheckCatalogLoadTransaction unexpectedProfile;
    REQUIRE(StageSnapshot(unexpectedProfile, rows, rows.size(), diagnostic) ==
        warden::WardenCheckCatalogLoadFailure::None);
    CHECK(unexpectedProfile.Finish(warden::WardenModuleCatalog{},
        AcceptPreflight(), publisher, diagnostic) ==
        warden::WardenCheckCatalogLoadFailure::ProfileWithoutModule);
    CHECK_EQ(publicationCalls, uint32(0));
}

TEST(WardenCheckCatalogLoader_validates_disabled_rows_before_publication)
{
    std::vector<warden::WardenCheckRowInput> rows =
        warden::test::InitialWardenRows();
    warden::WardenCheckRowInput unsupported = rows[3];
    unsupported.checkId = 9000;
    unsupported.sortOrder = 41;
    unsupported.type = 0x71;
    unsupported.enabled = 0;
    rows.push_back(unsupported);

    warden::WardenCheckDiagnostic diagnostic;
    warden::WardenCheckCatalogLoadTransaction transaction;
    CHECK(StageSnapshot(transaction, rows, rows.size(), diagnostic) ==
        warden::WardenCheckCatalogLoadFailure::InvalidRow);
    CHECK(diagnostic.validation ==
        warden::CheckCatalogValidation::InvalidType);
    uint32 publicationCalls = 0;
    warden::WardenCatalogPublisher publisher =
        [&publicationCalls](
            std::shared_ptr<warden::WardenCheckCatalog const> const&)
        {
            ++publicationCalls;
            return true;
        };
    CHECK(transaction.Finish(warden::WardenModuleCatalog{},
        AcceptPreflight(), publisher, diagnostic) ==
        warden::WardenCheckCatalogLoadFailure::InvalidRow);
    CHECK_EQ(publicationCalls, uint32(0));
}

TEST(WardenCheckCatalogLoader_publishes_only_after_complete_preflight)
{
    std::vector<warden::WardenCheckRowInput> const rows =
        warden::test::InitialWardenRows();
    warden::WardenCheckDiagnostic diagnostic;
    warden::WardenCheckCatalogLoadTransaction rejected;
    REQUIRE(StageSnapshot(rejected, rows, rows.size(), diagnostic) ==
        warden::WardenCheckCatalogLoadFailure::None);
    uint32 publicationCalls = 0;
    warden::WardenCatalogPublisher publisher =
        [&publicationCalls](
            std::shared_ptr<warden::WardenCheckCatalog const> const&)
        {
            ++publicationCalls;
            return true;
        };
    CHECK(rejected.Finish(warden::WardenModuleCatalog{},
        [](warden::WardenCheckProfile const&) { return false; }, publisher,
        diagnostic) == warden::WardenCheckCatalogLoadFailure::InvalidPlan);
    CHECK_EQ(publicationCalls, uint32(0));

    warden::WardenCheckCatalogLoadTransaction accepted;
    REQUIRE(StageSnapshot(accepted, rows, rows.size(), diagnostic) ==
        warden::WardenCheckCatalogLoadFailure::None);
    uint32 preflightCalls = 0;
    warden::WardenCatalogPreflight preflight =
        [&preflightCalls](warden::WardenCheckProfile const&)
        {
            ++preflightCalls;
            return true;
        };
    warden::WardenCatalogPublisher verifyingPublisher =
        [&publicationCalls](
            std::shared_ptr<warden::WardenCheckCatalog const> const& snapshot)
        {
            ++publicationCalls;
            CHECK_EQ(snapshot->TotalRows(), uint32(40));
            CHECK_EQ(snapshot->Profiles().size(), size_t(10));
            return true;
        };
    CHECK(accepted.Finish(warden::WardenModuleCatalog{}, preflight,
        verifyingPublisher, diagnostic) ==
        warden::WardenCheckCatalogLoadFailure::None);
    CHECK_EQ(preflightCalls, uint32(10));
    CHECK_EQ(publicationCalls, uint32(1));
}

TEST(WardenCheckCatalog_exposes_stable_validation_names)
{
    CHECK_STR(warden::ToString(warden::CheckCatalogValidation::Valid),
        "Valid");
    CHECK_STR(warden::ToString(
        warden::CheckCatalogValidation::IllegalTypeEvidenceClass),
        "IllegalTypeEvidenceClass");
    CHECK_STR(warden::ToString(
        warden::CheckCatalogValidation::MissingNonHealth),
        "MissingNonHealth");
    CHECK_STR(warden::ToString(
        warden::CheckCatalogValidation::ConflictingRequestExpectation),
        "ConflictingRequestExpectation");
}

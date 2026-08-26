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

#ifndef MANGOS_TEST_WARDEN_CHECK_FIXTURES_H
#define MANGOS_TEST_WARDEN_CHECK_FIXTURES_H

#include "WardenCheckCatalog.h"

#include <string>
#include <vector>

namespace warden
{
namespace test
{
inline WardenCheckRowInput MakeRow(uint32 build,
    std::string const& localeHex, uint32 checkId, WardenCheckType type,
    uint32 sortOrder, WardenEvidenceClass evidenceClass)
{
    WardenCheckRowInput row;
    row.build = build;
    row.platformHex = "57696E";
    row.localeHex = localeHex;
    row.checkId = checkId;
    row.type = static_cast<uint32>(type);
    row.enabled = 1;
    row.sortOrder = sortOrder;
    row.evidenceClass = static_cast<uint32>(evidenceClass);
    return row;
}

inline void AppendInitialProfile(std::vector<WardenCheckRowInput>& rows,
    uint32 build, std::string const& localeHex,
    std::string const& mpqExpectedHex, std::string const& luaExpectedHex)
{
    rows.push_back(MakeRow(build, localeHex, 65536,
        WardenCheckType::Timing, 10, WardenEvidenceClass::ProtocolHealth));

    WardenCheckRowInput mpq = MakeRow(build, localeHex, 1,
        WardenCheckType::Mpq, 20, WardenEvidenceClass::Corroboration);
    mpq.requestHex =
        "444246696C6573436C69656E745C417265615461626C652E646263";
    mpq.expectedHex = mpqExpectedHex;
    rows.push_back(mpq);

    WardenCheckRowInput lua = MakeRow(build, localeHex, 2,
        WardenCheckType::Lua, 30, WardenEvidenceClass::Corroboration);
    lua.requestHex = "4F4B4159";
    lua.expectedHex = luaExpectedHex;
    rows.push_back(lua);

    WardenCheckRowInput bootstrap = MakeRow(build, localeHex, 3,
        WardenCheckType::Mem, 40, WardenEvidenceClass::IntegrityInvariant);
    // Empty module selects an absolute process address and cannot be bypassed
    // by renaming the exact executable.
    bootstrap.moduleHex.clear();
    bootstrap.address = 0x007DA8C0;
    bootstrap.length = 40;
    bootstrap.expectedHex =
        "B9601AD300E8769DF9FFE851FBFFFF688C29AF0068D816AF00B8B3120000E82D"
        "FDFFFFA3441AD300";
    rows.push_back(bootstrap);
}

/** Exact database rows intended for all ten evidenced Wrath locale profiles. */
inline std::vector<WardenCheckRowInput> InitialWardenRows()
{
    std::vector<WardenCheckRowInput> rows;
    rows.reserve(40);
    AppendInitialProfile(rows, 12340, "656E5553",
        "8C7CED99F8DDDD48296551EFE05A2CF27B26F818", "4F6B6179");
    AppendInitialProfile(rows, 12340, "656E4742",
        "8C7CED99F8DDDD48296551EFE05A2CF27B26F818", "4F6B6179");
    AppendInitialProfile(rows, 12340, "64654445",
        "0B4D01BDEB4F47DE030B57D81506093EB887EE0B", "4F4B");
    AppendInitialProfile(rows, 12340, "65734553",
        "20EC8371EC168B4723AF6DE3AFE81D46843726F4", "41636570746172");
    AppendInitialProfile(rows, 12340, "65734D58",
        "0E39F4AF09E3CF08925D41E61FBAC8EE16478FC9", "41636570746172");
    AppendInitialProfile(rows, 12340, "66724652",
        "E6F5A0C5C63056F63097420AE29B47ACA2E4D496", "4F4B");
    AppendInitialProfile(rows, 12340, "72755255",
        "329BF203079002D36E05EBF54BD5746AA37E47C8", "D09ED09A");
    AppendInitialProfile(rows, 12340, "6B6F4B52",
        "39BCDE7E67F7DA4A366D15007DBAF3D438338E00", "ED9995EC9DB8");
    AppendInitialProfile(rows, 12340, "7A68434E",
        "53538853E7026786EB30FCB247D7E8179A3CAAF8", "E7A1AEE5AE9A");
    AppendInitialProfile(rows, 12340, "7A685457",
        "ED14F2C71688B1DE9660F9CE04A62D63A9EB297A", "E7A2BAE5AE9A");
    return rows;
}

inline WardenCheckCatalog BuildInitialWardenCatalog()
{
    WardenCheckCatalogBuilder builder;
    WardenCheckDiagnostic diagnostic;
    for (WardenCheckRowInput const& row : InitialWardenRows())
    {
        if (builder.Add(row, diagnostic) != CheckCatalogValidation::Valid)
            return WardenCheckCatalog();
    }

    WardenCheckCatalog catalog;
    if (builder.Build(catalog, diagnostic) != CheckCatalogValidation::Valid)
        return WardenCheckCatalog();
    return catalog;
}
}
}

#endif

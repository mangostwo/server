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
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

#ifndef MANGOS_H_MOVE_MAP_SHARED_DEFINES
#define MANGOS_H_MOVE_MAP_SHARED_DEFINES

// Detour exports its own Include directory, so this needs no path walking.
// The relative form resolved only by accident of the include path.
#include "DetourNavMesh.h"
#include "Platform/Define.h"

#define MMAP_MAGIC 0x4d4d4150   // 'MMAP'
// Version 6 adds orthogonal terrain/liquid border cells during fused navmesh baking.
// Version 7 stores the TERRAIN BIT in each polygon's flags instead of a bare 1. Detour
// filters on flags, never on the area id, so a version 6 tile tells every query that
// every polygon is ground: a swimmer's WATER|MAGMA|SLIME mask matches nothing and a
// walker is cleared to cross magma. The two are indistinguishable at load time -- both
// are "a poly with flags set" -- so the version is what makes a stale bake say so
// instead of pathing wrongly for as long as it stays on disk.
#define MMAP_VERSION 7

struct MmapTileHeader
{
    uint32 mmapMagic;
    uint32 dtVersion;
    uint32 mmapVersion;
    uint32 size;
    bool usesLiquids : 1;

    MmapTileHeader() : mmapMagic(MMAP_MAGIC), dtVersion(DT_NAVMESH_VERSION),
        mmapVersion(MMAP_VERSION), size(0), usesLiquids(true) {}
};

enum NavTerrain
{
    NAV_EMPTY   = 0x00,
    NAV_GROUND  = 0x01,
    NAV_MAGMA   = 0x02,
    NAV_SLIME   = 0x04,
    NAV_WATER   = 0x08,
    NAV_UNUSED1 = 0x10,
    NAV_UNUSED2 = 0x20,
    NAV_UNUSED3 = 0x40,
    NAV_UNUSED4 = 0x80
    // we only have 8 bits
};

#endif  // _MOVE_MAP_SHARED_DEFINES_H

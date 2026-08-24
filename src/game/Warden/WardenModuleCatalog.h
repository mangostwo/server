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

#ifndef MANGOS_WARDEN_MODULE_CATALOG_H
#define MANGOS_WARDEN_MODULE_CATALOG_H

#include "WardenProtocol.h"

#include <array>
#include <string>
#include <vector>

namespace warden
{
/** Custody or shape failure found before a module can enter a session. */
enum class ModuleValidation : uint8
{
    Valid,
    WrongLength,
    DigestMismatch,
    InvalidInitialization
};

/** Build-specific host callbacks installed by command-3 archive record 1. */
struct ArchiveInitializationProfile
{
    // selectors[2] chooses the six-argument archive-read ABI in this module.
    std::array<uint8, 4> selectors{};
    uint32 openRva = 0;
    uint32 sizeRva = 0;
    uint32 readRva = 0;
    uint32 closeRva = 0;
};

/** Build-specific FrameScript callback installed by command-3 record 2. */
struct LuaInitializationProfile
{
    std::array<uint8, 3> prefix{};
    uint32 callbackRva = 0;
    uint8 selector = 0;
};

/** Build-specific client clock callback installed by command-3 record 3. */
struct TimingInitializationProfile
{
    std::array<uint8, 3> prefix{};
    uint32 callbackRva = 0;
    uint8 install = 0;
};

/** The three adjacent command-3 records required by the delivered module. */
struct ModuleInitializationProfile
{
    ArchiveInitializationProfile archive;
    LuaInitializationProfile lua;
    TimingInitializationProfile timing;
};

/** Exact build/platform module bytes, keys, hashes, and host callbacks. */
struct ModuleProfile
{
    // Host callbacks are exact-build contracts even if another client can load
    // the same delivered module bytes.
    uint32 build;
    char const* platform;
    ByteView module;
    ModuleId moduleId;
    Digest32 moduleSha256;
    Key16 moduleKey;
    Key16 hashSeed;
    Digest20 clientKeySeedHash;
    Key16 clientKeySeed;
    Key16 serverKeySeed;
    ModuleInitializationProfile initialization;
    // Exact client locales whose database check profiles must all be present
    // before this module can be published for strict admission.
    std::vector<std::string> requiredCheckLocales;
};

/** Selects and validates immutable, custody-pinned delivered modules. */
class WardenModuleCatalog
{
public:
    // Returns null rather than falling back across builds or platforms.
    ModuleProfile const* Find(uint32 build, std::string const& platform) const;
    std::vector<ModuleProfile const*> Profiles() const;

    // Recomputes both Blizzard's MD5 wire identity and the custody SHA-256.
    ModuleValidation Validate(ModuleProfile const& profile) const;
};
}

#endif

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

#ifndef MANGOS_SHA256_H
#define MANGOS_SHA256_H

// SHA-256 over bytes and over whole files, as lower-case hex.
//
// OpenSSL's EVP rather than a vendored implementation: the project already requires
// OpenSSL 3.x, and the one thing a hand-written hash buys -- a baker with no dependency
// -- is not worth owning a primitive whose failure mode is a digest that looks fine.
//
// This is INTEGRITY, not authentication. It answers "are these the bytes the baker
// wrote", which catches a truncated copy, a half-finished download, a stale tile left
// behind by an older bake, and a disk that flipped a sector. It does not answer "did an
// attacker replace them", because a manifest sitting beside the files it describes can
// be rewritten by anyone who can rewrite the files.

#include <cstddef>
#include <optional>
#include <string>

namespace MaNGOS
{
    namespace DataIntegrity
    {
        /// 64 lower-case hex characters, or empty when OpenSSL refuses.
        std::string Sha256Hex(const void* data, std::size_t length);

        /// Reads the file in blocks; nothing when it cannot be opened or read whole.
        std::optional<std::string> Sha256File(const std::string& path);
    }
}

#endif

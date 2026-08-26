/**
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2026 MaNGOS <https://www.getmangos.eu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

#include "WorldGatewayAuth.h"

#include <openssl/crypto.h>

#include <algorithm>
#include <utility>

bool IsSupportedAccountClientOS(const std::string& os)
{
    return os == "Win" || os == "OSX";
}

warden::AdmissionData BuildWardenAdmissionData(uint32 build,
    std::string platform, std::string clientLocale, BigNumber& sessionKey)
{
    warden::AdmissionData admission;
    admission.build = build;
    admission.platform = std::move(platform);
    admission.clientLocale = std::move(clientLocale);

    // BigNumber owns this buffer and will replace it on the next serialization.
    // Transfer the bytes, cleanse in place, and neither retain nor free it.
    uint8* const serialized = sessionKey.AsByteArray(
        static_cast<int>(admission.sessionKey.size()));
    if (!serialized)
    {
        admission.Clear();
        return admission;
    }
    std::copy(serialized, serialized + admission.sessionKey.size(),
        admission.sessionKey.begin());
    OPENSSL_cleanse(serialized, admission.sessionKey.size());
    admission.available = true;
    return admission;
}

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

#include "WardenCryptoContext.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>
#include <utility>

namespace
{
warden::Bytes FromHex(char const* text)
{
    auto nibble = [](char value) -> uint8
    {
        if (value >= '0' && value <= '9')
            return uint8(value - '0');
        return uint8(std::toupper(static_cast<unsigned char>(value)) - 'A' + 10);
    };

    std::string const hex(text);
    warden::Bytes result;
    result.reserve(hex.size() / 2);
    for (std::size_t i = 0; i < hex.size(); i += 2)
        result.push_back(uint8((nibble(hex[i]) << 4) | nibble(hex[i + 1])));
    return result;
}

warden::SessionKey LeadingZeroSessionKey()
{
    warden::SessionKey key{};
    for (uint8 i = 0; i < 39; ++i)
        key[i] = uint8(i + 1);
    return key;
}
}

TEST(WardenCrypto_fixed_40_byte_derivation_matches_wrath_module_use_vector)
{
    warden::WardenCryptoContext crypto;
    CHECK(crypto.Initialize(LeadingZeroSessionKey()));
    CHECK(crypto.IsInitialized());

    warden::Bytes moduleUse = FromHex(
        "0079C0768D657977D697E10BAD956CCED1"
        "AE25BC51063B77BD363C3EFE0FC173F9"
        "44490000");
    CHECK(crypto.TransformServerToClient(moduleUse));
    CHECK_HEX(moduleUse.data(), moduleUse.size(),
        "e7a486e53584d55beb2494c9a10a7ef101909cd834b37962154af931bad33e640e45034bd2");

    warden::Bytes moduleOk = {0x89};
    CHECK(crypto.TransformClientToServer(moduleOk));
    CHECK_HEX(moduleOk.data(), moduleOk.size(), "01");
}

TEST(WardenCrypto_split_transforms_preserve_stream_continuity)
{
    warden::WardenCryptoContext whole;
    warden::WardenCryptoContext split;
    CHECK(whole.Initialize(LeadingZeroSessionKey()));
    CHECK(split.Initialize(LeadingZeroSessionKey()));

    warden::Bytes wholeBytes(64);
    for (std::size_t i = 0; i < wholeBytes.size(); ++i)
        wholeBytes[i] = uint8(i);
    warden::Bytes splitBytes = wholeBytes;

    CHECK(whole.TransformServerToClient(wholeBytes));
    warden::Bytes first(splitBytes.begin(), splitBytes.begin() + 7);
    warden::Bytes second(splitBytes.begin() + 7, splitBytes.end());
    CHECK(split.TransformServerToClient(first));
    CHECK(split.TransformServerToClient(second));
    std::copy(first.begin(), first.end(), splitBytes.begin());
    std::copy(second.begin(), second.end(), splitBytes.begin() + first.size());

    CHECK(std::equal(wholeBytes.begin(), wholeBytes.end(), splitBytes.begin()));
}

TEST(WardenCrypto_transaction_clone_rolls_back_inbound_when_discarded)
{
    warden::WardenCryptoContext original;
    warden::WardenCryptoContext reference;
    CHECK(original.Initialize(LeadingZeroSessionKey()));
    CHECK(reference.Initialize(LeadingZeroSessionKey()));

    warden::WardenCryptoContext rejected = original.CloneForTransaction();
    warden::Bytes malformed = {0x01, 0x02, 0x03, 0x04};
    CHECK(rejected.TransformClientToServer(malformed));

    warden::Bytes actual = {0x10, 0x20, 0x30};
    warden::Bytes expected = actual;
    CHECK(original.TransformClientToServer(actual));
    CHECK(reference.TransformClientToServer(expected));
    CHECK(std::equal(actual.begin(), actual.end(), expected.begin()));
}

TEST(WardenCrypto_transaction_clone_commits_outbound_stream_by_move)
{
    warden::WardenCryptoContext original;
    warden::WardenCryptoContext reference;
    CHECK(original.Initialize(LeadingZeroSessionKey()));
    CHECK(reference.Initialize(LeadingZeroSessionKey()));

    warden::WardenCryptoContext accepted = original.CloneForTransaction();
    warden::Bytes first = {0x01, 0x02, 0x03, 0x04};
    warden::Bytes expectedFirst = first;
    CHECK(accepted.TransformServerToClient(first));
    CHECK(reference.TransformServerToClient(expectedFirst));
    CHECK(std::equal(first.begin(), first.end(), expectedFirst.begin()));
    original = std::move(accepted);

    warden::Bytes second = {0x10, 0x20, 0x30};
    warden::Bytes expectedSecond = second;
    CHECK(original.TransformServerToClient(second));
    CHECK(reference.TransformServerToClient(expectedSecond));
    CHECK(std::equal(second.begin(), second.end(), expectedSecond.begin()));
}

TEST(WardenCrypto_rekey_replaces_both_directions_atomically)
{
    warden::WardenCryptoContext crypto;
    CHECK(crypto.Initialize(LeadingZeroSessionKey()));

    warden::Key16 const clientKey = {0x7F, 0x96, 0xEE, 0xFD, 0xA5, 0xB6,
        0x3D, 0x20, 0xA4, 0xDF, 0x8E, 0x00, 0xCB, 0xF4, 0x83, 0x04};
    warden::Key16 const serverKey = {0xC2, 0xB7, 0xAD, 0xED, 0xFC, 0xCC,
        0xA9, 0xC2, 0xBF, 0xB3, 0xF8, 0x56, 0x02, 0xBA, 0x80, 0x9B};
    CHECK(crypto.InstallModuleKeys(clientKey, serverKey));

    warden::Bytes inbound(16, 0);
    warden::Bytes outbound(16, 0);
    CHECK(crypto.TransformClientToServer(inbound));
    CHECK(crypto.TransformServerToClient(outbound));
    CHECK_HEX(inbound.data(), inbound.size(),
        "c8b92171fbfffc1ce7f3eec8d70f6278");
    CHECK_HEX(outbound.data(), outbound.size(),
        "526ac7993f3d4393504365b536099aa2");
}

TEST(WardenCrypto_rejects_use_before_initialization)
{
    warden::WardenCryptoContext crypto;
    warden::Bytes bytes = {1, 2, 3};
    warden::Key16 key{};

    CHECK(!crypto.IsInitialized());
    CHECK(!crypto.TransformClientToServer(bytes));
    CHECK(!crypto.TransformServerToClient(bytes));
    CHECK(!crypto.InstallModuleKeys(key, key));
    CHECK_HEX(bytes.data(), bytes.size(), "010203");
}

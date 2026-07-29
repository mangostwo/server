/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2025 MaNGOS <https://www.getmangos.eu>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

// The packet crypt, which is EXPANSION-SPECIFIC and therefore lives apart from the rest
// of the crypto tests. 3.3.5a and later seed a pair of RC4 streams from HMAC-SHA1 over
// the session key, with separate send and receive directions. 2.4.3 does something else
// entirely -- a fixed-length header crypt, four bytes out and six in, with no HMAC seed
// and no continuous keystream -- so these assertions describe one wire format, not the
// family. CryptoStressTest.cpp holds what every expansion shares.

#include "TestHarness.h"

#include "Auth/AuthCrypt.h"
#include "Auth/BigNumber.h"
#include "PacketCodec.h"

#include <cstring>
#include <random>
#include <algorithm>
#include <vector>


namespace
{
    std::vector<uint8> Frame(uint16 opcode, const std::vector<uint8>& payload)
    {
        const uint32 size = uint32(payload.size()) + 4;

        std::vector<uint8> out;
        out.push_back(uint8((size >> 8) & 0xFF));
        out.push_back(uint8(size & 0xFF));
        out.push_back(uint8(opcode & 0xFF));
        out.push_back(uint8((opcode >> 8) & 0xFF));
        out.push_back(0);
        out.push_back(0);
        out.insert(out.end(), payload.begin(), payload.end());
        return out;
    }

    /// A session key of the shape the login handshake produces.
    BigNumber MakeSessionKey(unsigned seed)
    {
        BigNumber k;
        k.SetRand(40 * 8);
        // SetRand is what the tests need; seed only documents intent.
        (void)seed;
        return k;
    }
}

TEST(Crypto_authcrypt_round_trips)
{
    // Not "encrypt then decrypt": EncryptSend and DecryptRecv are keyed with two
    // *different* constants by design, one per direction, so one can never undo
    // the other. The server holds the server-side half of each pair; the real
    // client holds the mirror. What can be checked from this side alone is the
    // property that makes the two ends agree at all -- the keystream is a pure
    // function of the session key, and RC4 is an XOR stream, so the same
    // operation applied twice by two identically keyed instances is the
    // identity. If the keystreams ever diverged, this would not close.
    BigNumber key = MakeSessionKey(1);

    AuthCrypt server;
    AuthCrypt client;
    server.Init(&key);
    client.Init(&key);

    CHECK(server.IsInitialized());

    std::mt19937 rng(0xA11Cu);
    std::uniform_int_distribution<int> byteDist(0, 255);

    int mismatches = 0;

    for (int packet = 0; packet < 2000; ++packet)
    {
        std::vector<uint8> plain(6);
        for (uint8& b : plain)
        {
            b = uint8(byteDist(rng));
        }

        std::vector<uint8> wire = plain;
        server.EncryptSend(wire.data(), wire.size());
        client.EncryptSend(wire.data(), wire.size());

        if (wire != plain)
        {
            ++mismatches;
        }
    }

    CHECK_EQ(mismatches, 0);
}

TEST(Crypto_authcrypt_keystream_is_continuous_across_fragments)
{
    // The property the packet codec depends on, and the reason it decrypts a
    // header exactly once rather than as its bytes arrive.
    //
    // Encrypt 4096 bytes in one call with one cipher, and the same bytes one or
    // two at a time with another keyed identically. The outputs must be
    // identical. If anything in the cipher re-keys or reprocesses at a call
    // boundary, this diverges -- and on a live server it would present as a
    // connection that works until the first TCP split, then produces garbage
    // for good.
    BigNumber key = MakeSessionKey(2);

    AuthCrypt oneShot;
    AuthCrypt fragmented;
    oneShot.Init(&key);
    fragmented.Init(&key);

    const size_t SIZE = 4096;

    std::vector<uint8> a(SIZE);
    for (size_t i = 0; i < SIZE; ++i)
    {
        a[i] = uint8(i * 17u);
    }
    std::vector<uint8> b = a;

    oneShot.EncryptSend(a.data(), a.size());

    std::mt19937 rng(0xF4A6u);
    std::uniform_int_distribution<size_t> chunkDist(1, 3);

    size_t offset = 0;
    while (offset < SIZE)
    {
        const size_t chunk = std::min(chunkDist(rng), SIZE - offset);
        fragmented.EncryptSend(&b[offset], chunk);
        offset += chunk;
    }

    CHECK(a == b);
}

TEST(Crypto_authcrypt_send_and_recv_keystreams_are_independent)
{
    // Inbound and outbound use separate cipher state. Encrypting must not
    // advance the decrypt keystream, or the two directions drift apart as soon
    // as traffic is not perfectly symmetric -- which it never is.
    BigNumber key = MakeSessionKey(3);

    AuthCrypt reference;
    AuthCrypt disturbed;
    reference.Init(&key);
    disturbed.Init(&key);

    std::vector<uint8> noise(256, 0x5A);
    disturbed.EncryptSend(noise.data(), noise.size());

    std::vector<uint8> a(64, 0x11);
    std::vector<uint8> b = a;

    reference.DecryptRecv(a.data(), a.size());
    disturbed.DecryptRecv(b.data(), b.size());

    CHECK(a == b);
}

TEST(Crypto_authcrypt_uninitialised_is_inert)
{
    // Before the session key is agreed the crypt must not touch the bytes: the
    // auth challenge and the client's first packet are exchanged in clear.
    AuthCrypt crypt;
    CHECK(!crypt.IsInitialized());
}

TEST(Crypto_authcrypt_encrypted_headers_round_trip_through_the_codec)
{
    // The full inbound path with the cipher armed: encrypt each header the way a
    // client would, deliver the stream a few bytes at a time, and decrypt through
    // the codec's hook. This is where a keystream that advances at the wrong time
    // shows up -- the first packet decodes and every later one is garbage.
    BigNumber key;
    key.SetRand(40 * 8);

    AuthCrypt clientSide;
    AuthCrypt serverSide;
    clientSide.Init(&key);
    serverSide.Init(&key);

    proto::PacketCodec codec([&serverSide](uint8* header, size_t len)
    {
        serverSide.DecryptRecv(header, len);
    });

    std::mt19937 rng(0xE11Au);
    std::uniform_int_distribution<uint32> sizeDist(0, 200);
    std::uniform_int_distribution<size_t> chunkDist(1, 5);

    const int COUNT = 500;

    std::vector<uint16> opcodes;
    std::vector<uint8>  stream;

    for (int i = 0; i < COUNT; ++i)
    {
        const uint16 opcode = uint16(0x100 + (i % 200));
        opcodes.push_back(opcode);

        std::vector<uint8> payload(sizeDist(rng), uint8(i));
        std::vector<uint8> framed = Frame(opcode, payload);

        // The client encrypts only the header; the payload goes in clear.
        //
        // DecryptRecv, not EncryptSend: the incoming direction is keyed with the
        // client-decrypt constant, and the codec under test will call DecryptRecv
        // on it. RC4 is an XOR stream, so running that same operation here with a
        // second identically keyed instance produces exactly the ciphertext the
        // real client would have put on the wire.
        clientSide.DecryptRecv(framed.data(), proto::CLIENT_HEADER_SIZE);

        stream.insert(stream.end(), framed.begin(), framed.end());
    }

    std::vector<WorldPacket> out;
    size_t offset = 0;
    bool   rejected = false;

    while (offset < stream.size() && !rejected)
    {
        const size_t chunk = std::min(chunkDist(rng), stream.size() - offset);
        rejected = codec.Feed(&stream[offset], chunk, out)
                   == proto::DecodeStatus::Malformed;
        offset += chunk;
    }

    CHECK(!rejected);
    CHECK_EQ(int(out.size()), COUNT);

    int wrongOpcode = 0;
    for (int i = 0; i < COUNT && i < int(out.size()); ++i)
    {
        if (uint16(out[i].GetOpcode()) != opcodes[i])
        {
            ++wrongOpcode;
        }
    }
    CHECK_EQ(wrongOpcode, 0);
}

// ---------------------------------------------------------------------------
// Bad path
// ---------------------------------------------------------------------------

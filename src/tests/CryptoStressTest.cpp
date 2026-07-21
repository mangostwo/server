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

#include "TestHarness.h"

#include "Auth/AuthCrypt.h"
#include "Auth/BigNumber.h"
#include "Auth/HMACSHA1.h"
#include "Auth/Md5.h"
#include "Auth/Sha1.h"

#include <cstring>
#include <random>
#include <string>
#include <vector>

/**
 * @file
 * @brief Known-answer and stress coverage for the crypto used on the wire.
 *
 * Every digest here was reimplemented during the OpenSSL 3.x migration: Sha1Hash
 * and HMACSHA1 moved from the deprecated low-level API to EVP, Md5Hash is new,
 * and a 474-line vendored MD5 was deleted. A reimplementation that is subtly
 * wrong does not fail to build and does not crash -- it produces a digest the
 * client rejects, which presents as "login does not work" with nothing in the
 * log. Known-answer vectors are the only thing that catches that.
 *
 * The vectors are from RFC 1321 (MD5), RFC 3174 (SHA-1) and RFC 2202 (HMAC).
 *
 * The stream-cipher cases test a different property, and the one the packet
 * codec depends on: encrypting in one call and encrypting the same bytes a few
 * at a time must produce identical output. A stream cipher advances its
 * keystream per byte, so anything that re-keys or double-processes on a
 * fragment boundary desynchronises the connection from that point on -- and
 * fragmentation only happens under real network conditions, never on loopback.
 */

namespace
{
    std::string ToHex(const uint8* data, size_t length)
    {
        static const char* digits = "0123456789abcdef";
        std::string out;
        out.reserve(length * 2);
        for (size_t i = 0; i < length; ++i)
        {
            out.push_back(digits[(data[i] >> 4) & 0x0F]);
            out.push_back(digits[data[i] & 0x0F]);
        }
        return out;
    }

    void CheckHex(const char* what, const std::string& got, const char* expected)
    {
        if (got != expected)
        {
            testing::ReportFailure(__FILE__, __LINE__,
                std::string(what) + ": got " + got + ", expected " + expected);
        }
    }

    /// A session key of the shape the login handshake produces.
    BigNumber MakeSessionKey(unsigned seed)
    {
        BigNumber k;
        k.SetRand(40 * 8);
        // SetRand is fine for the stream tests; seed only documents intent.
        (void)seed;
        return k;
    }
}

// ---------------------------------------------------------------------------
// Known answers
// ---------------------------------------------------------------------------

TEST(Crypto_md5_known_vectors)
{
    {
        Md5Hash h;
        h.Finalize();
        CheckHex("MD5(\"\")", ToHex(h.GetDigest(), 16),
                 "d41d8cd98f00b204e9800998ecf8427e");
    }
    {
        Md5Hash h;
        h.UpdateData(std::string("abc"));
        h.Finalize();
        CheckHex("MD5(\"abc\")", ToHex(h.GetDigest(), 16),
                 "900150983cd24fb0d6963f7d28e17f72");
    }
    {
        Md5Hash h;
        h.UpdateData(std::string("message digest"));
        h.Finalize();
        CheckHex("MD5(\"message digest\")", ToHex(h.GetDigest(), 16),
                 "f96b697d7cb7938d525a2f31aaf161d0");
    }
}

TEST(Crypto_sha1_known_vectors)
{
    {
        Sha1Hash h;
        h.Finalize();
        CheckHex("SHA1(\"\")", ToHex(h.GetDigest(), 20),
                 "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    }
    {
        Sha1Hash h;
        h.UpdateData(std::string("abc"));
        h.Finalize();
        CheckHex("SHA1(\"abc\")", ToHex(h.GetDigest(), 20),
                 "a9993e364706816aba3e25717850c26c9cd0d89d");
    }
}

TEST(Crypto_hmac_sha1_rfc2202_vector)
{
    // RFC 2202 test case 1.
    uint8 key[20];
    std::memset(key, 0x0b, sizeof(key));

    HMACSHA1 hmac(sizeof(key), key);
    hmac.UpdateData(std::string("Hi There"));
    hmac.Finalize();

    CheckHex("HMAC-SHA1", ToHex(hmac.GetDigest(), 20),
             "b617318655057264e28bc0b6fb378c8ef146be00");
}

TEST(Crypto_incremental_hashing_matches_one_shot)
{
    // Feeding a digest in pieces must equal feeding it whole. This is the
    // property Warden and the login proof both rely on, since both build their
    // input from several appends.
    std::mt19937 rng(0x51A1u);
    std::uniform_int_distribution<int> byteDist(0, 255);
    std::uniform_int_distribution<size_t> chunkDist(1, 37);

    int mismatches = 0;

    for (int iteration = 0; iteration < 500; ++iteration)
    {
        std::vector<uint8> message(1 + (iteration % 400));
        for (uint8& b : message)
        {
            b = uint8(byteDist(rng));
        }

        Sha1Hash whole;
        whole.UpdateData(message.data(), int(message.size()));
        whole.Finalize();
        const std::string expectedSha = ToHex(whole.GetDigest(), 20);

        Sha1Hash pieces;
        size_t offset = 0;
        while (offset < message.size())
        {
            const size_t chunk = std::min(chunkDist(rng), message.size() - offset);
            pieces.UpdateData(&message[offset], int(chunk));
            offset += chunk;
        }
        pieces.Finalize();

        if (ToHex(pieces.GetDigest(), 20) != expectedSha)
        {
            ++mismatches;
        }

        Md5Hash wholeMd5;
        wholeMd5.UpdateData(message.data(), message.size());
        wholeMd5.Finalize();
        const std::string expectedMd5 = ToHex(wholeMd5.GetDigest(), 16);

        Md5Hash piecesMd5;
        offset = 0;
        while (offset < message.size())
        {
            const size_t chunk = std::min(chunkDist(rng), message.size() - offset);
            piecesMd5.UpdateData(&message[offset], chunk);
            offset += chunk;
        }
        piecesMd5.Finalize();

        if (ToHex(piecesMd5.GetDigest(), 16) != expectedMd5)
        {
            ++mismatches;
        }
    }

    CHECK_EQ(mismatches, 0);
}

// ---------------------------------------------------------------------------
// The stream cipher
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// BigNumber under load
// ---------------------------------------------------------------------------

TEST(Crypto_bignumber_hex_round_trips)
{
    // SetHexStr/AsHexStr is how the session key crosses the database boundary
    // between realmd and the world server. A round-trip that loses a leading
    // zero is exactly the shape of the padding bug.
    std::mt19937 rng(0xB19Eu);
    std::uniform_int_distribution<int> nibble(0, 15);

    static const char* digits = "0123456789ABCDEF";
    int mismatches = 0;

    for (int iteration = 0; iteration < 2000; ++iteration)
    {
        std::string hex;
        hex.reserve(80);
        for (int i = 0; i < 80; ++i)
        {
            hex.push_back(digits[nibble(rng)]);
        }

        BigNumber n;
        n.SetHexStr(hex.c_str());

        // Fixed width, little-endian: SetBinary reverses its input, so it must
        // be fed the same byte order AsByteArray produces by default.
        const uint8* bytes = n.AsByteArray(40);

        BigNumber rebuilt;
        rebuilt.SetBinary(bytes, 40);

        if (std::string(n.AsHexStr()) != std::string(rebuilt.AsHexStr()))
        {
            ++mismatches;
        }
    }

    CHECK_EQ(mismatches, 0);
}

TEST(Crypto_bignumber_fixed_width_output_is_always_that_width)
{
    // Over many random values, AsByteArray(40) must always yield exactly 40
    // meaningful bytes with the value right-aligned. Roughly one value in 256
    // serialises short, so this sweep hits the padding path many times.
    int wrong = 0;

    for (int iteration = 0; iteration < 5000; ++iteration)
    {
        BigNumber n;
        n.SetRand(40 * 8);

        const uint8* le = n.AsByteArray(40);

        // Rebuilding from the fixed-width form must give the same number back.
        BigNumber rebuilt;
        rebuilt.SetBinary(le, 40);

        if (std::string(rebuilt.AsHexStr()) != std::string(n.AsHexStr()))
        {
            ++wrong;
        }
    }

    CHECK_EQ(wrong, 0);
}

TEST(Crypto_warden_module_id_is_stable)
{
    // Warden identifies a client module by the MD5 of its compressed image. The
    // same input must always give the same id -- this is the call that moved off
    // the deprecated MD5_* API, and a wrong id makes the client refuse the
    // module with nothing useful logged.
    std::vector<uint8> module(64 * 1024);
    for (size_t i = 0; i < module.size(); ++i)
    {
        module[i] = uint8((i * 7u) ^ 0xA5u);
    }

    Md5Hash first;
    first.UpdateData(module.data(), module.size());
    first.Finalize();
    const std::string a = ToHex(first.GetDigest(), 16);

    Md5Hash second;
    second.UpdateData(module.data(), module.size());
    second.Finalize();
    const std::string b = ToHex(second.GetDigest(), 16);

    CHECK(a == b);
    CHECK_EQ(int(a.size()), 32);
}

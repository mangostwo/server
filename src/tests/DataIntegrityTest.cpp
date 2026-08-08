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

// The digests are checked against the PUBLISHED vectors, not against a second run of the
// same code -- a hash agreeing with itself proves nothing at all, and this one is written
// into a file that another tool (sha256sum) has to agree with.

#include "TestHarness.h"

#include "DataIntegrity/DataManifest.h"
#include "DataIntegrity/Sha256.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <process.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

using namespace MaNGOS::DataIntegrity;

namespace
{
    unsigned long TestPid()
    {
#ifdef _WIN32
        return static_cast<unsigned long>(::GetCurrentProcessId());
#else
        return static_cast<unsigned long>(::getpid());
#endif
    }

    std::string TempRoot(const char* leaf)
    {
        const char* dir = std::getenv("TMPDIR");
        if (!dir)
        {
            dir = std::getenv("TEMP");
        }
        if (!dir)
        {
            dir = "/tmp";
        }
        return std::string(dir) + "/mangos_manifest_" + std::to_string(TestPid()) + "_" +
               leaf;
    }

    /// A data directory that cleans itself up however the test leaves.
    struct ScopedTree
    {
        std::string root;

        explicit ScopedTree(const char* leaf) : root(TempRoot(leaf))
        {
            std::error_code ec;
            std::filesystem::remove_all(root, ec);
            std::filesystem::create_directories(root + "/tiles", ec);
            std::filesystem::create_directories(root + "/dbc", ec);
        }

        ~ScopedTree()
        {
            std::error_code ec;
            std::filesystem::remove_all(root, ec);
        }

        void Put(const std::string& relative, const std::string& bytes) const
        {
            std::ofstream out(root + "/" + relative, std::ios::binary | std::ios::trunc);
            out.write(bytes.data(), std::streamsize(bytes.size()));
        }

        std::string Read(const std::string& relative) const
        {
            std::ifstream in(root + "/" + relative, std::ios::binary);
            return std::string((std::istreambuf_iterator<char>(in)),
                               std::istreambuf_iterator<char>());
        }
    };
}

TEST(Sha256MatchesThePublishedVectors)
{
    // FIPS 180-4 / NIST examples.
    CHECK_STR(Sha256Hex("", 0),
              "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    CHECK_STR(Sha256Hex("abc", 3),
              "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

    const std::string abcdef =
        "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    CHECK_STR(Sha256Hex(abcdef.data(), abcdef.size()),
              "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");

    // A million 'a'. Slow enough to matter only here, and the one vector that exercises
    // the block loop rather than a single padded block.
    const std::string million(1000000, 'a');
    CHECK_STR(Sha256Hex(million.data(), million.size()),
              "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
}

TEST(Sha256OfAFileMatchesTheSameBytesInMemory)
{
    ScopedTree tree("filehash");
    tree.Put("tiles/one.tile", "abc");

    const auto hex = Sha256File(tree.root + "/tiles/one.tile");
    REQUIRE(hex.has_value());
    CHECK_STR(*hex, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

    CHECK(!Sha256File(tree.root + "/tiles/absent.tile").has_value());
}

TEST(ManifestRoundTripsAndIsSha256sumShaped)
{
    ScopedTree tree("roundtrip");
    tree.Put("tiles/t_0_32_32.tile", "abc");
    tree.Put("dbc/Map.dbc", "");

    REQUIRE(WriteManifest(tree.root, {"tiles", "dbc"}));

    // `<64 hex><two spaces><path>`, LF, sorted -- the layout `sha256sum -c` reads. A
    // header or a comment would be friendlier to us and unreadable to it.
    const std::string text = tree.Read(MANIFEST_FILE_NAME);
    CHECK(text.find('\r') == std::string::npos);
    CHECK(text.find(
              "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855  dbc/Map.dbc\n") == 0);
    CHECK(text.find(
              "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad  tiles/t_0_32_32.tile\n") !=
          std::string::npos);

    std::vector<ManifestEntry> entries;
    REQUIRE(ReadManifest(tree.root + "/" + MANIFEST_FILE_NAME, entries));
    CHECK_EQ(entries.size(), size_t(2));
    CHECK_STR(entries[0].path, "dbc/Map.dbc");
    CHECK_STR(entries[1].path, "tiles/t_0_32_32.tile");

    const VerifyResult ok = VerifyManifest(tree.root);
    CHECK(ok.Ok());
    CHECK_EQ(ok.verified, size_t(2));
    CHECK_EQ(ok.changed, size_t(0));
}

TEST(ManifestNoticesEveryWayADataSetGoesWrong)
{
    ScopedTree tree("tamper");
    tree.Put("tiles/a.tile", "aaaa");
    tree.Put("tiles/b.tile", "bbbb");
    tree.Put("dbc/Map.dbc", "cccc");
    REQUIRE(WriteManifest(tree.root, {"tiles", "dbc"}));

    // One byte, which is the case the whole thing exists for: a tile that still has its
    // magic, its version and its length, and is not the tile that was baked.
    tree.Put("tiles/a.tile", "aaab");

    std::error_code ec;
    std::filesystem::remove(tree.root + "/tiles/b.tile", ec);

    const VerifyResult bad = VerifyManifest(tree.root);
    CHECK(!bad.Ok());
    CHECK_EQ(bad.listed, size_t(3));
    CHECK_EQ(bad.verified, size_t(1));
    CHECK_EQ(bad.changed, size_t(1));
    CHECK_EQ(bad.missing, size_t(1));
    CHECK(!bad.examples.empty());

    // A file the manifest never listed is not damage: a data directory holds things the
    // baker did not write, and calling those corruption would make the check unusable
    // exactly where it is needed.
    tree.Put("tiles/a.tile", "aaaa");
    std::filesystem::remove(tree.root + "/tiles/b.tile", ec);
    tree.Put("tiles/b.tile", "bbbb");
    tree.Put("tiles/mine.txt", "not from the baker");

    const VerifyResult restored = VerifyManifest(tree.root);
    CHECK(restored.Ok());
    CHECK_EQ(restored.verified, size_t(3));
}

TEST(ManifestIsAbsentRatherThanEmptyWhenThereIsNone)
{
    ScopedTree tree("nomanifest");
    tree.Put("tiles/a.tile", "aaaa");

    const VerifyResult none = VerifyManifest(tree.root);
    CHECK(!none.manifestFound);
    CHECK(!none.Ok());              // not verified, which is not the same as verified bad
    CHECK_EQ(none.listed, size_t(0));
}

TEST(ManifestRejectsALineItDidNotWrite)
{
    ScopedTree tree("badline");
    tree.Put("tiles/a.tile", "aaaa");
    REQUIRE(WriteManifest(tree.root, {"tiles"}));

    // Truncated digest: accepted, this would compare a 32-character prefix against a
    // 64-character one and report every file as changed.
    tree.Put(MANIFEST_FILE_NAME, "deadbeef  tiles/a.tile\n");
    std::vector<ManifestEntry> entries;
    CHECK(!ReadManifest(tree.root + "/" + MANIFEST_FILE_NAME, entries));

    const VerifyResult bad = VerifyManifest(tree.root);
    CHECK(bad.manifestFound);
    CHECK(!bad.manifestReadable);
    CHECK(!bad.Ok());
}

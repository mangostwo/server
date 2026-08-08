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

#include "DataIntegrity/Sha256.h"

#include <openssl/evp.h>

#include <array>
#include <cstdio>
#include <memory>
#include <vector>

namespace MaNGOS
{
    namespace DataIntegrity
    {
        namespace
        {
            /// 64 KiB. Large enough that the syscall is not the cost and small enough
            /// that hashing a thousand files never holds a megabyte per worker.
            constexpr std::size_t READ_BLOCK = 64 * 1024;

            struct EvpDeleter
            {
                void operator()(EVP_MD_CTX* ctx) const { EVP_MD_CTX_free(ctx); }
            };

            using EvpCtx = std::unique_ptr<EVP_MD_CTX, EvpDeleter>;

            std::string ToHex(const unsigned char* digest, unsigned int length)
            {
                static const char* kDigits = "0123456789abcdef";
                std::string out;
                out.reserve(std::size_t(length) * 2);
                for (unsigned int i = 0; i < length; ++i)
                {
                    out.push_back(kDigits[digest[i] >> 4]);
                    out.push_back(kDigits[digest[i] & 0x0F]);
                }
                return out;
            }
        }

        std::string Sha256Hex(const void* data, std::size_t length)
        {
            EvpCtx ctx(EVP_MD_CTX_new());
            if (!ctx)
            {
                return std::string();
            }

            if (EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr) != 1)
            {
                return std::string();
            }

            // A null pointer with a zero length is the empty message, which has a
            // perfectly good digest; passing null to EVP_DigestUpdate is not defined.
            if (length && EVP_DigestUpdate(ctx.get(), data, length) != 1)
            {
                return std::string();
            }

            std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
            unsigned int digestLength = 0;
            if (EVP_DigestFinal_ex(ctx.get(), digest.data(), &digestLength) != 1)
            {
                return std::string();
            }

            return ToHex(digest.data(), digestLength);
        }

        std::optional<std::string> Sha256File(const std::string& path)
        {
            std::FILE* f = std::fopen(path.c_str(), "rb");
            if (!f)
            {
                return std::nullopt;
            }

            EvpCtx ctx(EVP_MD_CTX_new());
            if (!ctx || EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr) != 1)
            {
                std::fclose(f);
                return std::nullopt;
            }

            std::vector<unsigned char> block(READ_BLOCK);
            for (;;)
            {
                const std::size_t got = std::fread(block.data(), 1, block.size(), f);
                if (got && EVP_DigestUpdate(ctx.get(), block.data(), got) != 1)
                {
                    std::fclose(f);
                    return std::nullopt;
                }
                if (got < block.size())
                {
                    // A short read is the end of the file OR an I/O error, and the two
                    // must not produce the same answer: hashing what was read up to a
                    // failed sector yields a digest for bytes nobody has.
                    const bool failed = std::ferror(f) != 0;
                    std::fclose(f);
                    if (failed)
                    {
                        return std::nullopt;
                    }
                    break;
                }
            }

            std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
            unsigned int digestLength = 0;
            if (EVP_DigestFinal_ex(ctx.get(), digest.data(), &digestLength) != 1)
            {
                return std::nullopt;
            }

            return ToHex(digest.data(), digestLength);
        }
    }
}

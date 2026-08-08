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

#include "DataIntegrity/DataManifest.h"
#include "DataIntegrity/Sha256.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <system_error>
#include <thread>

namespace MaNGOS
{
    namespace DataIntegrity
    {
        const char* const MANIFEST_FILE_NAME = "data.manifest";

        namespace
        {
            constexpr std::size_t MAX_EXAMPLES = 8;
            constexpr std::size_t HEX_LENGTH = 64;

            std::string JoinPath(const std::string& a, const std::string& b)
            {
                if (a.empty())
                {
                    return b;
                }
                if (b.empty())
                {
                    return a;
                }
                const char last = a[a.size() - 1];
                return (last == '/' || last == '\\') ? a + b : a + "/" + b;
            }

            /// Always '/', on every platform. The manifest travels with the data set:
            /// a bake done on Windows must verify on the Linux box it is copied to, and
            /// a path separator is not a property of the bytes being described.
            std::string ToPortablePath(const std::filesystem::path& relative)
            {
                std::string s = relative.generic_string();
                return s;
            }

            /// Sorted so two bakes of the same data produce byte-identical manifests --
            /// which is what lets one be diffed against another, and what stops a
            /// directory iteration order from looking like a change.
            void SortEntries(std::vector<ManifestEntry>& entries)
            {
                std::sort(entries.begin(), entries.end(),
                          [](const ManifestEntry& a, const ManifestEntry& b)
                          {
                              return a.path < b.path;
                          });
            }

            bool CollectFiles(const std::string& root, const std::string& subdir,
                              std::vector<std::string>& out)
            {
                std::error_code ec;
                const std::filesystem::path base = std::filesystem::path(root) / subdir;
                if (!std::filesystem::exists(base, ec))
                {
                    // A data set without a navmesh is a data set without a navmesh, not
                    // a broken one; the caller lists what it hopes for, not what it
                    // requires.
                    return true;
                }

                std::filesystem::recursive_directory_iterator it(
                    base, std::filesystem::directory_options::skip_permission_denied, ec);
                if (ec)
                {
                    return false;
                }

                const std::filesystem::path rootPath(root);
                for (const auto& entry : it)
                {
                    if (ec)
                    {
                        return false;
                    }
                    if (!entry.is_regular_file(ec) || ec)
                    {
                        continue;
                    }
                    const std::filesystem::path rel =
                        std::filesystem::relative(entry.path(), rootPath, ec);
                    if (ec)
                    {
                        return false;
                    }
                    out.push_back(ToPortablePath(rel));
                }
                return true;
            }

            /// Hashes `paths` across `threads` workers, writing digests into `digests`
            /// by index. Empty string means the file could not be read.
            void HashAll(const std::string& root, const std::vector<std::string>& paths,
                         std::vector<std::string>& digests, unsigned threads,
                         const ProgressFn& progress)
            {
                digests.assign(paths.size(), std::string());
                if (paths.empty())
                {
                    return;
                }

                if (threads == 0)
                {
                    threads = std::thread::hardware_concurrency();
                }
                threads = std::max(1u, std::min<unsigned>(threads, 16u));
                threads = unsigned(std::min<std::size_t>(threads, paths.size()));

                std::atomic<std::size_t> next{0};
                std::atomic<std::size_t> done{0};
                std::mutex progressMutex;

                const auto worker = [&]()
                {
                    for (;;)
                    {
                        const std::size_t i = next.fetch_add(1, std::memory_order_relaxed);
                        if (i >= paths.size())
                        {
                            return;
                        }

                        if (auto hex = Sha256File(JoinPath(root, paths[i])))
                        {
                            digests[i] = *hex;
                        }

                        const std::size_t n = done.fetch_add(1, std::memory_order_relaxed) + 1;
                        if (progress)
                        {
                            // Serialised: the callback writes to the console, and the
                            // whole point of hashing in parallel is not to interleave
                            // the output of it.
                            std::lock_guard<std::mutex> lock(progressMutex);
                            progress(n, paths.size(), paths[i]);
                        }
                    }
                };

                std::vector<std::thread> pool;
                pool.reserve(threads - 1);
                for (unsigned t = 1; t < threads; ++t)
                {
                    pool.emplace_back(worker);
                }
                worker();
                for (std::thread& t : pool)
                {
                    t.join();
                }
            }
        }

        bool ReadManifest(const std::string& manifestPath,
                          std::vector<ManifestEntry>& out)
        {
            std::ifstream in(manifestPath, std::ios::binary);
            if (!in.good())
            {
                return false;
            }

            std::string line;
            while (std::getline(in, line))
            {
                // Tolerate CRLF: the manifest is written LF, but it travels with the
                // data set and a Windows editor or an archiver may have been over it.
                while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
                {
                    line.pop_back();
                }
                if (line.empty())
                {
                    continue;
                }

                // `<64 hex><two spaces><path>` -- the sha256sum layout, and anything
                // else is a line this did not write.
                if (line.size() < HEX_LENGTH + 3 || line[HEX_LENGTH] != ' ' ||
                    line[HEX_LENGTH + 1] != ' ')
                {
                    return false;
                }

                ManifestEntry entry;
                entry.sha256 = line.substr(0, HEX_LENGTH);
                if (entry.sha256.find_first_not_of("0123456789abcdef") != std::string::npos)
                {
                    return false;
                }
                entry.path = line.substr(HEX_LENGTH + 2);
                if (entry.path.empty())
                {
                    return false;
                }
                out.push_back(std::move(entry));
            }

            return true;
        }

        bool WriteManifest(const std::string& root,
                           const std::vector<std::string>& subdirs,
                           const ProgressFn& progress)
        {
            std::vector<std::string> paths;
            for (const std::string& subdir : subdirs)
            {
                if (!CollectFiles(root, subdir, paths))
                {
                    return false;
                }
            }

            // The manifest itself is never in its own manifest.
            paths.erase(std::remove(paths.begin(), paths.end(),
                                    std::string(MANIFEST_FILE_NAME)),
                        paths.end());
            std::sort(paths.begin(), paths.end());

            std::vector<std::string> digests;
            HashAll(root, paths, digests, 0, progress);

            std::vector<ManifestEntry> entries;
            entries.reserve(paths.size());
            for (std::size_t i = 0; i < paths.size(); ++i)
            {
                if (digests[i].empty())
                {
                    // A manifest listing only what could be read certifies the gaps.
                    return false;
                }
                entries.push_back(ManifestEntry{paths[i], digests[i]});
            }
            SortEntries(entries);

            // Written aside and renamed over: a manifest interrupted half way is worse
            // than no manifest at all, because the server would believe it and report
            // every unwritten file as missing.
            const std::string finalPath = JoinPath(root, MANIFEST_FILE_NAME);
            const std::string tempPath = finalPath + ".part";

            {
                std::ofstream out(tempPath, std::ios::binary | std::ios::trunc);
                if (!out.good())
                {
                    return false;
                }
                for (const ManifestEntry& e : entries)
                {
                    // '\n' with a binary stream on purpose: the file must be LF on every
                    // platform or the same data set hashes to two different manifests.
                    out << e.sha256 << "  " << e.path << '\n';
                }
                out.flush();
                if (!out.good())
                {
                    return false;
                }
            }

            std::error_code ec;
            std::filesystem::rename(tempPath, finalPath, ec);
            if (ec)
            {
                // Windows will not rename over an existing file on every filesystem.
                std::filesystem::remove(finalPath, ec);
                ec.clear();
                std::filesystem::rename(tempPath, finalPath, ec);
            }
            if (ec)
            {
                std::filesystem::remove(tempPath, ec);
                return false;
            }
            return true;
        }

        VerifyResult VerifyManifest(const std::string& root, unsigned threads,
                                    const ProgressFn& progress)
        {
            VerifyResult result;

            const std::string manifestPath = JoinPath(root, MANIFEST_FILE_NAME);
            std::error_code ec;
            if (!std::filesystem::exists(manifestPath, ec))
            {
                return result;
            }
            result.manifestFound = true;

            std::vector<ManifestEntry> entries;
            if (!ReadManifest(manifestPath, entries))
            {
                return result;
            }
            result.manifestReadable = true;
            result.listed = entries.size();

            std::vector<std::string> paths;
            paths.reserve(entries.size());
            for (const ManifestEntry& e : entries)
            {
                paths.push_back(e.path);
            }

            std::vector<std::string> digests;
            HashAll(root, paths, digests, threads, progress);

            for (std::size_t i = 0; i < entries.size(); ++i)
            {
                if (!digests[i].empty())
                {
                    if (digests[i] == entries[i].sha256)
                    {
                        ++result.verified;
                        continue;
                    }
                    ++result.changed;
                }
                else if (std::filesystem::exists(JoinPath(root, entries[i].path), ec))
                {
                    // Present but unreadable is its own answer: a permission problem or
                    // a failing disk is not the same report as a file somebody deleted.
                    ++result.unreadable;
                }
                else
                {
                    ++result.missing;
                }

                if (result.examples.size() < MAX_EXAMPLES)
                {
                    result.examples.push_back(entries[i].path);
                }
            }

            return result;
        }
    }
}

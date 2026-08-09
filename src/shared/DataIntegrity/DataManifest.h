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

#ifndef MANGOS_DATAMANIFEST_H
#define MANGOS_DATAMANIFEST_H

// WHAT THE BAKER WROTE, so the server can tell whether that is what it is reading.
//
// A tile carries a format version, which catches a bake from an older extractor. It
// cannot catch anything else: a tile truncated by a full disk or an interrupted copy, a
// map whose tiles came half from one bake and half from another, a stale file an older
// run left behind, or a sector that rotted. Every one of those loads as a well-formed
// tile of the right version and answers wrongly, or answers nothing, in one corner of
// one map -- which is exactly the failure nobody can reproduce.
//
// So the baker records a SHA-256 per file and the server checks them at start-up.
//
// FORMAT. One line per file, `<64 hex>  <path relative to the data directory>`, two
// spaces between, sorted by path, LF endings. That is deliberately the layout
// `sha256sum -c` reads, so an administrator can verify a data set with a tool that was
// not written here and does not have to be trusted:
//
//     cd <DataDir> && sha256sum -c data.manifest
//
// There is no header line and no version field, because a comment would break that
// property. The manifest is identified by its NAME and its location, and every file it
// covers carries its own format version inside it.
//
// NOT AUTHENTICATION. The manifest sits beside the files it describes, so whoever can
// replace a tile can replace the line about it. This detects damage, not an attacker.

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace MaNGOS
{
    namespace DataIntegrity
    {
        /// The manifest's file name, inside the data directory.
        extern const char* const MANIFEST_FILE_NAME;

        struct ManifestEntry
        {
            std::string path;       ///< relative to the data directory, '/' separated
            std::string sha256;     ///< 64 lower-case hex characters
        };

        /// Reports a file just finished. `done` counts files, not bytes.
        using ProgressFn = std::function<void(std::size_t done, std::size_t total,
                                              const std::string& path)>;

        /**
         * @brief Hash every file under @p subdirs of @p root and write the manifest.
         *
         * Writes to a temporary name and renames over the target, so a manifest never
         * exists in a half-written state -- which would be worse than none at all, since
         * the server would read it and report failures for files that are perfectly good.
         *
         * @return false if a directory could not be walked, a file could not be hashed,
         *         or the write failed. A manifest that lists only what it managed to
         *         read certifies the gaps.
         */
        bool WriteManifest(const std::string& root,
                           const std::vector<std::string>& subdirs,
                           const ProgressFn& progress = nullptr);

        struct VerifyResult
        {
            bool manifestFound = false;
            bool manifestReadable = false;
            std::size_t listed = 0;         ///< entries in the manifest
            std::size_t verified = 0;       ///< present and matching
            std::size_t missing = 0;        ///< listed but not on disk
            std::size_t changed = 0;        ///< present with a different digest
            std::size_t unreadable = 0;     ///< present but could not be read
            /// The first few offending paths, for a log line that names something.
            std::vector<std::string> examples;

            bool Ok() const
            {
                return manifestFound && manifestReadable && !missing && !changed &&
                       !unreadable;
            }
        };

        /**
         * @brief Re-hash everything the manifest lists and compare.
         *
         * @param threads how many files to hash at once; 0 asks the hardware. This is a
         *        gigabyte or two of reading at start-up, and it is embarrassingly
         *        parallel -- one file per worker, no shared state but the index.
         *
         * A file present on disk and NOT listed is not an error: a data directory holds
         * things the baker did not write (configs, a second realm's copy), and treating
         * those as corruption would make the check useless in exactly the deployments
         * that need it.
         */
        VerifyResult VerifyManifest(const std::string& root, unsigned threads = 0,
                                    const ProgressFn& progress = nullptr);

        /// Parses a manifest file. Exposed for the tests and for tooling.
        bool ReadManifest(const std::string& manifestPath,
                          std::vector<ManifestEntry>& out);
    }
}

#endif

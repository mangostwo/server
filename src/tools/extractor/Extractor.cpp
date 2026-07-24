/**
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

// The offline baker. One pass over a 3.3.5a client's MPQs, producing the caches
// mangosd reads: the raw DBC set, and the fused terrain + collision tiles.
//
// It shares -- does not copy -- the runtime's terrain engine, so the writer and the
// reader cannot disagree about the tile format.

#include "ExtractorConsole.hpp"
#include "nav/NavMeshBuilder.hpp"
#include "client/ModelLoaders.hpp"
#include "client/MpqTileSource.hpp"
#include "client/StormLibArchive.hpp"
#include "stores/LiquidTypeStore.hpp"
#include "stores/GameObjectDisplayInfoStore.hpp"
#include "stores/MapDbcStore.hpp"
#include "terrain/TileSerializer.hpp"

#include <chrono>
#include <cstdio>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

using namespace world::terrain;

namespace
{
    struct Options
    {
        std::string src = "Data";
        std::string dest = "cache";
        std::string locale = "enUS";
        int mapFilter = -1;
        bool dbc = false;
        bool tiles = false;
        bool goModels = false;
        bool nav = false;

        // Whether the command line named components itself. Naming them is an
        // instruction, so it suppresses the menu; leaving them out is a question, so
        // it asks one -- when there is a terminal to ask through.
        bool named = false;
        bool noMenu = false;
        std::string offMesh;
        int threads = 0;
    };

    ExtractorConsole g_console;
    std::chrono::steady_clock::time_point g_started;

    void Tick()
    {
        const auto now = std::chrono::steady_clock::now();
        g_console.SetElapsed(unsigned(
            std::chrono::duration_cast<std::chrono::seconds>(now - g_started).count()));
    }

    void Usage()
    {
        std::printf(
            "usage: mangos-extractor [dbc] [tile] [gomodels] [all] [options]\n"
            "  --src <dir>     client Data directory (default Data)\n"
            "  --dest <dir>    output root (default cache)\n"
            "  --locale <loc>  client locale folder (default enUS)\n"
            "  --map <id>      bake only this map id\n"
            "  --no-menu       never ask; bake everything not already named\n"
            "\n"
            "Naming no component opens the interactive menu. Without a terminal to\n"
            "open it on -- a pipe, a CI log -- everything is baked instead.\n");
    }

    bool ParseArgs(int argc, char** argv, Options& out)
    {
        for (int i = 1; i < argc; ++i)
        {
            const std::string a = argv[i];
            const bool hasValue = (i + 1 < argc);

            if (a == "dbc") { out.dbc = out.named = true; }
            else if (a == "tile") { out.tiles = out.named = true; }
            else if (a == "gomodels") { out.goModels = out.named = true; }
            else if (a == "nav") { out.nav = out.named = true; }
            else if (a == "all")
            {
                out.dbc = out.tiles = out.goModels = out.nav = out.named = true;
            }
            else if (a == "--src" && hasValue) { out.src = argv[++i]; }
            else if (a == "--dest" && hasValue) { out.dest = argv[++i]; }
            else if (a == "--locale" && hasValue) { out.locale = argv[++i]; }
            else if (a == "--map" && hasValue) { out.mapFilter = std::atoi(argv[++i]); }
            else if (a == "--offmesh" && hasValue) { out.offMesh = argv[++i]; }
            else if (a == "--threads" && hasValue) { out.threads = std::atoi(argv[++i]); }
            else if (a == "--no-menu") { out.noMenu = true; }
            else if (a == "-h" || a == "--help") { return false; }
            else
            {
                std::printf("unknown argument: %s\n", a.c_str());
                return false;
            }
        }

        return true;
    }

    int ExtractDbc(StormLibArchive& mpq, const std::string& dest,
                   const std::string& locale)
    {
        std::error_code ec;
        std::filesystem::create_directories(dest, ec);

        int written = 0;
        for (const std::string& name : mpq.FindFiles("DBFilesClient\\*.dbc"))
        {
            std::vector<uint8_t> bytes;
            if (!mpq.Read(name, bytes))
            {
                continue;
            }
            const size_t slash = name.find_last_of('\\');
            const std::string leaf = slash == std::string::npos ? name
                                                                : name.substr(slash + 1);
            if ((written % 16) == 0)
            {
                g_console.Activity("dbc " + leaf);
                Tick();
            }
            std::FILE* f = std::fopen((dest + "/" + leaf).c_str(), "wb");
            if (!f)
            {
                continue;
            }
            const bool ok = bytes.empty() ||
                            std::fwrite(bytes.data(), 1, bytes.size(), f) == bytes.size();
            std::fclose(f);
            written += ok ? 1 : 0;
        }
        // The build stamp. The server reads it to check the DBCs came from a client it
        // supports -- extract from the wrong expansion and the column layouts differ
        // with no other symptom. It is the client's own file, so it is copied, never
        // synthesised: a stamp this tool made up would assert exactly nothing.
        int stamps = 0;
        for (const std::string& name : mpq.FindFiles("component.wow-*.txt"))
        {
            std::vector<uint8_t> bytes;
            if (!mpq.Read(name, bytes))
            {
                continue;
            }
            const size_t slash = name.find_last_of("\/");
            std::string leaf = slash == std::string::npos ? name : name.substr(slash + 1);

            // The archive stores the locale lower-cased; the server fopen()s the name it
            // built from its own locale string. Windows does not care and Linux does, so
            // the stamp is written under the spelling the server will actually ask for.
            std::string lower = leaf;
            for (char& c : lower)
            {
                c = char(std::tolower(static_cast<unsigned char>(c)));
            }
            std::string wanted = "component.wow-" + locale + ".txt";
            std::string wantedLower = wanted;
            for (char& c : wantedLower)
            {
                c = char(std::tolower(static_cast<unsigned char>(c)));
            }
            if (lower == wantedLower)
            {
                leaf = wanted;
            }

            if (std::FILE* f = std::fopen((dest + "/" + leaf).c_str(), "wb"))
            {
                if (bytes.empty() ||
                    std::fwrite(bytes.data(), 1, bytes.size(), f) == bytes.size())
                {
                    ++stamps;
                }
                std::fclose(f);
            }
        }

        char msg[512];
        std::snprintf(msg, sizeof(msg), "dbc: %d files, %d build stamps -> %s", written,
                      stamps, dest.c_str());
        if (stamps)
        {
            g_console.Success(msg);
        }
        else
        {
            g_console.Warn(msg);
            g_console.Warn("  no component.wow-<locale>.txt in the client; the server "
                           "cannot check the DBC build");
        }
        return written;
    }

    // Every collidable game-object model, keyed by GameObjectDisplayInfo id: doors,
    // lifts, bridges. Written as ordinary one-instance tiles at identity, so the runtime
    // reads them with the same code that reads terrain.
    void BakeGoModels(WmoLoader& wmo, M2Loader& m2,
                      const world::GameObjectDisplayInfoStore& display,
                      const std::string& dest)
    {
        std::error_code ec;
        std::filesystem::create_directories(dest, ec);

        int written = 0, empty = 0;
        for (const auto& entry : display.All())
        {
            std::shared_ptr<const ICollisionModel> model =
                world::GameObjectDisplayInfoStore::IsWmo(entry.second)
                    ? wmo.Load(entry.second)
                    : m2.Load(entry.second);

            if (!model || model->Empty())
            {
                ++empty;
                continue;
            }

            if ((written % 32) == 0)
            {
                g_console.Activity("gomodel " + entry.second);
                Tick();
            }

            TerrainTile tile;
            StaticInstance inst;
            inst.model = model;
            inst.worldBounds = model->Bounds();
            tile.instances.push_back(std::move(inst));

            if (WriteTile(tile, dest + "/" + GoModelFileName(entry.first)))
            {
                ++written;
            }
        }
        char msg[512];
        std::snprintf(msg, sizeof(msg), "gomodels: %d written, %d without collision",
                      written, empty);
        g_console.Success(msg);
    }

    void NavProgress(void* ctx, uint32_t, const char* label, size_t done, size_t total)
    {
        (void)ctx;
        g_console.SetCounts(done, total);
        if (label && *label)
        {
            g_console.Activity(std::string("nav ") + label);
        }
        Tick();
    }

    // The navmesh is baked from the TILES, never from the MPQs, so the surface the
    // pathfinder walks is the one the collision engine answers with.
    bool BakeNav(const Options& opt, const std::string& tileDir)
    {
        if (!opt.nav)
        {
            return true;
        }

        g_console.SetStage("nav");

        world::nav::NavConfig cfg;
        cfg.threads = opt.threads;
        cfg.offMeshFile = opt.offMesh;

        world::nav::NavMeshBuilder builder(tileDir, opt.dest + "/mmaps", cfg);
        builder.SetProgress(&NavProgress, nullptr);

        const int written = builder.BakeAll(opt.mapFilter);
        if (written < 0)
        {
            g_console.Error("nav: bake failed; inspect the earlier diagnostics and " +
                            tileDir);
            return false;
        }

        char msg[256];
        std::snprintf(msg, sizeof(msg), "nav: %d mmtile files -> %s/mmaps", written,
                      opt.dest.c_str());
        g_console.Success(msg);
        return true;
    }

    // One map's tiles. A map is either an ADT grid or a single global WMO; both end up
    // as the same payload, so the runtime has nothing to reconcile.
    void BakeMap(MpqTileSource& source, uint32_t mapId, const std::string& name,
                 const std::string& dest)
    {
        const WdtData* wdt = source.Wdt(mapId);
        if (!wdt)
        {
            return;
        }

        if (!wdt->HasAnyAdt())
        {
            auto tile = source.Load(mapId, 0, 0);
            if (tile && tile->isGlobalWmo)
            {
                const bool ok =
                    WriteTile(*tile, dest + "/" + GlobalWmoFileName(mapId));
                char msg[256];
                std::snprintf(msg, sizeof(msg), "  map %4u %-24s global WMO %s", mapId,
                              name.c_str(), ok ? "ok" : "FAILED");
                if (ok) { g_console.Detail(msg); } else { g_console.Error(msg); }
            }
            return;
        }

        size_t expected = 0;
        for (int ty = 0; ty < 64; ++ty)
        {
            for (int tx = 0; tx < 64; ++tx)
            {
                expected += wdt->HasAdt(tx, ty) ? 1 : 0;
            }
        }

        int written = 0, failed = 0;
        for (int ty = 0; ty < 64; ++ty)
        {
            for (int tx = 0; tx < 64; ++tx)
            {
                if (!wdt->HasAdt(tx, ty))
                {
                    continue;
                }
                g_console.SetCounts(size_t(written), expected);
                Tick();
                auto tile = source.Load(mapId, tx, ty);
                if (!tile || !tile->hasTerrain)
                {
                    ++failed;
                    continue;
                }
                if (WriteTile(*tile, dest + "/" + TileFileName(mapId, tx, ty)))
                {
                    ++written;
                }
                else
                {
                    ++failed;
                }
            }
        }
        char msg[256];
        std::snprintf(msg, sizeof(msg), "  map %4u %-24s %5d tiles%s", mapId,
                      name.c_str(), written, failed ? " (SOME FAILED)" : "");
        if (failed) { g_console.Warn(msg); } else { g_console.Detail(msg); }
    }
}

int main(int argc, char** argv)
{
    Options opt;
    if (!ParseArgs(argc, argv, opt))
    {
        Usage();
        return 2;
    }

    opt.src = ExtractorConsole::ToUnixPath(opt.src);
    opt.dest = ExtractorConsole::ToUnixPath(opt.dest);

    g_started = std::chrono::steady_clock::now();
    g_console.Start(opt.src, opt.dest);

    // Ask by default, and only when asking is possible. A console that could not be
    // created is not an error here -- it is the ordinary case for a pipe or a CI log,
    // and it means the run falls back to the classic "bake everything" behaviour.
    if (!opt.named && !opt.noMenu && g_console.Active())
    {
        ExtractorConsole::Choice choice;
        if (!g_console.RunMenu(choice))
        {
            g_console.Stop();
            return 0;
        }
        opt.dbc = choice.dbc;
        opt.tiles = choice.tiles;
        opt.goModels = choice.goModels;
        opt.nav = choice.nav;
        if (choice.mapFilter >= 0)
        {
            opt.mapFilter = choice.mapFilter;
        }
        if (!choice.src.empty())
        {
            opt.src = choice.src;
        }
        if (!choice.dest.empty())
        {
            opt.dest = choice.dest;
        }
    }
    else if (!opt.named)
    {
        opt.dbc = opt.tiles = opt.goModels = opt.nav = true;
    }

    const std::string tileDir = opt.dest + "/tiles";

    // nav reads the baked tiles and never touches the client, so a nav-only run must
    // not require a client to be present at all.
    if (!opt.dbc && !opt.tiles && !opt.goModels)
    {
        const bool ok = BakeNav(opt, tileDir);
        g_console.SetStage("done");
        g_console.Progress(-1);
        g_console.Stop();
        return ok ? 0 : 1;
    }

    StormLibArchive mpq;
    const int opened = mpq.OpenClientData(opt.src, ClientArchives335a(),
                                          ClientLocaleArchives335a(), opt.locale);
    if (!opened)
    {
        g_console.Error("no client archives opened under " + opt.src);
        g_console.Stop();
        return 1;
    }
    {
        char msg[512];
        std::snprintf(msg, sizeof(msg), "opened %d archives from %s (%s)", opened,
                      opt.src.c_str(), opt.locale.c_str());
        g_console.Log(msg);
    }

    if (opt.dbc)
    {
        g_console.SetStage("dbc");
        ExtractDbc(mpq, opt.dest + "/dbc", opt.locale);
    }

    if (!opt.tiles && !opt.goModels)
    {
        g_console.SetStage("done");
        g_console.Progress(-1);
        g_console.Stop();
        return 0;
    }

    world::MapDbcStore maps;
    world::LiquidTypeStore liquids;
    if (!maps.LoadFromDbc(mpq) || !liquids.LoadFromDbc(mpq))
    {
        g_console.Error("Map.dbc or LiquidType.dbc could not be read");
        g_console.Stop();
        return 1;
    }

    std::error_code ec;
    std::filesystem::create_directories(tileDir, ec);

    if (opt.goModels)
    {
        g_console.SetStage("gomodels");
        world::GameObjectDisplayInfoStore display;
        if (display.LoadFromDbc(mpq))
        {
            WmoLoader wmo(mpq, &liquids);
            M2Loader m2(mpq);
            BakeGoModels(wmo, m2, display, opt.dest + "/gomodels");
        }
        else
        {
            g_console.Error("GameObjectDisplayInfo.dbc could not be read");
        }
    }

    if (!opt.tiles)
    {
        const bool ok = BakeNav(opt, tileDir);
        g_console.SetStage("done");
        g_console.Progress(-1);
        g_console.Stop();
        return ok ? 0 : 1;
    }

    g_console.SetStage("tiles");
    MpqTileSource source(mpq, &maps, &liquids);
    g_console.Log("tiles -> " + tileDir);

    for (const auto& entry : maps.All())
    {
        if (opt.mapFilter >= 0 && uint32_t(opt.mapFilter) != entry.first)
        {
            continue;
        }
        BakeMap(source, entry.first, entry.second, tileDir);
    }

    if (!BakeNav(opt, tileDir))
    {
        g_console.Stop();
        return 1;
    }

    g_console.SetStage("done");
    g_console.Progress(-1);
    Tick();
    g_console.Success("bake complete");
    g_console.Stop();
    return 0;
}

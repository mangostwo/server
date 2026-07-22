#pragma once

// The runtime terrain query engine for one map: a cache of baked tiles and the
// geometric questions the server asks of them.
//
// It is deliberately the ENGINE HALF only. It knows nothing of AreaTable, of
// WMOAreaTable, of liquid spells or of the navmesh -- everything that needs a DBC or
// the world lives above it, in the game. What comes back from here is geometry and the
// identifiers baked into the tile; translating those into MaNGOS's DBC world is the
// caller's job. That is what lets the offline probe tools ask the terrain exactly what
// mangosd asks it, and link none of the server to do so.

#include "terrain/Terrain.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>

namespace world::terrain
{
    class FusedTerrain
    {
    public:
        static constexpr int GRID_COUNT = FusedTerrainGridCount;

        explicit FusedTerrain(uint32_t mapId);

        FusedTerrain(const FusedTerrain&) = delete;
        FusedTerrain& operator=(const FusedTerrain&) = delete;

        uint32_t MapId() const { return m_mapId; }

        static void SetTileDir(const std::string& dir);
        static const std::string& TileDir();
        static bool HasTile(uint32_t mapId, int tx, int ty);

        // Highest floor at (x,y) at or just below z, over terrain and every static under
        // the column. searchUp starts the probe a little above z; maxDrop bounds how far
        // down a floor may be. False when nothing was found.
        bool GetHeight(float x, float y, float z, float& outZ, float searchUp = 2.0f,
                       float maxDrop = 200.0f) const;

        // What "how high is the ground here" means to the server. Not a plain GetHeight:
        // a query point often sits a little UNDER the surface -- a spawn buried a yard
        // into a hillside -- and a bare downward probe from z+2 misses the ground above
        // it and reports nothing. Hence the second pass.
        bool GetFloor(float x, float y, float z, float& outZ) const;

        // Liquid surface at (x,y) near z, from ADT liquid and WMO interior MLIQ alike.
        bool GetLiquid(float x, float y, float z, LiquidInfo& out) const;

        // Nearest static hit along a->b as a fraction of the segment; > 1 when nothing
        // blocks. A fraction rather than a point on purpose: the static and dynamic
        // worlds are queried over the same segment and the nearer fraction wins, so only
        // the winner is ever turned back into a point.
        //
        // The segment is used VERBATIM. Any agent-height lift belongs to the caller,
        // which already applies it; adding one here would make static line of sight more
        // permissive than the eyes it models, and would sample a door at a different
        // height than the doorway it sits in.
        float NearestHitFraction(float x1, float y1, float z1, float x2, float y2,
                                 float z2) const;

        bool IsInLineOfSight(float x1, float y1, float z1, float x2, float y2,
                             float z2) const;

        // AreaTable.dbc id of the MCNK chunk under (x,y), or 0 when unknown.
        uint16_t GetAreaId(float x, float y) const;

        // The WMO group whose floor is under (x,y) near z: the WMOAreaTable triple plus
        // the group's MOGP flags, with groundZ snapped to that floor. Deciding what the
        // flags MEAN needs WMOAreaTable and belongs to the caller.
        bool GetAreaInfo(float x, float y, float z, uint32_t& mogpFlags, int32_t& adtId,
                         int32_t& rootId, int32_t& groupId, float& groundZ) const;

        // Advances the cache clock and, once a minute, drops tiles that nothing pins and
        // no query has touched for a while. Without it the cache is monotonic: a player
        // walking a continent leaves every WMO he passed resident for the map's life.
        void Update(uint32_t diff);

        // Pins a cell's tile against the sweep while a grid is active.
        void PinCell(int tx, int ty);
        void UnpinCell(int tx, int ty);

        size_t ResidentTiles() const;

    private:
        using TilePtr = std::shared_ptr<const TerrainTile>;

        TilePtr TileAt(float x, float y) const;
        TilePtr GlobalWmo() const;
        TilePtr LoadCell(int tx, int ty) const;
        void EvictTile(int tx, int ty) const;

        void CollectSegmentInstances(const Vec3& a, const Vec3& b,
                                     std::vector<const StaticInstance*>& out,
                                     std::vector<TilePtr>& keepAlive) const;

        const uint32_t m_mapId;

        // Every query from every map-update thread goes through this cache, so the hit
        // path takes the lock SHARED: readers do not serialise against each other. All
        // instances of one dungeon share a single FusedTerrain, so they would otherwise
        // contend on one exclusive lock for every height, liquid and sight query.
        //
        // A null-but-probed entry is a memo that the map has no such tile; it costs one
        // byte and spares a failed file open per query, so the sweep keeps it.
        mutable std::array<std::array<TilePtr, GRID_COUNT>, GRID_COUNT> m_tiles;
        mutable std::array<std::array<uint8_t, GRID_COUNT>, GRID_COUNT> m_loaded{};
        mutable TilePtr m_globalWmo;
        mutable uint8_t m_globalWmoProbed = 0;
        mutable std::shared_mutex m_mutex;

        mutable std::array<std::array<std::atomic<uint32_t>, GRID_COUNT>, GRID_COUNT>
            m_tileLastUse{};
        std::atomic<uint32_t> m_clockMs{0};
        uint32_t m_sweepAccumMs = 0;

        std::array<std::array<int16_t, GRID_COUNT>, GRID_COUNT> m_cellRef{};
        mutable std::mutex m_cellRefMutex;
    };
}

#include <memory>
#include <string>
#include <vector>
#include "terrain/TileSerializer.hpp"
#include "terrain/CollisionModel.hpp"
#include "terrain/WmoModel.hpp"

#include <array>
#include <cmath>
#include <cstdio>
#include <unordered_map>

namespace world::terrain
{
    namespace
    {
        constexpr uint32_t MAGIC = 0x32474E4D;  // "MNG2" in file order
        // Version 2 is the same byte LAYOUT as 1 and a different set of accepted files.
        // The reader now requires each present grid to match its fixed dimensions, a WMO
        // group's liquid to match its own tile counts, a BVH node array to be a tree over
        // the triangles it indexes, and an instance to carry a finite pose -- all of which
        // a version 1 bake was free to violate. A tile that does is now REJECTED, and a
        // rejected tile is silence: no height, no liquid, no collision, no diagnostic
        // beyond the miss. The version is what turns "this map answers nothing" into
        // "this bake is stale, run the extractor".
        constexpr uint32_t VERSION = 2;

        constexpr uint32_t MAX_MODELS = 1u << 20;
        constexpr uint32_t MAX_INSTANCES = 1u << 22;
        // A WMO's group count is a uint32 in the file and a few hundred in reality. The
        // model ceiling is far too loose for it: it lets a corrupt header size a vector
        // of a million groups -- each one a Group with two vectors inside -- before any
        // per-group read measures anything against the file.
        constexpr uint32_t MAX_GROUPS = 4096;

        // A count is only believable if the file still holds that many elements. A fixed
        // ceiling is not enough: it still lets a corrupt header reserve hundreds of
        // megabytes before the short read is noticed, and on an overcommitting kernel
        // that reservation succeeds, so the guard never fires and the test that was
        // supposed to prove it stays green either way. Measuring against the actual file
        // makes the allocation impossible rather than merely unlikely.
        long RemainingBytes(std::FILE* f)
        {
            const long here = std::ftell(f);
            if (here < 0 || std::fseek(f, 0, SEEK_END) != 0)
            {
                return -1;
            }
            const long end = std::ftell(f);
            if (end < 0 || std::fseek(f, here, SEEK_SET) != 0)
            {
                return -1;
            }
            return end - here;
        }

        template <class T>
        bool WPod(std::FILE* f, const T& v)
        {
            return std::fwrite(&v, sizeof(T), 1, f) == 1;
        }

        template <class T>
        bool RPod(std::FILE* f, T& v)
        {
            return std::fread(&v, sizeof(T), 1, f) == 1;
        }

        template <class T>
        bool WVec(std::FILE* f, const std::vector<T>& v)
        {
            const uint32_t n = uint32_t(v.size());
            if (std::fwrite(&n, 4, 1, f) != 1)
            {
                return false;
            }
            return n == 0 || std::fwrite(v.data(), sizeof(T), n, f) == n;
        }

        template <class T>
        bool RVec(std::FILE* f, std::vector<T>& v)
        {
            uint32_t n = 0;
            if (std::fread(&n, 4, 1, f) != 1)
            {
                return false;
            }
            const long left = RemainingBytes(f);
            if (left < 0 || uint64_t(n) * sizeof(T) > uint64_t(left))
            {
                return false;
            }
            v.resize(n);
            return n == 0 || std::fread(v.data(), sizeof(T), n, f) == n;
        }

        template <class T, size_t N>
        bool WArray(std::FILE* f, const std::array<T, N>& a)
        {
            return std::fwrite(a.data(), sizeof(T), N, f) == N;
        }

        template <class T, size_t N>
        bool RArray(std::FILE* f, std::array<T, N>& a)
        {
            return std::fread(a.data(), sizeof(T), N, f) == N;
        }

        // MLIQ is a (tilesX+1) x (tilesY+1) grid of corner heights over a tilesX x tilesY
        // grid of flags. WmoModel::LiquidLocal interpolates the four corners around a
        // point with no bounds test, so a pair of counts that does not match the vectors
        // is an out-of-bounds read at query time -- long after the file was accepted.
        bool LiquidSizesOk(uint32_t tilesX, uint32_t tilesY,
                           const std::vector<float>& heights,
                           const std::vector<uint8_t>& flags)
        {
            if (tilesX == 0 || tilesY == 0)
            {
                return false;
            }
            const size_t wantHeights = size_t(tilesX + 1) * size_t(tilesY + 1);
            const size_t wantFlags = size_t(tilesX) * size_t(tilesY);
            return heights.size() == wantHeights && flags.size() == wantFlags;
        }

        // The height and liquid grids are indexed by fixed arithmetic over the tile's
        // constant dimensions -- TerrainHeight and LiquidAt never look at .size(). A
        // short vector read off disk is therefore not a wrong height, it is a read past
        // the end of the allocation.
        bool TerrainGridSizesOk(const TerrainTile& tile)
        {
            constexpr size_t V9 = size_t(V9_SIDE) * size_t(V9_SIDE);
            constexpr size_t V8 = size_t(GRID_PER_TILE) * size_t(GRID_PER_TILE);

            if (tile.hasTerrain && (tile.v9.size() != V9 || tile.v8.size() != V8))
            {
                return false;
            }
            if (tile.hasLiquid &&
                (tile.liquidHeight.size() != V9 || tile.liquidShow.size() != V8 ||
                 tile.liquidKind.size() != V8 || tile.liquidEntry.size() != V8 ||
                 tile.liquidDeep.size() != V8))
            {
                return false;
            }
            return true;
        }

        // The bake writes proper rotation matrices, so |det| is 1. Anything else is a
        // corrupt placement, and a non-finite one poisons every ray transformed by it.
        bool RotationOk(const Mat3& rot)
        {
            for (const float c : rot.m)
            {
                if (!std::isfinite(c))
                {
                    return false;
                }
            }
            const auto& m = rot.m;
            const float det = m[0] * (m[4] * m[8] - m[5] * m[7]) -
                              m[1] * (m[3] * m[8] - m[5] * m[6]) +
                              m[2] * (m[3] * m[7] - m[4] * m[6]);
            constexpr float eps = 1e-3f;
            return std::fabs(det - 1.0f) < eps || std::fabs(det + 1.0f) < eps;
        }

        bool WriteGroup(std::FILE* f, const WmoModel::Group& g)
        {
            bool ok = WPod(f, g.mogpFlags) && WPod(f, g.groupWmoId);
            const uint8_t hasLiquid = g.hasLiquid ? 1 : 0;
            ok = ok && WPod(f, hasLiquid);
            if (g.hasLiquid)
            {
                ok = ok && LiquidSizesOk(g.liquid.tilesX, g.liquid.tilesY,
                                         g.liquid.heights, g.liquid.flags);
                ok = ok && WPod(f, g.liquid.tilesX) && WPod(f, g.liquid.tilesY) &&
                     WPod(f, g.liquid.corner) && WPod(f, g.liquid.entry) &&
                     WPod(f, g.liquid.kind) && WVec(f, g.liquid.heights) &&
                     WVec(f, g.liquid.flags);
            }
            return ok;
        }

        bool ReadGroup(std::FILE* f, WmoModel::Group& g)
        {
            bool ok = RPod(f, g.mogpFlags) && RPod(f, g.groupWmoId);
            uint8_t hasLiquid = 0;
            ok = ok && RPod(f, hasLiquid);
            g.hasLiquid = hasLiquid != 0;
            if (ok && g.hasLiquid)
            {
                ok = RPod(f, g.liquid.tilesX) && RPod(f, g.liquid.tilesY) &&
                     RPod(f, g.liquid.corner) && RPod(f, g.liquid.entry) &&
                     RPod(f, g.liquid.kind) && RVec(f, g.liquid.heights) &&
                     RVec(f, g.liquid.flags);
                ok = ok && LiquidSizesOk(g.liquid.tilesX, g.liquid.tilesY,
                                         g.liquid.heights, g.liquid.flags);
            }
            return ok;
        }
    }

    std::string TileFileName(uint32_t mapId, int tx, int ty)
    {
        char name[64];
        std::snprintf(name, sizeof(name), "t_%u_%d_%d.tile", mapId, tx, ty);
        return name;
    }

    std::string GlobalWmoFileName(uint32_t mapId)
    {
        char name[64];
        std::snprintf(name, sizeof(name), "w_%u.tile", mapId);
        return name;
    }

    std::string GoModelFileName(uint32_t displayId)
    {
        char name[64];
        std::snprintf(name, sizeof(name), "go_%u.tile", displayId);
        return name;
    }

    bool WriteTile(const TerrainTile& tile, const std::string& path)
    {
        std::FILE* f = std::fopen(path.c_str(), "wb");
        if (!f)
        {
            return false;
        }

        const uint8_t hasTerrain = tile.hasTerrain ? 1 : 0;
        const uint8_t globalWmo = tile.isGlobalWmo ? 1 : 0;
        const uint8_t hasLiquid = tile.hasLiquid ? 1 : 0;

        bool ok = TerrainGridSizesOk(tile);
        ok = ok && WPod(f, MAGIC) && WPod(f, VERSION) && WPod(f, tile.tx) &&
                  WPod(f, tile.ty) && WPod(f, hasTerrain) && WPod(f, globalWmo) &&
                  WVec(f, tile.v9) && WVec(f, tile.v8) && WArray(f, tile.holes) &&
                  WArray(f, tile.areaIds) && WPod(f, hasLiquid) &&
                  WVec(f, tile.liquidHeight) && WVec(f, tile.liquidShow) &&
                  WVec(f, tile.liquidKind) && WVec(f, tile.liquidEntry) &&
                  WVec(f, tile.liquidDeep);

        // Deduped model table: a WMO instanced fifty times is written once and the
        // instances index it.
        std::unordered_map<const ICollisionModel*, uint32_t> modelIndex;
        std::vector<const ICollisionModel*> models;
        for (const StaticInstance& inst : tile.instances)
        {
            const ICollisionModel* m = inst.model.get();
            if (m && !modelIndex.count(m))
            {
                modelIndex[m] = uint32_t(models.size());
                models.push_back(m);
            }
        }

        const uint32_t nModels = uint32_t(models.size());
        ok = ok && WPod(f, nModels);
        for (const ICollisionModel* m : models)
        {
            const uint8_t kind = uint8_t(m->Kind());
            ok = ok && WPod(f, kind);
            if (m->Kind() == ModelKind::Wmo)
            {
                const auto* w = static_cast<const WmoModel*>(m);
                const uint32_t nGroups = uint32_t(w->Groups().size());
                ok = ok && WPod(f, w->RootId()) && WPod(f, nGroups);
                for (const WmoModel::Group& g : w->Groups())
                {
                    ok = ok && WriteGroup(f, g);
                }
                // soup.tris is already in the BVH's leaf order, so nothing is rebuilt.
                ok = ok && WVec(f, w->Soup().verts) && WVec(f, w->Soup().tris) &&
                     WVec(f, w->TriGroups()) && WVec(f, w->GetBvh().Nodes());
            }
            else
            {
                const auto* c = static_cast<const CollisionModel*>(m);
                ok = ok && WVec(f, c->Soup().verts) && WVec(f, c->Soup().tris) &&
                     WVec(f, c->GetBvh().Nodes());
            }
        }

        const uint32_t nInstances = uint32_t(tile.instances.size());
        ok = ok && WPod(f, nInstances);
        for (const StaticInstance& inst : tile.instances)
        {
            auto found = modelIndex.find(inst.model.get());
            const uint32_t idx = found != modelIndex.end() ? found->second : 0xFFFFFFFFu;
            ok = ok && WPod(f, inst.xf.pos) && WArray(f, inst.xf.rot.m) &&
                 WPod(f, inst.xf.scale) && WPod(f, inst.worldBounds.lo) &&
                 WPod(f, inst.worldBounds.hi) && WPod(f, idx) && WPod(f, inst.adtId);
        }

        // fclose is what flushes: a write that failed only in the buffer -- a full disk
        // is the ordinary way -- reports success here and leaves a truncated tile that
        // looks complete to every later reader.
        ok = (std::fclose(f) == 0) && ok;
        if (!ok)
        {
            std::remove(path.c_str());
        }
        return ok;
    }

    std::shared_ptr<TerrainTile> ReadTile(const std::string& path)
    {
        std::FILE* f = std::fopen(path.c_str(), "rb");
        if (!f)
        {
            return nullptr;
        }

        uint32_t magic = 0, version = 0;
        if (!RPod(f, magic) || !RPod(f, version) || magic != MAGIC || version != VERSION)
        {
            std::fclose(f);
            return nullptr;
        }

        auto tile = std::make_shared<TerrainTile>();
        uint8_t hasTerrain = 0, globalWmo = 0, hasLiquid = 0;

        bool ok = RPod(f, tile->tx) && RPod(f, tile->ty) && RPod(f, hasTerrain) &&
                  RPod(f, globalWmo) && RVec(f, tile->v9) && RVec(f, tile->v8) &&
                  RArray(f, tile->holes) && RArray(f, tile->areaIds) &&
                  RPod(f, hasLiquid) && RVec(f, tile->liquidHeight) &&
                  RVec(f, tile->liquidShow) && RVec(f, tile->liquidKind) &&
                  RVec(f, tile->liquidEntry) && RVec(f, tile->liquidDeep);

        tile->hasTerrain = hasTerrain != 0;
        tile->isGlobalWmo = globalWmo != 0;
        tile->hasLiquid = hasLiquid != 0;
        ok = ok && TerrainGridSizesOk(*tile);

        uint32_t nModels = 0;
        ok = ok && RPod(f, nModels) && nModels <= MAX_MODELS;

        std::vector<std::shared_ptr<const ICollisionModel>> models;
        if (ok)
        {
            models.resize(nModels);
        }

        for (uint32_t i = 0; ok && i < nModels; ++i)
        {
            uint8_t kind = 0;
            if (!RPod(f, kind))
            {
                ok = false;
                break;
            }

            TriSoup soup;
            std::vector<Bvh::Node> nodes;

            if (kind == uint8_t(ModelKind::Wmo))
            {
                uint32_t rootId = 0, nGroups = 0;
                ok = RPod(f, rootId) && RPod(f, nGroups) && nGroups <= MAX_GROUPS;
                std::vector<WmoModel::Group> groups(ok ? nGroups : 0);
                for (uint32_t g = 0; ok && g < nGroups; ++g)
                {
                    ok = ReadGroup(f, groups[g]);
                }

                std::vector<uint16_t> triGroup;
                ok = ok && RVec(f, soup.verts) && RVec(f, soup.tris) &&
                     RVec(f, triGroup) && RVec(f, nodes) &&
                     triGroup.size() == soup.tris.size() && soup.IndicesValid();

                Bvh bvh;
                ok = ok && bvh.Adopt(std::move(nodes), soup.tris.size());
                if (ok)
                {
                    models[i] = std::make_shared<WmoModel>(std::move(soup),
                                                           std::move(triGroup),
                                                           std::move(groups), rootId,
                                                           std::move(bvh));
                }
            }
            else if (kind == uint8_t(ModelKind::Mesh))
            {
                ok = RVec(f, soup.verts) && RVec(f, soup.tris) && RVec(f, nodes) &&
                     soup.IndicesValid();

                Bvh bvh;
                ok = ok && bvh.Adopt(std::move(nodes), soup.tris.size());
                if (ok)
                {
                    models[i] = std::make_shared<CollisionModel>(std::move(soup),
                                                                 std::move(bvh));
                }
            }
            else
            {
                ok = false;
            }
        }

        uint32_t nInstances = 0;
        ok = ok && RPod(f, nInstances) && nInstances <= MAX_INSTANCES;
        for (uint32_t i = 0; ok && i < nInstances; ++i)
        {
            StaticInstance inst;
            uint32_t idx = 0;
            float scale = 1.0f;
            ok = RPod(f, inst.xf.pos) && RArray(f, inst.xf.rot.m) &&
                 RPod(f, scale) && RPod(f, inst.worldBounds.lo) &&
                 RPod(f, inst.worldBounds.hi) && RPod(f, idx) && RPod(f, inst.adtId) &&
                 inst.xf.pos.isFinite() && inst.worldBounds.lo.isFinite() &&
                 inst.worldBounds.hi.isFinite() && RotationOk(inst.xf.rot);
            if (ok)
            {
                // Through the CLAMPING constructor, never by assigning the field. The
                // comment on that constructor says a non-positive or non-finite scale
                // is screened once, where a model is placed -- and this is the one place
                // a scale enters the server without passing through it. A zero read off
                // disk divides to infinity in worldToLocal, and every ray then misses
                // the model instead of failing.
                inst.xf = Transform(inst.xf.pos, inst.xf.rot, scale);
                if (idx < models.size())
                {
                    inst.model = models[idx];
                }
                tile->instances.push_back(std::move(inst));
            }
        }

        std::fclose(f);
        return ok ? tile : nullptr;
    }
}

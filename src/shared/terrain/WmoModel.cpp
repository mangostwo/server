#include <vector>
#include "terrain/WmoModel.hpp"

#include <algorithm>

namespace world::terrain
{
    namespace
    {
        constexpr float LIQUID_TILE_SIZE = 533.333f / 128.f;
    }

    WmoModel::WmoModel(TriSoup soup, std::vector<uint16_t> triGroup,
                       std::vector<Group> groups, uint32_t rootWmoId, Bvh bvh)
        : m_triGroup(std::move(triGroup)), m_groups(std::move(groups)), m_rootId(rootWmoId)
    {
        m_soup = std::move(soup);
        m_bvh = std::move(bvh);
        if (m_bvh.Empty() && !m_soup.tris.empty())
        {
            m_bvh.Build(m_soup, &m_triGroup, 4);
        }
        DeriveWmoBounds();
    }

    void WmoModel::DeriveWmoBounds()
    {
        DeriveBounds();

        // Liquid is content too, so a liquid-only group is not "empty", and its footprint
        // has to be inside the bounds for the column cull to ever reach it.
        for (const Group& g : m_groups)
        {
            if (!g.hasLiquid || g.liquid.heights.empty())
            {
                continue;
            }
            m_empty = false;

            float zmin = g.liquid.heights[0], zmax = g.liquid.heights[0];
            for (float hz : g.liquid.heights)
            {
                zmin = std::min(zmin, hz);
                zmax = std::max(zmax, hz);
            }
            const Vec3 c = g.liquid.corner;
            m_bounds.expand({c.x, c.y, zmin});
            m_bounds.expand({c.x + g.liquid.tilesX * LIQUID_TILE_SIZE,
                             c.y + g.liquid.tilesY * LIQUID_TILE_SIZE, zmax});
        }
    }

    std::optional<WmoModel::AreaResult> WmoModel::AreaInfo(const Vec3& origin, const Vec3& dir,
                                                           float tMax) const
    {
        uint32_t tri = 0;
        const auto t = m_bvh.Raycast(m_soup, origin, dir, tMax, &tri);
        if (!t || tri >= m_triGroup.size())
        {
            return std::nullopt;
        }
        const uint16_t gi = m_triGroup[tri];
        if (gi >= m_groups.size())
        {
            return std::nullopt;
        }
        return AreaResult{m_groups[gi].groupWmoId, m_groups[gi].mogpFlags, *t};
    }

    std::optional<ICollisionModel::LocalLiquid> WmoModel::LiquidLocal(const Vec3& p) const
    {
        for (const Group& g : m_groups)
        {
            if (!g.hasLiquid)
            {
                continue;
            }
            const Liquid& lq = g.liquid;
            if (!lq.tilesX || !lq.tilesY || lq.heights.empty())
            {
                continue;
            }

            // H() below indexes the corner grid unchecked, up to (tx+1, ty+1). That is
            // only in bounds if the heights really are (tilesX+1) x (tilesY+1) -- so a
            // group whose counts disagree with its vector is dropped rather than
            // sampled off the end. The serializer screens this on load too; here is
            // where the read would happen.
            if (lq.heights.size() != size_t(lq.tilesX + 1) * size_t(lq.tilesY + 1))
            {
                continue;
            }

            const float txf = (p.x - lq.corner.x) / LIQUID_TILE_SIZE;
            const float tyf = (p.y - lq.corner.y) / LIQUID_TILE_SIZE;
            // Stated POSITIVELY so a NaN falls out here rather than through: every
            // comparison against a NaN is false, so "not outside" would have let one
            // reach the conversion below.
            if (!(txf >= 0.f && txf < float(lq.tilesX)) ||
                !(tyf >= 0.f && tyf < float(lq.tilesY)))
            {
                continue;
            }
            // Ranged in FLOAT before the conversion, not after: int(txf) for a point far
            // outside the group -- which is most of the model, most of the time -- is a
            // conversion that does not fit, and that is undefined, not a big number.
            // Non-negative and in range here, so the truncation floors.
            const int tx = int(txf), ty = int(tyf);

            const size_t fi = size_t(tx) + size_t(ty) * lq.tilesX;
            if (fi < lq.flags.size() && (lq.flags[fi] & 0x0F) == 0x0F)
            {
                continue;
            }

            const float dx = txf - tx, dy = tyf - ty;
            const uint32_t row = lq.tilesX + 1;
            auto H = [&](int a, int b) { return lq.heights[size_t(a) + size_t(b) * row]; };

            float z;
            if (dx > dy)
            {
                const float sx = H(tx + 1, ty) - H(tx, ty);
                const float sy = H(tx + 1, ty + 1) - H(tx + 1, ty);
                z = H(tx, ty) + dx * sx + dy * sy;
            }
            else
            {
                const float sx = H(tx + 1, ty + 1) - H(tx, ty + 1);
                const float sy = H(tx, ty + 1) - H(tx, ty);
                z = H(tx, ty) + dx * sx + dy * sy;
            }

            LocalLiquid out;
            out.z = z;
            out.entry = lq.entry;
            out.kind = lq.kind;
            return out;
        }
        return std::nullopt;
    }
}

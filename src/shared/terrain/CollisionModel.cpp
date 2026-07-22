#include "terrain/CollisionModel.hpp"

namespace world::terrain
{
    CollisionModel::CollisionModel(TriSoup soup, Bvh bvh)
        : m_soup(std::move(soup)), m_bvh(std::move(bvh))
    {
        if (m_bvh.Empty() && !m_soup.tris.empty())
        {
            m_bvh.Build(m_soup, nullptr, 4);
        }
        DeriveBounds();
    }

    void CollisionModel::DeriveBounds()
    {
        m_bounds = Aabb{};
        m_empty = m_soup.tris.empty();
        for (uint32_t i = 0; i < m_soup.tris.size(); ++i)
        {
            m_bounds.expand(m_soup.TriBounds(i));
        }
    }

    std::optional<float> CollisionModel::RaycastNearest(const Vec3& origin, const Vec3& dir,
                                                        float tMax) const
    {
        return m_bvh.Raycast(m_soup, origin, dir, tMax);
    }
}

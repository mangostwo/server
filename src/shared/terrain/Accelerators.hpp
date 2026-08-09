#pragma once

// A triangle soup in model space and the binned-SAH BVH built over it. The BVH is
// built offline by the baker and stored in the tile verbatim, so the server never
// pays to construct one.

#include "terrain/Geometry.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace world::terrain
{
    struct TriSoup
    {
        std::vector<Vec3> verts;
        std::vector<std::array<uint32_t, 3>> tris;

        Tri At(uint32_t i) const
        {
            const auto& t = tris[i];
            return Tri{verts[t[0]], verts[t[1]], verts[t[2]]};
        }

        Aabb TriBounds(uint32_t i) const
        {
            const Tri t = At(i);
            Aabb b;
            b.expand(t.a);
            b.expand(t.b);
            b.expand(t.c);
            return b;
        }

        Vec3 Centroid(uint32_t i) const
        {
            const Tri t = At(i);
            return Vec3{(t.a.x + t.b.x + t.c.x) / 3.f, (t.a.y + t.b.y + t.c.y) / 3.f,
                        (t.a.z + t.b.z + t.c.z) / 3.f};
        }

        size_t Size() const { return tris.size(); }

        /// At() indexes verts blindly, because the query path cannot afford a bounds
        /// test per triangle. That is only sound if the soup was screened ONCE, when it
        /// came off disk: a bit-flip in a .tile otherwise becomes an out-of-bounds read
        /// inside a raycast, which is the one place nothing is watching.
        bool IndicesValid() const;
    };

    class Bvh
    {
    public:
        // Deepest node Build will create. Raycast's stack is sized from this so a push
        // can never overflow: the walk pops one node and pushes two, netting at most one
        // entry per level. Keep the two in step.
        static constexpr int MAX_DEPTH = 48;

        // POD, written raw into the tile: do not reorder or resize the fields.
        struct Node
        {
            Aabb box;
            // Children are allocated depth-first, so the right child is not left + 1.
            int32_t left = -1;
            int32_t right = -1;
            uint32_t first = 0;
            uint32_t count = 0;
        };

        // PERMUTES soup.tris so each leaf owns a contiguous run, which removes the
        // per-triangle indirection from the query entirely. `parallel`, when given, is
        // permuted elementwise alongside it.
        void Build(TriSoup& soup, std::vector<uint16_t>* parallel = nullptr, int leafSize = 4);

        std::optional<float> Raycast(const TriSoup& soup, const Vec3& o, const Vec3& d,
                                     float tMax, uint32_t* hitTri = nullptr) const;

        struct Crossing
        {
            float t = 0.f;
            uint32_t tri = 0;
        };

        // Every triangle the ray crosses, not just the first. A nearest-hit walk shrinks
        // its box test against the best t found so far and prunes the rest of the column;
        // this one cannot, so it is the more expensive of the two and exists for the one
        // question that needs the whole column rather than its top.
        void RaycastAll(const TriSoup& soup, const Vec3& o, const Vec3& d, float tMax,
                        std::vector<Crossing>& out) const;

        const std::vector<Node>& Nodes() const { return m_nodes; }

        /**
         * @brief Take a node array read from disk, after checking it really is a tree.
         *
         * Nothing downstream re-checks. Raycast pushes both children unconditionally
         * onto a stack sized from MAX_DEPTH, indexes m_nodes with whatever it popped,
         * and reads a leaf's triangle run without a bounds test. All three are safe
         * only because Build cannot emit a node array that violates them -- a FILE can,
         * whether corrupt or hostile, and the tile format has no checksum.
         *
         * The check requires each child's index to exceed its parent's. That is the
         * layout Build emits (the parent is appended before either child) and it is
         * therefore part of the tile format, not an implementation detail: it makes a
         * cycle unrepresentable, which is what lets the depth pass be one forward sweep.
         *
         * @return false when the array is not a depth-first tree over @p triangleCount
         *         triangles; the tree is left empty and the caller must reject the tile.
         */
        bool Adopt(std::vector<Node> nodes, size_t triangleCount);

        size_t NodeCount() const { return m_nodes.size(); }
        int MaxDepth() const { return m_maxDepth; }
        bool Empty() const { return m_nodes.empty(); }

    private:
        int BuildNode(const TriSoup& soup, std::vector<uint32_t>& order, uint32_t first,
                      uint32_t count, int leafSize, int depth);

        std::vector<Node> m_nodes;
        int m_maxDepth = 0;
    };
}

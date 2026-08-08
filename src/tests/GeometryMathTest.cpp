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

// The degenerate inputs the geometry primitives are actually handed at runtime, and
// which no ordinary play ever produces: a zero-length direction, a zero quaternion,
// an orientation a client reported as NaN, an empty bounding box folded into an
// accumulator. Every one of them used to answer with NaN or infinity, and NaN is the
// worst possible failure here because every comparison against it is false -- nothing
// reports an error, a ray simply misses and a creature simply stands in the wrong place.

#include "TestHarness.h"

#include "Geometry/GeometryMath.h"
#include "Geometry/Quat.h"
#include "Geometry/Shapes.h"
#include "Geometry/Vector3.h"

#include <cmath>
#include <limits>

static const float kNan = std::numeric_limits<float>::quiet_NaN();
static const float kInf = std::numeric_limits<float>::infinity();

static bool Near(float a, float b, float tol = 1e-5f)
{
    return std::fabs(a - b) < tol;
}

TEST(GeometryMath_WrapKeepsAnOrdinaryOrientationExactly)
{
    const float twoPi = 2.0f * Geometry::pif();

    CHECK(Near(Geometry::wrap(1.0f, 0.0f, twoPi), 1.0f));
    CHECK(Near(Geometry::wrap(0.0f, 0.0f, twoPi), 0.0f));
    CHECK(Near(Geometry::wrap(-1.0f, 0.0f, twoPi), twoPi - 1.0f));
    CHECK(Near(Geometry::wrap(twoPi + 1.0f, 0.0f, twoPi), 1.0f));
}

TEST(GeometryMath_WrapSurvivesWhatAClientCanSend)
{
    // MoveSpline normalises the orientation the CLIENT reports, so these are reachable
    // from a movement packet. The old form computed int(floor(x)) on the quotient:
    // undefined behaviour for a NaN and for anything past 2^31 intervals, not merely a
    // wrong angle. Nothing here asserts a value for the huge case -- the cancellation
    // makes it meaningless -- only that it is a finite number and not a trap.
    const float twoPi = 2.0f * Geometry::pif();

    CHECK(Geometry::wrap(kNan, 0.0f, twoPi) == 0.0f);
    CHECK(Geometry::wrap(kInf, 0.0f, twoPi) == 0.0f);
    CHECK(Geometry::wrap(-kInf, 0.0f, twoPi) == 0.0f);
    CHECK(std::isfinite(Geometry::wrap(1e30f, 0.0f, twoPi)));
    CHECK(std::isfinite(Geometry::wrap(-1e30f, 0.0f, twoPi)));

    // A degenerate interval has no half-open range to land in; it fails closed to lo.
    CHECK(Geometry::wrap(3.0f, 1.0f, 1.0f) == 1.0f);
    CHECK(Geometry::wrap(3.0f, 1.0f, 0.0f) == 1.0f);
}

TEST(Geometry_ZeroVectorHasNoDirection)
{
    const Geometry::Vector3 zero(0.0f, 0.0f, 0.0f);
    const Geometry::Vector3 dir = zero.direction();

    CHECK(dir.isFinite());
    CHECK(dir.isZero());

    const Geometry::Vector3 down(0.0f, 0.0f, -3.0f);
    CHECK(down.unit().isUnit());
    CHECK(Near(down.unit().z, -1.0f));

    const Geometry::Vector3 broken(kNan, 0.0f, 0.0f);
    CHECK(broken.direction().isFinite());
}

TEST(Geometry_ZeroQuaternionUnitizesToTheIdentity)
{
    Geometry::Quat zero(0.0f, 0.0f, 0.0f, 0.0f);
    zero.unitize();

    CHECK(zero.isUnit());
    CHECK(Near(zero.w, 1.0f));
    CHECK(Near(zero.imag().x, 0.0f));

    Geometry::Quat scaled(0.0f, 0.0f, 2.0f, 2.0f);
    scaled.unitize();
    CHECK(scaled.isUnit());
}

TEST(Geometry_AnEmptyBoxDoesNotPoisonAnAccumulator)
{
    // The empty box is lo = +max, hi = -max. Folding one into an accumulator used to
    // widen it to the whole float range, whose extent overflows to infinity -- after
    // which every SAH split of the BVH costs the same and the tree stops separating.
    Geometry::Aabb acc;
    CHECK(!acc.valid());

    acc.expand(Geometry::Vector3(0.0f, 0.0f, 0.0f));
    acc.expand(Geometry::Vector3(10.0f, 4.0f, 2.0f));
    CHECK(acc.valid());

    acc.expand(Geometry::Aabb{});
    CHECK(acc.valid());
    CHECK(Near(acc.lo.x, 0.0f));
    CHECK(Near(acc.hi.x, 10.0f));
    CHECK(std::isfinite(acc.hi.z - acc.lo.z));

    // A box that has seen a point on no axis is valid on none of them.
    Geometry::Aabb empty;
    CHECK(!empty.valid());
}

TEST(Geometry_ADegenerateRayMatchesNoBox)
{
    // A direction of zero length normalises to NaN, and 1/NaN is NaN. Every comparison
    // in the slab test against a NaN is false, so the axis used to be dropped silently
    // and the node accepted -- for every origin, however far outside the box.
    Geometry::Aabb box;
    box.expand(Geometry::Vector3(0.0f, 0.0f, 0.0f));
    box.expand(Geometry::Vector3(10.0f, 10.0f, 10.0f));

    const Geometry::Vector3 farAway(1000.0f, 1000.0f, 1000.0f);
    const Geometry::Vector3 nanInvDir(kNan, kNan, kNan);
    CHECK(!box.intersectsRay(farAway, nanInvDir, 10000.0f));

    // A ray straight down THROUGH the box still hits: two of its reciprocals are
    // infinite, and the padding must keep answering yes.
    const Geometry::Vector3 above(5.0f, 5.0f, 100.0f);
    const Geometry::Vector3 downInvDir(kInf, kInf, -1.0f);
    CHECK(box.intersectsRay(above, downInvDir, 200.0f));

    // The same ray beside the box, on an axis whose reciprocal is infinite, misses.
    const Geometry::Vector3 beside(50.0f, 5.0f, 100.0f);
    CHECK(!box.intersectsRay(beside, downInvDir, 200.0f));
}

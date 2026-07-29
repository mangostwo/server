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

#include "Utilities/MathDefines.h"
#include "RandomMovementGenerator.h"
#include "Creature.h"
#include "Log.h"
#include "MotionFrame.h"
#include "Util.h"

#include <algorithm>
#include <cmath>

namespace
{
    /// Percent chance the creature does not pause at all between hops, so a wandering
    /// mob occasionally strings two legs together instead of always resting.
    constexpr int CHANCE_NO_BREAK = 30;

    /// A leash radius below this is meaningless and would make every hop degenerate.
    constexpr float MIN_WANDER_RADIUS = 0.1f;

    constexpr uint32 REST_AFTER_HOP_MIN = 3000;
    constexpr uint32 REST_AFTER_HOP_MAX = 10000;

    /// Retry delay after a hop that could not be routed, or a point that could not be
    /// found. Short enough to look alive, long enough not to hammer the router.
    constexpr uint32 RETRY_DELAY = 50;

    /// Clearance kept between a flying hop and the floor beneath it, so a downward
    /// drift cannot end up inside the terrain (or inside a deck).
    constexpr float MIN_FLIGHT_CLEARANCE = 0.5f;

    /// How far around the orbit one leg carries the creature. Bounded at both ends: too
    /// small and it crawls, too large and the sine between two legs steps far enough to
    /// read as a kink rather than a glide.
    constexpr float ORBIT_STEP_MIN = 0.45f;   // ~26 degrees
    constexpr float ORBIT_STEP_MAX = 1.15f;   // ~66 degrees

    /// The orbit does not use the whole leash every lap, or every flier would trace the
    /// same rigid circle.
    constexpr float ORBIT_RADIUS_MIN_FACTOR = 0.55f;
}

RandomMovementGenerator::RandomMovementGenerator(float x, float y, float z, float radius,
                                                 float verticalZ)
    : m_centre(x, y, z), m_radius(radius), m_verticalZ(verticalZ)
{
    if (m_radius < MIN_WANDER_RADIUS)
    {
        DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS,
                         "RandomMovementGenerator: wander radius too small, clamped to %f",
                         MIN_WANDER_RADIUS);
        m_radius = MIN_WANDER_RADIUS;
    }
}

RandomMovementGenerator::RandomMovementGenerator(Creature const& creature)
{
    float x, y, z, o, wanderDistance;
    const Geometry::Placement& spawnPose = creature.Spawn();
    x = spawnPose.X();
    y = spawnPose.Y();
    z = spawnPose.Z();
    o = spawnPose.Facing();
        wanderDistance = creature.GetRespawnRadius();;

    m_centre = Motion::Vector3(x, y, z);
    m_radius = std::max(wanderDistance, MIN_WANDER_RADIUS);
}

bool RandomMovementGenerator::Airborne(Unit& owner) const
{
    // CanFly() is the right question rather than the InhabitType bit alone: it already
    // means "air inhabit type, OR the fly animation, OR levitating/can-fly movement
    // flags", which is every way a creature is actually off the ground.
    return m_verticalZ > 0.0f &&
           owner.GetTypeId() == TYPEID_UNIT &&
           static_cast<Creature&>(owner).CanFly();
}

/**
 * @brief The next point on an INCLINED ELLIPSE around the leash centre.
 *
 * Picking a fresh random height for every hop is what makes a flier stagger: two legs in
 * a row can be up-then-down, and the result is a zig-zag with a corner at every waypoint.
 *
 * So the height is not random at all. One angle advances around the leash, and both the
 * horizontal position and the height are read off it:
 *
 *     x = cx + R*cos(t),  y = cy + R*sin(t),  z = cz + verticalZ*sin(t - tilt)
 *
 * A circle in xy with a sinusoidal z of the SAME period is exactly the intersection of a
 * cylinder with a tilted plane -- an ellipse lying in an inclined plane. Because the
 * angle only ever advances, and by a bounded step, the height moves continuously along
 * that ellipse: it rises for half a lap and falls for the other half, and can never jump.
 * `tilt` is drawn once per creature, so each flier gets its own orbital plane instead of
 * every one of them banking the same way.
 */
Motion::Vector3 RandomMovementGenerator::NextOrbitPoint(Unit& owner)
{
    m_orbitAngle += frand(ORBIT_STEP_MIN, ORBIT_STEP_MAX);
    if (m_orbitAngle > 2 * M_PI_F)
    {
        m_orbitAngle -= 2 * M_PI_F;
    }

    const float r = m_radius * frand(ORBIT_RADIUS_MIN_FACTOR, 1.0f);

    Motion::Vector3 p(m_centre.x + r * std::cos(m_orbitAngle),
                      m_centre.y + r * std::sin(m_orbitAngle),
                      m_centre.z + m_verticalZ * std::sin(m_orbitAngle - m_orbitTilt));

    // Never below the floor under that point. GroundPoint answers in whatever frame the
    // creature is moving in, so this keeps a flier above the terrain at sea and above the
    // DECK when it is orbiting over a ship.
    Motion::IMotionFrame const& frame = Motion::FrameFor(owner);
    if (const auto floor = frame.GroundPoint(owner, frame.MoverPosition(owner), p))
    {
        p.z = std::max(p.z, floor->z + MIN_FLIGHT_CLEARANCE);
    }

    return p;
}

void RandomMovementGenerator::Initialize(Unit& owner)
{
    // _MOVE is set once a hop is actually picked.
    owner.addUnitState(UNIT_STAT_ROAMING);

    // Each flier banks its own way; drawn once so the plane stays put for its lifetime.
    m_orbitTilt = frand(0.0f, 2 * M_PI_F);
    m_orbitAngle = frand(0.0f, 2 * M_PI_F);

    m_restTime.Reset(0);
    m_haveHop = false;
    ResetLeg();
}

void RandomMovementGenerator::Reset(Unit& owner)
{
    Initialize(owner);
}

void RandomMovementGenerator::Interrupt(Unit& owner)
{
    owner.InterruptMoving();
    Finalize(owner);
    m_haveHop = false;
    ResetLeg();
}

void RandomMovementGenerator::Finalize(Unit& owner)
{
    owner.clearUnitState(UNIT_STAT_ROAMING | UNIT_STAT_ROAMING_MOVE);
    static_cast<Creature&>(owner).SetWalk(!owner.hasUnitState(UNIT_STAT_RUNNING_STATE), false);
}

Motion::MoveIntent RandomMovementGenerator::Intent(Unit& owner,
                                                   Motion::MoveStatus const& status,
                                                   uint32 diff)
{
    // A flier glides through the air: routing the hop would drag it back down onto the
    // navmesh, which is the whole thing a vertical wander is not. MOVE_FLY also puts the
    // spline on Catmull-Rom, so the client rounds the corners between legs and the orbit
    // reads as one continuous curve rather than a chain of segments.
    const bool airborne = Airborne(owner);
    const uint32 hopFlags = airborne
        ? (Motion::MOVE_FLY | Motion::MOVE_STRAIGHT)
        : Motion::MOVE_WALK;

    if (!owner.IsAlive() || owner.hasUnitState(UNIT_STAT_NOT_MOVE))
    {
        m_restTime.Reset(0);
        owner.clearUnitState(UNIT_STAT_ROAMING_MOVE);
        return Motion::MoveIntent::Hold();
    }

    // The point we picked turned out to be unreachable: try somewhere else shortly,
    // rather than hammering the router every tick.
    if (status.blocked)
    {
        m_haveHop = false;
        m_restTime.Reset(RETRY_DELAY);
    }

    // Mid-hop: re-state the same goal, which the driver recognises as the leg it is
    // already walking and leaves alone.
    if (status.traveling && m_haveHop)
    {
        return Motion::MoveIntent::Move(m_hop, hopFlags);
    }

    // Standing: run down the rest timer.
    m_restTime.Update(diff);
    if (!m_restTime.Passed())
    {
        return Motion::MoveIntent::Hold();
    }

    if (airborne)
    {
        // A flier is not looking for a reachable spot on the floor -- it owns the air
        // above the leash, so its next point comes off the orbit rather than the navmesh.
        m_hop = NextOrbitPoint(owner);
    }
    else
    {
        const auto hop = Motion::FrameFor(owner).RandomPoint(owner, m_centre, m_radius);
        if (!hop)
        {
            m_restTime.Reset(RETRY_DELAY);
            return Motion::MoveIntent::Hold();
        }

        m_hop = *hop;
    }

    owner.addUnitState(UNIT_STAT_ROAMING_MOVE);
    m_haveHop = true;

    // The rest that follows THIS hop is decided now: the timer only runs while the
    // creature is standing, so it starts counting the moment the leg ends.
    //
    // A flier does not rest. Pausing between arcs would stop it dead in mid-air at the
    // end of every leg, which is the same visible break the orbit exists to remove -- so
    // it goes straight into the next arc and the whole path stays one continuous glide.
    m_restTime.Reset((airborne || roll_chance_i(CHANCE_NO_BREAK))
        ? RETRY_DELAY
        : urand(REST_AFTER_HOP_MIN, REST_AFTER_HOP_MAX));

    return Motion::MoveIntent::Move(m_hop, hopFlags);
}

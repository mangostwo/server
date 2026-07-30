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

/**
 * @file WorldObjectPosition.cpp
 * @brief Cohesion split of Object.cpp -- WorldObject position/orientation, distance/angle/arc queries, LoS, and ground-Z helpers.
 *        Same classes; no behaviour change.
 */

#include <cmath>
#include "Utilities/MathDefines.h"
#include "Object.h"
#include "SharedDefines.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "Log.h"
#include "World.h"
#include "Creature.h"
#include "Player.h"
#include "Vehicle.h"
#include "ObjectMgr.h"
#include "ObjectGuid.h"
#include "UpdateData.h"
#include "UpdateMask.h"
#include "Util.h"
#include "MapManager.h"
#include "Log.h"
#include "Transports.h"
#include "TransportMap.h"
#include "TargetedMovementGenerator.h"
#include "WaypointMovementGenerator.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "ObjectPosSelector.h"
#include "TemporarySummon.h"
#include "movement/packet_builder.h"
#include "CreatureLinkingMgr.h"
#include "Chat.h"
#include "GameTime.h"
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#include "ElunaConfig.h"
#include "ElunaEventMgr.h"
#endif /* ENABLE_ELUNA */

/**
 * @brief Cleanups before delete
 *
 * Removes the object from the world before deletion.
 */
void WorldObject::CleanupsBeforeDelete()
{
    RemoveFromWorld();
}

/**
 * @brief Update world object
 * @param update_diff Time since last update
 * @param time_diff Time parameter (unused)
 *
 * Updates Eluna events if enabled.
 */
void WorldObject::Update(uint32 update_diff, uint32 time_diff)
{
#ifdef ENABLE_ELUNA
    if (elunaEvents) // can be null on maps without eluna
    {
        elunaEvents->Update(update_diff);
    }
#endif /* ENABLE_ELUNA */
}

/**
 * @brief Create world object
 * @param guidlow Low GUID
 * @param guidhigh High GUID type
 *
 * Creates the world object with the specified GUID.
 */
void WorldObject::_Create(uint32 guidlow, HighGuid guidhigh, uint32 phaseMask)
{
    Object::_Create(guidlow, 0, guidhigh);
    m_phaseMask = phaseMask;
}

/**
 * @brief Get instance data
 * @return Instance data pointer
 *
 * Returns the instance data for the map this object is on.
 */
InstanceData* WorldObject::GetInstanceData() const
{
    return GetMap()->GetInstanceData();
}

/**
 * @brief Get random point near position
 * @param x Center X coordinate
 * @param y Center Y coordinate
 * @param z Center Z coordinate
 * @param distance Maximum distance from center
 * @param rand_x Output random X coordinate
 * @param rand_y Output random Y coordinate
 * @param rand_z Output random Z coordinate
 * @param minDist Minimum distance from center
 * @param ori Optional orientation to use instead of random
 *
 * Generates a random point within the specified distance
 * of the center position.
 */
Geometry::Vector3 RandomGroundPointNear(WorldObject const& obj, Geometry::Vector3 const& centre,
                                       float distance, float minDist, float const* ori)
{
    if (distance == 0.0f)
    {
        return centre;
    }

    const float angle = ori ? *ori : (rand_norm_f() * Geometry::Placement::TwoPi());

    Geometry::Placement around;
    around.EnterFrame(obj.Where().CurrentFrame(), centre, angle);

    Geometry::Vector3 point = around.RandomPointAround(minDist, distance, angle, rand_norm_f());
    MaNGOS::NormalizeMapCoord(point.x);
    MaNGOS::NormalizeMapCoord(point.y);
    DropToGround(obj, point.x, point.y, point.z);
    return point;
}


/**
 * @brief Update ground position Z
 * @param x X coordinate
 * @param y Y coordinate
 * @param z Z-coordinate to update
 *
 * Updates the Z-coordinate to the ground height at the
 * specified position.
 */
void DropToGround(WorldObject const& obj, float x, float y, float& z)
{
    if (auto floor = obj.GetMap()->Floor(obj.GetPhaseMask(), x, y, z))
    {
        z = *floor + 0.05f;                                  // just to be sure that we are not a few pixel under the surface
    }
}

/**
 * @brief Update allowed position Z
 * @param x X coordinate
 * @param y Y coordinate
 * @param z Z-coordinate to update
 * @param atMap Map to use for height calculation (optional)
 *
 * Updates the Z-coordinate to a valid height based on the
 * object's movement capabilities (flying, swimming, etc.).
 */
void ClampToAllowedZ(WorldObject const& obj, float x, float y, float& z, Map* atMap /*=NULL*/)
{
    if (!atMap)
    {
        atMap = obj.GetMap();
    }

    const auto floor = atMap->Floor(obj.GetPhaseMask(), x, y, z);
    if (!floor)
    {
        return;
    }

    // Anything that is not a unit has no say in the matter: it sits on the floor.
    const bool isUnit = obj.GetTypeId() == TYPEID_UNIT || obj.GetTypeId() == TYPEID_PLAYER;
    if (!isUnit)
    {
        z = *floor;
        return;
    }

    const Unit& unit = static_cast<const Unit&>(obj);
    if (unit.CanFly())
    {
        if (z < *floor)
        {
            z = *floor;
        }
        return;
    }

    // Held between the floor and the highest surface this unit may occupy: the water it
    // can swim in, or the floor itself when it cannot.
    float ceiling = *floor;
    if (unit.CanSwim())
    {
        ceiling = atMap->GetTerrain()->GetWaterOrGroundLevel(
                      x, y, z, NULL, !unit.HasAuraType(SPELL_AURA_WATER_WALK));
    }

    if (z > ceiling)
    {
        z = ceiling;
    }
    else if (z < *floor)
    {
        z = *floor;
    }
}

// ---- not geometry, so neither the object's nor the component's --------------
//
// Phasing and world membership are game state; line of sight and a map's coordinate
// bounds are the terrain engine's. Each of these asks the placement for the geometry and
// contributes only the part the placement must never know about.

/**
 * @brief The frame both objects can be answered for, if one exists.
 *
 * Two passengers of the same vessel are ordinary neighbours in its deck frame, whichever
 * way each came to be aboard -- a crew member carries its offset, a player's client sends
 * one. Anything else pairs a deck with a map, and there is no composition that would give
 * those two a common frame: the server does not know where the hull is.
 *
 * Nothing when no such frame exists, which is what makes a world distance to a passenger
 * unanswerable rather than merely wrong.
 */
static bool InCommonFrame(WorldObject const& a, WorldObject const& b,
                          Geometry::Placement& outA, Geometry::Placement& outB)
{
    // THE FRAME IS THE AUTHORITY, not the passenger registry. Two things on the same deck
    // map already speak the same coordinates whether or not either was ever boarded -- a
    // creature summoned straight onto a deck is exactly that, and asking the roster about
    // it answers no while the geometry answers yes.
    if (a.Where().ShareFrame(b.Where()))
    {
        outA = a.Where();
        outB = b.Where();
        return true;
    }

    TransportMap* va = a.GetMap() ? a.GetMap()->AsTransport() : NULL;
    TransportMap* vb = b.GetMap() ? b.GetMap()->AsTransport() : NULL;

    if (!va && !vb)
    {
        outA = a.Where();
        outB = b.Where();
        return true;
    }

    if (va != vb)
    {
        return false;
    }

    const auto la = va->PositionOf(a);
    const auto lb = va->PositionOf(b);
    if (!la || !lb)
    {
        return false;
    }

    outA = *la;
    outB = *lb;
    return true;
}

/**
 * @brief CAN A REACH B AT ALL -- the question every melee swing, spell, threat entry and
 *        aggro check is really asking.
 *
 * It demands a COMMON FRAME, and that is the whole point: there is no distance between a
 * deck and the sea it sails over, so nothing can be measured across that boundary and
 * nothing may reach across it. Two passengers of one vessel do share a frame -- its deck --
 * and are ordinary neighbours in it.
 *
 * This is NOT the question "can B see A". Seeing a crow overhead is not being able to hit
 * it, and on a vessel the two come apart completely: the shore sees a deckhand it cannot
 * touch, and a passenger sees a harbour he cannot reach. For that, ask CanBeSeen.
 */
bool CanInteract(WorldObject const& a, WorldObject const& b)
{
    Geometry::Placement pa, pb;
    return a.IsInWorld() && b.IsInWorld() && a.InSamePhase(&b) &&
           InCommonFrame(a, b, pa, pb) && pa.ShareFrame(pb);
}

/**
 * @brief CAN B BE SHOWN A -- a wider question, and a cheaper one.
 *
 * Same frame, as usual. But also across a vessel's boundary: a deck and the map that
 * vessel is sailing can see each other in both directions, because the vessel relays them
 * -- the shore is handed the deck's contents, and the deck is handed the shore's. Neither
 * side can touch the other, and neither needs to in order to be drawn.
 *
 * It is also what lets a passenger see the hull he is standing on: the vessel lives on the
 * map it sails, he lives on its deck, and they never share a frame at all.
 */
bool CanBeSeen(WorldObject const& seen, WorldObject const& viewer)
{
    if (!seen.IsInWorld() || !viewer.IsInWorld() || !seen.InSamePhase(&viewer))
    {
        return false;
    }

    if (seen.Where().ShareFrame(viewer.Where()))
    {
        return true;
    }

    if (Transport* aboard = Transport::VesselOf(seen))
    {
        if (aboard->GetMap() == viewer.GetMap() || aboard == &viewer)
        {
            return true;
        }
    }

    if (Transport* watching = Transport::VesselOf(viewer))
    {
        if (watching->GetMap() == seen.GetMap() || watching == &seen)
        {
            return true;
        }
    }

    return false;
}

/// The object a proximity question must be asked from. A passenger has no pose the shore
/// can measure against, so the vessel answers for him -- and its hull radius is added as
/// slack, because he may stand anywhere on it.
static WorldObject const& ProximityAnchor(WorldObject const& obj, float& slack)
{
    TransportMap* hull = obj.GetMap() ? obj.GetMap()->AsTransport() : NULL;
    Transport* vessel = hull ? hull->Vessel() : NULL;

    if (vessel)
    {
        slack += hull->HullRadius();
        return *vessel;
    }
    return obj;
}

bool SeenWithin(WorldObject const& seen, WorldObject const& viewer, float dist, bool is3D)
{
    if (!CanBeSeen(seen, viewer))
    {
        return false;
    }

    // Same frame -- the world's, or one deck's. Exact, so measure it and be done.
    if (seen.Where().ShareFrame(viewer.Where()))
    {
        return seen.Where().WithinDist(viewer.Where(), dist, is3D);
    }

    // Across a vessel's boundary. Whatever is aboard answers with its hull.
    float slack = 0.0f;
    WorldObject const& a = ProximityAnchor(seen, slack);
    WorldObject const& b = ProximityAnchor(viewer, slack);

    // One of them IS the anchor: he is standing on the very thing he is looking at.
    if (&a == &b)
    {
        return true;
    }

    return a.Where().WithinDist(b.Where(), dist + slack, is3D);
}

bool InReach(WorldObject const& a, WorldObject const& b, float dist, bool is3D)
{
    Geometry::Placement pa, pb;
    return CanInteract(a, b) && InCommonFrame(a, b, pa, pb) &&
           pa.WithinDist(pb, dist, is3D);
}

bool InFrontPhased(WorldObject const& a, WorldObject const& b, float dist, float arc)
{
    Geometry::Placement pa, pb;
    return CanInteract(a, b) && InCommonFrame(a, b, pa, pb) &&
           pa.IsInFront(pb, dist, arc);
}

bool InBackPhased(WorldObject const& a, WorldObject const& b, float dist, float arc)
{
    Geometry::Placement pa, pb;
    return CanInteract(a, b) && InCommonFrame(a, b, pa, pb) &&
           pa.IsInBack(pb, dist, arc);
}

bool HasLineOfSight(WorldObject const& a, Geometry::Vector3 const& point)
{
    // The two-yard lift is eye height: a sight line is cast between heads, not feet.
    return a.GetMap()->IsInLineOfSight(a.Where().X(), a.Where().Y(), a.Where().Z() + 2.0f,
                                       point.x, point.y, point.z + 2.0f, a.GetPhaseMask());
}

bool HasLineOfSight(WorldObject const& a, WorldObject const& b)
{
    if (!CanInteract(a, b))
    {
        return false;
    }

    // Aboard, the sight line is cast through the HULL's own geometry, on the ship's own map,
    // in the coordinates both passengers already speak.
    if (TransportMap* hull = a.GetMap() ? a.GetMap()->AsTransport() : NULL)
    {
        Geometry::Placement pa, pb;
        if (!InCommonFrame(a, b, pa, pb))
        {
            return false;
        }
        return !hull->IsBlocked(
            Geometry::Vector3(pa.X(), pa.Y(), pa.Z() + 2.0f),
            Geometry::Vector3(pb.X(), pb.Y(), pb.Z() + 2.0f));
    }

    return HasLineOfSight(a, b.Where().Pos());
}

bool IsPlaceable(WorldObject const& obj)
{
    return obj.Where().IsFinite() &&
           MaNGOS::IsValidMapCoord(obj.Where().X(), obj.Where().Y(),
                                   obj.Where().Z(), obj.Where().Facing());
}

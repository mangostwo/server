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

#include "MoveSplineInit.h"
#include "MoveSpline.h"
#include "packet_builder.h"
#include "Unit.h"
#include "Transports.h"
#include "Vehicle.h"
#include "TransportMap.h"
#include "Map.h"

namespace
{
    /// The vessel whose deck this unit is standing on, or an empty guid. Derived from the
    /// map, so a spline goes out as SMSG_MONSTER_MOVE_TRANSPORT for anything on a deck --
    /// crew, pet or totem alike -- without anyone having registered it as anything.
    ObjectGuid DeckVesselGuidOf(Unit const& unit)
    {
        if (Map* on = unit.GetMap())
        {
            if (TransportMap* hull = on->AsTransport())
            {
                if (Transport* vessel = hull->Vessel())
                {
                    return vessel->GetObjectGuid();
                }
            }
        }
        return ObjectGuid();
    }
}

namespace Movement
{
    /**
     * @brief Selects the appropriate speed type based on movement flags.
     * @param moveFlags The movement flags.
     * @return The selected UnitMoveType.
     */
    UnitMoveType SelectSpeedType(uint32 moveFlags)
    {
        if (moveFlags & MOVEFLAG_FLYING)
        {
            if (moveFlags & MOVEFLAG_BACKWARD /*&& speed_obj.flight >= speed_obj.flight_back*/)
            {
                return MOVE_FLIGHT_BACK;
            }
            else
            {
                return MOVE_FLIGHT;
            }
        }
        else if (moveFlags & MOVEFLAG_SWIMMING)
        {
            if (moveFlags & MOVEFLAG_BACKWARD /*&& speed_obj.swim >= speed_obj.swim_back*/)
            {
                return MOVE_SWIM_BACK;
            }
            else
            {
                return MOVE_SWIM;
            }
        }
        else if (moveFlags & MOVEFLAG_WALK_MODE)
        {
            // if ( speed_obj.run > speed_obj.walk )
            return MOVE_WALK;
        }
        else if (moveFlags & MOVEFLAG_BACKWARD /*&& speed_obj.run >= speed_obj.run_back*/)
        {
            return MOVE_RUN_BACK;
        }

        return MOVE_RUN;
    }

    /**
     * @brief Final pass of initialization that launches spline movement.
     * @return int32 duration - estimated travel time
     */
    int32 MoveSplineInit::Launch()
    {
        MoveSpline& move_spline = *unit.movespline;

        // A VEHICLE seat is a real transform the server owns, so a rider's pose has to be
        // fetched from it. A DECK is not: the unit's map is the vessel and its position is
        // already deck-local, so Where() is the answer and nothing is composed.
        TransportInfo* transportInfo = unit.GetTransportInfo();
        if (transportInfo && !transportInfo->IsOnVehicle())
        {
            transportInfo = NULL;
        }

        const ObjectGuid vesselGuid = DeckVesselGuidOf(unit);

        Location real_position(unit.Where().X(), unit.Where().Y(), unit.Where().Z(), unit.Where().Facing());

        if (transportInfo)
        {
            Geometry::Placement const& deck = transportInfo->Seat();
            real_position.x = deck.X();
            real_position.y = deck.Y();
            real_position.z = deck.Z();
            real_position.orientation = deck.Facing();
        }

        // there is a big chance that current position is unknown if current state is not finalized, need compute it
        // this also allows calculate spline position and update map position in much greater intervals
        if (!move_spline.Finalized() && !transportInfo)
        {
            real_position = move_spline.ComputePosition();
        }
        else if (!transportInfo)
        {
            // A stop just took the spline's position and the placement has not caught up
            // (it is written on the unit's next Update): start from where the stop was sent.
            if (Position const* pending = unit.PendingSplineCommit())
            {
                real_position = Location(pending->x, pending->y, pending->z, pending->o);
            }
        }

        if (args.path.empty())
        {
            // should i do the things that user should do?
            MoveTo(real_position);
        }

        // correct first vertex
        args.path[0] = real_position;
        args.initialOrientation = real_position.orientation;

        uint32 moveFlags = unit.m_movementInfo.GetMovementFlags();
        if (args.flags.walkmode)
        {
            moveFlags |= MOVEFLAG_WALK_MODE;
        }
        else
        {
            moveFlags &= ~MOVEFLAG_WALK_MODE;
        }

        moveFlags |= (MOVEFLAG_SPLINE_ENABLED | MOVEFLAG_FORWARD);

        if (args.velocity == 0.f)
        {
            args.velocity = unit.GetSpeed(SelectSpeedType(moveFlags));
        }

        if (!args.Validate(&unit))
        {
            return 0;
        }

        unit.m_movementInfo.SetMovementFlags((MovementFlags)moveFlags);
        move_spline.Initialize(args);

        WorldPacket data(SMSG_MONSTER_MOVE, 64);
        data << unit.GetPackGUID();

        if (transportInfo)
        {
            data.SetOpcode(SMSG_MONSTER_MOVE_TRANSPORT);
            data << transportInfo->GetTransportGuid().WriteAsPacked();
            data << int8(transportInfo->GetTransportSeat());
        }
        else if (!vesselGuid.IsEmpty())
        {
            // NO SEAT. A seat is a vehicle's, and a vehicle is a unit: it has a seat map,
            // a transform per seat and a passenger bound to one. A ship has none of that --
            // she is a map, and what is on her is simply on her. -1 is how the client is
            // told there is no seat, and it is what both reference cores send for a
            // MO_TRANSPORT. The create block has always said -1; this said 0, and the two
            // described different things about the same creature.
            data.SetOpcode(SMSG_MONSTER_MOVE_TRANSPORT);
            data << vesselGuid.WriteAsPacked();
            data << int8(-1);
        }

        PacketBuilder::WriteMonsterMove(move_spline, data);
        unit.SendMessageToSet(&data, true);

        return move_spline.Duration();
    }

    /**
     * @brief Stops any creature movement.
     */
    void MoveSplineInit::Stop()
    {
        MoveSpline& move_spline = *unit.movespline;

        // No need to stop if we are not moving
        if (move_spline.Finalized())
        {
            return;
        }

        TransportInfo* transportInfo = unit.GetTransportInfo();
        if (transportInfo && !transportInfo->IsOnVehicle())
        {
            transportInfo = NULL;
        }

        const ObjectGuid vesselGuid = DeckVesselGuidOf(unit);

        Location real_position(unit.Where().X(), unit.Where().Y(), unit.Where().Z(), unit.Where().Facing());

        if (transportInfo)
        {
            Geometry::Placement const& deck = transportInfo->Seat();
            real_position.x = deck.X();
            real_position.y = deck.Y();
            real_position.z = deck.Z();
            real_position.orientation = deck.Facing();
        }

        // there is a big chance that current position is unknown if current state is not finalized, need compute it
        // this also allows calculate spline position and update map position in much greater intervals
        if (!move_spline.Finalized() && !transportInfo)
        {
            real_position = move_spline.ComputePosition();
        }

        if (args.path.empty())
        {
            // should i do the things that user should do?
            MoveTo(real_position);
        }

        // current first vertex
        args.path[0] = real_position;

        args.flags = MoveSplineFlag::Done;
        unit.m_movementInfo.RemoveMovementFlag(MovementFlags(MOVEFLAG_FORWARD | MOVEFLAG_SPLINE_ENABLED));
        move_spline.Initialize(args);

        WorldPacket data(SMSG_MONSTER_MOVE, 64);
        data << unit.GetPackGUID();

        if (transportInfo)
        {
            data.SetOpcode(SMSG_MONSTER_MOVE_TRANSPORT);
            data << transportInfo->GetTransportGuid().WriteAsPacked();
            data << int8(transportInfo->GetTransportSeat());
        }
        else if (!vesselGuid.IsEmpty())
        {
            // NO SEAT. A seat is a vehicle's, and a vehicle is a unit: it has a seat map,
            // a transform per seat and a passenger bound to one. A ship has none of that --
            // she is a map, and what is on her is simply on her. -1 is how the client is
            // told there is no seat, and it is what both reference cores send for a
            // MO_TRANSPORT. The create block has always said -1; this said 0, and the two
            // described different things about the same creature.
            data.SetOpcode(SMSG_MONSTER_MOVE_TRANSPORT);
            data << vesselGuid.WriteAsPacked();
            data << int8(-1);
        }

        data << uint8(0);
        data << real_position.x << real_position.y << real_position.z;
        data << move_spline.GetId();
        data << uint8(MonsterMoveStop);
        unit.SendMessageToSet(&data, true);
    }

    /**
     * @brief Constructor that initializes the MoveSplineInit with a reference to a Unit.
     * @param m Reference to the Unit to be moved.
     */
    MoveSplineInit::MoveSplineInit(Unit& m) : unit(m)
    {
        // mix existing state into new
        args.flags.walkmode = unit.m_movementInfo.HasMovementFlag(MOVEFLAG_WALK_MODE);
        args.flags.flying = unit.m_movementInfo.HasMovementFlag((MovementFlags)(MOVEFLAG_CAN_FLY | MOVEFLAG_FLYING | MOVEFLAG_LEVITATING));
    }

    /**
     * @brief Sets unit's facing to a specified target after all path done.
     * @param target The target to face.
     */
    void MoveSplineInit::SetFacing(const Unit* target)
    {
        args.flags.EnableFacingTarget();
        args.facing.target = target->GetObjectGuid().GetRawValue();
    }

    /**
     * @brief Adds final facing animation.
     * Sets unit's facing to specified point/angle after all path done.
     * You can have only one final facing: previous will be overridden.
     * @param angle The angle to face.
     */
    void MoveSplineInit::SetFacing(float angle)
    {
        args.facing.angle = Geometry::wrap(angle, 0.f, (float)Geometry::twoPi());
        args.flags.EnableFacingAngle();
    }
}

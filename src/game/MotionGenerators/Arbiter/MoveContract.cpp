/**
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2026 MaNGOS <https://www.getmangos.eu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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

#include "MoveContract.h"

namespace Arbiter
{
    Layer LayerOf(MoveKind kind)
    {
        switch (kind)
        {
            case MoveKind::Idle:
            case MoveKind::Random:
            case MoveKind::Waypoint:
            case MoveKind::FollowTarget:
                return Layer::Default;
            case MoveKind::Chase:
                return Layer::Combat;
            case MoveKind::Point:
            case MoveKind::FlyLand:
            case MoveKind::Home:
            case MoveKind::AssistanceRun:
                return Layer::Scripted;
            case MoveKind::Distract:
            case MoveKind::AssistanceDistract:
                return Layer::Distract;
            case MoveKind::Fear:
            case MoveKind::Confused:
                return Layer::Control;
            case MoveKind::Effect:
                return Layer::Forced;
            case MoveKind::Taxi:
                return Layer::Taxi;
            default:
                return Layer::Default;
        }
    }

    Policy PolicyOf(MoveKind kind, bool resumeCombat)
    {
        switch (kind)
        {
            case MoveKind::Random:
            case MoveKind::Waypoint:
            case MoveKind::FlyLand:
            case MoveKind::Home:
            case MoveKind::AssistanceRun:
            case MoveKind::Taxi:
                return Policy::Override;
            case MoveKind::Point:
                return resumeCombat ? Policy::Suspend : Policy::Override;
            case MoveKind::FollowTarget:
            case MoveKind::Chase:
                return Policy::Supersede;
            default:
                return Policy::Suspend;   // Idle-as-command, Distract, AssistanceDistract, Fear, Confused, Effect
        }
    }

    bool SelfExpiring(MoveKind kind)
    {
        return kind == MoveKind::Home || kind == MoveKind::Distract ||
               kind == MoveKind::Effect;
    }

    const char* KindName(MoveKind kind)
    {
        static const char* const names[] =
        {
            "Idle", "Random", "Waypoint", "FollowTarget", "Chase", "Point", "FlyLand", "Home",
            "AssistanceRun", "Distract", "AssistanceDistract", "Fear", "Confused", "Effect", "Taxi"
        };
        static_assert(sizeof(names) / sizeof(names[0]) == static_cast<size_t>(MoveKind::Count),
                      "KindName out of sync with MoveKind");
        return kind < MoveKind::Count ? names[static_cast<uint8>(kind)] : "?";
    }

    MoveKind KindOf(MovementGeneratorType type)
    {
        switch (type)
        {
            case IDLE_MOTION_TYPE:                return MoveKind::Idle;
            case RANDOM_MOTION_TYPE:              return MoveKind::Random;
            case WAYPOINT_MOTION_TYPE:            return MoveKind::Waypoint;
            case CONFUSED_MOTION_TYPE:            return MoveKind::Confused;
            case CHASE_MOTION_TYPE:               return MoveKind::Chase;
            case HOME_MOTION_TYPE:                return MoveKind::Home;
            case FLIGHT_MOTION_TYPE:              return MoveKind::Taxi;
            case POINT_MOTION_TYPE:               return MoveKind::Point;
            case FLEEING_MOTION_TYPE:
            case TIMED_FLEEING_MOTION_TYPE:       return MoveKind::Fear;
            case DISTRACT_MOTION_TYPE:            return MoveKind::Distract;
            case ASSISTANCE_MOTION_TYPE:          return MoveKind::AssistanceRun;
            case ASSISTANCE_DISTRACT_MOTION_TYPE: return MoveKind::AssistanceDistract;
            case FOLLOW_MOTION_TYPE:              return MoveKind::FollowTarget;
            case EFFECT_MOTION_TYPE:              return MoveKind::Effect;
            default:                              return MoveKind::Idle;
        }
    }

    bool SameKind(MovementGeneratorType type, MoveKind kind)
    {
        if (type == POINT_MOTION_TYPE)
        {
            return kind == MoveKind::Point || kind == MoveKind::FlyLand;
        }
        return KindOf(type) == kind;
    }
}

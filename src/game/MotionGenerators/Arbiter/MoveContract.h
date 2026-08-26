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

#ifndef MANGOS_ARBITER_MOVECONTRACT_H
#define MANGOS_ARBITER_MOVECONTRACT_H

#include "Platform/Define.h"
#include "MotionMaster.h"

/// The movement arbiter's contract: what can be requested, on which layer it
/// runs, and how it treats what was running before (design §4.1-4.3).
namespace Arbiter
{
    enum class MoveKind : uint8
    {
        Idle, Random, Waypoint, FollowTarget,   // Default layer
        Chase,                                  // Combat layer
        Point, FlyLand, Home, AssistanceRun,    // Scripted layer
        Distract, AssistanceDistract,           // Distract layer
        Fear, Confused,                         // Control layer
        Effect,                                 // Forced layer
        Taxi,                                   // Taxi layer
        Count
    };

    /// Ascending priority: the highest occupied layer is selected.
    enum class Layer : uint8 { Default, Combat, Scripted, Distract, Control, Forced, Taxi, Count };

    enum class Policy : uint8 { Supersede, Suspend, Override };

    enum class FinishReason : uint8
    {
        Arrived, Cut, Blocked, Expired, Superseded, Overridden, Cleared, Cancelled, TargetLost, Died
    };

    struct MoveRequest
    {
        MoveKind kind;
        uint32   id;            ///< MovementInform id, 0 when none
        bool     resumeCombat;  ///< Point only: Suspend instead of Override (D2)
    };

    /// The layer a kind runs on; Layer::Default is the lowest priority, Layer::Taxi the highest.
    Layer LayerOf(MoveKind kind);
    /// How a request treats what runs beneath it; resumeCombat turns a Point's Override into Suspend.
    Policy PolicyOf(MoveKind kind, bool resumeCombat);
    /// Kinds MotionMaster::Mutate expires on any new request (Home, Distract,
    /// AssistanceDistract, Effect).
    bool SelfExpiring(MoveKind kind);
    /// Human-readable kind name for traces and tests.
    const char* KindName(MoveKind kind);
    /// The kind a legacy generator type maps to (FlyOrLand reports POINT, both fleeing types map to Fear).
    MoveKind KindOf(MovementGeneratorType type);
    /// True when a stack entry of this type stands for the model kind (POINT matches Point and FlyLand).
    bool SameKind(MovementGeneratorType type, MoveKind kind);
}

#endif

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

// The movement arbiter's pure core: the kind/layer/policy tables, the selection
// model and the shadow classifier. Nothing here touches Unit, the driver or a map;
// the game-side ArbiterShadow is proven by the headless scenario suite instead.

#include "TestHarness.h"

#include "Arbiter/MoveContract.h"
#include "Arbiter/ArbiterModel.h"

#include <string>

using namespace Arbiter;

namespace
{
    int L(Layer layer) { return static_cast<int>(layer); }
    int P(Policy policy) { return static_cast<int>(policy); }
    int K(MoveKind kind) { return static_cast<int>(kind); }
}

TEST(ArbiterContract_LayerTable)
{
    CHECK_EQ(L(LayerOf(MoveKind::Idle)), L(Layer::Default));
    CHECK_EQ(L(LayerOf(MoveKind::Random)), L(Layer::Default));
    CHECK_EQ(L(LayerOf(MoveKind::Waypoint)), L(Layer::Default));
    CHECK_EQ(L(LayerOf(MoveKind::FollowTarget)), L(Layer::Default));
    CHECK_EQ(L(LayerOf(MoveKind::Chase)), L(Layer::Combat));
    CHECK_EQ(L(LayerOf(MoveKind::Point)), L(Layer::Scripted));
    CHECK_EQ(L(LayerOf(MoveKind::FlyLand)), L(Layer::Scripted));
    CHECK_EQ(L(LayerOf(MoveKind::Home)), L(Layer::Scripted));
    CHECK_EQ(L(LayerOf(MoveKind::AssistanceRun)), L(Layer::Scripted));
    CHECK_EQ(L(LayerOf(MoveKind::Distract)), L(Layer::Distract));
    CHECK_EQ(L(LayerOf(MoveKind::AssistanceDistract)), L(Layer::Distract));
    CHECK_EQ(L(LayerOf(MoveKind::Fear)), L(Layer::Control));
    CHECK_EQ(L(LayerOf(MoveKind::Confused)), L(Layer::Control));
    CHECK_EQ(L(LayerOf(MoveKind::Effect)), L(Layer::Forced));
    CHECK_EQ(L(LayerOf(MoveKind::Taxi)), L(Layer::Taxi));
}

TEST(ArbiterContract_PolicyTable)
{
    CHECK_EQ(P(PolicyOf(MoveKind::Random, false)), P(Policy::Override));    // public MoveRandomAroundPoint
    CHECK_EQ(P(PolicyOf(MoveKind::Waypoint, false)), P(Policy::Override));  // D7
    CHECK_EQ(P(PolicyOf(MoveKind::FollowTarget, false)), P(Policy::Supersede));
    CHECK_EQ(P(PolicyOf(MoveKind::Idle, false)), P(Policy::Suspend));
    CHECK_EQ(P(PolicyOf(MoveKind::Chase, false)), P(Policy::Supersede));    // D6: update
    CHECK_EQ(P(PolicyOf(MoveKind::Point, false)), P(Policy::Override));     // D2
    CHECK_EQ(P(PolicyOf(MoveKind::Point, true)), P(Policy::Suspend));       // resumeCombat
    CHECK_EQ(P(PolicyOf(MoveKind::Home, false)), P(Policy::Override));
    CHECK_EQ(P(PolicyOf(MoveKind::Distract, false)), P(Policy::Suspend));
    CHECK_EQ(P(PolicyOf(MoveKind::Fear, false)), P(Policy::Suspend));
    CHECK_EQ(P(PolicyOf(MoveKind::Effect, false)), P(Policy::Suspend));
    CHECK_EQ(P(PolicyOf(MoveKind::Taxi, false)), P(Policy::Override));      // D8 split handled in the model
}

TEST(ArbiterContract_SelfExpiringMatchesMutate)
{
    // MotionMaster::Mutate expires a HOME, DISTRACT or EFFECT top before pushing
    // anything (MotionMaster.cpp:629-647); AssistanceDistract reports DISTRACT's family.
    CHECK(SelfExpiring(MoveKind::Home));
    CHECK(SelfExpiring(MoveKind::Distract));
    CHECK(SelfExpiring(MoveKind::AssistanceDistract));
    CHECK(SelfExpiring(MoveKind::Effect));
    CHECK(!SelfExpiring(MoveKind::Point));
    CHECK(!SelfExpiring(MoveKind::Chase));
    CHECK(!SelfExpiring(MoveKind::Fear));
}

TEST(ArbiterContract_KindOfGeneratorType)
{
    CHECK_EQ(K(KindOf(IDLE_MOTION_TYPE)), K(MoveKind::Idle));
    CHECK_EQ(K(KindOf(RANDOM_MOTION_TYPE)), K(MoveKind::Random));
    CHECK_EQ(K(KindOf(WAYPOINT_MOTION_TYPE)), K(MoveKind::Waypoint));
    CHECK_EQ(K(KindOf(CONFUSED_MOTION_TYPE)), K(MoveKind::Confused));
    CHECK_EQ(K(KindOf(CHASE_MOTION_TYPE)), K(MoveKind::Chase));
    CHECK_EQ(K(KindOf(HOME_MOTION_TYPE)), K(MoveKind::Home));
    CHECK_EQ(K(KindOf(FLIGHT_MOTION_TYPE)), K(MoveKind::Taxi));
    CHECK_EQ(K(KindOf(POINT_MOTION_TYPE)), K(MoveKind::Point));
    CHECK_EQ(K(KindOf(FLEEING_MOTION_TYPE)), K(MoveKind::Fear));
    CHECK_EQ(K(KindOf(TIMED_FLEEING_MOTION_TYPE)), K(MoveKind::Fear));
    CHECK_EQ(K(KindOf(DISTRACT_MOTION_TYPE)), K(MoveKind::Distract));
    CHECK_EQ(K(KindOf(ASSISTANCE_MOTION_TYPE)), K(MoveKind::AssistanceRun));
    CHECK_EQ(K(KindOf(ASSISTANCE_DISTRACT_MOTION_TYPE)), K(MoveKind::AssistanceDistract));
    CHECK_EQ(K(KindOf(FOLLOW_MOTION_TYPE)), K(MoveKind::FollowTarget));
    CHECK_EQ(K(KindOf(EFFECT_MOTION_TYPE)), K(MoveKind::Effect));
    // FlyOrLandMovementGenerator reports POINT_MOTION_TYPE (PointMovementGenerator.h:49,91).
    CHECK(SameKind(POINT_MOTION_TYPE, MoveKind::FlyLand));
    CHECK(SameKind(POINT_MOTION_TYPE, MoveKind::Point));
    CHECK(!SameKind(POINT_MOTION_TYPE, MoveKind::Home));
    CHECK_STR(KindName(MoveKind::AssistanceRun), "AssistanceRun");
}

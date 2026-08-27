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

#include "TestHarness.h"

#include "Arbiter/ShadowClassifier.h"

using namespace Arbiter;

namespace
{
    MoveRequest Req(MoveKind kind, uint32 id = 0)
    {
        MoveRequest r;
        r.kind = kind;
        r.id = id;
        r.resumeCombat = false;
        return r;
    }

    int D(Divergence d) { return static_cast<int>(d); }
}

TEST(ShadowClassifier_AgreeingStackIsNone)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Random);
    m.Request(Req(MoveKind::Chase));
    CHECK_EQ(D(Classify({RANDOM_MOTION_TYPE, CHASE_MOTION_TYPE}, m)), D(Divergence::None));
    m.Request(Req(MoveKind::Effect, 1));
    CHECK_EQ(D(Classify({RANDOM_MOTION_TYPE, CHASE_MOTION_TYPE, EFFECT_MOTION_TYPE}, m)), D(Divergence::None));
}

TEST(ShadowClassifier_FlyOrLandReportsPoint)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Idle);
    m.Request(Req(MoveKind::FlyLand, 2));
    CHECK_EQ(D(Classify({IDLE_MOTION_TYPE, POINT_MOTION_TYPE}, m)), D(Divergence::None));
}

TEST(ShadowClassifier_StalePointBeneathIsSupersededOneShot)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Idle);
    m.Request(Req(MoveKind::Point, 1));
    m.Request(Req(MoveKind::Point, 2));
    CHECK_EQ(D(Classify({IDLE_MOTION_TYPE, POINT_MOTION_TYPE, POINT_MOTION_TYPE}, m)), D(Divergence::SupersededOneShot));
    m.ExpireSelected();                                                        // model: back to Idle
    CHECK_EQ(D(Classify({IDLE_MOTION_TYPE, POINT_MOTION_TYPE}, m)), D(Divergence::SupersededOneShot));   // stack: stale point resumed
}

TEST(ShadowClassifier_StalePointUnderFearIsSupersededOneShot)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Random);
    m.Request(Req(MoveKind::Point, 1));
    m.Request(Req(MoveKind::Point, 2));                                        // the model superseded the first
    m.Request(Req(MoveKind::Fear));                                            // a mask, not a one-shot of its own
    CHECK_EQ(D(Classify({RANDOM_MOTION_TYPE, POINT_MOTION_TYPE, POINT_MOTION_TYPE, FLEEING_MOTION_TYPE}, m)),
             D(Divergence::SupersededOneShot));
}

TEST(ShadowClassifier_StalePointResumedUnderFearIsSupersededOneShot)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Idle);
    m.Request(Req(MoveKind::Fear));
    m.Request(Req(MoveKind::Point, 1));
    m.Request(Req(MoveKind::Point, 2));
    m.Expire(MoveKind::Point);                                                 // the point on the stack expired
    // The model holds [Idle, Fear]; the stack resumed the stale first point above the fear.
    CHECK_EQ(D(Classify({IDLE_MOTION_TYPE, FLEEING_MOTION_TYPE, POINT_MOTION_TYPE}, m)),
             D(Divergence::SupersededOneShot));
}

TEST(ShadowClassifier_TwoChasesIsTargetedMultiplicity)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Random);
    m.Request(Req(MoveKind::Chase));
    m.Request(Req(MoveKind::Chase));
    CHECK_EQ(D(Classify({RANDOM_MOTION_TYPE, CHASE_MOTION_TYPE, CHASE_MOTION_TYPE}, m)), D(Divergence::TargetedMultiplicity));
}

TEST(ShadowClassifier_PointUnderFearIsLayerOrder)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Idle);
    m.Request(Req(MoveKind::Fear));
    m.Request(Req(MoveKind::Point, 3));
    CHECK_EQ(D(Classify({IDLE_MOTION_TYPE, FLEEING_MOTION_TYPE, POINT_MOTION_TYPE}, m)), D(Divergence::LayerOrder));
}

TEST(ShadowClassifier_ChaseDroppedBeneathFearIsCombatCancelledEarly)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Random);
    m.Request(Req(MoveKind::Chase));
    m.Request(Req(MoveKind::Fear));
    m.ExpireSelected();                                                        // fear ends: model resumes the chase
    CHECK_EQ(D(Classify({RANDOM_MOTION_TYPE}, m)), D(Divergence::CombatCancelledEarly));   // stack deleted it (DirectExpire)
}

TEST(ShadowClassifier_FollowDroppedBeneathChaseIsCombatCancelledEarly)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Idle);
    m.Clear(false);
    m.Request(Req(MoveKind::FollowTarget));
    m.Request(Req(MoveKind::Chase));
    m.ExpireSelected();                                                        // chase target gone: model → Follow
    CHECK_EQ(D(Classify({IDLE_MOTION_TYPE}, m)), D(Divergence::CombatCancelledEarly));
}

TEST(ShadowClassifier_EmptyModelIsUnexpected)
{
    ArbiterModel m;
    CHECK_EQ(D(Classify({RANDOM_MOTION_TYPE}, m)), D(Divergence::Unexpected));
}

TEST(ShadowClassifier_UnrelatedTopIsUnexpected)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Random);
    m.Request(Req(MoveKind::Point, 1));
    CHECK_EQ(D(Classify({RANDOM_MOTION_TYPE, FLIGHT_MOTION_TYPE}, m)), D(Divergence::Unexpected));
}

TEST(ShadowClassifier_Describe)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Random);
    m.Request(Req(MoveKind::Chase));
    m.Request(Req(MoveKind::Point, 7));
    CHECK_STR(DescribeStack({RANDOM_MOTION_TYPE, CHASE_MOTION_TYPE, POINT_MOTION_TYPE}), "[Random,Chase,Point]");
    CHECK_STR(DescribeModel(m), "[Random|Point#7]");
    CHECK_STR(DivergenceName(Divergence::LayerOrder), "LayerOrder");
}

TEST(ShadowClassifier_ChaseOverIdleCommandIsLayerOrder)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Random);
    m.Request(Req(MoveKind::Idle));                                            // MoveIdle on a non-empty stack
    m.Request(Req(MoveKind::Chase));                                           // aggro: the stack runs the chase
    CHECK_EQ(D(Classify({RANDOM_MOTION_TYPE, IDLE_MOTION_TYPE, CHASE_MOTION_TYPE}, m)), D(Divergence::LayerOrder));
}

TEST(ShadowClassifier_IdleOverStalePointIsSupersededOneShot)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Random);
    m.Request(Req(MoveKind::Point, 1));
    m.Request(Req(MoveKind::Idle));                                            // the model superseded the point
    CHECK_EQ(D(Classify({RANDOM_MOTION_TYPE, POINT_MOTION_TYPE, IDLE_MOTION_TYPE}, m)), D(Divergence::SupersededOneShot));
}

TEST(ShadowClassifier_IdleCommandUnderFearIsLayerOrder)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Idle);                                          // a default Idle beneath an Idle command
    m.Request(Req(MoveKind::Fear));
    m.Request(Req(MoveKind::Idle));                                            // MoveIdle while feared: the stack runs the idle
    CHECK_EQ(D(Classify({IDLE_MOTION_TYPE, FLEEING_MOTION_TYPE, IDLE_MOTION_TYPE}, m)), D(Divergence::LayerOrder));
}

TEST(ShadowClassifier_DoubleFearIsSupersededOneShot)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Random);
    m.Request(Req(MoveKind::Fear));
    m.Request(Req(MoveKind::Fear));                                            // feared again: the stack keeps both
    CHECK_EQ(D(Classify({RANDOM_MOTION_TYPE, FLEEING_MOTION_TYPE, FLEEING_MOTION_TYPE}, m)),
             D(Divergence::SupersededOneShot));
}

TEST(ShadowClassifier_StaleFearResumedIsSupersededOneShot)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Random);
    m.Request(Req(MoveKind::Fear));
    m.Request(Req(MoveKind::Fear));
    m.Expire(MoveKind::Fear);                                                  // the second fear ended; the stack resumes the first
    CHECK_EQ(D(Classify({RANDOM_MOTION_TYPE, FLEEING_MOTION_TYPE}, m)), D(Divergence::SupersededOneShot));
}

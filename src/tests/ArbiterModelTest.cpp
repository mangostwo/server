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
    CHECK_EQ(P(PolicyOf(MoveKind::FlyLand, false)), P(Policy::Override));
    CHECK_EQ(P(PolicyOf(MoveKind::Home, false)), P(Policy::Override));
    CHECK_EQ(P(PolicyOf(MoveKind::AssistanceRun, false)), P(Policy::Override));
    CHECK_EQ(P(PolicyOf(MoveKind::Distract, false)), P(Policy::Suspend));
    CHECK_EQ(P(PolicyOf(MoveKind::AssistanceDistract, false)), P(Policy::Suspend));
    CHECK_EQ(P(PolicyOf(MoveKind::Fear, false)), P(Policy::Suspend));
    CHECK_EQ(P(PolicyOf(MoveKind::Confused, false)), P(Policy::Suspend));
    CHECK_EQ(P(PolicyOf(MoveKind::Effect, false)), P(Policy::Suspend));
    CHECK_EQ(P(PolicyOf(MoveKind::Taxi, false)), P(Policy::Override));      // D8 split handled in the model
}

TEST(ArbiterContract_SelfExpiringMatchesMutate)
{
    // MotionMaster::Mutate expires a HOME, DISTRACT or EFFECT top before pushing
    // anything; AssistanceDistract reports its own type, which is not in that switch.
    CHECK(SelfExpiring(MoveKind::Home));
    CHECK(SelfExpiring(MoveKind::Distract));
    CHECK(!SelfExpiring(MoveKind::AssistanceDistract));
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

namespace
{
    MoveRequest Req(MoveKind kind, uint32 id = 0, bool resumeCombat = false)
    {
        MoveRequest r;
        r.kind = kind;
        r.id = id;
        r.resumeCombat = resumeCombat;
        return r;
    }

    int SelectedKind(ArbiterModel const& m)
    {
        std::optional<Held> sel = m.Selected();
        return sel ? K(sel->kind) : -1;
    }

    int CountEvents(std::vector<MovementEvent> const& events, MovementEvent::Kind kind, MoveKind who)
    {
        int n = 0;
        for (MovementEvent const& e : events)
        {
            if (e.kind == kind && e.who == who)
            {
                ++n;
            }
        }
        return n;
    }
}

TEST(ArbiterModel_EmptyThenFactoryDefault)
{
    ArbiterModel m;
    CHECK(m.Empty());
    CHECK_EQ(SelectedKind(m), -1);
    m.InstallDefault(MoveKind::Random);
    CHECK(!m.Empty());
    CHECK_EQ(SelectedKind(m), K(MoveKind::Random));
    CHECK_EQ(static_cast<int>(m.Contents().size()), 1);
}

TEST(ArbiterModel_ChaseAboveDefault_AndUpdatesInPlace)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Random);
    m.Request(Req(MoveKind::Chase));
    CHECK_EQ(SelectedKind(m), K(MoveKind::Chase));
    const uint32 firstSeq = m.Combat()->seq;
    m.Request(Req(MoveKind::Chase));                                  // D6: update, no second chase
    CHECK_EQ(static_cast<int>(m.Contents().size()), 2);
    CHECK(m.Combat()->seq != firstSeq);
    std::vector<MovementEvent> ev = m.DrainEvents();
    CHECK_EQ(CountEvents(ev, MovementEvent::Kind::Suspended, MoveKind::Random), 1);
    CHECK_EQ(CountEvents(ev, MovementEvent::Kind::Finished, MoveKind::Chase), 0);
}

TEST(ArbiterModel_PointOverridesCombat_DefaultResumesAfter)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Waypoint);
    m.Request(Req(MoveKind::Chase));
    m.Request(Req(MoveKind::Point, 7));
    CHECK_EQ(SelectedKind(m), K(MoveKind::Point));
    CHECK(!m.Combat());                                               // D2: combat cancelled
    std::vector<MovementEvent> ev = m.DrainEvents();
    CHECK_EQ(CountEvents(ev, MovementEvent::Kind::Finished, MoveKind::Chase), 1);
    m.ExpireSelected();                                               // the point arrives
    CHECK_EQ(SelectedKind(m), K(MoveKind::Waypoint));
    ev = m.DrainEvents();
    CHECK_EQ(CountEvents(ev, MovementEvent::Kind::Finished, MoveKind::Point), 1);
    CHECK_EQ(CountEvents(ev, MovementEvent::Kind::Resumed, MoveKind::Waypoint), 1);
}

TEST(ArbiterModel_PointWithResumeCombatSuspends)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Waypoint);
    m.Request(Req(MoveKind::Chase));
    m.Request(Req(MoveKind::Point, 7, true));
    CHECK(m.Combat());
    m.ExpireSelected();
    CHECK_EQ(SelectedKind(m), K(MoveKind::Chase));
}

TEST(ArbiterModel_PointSupersedesPoint)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Idle);
    m.Request(Req(MoveKind::Point, 1));
    m.Request(Req(MoveKind::Point, 2));
    CHECK_EQ(static_cast<int>(m.Contents().size()), 2);               // default + one point
    CHECK_EQ(static_cast<int>(m.Command(Layer::Scripted)->id), 2);
    std::vector<MovementEvent> ev = m.DrainEvents();
    bool superseded = false;
    for (MovementEvent const& e : ev)
    {
        if (e.kind == MovementEvent::Kind::Finished && e.who == MoveKind::Point && e.id == 1 &&
            e.reason == FinishReason::Superseded)
        {
            superseded = true;
        }
    }
    CHECK(superseded);
}

TEST(ArbiterModel_FearSuspendsPoint_PointResumes)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Idle);
    m.Request(Req(MoveKind::Point, 3));
    m.Request(Req(MoveKind::Fear));
    CHECK_EQ(SelectedKind(m), K(MoveKind::Fear));
    CHECK(m.Command(Layer::Scripted));                                // still held, masked
    m.CancelControl(MoveKind::Fear);                                  // D3: by identity
    CHECK_EQ(SelectedKind(m), K(MoveKind::Point));
    CHECK_EQ(CountEvents(m.DrainEvents(), MovementEvent::Kind::Resumed, MoveKind::Point), 1);
}

TEST(ArbiterModel_PointUnderFearWaits)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Idle);
    m.Request(Req(MoveKind::Fear));
    m.Request(Req(MoveKind::Point, 4));
    CHECK_EQ(SelectedKind(m), K(MoveKind::Fear));                     // D1: layer order, not push order
    CHECK(m.Command(Layer::Scripted));
}

TEST(ArbiterModel_EffectKeepsCombat_EffectSupersedesEffect)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Random);
    m.Request(Req(MoveKind::Chase));
    m.Request(Req(MoveKind::Effect, 10));
    CHECK(m.Combat());
    m.Request(Req(MoveKind::Effect, 11));                             // knockback during a knockback
    CHECK(m.Combat());
    CHECK_EQ(static_cast<int>(m.Command(Layer::Forced)->id), 11);
    m.ExpireSelected();                                               // lands
    CHECK_EQ(SelectedKind(m), K(MoveKind::Chase));
}

TEST(ArbiterModel_SelfExpiringCancelledByAnyRequest)
{
    // MotionMaster::Mutate expires a HOME/DISTRACT/EFFECT top before pushing.
    ArbiterModel m;
    m.InstallDefault(MoveKind::Random);
    m.Request(Req(MoveKind::Home));
    m.Request(Req(MoveKind::Chase));                                  // aggro on the way home
    CHECK(!m.Command(Layer::Scripted));
    CHECK_EQ(SelectedKind(m), K(MoveKind::Chase));
    m.Request(Req(MoveKind::Distract));
    m.Request(Req(MoveKind::Point, 5));
    CHECK(!m.Command(Layer::Distract));
    CHECK_EQ(SelectedKind(m), K(MoveKind::Point));
}

TEST(ArbiterModel_TaxiCancelsScriptedAndCombat_KeepsControlAndDefault)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Idle);
    m.Request(Req(MoveKind::Confused));
    m.Request(Req(MoveKind::Point, 6));
    m.Request(Req(MoveKind::Chase));
    m.Request(Req(MoveKind::Taxi));
    CHECK_EQ(SelectedKind(m), K(MoveKind::Taxi));
    CHECK(!m.Command(Layer::Scripted));
    CHECK(!m.Combat());
    CHECK(m.Command(Layer::Control));                                 // D8
    CHECK(m.Default());
    m.ExpireSelected();                                               // landed
    CHECK_EQ(SelectedKind(m), K(MoveKind::Confused));
}

TEST(ArbiterModel_FollowIsDefault_FallbackOnTargetLost)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Random);
    m.Clear(false);                                                   // MoveFollow does Clear() first
    m.Request(Req(MoveKind::FollowTarget));
    CHECK_EQ(SelectedKind(m), K(MoveKind::FollowTarget));
    CHECK_EQ(static_cast<int>(m.Contents().size()), 1);
    m.ExpireSelected();                                               // Follow::Update returns false: target gone
    CHECK_EQ(SelectedKind(m), K(MoveKind::Random));                   // retained fallback default
}

TEST(ArbiterModel_IdleOnNonEmptyIsMaskingCommand)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Random);
    m.Request(Req(MoveKind::Chase));
    m.Clear(false);                                                   // possession: Clear(false) + MoveIdle
    m.Request(Req(MoveKind::Idle));
    CHECK_EQ(SelectedKind(m), K(MoveKind::Idle));
    CHECK_EQ(K(m.Default()->kind), K(MoveKind::Random));              // default untouched beneath
    m.ExpireSelected();                                               // MovementExpired
    CHECK_EQ(SelectedKind(m), K(MoveKind::Random));
}

TEST(ArbiterModel_ClearProjections)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Waypoint);
    m.Request(Req(MoveKind::Chase));
    m.Request(Req(MoveKind::Point, 8));
    m.Request(Req(MoveKind::Fear));
    m.Clear(false);                                                   // everything but the bottom
    CHECK_EQ(SelectedKind(m), K(MoveKind::Waypoint));
    CHECK_EQ(static_cast<int>(m.Contents().size()), 1);
    m.Clear(true);                                                    // the bottom too
    CHECK(m.Empty());
    std::vector<MovementEvent> ev = m.DrainEvents();
    CHECK_EQ(CountEvents(ev, MovementEvent::Kind::Finished, MoveKind::Waypoint), 1);
}

TEST(ArbiterModel_ExpireAtDepthOneIsNoOp)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Random);
    m.ExpireSelected();
    CHECK_EQ(SelectedKind(m), K(MoveKind::Random));
    CHECK_EQ(static_cast<int>(m.DrainEvents().size()), 0);
}

TEST(ArbiterModel_LowerRequestWhileHigherCommandRuns_NoResumeEvent)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Idle);
    m.Request(Req(MoveKind::Fear));
    m.DrainEvents();
    m.Request(Req(MoveKind::Chase));
    std::vector<MovementEvent> ev = m.DrainEvents();
    CHECK_EQ(static_cast<int>(ev.size()), 0);                         // stored masked, nothing suspended or resumed
    CHECK(m.Combat());
}

TEST(ArbiterModel_IdleOverPointSupersedesIt)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Random);
    m.Request(Req(MoveKind::Point, 9));
    m.DrainEvents();
    m.Request(Req(MoveKind::Idle));
    CHECK_EQ(SelectedKind(m), K(MoveKind::Idle));
    CHECK_EQ(K(m.Command(Layer::Scripted)->kind), K(MoveKind::Idle));
    std::vector<MovementEvent> ev = m.DrainEvents();
    bool superseded = false;
    for (MovementEvent const& e : ev)
    {
        if (e.kind == MovementEvent::Kind::Finished && e.who == MoveKind::Point && e.id == 9 &&
            e.reason == FinishReason::Superseded)
        {
            superseded = true;
        }
    }
    CHECK(superseded);
    m.ExpireSelected();                                               // the idle expires
    CHECK_EQ(SelectedKind(m), K(MoveKind::Random));                   // the default, never the stale point
}

TEST(ArbiterModel_IdleTwiceIsNoOp)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Random);
    m.Request(Req(MoveKind::Idle));
    m.DrainEvents();
    m.Request(Req(MoveKind::Idle));
    CHECK_EQ(static_cast<int>(m.DrainEvents().size()), 0);
    CHECK_EQ(static_cast<int>(m.Contents().size()), 2);
    CHECK_EQ(SelectedKind(m), K(MoveKind::Idle));
}

TEST(ArbiterModel_ExpireKindFinishesMaskedEntryOnly)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Idle);
    m.Request(Req(MoveKind::Fear));
    m.Request(Req(MoveKind::Point, 4));                               // masked beneath the fear
    m.DrainEvents();
    m.Expire(MoveKind::Point);                                        // the stack's point expired
    CHECK_EQ(SelectedKind(m), K(MoveKind::Fear));                     // the fear is untouched
    CHECK(!m.Command(Layer::Scripted));
    CHECK_EQ(CountEvents(m.DrainEvents(), MovementEvent::Kind::Finished, MoveKind::Point), 1);
}

TEST(ArbiterModel_ExpireKindNotHeldIsNoOp)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Random);
    m.Request(Req(MoveKind::Chase));
    m.DrainEvents();
    m.Expire(MoveKind::Point);                                        // a stale point the model never held
    CHECK_EQ(SelectedKind(m), K(MoveKind::Chase));
    CHECK_EQ(static_cast<int>(m.DrainEvents().size()), 0);
}

TEST(ArbiterModel_ExpireKindPrefersIdleCommandOverDefault)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Idle);
    m.Request(Req(MoveKind::Idle));                                   // MoveIdle on a non-empty stack
    m.Expire(MoveKind::Idle);
    CHECK(!m.Command(Layer::Scripted));
    CHECK_EQ(SelectedKind(m), K(MoveKind::Idle));                     // the default remains
}

TEST(ArbiterModel_ExpireKindFollowRestoresFallback)
{
    ArbiterModel m;
    m.InstallDefault(MoveKind::Random);
    m.Clear(false);
    m.Request(Req(MoveKind::FollowTarget));
    m.Expire(MoveKind::FollowTarget);
    CHECK_EQ(SelectedKind(m), K(MoveKind::Random));
}

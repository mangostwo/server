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

#include "PointMovementGenerator.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "Map.h"
#include "MotionFrame.h"
#include "TemporarySummon.h"
#include "World.h"
#include "movement/MoveSpline.h"

void PointMovementGenerator::Initialize(Unit& owner)
{
    if (owner.hasUnitState(UNIT_STAT_CAN_NOT_REACT | UNIT_STAT_NOT_MOVE))
    {
        return;
    }

    owner.StopMoving();
    owner.addUnitState(UNIT_STAT_ROAMING | UNIT_STAT_ROAMING_MOVE);

    // The leg itself is laid on the first tick, by the driver.
    ResetLeg();
}

void PointMovementGenerator::Reset(Unit& owner)
{
    Initialize(owner);
}

void PointMovementGenerator::Interrupt(Unit& owner)
{
    owner.InterruptMoving();
    owner.clearUnitState(UNIT_STAT_ROAMING | UNIT_STAT_ROAMING_MOVE);
    ResetLeg();
}

void PointMovementGenerator::Finalize(Unit& owner)
{
    owner.clearUnitState(UNIT_STAT_ROAMING | UNIT_STAT_ROAMING_MOVE);

    // Only a leg that ended on its own counts as reaching the point. One cut short by an
    // interrupt finalizes the spline too, so the spline state cannot tell them apart.
    if (m_done)
    {
        MovementInform(owner);
    }
}

void PointMovementGenerator::MovementInform(Unit& owner) const
{
    if (owner.GetTypeId() != TYPEID_UNIT)
    {
        return;
    }

    Creature& creature = static_cast<Creature&>(owner);

    if (creature.AI())
    {
        creature.AI()->MovementInform(POINT_MOTION_TYPE, m_id);
    }

    if (!creature.IsTemporarySummon())
    {
        return;
    }

    const ObjectGuid summonerGuid = static_cast<TemporarySummon&>(creature).GetSummonerGuid();
    if (!summonerGuid.IsCreature())
    {
        return;
    }

    if (Creature* summoner = creature.GetMap()->GetCreature(summonerGuid))
    {
        if (summoner->AI())
        {
            summoner->AI()->SummonedMovementInform(&creature, POINT_MOTION_TYPE, m_id);
        }
    }
}

Motion::MoveIntent PointMovementGenerator::Intent(Unit& owner,
                                                  Motion::MoveStatus const& status,
                                                  uint32 /*diff*/)
{
    if (owner.hasUnitState(UNIT_STAT_CAN_NOT_MOVE))
    {
        owner.clearUnitState(UNIT_STAT_ROAMING_MOVE);
        return Motion::MoveIntent::Hold();
    }

    // Arrived, or there was no way to get there at all: either way this one-shot is over
    // and the generator beneath it takes back over. Finalize fires the AI inform.
    if (status.arrived || status.blocked)
    {
        m_done = true;
        return Motion::MoveIntent::Done();
    }

    // Stopped or interrupted from outside: the move is over. Tell the script anyway, so a
    // sequence waiting on this point does not stall; the point is wherever we stand now.
    if (status.cut)
    {
        m_done = true;
        return Motion::MoveIntent::Done();
    }

    owner.addUnitState(UNIT_STAT_ROAMING | UNIT_STAT_ROAMING_MOVE);

    // m_dest arrived from a script, an AI or a spell effect, and those speak WORLD
    // coordinates. Converted here rather than in the constructor, so the conversion
    // survives a Reset() and is re-done if the frame beneath us ever changes. FromWorld
    // is the identity in the world frame, so this costs nothing today.
    const Motion::Vector3 goal = Motion::FrameFor(owner).FromWorld(owner, m_dest);

    return Motion::MoveIntent::Move(goal, LegFlags());
}

void AssistanceMovementGenerator::Finalize(Unit& owner)
{
    owner.clearUnitState(UNIT_STAT_ROAMING | UNIT_STAT_ROAMING_MOVE);

    Creature& creature = static_cast<Creature&>(owner);
    creature.SetNoCallAssistance(false);
    creature.CallAssistance();

    if (creature.IsAlive())
    {
        creature.GetMotionMaster()->MoveSeekAssistanceDistract(
            sWorld.getConfig(CONFIG_UINT32_CREATURE_FAMILY_ASSISTANCE_DELAY));
    }
}

Motion::MoveIntent EffectMovementGenerator::Intent(Unit& /*owner*/,
                                                   Motion::MoveStatus const& status,
                                                   uint32 /*diff*/)
{
    // Note this is `traveling`, not `arrived`: the spline was launched by the effect, not
    // by us, so if it was never running at all we must pop immediately rather than wait
    // for an arrival edge that will never come.
    if (status.traveling)
    {
        return Motion::MoveIntent::Hold();
    }

    // The spline is over -- run out, cut short, or never launched: report it. A script
    // waiting on a jump must not stall; the landing is wherever the unit stands now.
    // (Popped mid-flight by Clear or Mutate, nothing has ended and nothing is reported.)
    m_done = true;
    return Motion::MoveIntent::Done();
}

void EffectMovementGenerator::Finalize(Unit& owner)
{
    if (owner.GetTypeId() != TYPEID_UNIT)
    {
        return;
    }

    Creature& creature = static_cast<Creature&>(owner);

    // Ended on our own tick, or landed while we were not ticking at all (a stun): both
    // are the effect having happened. A spline still flying, or one cut short by the
    // generator that replaces us, is not.
    const bool landed = owner.movespline->Finalized() && !owner.movespline->Cut();
    if (creature.AI() && (m_done || landed))
    {
        creature.AI()->MovementInform(EFFECT_MOTION_TYPE, m_id);
    }

    if (!owner.IsAlive() ||
        owner.hasUnitState(UNIT_STAT_CONFUSED | UNIT_STAT_FLEEING | UNIT_STAT_NO_COMBAT_MOVEMENT))
    {
        return;
    }

    // Whatever we interrupted resumes by itself -- a chase or follow included, which expiry
    // now leaves beneath us. Only a victim with no chase left to resume gets a fresh one.
    const MovementGeneratorType beneath = owner.GetMotionMaster()->GetCurrentMovementGeneratorType();
    if (beneath == CHASE_MOTION_TYPE || beneath == FOLLOW_MOTION_TYPE)
    {
        return;
    }

    if (Unit* victim = owner.getVictim())
    {
        owner.GetMotionMaster()->MoveChase(victim);
    }
}

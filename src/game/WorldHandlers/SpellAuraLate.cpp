/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2025 MaNGOS <https://www.getmangos.eu>
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

/**
 * @file SpellAuraLate.cpp
 * @brief Cohesion split of SpellAuras.cpp -- late crit/mastery/vehicle aura handlers.
 *        Same Aura/SpellAuraHolder classes; no behaviour change.
 */

#include "Common.h"
#include "Database/DatabaseEnv.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "UpdateMask.h"
#include "World.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Player.h"
#include "Unit.h"
#include "Spell.h"
#include "DynamicObject.h"
#include "Group.h"
#include "UpdateData.h"
#include "ObjectAccessor.h"
#include "Policies/Singleton.h"
#include "Totem.h"
#include "Creature.h"
#include "Formulas.h"
#include "BattleGround/BattleGround.h"
#include "OutdoorPvP/OutdoorPvP.h"
#include "CreatureAI.h"
#include "ScriptMgr.h"
#include "Util.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Vehicle.h"
#include "CellImpl.h"
#include "Language.h"

void Aura::HandleAuraModAllCritChance(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
    {
        return;
    }

    Unit* target = GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    ((Player*)target)->HandleBaseModValue(CRIT_PERCENTAGE,         FLAT_MOD, float(m_modifier.m_amount), apply);
    ((Player*)target)->HandleBaseModValue(OFFHAND_CRIT_PERCENTAGE, FLAT_MOD, float(m_modifier.m_amount), apply);
    ((Player*)target)->HandleBaseModValue(RANGED_CRIT_PERCENTAGE,  FLAT_MOD, float(m_modifier.m_amount), apply);

    // included in Player::UpdateSpellCritChance calculation
    ((Player*)target)->UpdateAllSpellCritChances();
}

void Aura::HandleAuraStopNaturalManaRegen(bool apply, bool Real)
{
    if (!Real)
    {
        return;
    }

    GetTarget()->ApplyModFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_REGENERATE_POWER, !apply && !GetTarget()->IsUnderLastManaUseEffect());
}

void Aura::HandleAuraSetVehicleId(bool apply, bool Real)
{
    if (!Real)
    {
        return;
    }

    GetTarget()->SetVehicleId(apply ? GetMiscValue() : 0, 0);
}

void Aura::HandlePreventResurrection(bool apply, bool Real)
{
    if (!Real)
    {
        return;
    }

    Unit* target = GetTarget();
    if (!target || target->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    if (apply)
    {
        target->RemoveByteFlag(PLAYER_FIELD_BYTES, 0, PLAYER_FIELD_BYTE_RELEASE_TIMER);
    }
    else if (!target->GetMap()->Instanceable())
    {
        target->SetByteFlag(PLAYER_FIELD_BYTES, 0, PLAYER_FIELD_BYTE_RELEASE_TIMER);
    }
}

void Aura::HandleFactionOverride(bool apply, bool Real)
{
    if (!Real)
    {
        return;
    }

    Unit* target = GetTarget();
    if (!target || !sFactionTemplateStore.LookupEntry(GetMiscValue()))
    {
        return;
    }

    if (apply)
    {
        target->setFaction(GetMiscValue());
    }
    else
    {
        target->RestoreOriginalFaction();
    }
}

void Aura::HandleTriggerLinkedAura(bool apply, bool Real)
{
    if (!Real)
    {
        return;
    }

    uint32 linkedSpell = GetSpellProto()->EffectTriggerSpell[m_effIndex];
    SpellEntry const* spellInfo = sSpellStore.LookupEntry(linkedSpell);
    if (!spellInfo)
    {
        sLog.outError("Aura::HandleTriggerLinkedAura for spell %u effect %u triggering unknown spell id %u", GetSpellProto()->ID, m_effIndex, linkedSpell);
        return;
    }

    Unit* target = GetTarget();

    if (apply)
    {
        // ToDo: handle various cases where base points need to be applied!
        target->CastSpell(target, spellInfo, true, NULL, this);
    }
    else
    {
        target->RemoveAurasByCasterSpell(linkedSpell, GetCasterGuid());
    }
}

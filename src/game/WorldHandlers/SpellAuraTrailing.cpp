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
 * @file SpellAuraTrailing.cpp
 * @brief Cohesion split of SpellAuras.cpp -- trailing aura handlers.
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

/**
 * @brief Applies or removes prevention of fleeing on feared targets.
 *
 * @param apply True to prevent fleeing; false to restore it.
 * @param Real True when processing the real aura state change.
 */
void Aura::HandlePreventFleeing(bool apply, bool Real)
{
    if (!Real)
    {
        return;
    }

    Unit::AuraList const& fearAuras = GetTarget()->GetAurasByType(SPELL_AURA_MOD_FEAR);
    if (!fearAuras.empty())
    {
        if (apply)
        {
            GetTarget()->SetFeared(false, fearAuras.front()->GetCasterGuid());
        }
        else
        {
            GetTarget()->SetFeared(true);
        }
    }
}

/**
 * @brief Calculates bonus absorb values for mana shield effects.
 *
 * @param apply True to apply the shield; false to remove it.
 * @param Real True when processing the real aura state change.
 */
void Aura::HandleManaShield(bool apply, bool Real)
{
    if (!Real)
    {
        return;
    }

    // prevent double apply bonuses
    if (apply && (GetTarget()->GetTypeId() != TYPEID_PLAYER || !((Player*)GetTarget())->GetSession()->PlayerLoading()))
    {
        if (Unit* caster = GetCaster())
        {
            float DoneActualBenefit = 0.0f;
            switch (GetSpellProto()->SpellClassSet)
            {
                case SPELLFAMILY_MAGE:
                    if (GetSpellProto()->SpellClassMask & UI64LIT(0x0000000000008000))
                    {
                        // Mana Shield
                        // +50% from +spd bonus
                        DoneActualBenefit = caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(GetSpellProto())) * 0.5f;
                        break;
                    }
                    break;
                default:
                    break;
            }

            DoneActualBenefit *= caster->CalculateLevelPenalty(GetSpellProto());

            m_modifier.m_amount += (int32)DoneActualBenefit;
        }
    }
}

void Aura::HandleArenaPreparation(bool apply, bool Real)
{
    if (!Real)
    {
        return;
    }

    Unit* target = GetTarget();

    target->ApplyModFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PREPARATION, apply);

    if (apply)
    {
        // max regen powers at start preparation
        target->SetHealth(target->GetMaxHealth());
        target->SetPower(POWER_MANA, target->GetMaxPower(POWER_MANA));
        target->SetPower(POWER_ENERGY, target->GetMaxPower(POWER_ENERGY));
    }
    else
    {
        // reset originally 0 powers at start/leave
        target->SetPower(POWER_RAGE, 0);
        target->SetPower(POWER_RUNIC_POWER, 0);
    }
}

/*
 * Such auras are applied from a caster(=player) to a vehicle.
 * This has been verified using spell #49256
 */
void Aura::HandleAuraControlVehicle(bool apply, bool Real)
{
    if (!Real)
    {
        return;
    }

    Unit* target = GetTarget();
    if (!target->IsVehicle())
    {
        return;
    }

    Unit* caster = GetCaster();
    if (!caster)
    {
        return;
    }

    if (apply)
    {
        target->GetVehicleInfo()->Board(caster, GetBasePoints() - 1);
    }
    else
    {
        target->GetVehicleInfo()->UnBoard(caster, m_removeMode == AURA_REMOVE_BY_TRACKING);
    }
}

void Aura::HandleAuraAddMechanicAbilities(bool apply, bool Real)
{
    if (!Real)
    {
        return;
    }

    Unit* target = GetTarget();

    if (!target || target->GetTypeId() != TYPEID_PLAYER)    // only players should be affected by this aura
    {
        return;
    }

    uint16 i_OverrideSetId = GetMiscValue();

    const OverrideSpellDataEntry* spellSet = sOverrideSpellDataStore.LookupEntry(i_OverrideSetId);
    if (!spellSet)
    {
        return;
    }

    if (apply)
    {
        // spell give the player a new castbar with some spells.. this is a clientside process..
        // serverside just needs to register the new spells so that player isn't kicked as cheater
        for (int i = 0; i < MAX_OVERRIDE_SPELLS; ++i)
        {
            if (uint32 spellId = spellSet->Spells[i])
            {
                static_cast<Player*>(target)->addSpell(spellId, true, false, false, false);
            }
        }

        target->SetUInt16Value(PLAYER_FIELD_BYTES2, 0, i_OverrideSetId);
    }
    else
    {
        target->SetUInt16Value(PLAYER_FIELD_BYTES2, 0, 0);
        for (int i = 0; i < MAX_OVERRIDE_SPELLS; ++i)
        {
            if (uint32 spellId = spellSet->Spells[i])
            {
                static_cast<Player*>(target)->removeSpell(spellId, false , false, false);
            }
        }
    }
}

void Aura::HandleAuraOpenStable(bool apply, bool Real)
{
    if (!Real || GetTarget()->GetTypeId() != TYPEID_PLAYER || !GetTarget()->IsInWorld())
    {
        return;
    }

    Player* player = (Player*)GetTarget();

    if (apply)
    {
        player->GetSession()->SendStablePet(player->GetObjectGuid());
    }

    // client auto close stable dialog at !apply aura
}

void Aura::HandleAuraMirrorImage(bool apply, bool Real)
{
    if (!Real)
    {
        return;
    }

    // Target of aura should always be creature (ref Spell::CheckCast)
    Creature* pCreature = (Creature*)GetTarget();

    if (apply)
    {
        // Caster can be player or creature, the unit who pCreature will become an clone of.
        Unit* caster = GetCaster();

        if (caster->GetTypeId() == TYPEID_PLAYER)           // TODO - Verify! Does it take a 'pseudo-race' (from display-id) for creature-mirroring, and what is sent in SMSG_MIRRORIMAGE_DATA
        {
            pCreature->SetByteValue(UNIT_FIELD_BYTES_0, 0, caster->getRace());
        }

        pCreature->SetByteValue(UNIT_FIELD_BYTES_0, 1, caster->getClass());
        pCreature->SetByteValue(UNIT_FIELD_BYTES_0, 2, caster->getGender());
        pCreature->SetByteValue(UNIT_FIELD_BYTES_0, 3, caster->GetPowerType());

        pCreature->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_CLONED);

        pCreature->SetDisplayId(caster->GetNativeDisplayId());
    }
    else
    {
        const CreatureInfo* cinfo = pCreature->GetCreatureInfo();
        const CreatureModelInfo* minfo = sObjectMgr.GetCreatureModelInfo(pCreature->GetNativeDisplayId());

        pCreature->SetByteValue(UNIT_FIELD_BYTES_0, 0, 0);
        pCreature->SetByteValue(UNIT_FIELD_BYTES_0, 1, cinfo->UnitClass);
        pCreature->SetByteValue(UNIT_FIELD_BYTES_0, 2, minfo->gender);
        pCreature->SetByteValue(UNIT_FIELD_BYTES_0, 3, 0);

        pCreature->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_CLONED);

        pCreature->SetDisplayId(pCreature->GetNativeDisplayId());
    }
}

void Aura::HandleMirrorName(bool apply, bool Real)
{
    if (!Real)
    {
        return;
    }

    Unit* caster = GetCaster();
    Unit* target = GetTarget();

    if (!target || !caster || target->GetTypeId() != TYPEID_UNIT)
    {
        return;
    }

    if (apply)
    {
        target->SetName(caster->GetName());
    }
    else
    {
        CreatureInfo const* cinfo = ((Creature*)target)->GetCreatureInfo();
        if (!cinfo)
        {
            return;
        }

        target->SetName(cinfo->Name);
    }
}

void Aura::HandleAuraConvertRune(bool apply, bool Real)
{
    if (!Real)
    {
        return;
    }

    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    Player* plr = (Player*)GetTarget();

    if (plr->getClass() != CLASS_DEATH_KNIGHT)
    {
        return;
    }

    RuneType runeFrom = RuneType(GetSpellProto()->EffectMiscValue[m_effIndex]);
    RuneType runeTo   = RuneType(GetSpellProto()->EffectMiscValueB[m_effIndex]);

    if (apply)
    {
        for (uint32 i = 0; i < MAX_RUNES; ++i)
        {
            if (plr->GetCurrentRune(i) == runeFrom && !plr->GetRuneCooldown(i))
            {
                plr->ConvertRune(i, runeTo);
                break;
            }
        }
    }
    else
    {
        for (uint32 i = 0; i < MAX_RUNES; ++i)
        {
            if (plr->GetCurrentRune(i) == runeTo && plr->GetBaseRune(i) == runeFrom)
            {
                plr->ConvertRune(i, runeFrom);
                break;
            }
        }
    }
}

void Aura::HandlePhase(bool apply, bool Real)
{
    if (!Real)
    {
        return;
    }

    Unit* target = GetTarget();

    // always non stackable
    if (apply)
    {
        Unit::AuraList const& phases = target->GetAurasByType(SPELL_AURA_PHASE);
        if (!phases.empty())
        {
            target->RemoveAurasDueToSpell(phases.front()->GetId(), GetHolder());
        }
    }

    target->SetPhaseMask(apply ? GetMiscValue() : uint32(PHASEMASK_NORMAL), true);
    // no-phase is also phase state so same code for apply and remove
    if (target->GetTypeId() == TYPEID_PLAYER)
    {
        SpellAreaForAreaMapBounds saBounds = sSpellMgr.GetSpellAreaForAuraMapBounds(GetId());
        if (saBounds.first != saBounds.second)
        {
            uint32 zone, area;
            target->GetZoneAndAreaId(zone, area);

            for (SpellAreaForAreaMap::const_iterator itr = saBounds.first; itr != saBounds.second; ++itr)
            {
                itr->second->ApplyOrRemoveSpellIfCan((Player*)target, zone, area, false);
            }
        }
    }
}

void Aura::HandleAuraSafeFall(bool Apply, bool Real)
{
    // implemented in WorldSession::HandleMovementOpcodes

    // only special case
    if (Apply && Real && GetId() == 32474 && GetTarget()->GetTypeId() == TYPEID_PLAYER)
    {
        ((Player*)GetTarget())->ActivateTaxiPathTo(506, GetId());
    }
}

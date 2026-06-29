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
 * @file SpellAuraAddModifier.cpp
 * @brief Cohesion split of SpellAuras.cpp -- HandleAddModifier handler.
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

/*********************************************************/
/***               BASIC AURA FUNCTION                 ***/
/*********************************************************/
void Aura::HandleAddModifier(bool apply, bool Real)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER || !Real)
    {
        return;
    }

    if (m_modifier.m_miscvalue >= MAX_SPELLMOD)
    {
        return;
    }

    if (apply)
    {
        SpellEntry const* spellProto = GetSpellProto();

        // Add custom charges for some mod aura
        switch (spellProto->Id)
        {
            case 17941:                                     // Shadow Trance
            case 22008:                                     // Netherwind Focus
            case 31834:                                     // Light's Grace
            case 34754:                                     // Clearcasting
            case 34936:                                     // Backlash
            case 44401:                                     // Missile Barrage
            case 48108:                                     // Hot Streak
            case 51124:                                     // Killing Machine
            case 54741:                                     // Firestarter
            case 57761:                                     // Fireball!
            case 64823:                                     // Elune's Wrath (Balance druid t8 set
                GetHolder()->SetAuraCharges(1);
                break;
        }

        // Everlasting Affliction, overwrite wrong data, if will need more better restore support of spell_affect table
        if (spellProto->SpellFamilyName == SPELLFAMILY_WARLOCK && spellProto->SpellIconID == 3169)
        {
            // Corruption and Unstable Affliction
            // TODO: drop when override will be possible
            SpellEntry* entry = const_cast<SpellEntry*>(spellProto);
            entry->EffectSpellClassMask[GetEffIndex()].Flags = UI64LIT(0x0000010000000002);
        }
        // Improved Flametongue Weapon, overwrite wrong data, maybe time re-add table
        else if (spellProto->Id == 37212)
        {
            // Flametongue Weapon (Passive)
            // TODO: drop when override will be possible
            SpellEntry* entry = const_cast<SpellEntry*>(spellProto);
            entry->EffectSpellClassMask[GetEffIndex()].Flags = UI64LIT(0x0000000000200000);
        }
    }

    ((Player*)GetTarget())->AddSpellMod(this, apply);

    ReapplyAffectedPassiveAuras();
}

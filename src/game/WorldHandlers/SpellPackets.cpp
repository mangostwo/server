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
 * @file SpellPackets.cpp
 * @brief Cohesion split of Spell.cpp -- packet senders.
 *        Same `Spell` class; no behaviour change.
 */

#include "Spell.h"
#include "Database/DatabaseEnv.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Opcodes.h"
#include "Log.h"
#include "UpdateMask.h"
#include "World.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Player.h"
#include "Pet.h"
#include "Unit.h"
#include "DynamicObject.h"
#include "Group.h"
#include "UpdateData.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "CellImpl.h"
#include "Policies/Singleton.h"
#include "SharedDefines.h"
#include "LootMgr.h"
#include "VMapFactory.h"
#include "BattleGround/BattleGround.h"
#include "Util.h"
#include "Chat.h"
#include "Vehicle.h"
#include "TemporarySummon.h"
#include "SQLStorages.h"
#include "DisableMgr.h"
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif /*ENABLE_ELUNA*/

/**
 * @brief Sends the cast result for this spell to the appropriate receiver.
 *
 * @param result The cast result code.
 */
void Spell::SendCastResult(SpellCastResult result)
{
    if (result == SPELL_CAST_OK)
    {
        return;
    }

    if (m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    if (((Player*)m_caster)->GetSession()->PlayerLoading()) // don't send cast results at loading time
    {
        return;
    }

    SendCastResult((Player*)m_caster, m_spellInfo, m_cast_count, result);
}

/**
 * @brief Sends a cast result packet for a specific player and spell.
 *
 * @param caster The player receiving the result.
 * @param spellInfo The spell being reported.
 * @param result The cast result code.
 */
void Spell::SendCastResult(Player* caster, SpellEntry const* spellInfo, uint8 cast_count, SpellCastResult result, bool isPetCastResult /*=false*/)
{
    if (result == SPELL_CAST_OK)
    {
        return;
    }

    WorldPacket data(isPetCastResult ? SMSG_PET_CAST_FAILED : SMSG_CAST_FAILED, (4 + 1 + 2));
    data << uint8(cast_count);                              // single cast or multi 2.3 (0/1)
    data << uint32(spellInfo->ID);
    data << uint8(!IsPassiveSpell(spellInfo) ? result : SPELL_FAILED_DONT_REPORT); // do not report failed passive spells
    switch (result)
    {
        case SPELL_FAILED_NOT_READY:
            data << uint32(0);                              // unknown, value 1 seen for 14177 (update cooldowns on client flag)
            break;
        case SPELL_FAILED_REQUIRES_SPELL_FOCUS:
            data << uint32(spellInfo->RequiresSpellFocus);  // SpellFocusObject.dbc id
            break;
        case SPELL_FAILED_REQUIRES_AREA:                    // AreaTable.dbc id
            // hardcode areas limitation case
            switch (spellInfo->ID)
            {
                case 41617:                                 // Cenarion Mana Salve
                case 41619:                                 // Cenarion Healing Salve
                    data << uint32(3905);
                    break;
                case 41618:                                 // Bottled Nethergon Energy
                case 41620:                                 // Bottled Nethergon Vapor
                    data << uint32(3842);
                    break;
                case 45373:                                 // Bloodberry Elixir
                    data << uint32(4075);
                    break;
                default:                                    // default case (don't must be)
                    data << uint32(0);
                    break;
            }
            break;
        case SPELL_FAILED_TOTEMS:
            for (int i = 0; i < MAX_SPELL_TOTEMS; ++i)
            {
                if (spellInfo->Totem[i])
                {
                    data << uint32(spellInfo->Totem[i]);    // client needs only one id, not 2...
                }
            }
            break;
        case SPELL_FAILED_TOTEM_CATEGORY:
            for (int i = 0; i < MAX_SPELL_TOTEM_CATEGORIES; ++i)
            {
                if (spellInfo->RequiredTotemCategoryID[i])
                {
                    data << uint32(spellInfo->RequiredTotemCategoryID[i]);// client needs only one id, not 2...
                }
            }
            break;
        case SPELL_FAILED_EQUIPPED_ITEM_CLASS:
        case SPELL_FAILED_EQUIPPED_ITEM_CLASS_MAINHAND:
        case SPELL_FAILED_EQUIPPED_ITEM_CLASS_OFFHAND:
            data << uint32(spellInfo->EquippedItemClass);
            data << uint32(spellInfo->EquippedItemSubclass);
            break;
        case SPELL_FAILED_PREVENTED_BY_MECHANIC:
            data << uint32(0);                              // SpellMechanic.dbc id
            break;
        case SPELL_FAILED_CUSTOM_ERROR:
            data << uint32(0);                              // custom error id (see enum SpellCastResultCustom)
            break;
        case SPELL_FAILED_NEED_EXOTIC_AMMO:
            data << uint32(spellInfo->EquippedItemSubclass);// seems correct...
            break;
        case SPELL_FAILED_REAGENTS:
            data << uint32(0);                              // item id
            break;
        case SPELL_FAILED_NEED_MORE_ITEMS:
            data << uint32(0);                              // item id
            data << uint32(0);                              // item count?
            break;
        case SPELL_FAILED_MIN_SKILL:
            data << uint32(0);                              // SkillLine.dbc id
            data << uint32(0);                              // required skill value
            break;
        case SPELL_FAILED_TOO_MANY_OF_ITEM:
            data << uint32(0);                              // ItemLimitCategory.dbc id
            break;
        case SPELL_FAILED_FISHING_TOO_LOW:
            data << uint32(0);                              // required fishing skill
            break;
        default:
            break;
    }
    caster->GetSession()->SendPacket(&data);
}

/**
 * @brief Sends the spell start packet for visible casts.
 */
void Spell::SendSpellStart()
{
    if (!IsNeedSendToClient())
    {
        return;
    }

    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "Sending SMSG_SPELL_START id=%u", m_spellInfo->ID);

    uint32 castFlags = CAST_FLAG_UNKNOWN2;
    if (IsRangedSpell())
    {
        castFlags |= CAST_FLAG_AMMO;
    }

    if (m_spellInfo->RuneCostID)
    {
        castFlags |= CAST_FLAG_UNKNOWN19;
    }

    WorldPacket data(SMSG_SPELL_START, (8 + 8 + 4 + 4 + 2));
    if (m_CastItem)
    {
        data << m_CastItem->GetPackGUID();
    }
    else
    {
        data << m_caster->GetPackGUID();
    }

    data << m_caster->GetPackGUID();
    data << uint8(m_cast_count);                            // pending spell cast
    data << uint32(m_spellInfo->ID);                        // spellId
    data << uint32(castFlags);                              // cast flags
    data << uint32(m_timer);                                // delay?

    data << m_targets;

    if (castFlags & CAST_FLAG_PREDICTED_POWER)              // predicted power
    {
        data << uint32(0);
    }

    if (castFlags & CAST_FLAG_PREDICTED_RUNES)              // predicted runes
    {
        uint8 v1 = 0;// m_runesState;
        uint8 v2 = 0;//((Player*)m_caster)->GetRunesState();
        data << uint8(v1);                                  // runes state before
        data << uint8(v2);                                  // runes state after
        for (uint8 i = 0; i < MAX_RUNES; ++i)
        {
            uint8 m = (1 << i);
            if (m & v1)                                     // usable before...
            {
                if (!(m & v2))                              // ...but on cooldown now...
                {
                    data << uint8(0);                       // some unknown byte (time?)
                }
            }
        }
    }

    if (castFlags & CAST_FLAG_AMMO)                         // projectile info
    {
        WriteAmmoToPacket(&data);
    }

    if (castFlags & CAST_FLAG_IMMUNITY)                     // cast immunity
    {
        data << uint32(0);                                  // used for SetCastSchoolImmunities
        data << uint32(0);                                  // used for SetCastImmunities
    }

    m_caster->SendMessageToSet(&data, true);
}

/**
 * @brief Sends the spell go packet for visible casts.
 */
void Spell::SendSpellGo()
{
    // not send invisible spell casting
    if (!IsNeedSendToClient())
    {
        return;
    }

    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "Sending SMSG_SPELL_GO id=%u", m_spellInfo->ID);

    uint32 castFlags = CAST_FLAG_UNKNOWN9;
    if (IsRangedSpell())
    {
        castFlags |= CAST_FLAG_AMMO;                         // arrows/bullets visual
    }

    if ((m_caster->GetTypeId() == TYPEID_PLAYER) && (m_caster->getClass() == CLASS_DEATH_KNIGHT) && m_spellInfo->RuneCostID)
    {
        castFlags |= CAST_FLAG_UNKNOWN19;                   // same as in SMSG_SPELL_START
        castFlags |= CAST_FLAG_PREDICTED_POWER;             // makes cooldowns visible
        castFlags |= CAST_FLAG_PREDICTED_RUNES;             // rune cooldowns list
    }

    if (m_powerCost)
    {
        castFlags |= CAST_FLAG_PREDICTED_POWER;             // all powerCost spells have this
    }

    WorldPacket data(SMSG_SPELL_GO, 50);                    // guess size

    if (m_CastItem)
    {
        data << m_CastItem->GetPackGUID();
    }
    else
    {
        data << m_caster->GetPackGUID();
    }

    data << m_caster->GetPackGUID();
    data << uint8(m_cast_count);                            // pending spell cast?
    data << uint32(m_spellInfo->ID);                        // spellId
    data << uint32(castFlags);                              // cast flags
    data << uint32(GameTime::GetGameTimeMS());              // timestamp

    WriteSpellGoTargets(&data);

    data << m_targets;

    if (castFlags & CAST_FLAG_PREDICTED_POWER)              // predicted power
    {
        data << uint32(m_caster->GetPower((Powers)m_spellInfo->PowerType));
    }

    if (castFlags & CAST_FLAG_PREDICTED_RUNES)              // predicted runes
    {
        uint8 v1 = m_runesState;
        uint8 v2 =  m_caster->getClass() == CLASS_DEATH_KNIGHT ? ((Player*)m_caster)->GetRunesState() : 0;
        data << uint8(v1);                                  // runes state before
        data << uint8(v2);                                  // runes state after
        for (uint8 i = 0; i < MAX_RUNES; ++i)
        {
            uint8 m = (1 << i);
            if (m & v1)                                     // usable before...
            {
                if (!(m & v2))                              // ...but on cooldown now...
                {
                    data << uint8(0);                       // some unknown byte (time?)
                }
            }
        }
    }

    if (castFlags & CAST_FLAG_ADJUST_MISSILE)               // adjust missile trajectory duration
    {
        data << float(0);
        data << uint32(0);
    }

    if (castFlags & CAST_FLAG_AMMO)                         // projectile info
    {
        WriteAmmoToPacket(&data);
    }

    if (castFlags & CAST_FLAG_VISUAL_CHAIN)                 // spell visual chain effect
    {
        data << uint32(0);                                  // SpellVisual.dbc id?
        data << uint32(0);                                  // overrides previous field if > 0 and violencelevel client cvar < 2
    }

    if (m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION)
    {
        data << uint8(0);                                   // The value increase for each time, can remind of a cast count for the spell
    }

    if (m_targets.m_targetMask & TARGET_FLAG_VISUAL_CHAIN)  // probably used (or can be used) with CAST_FLAG_VISUAL_CHAIN flag
    {
        data << uint32(0);                                  // count

        // for(int = 0; i < count; ++i)
        //{
        //    // position and guid?
        //    data << float(0) << float(0) << float(0) << uint64(0);
        //}
    }

    m_caster->SendMessageToSet(&data, true);
}

/**
 * @brief Writes projectile display and inventory type data into a packet.
 *
 * @param data The packet being populated.
 */
void Spell::WriteAmmoToPacket(WorldPacket* data)
{
    uint32 ammoInventoryType = 0;
    uint32 ammoDisplayID = 0;

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        Item* pItem = ((Player*)m_caster)->GetWeaponForAttack(RANGED_ATTACK);
        if (pItem)
        {
            ammoInventoryType = pItem->GetProto()->InventoryType;
            if (ammoInventoryType == INVTYPE_THROWN)
            {
                ammoDisplayID = pItem->GetProto()->DisplayInfoID;
            }
            else
            {
                uint32 ammoID = ((Player*)m_caster)->GetUInt32Value(PLAYER_AMMO_ID);
                if (ammoID)
                {
                    ItemPrototype const* pProto = ObjectMgr::GetItemPrototype(ammoID);
                    if (pProto)
                    {
                        ammoDisplayID = pProto->DisplayInfoID;
                        ammoInventoryType = pProto->InventoryType;
                    }
                }
                else if (m_caster->GetDummyAura(46699))     // Requires No Ammo
                {
                    ammoDisplayID = 5996;                   // normal arrow
                    ammoInventoryType = INVTYPE_AMMO;
                }
            }
        }
    }
    else
    {
        for (uint8 i = 0; i < MAX_VIRTUAL_ITEM_SLOT; ++i)
        {
            // see Creature::SetVirtualItem for structure data
            if (uint32 item_id = m_caster->GetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + i))
            {
                if (ItemEntry const* itemEntry = sItemStore.LookupEntry(item_id))
                {
                    if (itemEntry->ClassID == ITEM_CLASS_WEAPON)
                    {
                        switch (itemEntry->SubclassID)
                        {
                            case ITEM_SUBCLASS_WEAPON_THROWN:
                                ammoDisplayID = itemEntry->DisplayInfoID;
                                ammoInventoryType = itemEntry->InventoryType;
                                break;
                            case ITEM_SUBCLASS_WEAPON_BOW:
                            case ITEM_SUBCLASS_WEAPON_CROSSBOW:
                                ammoDisplayID = 5996;       // is this need fixing?
                                ammoInventoryType = INVTYPE_AMMO;
                                break;
                            case ITEM_SUBCLASS_WEAPON_GUN:
                                ammoDisplayID = 5998;       // is this need fixing?
                                ammoInventoryType = INVTYPE_AMMO;
                                break;
                        }

                        if (ammoDisplayID)
                        {
                            break;
                        }
                    }
                }
            }
        }
    }

    *data << uint32(ammoDisplayID);
    *data << uint32(ammoInventoryType);
}

/**
 * @brief Writes spell target guids into the spell-go packet and updates alive-target tracking.
 *
 * @param data The packet being populated.
 */
void Spell::WriteSpellGoTargets(WorldPacket* data)
{
    size_t count_pos = data->wpos();
    *data << uint8(0);                                      // placeholder

    // This function also fill data for channeled spells:
    // m_needAliveTargetMask req for stop channeling if one target die
    uint32 hit  = m_UniqueGOTargetInfo.size();              // Always hits on GO
    uint32 miss = 0;

    for (TargetList::iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
    {
        if (ihit->effectMask == 0)                          // No effect apply - all immuned add state
        {
            // possibly SPELL_MISS_IMMUNE2 for this??
            ihit->missCondition = SPELL_MISS_IMMUNE2;
            ++miss;
        }
        else if (ihit->missCondition == SPELL_MISS_NONE)    // Add only hits
        {
            ++hit;
            *data << ihit->targetGUID;
            m_needAliveTargetMask |= ihit->effectMask;
        }
        else
        {
            ++miss;
        }
    }

    for (GOTargetList::const_iterator ighit = m_UniqueGOTargetInfo.begin(); ighit != m_UniqueGOTargetInfo.end(); ++ighit)
    {
        *data << ighit->targetGUID;                          // Always hits
    }

    data->put<uint8>(count_pos, hit);

    *data << (uint8)miss;
    for (TargetList::const_iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
    {
        if (ihit->missCondition != SPELL_MISS_NONE)         // Add only miss
        {
            *data << ihit->targetGUID;
            *data << uint8(ihit->missCondition);
            if (ihit->missCondition == SPELL_MISS_REFLECT)
            {
                *data << uint8(ihit->reflectResult);
            }
        }
    }
    // Reset m_needAliveTargetMask for non channeled spell
    if (!IsChanneledSpell(m_spellInfo))
    {
        m_needAliveTargetMask = 0;
    }
}

/**
 * @brief Sends the spell log execute packet for special client-side effect logging.
 */
void Spell::SendLogExecute()
{
    Unit* target = m_targets.getUnitTarget() ? m_targets.getUnitTarget() : m_caster;

    WorldPacket data(SMSG_SPELLLOGEXECUTE, (8 + 4 + 4 + 4 + 4 + 8));

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        data << m_caster->GetPackGUID();
    }
    else
    {
        data << target->GetPackGUID();
    }

    data << uint32(m_spellInfo->ID);
    uint32 count1 = 1;
    data << uint32(count1);                                 // count1 (effect count?)
    for (uint32 i = 0; i < count1; ++i)
    {
        data << uint32(m_spellInfo->Effect[EFFECT_INDEX_0]);// spell effect
        uint32 count2 = 1;
        data << uint32(count2);                             // count2 (target count?)
        for (uint32 j = 0; j < count2; ++j)
        {
            switch (m_spellInfo->Effect[EFFECT_INDEX_0])
            {
                case SPELL_EFFECT_POWER_DRAIN:
                    if (Unit* unit = m_targets.getUnitTarget())
                    {
                        data << unit->GetPackGUID();
                    }
                    else
                    {
                        data << uint8(0);
                    }
                    data << uint32(0);
                    data << uint32(0);
                    data << float(0);
                    break;
                case SPELL_EFFECT_ADD_EXTRA_ATTACKS:
                    if (Unit* unit = m_targets.getUnitTarget())
                    {
                        data << unit->GetPackGUID();
                    }
                    else
                    {
                        data << uint8(0);
                    }
                    data << uint32(0);                      // count?
                    break;
                case SPELL_EFFECT_INTERRUPT_CAST:
                    if (Unit* unit = m_targets.getUnitTarget())
                    {
                        data << unit->GetPackGUID();
                    }
                    else
                    {
                        data << uint8(0);
                    }
                    data << uint32(0);                      // spellid
                    break;
                case SPELL_EFFECT_DURABILITY_DAMAGE:
                    if (Unit* unit = m_targets.getUnitTarget())
                    {
                        data << unit->GetPackGUID();
                    }
                    else
                    {
                        data << uint8(0);
                    }
                    data << uint32(0);
                    data << uint32(0);
                    break;
                case SPELL_EFFECT_OPEN_LOCK:
                    if (Item* item = m_targets.getItemTarget())
                    {
                        data << item->GetPackGUID();
                    }
                    else
                    {
                        data << uint8(0);
                    }
                    break;
                case SPELL_EFFECT_CREATE_ITEM:
                case SPELL_EFFECT_CREATE_ITEM_2:
                    data << uint32(m_spellInfo->EffectItemType[EFFECT_INDEX_0]);
                    break;
                case SPELL_EFFECT_SUMMON:
                case SPELL_EFFECT_TRANS_DOOR:
                case SPELL_EFFECT_SUMMON_PET:
                case SPELL_EFFECT_SUMMON_OBJECT_WILD:
                case SPELL_EFFECT_CREATE_HOUSE:
                case SPELL_EFFECT_DUEL:
                case SPELL_EFFECT_SUMMON_OBJECT_SLOT1:
                case SPELL_EFFECT_SUMMON_OBJECT_SLOT2:
                case SPELL_EFFECT_SUMMON_OBJECT_SLOT3:
                case SPELL_EFFECT_SUMMON_OBJECT_SLOT4:
                    if (Unit* unit = m_targets.getUnitTarget())
                    {
                        data << unit->GetPackGUID();
                    }
                    else if (m_targets.getItemTargetGuid())
                    {
                        data << m_targets.getItemTargetGuid().WriteAsPacked();
                    }
                    else if (GameObject* go = m_targets.getGOTarget())
                    {
                        data << go->GetPackGUID();
                    }
                    else
                    {
                        data << uint8(0);                    // guid
                    }
                    break;
                case SPELL_EFFECT_FEED_PET:
                    data << uint32(m_targets.getItemTargetEntry());
                    break;
                case SPELL_EFFECT_DISMISS_PET:
                    if (Unit* unit = m_targets.getUnitTarget())
                    {
                        data << unit->GetPackGUID();
                    }
                    else
                    {
                        data << uint8(0);
                    }
                    break;
                case SPELL_EFFECT_RESURRECT:
                case SPELL_EFFECT_RESURRECT_NEW:
                    if (Unit* unit = m_targets.getUnitTarget())
                    {
                        data << unit->GetPackGUID();
                    }
                    else
                    {
                        data << uint8(0);
                    }
                    break;
                default:
                    return;
            }
        }
    }

    m_caster->SendMessageToSet(&data, true);
}

/**
 * @brief Sends interruption packets for the current spell cast.
 *
 * @param result The interruption result code.
 */
void Spell::SendInterrupted(uint8 result)
{
    WorldPacket data(SMSG_SPELL_FAILURE, (8 + 4 + 1));
    data << m_caster->GetPackGUID();
    data << uint8(m_cast_count);
    data << uint32(m_spellInfo->ID);
    data << uint8(result);
    m_caster->SendMessageToSet(&data, true);

    data.Initialize(SMSG_SPELL_FAILED_OTHER, (8 + 4));
    data << m_caster->GetPackGUID();
    data << uint8(m_cast_count);
    data << uint32(m_spellInfo->ID);
    data << uint8(result);
    m_caster->SendMessageToSet(&data, true);
}

/**
 * @brief Sends channel progress updates and clears channel state when ending.
 *
 * @param time The remaining channel time.
 */
void Spell::SendChannelUpdate(uint32 time)
{
    if (time == 0)
    {
        // Reset farsight for some possessing auras of possessed summoned (as they might work with different aura types)
        if (m_spellInfo->HasAttribute(SPELL_ATTR_EX_FARSIGHT) && m_caster->GetTypeId() == TYPEID_PLAYER && m_caster->GetCharmGuid()
            && !IsSpellHaveAura(m_spellInfo, SPELL_AURA_MOD_POSSESS) && !IsSpellHaveAura(m_spellInfo, SPELL_AURA_MOD_POSSESS_PET))
        {
            Player* player = (Player*)m_caster;
            // These Auras are applied to self, so get the possessed first
            Unit* possessed = player->GetCharm();

            player->SetCharm(NULL);
            if (possessed)
            {
                player->SetClientControl(possessed, 0);
            }
            player->SetMover(NULL);
            player->GetCamera().ResetView();
            player->RemovePetActionBar();

            if (possessed)
            {
                possessed->clearUnitState(UNIT_STAT_CONTROLLED);
                possessed->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);
                possessed->SetCharmerGuid(ObjectGuid());
                // TODO - Requires more specials for target?

                // Some possessed might want to despawn?
                if (possessed->GetUInt32Value(UNIT_CREATED_BY_SPELL) == m_spellInfo->ID && possessed->GetTypeId() == TYPEID_UNIT)
                {
                    ((Creature*)possessed)->ForcedDespawn();
                }
            }
        }

        m_caster->RemoveAurasByCasterSpell(m_spellInfo->ID, m_caster->GetObjectGuid());

        ObjectGuid target_guid = m_caster->GetChannelObjectGuid();
        if (target_guid != m_caster->GetObjectGuid() && target_guid.IsUnit())
        {
            if (Unit* target = sObjectAccessor.GetUnit(*m_caster, target_guid))
            {
                target->RemoveAurasByCasterSpell(m_spellInfo->ID, m_caster->GetObjectGuid());
            }
        }

        // Only finish channeling when latest channeled spell finishes
        if (m_caster->GetUInt32Value(UNIT_CHANNEL_SPELL) != m_spellInfo->ID)
        {
            return;
        }

        m_caster->SetChannelObjectGuid(ObjectGuid());
        m_caster->SetUInt32Value(UNIT_CHANNEL_SPELL, 0);
    }

    WorldPacket data(MSG_CHANNEL_UPDATE, 8 + 4);
    data << m_caster->GetPackGUID();
    data << uint32(time);
    m_caster->SendMessageToSet(&data, true);
}

/**
 * @brief Starts channeling visuals and channel state for the spell.
 *
 * @param duration The channel duration in milliseconds.
 */
void Spell::SendChannelStart(uint32 duration)
{
    WorldObject* target = NULL;

    // select dynobject created by first effect if any
    if (m_spellInfo->Effect[EFFECT_INDEX_0] == SPELL_EFFECT_PERSISTENT_AREA_AURA)
    {
        target = m_caster->GetDynObject(m_spellInfo->ID, EFFECT_INDEX_0);
    }
    // select first not resisted target from target list for _0_ effect
    else if (!m_UniqueTargetInfo.empty())
    {
        for (TargetList::const_iterator itr = m_UniqueTargetInfo.begin(); itr != m_UniqueTargetInfo.end(); ++itr)
        {
            if ((itr->effectMask & (1 << EFFECT_INDEX_0)) && itr->reflectResult == SPELL_MISS_NONE &&
                itr->targetGUID != m_caster->GetObjectGuid())
            {
                target = sObjectAccessor.GetUnit(*m_caster, itr->targetGUID);
                break;
            }
        }
    }
    else if (!m_UniqueGOTargetInfo.empty())
    {
        for (GOTargetList::const_iterator itr = m_UniqueGOTargetInfo.begin(); itr != m_UniqueGOTargetInfo.end(); ++itr)
        {
            if (itr->effectMask & (1 << EFFECT_INDEX_0))
            {
                target = m_caster->GetMap()->GetGameObject(itr->targetGUID);
                break;
            }
        }
    }

    WorldPacket data(MSG_CHANNEL_START, (8 + 4 + 4));
    data << m_caster->GetPackGUID();
    data << uint32(m_spellInfo->ID);
    data << uint32(duration);
    m_caster->SendMessageToSet(&data, true);

    m_timer = duration;

    if (target)
    {
        m_caster->SetChannelObjectGuid(target->GetObjectGuid());
    }

    m_caster->SetUInt32Value(UNIT_CHANNEL_SPELL, m_spellInfo->ID);
}

/**
 * @brief Sends a resurrection request to the target player.
 *
 * @param target The player being offered resurrection.
 */
void Spell::SendResurrectRequest(Player* target)
{
    // Both players and NPCs can resurrect using spells - have a look at creature 28487 for example
    // However, the packet structure differs slightly

    const char* sentName = m_caster->GetTypeId() == TYPEID_PLAYER ? "" : m_caster->GetNameForLocaleIdx(target->GetSession()->GetSessionDbLocaleIndex());

    WorldPacket data(SMSG_RESURRECT_REQUEST, (8 + 4 + strlen(sentName) + 1 + 1 + 1));
    data << m_caster->GetObjectGuid();
    data << uint32(strlen(sentName) + 1);

    data << sentName;
    data << uint8(0);

    data << uint8(m_caster->GetTypeId() == TYPEID_PLAYER ? 0 : 1);
    target->GetSession()->SendPacket(&data);
}

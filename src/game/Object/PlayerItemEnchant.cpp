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
 * @file PlayerItemEnchant.cpp
 * @brief Cohesion split of Player.cpp -- item duration + enchantment lifecycle and item packet senders.
 */

#include "Player.h"
#include "Log.h"
#include "Opcodes.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "DBCStores.h"

/**
 * @brief Updates durations for items tracked by the player.
 *
 * @param time The elapsed time in milliseconds.
 * @param realtimeonly True to update only items using real-time duration tracking.
 */
void Player::UpdateItemDuration(uint32 time, bool realtimeonly)
{
    if (m_itemDuration.empty())
    {
        return;
    }

    DEBUG_LOG("Player::UpdateItemDuration(%u,%u)", time, realtimeonly);

    for (ItemDurationList::const_iterator itr = m_itemDuration.begin(); itr != m_itemDuration.end();)
    {
        Item* item = *itr;
        ++itr;                                              // current element can be erased in UpdateDuration

        if (!(realtimeonly) || (item->GetProto()->ExtraFlags & ITEM_EXTRA_REAL_TIME_DURATION))
        {
            item->UpdateDuration(this, time);
        }
    }
}

/**
 * @brief Updates remaining durations for temporary item enchantments.
 *
 * @param time The elapsed time in milliseconds.
 */
void Player::UpdateEnchantTime(uint32 time)
{
    for (EnchantDurationList::iterator itr = m_enchantDuration.begin(), next; itr != m_enchantDuration.end(); itr = next)
    {
        MANGOS_ASSERT(itr->item);
        next = itr;
        if (!itr->item->GetEnchantmentId(itr->slot))
        {
            next = m_enchantDuration.erase(itr);
        }
        else if (itr->leftduration <= time)
        {
            ApplyEnchantment(itr->item, itr->slot, false, false);
            itr->item->ClearEnchantment(itr->slot);
            next = m_enchantDuration.erase(itr);
        }
        else if (itr->leftduration > time)
        {
            itr->leftduration -= time;
            ++next;
        }
    }
}

/**
 * @brief Registers timed enchantments from an item for duration tracking.
 *
 * @param item The item whose enchantments should be tracked.
 */
void Player::AddEnchantmentDurations(Item* item)
{
    for (int x = 0; x < MAX_ENCHANTMENT_SLOT; ++x)
    {
        if (!item->GetEnchantmentId(EnchantmentSlot(x)))
        {
            continue;
        }

        uint32 duration = item->GetEnchantmentDuration(EnchantmentSlot(x));
        if (duration > 0)
        {
            AddEnchantmentDuration(item, EnchantmentSlot(x), duration);
        }
    }
}

/**
 * @brief Removes an item's enchantments from duration tracking and stores remaining time.
 *
 * @param item The item whose enchantments should be untracked.
 */
void Player::RemoveEnchantmentDurations(Item* item)
{
    for (EnchantDurationList::iterator itr = m_enchantDuration.begin(); itr != m_enchantDuration.end();)
    {
        if (itr->item == item)
        {
            // save duration in item
            item->SetEnchantmentDuration(EnchantmentSlot(itr->slot), itr->leftduration);
            itr = m_enchantDuration.erase(itr);
        }
        else
        {
            ++itr;
        }
    }
}

/**
 * @brief Removes all enchantments of a given slot type from equipped and stored items.
 *
 * @param slot The enchantment slot type to clear.
 */
void Player::RemoveAllEnchantments(EnchantmentSlot slot)
{
    // remove enchantments from equipped items first to clean up the m_enchantDuration list
    for (EnchantDurationList::iterator itr = m_enchantDuration.begin(), next; itr != m_enchantDuration.end(); itr = next)
    {
        next = itr;
        if (itr->slot == slot)
        {
            if (itr->item && itr->item->GetEnchantmentId(slot))
            {
                // remove from stats
                ApplyEnchantment(itr->item, slot, false, false);
                // remove visual
                itr->item->ClearEnchantment(slot);
            }
            // remove from update list
            next = m_enchantDuration.erase(itr);
        }
        else
        {
            ++next;
        }
    }

    // remove enchants from inventory items
    // NOTE: no need to remove these from stats, since these aren't equipped
    // in inventory
    for (int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        if (Item* pItem = GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            if (pItem->GetEnchantmentId(slot))
            {
                pItem->ClearEnchantment(slot);
            }
    }

    // in inventory bags
    for (int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        if (Bag* pBag = reinterpret_cast<Bag*>(GetItemByPos(INVENTORY_SLOT_BAG_0, i)))
            for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
            {
                if (Item* pItem = pBag->GetItemByPos(j))
                    if (pItem->GetEnchantmentId(slot))
                    {
                        pItem->ClearEnchantment(slot);
                    }
            }
    }
}

// duration == 0 will remove item enchant
void Player::AddEnchantmentDuration(Item* item, EnchantmentSlot slot, uint32 duration)
{
    if (!item)
    {
        return;
    }

    if (slot >= MAX_ENCHANTMENT_SLOT)
    {
        return;
    }

    for (EnchantDurationList::iterator itr = m_enchantDuration.begin(); itr != m_enchantDuration.end(); ++itr)
    {
        if (itr->item == item && itr->slot == slot)
        {
            itr->item->SetEnchantmentDuration(itr->slot, itr->leftduration);
            m_enchantDuration.erase(itr);
            break;
        }
    }
    if (item && duration > 0)
    {
        GetSession()->SendItemEnchantTimeUpdate(GetObjectGuid(), item->GetObjectGuid(), slot, uint32(duration / 1000));
        m_enchantDuration.push_back(EnchantDuration(item, slot, duration));
    }
}

/**
 * @brief Applies or removes all enchantments on an equipped item.
 *
 * @param item The item whose enchantments should be processed.
 * @param apply True to apply enchantments; false to remove them.
 */
void Player::ApplyEnchantment(Item* item, bool apply)
{
    for (uint32 slot = 0; slot < MAX_ENCHANTMENT_SLOT; ++slot)
    {
        ApplyEnchantment(item, EnchantmentSlot(slot), apply);
    }
}

/**
 * @brief Applies or removes a specific enchantment slot on an equipped item.
 *
 * @param item The equipped item whose enchantment should be processed.
 * @param slot The enchantment slot to process.
 * @param apply True to apply the enchantment; false to remove it.
 * @param apply_dur True to update tracked enchantment duration state.
 * @param ignore_condition Unused flag for conditional processing.
 */
void Player::ApplyEnchantment(Item* item, EnchantmentSlot slot, bool apply, bool apply_dur, bool ignore_condition)
{
    if (!item)
    {
        return;
    }

    if (!item->IsEquipped())
    {
        return;
    }

    if (slot >= MAX_ENCHANTMENT_SLOT)
    {
        return;
    }

    uint32 enchant_id = item->GetEnchantmentId(slot);
    if (!enchant_id)
    {
        return;
    }

    SpellItemEnchantmentEntry const* pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
    if (!pEnchant)
    {
        return;
    }

    if (!ignore_condition && pEnchant->EnchantmentCondition && !EnchantmentFitsRequirements(pEnchant->EnchantmentCondition, -1))
    {
        return;
    }

    if (!item->IsBroken())
    {
        for (int s = 0; s < 3; ++s)
        {
            uint32 enchant_display_type = pEnchant->type[s];
            uint32 enchant_amount = pEnchant->amount[s];
            uint32 enchant_spell_id = pEnchant->spellid[s];

            switch (enchant_display_type)
            {
                case ITEM_ENCHANTMENT_TYPE_NONE:
                    break;
                case ITEM_ENCHANTMENT_TYPE_COMBAT_SPELL:
                    // processed in Player::CastItemCombatSpell
                    break;
                case ITEM_ENCHANTMENT_TYPE_DAMAGE:
                    if (item->GetSlot() == EQUIPMENT_SLOT_MAINHAND)
                    {
                        HandleStatModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_VALUE, float(enchant_amount), apply);
                    }
                    else if (item->GetSlot() == EQUIPMENT_SLOT_OFFHAND)
                    {
                        HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_VALUE, float(enchant_amount), apply);
                    }
                    else if (item->GetSlot() == EQUIPMENT_SLOT_RANGED)
                    {
                        HandleStatModifier(UNIT_MOD_DAMAGE_RANGED, TOTAL_VALUE, float(enchant_amount), apply);
                    }
                    break;
                case ITEM_ENCHANTMENT_TYPE_EQUIP_SPELL:
                {
                    if (enchant_spell_id)
                    {
                        if (apply)
                        {
                            int32 basepoints = 0;
                            // Random Property Exist - try found basepoints for spell (basepoints depends from item suffix factor)
                            if (item->GetItemRandomPropertyId())
                            {
                                ItemRandomSuffixEntry const* item_rand = sItemRandomSuffixStore.LookupEntry(abs(item->GetItemRandomPropertyId()));
                                if (item_rand)
                                {
                                    // Search enchant_amount
                                    for (int k = 0; k < 3; ++k)
                                    {
                                        if (item_rand->enchant_id[k] == enchant_id)
                                        {
                                            basepoints = int32((item_rand->prefix[k] * item->GetItemSuffixFactor()) / 10000);
                                            break;
                                        }
                                    }
                                }
                            }
                            // Cast custom spell vs all equal basepoints got from enchant_amount
                            if (basepoints)
                            {
                                CastCustomSpell(this, enchant_spell_id, &basepoints, &basepoints, &basepoints, true, item);
                            }
                            else
                            {
                                CastSpell(this, enchant_spell_id, true, item);
                            }
                        }
                        else
                        {
                            RemoveAurasDueToItemSpell(item, enchant_spell_id);
                        }
                    }
                    break;
                }
                case ITEM_ENCHANTMENT_TYPE_RESISTANCE:
                    if (!enchant_amount)
                    {
                        ItemRandomSuffixEntry const* item_rand = sItemRandomSuffixStore.LookupEntry(abs(item->GetItemRandomPropertyId()));
                        if (item_rand)
                        {
                            for (int k = 0; k < 3; ++k)
                            {
                                if (item_rand->enchant_id[k] == enchant_id)
                                {
                                    enchant_amount = uint32((item_rand->prefix[k] * item->GetItemSuffixFactor()) / 10000);
                                    break;
                                }
                            }
                        }
                    }

                    HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + enchant_spell_id), TOTAL_VALUE, float(enchant_amount), apply);
                    break;
                case ITEM_ENCHANTMENT_TYPE_STAT:
                {
                    if (!enchant_amount)
                    {
                        ItemRandomSuffixEntry const* item_rand_suffix = sItemRandomSuffixStore.LookupEntry(abs(item->GetItemRandomPropertyId()));
                        if (item_rand_suffix)
                        {
                            for (int k = 0; k < 3; ++k)
                            {
                                if (item_rand_suffix->enchant_id[k] == enchant_id)
                                {
                                    enchant_amount = uint32((item_rand_suffix->prefix[k] * item->GetItemSuffixFactor()) / 10000);
                                    break;
                                }
                            }
                        }
                    }

                    DEBUG_LOG("Adding %u to stat nb %u", enchant_amount, enchant_spell_id);
                    switch (enchant_spell_id)
                    {
                        case ITEM_MOD_MANA:
                            DEBUG_LOG("+ %u MANA", enchant_amount);
                            HandleStatModifier(UNIT_MOD_MANA, BASE_VALUE, float(enchant_amount), apply);
                            break;
                        case ITEM_MOD_HEALTH:
                            DEBUG_LOG("+ %u HEALTH", enchant_amount);
                            HandleStatModifier(UNIT_MOD_HEALTH, BASE_VALUE, float(enchant_amount), apply);
                            break;
                        case ITEM_MOD_AGILITY:
                            DEBUG_LOG("+ %u AGILITY", enchant_amount);
                            HandleStatModifier(UNIT_MOD_STAT_AGILITY, TOTAL_VALUE, float(enchant_amount), apply);
                            ApplyStatBuffMod(STAT_AGILITY, float(enchant_amount), apply);
                            break;
                        case ITEM_MOD_STRENGTH:
                            DEBUG_LOG("+ %u STRENGTH", enchant_amount);
                            HandleStatModifier(UNIT_MOD_STAT_STRENGTH, TOTAL_VALUE, float(enchant_amount), apply);
                            ApplyStatBuffMod(STAT_STRENGTH, float(enchant_amount), apply);
                            break;
                        case ITEM_MOD_INTELLECT:
                            DEBUG_LOG("+ %u INTELLECT", enchant_amount);
                            HandleStatModifier(UNIT_MOD_STAT_INTELLECT, TOTAL_VALUE, float(enchant_amount), apply);
                            ApplyStatBuffMod(STAT_INTELLECT, float(enchant_amount), apply);
                            break;
                        case ITEM_MOD_SPIRIT:
                            DEBUG_LOG("+ %u SPIRIT", enchant_amount);
                            HandleStatModifier(UNIT_MOD_STAT_SPIRIT, TOTAL_VALUE, float(enchant_amount), apply);
                            ApplyStatBuffMod(STAT_SPIRIT, float(enchant_amount), apply);
                            break;
                        case ITEM_MOD_STAMINA:
                            DEBUG_LOG("+ %u STAMINA", enchant_amount);
                            HandleStatModifier(UNIT_MOD_STAT_STAMINA, TOTAL_VALUE, float(enchant_amount), apply);
                            ApplyStatBuffMod(STAT_STAMINA, float(enchant_amount), apply);
                            break;
                        case ITEM_MOD_DEFENSE_SKILL_RATING:
                            ApplyRatingMod(CR_DEFENSE_SKILL, enchant_amount, apply);
                            DEBUG_LOG("+ %u DEFENCE", enchant_amount);
                            break;
                        case  ITEM_MOD_DODGE_RATING:
                            ApplyRatingMod(CR_DODGE, enchant_amount, apply);
                            DEBUG_LOG("+ %u DODGE", enchant_amount);
                            break;
                        case ITEM_MOD_PARRY_RATING:
                            ApplyRatingMod(CR_PARRY, enchant_amount, apply);
                            DEBUG_LOG("+ %u PARRY", enchant_amount);
                            break;
                        case ITEM_MOD_BLOCK_RATING:
                            ApplyRatingMod(CR_BLOCK, enchant_amount, apply);
                            DEBUG_LOG("+ %u SHIELD_BLOCK", enchant_amount);
                            break;
                        case ITEM_MOD_HIT_MELEE_RATING:
                            ApplyRatingMod(CR_HIT_MELEE, enchant_amount, apply);
                            DEBUG_LOG("+ %u MELEE_HIT", enchant_amount);
                            break;
                        case ITEM_MOD_HIT_RANGED_RATING:
                            ApplyRatingMod(CR_HIT_RANGED, enchant_amount, apply);
                            DEBUG_LOG("+ %u RANGED_HIT", enchant_amount);
                            break;
                        case ITEM_MOD_HIT_SPELL_RATING:
                            ApplyRatingMod(CR_HIT_SPELL, enchant_amount, apply);
                            DEBUG_LOG("+ %u SPELL_HIT", enchant_amount);
                            break;
                        case ITEM_MOD_CRIT_MELEE_RATING:
                            ApplyRatingMod(CR_CRIT_MELEE, enchant_amount, apply);
                            DEBUG_LOG("+ %u MELEE_CRIT", enchant_amount);
                            break;
                        case ITEM_MOD_CRIT_RANGED_RATING:
                            ApplyRatingMod(CR_CRIT_RANGED, enchant_amount, apply);
                            DEBUG_LOG("+ %u RANGED_CRIT", enchant_amount);
                            break;
                        case ITEM_MOD_CRIT_SPELL_RATING:
                            ApplyRatingMod(CR_CRIT_SPELL, enchant_amount, apply);
                            DEBUG_LOG("+ %u SPELL_CRIT", enchant_amount);
                            break;
//                        Values from ITEM_STAT_MELEE_HA_RATING to ITEM_MOD_HASTE_RANGED_RATING are never used
//                        in Enchantments
//                        case ITEM_MOD_HIT_TAKEN_MELEE_RATING:
//                            ApplyRatingMod(CR_HIT_TAKEN_MELEE, enchant_amount, apply);
//                            break;
//                        case ITEM_MOD_HIT_TAKEN_RANGED_RATING:
//                            ApplyRatingMod(CR_HIT_TAKEN_RANGED, enchant_amount, apply);
//                            break;
//                        case ITEM_MOD_HIT_TAKEN_SPELL_RATING:
//                            ApplyRatingMod(CR_HIT_TAKEN_SPELL, enchant_amount, apply);
//                            break;
//                        case ITEM_MOD_CRIT_TAKEN_MELEE_RATING:
//                            ApplyRatingMod(CR_CRIT_TAKEN_MELEE, enchant_amount, apply);
//                            break;
//                        case ITEM_MOD_CRIT_TAKEN_RANGED_RATING:
//                            ApplyRatingMod(CR_CRIT_TAKEN_RANGED, enchant_amount, apply);
//                            break;
//                        case ITEM_MOD_CRIT_TAKEN_SPELL_RATING:
//                            ApplyRatingMod(CR_CRIT_TAKEN_SPELL, enchant_amount, apply);
//                            break;
//                        case ITEM_MOD_HASTE_MELEE_RATING:
//                            ApplyRatingMod(CR_HASTE_MELEE, enchant_amount, apply);
//                            break;
//                        case ITEM_MOD_HASTE_RANGED_RATING:
//                            ApplyRatingMod(CR_HASTE_RANGED, enchant_amount, apply);
//                            break;
                        case ITEM_MOD_HASTE_SPELL_RATING:
                            ApplyRatingMod(CR_HASTE_SPELL, enchant_amount, apply);
                            break;
                        case ITEM_MOD_HIT_RATING:
                            ApplyRatingMod(CR_HIT_MELEE, enchant_amount, apply);
                            ApplyRatingMod(CR_HIT_RANGED, enchant_amount, apply);
                            ApplyRatingMod(CR_HIT_SPELL, enchant_amount, apply);
                            DEBUG_LOG("+ %u HIT", enchant_amount);
                            break;
                        case ITEM_MOD_CRIT_RATING:
                            ApplyRatingMod(CR_CRIT_MELEE, enchant_amount, apply);
                            ApplyRatingMod(CR_CRIT_RANGED, enchant_amount, apply);
                            ApplyRatingMod(CR_CRIT_SPELL, enchant_amount, apply);
                            DEBUG_LOG("+ %u CRITICAL", enchant_amount);
                            break;
//                        Values ITEM_MOD_HIT_TAKEN_RATING and ITEM_MOD_CRIT_TAKEN_RATING are never used in Enchantment
//                        case ITEM_MOD_HIT_TAKEN_RATING:
//                            ApplyRatingMod(CR_HIT_TAKEN_MELEE, enchant_amount, apply);
//                            ApplyRatingMod(CR_HIT_TAKEN_RANGED, enchant_amount, apply);
//                            ApplyRatingMod(CR_HIT_TAKEN_SPELL, enchant_amount, apply);
//                            break;
//                        case ITEM_MOD_CRIT_TAKEN_RATING:
//                            ApplyRatingMod(CR_CRIT_TAKEN_MELEE, enchant_amount, apply);
//                            ApplyRatingMod(CR_CRIT_TAKEN_RANGED, enchant_amount, apply);
//                            ApplyRatingMod(CR_CRIT_TAKEN_SPELL, enchant_amount, apply);
//                            break;
                        case ITEM_MOD_RESILIENCE_RATING:
                            ApplyRatingMod(CR_CRIT_TAKEN_MELEE, enchant_amount, apply);
                            ApplyRatingMod(CR_CRIT_TAKEN_RANGED, enchant_amount, apply);
                            ApplyRatingMod(CR_CRIT_TAKEN_SPELL, enchant_amount, apply);
                            DEBUG_LOG("+ %u RESILIENCE", enchant_amount);
                            break;
                        case ITEM_MOD_HASTE_RATING:
                            ApplyRatingMod(CR_HASTE_MELEE, enchant_amount, apply);
                            ApplyRatingMod(CR_HASTE_RANGED, enchant_amount, apply);
                            ApplyRatingMod(CR_HASTE_SPELL, enchant_amount, apply);
                            DEBUG_LOG("+ %u HASTE", enchant_amount);
                            break;
                        case ITEM_MOD_EXPERTISE_RATING:
                            ApplyRatingMod(CR_EXPERTISE, enchant_amount, apply);
                            DEBUG_LOG("+ %u EXPERTISE", enchant_amount);
                            break;
                        case ITEM_MOD_ATTACK_POWER:
                            HandleStatModifier(UNIT_MOD_ATTACK_POWER, TOTAL_VALUE, float(enchant_amount), apply);
                            HandleStatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_VALUE, float(enchant_amount), apply);
                            DEBUG_LOG("+ %u ATTACK_POWER", enchant_amount);
                            break;
                        case ITEM_MOD_RANGED_ATTACK_POWER:
                            HandleStatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_VALUE, float(enchant_amount), apply);
                            DEBUG_LOG("+ %u RANGED_ATTACK_POWER", enchant_amount);
                            break;
                        case ITEM_MOD_MANA_REGENERATION:
                            ApplyManaRegenBonus(enchant_amount, apply);
                            DEBUG_LOG("+ %u MANA_REGENERATION", enchant_amount);
                            break;
                        case ITEM_MOD_ARMOR_PENETRATION_RATING:
                            ApplyRatingMod(CR_ARMOR_PENETRATION, enchant_amount, apply);
                            DEBUG_LOG("+ %u ARMOR PENETRATION", enchant_amount);
                            break;
                        case ITEM_MOD_SPELL_POWER:
                            ApplySpellPowerBonus(enchant_amount, apply);
                            DEBUG_LOG("+ %u SPELL_POWER", enchant_amount);
                            break;
                        case ITEM_MOD_BLOCK_VALUE:
                            HandleBaseModValue(SHIELD_BLOCK_VALUE, FLAT_MOD, float(enchant_amount), apply);
                            break;
                        case ITEM_MOD_FERAL_ATTACK_POWER:
                        case ITEM_MOD_SPELL_HEALING_DONE:   // deprecated
                        case ITEM_MOD_SPELL_DAMAGE_DONE:    // deprecated
                        default:
                            break;
                    }
                    break;
                }
                case ITEM_ENCHANTMENT_TYPE_TOTEM:           // Shaman Rockbiter Weapon
                {
                    if (getClass() == CLASS_SHAMAN)
                    {
                        float addValue = 0.0f;
                        if (item->GetSlot() == EQUIPMENT_SLOT_MAINHAND)
                        {
                            addValue = float(enchant_amount * item->GetProto()->Delay / 1000.0f);
                            HandleStatModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_VALUE, addValue, apply);
                        }
                        else if (item->GetSlot() == EQUIPMENT_SLOT_OFFHAND)
                        {
                            addValue = float(enchant_amount * item->GetProto()->Delay / 1000.0f);
                            HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_VALUE, addValue, apply);
                        }
                    }
                    break;
                }
                case ITEM_ENCHANTMENT_TYPE_USE_SPELL:
                    // processed in Player::CastItemUseSpell
                    break;
                case ITEM_ENCHANTMENT_TYPE_PRISMATIC_SOCKET:
                    // nothing do..
                    break;
                default:
                    sLog.outError("Unknown item enchantment (id = %d) display type: %d", enchant_id, enchant_display_type);
                    break;
            }                                               /*switch(enchant_display_type)*/
        }                                                   /*for*/
    }

    // visualize enchantment at player and equipped items
    if (slot == PERM_ENCHANTMENT_SLOT)
    {
        SetUInt16Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + (item->GetSlot() * 2), 0, apply ? item->GetEnchantmentId(slot) : 0);
    }

    if (slot == TEMP_ENCHANTMENT_SLOT)
    {
        SetUInt16Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + (item->GetSlot() * 2), 1, apply ? item->GetEnchantmentId(slot) : 0);
    }

    if (apply_dur)
    {
        if (apply)
        {
            // set duration
            uint32 duration = item->GetEnchantmentDuration(slot);
            if (duration > 0)
            {
                AddEnchantmentDuration(item, slot, duration);
            }
        }
        else
        {
            // duration == 0 will remove EnchantDuration
            AddEnchantmentDuration(item, slot, 0);
        }
    }
}

/**
 * @brief Sends all active enchantment duration timers to the client.
 */
void Player::SendEnchantmentDurations()
{
    for (EnchantDurationList::const_iterator itr = m_enchantDuration.begin(); itr != m_enchantDuration.end(); ++itr)
    {
        GetSession()->SendItemEnchantTimeUpdate(GetObjectGuid(), itr->item->GetObjectGuid(), itr->slot, uint32(itr->leftduration) / 1000);
    }
}

/**
 * @brief Sends all tracked item duration timers to the client.
 */
void Player::SendItemDurations()
{
    for (ItemDurationList::const_iterator itr = m_itemDuration.begin(); itr != m_itemDuration.end(); ++itr)
    {
        (*itr)->SendTimeUpdate(this);
    }
}

/**
 * @brief Sends an item push notification for a newly received or created item.
 *
 * @param item The item instance being reported.
 * @param count The quantity gained.
 * @param received True if the item was received rather than looted.
 * @param created True if the item was created rather than simply received.
 * @param broadcast True to broadcast the message to the player's group.
 * @param showInChat True to show the gain in chat.
 */
void Player::SendNewItem(Item* item, uint32 count, bool received, bool created, bool broadcast /*=false*/, bool showInChat /*=true*/)
{
    if (!item)                                              // prevent crash
    {
        return;
    }

    // last check 2.0.10
    WorldPacket data(SMSG_ITEM_PUSH_RESULT, (8 + 4 + 4 + 4 + 1 + 4 + 4 + 4 + 4 + 4));
    data << GetObjectGuid();                                // player GUID
    data << uint32(received);                               // 0=looted, 1=from npc
    data << uint32(created);                                // 0=received, 1=created
    data << uint32(showInChat);                             // showInChat
    data << uint8(item->GetBagSlot());                      // bagslot
    // item slot, but when added to stack: 0xFFFFFFFF
    data << uint32((item->GetCount() == count) ? item->GetSlot() : -1);
    data << uint32(item->GetEntry());                       // item id
    data << uint32(item->GetItemSuffixFactor());            // SuffixFactor
    data << uint32(item->GetItemRandomPropertyId());        // random item property id
    data << uint32(count);                                  // count of items
    data << uint32(GetItemCount(item->GetEntry()));         // count of items in inventory

    if (broadcast && GetGroup())
    {
        GetGroup()->BroadcastPacket(&data, true);
    }
    else
    {
        GetSession()->SendPacket(&data);
    }
}

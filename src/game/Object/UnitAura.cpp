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
 * @file UnitAura.cpp
 * @brief Cohesion split of Unit.cpp -- aura modifier / holder add-remove management.
 *        Same `Unit` class; no behaviour change.
 */

#include "Unit.h"
#include "Log.h"
#include "Opcodes.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "World.h"
#include "ObjectMgr.h"
#include "ObjectGuid.h"
#include "SpellMgr.h"
#include "QuestDef.h"
#include "Player.h"
#include "Creature.h"
#include "Spell.h"
#include "Group.h"
#include "SpellAuras.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "CreatureAI.h"
#include "TemporarySummon.h"
#include "Formulas.h"
#include "Pet.h"
#include "Util.h"
#include "Totem.h"
#include "Vehicle.h"
#include "BattleGround/BattleGround.h"
#include "InstanceData.h"
#include "OutdoorPvP/OutdoorPvP.h"
#include "MapPersistentStateMgr.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "VMapFactory.h"
#include "MovementGenerator.h"
#include "movement/MoveSplineInit.h"
#include "movement/MoveSpline.h"
#include "CreatureLinkingMgr.h"
#include "GameTime.h"
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#include "ElunaConfig.h"
#include "ElunaEventMgr.h"
#endif /* ENABLE_ELUNA */
#include <math.h>
#include <stdarg.h>

/**
 * @brief Sums all aura modifiers of a given type.
 *
 * @param auratype The aura type to sum.
 * @return The total modifier amount.
 */
int32 Unit::GetTotalAuraModifier(AuraType auratype) const
{
    int32 modifier = 0;

    AuraList const& mTotalAuraList = GetAurasByType(auratype);
    for (AuraList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
    {
        modifier += (*i)->GetModifier()->m_amount;
    }

    return modifier;
}

/**
 * @brief Multiplies all percentage aura modifiers of a given type.
 *
 * @param auratype The aura type to evaluate.
 * @return The combined multiplier.
 */
float Unit::GetTotalAuraMultiplier(AuraType auratype) const
{
    float multiplier = 1.0f;

    AuraList const& mTotalAuraList = GetAurasByType(auratype);
    for (AuraList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
    {
        multiplier *= (100.0f + (*i)->GetModifier()->m_amount) / 100.0f;
    }

    return multiplier;
}

/**
 * @brief Gets the highest positive aura modifier of a given type.
 *
 * @param auratype The aura type to inspect.
 * @return The largest positive modifier.
 */
int32 Unit::GetMaxPositiveAuraModifier(AuraType auratype) const
{
    int32 modifier = 0;

    AuraList const& mTotalAuraList = GetAurasByType(auratype);
    for (AuraList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
    {
        if ((*i)->GetModifier()->m_amount > modifier)
        {
            modifier = (*i)->GetModifier()->m_amount;
        }
    }

    return modifier;
}

/**
 * @brief Gets the lowest negative aura modifier of a given type.
 *
 * @param auratype The aura type to inspect.
 * @return The most negative modifier.
 */
int32 Unit::GetMaxNegativeAuraModifier(AuraType auratype) const
{
    int32 modifier = 0;

    AuraList const& mTotalAuraList = GetAurasByType(auratype);
    for (AuraList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
    {
        if ((*i)->GetModifier()->m_amount < modifier)
        {
            modifier = (*i)->GetModifier()->m_amount;
        }
    }

    return modifier;
}

/**
 * @brief Sums aura modifiers of a type that match a misc-value mask.
 *
 * @param auratype The aura type to inspect.
 * @param misc_mask The misc-value bitmask to match.
 * @return The total modifier amount.
 */
int32 Unit::GetTotalAuraModifierByMiscMask(AuraType auratype, uint32 misc_mask) const
{
    if (!misc_mask)
    {
        return 0;
    }

    int32 modifier = 0;

    AuraList const& mTotalAuraList = GetAurasByType(auratype);
    for (AuraList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
    {
        Modifier* mod = (*i)->GetModifier();
        if (mod->m_miscvalue & misc_mask)
        {
            modifier += mod->m_amount;
        }
    }
    return modifier;
}

/**
 * @brief Multiplies aura modifiers of a type that match a misc-value mask.
 *
 * @param auratype The aura type to inspect.
 * @param misc_mask The misc-value bitmask to match.
 * @return The combined multiplier.
 */
float Unit::GetTotalAuraMultiplierByMiscMask(AuraType auratype, uint32 misc_mask) const
{
    if (!misc_mask)
    {
        return 1.0f;
    }

    float multiplier = 1.0f;

    AuraList const& mTotalAuraList = GetAurasByType(auratype);
    for (AuraList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
    {
        Modifier* mod = (*i)->GetModifier();
        if (mod->m_miscvalue & misc_mask)
        {
            multiplier *= (100.0f + mod->m_amount) / 100.0f;
        }
    }
    return multiplier;
}

/**
 * @brief Gets the highest positive aura modifier matching a misc-value mask.
 *
 * @param auratype The aura type to inspect.
 * @param misc_mask The misc-value bitmask to match.
 * @return The largest positive modifier.
 */
int32 Unit::GetMaxPositiveAuraModifierByMiscMask(AuraType auratype, uint32 misc_mask) const
{
    if (!misc_mask)
    {
        return 0;
    }

    int32 modifier = 0;

    AuraList const& mTotalAuraList = GetAurasByType(auratype);
    for (AuraList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
    {
        Modifier* mod = (*i)->GetModifier();
        if (mod->m_miscvalue & misc_mask && mod->m_amount > modifier)
        {
            modifier = mod->m_amount;
        }
    }

    return modifier;
}

/**
 * @brief Gets the lowest negative aura modifier matching a misc-value mask.
 *
 * @param auratype The aura type to inspect.
 * @param misc_mask The misc-value bitmask to match.
 * @return The most negative modifier.
 */
int32 Unit::GetMaxNegativeAuraModifierByMiscMask(AuraType auratype, uint32 misc_mask) const
{
    if (!misc_mask)
    {
        return 0;
    }

    int32 modifier = 0;

    AuraList const& mTotalAuraList = GetAurasByType(auratype);
    for (AuraList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
    {
        Modifier* mod = (*i)->GetModifier();
        if (mod->m_miscvalue & misc_mask && mod->m_amount < modifier)
        {
            modifier = mod->m_amount;
        }
    }

    return modifier;
}

/**
 * @brief Sums aura modifiers of a type that match an exact misc value.
 *
 * @param auratype The aura type to inspect.
 * @param misc_value The exact misc value to match.
 * @return The total modifier amount.
 */
int32 Unit::GetTotalAuraModifierByMiscValue(AuraType auratype, int32 misc_value) const
{
    int32 modifier = 0;

    AuraList const& mTotalAuraList = GetAurasByType(auratype);
    for (AuraList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
    {
        Modifier* mod = (*i)->GetModifier();
        if (mod->m_miscvalue == misc_value)
        {
            modifier += mod->m_amount;
        }
    }
    return modifier;
}

/**
 * @brief Multiplies aura modifiers of a type that match an exact misc value.
 *
 * @param auratype The aura type to inspect.
 * @param misc_value The exact misc value to match.
 * @return The combined multiplier.
 */
float Unit::GetTotalAuraMultiplierByMiscValue(AuraType auratype, int32 misc_value) const
{
    float multiplier = 1.0f;

    AuraList const& mTotalAuraList = GetAurasByType(auratype);
    for (AuraList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
    {
        Modifier* mod = (*i)->GetModifier();
        if (mod->m_miscvalue == misc_value)
        {
            multiplier *= (100.0f + mod->m_amount) / 100.0f;
        }
    }
    return multiplier;
}

/**
 * @brief Gets the highest positive aura modifier matching an exact misc value.
 *
 * @param auratype The aura type to inspect.
 * @param misc_value The exact misc value to match.
 * @return The largest positive modifier.
 */
int32 Unit::GetMaxPositiveAuraModifierByMiscValue(AuraType auratype, int32 misc_value) const
{
    int32 modifier = 0;

    AuraList const& mTotalAuraList = GetAurasByType(auratype);
    for (AuraList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
    {
        Modifier* mod = (*i)->GetModifier();
        if (mod->m_miscvalue == misc_value && mod->m_amount > modifier)
        {
            modifier = mod->m_amount;
        }
    }

    return modifier;
}

/**
 * @brief Gets the lowest negative aura modifier matching an exact misc value.
 *
 * @param auratype The aura type to inspect.
 * @param misc_value The exact misc value to match.
 * @return The most negative modifier.
 */
int32 Unit::GetMaxNegativeAuraModifierByMiscValue(AuraType auratype, int32 misc_value) const
{
    int32 modifier = 0;

    AuraList const& mTotalAuraList = GetAurasByType(auratype);
    for (AuraList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
    {
        Modifier* mod = (*i)->GetModifier();
        if (mod->m_miscvalue == misc_value && mod->m_amount < modifier)
        {
            modifier = mod->m_amount;
        }
    }

    return modifier;
}

float Unit::GetTotalAuraMultiplierByMiscValueForMask(AuraType auratype, uint32 mask) const
{
    if (!mask)
    {
        return 1.0f;
    }

    float multiplier = 1.0f;

    AuraList const& mTotalAuraList = GetAurasByType(auratype);
    for (AuraList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
    {
        Modifier* mod = (*i)->GetModifier();
        if (mask & (1 << (mod->m_miscvalue - 1)))
        {
            multiplier *= (100.0f + mod->m_amount) / 100.0f;
        }
    }
    return multiplier;
}

/**
 * @brief Adds an aura holder to the unit after stacking and conflict checks.
 *
 * @param holder The aura holder to add.
 * @return True if the holder remained applied; otherwise, false.
 */
bool Unit::AddSpellAuraHolder(SpellAuraHolder* holder)
{
    SpellEntry const* aurSpellInfo = holder->GetSpellProto();

    // ghost spell check, allow apply any auras at player loading in ghost mode (will be cleanup after load)
    if (!IsAlive() && !IsDeathPersistentSpell(aurSpellInfo) &&
        !IsDeathOnlySpell(aurSpellInfo) &&
        (GetTypeId() != TYPEID_PLAYER || !((Player*)this)->GetSession()->PlayerLoading()))
    {
        delete holder;
        return false;
    }

    if (holder->GetTarget() != this)
    {
        sLog.outError("Holder (spell %u) add to spell aura holder list of %s (lowguid: %u) but spell aura holder target is %s (lowguid: %u)",
                      holder->GetId(), (GetTypeId() == TYPEID_PLAYER ? "player" : "creature"), GetGUIDLow(),
                      (holder->GetTarget()->GetTypeId() == TYPEID_PLAYER ? "player" : "creature"), holder->GetTarget()->GetGUIDLow());
        delete holder;
        return false;
    }

    // passive and persistent auras can stack with themselves any number of times
    if ((!holder->IsPassive() && !holder->IsPersistent()) || holder->IsAreaAura())
    {
        SpellAuraHolderBounds spair = GetSpellAuraHolderBounds(aurSpellInfo->ID);

        // take out same spell
        for (SpellAuraHolderMap::iterator iter = spair.first; iter != spair.second; ++iter)
        {
            SpellAuraHolder* foundHolder = iter->second;
            if (foundHolder->GetCasterGuid() == holder->GetCasterGuid())
            {
                // Aura can stack on self -> Stack it;
                if (aurSpellInfo->CumulativeAura)
                {
                    // can be created with >1 stack by some spell mods
                    foundHolder->ModStackAmount(holder->GetStackAmount());
                    delete holder;
                    return false;
                }

                // Check for coexisting Weapon-proced Auras
                if (holder->IsWeaponBuffCoexistableWith(foundHolder))
                {
                    continue;
                }

                // Carry over removed Aura's remaining damage if Aura still has ticks remaining
                if (foundHolder->GetSpellProto()->HasAttribute(SPELL_ATTR_EX4_STACK_DOT_MODIFIER))
                {
                    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
                    {
                        if (Aura* aur = holder->GetAuraByEffectIndex(SpellEffectIndex(i)))
                        {
                            // m_auraname can be modified to SPELL_AURA_NONE for area auras, use original
                            AuraType aurNameReal = AuraType(aurSpellInfo->EffectAura[i]);

                            if (aurNameReal == SPELL_AURA_PERIODIC_DAMAGE && aur->GetAuraDuration() > 0)
                            {
                                if (Aura* existing = foundHolder->GetAuraByEffectIndex(SpellEffectIndex(i)))
                                {
                                    int32 remainingTicks = existing->GetAuraMaxTicks() - existing->GetAuraTicks();
                                    int32 remainingDamage = existing->GetModifier()->m_amount * remainingTicks;

                                    aur->GetModifier()->m_amount += int32(remainingDamage / aur->GetAuraMaxTicks());
                                }
                                else
                                {
                                    DEBUG_LOG("Holder (spell %u) on target (lowguid: %u) doesn't have aura on effect index %u. skipping.", aurSpellInfo->ID, holder->GetTarget()->GetGUIDLow(), i);
                                }
                            }
                        }
                    }
                }

                // can be only single
                RemoveSpellAuraHolder(foundHolder, AURA_REMOVE_BY_STACK);
                break;
            }

            bool stop = false;

            for (int32 i = 0; i < MAX_EFFECT_INDEX && !stop; ++i)
            {
                // no need to check non stacking auras that weren't/won't be applied on this target
                if (!foundHolder->m_auras[i] || !holder->m_auras[i])
                {
                    continue;
                }

                // m_auraname can be modified to SPELL_AURA_NONE for area auras, use original
                AuraType aurNameReal = AuraType(aurSpellInfo->EffectAura[i]);

                switch (aurNameReal)
                {
                        // DoT/HoT/etc
                    case SPELL_AURA_DUMMY:                  // allow stack (HoTs checked later)
                    case SPELL_AURA_PERIODIC_DAMAGE:
                    case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
                    case SPELL_AURA_PERIODIC_LEECH:
                    case SPELL_AURA_PERIODIC_HEAL:
                    case SPELL_AURA_OBS_MOD_HEALTH:
                    case SPELL_AURA_PERIODIC_MANA_LEECH:
                    case SPELL_AURA_OBS_MOD_MANA:
                    case SPELL_AURA_POWER_BURN_MANA:
                    case SPELL_AURA_CONTROL_VEHICLE:
                    case SPELL_AURA_TRIGGER_LINKED_AURA:
                    case SPELL_AURA_PERIODIC_DUMMY:
                        break;
                    case SPELL_AURA_PERIODIC_ENERGIZE:      // all or self or clear non-stackable
                    default:                                // not allow
                        // can be only single (this check done at _each_ aura add
                        RemoveSpellAuraHolder(foundHolder, AURA_REMOVE_BY_STACK);
                        stop = true;
                        break;
                }
            }

            if (stop)
            {
                break;
            }
        }
    }

    // normal spell or passive auras not stackable with other ranks
    if (!IsPassiveSpell(aurSpellInfo) || !IsPassiveSpellStackableWithRanks(aurSpellInfo))
    {
        // Hack exceptions for Vehicle/Linked auras
        if (!IsSpellHaveAura(aurSpellInfo, SPELL_AURA_CONTROL_VEHICLE) && !IsSpellHaveAura(aurSpellInfo, SPELL_AURA_TRIGGER_LINKED_AURA) &&
                !RemoveNoStackAurasDueToAuraHolder(holder))
        {
            delete holder;
            return false;                                   // couldn't remove conflicting aura with higher rank
        }
    }

    // update tracked aura targets list (before aura add to aura list, to prevent unexpected remove recently added aura)
    if (TrackedAuraType trackedType = holder->GetTrackedAuraType())
    {
        if (Unit* caster = holder->GetCaster())             // caster not in world
        {
            // Only compare TrackedAuras of same tracking type
            TrackedAuraTargetMap& scTargets = caster->GetTrackedAuraTargets(trackedType);
            for (TrackedAuraTargetMap::iterator itr = scTargets.begin(); itr != scTargets.end();)
            {
                SpellEntry const* itr_spellEntry = itr->first;
                ObjectGuid itr_targetGuid = itr->second;    // Target on whom the tracked aura is

                if (itr_targetGuid == GetObjectGuid())      // Note: I don't understand this check (based on old aura concepts, kept when adding holders)
                {
                    ++itr;
                    continue;
                }

                bool removed = false;
                switch (trackedType)
                {
                    case TRACK_AURA_TYPE_SINGLE_TARGET:
                        if (IsSingleTargetSpells(itr_spellEntry, aurSpellInfo))
                        {
                            removed = true;
                            // remove from target if target found
                            if (Unit* itr_target = GetMap()->GetUnit(itr_targetGuid))
                            {
                                itr_target->RemoveAurasDueToSpell(itr_spellEntry->ID);  // TODO AURA_REMOVE_BY_TRACKING (might require additional work elsewhere)
                            }
                            else                            // Normally the tracking will be removed by the AuraRemoval
                            {
                                scTargets.erase(itr);
                            }
                        }
                        break;
                    case TRACK_AURA_TYPE_CONTROL_VEHICLE:
                    {
                        // find minimal effect-index that applies an aura
                        uint8 i = EFFECT_INDEX_0;
                        for (; i < MAX_EFFECT_INDEX; ++i)
                        {
                            if (IsAuraApplyEffect(aurSpellInfo, SpellEffectIndex(i)))
                            {
                                break;
                            }
                        }

                        // Remove auras when first holder is applied
                        if ((1 << i) & holder->GetAuraFlags())
                        {
                            removed = true;                 // each caster can only control one vehicle

                            // remove from target if target found
                            if (Unit* itr_target = GetMap()->GetUnit(itr_targetGuid))
                            {
                                itr_target->RemoveAurasByCasterSpell(itr_spellEntry->ID, caster->GetObjectGuid());
                            }
                            else                            // Normally the tracking will be removed by the AuraRemoval
                            {
                                scTargets.erase(itr);
                            }
                        }
                        break;
                    }
                    case TRACK_AURA_TYPE_NOT_TRACKED:       // These two can never happen
                    case MAX_TRACKED_AURA_TYPES:
                        MANGOS_ASSERT(false);
                        break;
                }

                if (removed)
                {
                    itr = scTargets.begin();                // list can be chnaged at remove aura
                    continue;
                }

                ++itr;
            }

            switch (trackedType)
            {
                case TRACK_AURA_TYPE_CONTROL_VEHICLE:       // Only track the controlled vehicle, no secondary effects
                    if (!IsSpellHaveAura(aurSpellInfo, SPELL_AURA_CONTROL_VEHICLE, holder->GetAuraFlags()))
                    {
                        break;
                    }
                    // no break here, track other controlled
                case TRACK_AURA_TYPE_SINGLE_TARGET:         // Register spell holder single target
                    scTargets[aurSpellInfo] = GetObjectGuid();
                    break;
            }
        }
    }

    // add aura, register in lists and arrays
    holder->_AddSpellAuraHolder();
    m_spellAuraHolders.insert(SpellAuraHolderMap::value_type(holder->GetId(), holder));

    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        if (Aura* aur = holder->GetAuraByEffectIndex(SpellEffectIndex(i)))
        {
            AddAuraToModList(aur);
        }
    }

    holder->ApplyAuraModifiers(true, true);                 // This is the place where auras are actually applied onto the target
    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "Holder of spell %u now is in use", holder->GetId());

    // if aura deleted before boosts apply ignore
    // this can be possible it it removed indirectly by triggered spell effect at ApplyModifier
    if (holder->IsDeleted())
    {
        return false;
    }

    holder->HandleSpellSpecificBoosts(true);

    return true;
}

/**
 * @brief Registers an aura in the unit's modifier lookup list.
 *
 * @param aura The aura to register.
 */
void Unit::AddAuraToModList(Aura* aura)
{
    if (aura->GetModifier()->m_auraname < TOTAL_AURAS)
    {
        m_modAuras[aura->GetModifier()->m_auraname].push_back(aura);
    }
}

/**
 * @brief Removes aura ranks that conflict with a spell being applied.
 *
 * @param spellId The spell identifier whose rank chain is checked.
 */
void Unit::RemoveRankAurasDueToSpell(uint32 spellId)
{
    SpellEntry const* spellInfo = sSpellStore.LookupEntry(spellId);
    if (!spellInfo)
    {
        return;
    }
    SpellAuraHolderMap::const_iterator i, next;
    for (i = m_spellAuraHolders.begin(); i != m_spellAuraHolders.end(); i = next)
    {
        next = i;
        ++next;
        uint32 i_spellId = (*i).second->GetId();
        if ((*i).second && i_spellId && i_spellId != spellId)
        {
            if (sSpellMgr.IsRankSpellDueToSpell(spellInfo, i_spellId))
            {
                RemoveAurasDueToSpell(i_spellId);

                if (m_spellAuraHolders.empty())
                {
                    break;
                }
                else
                {
                    next =  m_spellAuraHolders.begin();
                }
            }
        }
    }
}

/**
 * @brief Removes non-stacking auras that conflict with an incoming aura holder.
 *
 * @param holder The incoming aura holder.
 * @return True if conflicts were resolved and the holder may proceed; otherwise, false.
 */
bool Unit::RemoveNoStackAurasDueToAuraHolder(SpellAuraHolder* holder)
{
    if (!holder)
    {
        return false;
    }

    SpellEntry const* spellProto = holder->GetSpellProto();
    if (!spellProto)
    {
        return false;
    }

    uint32 spellId = holder->GetId();

    // passive spell special case (only non stackable with ranks)
    if (IsPassiveSpell(spellProto))
    {
        if (IsPassiveSpellStackableWithRanks(spellProto))
        {
            return true;
        }
    }

    SpellSpecific spellId_spec = GetSpellSpecific(spellId);

    SpellAuraHolderMap::iterator i, next;
    for (i = m_spellAuraHolders.begin(); i != m_spellAuraHolders.end(); i = next)
    {
        next = i;
        ++next;
        if (!(*i).second)
        {
            continue;
        }

        SpellEntry const* i_spellProto = (*i).second->GetSpellProto();

        if (!i_spellProto)
        {
            continue;
        }

        uint32 i_spellId = i_spellProto->ID;

        // early checks that spellId is passive non stackable spell
        if (IsPassiveSpell(i_spellProto))
        {
            // passive non-stackable spells not stackable only for same caster
            if (holder->GetCasterGuid() != i->second->GetCasterGuid())
            {
                continue;
            }

            // passive non-stackable spells not stackable only with another rank of same spell
            if (!sSpellMgr.IsRankSpellDueToSpell(spellProto, i_spellId))
            {
                continue;
            }
        }

        if (i_spellId == spellId)
        {
            continue;
        }

        bool is_triggered_by_spell = false;
        // prevent triggering aura of removing aura that triggered it
        for (int j = 0; j < MAX_EFFECT_INDEX; ++j)
        {
            if (i_spellProto->EffectTriggerSpell[j] == spellId)
            {
                is_triggered_by_spell = true;
            }
        }

        // prevent triggered aura of removing aura that triggering it (triggered effect early some aura of parent spell
        for (int j = 0; j < MAX_EFFECT_INDEX; ++j)
        {
            if (spellProto->EffectTriggerSpell[j] == i_spellId)
            {
                is_triggered_by_spell = true;
            }
        }

        if (is_triggered_by_spell)
        {
            continue;
        }

        SpellSpecific i_spellId_spec = GetSpellSpecific(i_spellId);

        // single allowed spell specific from same caster or from any caster at target
        bool is_spellSpecPerTargetPerCaster = IsSingleFromSpellSpecificPerTargetPerCaster(spellId_spec, i_spellId_spec);

        bool is_spellSpecPerTarget = IsSingleFromSpellSpecificPerTarget(spellId_spec, i_spellId_spec);
        if (is_spellSpecPerTarget || (is_spellSpecPerTargetPerCaster && holder->GetCasterGuid() == (*i).second->GetCasterGuid()))
        {
            // can not remove higher rank
            if (sSpellMgr.IsRankSpellDueToSpell(spellProto, i_spellId))
                if (CompareAuraRanks(spellId, i_spellId) < 0)
                {
                    return false;
                }

            // Its a parent aura (create this aura in ApplyModifier)
            if ((*i).second->IsInUse())
            {
                sLog.outError("SpellAuraHolder (Spell %u) is in process but attempt removed at SpellAuraHolder (Spell %u) adding, need add stack rule for Unit::RemoveNoStackAurasDueToAuraHolder", i->second->GetId(), holder->GetId());
                continue;
            }
            RemoveAurasDueToSpell(i_spellId);

            if (m_spellAuraHolders.empty())
            {
                break;
            }
            else
            {
                next =  m_spellAuraHolders.begin();
            }

            continue;
        }

        // spell with spell specific that allow single ranks for spell from diff caster
        // same caster case processed or early or later
        bool is_spellPerTarget = IsSingleFromSpellSpecificSpellRanksPerTarget(spellId_spec, i_spellId_spec);
        if (is_spellPerTarget && holder->GetCasterGuid() != (*i).second->GetCasterGuid() && sSpellMgr.IsRankSpellDueToSpell(spellProto, i_spellId))
        {
            // can not remove higher rank
            if (CompareAuraRanks(spellId, i_spellId) < 0)
            {
                return false;
            }

            // Its a parent aura (create this aura in ApplyModifier)
            if ((*i).second->IsInUse())
            {
                sLog.outError("SpellAuraHolder (Spell %u) is in process but attempt removed at SpellAuraHolder (Spell %u) adding, need add stack rule for Unit::RemoveNoStackAurasDueToAuraHolder", i->second->GetId(), holder->GetId());
                continue;
            }
            RemoveAurasDueToSpell(i_spellId);

            if (m_spellAuraHolders.empty())
            {
                break;
            }
            else
            {
                next =  m_spellAuraHolders.begin();
            }

            continue;
        }

        // non single (per caster) per target spell specific (possible single spell per target at caster)
        if (!is_spellSpecPerTargetPerCaster && !is_spellSpecPerTarget && sSpellMgr.IsNoStackSpellDueToSpell(spellId, i_spellId))
        {
            // Its a parent aura (create this aura in ApplyModifier)
            if ((*i).second->IsInUse())
            {
                sLog.outError("SpellAuraHolder (Spell %u) is in process but attempt removed at SpellAuraHolder (Spell %u) adding, need add stack rule for Unit::RemoveNoStackAurasDueToAuraHolder", i->second->GetId(), holder->GetId());
                continue;
            }
            RemoveAurasDueToSpell(i_spellId);

            if (m_spellAuraHolders.empty())
            {
                break;
            }
            else
            {
                next =  m_spellAuraHolders.begin();
            }

            continue;
        }

        // Potions stack aura by aura (elixirs/flask already checked)
        if (spellProto->SpellClassSet == SPELLFAMILY_POTION && i_spellProto->SpellClassSet == SPELLFAMILY_POTION)
        {
            if (IsNoStackAuraDueToAura(spellId, i_spellId))
            {
                if (CompareAuraRanks(spellId, i_spellId) < 0)
                {
                    return false;                        // can not remove higher rank
                }

                // Its a parent aura (create this aura in ApplyModifier)
                if ((*i).second->IsInUse())
                {
                    sLog.outError("SpellAuraHolder (Spell %u) is in process but attempt removed at SpellAuraHolder (Spell %u) adding, need add stack rule for Unit::RemoveNoStackAurasDueToAuraHolder", i->second->GetId(), holder->GetId());
                    continue;
                }
                RemoveAurasDueToSpell(i_spellId);

                if (m_spellAuraHolders.empty())
                {
                    break;
                }
                else
                {
                    next =  m_spellAuraHolders.begin();
                }
            }
        }
    }
    return true;
}

/**
 * @brief Removes one aura effect index from all holders of a spell except an optional aura.
 *
 * @param spellId The spell identifier.
 * @param effindex The effect index to remove.
 * @param except An aura instance to keep.
 */
void Unit::RemoveAura(uint32 spellId, SpellEffectIndex effindex, Aura* except)
{
    SpellAuraHolderBounds spair = GetSpellAuraHolderBounds(spellId);
    for (SpellAuraHolderMap::iterator iter = spair.first; iter != spair.second;)
    {
        Aura* aur = iter->second->m_auras[effindex];
        if (aur && aur != except)
        {
            RemoveSingleAuraFromSpellAuraHolder(iter->second, effindex);
            // may remove holder
            spair = GetSpellAuraHolderBounds(spellId);
            iter = spair.first;
        }
        else
        {
            ++iter;
        }
    }
}

/**
 * @brief Removes a specific spell's auras from a specific caster.
 *
 * @param spellId The spell identifier.
 * @param casterGuid The caster GUID to match.
 */
void Unit::RemoveAurasByCasterSpell(uint32 spellId, ObjectGuid casterGuid)
{
    SpellAuraHolderBounds spair = GetSpellAuraHolderBounds(spellId);
    for (SpellAuraHolderMap::iterator iter = spair.first; iter != spair.second;)
    {
        if (iter->second->GetCasterGuid() == casterGuid)
        {
            RemoveSpellAuraHolder(iter->second);
            spair = GetSpellAuraHolderBounds(spellId);
            iter = spair.first;
        }
        else
        {
            ++iter;
        }
    }
}

/**
 * @brief Removes one aura effect index from holders matching a spell and caster.
 *
 * @param spellId The spell identifier.
 * @param effindex The effect index to remove.
 * @param casterGuid The caster GUID to match.
 * @param mode The aura removal mode.
 */
void Unit::RemoveSingleAuraFromSpellAuraHolder(uint32 spellId, SpellEffectIndex effindex, ObjectGuid casterGuid, AuraRemoveMode mode)
{
    SpellAuraHolderBounds spair = GetSpellAuraHolderBounds(spellId);
    for (SpellAuraHolderMap::iterator iter = spair.first; iter != spair.second;)
    {
        Aura* aur = iter->second->m_auras[effindex];
        if (aur && aur->GetCasterGuid() == casterGuid)
        {
            RemoveSingleAuraFromSpellAuraHolder(iter->second, effindex, mode);
            spair = GetSpellAuraHolderBounds(spellId);
            iter = spair.first;
        }
        else
        {
            ++iter;
        }
    }
}

/**
 * @brief Removes aura stacks from a dispelled spell.
 *
 * @param spellId The dispelled spell identifier.
 * @param stackAmount The number of stacks to remove.
 * @param casterGuid The caster GUID to match.
 * @param dispeller Unused dispelling unit placeholder.
 */
void Unit::RemoveAuraHolderDueToSpellByDispel(uint32 spellId, uint32 stackAmount, ObjectGuid casterGuid, Unit* dispeller)
{
    SpellEntry const* spellEntry = sSpellStore.LookupEntry(spellId);

    // Custom dispel case
    // Unstable Affliction
    if (spellEntry->SpellClassSet == SPELLFAMILY_WARLOCK && (spellEntry->SpellClassMask & UI64LIT(0x010000000000)))
    {
        if (Aura* dotAura = GetAura(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_WARLOCK, UI64LIT(0x010000000000), 0x00000000, casterGuid))
        {
            // use clean value for initial damage
            int32 damage = dotAura->GetSpellProto()->CalculateSimpleValue(EFFECT_INDEX_0);
            damage *= 9;

            // Remove spell auras from stack
            RemoveAuraHolderFromStack(spellId, stackAmount, casterGuid, AURA_REMOVE_BY_DISPEL);

            // backfire damage and silence
            dispeller->CastCustomSpell(dispeller, 31117, &damage, NULL, NULL, true, NULL, NULL, casterGuid);
            return;
        }
    }
    // Lifebloom
    else if (spellEntry->SpellClassSet == SPELLFAMILY_DRUID && (spellEntry->SpellClassMask & UI64LIT(0x0000001000000000)))
    {
        if (Aura* dotAura = GetAura(SPELL_AURA_DUMMY, SPELLFAMILY_DRUID, UI64LIT(0x0000001000000000), 0x00000000, casterGuid))
        {
            int32 amount = (dotAura->GetModifier()->m_amount / dotAura->GetStackAmount()) * stackAmount;
            CastCustomSpell(this, 33778, &amount, NULL, NULL, true, NULL, dotAura, casterGuid);

            if (Unit* caster = dotAura->GetCaster())
            {
                int32 returnmana = (spellEntry->ManaCostPct * caster->GetCreateMana() / 100) * stackAmount / 2;
                caster->CastCustomSpell(caster, 64372, &returnmana, NULL, NULL, true, NULL, dotAura, casterGuid);
            }
        }
    }
    // Flame Shock
    else if (spellEntry->SpellClassSet == SPELLFAMILY_SHAMAN && (spellEntry->SpellClassMask & UI64LIT(0x10000000)))
    {
        Unit* caster = NULL;
        uint32 triggeredSpell = 0;

        if (Aura* dotAura = GetAura(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_SHAMAN, UI64LIT(0x10000000), 0x00000000, casterGuid))
        {
            caster = dotAura->GetCaster();
        }

        if (caster && !caster->IsDead())
        {
            Unit::AuraList const& auras = caster->GetAurasByType(SPELL_AURA_DUMMY);
            for (Unit::AuraList::const_iterator i = auras.begin(); i != auras.end(); ++i)
            {
                switch ((*i)->GetId())
                {
                    case 51480: triggeredSpell = 64694; break; // Lava Flows, Rank 1
                    case 51481: triggeredSpell = 65263; break; // Lava Flows, Rank 2
                    case 51482: triggeredSpell = 65264; break; // Lava Flows, Rank 3
                    default: continue;
                }
                break;
            }
        }

        // Remove spell auras from stack
        RemoveAuraHolderFromStack(spellId, stackAmount, casterGuid, AURA_REMOVE_BY_DISPEL);

        // Haste
        if (triggeredSpell)
        {
            caster->CastSpell(caster, triggeredSpell, true);
        }
        return;
    }
    // Vampiric touch (first dummy aura)
    else if (spellEntry->SpellClassSet == SPELLFAMILY_PRIEST && spellEntry->SpellClassMask & UI64LIT(0x0000040000000000))
    {
        if (Aura* dot = GetAura(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_PRIEST, UI64LIT(0x0000040000000000), 0x00000000, casterGuid))
        {
            if (dot->GetCaster())
            {
                // use clean value for initial damage
                int32 bp0 = dot->GetSpellProto()->CalculateSimpleValue(EFFECT_INDEX_1);
                bp0 *= 8;

                // Remove spell auras from stack
                RemoveAuraHolderFromStack(spellId, stackAmount, casterGuid, AURA_REMOVE_BY_DISPEL);

                CastCustomSpell(this, 64085, &bp0, NULL, NULL, true, NULL, NULL, casterGuid);
                return;
            }
        }
    }

    RemoveAuraHolderFromStack(spellId, stackAmount, casterGuid, AURA_REMOVE_BY_DISPEL);
}

void Unit::RemoveAurasDueToSpellBySteal(uint32 spellId, ObjectGuid casterGuid, Unit* stealer)
{
    SpellAuraHolder* holder = GetSpellAuraHolder(spellId, casterGuid);
    SpellEntry const* spellProto = sSpellStore.LookupEntry(spellId);
    SpellAuraHolder* new_holder = CreateSpellAuraHolder(spellProto, stealer, this);

    // set its duration and maximum duration
    // max duration 2 minutes (in msecs)
    int32 dur = holder->GetAuraDuration();
    int32 max_dur = 2 * MINUTE * IN_MILLISECONDS;
    int32 new_max_dur = max_dur > dur ? dur : max_dur;
    new_holder->SetAuraMaxDuration(new_max_dur);
    new_holder->SetAuraDuration(new_max_dur);

    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        Aura* aur = holder->GetAuraByEffectIndex(SpellEffectIndex(i));

        if (!aur)
        {
            continue;
        }

        int32 basePoints = aur->GetBasePoints();
        // construct the new aura for the attacker - will never return NULL, it's just a wrapper for
        // some different constructors
        Aura* new_aur = CreateAura(spellProto, aur->GetEffIndex(), &basePoints, new_holder, stealer, this);

        // set periodic to do at least one tick (for case when original aura has been at last tick preparing)
        int32 periodic = aur->GetModifier()->periodictime;
        new_aur->GetModifier()->periodictime = periodic < new_max_dur ? periodic : new_max_dur;

        // add the new aura to stealer
        new_holder->AddAura(new_aur, new_aur->GetEffIndex());
    }

    if (holder->ModStackAmount(-1))
        // Remove aura as dispel
        RemoveSpellAuraHolder(holder, AURA_REMOVE_BY_DISPEL);

    // strange but intended behaviour: Stolen single target auras won't be treated as single targeted
    new_holder->SetTrackedAuraType(TRACK_AURA_TYPE_NOT_TRACKED);

    stealer->AddSpellAuraHolder(new_holder);
}

/**
 * @brief Cancels all aura holders for a specific spell.
 *
 * @param spellId The spell identifier.
 */
void Unit::RemoveAurasDueToSpellByCancel(uint32 spellId)
{
    SpellAuraHolderBounds spair = GetSpellAuraHolderBounds(spellId);
    for (SpellAuraHolderMap::iterator iter = spair.first; iter != spair.second;)
    {
        RemoveSpellAuraHolder(iter->second, AURA_REMOVE_BY_CANCEL);
        spair = GetSpellAuraHolderBounds(spellId);
        iter = spair.first;
    }
}

/**
 * @brief Removes all auras matching a dispel type and optional caster.
 *
 * @param type The dispel type to match.
 * @param casterGuid An optional caster GUID filter.
 */
void Unit::RemoveAurasWithDispelType(DispelType type, ObjectGuid casterGuid)
{
    // Create dispel mask by dispel type
    uint32 dispelMask = GetDispellMask(type);
    // Dispel all existing auras vs current dispel type
    SpellAuraHolderMap& auras = GetSpellAuraHolderMap();
    for (SpellAuraHolderMap::iterator itr = auras.begin(); itr != auras.end();)
    {
        SpellEntry const* spell = itr->second->GetSpellProto();
        if (((1 << spell->DispelType) & dispelMask) && (!casterGuid || casterGuid == itr->second->GetCasterGuid()))
        {
            // Dispel aura
            RemoveAurasDueToSpell(spell->ID);
            itr = auras.begin();
        }
        else
        {
            ++itr;
        }
    }
}

/**
 * @brief Reduces stacks on a spell aura holder and removes it if emptied.
 *
 * @param spellId The spell identifier.
 * @param stackAmount The number of stacks to remove.
 * @param casterGuid An optional caster GUID filter.
 * @param mode The aura removal mode if the holder is removed.
 */
void Unit::RemoveAuraHolderFromStack(uint32 spellId, uint32 stackAmount, ObjectGuid casterGuid, AuraRemoveMode mode)
{
    SpellAuraHolderBounds spair = GetSpellAuraHolderBounds(spellId);
    for (SpellAuraHolderMap::iterator iter = spair.first; iter != spair.second; ++iter)
    {
        if (!casterGuid || iter->second->GetCasterGuid() == casterGuid)
        {
            if (iter->second->ModStackAmount(-int32(stackAmount)))
            {
                RemoveSpellAuraHolder(iter->second, mode);
                break;
            }
        }
    }
}

/**
 * @brief Removes all aura holders for a spell except an optional holder.
 *
 * @param spellId The spell identifier.
 * @param except A holder to keep.
 * @param mode The aura removal mode.
 */
void Unit::RemoveAurasDueToSpell(uint32 spellId, SpellAuraHolder* except, AuraRemoveMode mode)
{
    SpellEntry const* spellEntry = sSpellStore.LookupEntry(spellId);
    if (spellEntry && spellEntry->Difficulty && IsInWorld() && GetMap()->IsDungeon())
        if (SpellEntry const* spellDiffEntry = GetSpellEntryByDifficulty(spellEntry->Difficulty, GetMap()->GetDifficulty(), GetMap()->IsRaid()))
        {
            spellId = spellDiffEntry->ID;
        }

    SpellAuraHolderBounds bounds = GetSpellAuraHolderBounds(spellId);
    for (SpellAuraHolderMap::iterator iter = bounds.first; iter != bounds.second;)
    {
        if (iter->second != except)
        {
            RemoveSpellAuraHolder(iter->second, mode);
            bounds = GetSpellAuraHolderBounds(spellId);
            iter = bounds.first;
        }
        else
        {
            ++iter;
        }
    }
}

/**
 * @brief Removes aura holders created by a specific item spell cast.
 *
 * @param castItem The casting item.
 * @param spellId The spell identifier.
 */
void Unit::RemoveAurasDueToItemSpell(Item* castItem, uint32 spellId)
{
    SpellAuraHolderBounds bounds = GetSpellAuraHolderBounds(spellId);
    for (SpellAuraHolderMap::iterator iter = bounds.first; iter != bounds.second;)
    {
        if (iter->second->GetCastItemGuid() == castItem->GetObjectGuid())
        {
            RemoveSpellAuraHolder(iter->second);
            bounds = GetSpellAuraHolderBounds(spellId);
            iter = bounds.first;
        }
        else
        {
            ++iter;
        }
    }
}

/**
 * @brief Removes all aura holders whose interrupt flags match a mask.
 *
 * @param flags The interrupt flag mask.
 */
void Unit::RemoveAurasWithInterruptFlags(uint32 flags)
{
    for (SpellAuraHolderMap::iterator iter = m_spellAuraHolders.begin(); iter != m_spellAuraHolders.end();)
    {
        if (iter->second->GetSpellProto()->AuraInterruptFlags & flags)
        {
            RemoveSpellAuraHolder(iter->second);
            iter = m_spellAuraHolders.begin();
        }
        else
        {
            ++iter;
        }
    }
}

/**
 * @brief Removes all aura holders whose spells have a specific attribute mask.
 *
 * @param flags The spell attribute mask.
 */
void Unit::RemoveAurasWithAttribute(uint32 flags)
{
    for (SpellAuraHolderMap::iterator iter = m_spellAuraHolders.begin(); iter != m_spellAuraHolders.end();)
    {
        if (iter->second->GetSpellProto()->HasAttribute((SpellAttributes)flags))
        {
            RemoveSpellAuraHolder(iter->second);
            iter = m_spellAuraHolders.begin();
        }
        else
        {
            ++iter;
        }
    }
}

/**
 * @brief Clears tracked target auras that no longer belong to this unit.
 */
void Unit::RemoveNotOwnTrackedTargetAuras(uint32 newPhase)
{
    // tracked aura targets from other casters are removed if the phase does no more fit
    for (SpellAuraHolderMap::iterator iter = m_spellAuraHolders.begin(); iter != m_spellAuraHolders.end();)
    {
        TrackedAuraType trackedType = iter->second->GetTrackedAuraType();
        if (!trackedType)
        {
            ++iter;
            continue;
        }

        if (trackedType == TRACK_AURA_TYPE_CONTROL_VEHICLE || iter->second->GetCasterGuid() != GetObjectGuid())
        {
            if (!newPhase)
            {
                RemoveSpellAuraHolder(iter->second);
                iter = m_spellAuraHolders.begin();
                continue;
            }
            else
            {
                Unit* caster = iter->second->GetCaster();
                if (!caster || !caster->InSamePhase(newPhase))
                {
                    RemoveSpellAuraHolder(iter->second);
                    iter = m_spellAuraHolders.begin();
                    continue;
                }
            }
        }

        ++iter;
    }

    // tracked aura targets at other targets
    for (uint8 type = TRACK_AURA_TYPE_SINGLE_TARGET; type < MAX_TRACKED_AURA_TYPES; ++type)
    {
        TrackedAuraTargetMap& scTargets = GetTrackedAuraTargets(TrackedAuraType(type));
        for (TrackedAuraTargetMap::iterator itr = scTargets.begin(); itr != scTargets.end();)
        {
            SpellEntry const* itr_spellEntry = itr->first;
            ObjectGuid itr_targetGuid = itr->second;

            if (itr_targetGuid != GetObjectGuid())
            {
                if (!newPhase)
                {
                    scTargets.erase(itr);                   // remove for caster in any case

                    // remove from target if target found
                    if (Unit* itr_target = GetMap()->GetUnit(itr_targetGuid))
                    {
                        itr_target->RemoveAurasByCasterSpell(itr_spellEntry->ID, GetObjectGuid());
                    }

                    itr = scTargets.begin();                // list can be changed at remove aura
                    continue;
                }
                else
                {
                    Unit* itr_target = GetMap()->GetUnit(itr_targetGuid);
                    if (!itr_target || !itr_target->InSamePhase(newPhase))
                    {
                        scTargets.erase(itr);               // remove for caster in any case

                        // remove from target if target found
                        if (itr_target)
                        {
                            itr_target->RemoveAurasByCasterSpell(itr_spellEntry->ID, GetObjectGuid());
                        }

                        itr = scTargets.begin();            // list can be changed at remove aura
                        continue;
                    }
                }
            }

            ++itr;
        }
    }
}

/**
 * @brief Removes an aura holder from the unit and cleans up its effects.
 *
 * @param holder The aura holder to remove.
 * @param mode The aura removal mode.
 */
void Unit::RemoveSpellAuraHolder(SpellAuraHolder* holder, AuraRemoveMode mode)
{
    // Statue unsummoned at holder remove
    SpellEntry const* AurSpellInfo = holder->GetSpellProto();
    Totem* statue = NULL;
    Unit* caster = holder->GetCaster();
    if (IsChanneledSpell(AurSpellInfo) && caster)
        if (caster->GetTypeId() == TYPEID_UNIT && ((Creature*)caster)->IsTotem() && ((Totem*)caster)->GetTotemType() == TOTEM_STATUE)
        {
            statue = ((Totem*)caster);
        }

    if (m_spellAuraHoldersUpdateIterator != m_spellAuraHolders.end() && m_spellAuraHoldersUpdateIterator->second == holder)
    {
        ++m_spellAuraHoldersUpdateIterator;
    }

    SpellAuraHolderBounds bounds = GetSpellAuraHolderBounds(holder->GetId());
    for (SpellAuraHolderMap::iterator itr = bounds.first; itr != bounds.second; ++itr)
    {
        if (itr->second == holder)
        {
            m_spellAuraHolders.erase(itr);
            break;
        }
    }

    holder->SetRemoveMode(mode);
    holder->UnregisterAndCleanupTrackedAuras();

    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        if (Aura* aura = holder->m_auras[i])
        {
            RemoveAura(aura, mode);
        }
    }

    holder->_RemoveSpellAuraHolder();

    if (mode != AURA_REMOVE_BY_DELETE)
    {
        holder->HandleSpellSpecificBoosts(false);
    }

    if (statue)
    {
        statue->UnSummon();
    }

    // If holder in use (removed from code that plan access to it data after return)
    // store it in holder list with delayed deletion
    if (holder->IsInUse())
    {
        holder->SetDeleted();
        m_deletedHolders.push_back(holder);
    }
    else
    {
        delete holder;
    }

    if (mode != AURA_REMOVE_BY_EXPIRE && IsChanneledSpell(AurSpellInfo) && !IsAreaOfEffectSpell(AurSpellInfo) &&
        caster && caster->GetObjectGuid() != GetObjectGuid())
    {
        caster->InterruptSpell(CURRENT_CHANNELED_SPELL);
    }
}

/**
 * @brief Removes a single aura effect from a holder.
 *
 * @param holder The aura holder containing the effect.
 * @param index The effect index to remove.
 * @param mode The aura removal mode.
 */
void Unit::RemoveSingleAuraFromSpellAuraHolder(SpellAuraHolder* holder, SpellEffectIndex index, AuraRemoveMode mode)
{
    Aura* aura = holder->GetAuraByEffectIndex(index);
    if (!aura)
    {
        return;
    }

    if (aura->IsLastAuraOnHolder())
    {
        RemoveSpellAuraHolder(holder, mode);
    }
    else
    {
        RemoveAura(aura, mode);
    }
}

/**
 * @brief Removes an aura instance and unapplies its modifiers.
 *
 * @param Aur The aura to remove.
 * @param mode The aura removal mode.
 */
void Unit::RemoveAura(Aura* Aur, AuraRemoveMode mode)
{
    // remove from list before mods removing (prevent cyclic calls, mods added before including to aura list - use reverse order)
    if (Aur->GetModifier()->m_auraname < TOTAL_AURAS)
    {
        m_modAuras[Aur->GetModifier()->m_auraname].remove(Aur);
    }

    // Set remove mode
    Aur->SetRemoveMode(mode);

    // some ShapeshiftBoosts at remove trigger removing other auras including parent Shapeshift aura
    // remove aura from list before to prevent deleting it before
    /// m_Auras.erase(i);

    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "Aura %u now is remove mode %d", Aur->GetModifier()->m_auraname, mode);

    // aura _MUST_ be remove from holder before unapply.
    // un-apply code expected that aura not find by diff searches
    // in another case it can be double removed for example, if target die/etc in un-apply process.
    Aur->GetHolder()->RemoveAura(Aur->GetEffIndex());

    // some auras also need to apply modifier (on caster) on remove
    if (mode == AURA_REMOVE_BY_DELETE)
    {
        switch (Aur->GetModifier()->m_auraname)
        {
                // need properly undo any auras with player-caster mover set (or will crash at next caster move packet)
            case SPELL_AURA_MOD_POSSESS:
            case SPELL_AURA_MOD_POSSESS_PET:
            case SPELL_AURA_CONTROL_VEHICLE:
                Aur->ApplyModifier(false, true);
                break;
            default: break;
        }
    }
    else
    {
        Aur->ApplyModifier(false, true);
    }

    // If aura in use (removed from code that plan access to it data after return)
    // store it in aura list with delayed deletion
    if (Aur->IsInUse())
    {
        m_deletedAuras.push_back(Aur);
    }
    else
    {
        delete Aur;
    }
}

/**
 * @brief Removes all aura holders from the unit.
 *
 * @param mode The aura removal mode.
 */
void Unit::RemoveAllAuras(AuraRemoveMode mode /*= AURA_REMOVE_BY_DEFAULT*/)
{
    while (!m_spellAuraHolders.empty())
    {
        SpellAuraHolderMap::iterator iter = m_spellAuraHolders.begin();
        RemoveSpellAuraHolder(iter->second, mode);
    }
}

void Unit::RemoveArenaAuras(bool onleave)
{
    // in join, remove positive buffs, on end, remove negative
    // used to remove positive visible auras in arenas
    for (SpellAuraHolderMap::iterator iter = m_spellAuraHolders.begin(); iter != m_spellAuraHolders.end();)
    {
        if (!iter->second->GetSpellProto()->HasAttribute(SPELL_ATTR_EX4_UNK21) &&
                // don't remove stances, shadowform, pally/hunter auras
                !iter->second->IsPassive() &&               // don't remove passive auras
                (!iter->second->GetSpellProto()->HasAttribute(SPELL_ATTR_UNAFFECTED_BY_INVULNERABILITY) ||
                 !iter->second->GetSpellProto()->HasAttribute(SPELL_ATTR_HIDE_IN_COMBAT_LOG)) &&
                // not unaffected by invulnerability auras or not having that unknown flag (that seemed the most probable)
                (iter->second->IsPositive() != onleave))    // remove positive buffs on enter, negative buffs on leave
        {
            RemoveSpellAuraHolder(iter->second);
            iter = m_spellAuraHolders.begin();
        }
        else
        {
            ++iter;
        }
    }
}

/**
 * @brief Removes all non-persistent auras when the unit dies.
 */
void Unit::RemoveAllAurasOnDeath()
{
    // used just after dieing to remove all visible auras
    // and disable the mods for the passive ones
    for (SpellAuraHolderMap::iterator iter = m_spellAuraHolders.begin(); iter != m_spellAuraHolders.end();)
    {
        if (!iter->second->IsPassive() && !iter->second->IsDeathPersistent())
        {
            RemoveSpellAuraHolder(iter->second, AURA_REMOVE_BY_DEATH);
            iter = m_spellAuraHolders.begin();
        }
        else
        {
            ++iter;
        }
    }
}

/**
 * @brief Removes removable auras when the unit enters evade mode.
 */
void Unit::RemoveAllAurasOnEvade()
{
    // used when evading to remove all auras except some special auras
    // Vehicle control auras / Fly should not be removed on evade - neither should linked auras
    for (SpellAuraHolderMap::iterator iter = m_spellAuraHolders.begin(); iter != m_spellAuraHolders.end();)
    {
        SpellEntry const* proto = iter->second->GetSpellProto();
        if (!IsSpellHaveAura(proto, SPELL_AURA_CONTROL_VEHICLE) && !IsSpellHaveAura(proto, SPELL_AURA_FLY))
        {
            RemoveSpellAuraHolder(iter->second, AURA_REMOVE_BY_DEFAULT);
            iter = m_spellAuraHolders.begin();
        }
        else
        {
            ++iter;
        }
    }
}

/**
 * @brief Reduces the remaining duration of matching aura holders.
 *
 * @param spellId The spell identifier.
 * @param delaytime The delay amount in milliseconds.
 * @param casterGuid The caster GUID to match.
 */
void Unit::DelaySpellAuraHolder(uint32 spellId, int32 delaytime, ObjectGuid casterGuid)
{
    SpellAuraHolderBounds bounds = GetSpellAuraHolderBounds(spellId);
    for (SpellAuraHolderMap::iterator iter = bounds.first; iter != bounds.second; ++iter)
    {
        SpellAuraHolder* holder = iter->second;

        if (casterGuid != holder->GetCasterGuid())
        {
            continue;
        }

        if (holder->GetAuraDuration() < delaytime)
        {
            holder->SetAuraDuration(0);
        }
        else
        {
            holder->SetAuraDuration(holder->GetAuraDuration() - delaytime);
        }

        holder->SendAuraUpdate(false);

        DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "Spell %u partially interrupted on %s, new duration: %u ms", spellId, GetGuidStr().c_str(), holder->GetAuraDuration());
    }
}

/**
 * @brief Unapplies all aura modifiers currently active on the unit.
 */
void Unit::_RemoveAllAuraMods()
{
    for (SpellAuraHolderMap::const_iterator i = m_spellAuraHolders.begin(); i != m_spellAuraHolders.end(); ++i)
    {
        (*i).second->ApplyAuraModifiers(false);
    }
}

/**
 * @brief Reapplies all aura modifiers currently active on the unit.
 */
void Unit::_ApplyAllAuraMods()
{
    for (SpellAuraHolderMap::const_iterator i = m_spellAuraHolders.begin(); i != m_spellAuraHolders.end(); ++i)
    {
        (*i).second->ApplyAuraModifiers(true);
    }
}

/**
 * @brief Checks whether the unit has any aura of a given type.
 *
 * @param auraType The aura type to look for.
 * @return True if at least one matching aura exists; otherwise, false.
 */
bool Unit::HasAuraType(AuraType auraType) const
{
    return !GetAurasByType(auraType).empty();
}

/**
 * @brief Checks whether the unit has an aura type that affects a specific spell.
 *
 * @param auraType The aura type to search.
 * @param spellProto The spell entry to test.
 * @return True if a matching aura affects the spell; otherwise, false.
 */
bool Unit::HasAffectedAura(AuraType auraType, SpellEntry const* spellProto) const
{
    Unit::AuraList const& auras = GetAurasByType(auraType);

    for (Unit::AuraList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
    {
        if ((*itr)->isAffectedOnSpell(spellProto))
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief Gets an aura by spell id and effect index.
 *
 * @param spellId The spell identifier.
 * @param effindex The effect index.
 * @return The matching aura, or NULL if none exists.
 */
Aura* Unit::GetAura(uint32 spellId, SpellEffectIndex effindex)
{
    SpellAuraHolderBounds bounds = GetSpellAuraHolderBounds(spellId);
    if (bounds.first != bounds.second)
    {
        return bounds.first->second->GetAuraByEffectIndex(effindex);
    }
    return NULL;
}

/**
 * @brief Gets the first aura matching type, family, flags, and optional caster.
 *
 * @param type The aura type.
 * @param family The spell family.
 * @param familyFlag The required family flag mask.
 * @param casterGuid An optional caster GUID filter.
 * @return The matching aura, or NULL if none exists.
 */
Aura* Unit::GetAura(AuraType type, SpellFamily family, uint64 familyFlag, uint32 familyFlag2, ObjectGuid casterGuid)
{
    AuraList const& auras = GetAurasByType(type);
    for (AuraList::const_iterator i = auras.begin(); i != auras.end(); ++i)
    {
        if ((*i)->GetSpellProto()->IsFitToFamily(family, familyFlag, familyFlag2) &&
            (!casterGuid || (*i)->GetCasterGuid() == casterGuid))
        {
            return *i;
        }
    }

    return NULL;
}

Aura* Unit::GetTriggeredByClientAura(uint32 spellId) const
{
    MANGOS_ASSERT(spellId);

    AuraList const& auras = GetAurasByType(SPELL_AURA_PERIODIC_TRIGGER_BY_CLIENT);
    for (AuraList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
    {
        SpellAuraHolder const* holder = (*itr)->GetHolder();
        if (!holder || holder->IsDeleted())
        {
            continue;
        }

        // NOTE for further development: If there are more spells of this aura type, it might be required to check that this is the effect that applies SPELL_AURA_PERIODIC_TRIGGER_BY_CLIENT
        if (holder->GetCasterGuid() == GetObjectGuid() && holder->GetSpellProto()->EffectTriggerSpell[(*itr)->GetEffIndex()] == spellId)
        {
            return *itr;
        }
    }

    return NULL;
}

/**
 * @brief Checks whether the unit has a spell aura at a specific effect index.
 *
 * @param spellId The spell identifier.
 * @param effIndex The effect index.
 * @return True if a matching aura exists; otherwise, false.
 */
bool Unit::HasAura(uint32 spellId, SpellEffectIndex effIndex) const
{
    //Find all auras with corresponding spellid, can be more than one
    SpellAuraHolderConstBounds spair = GetSpellAuraHolderBounds(spellId);
    for (SpellAuraHolderMap::const_iterator i_holder = spair.first; i_holder != spair.second; ++i_holder)
    {
        if (i_holder->second->GetAuraByEffectIndex(effIndex))
        {
            return true;
        }
    }

    return false;
}

bool Unit::HasAuraOfDifficulty(uint32 spellId) const
{
    SpellEntry const* spellEntry = sSpellStore.LookupEntry(spellId);
    if (spellEntry && spellEntry->Difficulty && IsInWorld() && GetMap()->IsDungeon())
        if (SpellEntry const* spellDiffEntry = GetSpellEntryByDifficulty(spellEntry->Difficulty, GetMap()->GetDifficulty(), GetMap()->IsRaid()))
        {
            spellId = spellDiffEntry->ID;
        }

    return m_spellAuraHolders.find(spellId) != m_spellAuraHolders.end();
}

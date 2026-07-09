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
 * @file SpellTargeting.cpp
 * @brief Cohesion split of Spell.cpp -- SetTargetMap target selection.
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

template<typename T>
/**
 * @brief Finds a nearby corpse-like world object matching the search predicate.
 *
 * @tparam T The corpse search predicate type.
 * @return The first matching world object, or null if none are found.
 */
WorldObject* Spell::FindCorpseUsing()
{
    // non-standard target selection
    SpellRangeEntry const* srange = sSpellRangeStore.LookupEntry(m_spellInfo->RangeIndex);
    float max_range = GetSpellMaxRange(srange);

    WorldObject* result = NULL;

    T u_check(m_caster, max_range);
    MaNGOS::WorldObjectSearcher<T> searcher(result, u_check);

    Cell::VisitGridObjects(m_caster, searcher, max_range);

    if (!result)
    {
        Cell::VisitWorldObjects(m_caster, searcher, max_range);
    }

    return result;
}

// Helper for Chain Healing
// Spell target first
// Raidmates then descending by injury suffered (MaxHealth - Health)
// Other players/mobs then descending by injury suffered (MaxHealth - Health)
struct ChainHealingOrder
{
    const Unit* MainTarget;
    ChainHealingOrder(Unit const* Target) : MainTarget(Target) {};
    // functor for operator ">"
    bool operator()(Unit const* _Left, Unit const* _Right) const
    {
        return (ChainHealingHash(_Left) < ChainHealingHash(_Right));
    }
    int32 ChainHealingHash(Unit const* Target) const
    {
        if (Target == MainTarget)
        {
            return 0;
        }
        else if (Target->GetTypeId() == TYPEID_PLAYER && MainTarget->GetTypeId() == TYPEID_PLAYER &&
                 ((Player const*)Target)->IsInSameRaidWith((Player const*)MainTarget))
        {
            if (Target->GetHealth() == Target->GetMaxHealth())
            {
                return 40000;
            }
            else
            {
                return 20000 - Target->GetMaxHealth() + Target->GetHealth();
            }
        }
        else
        {
            return 40000 - Target->GetMaxHealth() + Target->GetHealth();
        }
    }
};

class ChainHealingFullHealth
{
    public:
        const Unit* MainTarget;
        ChainHealingFullHealth(const Unit* Target) : MainTarget(Target) {};

        bool operator()(const Unit* Target)
        {
            return (Target != MainTarget && Target->GetHealth() == Target->GetMaxHealth());
        }
};

// Helper for targets nearest to the spell target
// The spell target is always first unless there is a target at _completely_ the same position (unbelievable case)
struct TargetDistanceOrderNear
{
    const Unit* MainTarget;
    TargetDistanceOrderNear(const Unit* Target) : MainTarget(Target) {};
    // functor for operator ">"
    bool operator()(const Unit* _Left, const Unit* _Right) const
    {
        return MainTarget->GetDistanceOrder(_Left, _Right);
    }
};

template <class Arg1, class Arg2, class Result>
struct binary_function
{
    typedef Arg1   first_argument_type;
    typedef Arg2   second_argument_type;
    typedef Result result_type;
};

// The spell target is always first unless there is a target at _completely_ the same position (unbelievable case)
struct TargetDistanceOrderFarAway : public binary_function<const Unit, const Unit, bool>
{
    const Unit* MainTarget;
    TargetDistanceOrderFarAway(const Unit* Target) : MainTarget(Target) {};
    // functor for operator "<"
    bool operator()(const Unit* _Left, const Unit* _Right) const
    {
        return !MainTarget->GetDistanceOrder(_Left, _Right);
    }
};

/**
 * @brief Populates a unit target list for a specific implicit target mode.
 *
 * @param effIndex The effect index being processed.
 * @param targetMode The implicit target mode.
 * @param targetUnitMap The unit list being populated.
 */
void Spell::SetTargetMap(SpellEffectIndex effIndex, uint32 targetMode, UnitList& targetUnitMap)
{
    float radius;
    uint32 EffectChainTarget = m_spellInfo->EffectChainTargets[effIndex];
    uint32 unMaxTargets = m_spellInfo->MaxTargets;  // Get spell max affected targets

    GetSpellRangeAndRadius(effIndex, radius, EffectChainTarget, unMaxTargets);

    Unit::AuraList const& mod = m_caster->GetAurasByType(SPELL_AURA_MOD_MAX_AFFECTED_TARGETS);
    for (Unit::AuraList::const_iterator m = mod.begin(); m != mod.end(); ++m)
    {
        if (!(*m)->isAffectedOnSpell(m_spellInfo))
        {
            continue;
        }
        unMaxTargets += (*m)->GetModifier()->m_amount;
    }

    std::list<GameObject*> tempTargetGOList;

    switch (targetMode)
    {
        case TARGET_RANDOM_NEARBY_LOC:
            // special case for Fatal Attraction (BT, Mother Shahraz)
            if (m_spellInfo->ID == 40869)
            {
                radius = 30.0f;
            }

            // Get a random point in circle. Use sqrt(rand) to correct distribution when converting polar to Cartesian coordinates.
            radius *= sqrtf(rand_norm_f());
            // no 'break' expected since we use code in case TARGET_RANDOM_CIRCUMFERENCE_POINT!!!
        case TARGET_RANDOM_CIRCUMFERENCE_POINT:
        {
            // Get a random point AT the circumference
            float angle = 2.0f * M_PI_F * rand_norm_f();
            float dest_x, dest_y, dest_z;
            m_caster->GetClosePoint(dest_x, dest_y, dest_z, 0.0f, radius, angle);
            m_targets.setDestination(dest_x, dest_y, dest_z);

            // This targetMode is often used as 'last' implicitTarget for positive spells, that just require coordinates
            // and no unitTarget (e.g. summon effects). As MaNGOS always needs a unitTarget we add just the caster here.
            // Logic: This is first target, and no second target => use m_caster -- This is second target: use m_caster if the spell is positive or a summon spell
            if ((m_spellInfo->ImplicitTargetA[effIndex] == targetMode && m_spellInfo->ImplicitTargetB[effIndex] == TARGET_NONE) ||
                    (m_spellInfo->ImplicitTargetB[effIndex] == targetMode && (IsPositiveSpell(m_spellInfo) || m_spellInfo->Effect[effIndex] == SPELL_EFFECT_SUMMON)))
                targetUnitMap.push_back(m_caster);
            break;
        }
        case TARGET_91:
        case TARGET_RANDOM_NEARBY_DEST:
        {
            // Get a random point IN the CIRCEL around current M_TARGETS COORDINATES(!).
            if (radius > 0.0f)
            {
                // Use sqrt(rand) to correct distribution when converting polar to Cartesian coordinates.
                radius *= sqrtf(rand_norm_f());
                float angle = 2.0f * M_PI_F * rand_norm_f();
                float dest_x = m_targets.m_destX + cos(angle) * radius;
                float dest_y = m_targets.m_destY + sin(angle) * radius;
                float dest_z = m_caster->GetPositionZ();
                m_caster->UpdateGroundPositionZ(dest_x, dest_y, dest_z);
                m_targets.setDestination(dest_x, dest_y, dest_z);
            }

            // This targetMode is often used as 'last' implicitTarget for positive spells, that just require coordinates
            // and no unitTarget (e.g. summon effects). As MaNGOS always needs a unitTarget we add just the caster here.
            // Logic: This is first target, and no second target => use m_caster -- This is second target: use m_caster if the spell is positive or a summon spell
            if ((m_spellInfo->ImplicitTargetA[effIndex] == targetMode && m_spellInfo->ImplicitTargetB[effIndex] == TARGET_NONE) ||
                    (m_spellInfo->ImplicitTargetB[effIndex] == targetMode && (IsPositiveSpell(m_spellInfo) || m_spellInfo->Effect[effIndex] == SPELL_EFFECT_SUMMON)))
            {
                targetUnitMap.push_back(m_caster);
            }
            break;
        }
        case TARGET_TOTEM_EARTH:
        case TARGET_TOTEM_WATER:
        case TARGET_TOTEM_AIR:
        case TARGET_TOTEM_FIRE:
        {
            float angle = m_caster->GetOrientation();
            switch (targetMode)
            {
                case TARGET_TOTEM_FIRE:  angle += M_PI_F * 0.25f; break;            // front - left
                case TARGET_TOTEM_AIR:   angle += M_PI_F * 0.75f; break;            // back  - left
                case TARGET_TOTEM_WATER: angle += M_PI_F * 1.25f; break;            // back  - right
                case TARGET_TOTEM_EARTH: angle += M_PI_F * 1.75f; break;            // front - right
            }

            float x, y;
            float z = m_caster->GetPositionZ();
            // Do not search for a free spot. TODO: Should there be searched for a free spot. There was once a discussion that in case this space was impossible (LOS) m_caster's position should be used.
            // TODO Bring this back to memory and search for it!
            m_caster->GetNearPoint2D(x, y, radius, angle);
            m_caster->UpdateAllowedPositionZ(x, y, z);
            m_targets.setDestination(x, y, z);

            // Add Summoner
            targetUnitMap.push_back(m_caster);
            break;
        }
        case TARGET_SELF:
        case TARGET_SELF2:
            targetUnitMap.push_back(m_caster);
            break;
        case TARGET_RANDOM_ENEMY_CHAIN_IN_AREA:
        case TARGET_RANDOM_FRIEND_CHAIN_IN_AREA:
        case TARGET_RANDOM_UNIT_CHAIN_IN_AREA:
        {
            m_targets.m_targetMask = 0;
            unMaxTargets = EffectChainTarget;
            float max_range = radius + unMaxTargets * CHAIN_SPELL_JUMP_RADIUS;

            UnitList tempTargetUnitMap;

            switch (targetMode)
            {
                case TARGET_RANDOM_ENEMY_CHAIN_IN_AREA:
                {
                    MaNGOS::AnyAoETargetUnitInObjectRangeCheck u_check(m_caster, max_range);
                    MaNGOS::UnitListSearcher<MaNGOS::AnyAoETargetUnitInObjectRangeCheck> searcher(tempTargetUnitMap, u_check);
                    Cell::VisitAllObjects(m_caster, searcher, max_range);
                    break;
                }
                case TARGET_RANDOM_FRIEND_CHAIN_IN_AREA:
                {
                    MaNGOS::AnyFriendlyUnitInObjectRangeCheck u_check(m_caster, max_range);
                    MaNGOS::UnitListSearcher<MaNGOS::AnyFriendlyUnitInObjectRangeCheck> searcher(tempTargetUnitMap, u_check);
                    Cell::VisitAllObjects(m_caster, searcher, max_range);
                    break;
                }
            }

            if (tempTargetUnitMap.empty())
            {
                break;
            }

            tempTargetUnitMap.sort(TargetDistanceOrderNear(m_caster));

            // Now to get us a random target that's in the initial range of the spell
            uint32 t = 0;
            UnitList::iterator itr = tempTargetUnitMap.begin();
            while (itr != tempTargetUnitMap.end() && (*itr)->IsWithinDist(m_caster, radius))
            {
                ++t, ++itr;
            }

            if (!t)
            {
                break;
            }

            itr = tempTargetUnitMap.begin();
            std::advance(itr, rand() % t);
            Unit* pUnitTarget = *itr;
            targetUnitMap.push_back(pUnitTarget);

            tempTargetUnitMap.erase(itr);

            tempTargetUnitMap.sort(TargetDistanceOrderNear(pUnitTarget));

            t = unMaxTargets - 1;
            Unit* prev = pUnitTarget;
            UnitList::iterator next = tempTargetUnitMap.begin();

            while (t && next != tempTargetUnitMap.end())
            {
                if (!prev->IsWithinDist(*next, CHAIN_SPELL_JUMP_RADIUS))
                {
                    break;
                }

                if (!DisableMgr::IsDisabledFor(DISABLE_TYPE_SPELL, m_spellInfo->ID, NULL, SPELL_DISABLE_LOS) && !prev->IsWithinLOSInMap(*next))
                {
                    ++next;
                    continue;
                }
                prev = *next;
                targetUnitMap.push_back(prev);
                tempTargetUnitMap.erase(next);
                tempTargetUnitMap.sort(TargetDistanceOrderNear(prev));
                next = tempTargetUnitMap.begin();
                --t;
            }
            break;
        }
        case TARGET_PET:
        {
            Pet* tmpUnit = m_caster->GetPet();
            if (!tmpUnit)
            {
                break;
            }
            targetUnitMap.push_back(tmpUnit);
            break;
        }
        case TARGET_CHAIN_DAMAGE:
        {
            if (EffectChainTarget <= 1)
            {
                if (Unit* pUnitTarget = m_caster->SelectMagnetTarget(m_targets.getUnitTarget(), this, effIndex))
                {
                    m_targets.setUnitTarget(pUnitTarget);
                    m_spellFlags |= SPELL_FLAG_REDIRECTED;
                    targetUnitMap.push_back(pUnitTarget);
                }
            }
            else
            {
                Unit* pUnitTarget = m_targets.getUnitTarget();
                WorldObject* originalCaster = GetAffectiveCasterObject();
                if (!pUnitTarget || !originalCaster)
                {
                    break;
                }

                unMaxTargets = EffectChainTarget;

                float max_range;
                if (m_spellInfo->DefenseType == SPELL_DAMAGE_CLASS_MELEE)
                {
                    max_range = radius;
                }
                else
                    // FIXME: This very like horrible hack and wrong for most spells
                {
                    max_range = radius + unMaxTargets * CHAIN_SPELL_JUMP_RADIUS;
                }

                UnitList tempTargetUnitMap;
                {
                    MaNGOS::AnyAoEVisibleTargetUnitInObjectRangeCheck u_check(pUnitTarget, originalCaster, max_range);
                    MaNGOS::UnitListSearcher<MaNGOS::AnyAoEVisibleTargetUnitInObjectRangeCheck> searcher(tempTargetUnitMap, u_check);
                    Cell::VisitAllObjects(m_caster, searcher, max_range);
                }

                if (tempTargetUnitMap.empty())
                {
                    break;
                }

                tempTargetUnitMap.sort(TargetDistanceOrderNear(pUnitTarget));

                if (*tempTargetUnitMap.begin() == pUnitTarget)
                {
                    tempTargetUnitMap.erase(tempTargetUnitMap.begin());
                }

                targetUnitMap.push_back(pUnitTarget);
                uint32 t = unMaxTargets - 1;
                Unit* prev = pUnitTarget;
                UnitList::iterator next = tempTargetUnitMap.begin();

                while (t && next != tempTargetUnitMap.end())
                {
                    if (!prev->IsWithinDist(*next, CHAIN_SPELL_JUMP_RADIUS))
                    {
                        break;
                    }

                    if (!DisableMgr::IsDisabledFor(DISABLE_TYPE_SPELL, m_spellInfo->ID, NULL, SPELL_DISABLE_LOS) && !prev->IsWithinLOSInMap(*next))
                    {
                        ++next;
                        continue;
                    }

                    prev = *next;
                    targetUnitMap.push_back(prev);
                    tempTargetUnitMap.erase(next);
                    tempTargetUnitMap.sort(TargetDistanceOrderNear(prev));
                    next = tempTargetUnitMap.begin();

                    --t;
                }
            }
            break;
        }
        case TARGET_ALL_ENEMY_IN_AREA:
            FillAreaTargets(targetUnitMap, radius, PUSH_DEST_CENTER, SPELL_TARGETS_AOE_DAMAGE);

            switch (m_spellInfo->ID)
            {
                // Do not target current victim
                case 30843:                                 // Enfeeble
                case 31347:                                 // Doom
                case 37676:                                 // Insidious Whisper
                case 38028:                                 // Watery Grave
                case 40618:                                 // Insignificance
                case 41376:                                 // Spite
                case 62166:                                 // Stone Grip
                case 63981:                                 // Stone Grip (h)
                {
                    if (Unit* pVictim = m_caster->getVictim())
                    {
                        targetUnitMap.remove(pVictim);
                    }
                    break;
                }
                // Other special cases
                case 42005:                                 // Bloodboil (spell hits only the 5 furthest away targets)
                {
                    if (targetUnitMap.size() > unMaxTargets)
                    {
                        targetUnitMap.sort(TargetDistanceOrderFarAway(m_caster));
                        targetUnitMap.resize(unMaxTargets);
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        case TARGET_AREAEFFECT_INSTANT:
        {
            SpellTargets targetB = SPELL_TARGETS_AOE_DAMAGE;
            switch (m_spellInfo->Effect[effIndex])
            {
                case SPELL_EFFECT_QUEST_COMPLETE:
                case SPELL_EFFECT_KILL_CREDIT_PERSONAL:
                case SPELL_EFFECT_KILL_CREDIT_GROUP:
                    targetB = SPELL_TARGETS_ALL;
                    break;
                default:
                    // Select friendly targets for positive effect
                    if (IsPositiveEffect(m_spellInfo, effIndex))
                    {
                        targetB = SPELL_TARGETS_FRIENDLY;
                    }
                    break;
            }

            UnitList tempTargetUnitMap;
            SQLMultiStorage::SQLMSIteratorBounds<SpellTargetEntry> bounds = sSpellScriptTargetStorage.getBounds<SpellTargetEntry>(m_spellInfo->ID);

            // fill real target list if no spell script target defined
            FillAreaTargets(bounds.first != bounds.second ? tempTargetUnitMap : targetUnitMap,
                            radius, PUSH_DEST_CENTER, bounds.first != bounds.second ? SPELL_TARGETS_ALL : targetB);

            if (!tempTargetUnitMap.empty())
            {
                for (UnitList::const_iterator iter = tempTargetUnitMap.begin(); iter != tempTargetUnitMap.end(); ++iter)
                {
                    if ((*iter)->GetTypeId() != TYPEID_UNIT)
                    {
                        continue;
                    }

                    for (SQLMultiStorage::SQLMultiSIterator<SpellTargetEntry> i_spellST = bounds.first; i_spellST != bounds.second; ++i_spellST)
                    {
                        if (i_spellST->CanNotHitWithSpellEffect(effIndex))
                        {
                            continue;
                        }

                        // only creature entries supported for this target type
                        if (i_spellST->type == SPELL_TARGET_TYPE_GAMEOBJECT)
                        {
                            continue;
                        }

                        if ((*iter)->GetEntry() == i_spellST->targetEntry)
                        {
                            if (i_spellST->type == SPELL_TARGET_TYPE_DEAD && ((Creature*)(*iter))->IsCorpse())
                            {
                                targetUnitMap.push_back((*iter));
                            }
                            else if (i_spellST->type == SPELL_TARGET_TYPE_CREATURE && (*iter)->IsAlive())
                            {
                                targetUnitMap.push_back((*iter));
                            }

                            break;
                        }
                    }
                }
            }

            // exclude caster
            targetUnitMap.remove(m_caster);
            break;
        }
        case TARGET_AREAEFFECT_CUSTOM:
        {
            if (m_spellInfo->Effect[effIndex] == SPELL_EFFECT_PERSISTENT_AREA_AURA)
            {
                break;
            }
            else if (m_spellInfo->Effect[effIndex] == SPELL_EFFECT_SUMMON)
            {
                targetUnitMap.push_back(m_caster);
                break;
            }

            UnitList tempTargetUnitMap;
            SQLMultiStorage::SQLMSIteratorBounds<SpellTargetEntry> bounds = sSpellScriptTargetStorage.getBounds<SpellTargetEntry>(m_spellInfo->ID);
            // fill real target list if no spell script target defined
            FillAreaTargets(bounds.first != bounds.second ? tempTargetUnitMap : targetUnitMap, radius, PUSH_DEST_CENTER, SPELL_TARGETS_ALL);

            if (!tempTargetUnitMap.empty())
            {
                for (UnitList::const_iterator iter = tempTargetUnitMap.begin(); iter != tempTargetUnitMap.end(); ++iter)
                {
                    if ((*iter)->GetTypeId() != TYPEID_UNIT)
                    {
                        continue;
                    }

                    for (SQLMultiStorage::SQLMultiSIterator<SpellTargetEntry> i_spellST = bounds.first; i_spellST != bounds.second; ++i_spellST)
                    {
                        if (i_spellST->CanNotHitWithSpellEffect(effIndex))
                        {
                            continue;
                        }

                        // only creature entries supported for this target type
                        if (i_spellST->type == SPELL_TARGET_TYPE_GAMEOBJECT)
                        {
                            continue;
                        }

                        if ((*iter)->GetEntry() == i_spellST->targetEntry)
                        {
                            if (i_spellST->type == SPELL_TARGET_TYPE_DEAD && ((Creature*)(*iter))->IsCorpse())
                            {
                                targetUnitMap.push_back((*iter));
                            }
                            else if (i_spellST->type == SPELL_TARGET_TYPE_CREATURE && (*iter)->IsAlive())
                            {
                                targetUnitMap.push_back((*iter));
                            }

                            break;
                        }
                    }
                }
            }
            else
            {
                // remove not targetable units if spell has no script targets
                for (UnitList::iterator itr = targetUnitMap.begin(); itr != targetUnitMap.end();)
                {
                    if (!(*itr)->IsTargetableForAttack(m_spellInfo->HasAttribute(SPELL_ATTR_EX3_CAST_ON_DEAD)))
                    {
                        targetUnitMap.erase(itr++);
                    }
                    else
                    {
                        ++itr;
                    }
                }
            }
            break;
        }
        case TARGET_AREAEFFECT_GO_AROUND_SOURCE:
        case TARGET_AREAEFFECT_GO_AROUND_DEST:
        case TARGET_GO_IN_FRONT_OF_CASTER_90:
        {
            float x, y, z;

            if (targetMode == TARGET_AREAEFFECT_GO_AROUND_SOURCE && (m_targets.m_targetMask & TARGET_FLAG_SOURCE_LOCATION))
            {
                m_targets.getSource(x, y, z);
            }
            else if (targetMode == TARGET_AREAEFFECT_GO_AROUND_DEST)
            {
                m_targets.getDestination(x, y, z);
            }
            else                                            // can also happen for GO_AROUND_SOURCE without SOURCE_LOCATION
            {
                m_caster->GetPosition(x, y, z);
            }

            bool fixedTargetExist = false;
            SQLMultiStorage::SQLMSIteratorBounds<SpellTargetEntry> bounds = sSpellScriptTargetStorage.getBounds<SpellTargetEntry>(m_spellInfo->ID);
            for (SQLMultiStorage::SQLMultiSIterator<SpellTargetEntry> i_spellST = bounds.first; i_spellST != bounds.second; ++i_spellST)
            {
                if (i_spellST->CanNotHitWithSpellEffect(effIndex))
                {
                    continue;
                }

                if (i_spellST->type == SPELL_TARGET_TYPE_GAMEOBJECT)
                {
                    fixedTargetExist = true;
                    // search all GO's with entry, within range of m_destN
                    MaNGOS::GameObjectEntryInPosRangeCheck go_check(*m_caster, i_spellST->targetEntry, x, y, z, radius);
                    MaNGOS::GameObjectListSearcher<MaNGOS::GameObjectEntryInPosRangeCheck> checker(tempTargetGOList, go_check);
                    Cell::VisitGridObjects(m_caster, checker, radius + GetSpellMaxRange(sSpellRangeStore.LookupEntry(m_spellInfo->RangeIndex)));
                }
            }

            if (!fixedTargetExist)
            {
                // Generic handling for spells that require GO-type 33
                if (m_spellInfo->Effect[effIndex] == SPELL_EFFECT_WMO_DAMAGE || m_spellInfo->Effect[effIndex] == SPELL_EFFECT_WMO_REPAIR || m_spellInfo->Effect[effIndex] == SPELL_EFFECT_WMO_CHANGE)
                {
                    MaNGOS::GameObjectTypeInPosRangeCheck go_check(*m_caster, GAMEOBJECT_TYPE_DESTRUCTIBLE_BUILDING, x, y, z, radius, m_spellInfo->Effect[effIndex] == SPELL_EFFECT_WMO_DAMAGE, m_spellInfo->Effect[effIndex] == SPELL_EFFECT_WMO_REPAIR);
                    MaNGOS::GameObjectListSearcher<MaNGOS::GameObjectTypeInPosRangeCheck> checker(tempTargetGOList, go_check);
                    Cell::VisitGridObjects(m_caster, checker, radius + GetSpellMaxRange(sSpellRangeStore.LookupEntry(m_spellInfo->RangeIndex)));
                }
            }

            // Filter some targets for special target-type
            for (std::list<GameObject*>::iterator itr = tempTargetGOList.begin(); itr != tempTargetGOList.end();)
            {
                switch (targetMode)
                {
                    case TARGET_GO_IN_FRONT_OF_CASTER_90:
                        if (!m_caster->HasInArc(M_PI_F / 2, *itr))
                        {
                            tempTargetGOList.erase(itr++);
                            continue;
                        }
                        // no break here
                    case TARGET_AREAEFFECT_GO_AROUND_SOURCE:
                    case TARGET_AREAEFFECT_GO_AROUND_DEST:
                    default:
                        ++itr;
                }
            }
            break;
        }
        case TARGET_ALL_ENEMY_IN_AREA_INSTANT:
        {
            // targets the ground, not the units in the area
            switch (m_spellInfo->Effect[effIndex])
            {
                case SPELL_EFFECT_PERSISTENT_AREA_AURA:
                    break;
                case SPELL_EFFECT_SUMMON:
                    targetUnitMap.push_back(m_caster);
                    break;
                default:
                    FillAreaTargets(targetUnitMap, radius, PUSH_DEST_CENTER, SPELL_TARGETS_AOE_DAMAGE);

                    // Mind Sear, triggered
                    if (m_spellInfo->IsFitToFamily(SPELLFAMILY_PRIEST, UI64LIT(0x0008000000000000)))
                    {
                        if (Unit* unitTarget = m_targets.getUnitTarget())
                        {
                            targetUnitMap.remove(unitTarget);
                        }
                    }
                    break;
            }
            break;
        }
        case TARGET_DUELVSPLAYER_COORDINATES:
        {
            if (Unit* currentTarget = m_targets.getUnitTarget())
            {
                m_targets.setDestination(currentTarget->GetPositionX(), currentTarget->GetPositionY(), currentTarget->GetPositionZ());
            }
            break;
        }
        case TARGET_ALL_PARTY_AROUND_CASTER:
        {
            if (m_caster->GetObjectGuid().IsPet())
            {
                // only affect pet and owner
                targetUnitMap.push_back(m_caster);
                if (Unit* owner = m_caster->GetOwner())
                {
                    targetUnitMap.push_back(owner);
                }
            }
            else
            {
                FillRaidOrPartyTargets(targetUnitMap, m_caster, m_caster, radius, false, true, true);
            }
            break;
        }
        case TARGET_ALL_PARTY_AROUND_CASTER_2:
        case TARGET_ALL_PARTY:
        {
            FillRaidOrPartyTargets(targetUnitMap, m_caster, m_caster, radius, false, true, true);
            break;
        }
        case TARGET_ALL_RAID_AROUND_CASTER:
        {
            if (m_spellInfo->ID == 57669)                   // Replenishment (special target selection)
            {
                // in arena, target should be only caster
                if (m_caster->GetMap()->IsBattleArena())
                {
                    targetUnitMap.push_back(m_caster);
                }
                else
                {
                    FillRaidOrPartyManaPriorityTargets(targetUnitMap, m_caster, m_caster, radius, 10, true, false, true);
                }
            }
            else if (m_spellInfo->ID == 52759)              // Ancestral Awakening (special target selection)
            {
                FillRaidOrPartyHealthPriorityTargets(targetUnitMap, m_caster, m_caster, radius, 1, true, false, true);
            }
            else
            {
                FillRaidOrPartyTargets(targetUnitMap, m_caster, m_caster, radius, true, true, IsPositiveSpell(m_spellInfo->ID));
            }
            break;
        }
        case TARGET_SINGLE_FRIEND:
        case TARGET_SINGLE_FRIEND_2:
            if (m_targets.getUnitTarget())
            {
                targetUnitMap.push_back(m_targets.getUnitTarget());
            }
            break;
        case TARGET_NONCOMBAT_PET:
            if (Unit* target = m_targets.getUnitTarget())
            {
                if (target->GetTypeId() == TYPEID_UNIT && ((Creature*)target)->IsPet() && ((Pet*)target)->getPetType() == MINI_PET)
                {
                    targetUnitMap.push_back(target);
                }
            }
            break;
        case TARGET_SUMMONER:
        {
            WorldObject* caster = GetAffectiveCasterObject();
            if (!caster)
            {
                return;
            }

            if (caster->GetTypeId() == TYPEID_UNIT && ((Creature*)caster)->IsTemporarySummon())
            {
                targetUnitMap.push_back(((TemporarySummon*)(Creature*)caster)->GetSummoner());
            }
            else if (caster->GetTypeId() == TYPEID_GAMEOBJECT && !((GameObject*)caster)->HasStaticDBSpawnData())
            {
                targetUnitMap.push_back(((GameObject*)caster)->GetOwner());
            }
            else
            {
                sLog.outError("SPELL: Spell ID %u with target ID %u was used by non temporary summon object %s.", m_spellInfo->ID, targetMode, caster->GetGuidStr().c_str());
            }
            break;
        }
        case TARGET_CONTROLLED_VEHICLE:
            if (m_caster->IsBoarded() && m_caster->GetTransportInfo()->IsOnVehicle())
            {
                targetUnitMap.push_back((Unit*)m_caster->GetTransportInfo()->GetTransport());
            }
            break;
        case TARGET_VEHICLE_PASSENGER_0:
        case TARGET_VEHICLE_PASSENGER_1:
        case TARGET_VEHICLE_PASSENGER_2:
        case TARGET_VEHICLE_PASSENGER_3:
        case TARGET_VEHICLE_PASSENGER_4:
        case TARGET_VEHICLE_PASSENGER_5:
        case TARGET_VEHICLE_PASSENGER_6:
        case TARGET_VEHICLE_PASSENGER_7:
        {
            if (m_caster->IsVehicle())
            {
                if (Unit* passenger = m_caster->GetVehicleInfo()->GetPassenger(targetMode - TARGET_VEHICLE_PASSENGER_0))
                {
                    targetUnitMap.push_back(passenger);
                }
            }
            break;
        }
        case TARGET_CASTER_COORDINATES:
        {
            // Check original caster is GO - set its coordinates as src cast
            if (WorldObject* caster = GetCastingObject())
            {
                m_targets.setSource(caster->GetPositionX(), caster->GetPositionY(), caster->GetPositionZ());
            }
            break;
        }
        case TARGET_ALL_HOSTILE_UNITS_AROUND_CASTER:
            FillAreaTargets(targetUnitMap, radius, PUSH_SELF_CENTER, SPELL_TARGETS_HOSTILE);
            break;
        case TARGET_ALL_FRIENDLY_UNITS_AROUND_CASTER:
            switch (m_spellInfo->ID)
            {
                case 56153:                                 // Guardian Aura - Ahn'Kahet
                    FillAreaTargets(targetUnitMap, radius, PUSH_SELF_CENTER, SPELL_TARGETS_FRIENDLY);
                    targetUnitMap.remove(m_caster);
                    break;
                case 64844:                                 // Divine Hymn
                    // target amount stored in parent spell dummy effect but hard to access
                    FillRaidOrPartyHealthPriorityTargets(targetUnitMap, m_caster, m_caster, radius, 3, true, false, true);
                    break;
                case 64904:                                 // Hymn of Hope
                    // target amount stored in parent spell dummy effect but hard to access
                    FillRaidOrPartyManaPriorityTargets(targetUnitMap, m_caster, m_caster, radius, 3, true, false, true);
                    break;
                default:
                    // selected friendly units (for casting objects) around casting object
                    FillAreaTargets(targetUnitMap, radius, PUSH_SELF_CENTER, SPELL_TARGETS_FRIENDLY, GetCastingObject());
                    break;
            }
            break;
        case TARGET_ALL_FRIENDLY_UNITS_IN_AREA:
            // Death Pact (in fact selection by player selection)
            if (m_spellInfo->ID == 48743)
            {
                // checked in Spell::CheckCast
                if (m_caster->GetTypeId() == TYPEID_PLAYER)
                {
                    if (Unit* target = m_caster->GetMap()->GetPet(((Player*)m_caster)->GetSelectionGuid()))
                    {
                        targetUnitMap.push_back(target);
                    }
                }
            }

            // Circle of Healing
            else if (m_spellInfo->SpellClassSet == SPELLFAMILY_PRIEST && m_spellInfo->SpellVisualID[0] == 8253)
            {
                Unit* target = m_targets.getUnitTarget();
                if (!target)
                {
                    target = m_caster;
                }

                uint32 count = 5;
                // Glyph of Circle of Healing
                if (Aura const* glyph = m_caster->GetDummyAura(55675))
                {
                    count += glyph->GetModifier()->m_amount;
                }

                FillRaidOrPartyHealthPriorityTargets(targetUnitMap, m_caster, target, radius, count, true, false, true);
            }
            // Wild Growth
            else if (m_spellInfo->SpellClassSet == SPELLFAMILY_DRUID && m_spellInfo->SpellIconID == 2864)
            {
                Unit* target = m_targets.getUnitTarget();
                if (!target)
                {
                    target = m_caster;
                }
                uint32 count = CalculateDamage(EFFECT_INDEX_2, m_caster); // stored in dummy effect, affected by mods

                FillRaidOrPartyHealthPriorityTargets(targetUnitMap, m_caster, target, radius, count, true, false, true);
            }
            else
            {
                FillAreaTargets(targetUnitMap, radius, PUSH_DEST_CENTER, SPELL_TARGETS_FRIENDLY);
            }
            break;
        // TARGET_SINGLE_PARTY means that the spells can only be casted on a party member and not on the caster (some seals, fire shield from imp, etc..)
        case TARGET_SINGLE_PARTY:
        {
            Unit* target = m_targets.getUnitTarget();
            // Those spells apparently can't be casted on the caster.
            if (target && target != m_caster)
            {
                // Can only be casted on group's members or its pets
                Group*  pGroup = NULL;

                Unit* owner = m_caster->GetCharmerOrOwner();
                Unit* targetOwner = target->GetCharmerOrOwner();
                if (owner)
                {
                    if (owner->GetTypeId() == TYPEID_PLAYER)
                    {
                        if (target == owner)
                        {
                            targetUnitMap.push_back(target);
                            break;
                        }
                        pGroup = ((Player*)owner)->GetGroup();
                    }
                }
                else if (m_caster->GetTypeId() == TYPEID_PLAYER)
                {
                    if (targetOwner == m_caster && target->GetTypeId() == TYPEID_UNIT && ((Creature*)target)->IsPet())
                    {
                        targetUnitMap.push_back(target);
                        break;
                    }
                    pGroup = ((Player*)m_caster)->GetGroup();
                }

                if (pGroup)
                {
                    // Our target can also be a player's pet who's grouped with us or our pet. But can't be controlled player
                    if (targetOwner)
                    {
                        if (targetOwner->GetTypeId() == TYPEID_PLAYER &&
                            target->GetTypeId() == TYPEID_UNIT && (((Creature*)target)->IsPet()) &&
                            target->GetOwnerGuid() == targetOwner->GetObjectGuid() &&
                            pGroup->IsMember(((Player*)targetOwner)->GetObjectGuid()))
                        {
                            targetUnitMap.push_back(target);
                        }
                    }
                    // 1Our target can be a player who is on our group
                    else if (target->GetTypeId() == TYPEID_PLAYER && pGroup->IsMember(((Player*)target)->GetObjectGuid()))
                    {
                        targetUnitMap.push_back(target);
                    }
                }
            }
            break;
        }
        case TARGET_GAMEOBJECT:
            if (m_targets.getGOTarget())
            {
                AddGOTarget(m_targets.getGOTarget(), effIndex);
            }
            break;
        case TARGET_IN_FRONT_OF_CASTER:
        {
            SpellNotifyPushType pushType = PUSH_IN_FRONT;
            switch (m_spellInfo->SpellVisualID[0])            // Some spell require a different target fill
            {
                case 3879: pushType = PUSH_IN_BACK;     break;
                case 7441: pushType = PUSH_IN_FRONT_15; break;
                case 8669: pushType = PUSH_IN_FRONT_15; break;
            }
            FillAreaTargets(targetUnitMap, radius, pushType, SPELL_TARGETS_AOE_DAMAGE);
            break;
        }
        case TARGET_LARGE_FRONTAL_CONE:
            FillAreaTargets(targetUnitMap, radius, PUSH_IN_FRONT_90, SPELL_TARGETS_AOE_DAMAGE);
            break;
        case TARGET_FRIENDLY_FRONTAL_CONE:
            FillAreaTargets(targetUnitMap, radius, PUSH_IN_FRONT_30, SPELL_TARGETS_FRIENDLY);
            break;
        case TARGET_NARROW_FRONTAL_CONE:
        {
            SpellTargets targetB = SPELL_TARGETS_AOE_DAMAGE;

            if (m_spellInfo->Effect[effIndex] == SPELL_EFFECT_SCRIPT_EFFECT)
            {
                targetB = SPELL_TARGETS_ALL;
            }

            UnitList tempTargetUnitMap;
            SQLMultiStorage::SQLMSIteratorBounds<SpellTargetEntry> bounds = sSpellScriptTargetStorage.getBounds<SpellTargetEntry>(m_spellInfo->ID);

            // fill real target list if no spell script target defined
            FillAreaTargets(bounds.first != bounds.second ? tempTargetUnitMap : targetUnitMap,
                            radius, PUSH_IN_FRONT_15, bounds.first != bounds.second ? SPELL_TARGETS_ALL : targetB);

            if (!tempTargetUnitMap.empty())
            {
                for (UnitList::const_iterator iter = tempTargetUnitMap.begin(); iter != tempTargetUnitMap.end(); ++iter)
                {
                    if ((*iter)->GetTypeId() != TYPEID_UNIT)
                    {
                        continue;
                    }

                    for (SQLMultiStorage::SQLMultiSIterator<SpellTargetEntry> i_spellST = bounds.first; i_spellST != bounds.second; ++i_spellST)
                    {
                        if (i_spellST->CanNotHitWithSpellEffect(effIndex))
                        {
                            continue;
                        }

                        // only creature entries supported for this target type
                        if (i_spellST->type == SPELL_TARGET_TYPE_GAMEOBJECT)
                        {
                            continue;
                        }

                        if ((*iter)->GetEntry() == i_spellST->targetEntry)
                        {
                            if (i_spellST->type == SPELL_TARGET_TYPE_DEAD && ((Creature*)(*iter))->IsCorpse())
                            {
                                targetUnitMap.push_back((*iter));
                            }
                            else if (i_spellST->type == SPELL_TARGET_TYPE_CREATURE && (*iter)->IsAlive())
                            {
                                targetUnitMap.push_back((*iter));
                            }
                            break;
                        }
                    }
                }
            }
            break;
        }
        case TARGET_IN_FRONT_OF_CASTER_30:
            FillAreaTargets(targetUnitMap, radius, PUSH_IN_FRONT_30, SPELL_TARGETS_AOE_DAMAGE);
            break;
        case TARGET_DUELVSPLAYER:
        {
            if (Unit* target = m_targets.getUnitTarget())
            {
                if (m_caster->IsFriendlyTo(target))
                {
                    targetUnitMap.push_back(target);
                }
                else
                {
                    if (Unit* pUnitTarget = m_caster->SelectMagnetTarget(target, this, effIndex))
                    {
                        if (target != pUnitTarget)
                        {
                            m_targets.setUnitTarget(pUnitTarget);
                            m_spellFlags |= SPELL_FLAG_REDIRECTED;
                        }
                        targetUnitMap.push_back(pUnitTarget);
                    }
                }
            }
            break;
        }
        case TARGET_GAMEOBJECT_ITEM:
            if (m_targets.getGOTargetGuid())
            {
                AddGOTarget(m_targets.getGOTarget(), effIndex);
            }
            else if (m_targets.getItemTarget())
            {
                AddItemTarget(m_targets.getItemTarget(), effIndex);
            }
            break;
        case TARGET_MASTER:
            if (Unit* owner = m_caster->GetCharmerOrOwner())
            {
                targetUnitMap.push_back(owner);
            }
            break;
        case TARGET_ALL_ENEMY_IN_AREA_CHANNELED:
            // targets the ground, not the units in the area
            if (m_spellInfo->Effect[effIndex] != SPELL_EFFECT_PERSISTENT_AREA_AURA)
            {
                FillAreaTargets(targetUnitMap, radius, PUSH_DEST_CENTER, SPELL_TARGETS_AOE_DAMAGE);
            }
            break;
        case TARGET_MINION:
            if (m_spellInfo->Effect[effIndex] != SPELL_EFFECT_DUEL)
            {
                targetUnitMap.push_back(m_caster);
            }
            break;
        case TARGET_SINGLE_ENEMY:
        {
            if (Unit* pUnitTarget = m_caster->SelectMagnetTarget(m_targets.getUnitTarget(), this, effIndex))
            {
                m_targets.setUnitTarget(pUnitTarget);
                m_spellFlags |= SPELL_FLAG_REDIRECTED;
                targetUnitMap.push_back(pUnitTarget);
            }
            break;
        }
        case TARGET_AREAEFFECT_PARTY:
        {
            Unit* owner = m_caster->GetCharmerOrOwner();
            Player* pTarget = NULL;

            if (owner)
            {
                targetUnitMap.push_back(m_caster);
                if (owner->GetTypeId() == TYPEID_PLAYER)
                {
                    pTarget = (Player*)owner;
                }
            }
            else if (m_caster->GetTypeId() == TYPEID_PLAYER)
            {
                if (Unit* target = m_targets.getUnitTarget())
                {
                    if (target->GetTypeId() != TYPEID_PLAYER)
                    {
                        if (((Creature*)target)->IsPet())
                        {
                            Unit* targetOwner = target->GetOwner();
                            if (targetOwner->GetTypeId() == TYPEID_PLAYER)
                            {
                                pTarget = (Player*)targetOwner;
                            }
                        }
                    }
                    else
                    {
                        pTarget = (Player*)target;
                    }
                }
            }

            Group* pGroup = pTarget ? pTarget->GetGroup() : NULL;

            if (pGroup)
            {
                uint8 subgroup = pTarget->GetSubGroup();

                for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
                {
                    Player* Target = itr->getSource();

                    // IsHostileTo check duel and controlled by enemy
                    if (Target && Target->GetSubGroup() == subgroup && !m_caster->IsHostileTo(Target))
                    {
                        if (pTarget->IsWithinDistInMap(Target, radius))
                        {
                            targetUnitMap.push_back(Target);
                        }

                        if (Pet* pet = Target->GetPet())
                            if (pTarget->IsWithinDistInMap(pet, radius))
                            {
                                targetUnitMap.push_back(pet);
                            }
                    }
                }
            }
            else if (owner)
            {
                if (m_caster->IsWithinDistInMap(owner, radius))
                {
                    targetUnitMap.push_back(owner);
                }
            }
            else if (pTarget)
            {
                targetUnitMap.push_back(pTarget);

                if (Pet* pet = pTarget->GetPet())
                {
                    if (m_caster->IsWithinDistInMap(pet, radius))
                    {
                        targetUnitMap.push_back(pet);
                    }
                }
            }
            break;
        }
        case TARGET_SCRIPT:
        {
            if (m_targets.getUnitTarget())
            {
                targetUnitMap.push_back(m_targets.getUnitTarget());
            }
            if (m_targets.getItemTarget())
            {
                AddItemTarget(m_targets.getItemTarget(), effIndex);
            }
            break;
        }
        case TARGET_SELF_FISHING:
            targetUnitMap.push_back(m_caster);
            break;
        case TARGET_CHAIN_HEAL:
        {
            Unit* pUnitTarget = m_targets.getUnitTarget();
            if (!pUnitTarget)
            {
                break;
            }

            if (EffectChainTarget <= 1)
            {
                targetUnitMap.push_back(pUnitTarget);
            }
            else
            {
                unMaxTargets = EffectChainTarget;
                float max_range = radius + unMaxTargets * CHAIN_SPELL_JUMP_RADIUS;

                UnitList tempTargetUnitMap;

                FillAreaTargets(tempTargetUnitMap, max_range, PUSH_SELF_CENTER, SPELL_TARGETS_FRIENDLY);

                if (m_caster != pUnitTarget && std::find(tempTargetUnitMap.begin(), tempTargetUnitMap.end(), m_caster) == tempTargetUnitMap.end())
                {
                    tempTargetUnitMap.push_front(m_caster);
                }

                tempTargetUnitMap.sort(TargetDistanceOrderNear(pUnitTarget));

                if (tempTargetUnitMap.empty())
                {
                    break;
                }

                if (*tempTargetUnitMap.begin() == pUnitTarget)
                {
                    tempTargetUnitMap.erase(tempTargetUnitMap.begin());
                }

                targetUnitMap.push_back(pUnitTarget);
                uint32 t = unMaxTargets - 1;
                Unit* prev = pUnitTarget;
                UnitList::iterator next = tempTargetUnitMap.begin();

                while (t && next != tempTargetUnitMap.end())
                {
                    if (!prev->IsWithinDist(*next, CHAIN_SPELL_JUMP_RADIUS))
                    {
                        break;
                    }

                    if (!DisableMgr::IsDisabledFor(DISABLE_TYPE_SPELL, m_spellInfo->ID, NULL, SPELL_DISABLE_LOS) && !prev->IsWithinLOSInMap(*next))
                    {
                        ++next;
                        continue;
                    }

                    if ((*next)->GetHealth() == (*next)->GetMaxHealth())
                    {
                        next = tempTargetUnitMap.erase(next);
                        continue;
                    }

                    prev = *next;
                    targetUnitMap.push_back(prev);
                    tempTargetUnitMap.erase(next);
                    tempTargetUnitMap.sort(TargetDistanceOrderNear(prev));
                    next = tempTargetUnitMap.begin();

                    --t;
                }
            }
            break;
        }
        case TARGET_CURRENT_ENEMY_COORDINATES:
        {
            Unit* currentTarget = m_targets.getUnitTarget();
            if (currentTarget)
            {
                targetUnitMap.push_back(currentTarget);
                m_targets.setDestination(currentTarget->GetPositionX(), currentTarget->GetPositionY(), currentTarget->GetPositionZ());
            }
            break;
        }
        case TARGET_AREAEFFECT_PARTY_AND_CLASS:
        {
            Player* targetPlayer = m_targets.getUnitTarget() && m_targets.getUnitTarget()->GetTypeId() == TYPEID_PLAYER
                                   ? (Player*)m_targets.getUnitTarget() : NULL;

            Group* pGroup = targetPlayer ? targetPlayer->GetGroup() : NULL;
            if (pGroup)
            {
                for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
                {
                    Player* Target = itr->getSource();

                    // IsHostileTo check duel and controlled by enemy
                    if (Target && targetPlayer->IsWithinDistInMap(Target, radius) &&
                        targetPlayer->getClass() == Target->getClass() &&
                        !m_caster->IsHostileTo(Target))
                    {
                        targetUnitMap.push_back(Target);
                    }
                }
            }
            else if (m_targets.getUnitTarget())
            {
                targetUnitMap.push_back(m_targets.getUnitTarget());
            }
            break;
        }
        case TARGET_TABLE_X_Y_Z_COORDINATES:
        {
            if (SpellTargetPosition const* st = sSpellMgr.GetSpellTargetPosition(m_spellInfo->ID))
            {
                m_targets.setDestination(st->target_X, st->target_Y, st->target_Z);
                // TODO - maybe use an (internal) value for the map for neat far teleport handling

                // far-teleport spells are handled in SpellEffect, elsewise report an error about an unexpected map (spells are always locally)
                if (st->target_mapId != m_caster->GetMapId() && m_spellInfo->Effect[effIndex] != SPELL_EFFECT_TELEPORT_UNITS && m_spellInfo->Effect[effIndex] != SPELL_EFFECT_BIND)
                {
                    sLog.outError("SPELL: wrong map (%u instead %u) target coordinates for spell ID %u", st->target_mapId, m_caster->GetMapId(), m_spellInfo->ID);
                }
            }
            else
            {
                sLog.outError("SPELL: unknown target coordinates for spell ID %u", m_spellInfo->ID);
            }
            break;
        }
        case TARGET_INFRONT_OF_VICTIM:
        case TARGET_BEHIND_VICTIM:
        case TARGET_RIGHT_FROM_VICTIM:
        case TARGET_LEFT_FROM_VICTIM:
        {
            Unit* pTarget = NULL;

            // explicit cast data from client or server-side cast
            // some spell at client send caster
            if (m_targets.getUnitTarget() && m_targets.getUnitTarget() != m_caster)
            {
                pTarget = m_targets.getUnitTarget();
            }
            else if (m_caster->getVictim())
            {
                pTarget = m_caster->getVictim();
            }
            else if (m_caster->GetTypeId() == TYPEID_PLAYER)
            {
                pTarget = sObjectAccessor.GetUnit(*m_caster, ((Player*)m_caster)->GetSelectionGuid());
            }
            else if (m_targets.getUnitTarget())
            {
                pTarget = m_caster;
            }

            if (pTarget)
            {
                float angle = 0.0f;

                switch (targetMode)
                {
                    case TARGET_INFRONT_OF_VICTIM:                        break;
                    case TARGET_BEHIND_VICTIM:      angle = M_PI_F;       break;
                    case TARGET_RIGHT_FROM_VICTIM:  angle = -M_PI_F / 2;  break;
                    case TARGET_LEFT_FROM_VICTIM:   angle = M_PI_F / 2;   break;
                }

                float _target_x, _target_y, _target_z;
                pTarget->GetClosePoint(_target_x, _target_y, _target_z, pTarget->GetObjectBoundingRadius(), radius, angle);
                if (pTarget->IsWithinLOS(_target_x, _target_y, _target_z))
                {
                    targetUnitMap.push_back(m_caster);
                    m_targets.setDestination(_target_x, _target_y, _target_z);
                }
            }
            break;
        }
        case TARGET_DYNAMIC_OBJECT_COORDINATES:
            // if parent spell create dynamic object extract area from it
            if (DynamicObject* dynObj = m_caster->GetDynObject(m_triggeredByAuraSpell ? m_triggeredByAuraSpell->ID : m_spellInfo->ID))
            {
                m_targets.setDestination(dynObj->GetPositionX(), dynObj->GetPositionY(), dynObj->GetPositionZ());
            }
            // else use destination of target if no destination set (ie for Mind Sear - 53022)
            else if (!(m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION) && m_targets.m_targetMask & TARGET_FLAG_UNIT)
            {
                m_targets.setDestination(m_targets.m_destX, m_targets.m_destY, m_targets.m_destZ);
            }
            break;

        case TARGET_DYNAMIC_OBJECT_FRONT:
        case TARGET_DYNAMIC_OBJECT_BEHIND:
        case TARGET_DYNAMIC_OBJECT_LEFT_SIDE:
        case TARGET_DYNAMIC_OBJECT_RIGHT_SIDE:
        {
            if (!(m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION))
            {
                // General override, we don't want to use max spell range here.
                // Note: 0.0 radius is also for index 36. It is possible that 36 must be defined as
                // "at the base of", in difference to 0 which appear to be "directly in front of".
                // TODO: some summoned will make caster be half inside summoned object. Need to fix
                // that in the below code (nearpoint vs closepoint, etc).
                if (m_spellInfo->EffectRadiusIndex[effIndex] == 0)
                {
                    radius = 0.0f;
                }

                if (m_spellInfo->ID == 50019)               // Hawk Hunting, problematic 50K radius
                {
                    radius = 10.0f;
                }

                float angle = m_caster->GetOrientation();
                switch (targetMode)
                {
                    case TARGET_DYNAMIC_OBJECT_FRONT:                           break;
                    case TARGET_DYNAMIC_OBJECT_BEHIND:      angle += M_PI_F;      break;
                    case TARGET_DYNAMIC_OBJECT_LEFT_SIDE:   angle += M_PI_F / 2;  break;
                    case TARGET_DYNAMIC_OBJECT_RIGHT_SIDE:  angle -= M_PI_F / 2;  break;
                }

                float x, y;
                m_caster->GetNearPoint2D(x, y, radius + m_caster->GetObjectBoundingRadius(), angle);
                m_targets.setDestination(x, y, m_caster->GetPositionZ());
            }

            targetUnitMap.push_back(m_caster);
            break;
        }
        case TARGET_POINT_AT_NORTH:
        case TARGET_POINT_AT_SOUTH:
        case TARGET_POINT_AT_EAST:
        case TARGET_POINT_AT_WEST:
        case TARGET_POINT_AT_NE:
        case TARGET_POINT_AT_NW:
        case TARGET_POINT_AT_SE:
        case TARGET_POINT_AT_SW:
        {
            if (!(m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION))
            {
                Unit* currentTarget = m_targets.getUnitTarget() ? m_targets.getUnitTarget() : m_caster;
                float angle = currentTarget != m_caster ? currentTarget->GetAngle(m_caster) : m_caster->GetOrientation();

                switch (targetMode)
                {
                    case TARGET_POINT_AT_NORTH:                         break;
                    case TARGET_POINT_AT_SOUTH: angle +=   M_PI_F;        break;
                    case TARGET_POINT_AT_EAST:  angle -=   M_PI_F / 2;    break;
                    case TARGET_POINT_AT_WEST:  angle +=   M_PI_F / 2;    break;
                    case TARGET_POINT_AT_NE:    angle -=   M_PI_F / 4;    break;
                    case TARGET_POINT_AT_NW:    angle +=   M_PI_F / 4;    break;
                    case TARGET_POINT_AT_SE:    angle -= 3*M_PI_F / 4;    break;
                    case TARGET_POINT_AT_SW:    angle += 3*M_PI_F / 4;    break;
                }

                float x, y;
                currentTarget->GetNearPoint2D(x, y, radius + currentTarget->GetObjectBoundingRadius(), angle);
                m_targets.setDestination(x, y, currentTarget->GetPositionZ());
            }
            break;
        }
        case TARGET_DIRECTLY_FORWARD:
        {
            if (!(m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION))
            {
                SpellRangeEntry const* rEntry = sSpellRangeStore.LookupEntry(m_spellInfo->RangeIndex);
                float minRange = GetSpellMinRange(rEntry);
                float maxRange = GetSpellMaxRange(rEntry);
                float dist = minRange + rand_norm_f() * (maxRange - minRange);

                float _target_x, _target_y, _target_z;
                m_caster->GetClosePoint(_target_x, _target_y, _target_z, m_caster->GetObjectBoundingRadius(), dist);
                m_targets.setDestination(_target_x, _target_y, _target_z);
            }

            targetUnitMap.push_back(m_caster);
            break;
        }
        case TARGET_EFFECT_SELECT:
        {
            // add here custom effects that need default target.
            // FOR EVERY TARGET TYPE THERE IS A DIFFERENT FILL!!
            switch (m_spellInfo->Effect[effIndex])
            {
                case SPELL_EFFECT_DUMMY:
                {
                    switch (m_spellInfo->ID)
                    {
                        case 20577:                         // Cannibalize
                        {
                            WorldObject* result = FindCorpseUsing<MaNGOS::CannibalizeObjectCheck> ();

                            if (result)
                            {
                                switch (result->GetTypeId())
                                {
                                    case TYPEID_UNIT:
                                    case TYPEID_PLAYER:
                                        targetUnitMap.push_back((Unit*)result);
                                        break;
                                    case TYPEID_CORPSE:
                                        m_targets.setCorpseTarget((Corpse*)result);
                                        if (Player* owner = sObjectAccessor.FindPlayer(((Corpse*)result)->GetOwnerGuid()))
                                        {
                                            targetUnitMap.push_back(owner);
                                        }
                                        break;
                                }
                            }
                            else
                            {
                                // clear cooldown at fail
                                if (m_caster->GetTypeId() == TYPEID_PLAYER)
                                {
                                    ((Player*)m_caster)->RemoveSpellCooldown(m_spellInfo->ID, true);
                                }
                                SendCastResult(SPELL_FAILED_NO_EDIBLE_CORPSES);
                                finish(false);
                            }
                            break;
                        }
                        default:
                            if (m_targets.getUnitTarget())
                            {
                                targetUnitMap.push_back(m_targets.getUnitTarget());
                            }
                            break;
                    }
                    // Add AoE target-mask to self, if no target-dest provided already
                    if ((m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION) == 0)
                    {
                        m_targets.setDestination(m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ());
                    }
                    break;
                }
                case SPELL_EFFECT_BIND:
                case SPELL_EFFECT_RESURRECT:
                case SPELL_EFFECT_PARRY:
                case SPELL_EFFECT_BLOCK:
                case SPELL_EFFECT_CREATE_ITEM:
                case SPELL_EFFECT_WEAPON:
                case SPELL_EFFECT_TRIGGER_SPELL:
                case SPELL_EFFECT_TRIGGER_MISSILE:
                case SPELL_EFFECT_LEARN_SPELL:
                case SPELL_EFFECT_SKILL_STEP:
                case SPELL_EFFECT_PROFICIENCY:
                case SPELL_EFFECT_SUMMON_OBJECT_WILD:
                case SPELL_EFFECT_SELF_RESURRECT:
                case SPELL_EFFECT_REPUTATION:
                case SPELL_EFFECT_SEND_TAXI:
                    if (m_targets.getUnitTarget())
                    {
                        targetUnitMap.push_back(m_targets.getUnitTarget());
                    }
                    // Triggered spells have additional spell targets - cast them even if no explicit unit target is given (required for spell 50516 for example)
                    else if (m_spellInfo->Effect[effIndex] == SPELL_EFFECT_TRIGGER_SPELL)
                    {
                        targetUnitMap.push_back(m_caster);
                    }
                    break;
                case SPELL_EFFECT_SUMMON_PLAYER:
                    if (m_caster->GetTypeId() == TYPEID_PLAYER && ((Player*)m_caster)->GetSelectionGuid())
                    {
                        if (Player* target = sObjectMgr.GetPlayer(((Player*)m_caster)->GetSelectionGuid()))
                        {
                            targetUnitMap.push_back(target);
                        }
                    }
                    break;
                case SPELL_EFFECT_RESURRECT_NEW:
                    if (m_targets.getUnitTarget())
                    {
                        targetUnitMap.push_back(m_targets.getUnitTarget());
                    }
                    if (m_targets.getCorpseTargetGuid())
                    {
                        if (Corpse* corpse = m_caster->GetMap()->GetCorpse(m_targets.getCorpseTargetGuid()))
                        {
                            if (Player* owner = sObjectAccessor.FindPlayer(corpse->GetOwnerGuid()))
                            {
                                targetUnitMap.push_back(owner);
                            }
                        }
                    }
                    break;
                case SPELL_EFFECT_TELEPORT_UNITS:
                case SPELL_EFFECT_SUMMON:
                case SPELL_EFFECT_SUMMON_CHANGE_ITEM:
                case SPELL_EFFECT_TRANS_DOOR:
                case SPELL_EFFECT_ADD_FARSIGHT:
                case SPELL_EFFECT_APPLY_GLYPH:
                case SPELL_EFFECT_STUCK:
                case SPELL_EFFECT_BREAK_PLAYER_TARGETING:
                case SPELL_EFFECT_SUMMON_ALL_TOTEMS:
                case SPELL_EFFECT_FEED_PET:
                case SPELL_EFFECT_DESTROY_ALL_TOTEMS:
                case SPELL_EFFECT_SKILL:
                    targetUnitMap.push_back(m_caster);
                    break;
                case SPELL_EFFECT_PERSISTENT_AREA_AURA:
                    if (Unit* currentTarget = m_targets.getUnitTarget())
                    {
                        m_targets.setDestination(currentTarget->GetPositionX(), currentTarget->GetPositionY(), currentTarget->GetPositionZ());
                    }
                    break;
                case SPELL_EFFECT_LEARN_PET_SPELL:
                    if (Pet* pet = m_caster->GetPet())
                    {
                        targetUnitMap.push_back(pet);
                    }
                    break;
                case SPELL_EFFECT_ENCHANT_ITEM:
                case SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY:
                case SPELL_EFFECT_ENCHANT_ITEM_PRISMATIC:
                case SPELL_EFFECT_DISENCHANT:
                case SPELL_EFFECT_PROSPECTING:
                case SPELL_EFFECT_MILLING:
                    if (m_targets.getItemTarget())
                    {
                        AddItemTarget(m_targets.getItemTarget(), effIndex);
                    }
                    break;
                case SPELL_EFFECT_APPLY_AURA:
                    switch (m_spellInfo->EffectAura[effIndex])
                    {
                        case SPELL_AURA_ADD_FLAT_MODIFIER:  // some spell mods auras have 0 target modes instead expected TARGET_SELF(1) (and present for other ranks for same spell for example)
                        case SPELL_AURA_ADD_PCT_MODIFIER:
                            targetUnitMap.push_back(m_caster);
                            break;
                        default:                            // apply to target in other case
                            if (m_targets.getUnitTarget())
                            {
                                targetUnitMap.push_back(m_targets.getUnitTarget());
                            }
                            break;
                    }
                    break;
                case SPELL_EFFECT_APPLY_AREA_AURA_PARTY:
                    // AreaAura
                    if ((m_spellInfo->Attributes == (SPELL_ATTR_NOT_SHAPESHIFT | SPELL_ATTR_DONT_AFFECT_SHEATH_STATE | SPELL_ATTR_CASTABLE_WHILE_MOUNTED | SPELL_ATTR_CASTABLE_WHILE_SITTING)) || (m_spellInfo->Attributes == SPELL_ATTR_NOT_SHAPESHIFT))
                    {
                        SetTargetMap(effIndex, TARGET_AREAEFFECT_PARTY, targetUnitMap);
                    }
                    break;
                case SPELL_EFFECT_SKIN_PLAYER_CORPSE:
                    if (m_targets.getUnitTarget())
                    {
                        targetUnitMap.push_back(m_targets.getUnitTarget());
                    }
                    else if (m_targets.getCorpseTargetGuid())
                    {
                        if (Corpse* corpse = m_caster->GetMap()->GetCorpse(m_targets.getCorpseTargetGuid()))
                        {
                            if (Player* owner = sObjectAccessor.FindPlayer(corpse->GetOwnerGuid()))
                            {
                                targetUnitMap.push_back(owner);
                            }
                        }
                    }
                    break;
                default:
                    break;
            }
            break;
        }
        default:
            // sLog.outError( "SPELL: Unknown implicit target (%u) for spell ID %u", targetMode, m_spellInfo->Id );
            break;
    }

    if (unMaxTargets && targetUnitMap.size() > unMaxTargets)
    {
        // make sure one unit is always removed per iteration
        uint32 removed_utarget = 0;
        for (UnitList::iterator itr = targetUnitMap.begin(), next; itr != targetUnitMap.end(); itr = next)
        {
            next = itr;
            ++next;
            if (!*itr)
            {
                continue;
            }
            if ((*itr) == m_targets.getUnitTarget())
            {
                targetUnitMap.erase(itr);
                removed_utarget = 1;
                //        break;
            }
        }
        // remove random units from the map
        while (targetUnitMap.size() > unMaxTargets - removed_utarget)
        {
            uint32 poz = urand(0, targetUnitMap.size() - 1);
            for (UnitList::iterator itr = targetUnitMap.begin(); itr != targetUnitMap.end(); ++itr, --poz)
            {
                if (!*itr)
                {
                    continue;
                }

                if (!poz)
                {
                    targetUnitMap.erase(itr);
                    break;
                }
            }
        }
        // the player's target will always be added to the map
        if (removed_utarget && m_targets.getUnitTarget())
        {
            targetUnitMap.push_back(m_targets.getUnitTarget());
        }
    }
    if (!tempTargetGOList.empty())                          // GO CASE
    {
        if (unMaxTargets && tempTargetGOList.size() > unMaxTargets)
        {
            // make sure one go is always removed per iteration
            uint32 removed_utarget = 0;
            for (std::list<GameObject*>::iterator itr = tempTargetGOList.begin(), next; itr != tempTargetGOList.end(); itr = next)
            {
                next = itr;
                ++next;
                if (!*itr)
                {
                    continue;
                }
                if ((*itr) == m_targets.getGOTarget())
                {
                    tempTargetGOList.erase(itr);
                    removed_utarget = 1;
                    //        break;
                }
            }
            // remove random units from the map
            while (tempTargetGOList.size() > unMaxTargets - removed_utarget)
            {
                uint32 poz = urand(0, tempTargetGOList.size() - 1);
                for (std::list<GameObject*>::iterator itr = tempTargetGOList.begin(); itr != tempTargetGOList.end(); ++itr, --poz)
                {
                    if (!*itr)
                    {
                        continue;
                    }

                    if (!poz)
                    {
                        tempTargetGOList.erase(itr);
                        break;
                    }
                }
            }
        }
        // Add resulting GOs as GOTargets
        for (std::list<GameObject*>::iterator iter = tempTargetGOList.begin(); iter != tempTargetGOList.end(); ++iter)
        {
            AddGOTarget(*iter, effIndex);
        }
    }
}

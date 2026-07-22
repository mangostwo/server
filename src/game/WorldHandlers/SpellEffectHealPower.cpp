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
 * @file SpellEffectHealPower.cpp
 * @brief Cohesion split of SpellEffects.cpp -- heal, power/energize and item-create effect handlers.
 *        Same `Spell` class; no behaviour change.
 */

#include "Platform/Define.h"
#include <vector>
#include "Database/DatabaseEnv.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "Log.h"
#include "UpdateMask.h"
#include "World.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Player.h"
#include "SkillExtraItems.h"
#include "Unit.h"
#include "Spell.h"
#include "DynamicObject.h"
#include "SpellAuras.h"
#include "Group.h"
#include "UpdateData.h"
#include "MapManager.h"
#include "SharedDefines.h"
#include "Pet.h"
#include "GameObject.h"
#include "GossipDef.h"
#include "Creature.h"
#include "Totem.h"
#include "CreatureAI.h"
#include "BattleGround/BattleGroundMgr.h"
#include "BattleGround/BattleGround.h"
#include "BattleGround/BattleGroundEY.h"
#include "BattleGround/BattleGroundWS.h"
#include "Language.h"
#include "SocialMgr.h"
#include "Util.h"
#include "TemporarySummon.h"
#include "ScriptMgr.h"
#include "SkillDiscovery.h"
#include "Formulas.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "Vehicle.h"
#include "Geometry/Vector3.h"
#include <random>
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif /* ENABLE_ELUNA */

/**
 * @brief Creates and attaches an aura effect to the current unit target.
 *
 * @param eff_idx The aura effect index.
 */
void Spell::EffectApplyAura(SpellEffectIndex eff_idx)
{
    if (!unitTarget)
    {
        return;
    }

    // ghost spell check, allow apply any auras at player loading in ghost mode (will be cleanup after load)
    if ((!unitTarget->IsAlive() && !(IsDeathOnlySpell(m_spellInfo) || IsDeathPersistentSpell(m_spellInfo))) &&
        (unitTarget->GetTypeId() != TYPEID_PLAYER || !((Player*)unitTarget)->GetSession()->PlayerLoading()))
    {
        return;
    }

    Unit* caster = GetAffectiveCaster();
    if (!caster)
    {
        // FIXME: currently we can't have auras applied explicitly by gameobjects
        // so for auras from wild gameobjects (no owner) target used
        if (m_originalCasterGUID.IsGameObject())
        {
            caster = unitTarget;
        }
        else
        {
            return;
        }
    }

    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "Spell: Aura is: %u", m_spellInfo->EffectAura[eff_idx]);

    Aura* aur = CreateAura(m_spellInfo, eff_idx, &m_currentBasePoints[eff_idx], m_spellAuraHolder, unitTarget, caster, m_CastItem);
    m_spellAuraHolder->AddAura(aur, eff_idx);
}

void Spell::EffectUnlearnSpecialization(SpellEffectIndex eff_idx)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    Player* _player = (Player*)unitTarget;
    uint32 spellToUnlearn = m_spellInfo->EffectTriggerSpell[eff_idx];

    _player->removeSpell(spellToUnlearn);

    if (WorldObject const* caster = GetCastingObject())
    {
        DEBUG_LOG("Spell: %s has unlearned spell %u at %s", _player->GetGuidStr().c_str(), spellToUnlearn, caster->GetGuidStr().c_str());
    }
}

/**
 * @brief Drains power from the unit target and optionally restores mana to the caster.
 *
 * @param eff_idx The effect index defining the drained power type.
 */
void Spell::EffectPowerDrain(SpellEffectIndex eff_idx)
{
    if (m_spellInfo->EffectMiscValue[eff_idx] < 0 || m_spellInfo->EffectMiscValue[eff_idx] >= MAX_POWERS)
    {
        return;
    }

    Powers drain_power = Powers(m_spellInfo->EffectMiscValue[eff_idx]);

    if (!unitTarget)
    {
        return;
    }
    if (!unitTarget->IsAlive())
    {
        return;
    }
    if (unitTarget->GetPowerType() != drain_power)
    {
        return;
    }
    if (damage < 0)
    {
        return;
    }

    uint32 curPower = unitTarget->GetPower(drain_power);

    // add spell damage bonus
    damage = m_caster->SpellDamageBonusDone(unitTarget, m_spellInfo, uint32(damage), SPELL_DIRECT_DAMAGE);
    damage = unitTarget->SpellDamageBonusTaken(m_caster, m_spellInfo, uint32(damage), SPELL_DIRECT_DAMAGE);

    // resilience reduce mana draining effect at spell crit damage reduction (added in 2.4)
    uint32 power = damage;
    if (drain_power == POWER_MANA)
    {
        power -= unitTarget->GetSpellCritDamageReduction(power);
    }

    int32 new_damage;
    if (curPower < power)
    {
        new_damage = curPower;
    }
    else
    {
        new_damage = power;
    }

    unitTarget->ModifyPower(drain_power, -new_damage);

    // Don`t restore from self drain
    if (drain_power == POWER_MANA && m_caster != unitTarget)
    {
        float manaMultiplier = m_spellInfo->EffectAmplitude[eff_idx];
        if (manaMultiplier == 0)
        {
            manaMultiplier = 1;
        }

        if (Player* modOwner = m_caster->GetSpellModOwner())
        {
            modOwner->ApplySpellMod(m_spellInfo->ID, SPELLMOD_MULTIPLE_VALUE, manaMultiplier);
        }

        int32 gain = int32(new_damage * manaMultiplier);

        m_caster->EnergizeBySpell(m_caster, m_spellInfo->ID, gain, POWER_MANA);
    }
}

/**
 * @brief Starts a scripted event defined by the spell effect.
 *
 * @param effectIndex The effect index providing the event identifier.
 */
void Spell::EffectSendEvent(SpellEffectIndex effectIndex)
{
    /*
    we do not handle a flag dropping or clicking on flag in battleground by sendevent system
    TODO: Actually, why not...
    */
    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "Spell ScriptStart %u for spellid %u in EffectSendEvent ", m_spellInfo->EffectMiscValue[effectIndex], m_spellInfo->ID);

    StartEvents_Event(m_caster->GetMap(), m_spellInfo->EffectMiscValue[effectIndex], m_caster, focusObject, true, m_caster);
}

/**
 * @brief Burns target power and converts it into spell damage.
 *
 * @param eff_idx The effect index defining the burned power type.
 */
void Spell::EffectPowerBurn(SpellEffectIndex eff_idx)
{
    if (m_spellInfo->EffectMiscValue[eff_idx] < 0 || m_spellInfo->EffectMiscValue[eff_idx] >= MAX_POWERS)
    {
        return;
    }

    Powers powertype = Powers(m_spellInfo->EffectMiscValue[eff_idx]);

    if (!unitTarget)
    {
        return;
    }
    if (!unitTarget->IsAlive())
    {
        return;
    }
    if (unitTarget->GetPowerType() != powertype)
    {
        return;
    }
    if (damage < 0)
    {
        return;
    }

    // burn x% of target's mana, up to maximum of 2x% of caster's mana (Mana Burn)
    if (m_spellInfo->ManaCostPct)
    {
        int32 maxdamage = m_caster->GetMaxPower(powertype) * damage * 2 / 100;
        damage = unitTarget->GetMaxPower(powertype) * damage / 100;
        if (damage > maxdamage)
        {
            damage = maxdamage;
        }
    }

    int32 curPower = int32(unitTarget->GetPower(powertype));

    // resilience reduce mana draining effect at spell crit damage reduction (added in 2.4)
    int32 power = damage;
    if (powertype == POWER_MANA)
    {
        power -= unitTarget->GetSpellCritDamageReduction(power);
    }

    int32 new_damage = (curPower < power) ? curPower : power;

    unitTarget->ModifyPower(powertype, -new_damage);
    float multiplier = m_spellInfo->EffectAmplitude[eff_idx];

    if (Player* modOwner = m_caster->GetSpellModOwner())
    {
        modOwner->ApplySpellMod(m_spellInfo->ID, SPELLMOD_MULTIPLE_VALUE, multiplier);
    }

    new_damage = int32(new_damage * multiplier);
    m_damage += new_damage;
}

/**
 * @brief Accumulates healing for the current unit target.
 *
 * @param eff_idx Unused effect index.
 */
void Spell::EffectHeal(SpellEffectIndex /*eff_idx*/)
{
    if (unitTarget && unitTarget->IsAlive() && damage >= 0)
    {
        // Try to get original caster
        Unit* caster = GetAffectiveCaster();
        if (!caster)
        {
            return;
        }

        int32 addhealth = damage;

        // Seal of Light proc
        if (m_spellInfo->ID == 20167)
        {
            float ap = caster->GetTotalAttackPowerValue(BASE_ATTACK);
            int32 holy = caster->SpellBaseHealingBonusDone(GetSpellSchoolMask(m_spellInfo));
            if (holy < 0)
            {
                holy = 0;
            }
            addhealth += int32(ap * 0.15) + int32(holy * 15 / 100);
        }
        // Vessel of the Naaru (Vial of the Sunwell trinket)
        else if (m_spellInfo->ID == 45064)
        {
            // Amount of heal - depends from stacked Holy Energy
            int damageAmount = 0;
            Unit::AuraList const& mDummyAuras = m_caster->GetAurasByType(SPELL_AURA_DUMMY);
            for (Unit::AuraList::const_iterator i = mDummyAuras.begin(); i != mDummyAuras.end(); ++i)
            {
                if ((*i)->GetId() == 45062)
                {
                    damageAmount += (*i)->GetModifier()->m_amount;
                }
            }
            if (damageAmount)
            {
                m_caster->RemoveAurasDueToSpell(45062);
            }

            addhealth += damageAmount;
        }
        // Death Pact (percent heal)
        else if (m_spellInfo->ID == 48743)
        {
            addhealth = addhealth * unitTarget->GetMaxHealth() / 100;
        }
        // Swiftmend - consumes Regrowth or Rejuvenation
        else if (m_spellInfo->TargetAuraState == AURA_STATE_SWIFTMEND && unitTarget->HasAuraState(AURA_STATE_SWIFTMEND))
        {
            Unit::AuraList const& RejorRegr = unitTarget->GetAurasByType(SPELL_AURA_PERIODIC_HEAL);
            // find most short by duration
            Aura* targetAura = NULL;
            for (Unit::AuraList::const_iterator i = RejorRegr.begin(); i != RejorRegr.end(); ++i)
            {
                if ((*i)->GetSpellProto()->SpellClassSet == SPELLFAMILY_DRUID &&
                    // Regrowth or Rejuvenation 0x40 | 0x10
                    ((*i)->GetSpellProto()->SpellClassMask & UI64LIT(0x0000000000000050)))
                {
                    if (!targetAura || (*i)->GetAuraDuration() < targetAura->GetAuraDuration())
                    {
                        targetAura = *i;
                    }
                }
            }

            if (!targetAura)
            {
                sLog.outError("Target (GUID: %u TypeId: %u) has aurastate AURA_STATE_SWIFTMEND but no matching aura.", unitTarget->GetGUIDLow(), unitTarget->GetTypeId());
                return;
            }
            int idx = 0;
            while (idx < 3)
            {
                if (targetAura->GetSpellProto()->EffectAura[idx] == SPELL_AURA_PERIODIC_HEAL)
                {
                    break;
                }
                ++idx;
            }

            int32 tickheal = targetAura->GetModifier()->m_amount;
            int32 tickcount = GetSpellDuration(targetAura->GetSpellProto()) / targetAura->GetSpellProto()->EffectAuraPeriod[idx] - 1;

            // Glyph of Swiftmend
            if (!caster->HasAura(54824))
            {
                unitTarget->RemoveAurasDueToSpell(targetAura->GetId());
            }

            addhealth += tickheal * tickcount;
        }
        // Runic Healing Injector & Healing Potion Injector effect increase for engineers
        else if ((m_spellInfo->ID == 67486 || m_spellInfo->ID == 67489) && unitTarget->GetTypeId() == TYPEID_PLAYER)
        {
            Player* player = (Player*)unitTarget;
            if (player->HasSkill(SKILL_ENGINEERING))
            {
                addhealth += int32(addhealth * 0.25);
            }
        }

        // Chain Healing
        if (m_spellInfo->SpellClassSet == SPELLFAMILY_SHAMAN && m_spellInfo->SpellClassMask & UI64LIT(0x0000000000000100))
        {
            if (unitTarget == m_targets.getUnitTarget())
            {
                // check for Riptide
                Aura* riptide = unitTarget->GetAura(SPELL_AURA_PERIODIC_HEAL, SPELLFAMILY_SHAMAN, UI64LIT(0x0), 0x00000010, caster->GetObjectGuid());
                if (riptide)
                {
                    addhealth += addhealth / 4;
                    unitTarget->RemoveAurasDueToSpell(riptide->GetId());
                }
            }
        }

        addhealth = caster->SpellHealingBonusDone(unitTarget, m_spellInfo, addhealth, HEAL);
        addhealth = unitTarget->SpellHealingBonusTaken(caster, m_spellInfo, addhealth, HEAL);

        m_healing += addhealth;
    }
}

void Spell::EffectHealPct(SpellEffectIndex /*eff_idx*/)
{
    if (unitTarget && unitTarget->IsAlive() && damage >= 0)
    {
        // Try to get original caster
        Unit* caster = GetAffectiveCaster();
        if (!caster)
        {
            return;
        }

        uint32 addhealth = unitTarget->GetMaxHealth() * damage / 100;

        addhealth = caster->SpellHealingBonusDone(unitTarget, m_spellInfo, addhealth, HEAL);
        addhealth = unitTarget->SpellHealingBonusTaken(caster, m_spellInfo, addhealth, HEAL);

        uint32 absorb = 0;
        unitTarget->CalculateHealAbsorb(addhealth, &absorb);

        int32 gain = caster->DealHeal(unitTarget, addhealth - absorb, m_spellInfo, false, absorb);
        unitTarget->GetHostileRefManager().threatAssist(caster, float(gain) * 0.5f * sSpellMgr.GetSpellThreatMultiplier(m_spellInfo), m_spellInfo);
    }
}

/**
 * @brief Heals a mechanical target immediately.
 *
 * @param eff_idx Unused effect index.
 */
void Spell::EffectHealMechanical(SpellEffectIndex /*eff_idx*/)
{
    // Mechanic creature type should be correctly checked by targetCreatureType field
    if (unitTarget && unitTarget->IsAlive() && damage >= 0)
    {
        // Try to get original caster
        Unit* caster = GetAffectiveCaster();
        if (!caster)
        {
            return;
        }

        uint32 addhealth = caster->SpellHealingBonusDone(unitTarget, m_spellInfo, damage, HEAL);
        addhealth = unitTarget->SpellHealingBonusTaken(caster, m_spellInfo, addhealth, HEAL);

        uint32 absorb = 0;
        unitTarget->CalculateHealAbsorb(addhealth, &absorb);

        caster->DealHeal(unitTarget, addhealth - absorb, m_spellInfo, false, absorb);
    }
}

/**
 * @brief Damages the target and heals the caster for a portion of the damage dealt.
 *
 * @param eff_idx The effect index providing the leech multiplier.
 */
void Spell::EffectHealthLeech(SpellEffectIndex eff_idx)
{
    if (!unitTarget)
    {
        return;
    }
    if (!unitTarget->IsAlive())
    {
        return;
    }

    if (damage < 0)
    {
        return;
    }

    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "HealthLeech :%i", damage);

    uint32 curHealth = unitTarget->GetHealth();
    damage = m_caster->SpellNonMeleeDamageLog(unitTarget, m_spellInfo->ID, damage);
    if ((int32)curHealth < damage)
    {
        damage = curHealth;
    }

    float multiplier = m_spellInfo->EffectAmplitude[eff_idx];

    if (Player* modOwner = m_caster->GetSpellModOwner())
    {
        modOwner->ApplySpellMod(m_spellInfo->ID, SPELLMOD_MULTIPLE_VALUE, multiplier);
    }

    int32 heal = int32(damage * multiplier);
    if (m_caster->IsAlive())
    {
        heal = m_caster->SpellHealingBonusTaken(m_caster, m_spellInfo, heal, HEAL);

        uint32 absorb = 0;
        m_caster->CalculateHealAbsorb(heal, &absorb);

        m_caster->DealHeal(m_caster, heal - absorb, m_spellInfo, false, absorb);
    }
}

/**
 * @brief Creates an item and stores it in the player's inventory.
 *
 * @param eff_idx The effect index creating the item.
 * @param itemtype The item entry to create.
 */
void Spell::DoCreateItem(SpellEffectIndex eff_idx, uint32 itemtype)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    Player* player = (Player*)unitTarget;

    uint32 newitemid = itemtype;
    ItemPrototype const* pProto = ObjectMgr::GetItemPrototype(newitemid);
    if (!pProto)
    {
        player->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, NULL, NULL);
        return;
    }

    // bg reward have some special in code work
    bool bg_mark = false;
    switch (m_spellInfo->ID)
    {
        case SPELL_WG_MARK_VICTORY:
        case SPELL_WG_MARK_DEFEAT:
            bg_mark = true;
            break;
        default:
            break;
    }

    uint32 num_to_add = damage;

    if (num_to_add < 1)
    {
        num_to_add = 1;
    }
    if (num_to_add > pProto->GetMaxStackSize())
    {
        num_to_add = pProto->GetMaxStackSize();
    }

    // init items_count to 1, since 1 item will be created regardless of specialization
    int items_count = 1;
    // the chance to create additional items
    float additionalCreateChance = 0.0f;
    // the maximum number of created additional items
    uint8 additionalMaxNum = 0;
    // get the chance and maximum number for creating extra items
    if (canCreateExtraItems(player, m_spellInfo->ID, additionalCreateChance, additionalMaxNum))
    {
        // roll with this chance till we roll not to create or we create the max num
        while (roll_chance_f(additionalCreateChance) && items_count <= additionalMaxNum)
            ++items_count;
    }

    // really will be created more items
    num_to_add *= items_count;

    // can the player store the new item?
    ItemPosCountVec dest;
    uint32 no_space = 0;
    InventoryResult msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, newitemid, num_to_add, &no_space);
    if (msg != EQUIP_ERR_OK)
    {
        // convert to possible store amount
        if (msg == EQUIP_ERR_INVENTORY_FULL || msg == EQUIP_ERR_CANT_CARRY_MORE_OF_THIS)
        {
            num_to_add -= no_space;
        }
        else
        {
            // ignore mana gem case (next effect will recharge existing example)
            if (eff_idx == EFFECT_INDEX_0 && m_spellInfo->Effect[EFFECT_INDEX_1] == SPELL_EFFECT_DUMMY)
            {
                return;
            }

            // if not created by another reason from full inventory or unique items amount limitation
            player->SendEquipError(msg, NULL, NULL, newitemid);
            return;
        }
    }

    if (num_to_add)
    {
        // create the new item and store it
        Item* pItem = player->StoreNewItem(dest, newitemid, true, Item::GenerateItemRandomPropertyId(newitemid));

        // was it successful? return error if not
        if (!pItem)
        {
            player->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, NULL, NULL);
            return;
        }

        // set the "Crafted by ..." property of the item
        if (pItem->GetProto()->Class != ITEM_CLASS_CONSUMABLE && pItem->GetProto()->Class != ITEM_CLASS_QUEST)
        {
            pItem->SetGuidValue(ITEM_FIELD_CREATOR, player->GetObjectGuid());
        }

        // send info to the client
        player->SendNewItem(pItem, num_to_add, true, !bg_mark);

        // we succeeded in creating at least one item, so a levelup is possible
        if (!bg_mark)
        {
            player->UpdateCraftSkill(m_spellInfo->ID);
        }
    }
}

/**
 * @brief Handles item-creation effects and related special target cleanup.
 *
 * @param eff_idx The effect index creating the item.
 */
void Spell::EffectCreateItem(SpellEffectIndex eff_idx)
{
    switch (m_spellInfo->ID)
    {
    case SPELL_FILLING_EMPTY_JAR__CURSED_OOZE: // Spell 15698 (for Cursed Ooze)
    case SPELL_FILLING_EMPTY_JAR__TAINTED_OOZE: // Spell 15699 (for Tainted Ooze)
    {
        if (unitTarget->GetTypeId() == TYPEID_UNIT) {

            Creature* creature = static_cast<Creature*>(unitTarget);
            if (creature->IsDead() && (creature->GetEntry() == CREATURE_TAINTED_OOZE || creature->GetEntry() == CREATURE_CURSED_OOZE))
            {
                creature->ForcedDespawn();
            }
        }

        break;
    }
    case SPELL_FILLING_EMPTY_JAR__PURE_OOZE: // Spell 15702 (for Primal, Muculent and Glutonous Ooze):
    {
        if (unitTarget->GetTypeId() == TYPEID_UNIT) {

            Creature* creature = static_cast<Creature*>(unitTarget);
            if (creature->IsDead() && (
                creature->GetEntry() == CREATURE_MUCULENT_OOZE ||
                creature->GetEntry() == CREATURE_PRIMAL_OOZE ||
                creature->GetEntry() == CREATURE_GLUTINOUS_OOZE
                ))
            {
                creature->ForcedDespawn();
            }
        }

        break;
    }
    }
    DoCreateItem(eff_idx, m_spellInfo->EffectItemType[eff_idx]);
}

void Spell::EffectCreateItem2(SpellEffectIndex eff_idx)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }
    Player* player = (Player*)m_caster;

    // explicit item (possible fake)
    uint32 item_id = m_spellInfo->EffectItemType[eff_idx];

    if (item_id)
    {
        DoCreateItem(eff_idx, item_id);
    }

    // not explicit loot (with fake item drop if need)
    if (IsLootCraftingSpell(m_spellInfo))
    {
        if (item_id)
        {
            if (!player->HasItemCount(item_id, 1))
            {
                return;
            }

            // remove reagent
            uint32 count = 1;
            player->DestroyItemCount(item_id, count, true);
        }

        // create some random items
        player->AutoStoreLoot(NULL, m_spellInfo->ID, LootTemplates_Spell);
    }
}

void Spell::EffectCreateRandomItem(SpellEffectIndex /*eff_idx*/)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }
    Player* player = (Player*)m_caster;

    // create some random items
    player->AutoStoreLoot(NULL, m_spellInfo->ID, LootTemplates_Spell);
}

/**
 * @brief Creates a persistent area aura dynamic object.
 *
 * @param eff_idx The persistent area aura effect index.
 */
void Spell::EffectPersistentAA(SpellEffectIndex eff_idx)
{
    Unit* pCaster = GetAffectiveCaster();
    // FIXME: in case wild GO will used wrong affective caster (target in fact) as dynobject owner
    if (!pCaster)
    {
        pCaster = m_caster;
    }

    float radius = GetSpellRadius(sSpellRadiusStore.LookupEntry(m_spellInfo->EffectRadiusIndex[eff_idx]));

    if (Player* modOwner = pCaster->GetSpellModOwner())
    {
        modOwner->ApplySpellMod(m_spellInfo->ID, SPELLMOD_RADIUS, radius);
    }

    DynamicObject* dynObj = new DynamicObject;
    if (!dynObj->Create(pCaster->GetMap()->GenerateLocalLowGuid(HIGHGUID_DYNAMICOBJECT), pCaster, m_spellInfo->ID,
                        eff_idx, m_targets.m_destX, m_targets.m_destY, m_targets.m_destZ, m_duration, radius, DYNAMIC_OBJECT_AREA_SPELL))
    {
        delete dynObj;
        return;
    }

    pCaster->AddDynObject(dynObj);
    pCaster->GetMap()->Add(dynObj);
}

/**
 * @brief Restores power to the current unit target.
 *
 * @param eff_idx The effect index defining the power type.
 */
void Spell::EffectEnergize(SpellEffectIndex eff_idx)
{
    if (!unitTarget)
    {
        return;
    }
    if (!unitTarget->IsAlive())
    {
        return;
    }

    if (m_spellInfo->EffectMiscValue[eff_idx] < 0 || m_spellInfo->EffectMiscValue[eff_idx] >= MAX_POWERS)
    {
        return;
    }

    Powers power = Powers(m_spellInfo->EffectMiscValue[eff_idx]);

    // Some level depends spells
    int level_multiplier = 0;
    int level_diff = 0;
    switch (m_spellInfo->ID)
    {
        case 9512:                                          // Restore Energy
            level_diff = m_caster->getLevel() - 40;
            level_multiplier = 2;
            break;
        case 24571:                                         // Blood Fury
            level_diff = m_caster->getLevel() - 60;
            level_multiplier = 10;
            break;
        case 24532:                                         // Burst of Energy
            level_diff = m_caster->getLevel() - 60;
            level_multiplier = 4;
            break;
        case 31930:                                         // Judgements of the Wise
        case 48542:                                         // Revitalize (mana restore case)
        case 63375:                                         // Improved Stormstrike
        case 68082:                                         // Glyph of Seal of Command
            damage = damage * unitTarget->GetCreateMana() / 100;
            break;
        case 67487:                                         // Mana Potion Injector
        case 67490:                                         // Runic Mana Injector
        {
            if (unitTarget->GetTypeId() == TYPEID_PLAYER)
            {
                Player* player = (Player*)unitTarget;
                if (player->HasSkill(SKILL_ENGINEERING))
                {
                    damage += int32(damage * 0.25);
                }
            }
            break;
        }
        default:
            break;
    }

    if (level_diff > 0)
    {
        damage -= level_multiplier * level_diff;
    }

    if (damage < 0)
    {
        return;
    }

    if (unitTarget->GetMaxPower(power) == 0)
    {
        return;
    }

    m_caster->EnergizeBySpell(unitTarget, m_spellInfo->ID, damage, power);

    // Mad Alchemist's Potion
    if (m_spellInfo->ID == 45051)
    {
        // find elixirs on target
        uint32 elixir_mask = 0;
        Unit::SpellAuraHolderMap& Auras = unitTarget->GetSpellAuraHolderMap();
        for (Unit::SpellAuraHolderMap::iterator itr = Auras.begin(); itr != Auras.end(); ++itr)
        {
            uint32 spell_id = itr->second->GetId();
            if (uint32 mask = sSpellMgr.GetSpellElixirMask(spell_id))
            {
                elixir_mask |= mask;
            }
        }

        // get available elixir mask any not active type from battle/guardian (and flask if no any)
        elixir_mask = (elixir_mask & ELIXIR_FLASK_MASK) ^ ELIXIR_FLASK_MASK;

        // get all available elixirs by mask and spell level
        std::vector<uint32> elixirs;
        SpellElixirMap const& m_spellElixirs = sSpellMgr.GetSpellElixirMap();
        for (SpellElixirMap::const_iterator itr = m_spellElixirs.begin(); itr != m_spellElixirs.end(); ++itr)
        {
            if (itr->second & elixir_mask)
            {
                if (itr->second & (ELIXIR_UNSTABLE_MASK | ELIXIR_SHATTRATH_MASK))
                {
                    continue;
                }

                SpellEntry const* spellInfo = sSpellStore.LookupEntry(itr->first);
                if (spellInfo && (spellInfo->SpellLevel < m_spellInfo->SpellLevel || spellInfo->SpellLevel > unitTarget->getLevel()))
                {
                    continue;
                }

                elixirs.push_back(itr->first);
            }
        }

        if (!elixirs.empty())
        {
            // cast random elixir on target
            uint32 rand_spell = urand(0, elixirs.size() - 1);
            m_caster->CastSpell(unitTarget, elixirs[rand_spell], true, m_CastItem);
        }
    }
}

void Spell::EffectEnergisePct(SpellEffectIndex eff_idx)
{
    if (!unitTarget)
    {
        return;
    }
    if (!unitTarget->IsAlive())
    {
        return;
    }

    if (m_spellInfo->EffectMiscValue[eff_idx] < 0 || m_spellInfo->EffectMiscValue[eff_idx] >= MAX_POWERS)
    {
        return;
    }

    Powers power = Powers(m_spellInfo->EffectMiscValue[eff_idx]);

    uint32 maxPower = unitTarget->GetMaxPower(power);
    if (maxPower == 0)
    {
        return;
    }

    uint32 gain = damage * maxPower / 100;
    m_caster->EnergizeBySpell(unitTarget, m_spellInfo->ID, gain, power);
}

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
 * @file SpellEffectDamageTeleport.cpp
 * @brief Cohesion split of SpellEffects.cpp -- damage, trigger-spell and teleport effect handlers.
 *        Same `Spell` class; no behaviour change.
 */

#include "Common.h"
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
#include "ObjectAccessor.h"
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
#include "VMapFactory.h"
#include "Util.h"
#include "TemporarySummon.h"
#include "ScriptMgr.h"
#include "SkillDiscovery.h"
#include "Formulas.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "Vehicle.h"
#include "G3D/Vector3.h"
#include <random>
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif /* ENABLE_ELUNA */

/**
 * @brief Sends a resurrection request to a dead player target.
 *
 * @param eff_idx The effect index providing resurrection values.
 */
void Spell::EffectResurrectNew(SpellEffectIndex eff_idx)
{
    if (!unitTarget || unitTarget->IsAlive())
    {
        return;
    }

    if (unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    if (!unitTarget->IsInWorld())
    {
        return;
    }

    Player* pTarget = ((Player*)unitTarget);

    if (pTarget->isRessurectRequested())      // already have one active request
    {
        return;
    }

    uint32 health = damage;
    uint32 mana = m_spellInfo->EffectMiscValue[eff_idx];
    pTarget->setResurrectRequestData(m_caster->GetObjectGuid(), m_caster->GetMapId(), m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ(), health, mana);
    SendResurrectRequest(pTarget);
}

/**
 * @brief Instantly kills the unit target and handles spell-specific side effects.
 *
 * @param eff_idx Unused effect index.
 */
void Spell::EffectInstaKill(SpellEffectIndex /*eff_idx*/)
{
    if (!unitTarget || !unitTarget->IsAlive())
    {
        return;
    }

    if (m_caster == unitTarget)                             // prevent interrupt message
    {
        finish();
    }

    WorldObject* caster = GetCastingObject();               // we need the original casting object

    WorldPacket data(SMSG_SPELLINSTAKILLLOG, (8 + 8 + 4));
    data << (caster && caster->GetTypeId() != TYPEID_GAMEOBJECT ? m_caster->GetObjectGuid() : ObjectGuid()); // Caster GUID
    data << unitTarget->GetObjectGuid();                    // Victim GUID
    data << uint32(m_spellInfo->Id);
    m_caster->SendMessageToSet(&data, true);

    m_caster->DealDamage(unitTarget, unitTarget->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
}

/**
 * @brief Applies environmental damage to the caster.
 *
 * @param eff_idx The effect index used to calculate the base damage.
 */
void Spell::EffectEnvironmentalDMG(SpellEffectIndex eff_idx)
{
    uint32 absorb = 0;
    uint32 resist = 0;

    // Note: this hack with damage replace required until GO casting not implemented
    // environment damage spells already have around enemies targeting but this not help in case nonexistent GO casting support
    // currently each enemy selected explicitly and self cast damage, we prevent apply self casted spell bonuses/etc
    damage = m_spellInfo->CalculateSimpleValue(eff_idx);

    m_caster->CalculateDamageAbsorbAndResist(m_caster, GetSpellSchoolMask(m_spellInfo), SPELL_DIRECT_DAMAGE, damage, &absorb, &resist);

    m_caster->SendSpellNonMeleeDamageLog(m_caster, m_spellInfo->Id, damage, GetSpellSchoolMask(m_spellInfo), absorb, resist, false, 0, false);
    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        ((Player*)m_caster)->EnvironmentalDamage(DAMAGE_FIRE, damage);
    }
}

/**
 * @brief Computes school-damage special cases and accumulates resulting damage.
 *
 * @param effect_idx The damage effect index.
 */
void Spell::EffectSchoolDMG(SpellEffectIndex effect_idx)
{
    if (unitTarget && unitTarget->IsAlive())
    {
        switch (m_spellInfo->SpellFamilyName)
        {
            case SPELLFAMILY_GENERIC:
            {
                switch (m_spellInfo->Id)                    // better way to check unknown
                {
                        // Meteor like spells (divided damage to targets)
                    case 24340: case 26558: case 28884:     // Meteor
                    case 36837: case 38903: case 41276:     // Meteor
                    case 57467:                             // Meteor
                    case 26789:                             // Shard of the Fallen Star
                    case 31436:                             // Malevolent Cleave
                    case 35181:                             // Dive Bomb
                    case 40810: case 43267: case 43268:     // Saber Lash
                    case 42384:                             // Brutal Swipe
                    case 45150:                             // Meteor Slash
                    case 64422: case 64688:                 // Sonic Screech
                    case 70492: case 72505:                 // Ooze Eruption
                    case 71904:                             // Chaos Bane
                    case 72624: case 72625:                 // Ooze Eruption
                    {
                        uint32 count = 0;
                        for (TargetList::const_iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
                        {
                            if (ihit->effectMask & (1 << effect_idx))
                            {
                                ++count;
                            }
                        }

                        damage /= count;                    // divide to all targets
                        break;
                    }
                    // percent from health with min
                    case 25599:                             // Thundercrash
                    {
                        damage = unitTarget->GetHealth() / 2;
                        if (damage < 200)
                        {
                            damage = 200;
                        }
                        break;
                    }
                    // Intercept (warrior spell trigger)
                    case 20253:
                    case 61491:
                    {
                        damage += uint32(m_caster->GetTotalAttackPowerValue(BASE_ATTACK) * 0.12f);
                        break;
                    }
                    // percent max target health
                    case 29142:                             // Eyesore Blaster
                    case 35139:                             // Throw Boom's Doom
                    case 49882:                             // Leviroth Self-Impale
                    case 55269:                             // Deathly Stare
                    {
                        damage = damage * unitTarget->GetMaxHealth() / 100;
                        break;
                    }
                    // Cataclysmic Bolt
                    case 38441:
                    {
                        damage = unitTarget->GetMaxHealth() / 2;
                        break;
                    }
                    // Touch the Nightmare
                    case 50341:
                    {
                        if (effect_idx == EFFECT_INDEX_2)
                        {
                            damage = int32(unitTarget->GetMaxHealth() * 0.3f);
                        }
                        break;
                    }
                    // Tympanic Tantrum
                    case 62775:
                    {
                        damage = unitTarget->GetMaxHealth() / 10;
                        break;
                    }
                    // Hand of Rekoning (name not have typos ;) )
                    case 67485:
                        damage += uint32(0.5f * m_caster->GetTotalAttackPowerValue(BASE_ATTACK));
                        break;
                        // Magic Bane normal (Forge of Souls - Bronjahm)
                    case 68793:
                    {
                        damage += uint32(unitTarget->GetMaxPower(POWER_MANA) / 2);
                        damage = std::min(damage, 10000);
                        break;
                    }
                    // Magic Bane heroic (Forge of Souls - Bronjahm)
                    case 69050:
                    {
                        damage += uint32(unitTarget->GetMaxPower(POWER_MANA) / 2);
                        damage = std::min(damage, 15000);
                        break;
                    }
                }
                break;
            }
            case SPELLFAMILY_WARRIOR:
            {
                // Bloodthirst
                if (m_spellInfo->SpellFamilyFlags & UI64LIT(0x40000000000))
                {
                    damage = uint32(damage * (m_caster->GetTotalAttackPowerValue(BASE_ATTACK)) / 100);
                }
                // Shield Slam
                else if ((m_spellInfo->SpellFamilyFlags & UI64LIT(0x0000020000000000)) && m_spellInfo->Category == 1209)
                {
                    damage += int32(m_caster->GetShieldBlockValue());
                }
                // Victory Rush
                else if (m_spellInfo->SpellFamilyFlags & UI64LIT(0x10000000000))
                {
                    damage = uint32(damage * m_caster->GetTotalAttackPowerValue(BASE_ATTACK) / 100);
                    m_caster->ModifyAuraState(AURA_STATE_WARRIOR_VICTORY_RUSH, false);
                }
                // Revenge ${$m1+$AP*0.310} to ${$M1+$AP*0.310}
                else if (m_spellInfo->SpellFamilyFlags & UI64LIT(0x0000000000000400))
                {
                    damage += uint32(m_caster->GetTotalAttackPowerValue(BASE_ATTACK) * 0.310f);
                }
                // Heroic Throw ${$m1+$AP*.50}
                else if (m_spellInfo->SpellFamilyFlags & UI64LIT(0x0000000100000000))
                {
                    damage += uint32(m_caster->GetTotalAttackPowerValue(BASE_ATTACK) * 0.5f);
                }
                // Shattering Throw ${$m1+$AP*.50}
                else if (m_spellInfo->SpellFamilyFlags & UI64LIT(0x0040000000000000))
                {
                    damage += uint32(m_caster->GetTotalAttackPowerValue(BASE_ATTACK) * 0.5f);
                }
                // Shockwave ${$m3/100*$AP}
                else if (m_spellInfo->SpellFamilyFlags & UI64LIT(0x0000800000000000))
                {
                    int32 pct = m_caster->CalculateSpellDamage(unitTarget, m_spellInfo, EFFECT_INDEX_2);
                    if (pct > 0)
                    {
                        damage += int32(m_caster->GetTotalAttackPowerValue(BASE_ATTACK) * pct / 100);
                    }
                    break;
                }
                // Thunder Clap
                else if (m_spellInfo->SpellFamilyFlags & UI64LIT(0x0000000000000080))
                {
                    damage += int32(m_caster->GetTotalAttackPowerValue(BASE_ATTACK) * 12 / 100);
                }
                break;
            }
            case SPELLFAMILY_WARLOCK:
            {
                // Incinerate Rank 1 & 2
                if ((m_spellInfo->SpellFamilyFlags & UI64LIT(0x00004000000000)) && m_spellInfo->SpellIconID == 2128)
                {
                    // Incinerate does more dmg (dmg*0.25) if the target have Immolate debuff.
                    // Check aura state for speed but aura state set not only for Immolate spell
                    if (unitTarget->HasAuraState(AURA_STATE_CONFLAGRATE))
                    {
                        Unit::AuraList const& RejorRegr = unitTarget->GetAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
                        for (Unit::AuraList::const_iterator i = RejorRegr.begin(); i != RejorRegr.end(); ++i)
                        {
                            // Immolate
                            if ((*i)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_WARLOCK &&
                                    ((*i)->GetSpellProto()->SpellFamilyFlags & UI64LIT(0x00000000000004)))
                            {
                                damage += damage / 4;
                                break;
                            }
                        }
                    }
                }
                // Shadowflame
                else if (m_spellInfo->SpellFamilyFlags & UI64LIT(0x0001000000000000))
                {
                    // Apply DOT part
                    switch (m_spellInfo->Id)
                    {
                        case 47897: m_caster->CastSpell(unitTarget, 47960, true); break;
                        case 61290: m_caster->CastSpell(unitTarget, 61291, true); break;
                        default:
                            sLog.outError("Spell::EffectDummy: Unhandeled Shadowflame spell rank %u", m_spellInfo->Id);
                            break;
                    }
                }
                // Shadow Bite
                else if (m_spellInfo->SpellFamilyFlags & UI64LIT(0x0040000000000000))
                {
                    Unit* owner = m_caster->GetOwner();
                    if (!owner)
                    {
                        break;
                    }

                    uint32 counter = 0;
                    Unit::AuraList const& dotAuras = unitTarget->GetAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
                    for (Unit::AuraList::const_iterator itr = dotAuras.begin(); itr != dotAuras.end(); ++itr)
                    {
                        if ((*itr)->GetCasterGuid() == owner->GetObjectGuid())
                        {
                            ++counter;
                        }
                    }

                    if (counter)
                    {
                        damage += (counter * owner->CalculateSpellDamage(unitTarget, m_spellInfo, EFFECT_INDEX_2) * damage) / 100.0f;
                    }
                }
                // Conflagrate - consumes Immolate or Shadowflame
                else if (m_spellInfo->TargetAuraState == AURA_STATE_CONFLAGRATE)
                {
                    Aura const* aura = NULL;                // found req. aura for damage calculation

                    Unit::AuraList const& mPeriodic = unitTarget->GetAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
                    for (Unit::AuraList::const_iterator i = mPeriodic.begin(); i != mPeriodic.end(); ++i)
                    {
                        // for caster applied auras only
                        if ((*i)->GetSpellProto()->SpellFamilyName != SPELLFAMILY_WARLOCK ||
                                (*i)->GetCasterGuid() != m_caster->GetObjectGuid())
                            continue;

                        // Immolate
                        if ((*i)->GetSpellProto()->IsFitToFamilyMask(UI64LIT(0x0000000000000004)))
                        {
                            aura = *i;                      // it selected always if exist
                            break;
                        }

                        // Shadowflame
                        if ((*i)->GetSpellProto()->IsFitToFamilyMask(UI64LIT(0x0000000000000000), 0x00000002))
                        {
                            aura = *i;                      // remember but wait possible Immolate as primary priority
                        }
                    }

                    // found Immolate or Shadowflame
                    if (aura)
                    {
                        int32 damagetick = aura->GetModifier()->m_amount;
                        damage += damagetick * 4;

                        // Glyph of Conflagrate
                        if (!m_caster->HasAura(56235))
                        {
                            unitTarget->RemoveAurasByCasterSpell(aura->GetId(), m_caster->GetObjectGuid());
                        }
                        break;
                    }
                }
                break;
            }
            case SPELLFAMILY_PRIEST:
            {
                // Shadow Word: Death - deals damage equal to damage done to caster
                if (m_spellInfo->SpellFamilyFlags & UI64LIT(0x0000000200000000))
                {
                    m_caster->CastCustomSpell(m_caster, 32409, &damage, 0, 0, true);
                }
                // Improved Mind Blast (Mind Blast in shadow form bonus)
                else if (m_caster->GetShapeshiftForm() == FORM_SHADOW && (m_spellInfo->SpellFamilyFlags & UI64LIT(0x00002000)))
                {
                    Unit::AuraList const& ImprMindBlast = m_caster->GetAurasByType(SPELL_AURA_ADD_FLAT_MODIFIER);
                    for (Unit::AuraList::const_iterator i = ImprMindBlast.begin(); i != ImprMindBlast.end(); ++i)
                    {
                        if ((*i)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_PRIEST &&
                                ((*i)->GetSpellProto()->SpellIconID == 95))
                        {
                            int chance = (*i)->GetSpellProto()->CalculateSimpleValue(EFFECT_INDEX_1);
                            if (roll_chance_i(chance))
                                // Mind Trauma
                                m_caster->CastSpell(unitTarget, 48301, true);
                            break;
                        }
                    }
                }
                break;
            }
            case SPELLFAMILY_DRUID:
            {
                // Ferocious Bite
                if (m_caster->GetTypeId() == TYPEID_PLAYER && (m_spellInfo->SpellFamilyFlags & UI64LIT(0x000800000)) && m_spellInfo->SpellVisual[0] == 6587)
                {
                    // converts up to 30 points of energy into ($f1+$AP/410) additional damage
                    float ap = m_caster->GetTotalAttackPowerValue(BASE_ATTACK);
                    float multiple = ap / 410 + m_spellInfo->DmgMultiplier[effect_idx];
                    damage += int32(((Player*)m_caster)->GetComboPoints() * ap * 7 / 100);
                    uint32 energy = m_caster->GetPower(POWER_ENERGY);
                    uint32 used_energy = energy > 30 ? 30 : energy;
                    damage += int32(used_energy * multiple);
                    m_caster->SetPower(POWER_ENERGY, energy - used_energy);
                }
                // Rake
                else if (m_spellInfo->SpellFamilyFlags & UI64LIT(0x0000000000001000) && m_spellInfo->Effect[EFFECT_INDEX_2] == SPELL_EFFECT_ADD_COMBO_POINTS)
                {
                    // $AP*0.01 bonus
                    damage += int32(m_caster->GetTotalAttackPowerValue(BASE_ATTACK) / 100);
                }
                // Swipe
                else if (m_spellInfo->SpellFamilyFlags & UI64LIT(0x0010000000000000))
                {
                    damage += int32(m_caster->GetTotalAttackPowerValue(BASE_ATTACK) * 0.08f);
                }
                break;
            }
            case SPELLFAMILY_ROGUE:
            {
                // Envenom
                if (m_caster->GetTypeId() == TYPEID_PLAYER && (m_spellInfo->SpellFamilyFlags & UI64LIT(0x800000000)))
                {
                    // consume from stack dozes not more that have combo-points
                    if (uint32 combo = ((Player*)m_caster)->GetComboPoints())
                    {
                        Aura* poison = 0;
                        // Lookup for Deadly poison (only attacker applied)
                        Unit::AuraList const& auras = unitTarget->GetAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
                        for (Unit::AuraList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
                        {
                            if ((*itr)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_ROGUE &&
                                    ((*itr)->GetSpellProto()->SpellFamilyFlags & UI64LIT(0x10000)) &&
                                    (*itr)->GetCasterGuid() == m_caster->GetObjectGuid())
                            {
                                poison = *itr;
                                break;
                            }
                        }
                        // count consumed deadly poison doses at target
                        if (poison)
                        {
                            bool needConsume = true;
                            uint32 spellId = poison->GetId();
                            uint32 doses = poison->GetStackAmount();
                            if (doses > combo)
                            {
                                doses = combo;
                            }

                            // Master Poisoner
                            Unit::AuraList const& auraList = ((Player*)m_caster)->GetAurasByType(SPELL_AURA_MOD_DURATION_OF_EFFECTS_BY_DISPEL);
                            for (Unit::AuraList::const_iterator iter = auraList.begin(); iter != auraList.end(); ++iter)
                            {
                                if ((*iter)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_ROGUE && (*iter)->GetSpellProto()->SpellIconID == 1960)
                                {
                                    if (int32 chance = (*iter)->GetSpellProto()->CalculateSimpleValue(EFFECT_INDEX_2))
                                        if (roll_chance_i(chance))
                                        {
                                            needConsume = false;
                                        }

                                    break;
                                }
                            }

                            if (needConsume)
                            {
                                unitTarget->RemoveAuraHolderFromStack(spellId, doses, m_caster->GetObjectGuid());
                            }

                            damage *= doses;
                            damage += int32(((Player*)m_caster)->GetTotalAttackPowerValue(BASE_ATTACK) * 0.09f * doses);
                        }
                        // Eviscerate and Envenom Bonus Damage (item set effect)
                        if (m_caster->GetDummyAura(37169))
                        {
                            damage += ((Player*)m_caster)->GetComboPoints() * 40;
                        }
                    }
                }
                // Eviscerate
                else if ((m_spellInfo->SpellFamilyFlags & UI64LIT(0x00020000)) && m_caster->GetTypeId() == TYPEID_PLAYER)
                {
                    if (uint32 combo = ((Player*)m_caster)->GetComboPoints())
                    {
                        float ap = m_caster->GetTotalAttackPowerValue(BASE_ATTACK);
                        damage += irand(int32(ap * combo * 0.03f), int32(ap * combo * 0.07f));

                        // Eviscerate and Envenom Bonus Damage (item set effect)
                        if (m_caster->GetDummyAura(37169))
                        {
                            damage += combo * 40;
                        }
                    }
                }
                break;
            }
            case SPELLFAMILY_HUNTER:
            {
                // Gore
                if (m_spellInfo->SpellIconID == 1578)
                {
                    if (m_caster->HasAura(57627))           // Charge 6 sec post-affect
                    {
                        damage *= 2;
                    }
                }
                // Steady Shot
                else if (m_spellInfo->SpellFamilyFlags & UI64LIT(0x100000000))
                {
                    int32 base = irand((int32)m_caster->GetWeaponDamageRange(RANGED_ATTACK, MINDAMAGE), (int32)m_caster->GetWeaponDamageRange(RANGED_ATTACK, MAXDAMAGE));
                    damage += int32(float(base) / m_caster->GetAttackTime(RANGED_ATTACK) * 2800 + m_caster->GetTotalAttackPowerValue(RANGED_ATTACK) * 0.1f);
                }
                break;
            }
            case SPELLFAMILY_PALADIN:
            {
                // Judgement of Righteousness - receive benefit from Spell Damage and Attack power
                if (m_spellInfo->Id == 20187)
                {
                    float ap = m_caster->GetTotalAttackPowerValue(BASE_ATTACK);
                    int32 holy = m_caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(m_spellInfo));
                    if (holy < 0)
                    {
                        holy = 0;
                    }
                    damage += int32(ap * 0.2f) + int32(holy * 32 / 100);
                }
                // Judgement of Vengeance/Corruption ${1+0.22*$SPH+0.14*$AP} + 10% for each application of Holy Vengeance/Blood Corruption on the target
                else if ((m_spellInfo->SpellFamilyFlags & UI64LIT(0x800000000)) && m_spellInfo->SpellIconID == 2292)
                {
                    uint32 debuf_id;
                    switch (m_spellInfo->Id)
                    {
                        case 53733: debuf_id = 53742; break;// Judgement of Corruption -> Blood Corruption
                        case 31804: debuf_id = 31803; break;// Judgement of Vengeance -> Holy Vengeance
                        default: return;
                    }

                    float ap = m_caster->GetTotalAttackPowerValue(BASE_ATTACK);
                    int32 holy = m_caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(m_spellInfo));
                    if (holy < 0)
                    {
                        holy = 0;
                    }
                    damage += int32(ap * 0.14f) + int32(holy * 22 / 100);
                    // Get stack of Holy Vengeance on the target added by caster
                    uint32 stacks = 0;
                    Unit::AuraList const& auras = unitTarget->GetAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
                    for (Unit::AuraList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
                    {
                        if (((*itr)->GetId() == debuf_id) && (*itr)->GetCasterGuid() == m_caster->GetObjectGuid())
                        {
                            stacks = (*itr)->GetStackAmount();
                            break;
                        }
                    }
                    // + 10% for each application of Holy Vengeance on the target
                    if (stacks)
                    {
                        damage += damage * stacks * 10 / 100;
                    }
                }
                // Avenger's Shield ($m1+0.07*$SPH+0.07*$AP) - ranged sdb for future
                else if (m_spellInfo->SpellFamilyFlags & UI64LIT(0x0000000000004000))
                {
                    float ap = m_caster->GetTotalAttackPowerValue(BASE_ATTACK);
                    int32 holy = m_caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(m_spellInfo));
                    if (holy < 0)
                    {
                        holy = 0;
                    }
                    damage += int32(ap * 0.07f) + int32(holy * 7 / 100);
                }
                // Hammer of Wrath ($m1+0.15*$SPH+0.15*$AP) - ranged type sdb future fix
                else if (m_spellInfo->SpellFamilyFlags & UI64LIT(0x0000008000000000))
                {
                    float ap = m_caster->GetTotalAttackPowerValue(BASE_ATTACK);
                    int32 holy = m_caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(m_spellInfo));
                    if (holy < 0)
                    {
                        holy = 0;
                    }
                    damage += int32(ap * 0.15f) + int32(holy * 15 / 100);
                }
                // Hammer of the Righteous
                else if (m_spellInfo->SpellFamilyFlags & UI64LIT(0x0004000000000000))
                {
                    // Add main hand dps * effect[2] amount
                    float average = (m_caster->GetFloatValue(UNIT_FIELD_MINDAMAGE) + m_caster->GetFloatValue(UNIT_FIELD_MAXDAMAGE)) / 2;
                    int32 count = m_caster->CalculateSpellDamage(unitTarget, m_spellInfo, EFFECT_INDEX_2);
                    damage += count * int32(average * IN_MILLISECONDS) / m_caster->GetAttackTime(BASE_ATTACK);
                }
                // Shield of Righteousness
                else if (m_spellInfo->SpellFamilyFlags & UI64LIT(0x0010000000000000))
                {
                    damage += int32(m_caster->GetShieldBlockValue());
                }
                // Judgement
                else if (m_spellInfo->Id == 54158)
                {
                    // [1 + 0.25 * SPH + 0.16 * AP]
                    damage += int32(m_caster->GetTotalAttackPowerValue(BASE_ATTACK) * 0.16f);
                }
                break;
            }
            case SPELLFAMILY_DEATHKNIGHT:
            {
                // Blood Boil - bonus for diseased targets
                if (m_spellInfo->SpellFamilyFlags & 0x00040000 && unitTarget->GetAura(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_DEATHKNIGHT, 0, 0x00000002, m_caster->GetObjectGuid()))
                {
                    damage += m_damage / 2;
                    damage += int32(m_caster->GetTotalAttackPowerValue(BASE_ATTACK)* 0.035f);
                }
                break;
            }
        }

        if (damage >= 0)
        {
            m_damage += damage;
        }
    }
}

void Spell::EffectTriggerSpellWithValue(SpellEffectIndex eff_idx)
{
    uint32 triggered_spell_id = m_spellInfo->EffectTriggerSpell[eff_idx];

    // normal case
    SpellEntry const* spellInfo = sSpellStore.LookupEntry(triggered_spell_id);

    if (!spellInfo)
    {
        // No previous Effect might have started a script
        bool startDBScript = unitTarget && ScriptMgr::CanSpellEffectStartDBScript(m_spellInfo, eff_idx);
        if (startDBScript)
        {
            DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "Spell ScriptStart spellid %u in EffectTriggerSpell", m_spellInfo->Id);
            startDBScript = m_caster->GetMap()->ScriptsStart(DBS_ON_SPELL, m_spellInfo->Id, m_caster, unitTarget);
        }

        if (!startDBScript)
        {
            sLog.outError("EffectTriggerSpell of spell %u: triggering unknown spell id %i", m_spellInfo->Id, triggered_spell_id);
        }
        return;
    }

    int32 bp = damage;
    m_caster->CastCustomSpell(unitTarget, triggered_spell_id, &bp, &bp, &bp, true, m_CastItem , NULL, m_originalCasterGUID, m_spellInfo);
}

void Spell::EffectTriggerRitualOfSummoning(SpellEffectIndex eff_idx)
{
    uint32 triggered_spell_id = m_spellInfo->EffectTriggerSpell[eff_idx];
    SpellEntry const* spellInfo = sSpellStore.LookupEntry(triggered_spell_id);

    if (!spellInfo)
    {
        sLog.outError("EffectTriggerRitualOfSummoning of spell %u: triggering unknown spell id %i", m_spellInfo->Id, triggered_spell_id);
        return;
    }

    finish();

    m_caster->CastSpell(unitTarget, spellInfo, false);
}

void Spell::EffectClearQuest(SpellEffectIndex eff_idx)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    Player* player = (Player*)m_caster;

    uint32 quest_id = m_spellInfo->EffectMiscValue[eff_idx];

    if (!sObjectMgr.GetQuestTemplate(quest_id))
    {
        sLog.outError("Spell::EffectClearQuest spell entry %u attempt clear quest entry %u but this quest does not exist.", m_spellInfo->Id, quest_id);
        return;
    }

    // remove quest possibly in quest log (is that expected?)
    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 quest = player->GetQuestSlotQuestId(slot);

        if (quest == quest_id)
        {
            player->SetQuestSlot(slot, 0);
            // ignore unequippable quest items in this case, it will still be equipped
            player->TakeQuestSourceItem(quest_id, false);
        }
    }

    player->SetQuestStatus(quest_id, QUEST_STATUS_NONE);
    player->getQuestStatusMap()[quest_id].m_rewarded = false;
}

void Spell::EffectForceCast(SpellEffectIndex eff_idx)
{
    if (!unitTarget)
    {
        return;
    }

    uint32 triggered_spell_id = m_spellInfo->EffectTriggerSpell[eff_idx];

    // normal case
    SpellEntry const* spellInfo = sSpellStore.LookupEntry(triggered_spell_id);

    if (!spellInfo)
    {
        sLog.outError("EffectForceCast of spell %u: triggering unknown spell id %i", m_spellInfo->Id, triggered_spell_id);
        return;
    }

    int32 basePoints = damage;

    // forced cast spells by vehicle on master always unboard the master
    if (m_caster->IsVehicle() && m_caster->GetVehicleInfo()->HasOnBoard(unitTarget) &&
        m_spellInfo->EffectImplicitTargetA[eff_idx] == TARGET_MASTER)
    {
        if (sSpellStore.LookupEntry(basePoints))
        {
            m_caster->RemoveAurasDueToSpell(basePoints);
        }
    }

    // spell effect 141 needs to be cast as custom with basePoints
    if (m_spellInfo->Effect[eff_idx] == SPELL_EFFECT_FORCE_CAST_WITH_VALUE)
    {
        unitTarget->CastCustomSpell(unitTarget, spellInfo, &basePoints, &basePoints, &basePoints, true, NULL , NULL, m_originalCasterGUID, m_spellInfo);
    }
    else
    {
        unitTarget->CastSpell(unitTarget, spellInfo, true, NULL, NULL, m_originalCasterGUID, m_spellInfo);
    }
}

/**
 * @brief Triggers another spell on the current unit target.
 *
 * @param eff_idx The effect index providing the triggered spell id.
 */
void Spell::EffectTriggerSpell(SpellEffectIndex effIndex)
{
    // only unit case known
    if (!unitTarget)
    {
        if (gameObjTarget || itemTarget)
        {
            sLog.outError("Spell::EffectTriggerSpell (Spell: %u): Unsupported non-unit case!", m_spellInfo->Id);
        }
        return;
    }

    uint32 triggered_spell_id = m_spellInfo->EffectTriggerSpell[effIndex];

    // special cases
    switch (triggered_spell_id)
    {
            // Vanish (not exist)
        case 18461:
        {
            unitTarget->RemoveSpellsCausingAura(SPELL_AURA_MOD_ROOT);
            unitTarget->RemoveSpellsCausingAura(SPELL_AURA_MOD_DECREASE_SPEED);
            unitTarget->RemoveSpellsCausingAura(SPELL_AURA_MOD_STALKED);

            // if this spell is given to NPC it must handle rest by it's own AI
            if (unitTarget->GetTypeId() != TYPEID_PLAYER)
            {
                return;
            }

            uint32 spellId = 1784;
            // reset cooldown on it if needed
            if (((Player*)unitTarget)->HasSpellCooldown(spellId))
            {
                ((Player*)unitTarget)->RemoveSpellCooldown(spellId);
            }

            m_caster->CastSpell(unitTarget, spellId, true);
            return;
        }
            // Brittle Armor - (need add max stack of 24575 Brittle Armor)
        case 29284:
            m_caster->CastSpell(unitTarget, 24575, true, m_CastItem, NULL, m_originalCasterGUID);
            return;
            // Mercurial Shield - (need add max stack of 26464 Mercurial Shield)
        case 29286:
            m_caster->CastSpell(unitTarget, 26464, true, m_CastItem, NULL, m_originalCasterGUID);
            return;
            // Righteous Defense
        case 31980:
        {
            m_caster->CastSpell(unitTarget, 31790, true, m_CastItem, NULL, m_originalCasterGUID);
            return;
        }
        // Cloak of Shadows
        case 35729:
        {
            Unit::SpellAuraHolderMap& Auras = unitTarget->GetSpellAuraHolderMap();
            for (Unit::SpellAuraHolderMap::iterator iter = Auras.begin(); iter != Auras.end(); ++iter)
            {
                // Remove all harmful spells on you except positive/passive/physical auras
                if (!iter->second->IsPositive() &&
                        !iter->second->IsPassive() &&
                        !iter->second->IsDeathPersistent() &&
                        (GetSpellSchoolMask(iter->second->GetSpellProto()) & SPELL_SCHOOL_MASK_NORMAL) == 0)
                {
                    m_caster->RemoveAurasDueToSpell(iter->second->GetSpellProto()->Id);
                    iter = Auras.begin();
                }
            }
            return;
        }
        // Priest Shadowfiend (34433) need apply mana gain trigger aura on pet
        case 41967:
        {
            if (Unit* pet = unitTarget->GetPet())
            {
                pet->CastSpell(pet, 28305, true);
            }
            return;
        }
        case 53258:                                         // Empower Rune Weapon
        {
            // remove cooldown of frost/death, undead/blood activated in main spell
            if (unitTarget->GetTypeId() == TYPEID_PLAYER)
            {
                bool res1 = ((Player*)unitTarget)->ActivateRunes(RUNE_FROST, 2);
                bool res2 = ((Player*)unitTarget)->ActivateRunes(RUNE_DEATH, 2);
                if (res1 || res2)
                {
                    ((Player*)unitTarget)->ResyncRunes();
                }
            }
            return;
        }
        case 58832:                                         // Mirror Image
        {
            // Glyph of Mirror Image
            if (m_caster->HasAura(63093))
            {
                m_caster->CastSpell(m_caster, 65047, true, m_CastItem, NULL, m_originalCasterGUID);
            }
            break;
        }
    }

    // normal case
    SpellEntry const* spellInfo = sSpellStore.LookupEntry(triggered_spell_id);
    if (!spellInfo)
    {
        // No previous Effect might have started a script
        bool startDBScript = unitTarget && ScriptMgr::CanSpellEffectStartDBScript(m_spellInfo, effIndex);
        if (startDBScript)
        {
            DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "Spell ScriptStart spellid %u in EffectTriggerSpell", m_spellInfo->Id);
            startDBScript = m_caster->GetMap()->ScriptsStart(DBS_ON_SPELL, m_spellInfo->Id, m_caster, unitTarget);
        }

        if (!startDBScript)
        {
            sLog.outError("EffectTriggerSpell of spell %u: triggering unknown spell id %i", m_spellInfo->Id, triggered_spell_id);
        }
        return;
    }

    // select formal caster for triggered spell
    Unit* caster = m_caster;

    // some triggered spells require specific equipment
    if (spellInfo->EquippedItemClass >= 0 && m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        // main hand weapon required
        if (spellInfo->HasAttribute(SPELL_ATTR_EX3_MAIN_HAND))
        {
            Item* item = ((Player*)m_caster)->GetWeaponForAttack(BASE_ATTACK, true, false);

            // skip spell if no weapon in slot or broken
            if (!item)
            {
                return;
            }

            // skip spell if weapon not fit to triggered spell
            if (!item->IsFitToSpellRequirements(spellInfo))
            {
                return;
            }
        }

        // offhand hand weapon required
        if (spellInfo->HasAttribute(SPELL_ATTR_EX3_REQ_OFFHAND))
        {
            Item* item = ((Player*)m_caster)->GetWeaponForAttack(OFF_ATTACK, true, false);

            // skip spell if no weapon in slot or broken
            if (!item)
            {
                return;
            }

            // skip spell if weapon not fit to triggered spell
            if (!item->IsFitToSpellRequirements(spellInfo))
            {
                return;
            }
        }
    }
    else
    {
        // Note: not exist spells with weapon req. and IsSpellHaveCasterSourceTargets == true
        // so this just for speedup places in else
        caster = IsSpellWithCasterSourceTargetsOnly(spellInfo) ? unitTarget : m_caster;
    }

    caster->CastSpell(unitTarget, spellInfo, true, m_CastItem, NULL, m_originalCasterGUID, m_spellInfo);
}

/**
 * @brief Triggers a missile spell at the stored destination coordinates.
 *
 * @param effect_idx The effect index providing the triggered spell id.
 */
void Spell::EffectTriggerMissileSpell(SpellEffectIndex effect_idx)
{
    uint32 triggered_spell_id = m_spellInfo->EffectTriggerSpell[effect_idx];

    // normal case
    SpellEntry const* spellInfo = sSpellStore.LookupEntry(triggered_spell_id);

    if (!spellInfo)
    {
        sLog.outError("EffectTriggerMissileSpell of spell %u (eff: %u): triggering unknown spell id %u",
                      m_spellInfo->Id, effect_idx, triggered_spell_id);
        return;
    }

    if (m_CastItem)
    {
        DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "WORLD: cast Item spellId - %i", spellInfo->Id);
    }

    m_caster->CastSpell(m_targets.m_destX, m_targets.m_destY, m_targets.m_destZ, spellInfo, true, m_CastItem, NULL, m_originalCasterGUID, m_spellInfo);
}

void Spell::EffectJump(SpellEffectIndex eff_idx)
{
    if (m_caster->IsTaxiFlying())
    {
        return;
    }

    // Init dest coordinates
    float x, y, z, o;
    if (m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION)
    {
        m_targets.getDestination(x, y, z);

        if (m_spellInfo->EffectImplicitTargetA[eff_idx] == TARGET_BEHIND_VICTIM)
        {
            // explicit cast data from client or server-side cast
            // some spell at client send caster
            Unit* pTarget = NULL;
            if (m_targets.getUnitTarget() && m_targets.getUnitTarget() != m_caster)
            {
                pTarget = m_targets.getUnitTarget();
            }
            else if (unitTarget->getVictim())
            {
                pTarget = m_caster->getVictim();
            }
            else if (m_caster->GetTypeId() == TYPEID_PLAYER)
            {
                pTarget = m_caster->GetMap()->GetUnit(((Player*)m_caster)->GetSelectionGuid());
            }

            o = pTarget ? pTarget->GetOrientation() : m_caster->GetOrientation();
        }
        else
        {
            o = m_caster->GetOrientation();
        }
    }
    else if (unitTarget)
    {
        unitTarget->GetContactPoint(m_caster, x, y, z, CONTACT_DISTANCE);
        o = m_caster->GetOrientation();
    }
    else if (gameObjTarget)
    {
        gameObjTarget->GetContactPoint(m_caster, x, y, z, CONTACT_DISTANCE);
        o = m_caster->GetOrientation();
    }
    else
    {
        sLog.outError("Spell::EffectJump - unsupported target mode for spell ID %u", m_spellInfo->Id);
        return;
    }

    m_caster->NearTeleportTo(x, y, z, o, true);             // TODO Implement this as jump movement?
}

void Spell::EffectTeleportUnits(SpellEffectIndex eff_idx)   // TODO - Use target settings for this effect!
{
    if (!unitTarget || unitTarget->IsTaxiFlying())
    {
        return;
    }

        switch (m_spellInfo->Id)
        {
            case 48129:                                 // Scroll of Recall
            case 60320:                                 // Scroll of Recall II
            case 60321:                                 // Scroll of Recall III
            {
                uint32 failAtLevel = 0;
                switch (m_spellInfo->Id)
                {
                    case 48129: failAtLevel = 40; break;
                    case 60320: failAtLevel = 70; break;
                    case 60321: failAtLevel = 80; break;
                }

                if (unitTarget->getLevel() > failAtLevel && unitTarget->GetTypeId() == TYPEID_PLAYER)
                {
                    unitTarget->CastSpell(unitTarget, 60444, true);
                    // TODO: Unclear use of probably related spell 60322
                    uint32 spellId = (((Player*)unitTarget)->GetTeam() == ALLIANCE ? 60323 : 60328) + urand(0, 7);
                    unitTarget->CastSpell(unitTarget, spellId, true);
                    return;
                }
                break;
            }
        }

    // Target dependend on TargetB, if there is none provided, decide dependend on A
    uint32 targetType = m_spellInfo->EffectImplicitTargetB[eff_idx];
    if (!targetType)
    {
        targetType = m_spellInfo->EffectImplicitTargetA[eff_idx];
    }

    switch (targetType)
    {
        case TARGET_INNKEEPER_COORDINATES:
        {
            // Only players can teleport to innkeeper
            if (unitTarget->GetTypeId() != TYPEID_PLAYER)
            {
                return;
            }

            ((Player*)unitTarget)->TeleportToHomebind(unitTarget == m_caster ? TELE_TO_SPELL : 0);
            return;
        }
        case TARGET_AREAEFFECT_INSTANT:                     // in all cases first TARGET_TABLE_X_Y_Z_COORDINATES
        case TARGET_TABLE_X_Y_Z_COORDINATES:
        {
            SpellTargetPosition const* st = sSpellMgr.GetSpellTargetPosition(m_spellInfo->Id);
            if (!st)
            {
                sLog.outError("Spell::EffectTeleportUnits - unknown Teleport coordinates for spell ID %u", m_spellInfo->Id);
                return;
            }

            if (st->target_mapId == unitTarget->GetMapId())
            {
                unitTarget->NearTeleportTo(st->target_X, st->target_Y, st->target_Z, st->target_Orientation, unitTarget == m_caster);
            }
            else if (unitTarget->GetTypeId() == TYPEID_PLAYER)
            {
                ((Player*)unitTarget)->TeleportTo(st->target_mapId, st->target_X, st->target_Y, st->target_Z, st->target_Orientation, unitTarget == m_caster ? TELE_TO_SPELL : 0);
            }
            break;
        }
        case TARGET_EFFECT_SELECT:
        {
            // m_destN filled, but sometimes for wrong dest and does not have TARGET_FLAG_DEST_LOCATION

            float x = unitTarget->GetPositionX();
            float y = unitTarget->GetPositionY();
            float z = unitTarget->GetPositionZ();
            float orientation = m_caster->GetOrientation();

            m_caster->NearTeleportTo(x, y, z, orientation, unitTarget == m_caster);
            return;
        }
        case TARGET_BEHIND_VICTIM:
        {
            Unit* pTarget = NULL;

            // explicit cast data from client or server-side cast
            // some spell at client send caster
            if (m_targets.getUnitTarget() && m_targets.getUnitTarget() != unitTarget)
            {
                pTarget = m_targets.getUnitTarget();
            }
            else if (unitTarget->getVictim())
            {
                pTarget = unitTarget->getVictim();
            }
            else if (unitTarget->GetTypeId() == TYPEID_PLAYER)
            {
                pTarget = unitTarget->GetMap()->GetUnit(((Player*)unitTarget)->GetSelectionGuid());
            }

            // Init dest coordinates
            float x = m_targets.m_destX;
            float y = m_targets.m_destY;
            float z = m_targets.m_destZ;
            float orientation = pTarget ? pTarget->GetOrientation() : unitTarget->GetOrientation();
            unitTarget->NearTeleportTo(x, y, z, orientation, unitTarget == m_caster);
            return;
        }
        default:
        {
            // If not exist data for dest location - return
            if (!(m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION))
            {
                sLog.outError("Spell::EffectTeleportUnits - unknown EffectImplicitTargetB[%u] = %u for spell ID %u", eff_idx, m_spellInfo->EffectImplicitTargetB[eff_idx], m_spellInfo->Id);
                return;
            }
            // Init dest coordinates
            float x = m_targets.m_destX;
            float y = m_targets.m_destY;
            float z = m_targets.m_destZ;
            float orientation = unitTarget->GetOrientation();
            // Teleport
            unitTarget->NearTeleportTo(x, y, z, orientation, unitTarget == m_caster);
            return;
        }
    }

    // post effects for TARGET_TABLE_X_Y_Z_COORDINATES
    switch (m_spellInfo->Id)
    {
            // Dimensional Ripper - Everlook
        case 23442:
        {
            int32 r = irand(0, 119);
            if (r >= 70)                                    // 7/12 success
            {
                if (r < 100)                                // 4/12 evil twin
                {
                    m_caster->CastSpell(m_caster, 23445, true);
                }
                else                                        // 1/12 fire
                {
                    m_caster->CastSpell(m_caster, 23449, true);
                }
            }
            return;
        }
        // Ultrasafe Transporter: Toshley's Station
        case 36941:
        {
            if (roll_chance_i(50))                          // 50% success
            {
                int32 rand_eff = urand(1, 7);
                switch (rand_eff)
                {
                    case 1:
                        // soul split - evil
                        m_caster->CastSpell(m_caster, 36900, true);
                        break;
                    case 2:
                        // soul split - good
                        m_caster->CastSpell(m_caster, 36901, true);
                        break;
                    case 3:
                        // Increase the size
                        m_caster->CastSpell(m_caster, 36895, true);
                        break;
                    case 4:
                        // Decrease the size
                        m_caster->CastSpell(m_caster, 36893, true);
                        break;
                    case 5:
                        // Transform
                    {
                        if (((Player*)m_caster)->GetTeam() == ALLIANCE)
                        {
                            m_caster->CastSpell(m_caster, 36897, true);
                        }
                        else
                        {
                            m_caster->CastSpell(m_caster, 36899, true);
                        }
                        break;
                    }
                    case 6:
                        // chicken
                        m_caster->CastSpell(m_caster, 36940, true);
                        break;
                    case 7:
                        // evil twin
                        m_caster->CastSpell(m_caster, 23445, true);
                        break;
                }
            }
            return;
        }
        // Dimensional Ripper - Area 52
        case 36890:
        {
            if (roll_chance_i(50))                          // 50% success
            {
                int32 rand_eff = urand(1, 4);
                switch (rand_eff)
                {
                    case 1:
                        // soul split - evil
                        m_caster->CastSpell(m_caster, 36900, true);
                        break;
                    case 2:
                        // soul split - good
                        m_caster->CastSpell(m_caster, 36901, true);
                        break;
                    case 3:
                        // Increase the size
                        m_caster->CastSpell(m_caster, 36895, true);
                        break;
                    case 4:
                        // Transform
                    {
                        if (((Player*)m_caster)->GetTeam() == ALLIANCE)
                        {
                            m_caster->CastSpell(m_caster, 36897, true);
                        }
                        else
                        {
                            m_caster->CastSpell(m_caster, 36899, true);
                        }
                        break;
                    }
                }
            }
            return;
        }
    }
}

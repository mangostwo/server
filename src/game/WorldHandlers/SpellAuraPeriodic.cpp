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
 * @file SpellAuraPeriodic.cpp
 * @brief Cohesion split of SpellAuras.cpp -- periodic and stat aura handlers.
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
 * @brief Applies special proc-trigger spell setup for specific aura spells.
 *
 * @param apply True to apply the proc aura; false to remove it.
 * @param Real True when processing the real aura state change.
 */
void Aura::HandleAuraProcTriggerSpell(bool apply, bool Real)
{
    if (!Real)
    {
        return;
    }

    Unit* target = GetTarget();

    switch (GetId())
    {
            // some spell have charges by functionality not have its in spell data
        case 28200:                                         // Ascendance (Talisman of Ascendance trinket)
            if (apply)
            {
                GetHolder()->SetAuraCharges(6);
            }
            break;
        case 50720:                                         // Vigilance (threat transfering)
            if (apply)
            {
                if (Unit* caster = GetCaster())
                {
                    target->CastSpell(caster, 59665, true);
                }
            }
            else
            {
                target->GetHostileRefManager().ResetThreatRedirection();
            }
            break;
        default:
            break;
    }
}

/**
 * @brief Applies or removes the tracked-unit dynamic flag.
 *
 * @param apply True to mark the unit as tracked; false to clear it.
 * @param Real Unused.
 */
void Aura::HandleAuraModStalked(bool apply, bool /*Real*/)
{
    // used by spells: Hunter's Mark, Mind Vision, Syndicate Tracker (MURP) DND
    if (apply)
    {
        GetTarget()->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TRACK_UNIT);
    }
    else
    {
        GetTarget()->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TRACK_UNIT);
    }
}

void Aura::HandlePeriodicTriggerSpell(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;

    Unit* target = GetTarget();

    if (!apply)
    {
        switch (GetId())
        {
            case 51165:
                if (m_removeMode == AURA_REMOVE_BY_EXPIRE)
                {
                    target->CastSpell(target, 51173, true, NULL, this);
                }
                return;
            case 66:                                        // Invisibility
                if (m_removeMode == AURA_REMOVE_BY_EXPIRE)
                {
                    target->CastSpell(target, 32612, true, NULL, this);
                }

                return;
            case 42783:                                     // Wrath of the Astrom...
                if (m_removeMode == AURA_REMOVE_BY_EXPIRE && GetEffIndex() + 1 < MAX_EFFECT_INDEX)
                {
                    target->CastSpell(target, GetSpellProto()->CalculateSimpleValue(SpellEffectIndex(GetEffIndex() + 1)), true);
                }

                return;
            case 46221:                                     // Animal Blood
                if (target->GetTypeId() == TYPEID_PLAYER && m_removeMode == AURA_REMOVE_BY_DEFAULT && target->IsInWater())
                {
                    float position_z = target->GetTerrain()->GetWaterLevel(target->GetPositionX(), target->GetPositionY(), target->GetPositionZ());
                    // Spawn Blood Pool
                    target->CastSpell(target->GetPositionX(), target->GetPositionY(), position_z, 63471, true);
                }

                return;
            case 51912:                                     // Ultra-Advanced Proto-Typical Shortening Blaster
                if (m_removeMode == AURA_REMOVE_BY_EXPIRE)
                {
                    if (Unit* pCaster = GetCaster())
                    {
                        pCaster->CastSpell(target, GetSpellProto()->EffectTriggerSpell[GetEffIndex()], true, NULL, this);
                    }
                }

                return;
            default:
                break;
        }
    }
}

/**
 * @brief Enables or disables periodic trigger handling with an explicit value.
 *
 * @param apply True to enable periodic processing; false to disable it.
 * @param Real Unused.
 */
void Aura::HandlePeriodicTriggerSpellWithValue(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
}

/**
 * @brief Enables or disables periodic energize processing.
 *
 * @param apply True to enable periodic processing; false to disable it.
 * @param Real Unused.
 */
void Aura::HandlePeriodicEnergize(bool apply, bool Real)
{
    if (!Real)
    {
        return;
    }

    Unit* target = GetTarget();

    // For prevent double apply bonuses
    bool loading = (target->GetTypeId() == TYPEID_PLAYER && ((Player*)target)->GetSession()->PlayerLoading());

    if (apply && !loading)
    {
        switch (GetId())
        {
            case 54833:                                     // Glyph of Innervate (value%/2 of casters base mana)
            {
                if (Unit* caster = GetCaster())
                {
                    m_modifier.m_amount = int32(caster->GetCreateMana() * GetBasePoints() / (200 * GetAuraMaxTicks()));
                }
                break;
            }
            case 29166:                                     // Innervate (value% of casters base mana)
            {
                if (Unit* caster = GetCaster())
                {
                    // Glyph of Innervate
                    if (caster->HasAura(54832))
                    {
                        caster->CastSpell(caster, 54833, true, NULL, this);
                    }

                    m_modifier.m_amount = int32(caster->GetCreateMana() * GetBasePoints() / (100 * GetAuraMaxTicks()));
                }
                break;
            }
            case 48391:                                     // Owlkin Frenzy 2% base mana
                m_modifier.m_amount = target->GetCreateMana() * 2 / 100;
                break;
            case 57669:                                     // Replenishment (0.2% from max)
            case 61782:                                     // Infinite Replenishment
                m_modifier.m_amount = target->GetMaxPower(POWER_MANA) * 2 / 1000;
                break;
            default:
                break;
        }
    }

    m_isPeriodic = apply;
}

/**
 * @brief Enables or disables periodic power burn processing.
 *
 * @param apply True to enable periodic processing; false to disable it.
 * @param Real Unused.
 */
void Aura::HandleAuraPowerBurn(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
}

void Aura::HandleAuraPeriodicDummy(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
    {
        return;
    }

    Unit* target = GetTarget();

    // For prevent double apply bonuses
    bool loading = (target->GetTypeId() == TYPEID_PLAYER && ((Player*)target)->GetSession()->PlayerLoading());

    SpellEntry const* spell = GetSpellProto();
    switch (spell->SpellClassSet)
    {
        case SPELLFAMILY_ROGUE:
        {
            switch (spell->ID)
            {
                    // Master of Subtlety
                case 31666:
                {
                    if (apply)
                    {
                        // for make duration visible
                        if (SpellAuraHolder* holder = target->GetSpellAuraHolder(31665))
                        {
                            holder->SetAuraMaxDuration(GetHolder()->GetAuraDuration());
                            holder->RefreshHolder();
                        }
                    }
                    else
                    {
                        target->RemoveAurasDueToSpell(31665);
                    }
                    break;
                }
            }
            break;
        }
        case SPELLFAMILY_HUNTER:
        {
            Unit* caster = GetCaster();

            // Explosive Shot
            if (apply && !loading && caster)
            {
                m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(RANGED_ATTACK) * 14 / 100);
            }
            break;
        }
    }

    m_isPeriodic = apply;
}

/**
 * @brief Enables periodic healing and precalculates healing bonuses when applied.
 *
 * @param apply True to enable periodic healing; false to disable it.
 * @param Real Unused.
 */
void Aura::HandlePeriodicHeal(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;

    Unit* target = GetTarget();

    // For prevent double apply bonuses
    bool loading = (target->GetTypeId() == TYPEID_PLAYER && ((Player*)target)->GetSession()->PlayerLoading());

    // Custom damage calculation after
    if (apply)
    {
        if (loading)
        {
            return;
        }

        Unit* caster = GetCaster();
        if (!caster)
        {
            return;
        }

        // Gift of the Naaru (have diff spellfamilies)
        if (GetSpellProto()->SpellIconID == 329 && GetSpellProto()->SpellVisualID[0] == 7625)
        {
            int32 ap = int32(0.22f * caster->GetTotalAttackPowerValue(BASE_ATTACK));
            int32 holy = caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(GetSpellProto()));
            if (holy < 0)
            {
                holy = 0;
            }
            holy = int32(holy * 377 / 1000);
            m_modifier.m_amount += ap > holy ? ap : holy;
        }
        // Lifeblood
        else if (GetSpellProto()->SpellIconID == 3088 && GetSpellProto()->SpellVisualID[0] == 8145)
        {
            int32 healthBonus = int32(0.0032f * caster->GetMaxHealth());
            m_modifier.m_amount += healthBonus;
        }

        m_modifier.m_amount = caster->SpellHealingBonusDone(target, GetSpellProto(), m_modifier.m_amount, DOT, GetStackAmount());

        // Rejuvenation
        if (GetSpellProto()->IsFitToFamily(SPELLFAMILY_DRUID, UI64LIT(0x0000000000000010)))
            if (caster->HasAura(64760))                     // Item - Druid T8 Restoration 4P Bonus
            {
                caster->CastCustomSpell(target, 64801, &m_modifier.m_amount, NULL, NULL, true, NULL);
            }
    }
}

void Aura::HandleDamagePercentTaken(bool apply, bool Real)
{
    m_isPeriodic = apply;

    Unit* target = GetTarget();

    if (!Real)
    {
        return;
    }

    // For prevent double apply bonuses
    bool loading = (target->GetTypeId() == TYPEID_PLAYER && ((Player*)target)->GetSession()->PlayerLoading());

    if (apply)
    {
        if (loading)
        {
            return;
        }

        // Hand of Salvation (only it have this aura and mask)
        if (GetSpellProto()->IsFitToFamily(SPELLFAMILY_PALADIN, UI64LIT(0x0000000000000100)))
        {
            // Glyph of Salvation
            if (target->GetObjectGuid() == GetCasterGuid())
                if (Aura* aur = target->GetAura(63225, EFFECT_INDEX_0))
                {
                    m_modifier.m_amount -= aur->GetModifier()->m_amount;
                }
        }
    }
}

/**
 * @brief Enables periodic damage and precalculates damage bonuses when applied.
 *
 * @param apply True to enable periodic damage; false to disable it.
 * @param Real True when processing the real aura state change.
 */
void Aura::HandlePeriodicDamage(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
    {
        return;
    }

    m_isPeriodic = apply;

    Unit* target = GetTarget();
    SpellEntry const* spellProto = GetSpellProto();

    // For prevent double apply bonuses
    bool loading = (target->GetTypeId() == TYPEID_PLAYER && ((Player*)target)->GetSession()->PlayerLoading());

    // Custom damage calculation after
    if (apply)
    {
        if (loading)
        {
            return;
        }

        Unit* caster = GetCaster();
        if (!caster)
        {
            return;
        }

        switch (spellProto->SpellClassSet)
        {
            case SPELLFAMILY_WARRIOR:
            {
                // Rend
                if (spellProto->SpellClassMask & UI64LIT(0x0000000000000020))
                {
                    // $0.2*(($MWB+$mwb)/2+$AP/14*$MWS) bonus per tick
                    float ap = caster->GetTotalAttackPowerValue(BASE_ATTACK);
                    int32 mws = caster->GetAttackTime(BASE_ATTACK);
                    float mwb_min = caster->GetWeaponDamageRange(BASE_ATTACK, MINDAMAGE);
                    float mwb_max = caster->GetWeaponDamageRange(BASE_ATTACK, MAXDAMAGE);
                    m_modifier.m_amount += int32(((mwb_min + mwb_max) / 2 + ap * mws / 14000) * 0.2f);
                    // If used while target is above 75% health, Rend does 35% more damage
                    if (spellProto->CalculateSimpleValue(EFFECT_INDEX_1) != 0 &&
                            target->GetHealth() > target->GetMaxHealth() * spellProto->CalculateSimpleValue(EFFECT_INDEX_1) / 100)
                        m_modifier.m_amount += m_modifier.m_amount * spellProto->CalculateSimpleValue(EFFECT_INDEX_2) / 100;
                }
                break;
            }
            case SPELLFAMILY_DRUID:
            {
                // Rip
                if (spellProto->SpellClassMask & UI64LIT(0x000000000000800000))
                {
                    if (caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        break;
                    }

                    // 0.01*$AP*cp
                    uint8 cp = ((Player*)caster)->GetComboPoints();

                    // Idol of Feral Shadows. Cant be handled as SpellMod in SpellAura:Dummy due its dependency from CPs
                    Unit::AuraList const& dummyAuras = caster->GetAurasByType(SPELL_AURA_DUMMY);
                    for (Unit::AuraList::const_iterator itr = dummyAuras.begin(); itr != dummyAuras.end(); ++itr)
                    {
                        if ((*itr)->GetId() == 34241)
                        {
                            m_modifier.m_amount += cp * (*itr)->GetModifier()->m_amount;
                            break;
                        }
                    }
                    m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * cp / 100);
                }
                break;
            }
            case SPELLFAMILY_ROGUE:
            {
                // Rupture
                if (spellProto->SpellClassMask & UI64LIT(0x000000000000100000))
                {
                    if (caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        break;
                    }
                    // 1 point : ${($m1+$b1*1+0.015*$AP)*4} damage over 8 secs
                    // 2 points: ${($m1+$b1*2+0.024*$AP)*5} damage over 10 secs
                    // 3 points: ${($m1+$b1*3+0.03*$AP)*6} damage over 12 secs
                    // 4 points: ${($m1+$b1*4+0.03428571*$AP)*7} damage over 14 secs
                    // 5 points: ${($m1+$b1*5+0.0375*$AP)*8} damage over 16 secs
                    float AP_per_combo[6] = {0.0f, 0.015f, 0.024f, 0.03f, 0.03428571f, 0.0375f};
                    uint8 cp = ((Player*)caster)->GetComboPoints();
                    if (cp > 5) cp = 5;
                    {
                        m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * AP_per_combo[cp]);
                    }
                }
                break;
            }
            case SPELLFAMILY_PALADIN:
            {
                // Holy Vengeance / Blood Corruption
                if (spellProto->SpellClassMask & UI64LIT(0x0000080000000000) && spellProto->SpellVisualID[0] == 7902)
                {
                    // AP * 0.025 + SPH * 0.013 bonus per tick
                    float ap = caster->GetTotalAttackPowerValue(BASE_ATTACK);
                    int32 holy = caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(spellProto));
                    if (holy < 0)
                    {
                        holy = 0;
                    }
                    m_modifier.m_amount += int32(GetStackAmount()) * (int32(ap * 0.025f) + int32(holy * 13 / 1000));
                }
                break;
            }
            default:
                break;
        }

        if (m_modifier.m_auraname == SPELL_AURA_PERIODIC_DAMAGE)
        {
            // SpellDamageBonusDone for magic spells
            if (spellProto->DefenseType == SPELL_DAMAGE_CLASS_NONE || spellProto->DefenseType == SPELL_DAMAGE_CLASS_MAGIC)
            {
                m_modifier.m_amount = caster->SpellDamageBonusDone(target, GetSpellProto(), m_modifier.m_amount, DOT, GetStackAmount());
            }
            // MeleeDamagebonusDone for weapon based spells
            else
            {
                WeaponAttackType attackType = GetWeaponAttackType(GetSpellProto());
                m_modifier.m_amount = caster->MeleeDamageBonusDone(target, m_modifier.m_amount, attackType, GetSpellProto(), DOT, GetStackAmount());
            }
        }
    }
    // remove time effects
    else
    {
        // Parasitic Shadowfiend - handle summoning of two Shadowfiends on DoT expire
        if (spellProto->ID == 41917)
        {
            target->CastSpell(target, 41915, true);
        }
    }
}

/**
 * @brief Enables or disables periodic percentage-based damage processing.
 *
 * @param apply True to enable periodic processing; false to disable it.
 * @param Real Unused.
 */
void Aura::HandlePeriodicDamagePCT(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
}

/**
 * @brief Enables periodic health leech and precalculates spell bonuses when applied.
 *
 * @param apply True to enable periodic leeching; false to disable it.
 * @param Real Unused.
 */
void Aura::HandlePeriodicLeech(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;

    // For prevent double apply bonuses
    bool loading = (GetTarget()->GetTypeId() == TYPEID_PLAYER && ((Player*)GetTarget())->GetSession()->PlayerLoading());

    // Custom damage calculation after
    if (apply)
    {
        if (loading)
        {
            return;
        }

        Unit* caster = GetCaster();
        if (!caster)
        {
            return;
        }

        m_modifier.m_amount = caster->SpellDamageBonusDone(GetTarget(), GetSpellProto(), m_modifier.m_amount, DOT, GetStackAmount());
    }
}

/**
 * @brief Enables or disables periodic mana leech processing.
 *
 * @param apply True to enable periodic processing; false to disable it.
 * @param Real Unused.
 */
void Aura::HandlePeriodicManaLeech(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
}

/**
 * @brief Enables periodic health funnel processing and precalculates bonuses when applied.
 *
 * @param apply True to enable periodic processing; false to disable it.
 * @param Real Unused.
 */
void Aura::HandlePeriodicHealthFunnel(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;

    // For prevent double apply bonuses
    bool loading = (GetTarget()->GetTypeId() == TYPEID_PLAYER && ((Player*)GetTarget())->GetSession()->PlayerLoading());

    // Custom damage calculation after
    if (apply)
    {
        if (loading)
        {
            return;
        }

        Unit* caster = GetCaster();
        if (!caster)
        {
            return;
        }

        m_modifier.m_amount = caster->SpellDamageBonusDone(GetTarget(), GetSpellProto(), m_modifier.m_amount, DOT, GetStackAmount());
    }
}

void Aura::HandleAuraModResistanceExclusive(bool apply, bool /*Real*/)
{
    for (int8 x = SPELL_SCHOOL_NORMAL; x < MAX_SPELL_SCHOOL; ++x)
    {
        if (m_modifier.m_miscvalue & int32(1 << x))
        {
            GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), BASE_VALUE, float(m_modifier.m_amount), apply);
            if (GetTarget()->GetTypeId() == TYPEID_PLAYER)
            {
                ((Player*)GetTarget())->ApplyResistanceBuffModsMod(SpellSchools(x), m_positive, float(m_modifier.m_amount), apply);
            }
        }
    }
}

/**
 * @brief Applies or removes flat resistance modifiers.
 *
 * @param apply True to apply the modifier; false to remove it.
 * @param Real Unused.
 */
void Aura::HandleAuraModResistance(bool apply, bool /*Real*/)
{
    for (int8 x = SPELL_SCHOOL_NORMAL; x < MAX_SPELL_SCHOOL; ++x)
    {
        if (m_modifier.m_miscvalue & int32(1 << x))
        {
            GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), TOTAL_VALUE, float(m_modifier.m_amount), apply);
            if (GetTarget()->GetTypeId() == TYPEID_PLAYER || ((Creature*)GetTarget())->IsPet())
            {
                GetTarget()->ApplyResistanceBuffModsMod(SpellSchools(x), m_positive, float(m_modifier.m_amount), apply);
            }
        }
    }
}

/**
 * @brief Applies or removes percentage modifiers to base resistances.
 *
 * @param apply True to apply the modifier; false to remove it.
 * @param Real Unused.
 */
void Aura::HandleAuraModBaseResistancePCT(bool apply, bool /*Real*/)
{
    // only players have base stats
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
    {
        // pets only have base armor
        if (((Creature*)GetTarget())->IsPet() && (m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL))
        {
            GetTarget()->HandleStatModifier(UNIT_MOD_ARMOR, BASE_PCT, float(m_modifier.m_amount), apply);
        }
    }
    else
    {
        for (int8 x = SPELL_SCHOOL_NORMAL; x < MAX_SPELL_SCHOOL; ++x)
        {
            if (m_modifier.m_miscvalue & int32(1 << x))
            {
                GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), BASE_PCT, float(m_modifier.m_amount), apply);
            }
        }
    }
}

/**
 * @brief Applies or removes percentage modifiers to total resistances.
 *
 * @param apply True to apply the modifier; false to remove it.
 * @param Real Unused.
 */
void Aura::HandleModResistancePercent(bool apply, bool /*Real*/)
{
    Unit* target = GetTarget();

    for (int8 i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; ++i)
    {
        if (m_modifier.m_miscvalue & int32(1 << i))
        {
            target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + i), TOTAL_PCT, float(m_modifier.m_amount), apply);
            if (target->GetTypeId() == TYPEID_PLAYER || ((Creature*)target)->IsPet())
            {
                ((Player*)target)->ApplyResistanceBuffModsPercentMod(SpellSchools(i), true, float(m_modifier.m_amount), apply);
                ((Player*)target)->ApplyResistanceBuffModsPercentMod(SpellSchools(i), false, float(m_modifier.m_amount), apply);
            }
        }
    }
}

/**
 * @brief Applies or removes flat modifiers to base resistances.
 *
 * @param apply True to apply the modifier; false to remove it.
 * @param Real Unused.
 */
void Aura::HandleModBaseResistance(bool apply, bool /*Real*/)
{
    // only players have base stats
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
    {
        // only pets have base stats
        if (((Creature*)GetTarget())->IsPet() && (m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL))
        {
            GetTarget()->HandleStatModifier(UNIT_MOD_ARMOR, TOTAL_VALUE, float(m_modifier.m_amount), apply);
        }
    }
    else
    {
        for (int i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; ++i)
        {
            if (m_modifier.m_miscvalue & (1 << i))
            {
                GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + i), TOTAL_VALUE, float(m_modifier.m_amount), apply);
            }
        }
    }
}

void Aura::HandleAuraModStat(bool apply, bool /*Real*/)
{
    if (m_modifier.m_miscvalue < -2 || m_modifier.m_miscvalue > 4)
    {
        sLog.outError("WARNING: Spell %u effect %u have unsupported misc value (%i) for SPELL_AURA_MOD_STAT ", GetId(), GetEffIndex(), m_modifier.m_miscvalue);
        return;
    }

    for (int32 i = STAT_STRENGTH; i < MAX_STATS; ++i)
    {
        // -1 or -2 is all stats ( misc < -2 checked in function beginning )
        if (m_modifier.m_miscvalue < 0 || m_modifier.m_miscvalue == i)
        {
            // m_target->ApplyStatMod(Stats(i), m_modifier.m_amount,apply);
            GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), TOTAL_VALUE, float(m_modifier.m_amount), apply);
            if (GetTarget()->GetTypeId() == TYPEID_PLAYER || ((Creature*)GetTarget())->IsPet())
            {
                ((Player*)GetTarget())->ApplyStatBuffMod(Stats(i), float(m_modifier.m_amount), apply);
            }
        }
    }
}

/**
 * @brief Applies or removes percentage modifiers to player base stats.
 *
 * @param apply True to apply the modifier; false to remove it.
 * @param Real Unused.
 */
void Aura::HandleModPercentStat(bool apply, bool /*Real*/)
{
    if (m_modifier.m_miscvalue < -1 || m_modifier.m_miscvalue > 4)
    {
        sLog.outError("WARNING: Misc Value for SPELL_AURA_MOD_PERCENT_STAT not valid");
        return;
    }

    // only players have base stats
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    for (int32 i = STAT_STRENGTH; i < MAX_STATS; ++i)
    {
        if (m_modifier.m_miscvalue == i || m_modifier.m_miscvalue == -1)
        {
            GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), BASE_PCT, float(m_modifier.m_amount), apply);
        }
    }
}

/**
 * @brief Refreshes player spell damage and healing data derived from stats.
 *
 * @param apply Unused.
 * @param Real Unused.
 */
void Aura::HandleModSpellDamagePercentFromStat(bool /*apply*/, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    // Magic damage modifiers implemented in Unit::SpellDamageBonusDone
    // This information for client side use only
    // Recalculate bonus
    ((Player*)GetTarget())->UpdateSpellDamageAndHealingBonus();
}

/**
 * @brief Refreshes player healing data derived from stats.
 *
 * @param apply Unused.
 * @param Real Unused.
 */
void Aura::HandleModSpellHealingPercentFromStat(bool /*apply*/, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    // Recalculate bonus
    ((Player*)GetTarget())->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleAuraModDispelResist(bool apply, bool Real)
{
    if (!Real || !apply)
    {
        return;
    }

    if (GetId() == 33206)
    {
        GetTarget()->CastSpell(GetTarget(), 44416, true, NULL, this, GetCasterGuid());
    }
}

void Aura::HandleModSpellDamagePercentFromAttackPower(bool /*apply*/, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    // Magic damage modifiers implemented in Unit::SpellDamageBonusDone
    // This information for client side use only
    // Recalculate bonus
    ((Player*)GetTarget())->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleModSpellHealingPercentFromAttackPower(bool /*apply*/, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    // Recalculate bonus
    ((Player*)GetTarget())->UpdateSpellDamageAndHealingBonus();
}

/**
 * @brief Refreshes player healing bonus data exposed to the client.
 *
 * @param apply Unused.
 * @param Real Unused.
 */
void Aura::HandleModHealingDone(bool /*apply*/, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }
    // implemented in Unit::SpellHealingBonusDone
    // this information is for client side only
    ((Player*)GetTarget())->UpdateSpellDamageAndHealingBonus();
}

/**
 * @brief Applies or removes percentage modifiers to total stats and preserves health ratios when required.
 *
 * @param apply True to apply the modifier; false to remove it.
 * @param Real Unused.
 */
void Aura::HandleModTotalPercentStat(bool apply, bool /*Real*/)
{
    if (m_modifier.m_miscvalue < -1 || m_modifier.m_miscvalue > 4)
    {
        sLog.outError("WARNING: Misc Value for SPELL_AURA_MOD_PERCENT_STAT not valid");
        return;
    }

    Unit* target = GetTarget();

    // save current and max HP before applying aura
    uint32 curHPValue = target->GetHealth();
    uint32 maxHPValue = target->GetMaxHealth();

    for (int32 i = STAT_STRENGTH; i < MAX_STATS; ++i)
    {
        if (m_modifier.m_miscvalue == i || m_modifier.m_miscvalue == -1)
        {
            target->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), TOTAL_PCT, float(m_modifier.m_amount), apply);
            if (target->GetTypeId() == TYPEID_PLAYER || ((Creature*)target)->IsPet())
            {
                ((Player*)target)->ApplyStatPercentBuffMod(Stats(i), float(m_modifier.m_amount), apply);
            }
        }
    }

    // recalculate current HP/MP after applying aura modifications (only for spells with 0x10 flag)
    if (m_modifier.m_miscvalue == STAT_STAMINA && maxHPValue > 0 && GetSpellProto()->HasAttribute(SPELL_ATTR_ABILITY))
    {
        // newHP = (curHP / maxHP) * newMaxHP = (newMaxHP * curHP) / maxHP -> which is better because no int -> double -> int conversion is needed
        uint32 newHPValue = (target->GetMaxHealth() * curHPValue) / maxHPValue;
        target->SetHealth(newHPValue);
    }
}

/**
 * @brief Refreshes armor-from-stat style resistance data for players.
 *
 * @param apply Unused.
 * @param Real Unused.
 */
void Aura::HandleAuraModResistenceOfStatPercent(bool /*apply*/, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    if (m_modifier.m_miscvalue != SPELL_SCHOOL_MASK_NORMAL)
    {
        // support required adding replace UpdateArmor by loop by UpdateResistence at intellect update
        // and include in UpdateResistence same code as in UpdateArmor for aura mod apply.
        sLog.outError("Aura SPELL_AURA_MOD_RESISTANCE_OF_STAT_PERCENT(182) need adding support for non-armor resistances!");
        return;
    }

    // Recalculate Armor
    GetTarget()->UpdateArmor();
}

/********************************/
/***      HEAL & ENERGIZE     ***/
/********************************/
void Aura::HandleAuraModTotalHealthPercentRegen(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
}

/**
 * @brief Enables periodic total mana regeneration using a one-second tick when needed.
 *
 * @param apply True to enable the periodic effect; false to disable it.
 * @param Real Unused.
 */
void Aura::HandleAuraModTotalManaPercentRegen(bool apply, bool /*Real*/)
{
    if (m_modifier.periodictime == 0)
    {
        m_modifier.periodictime = 1000;
    }

    m_periodicTimer = m_modifier.periodictime;
    m_isPeriodic = apply;
}

void Aura::HandleModRegen(bool apply, bool /*Real*/)        // eating
{
    if (m_modifier.periodictime == 0)
    {
        m_modifier.periodictime = 5000;
    }

    m_periodicTimer = 5000;
    m_isPeriodic = apply;
}

void Aura::HandleModPowerRegen(bool apply, bool Real)       // drinking
{
    if (!Real)
    {
        return;
    }

    Powers powerType = GetTarget()->GetPowerType();
    if (m_modifier.periodictime == 0)
    {
        // Anger Management (only spell use this aura for rage)
        if (powerType == POWER_RAGE)
        {
            m_modifier.periodictime = 3000;
        }
        else
        {
            m_modifier.periodictime = 2000;
        }
    }

    m_periodicTimer = 5000;

    if (GetTarget()->GetTypeId() == TYPEID_PLAYER && m_modifier.m_miscvalue == POWER_MANA)
    {
        ((Player*)GetTarget())->UpdateManaRegen();
    }

    m_isPeriodic = apply;
}

/**
 * @brief Refreshes player mana regeneration after percentage-based regen changes.
 *
 * @param apply Unused.
 * @param Real True when processing the real aura state change.
 */
void Aura::HandleModPowerRegenPCT(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
    {
        return;
    }

    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    // Update manaregen value
    if (m_modifier.m_miscvalue == POWER_MANA)
    {
        ((Player*)GetTarget())->UpdateManaRegen();
    }
}

void Aura::HandleModManaRegen(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
    {
        return;
    }

    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    // Note: an increase in regen does NOT cause threat.
    ((Player*)GetTarget())->UpdateManaRegen();
}

void Aura::HandleComprehendLanguage(bool apply, bool /*Real*/)
{
    if (apply)
    {
        GetTarget()->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_COMPREHEND_LANG);
    }
    else
    {
        GetTarget()->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_COMPREHEND_LANG);
    }
}

/**
 * @brief Applies or removes maximum health increases, including special temporary-health cases.
 *
 * @param apply True to apply the modifier; false to remove it.
 * @param Real True when processing the real aura state change.
 */
void Aura::HandleAuraModIncreaseHealth(bool apply, bool Real)
{
    Unit* target = GetTarget();

    switch (GetId())
    {
            // Special case with temporary increase max/current health
            // Cases where we need to manually calculate the amount for the spell (by percentage)
            // recalculate to full amount at apply for proper remove
        case 54443:                                         // Demonic Empowerment (Voidwalker)
        case 55233:                                         // Vampiric Blood
        case 61254:                                         // Will of Sartharion (Obsidian Sanctum)
            if (Real && apply)
            {
                m_modifier.m_amount = target->GetMaxHealth() * m_modifier.m_amount / 100;
            }
            // no break here

            // Cases where m_amount already has the correct value (spells cast with CastCustomSpell or absolute values)
        case 12976:                                         // Warrior Last Stand triggered spell (Cast with percentage-value by CastCustomSpell)
        case 28726:                                         // Nightmare Seed
        case 31616:                                         // Nature's Guardian (Cast with percentage-value by CastCustomSpell)
        case 34511:                                         // Valor (Bulwark of Kings, Bulwark of the Ancient Kings)
        case 44055: case 55915: case 55917: case 67596:     // Tremendous Fortitude (Battlemaster's Alacrity)
        case 50322:                                         // Survival Instincts (Cast with percentage-value by CastCustomSpell)
        case 53479:                                         // Hunter pet - Last Stand (Cast with percentage-value by CastCustomSpell)
        case 59465:                                         // Brood Rage (Ahn'Kahet)
        {
            if (Real)
            {
                if (apply)
                {
                    target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(m_modifier.m_amount), apply);
                    target->ModifyHealth(m_modifier.m_amount);
                }
                else
                {
                    if (int32(target->GetHealth()) > m_modifier.m_amount)
                    {
                        target->ModifyHealth(-m_modifier.m_amount);
                    }
                    else
                    {
                        target->SetHealth(1);
                    }
                    target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(m_modifier.m_amount), apply);
                }
            }
            return;
        }
        // generic case
        default:
            target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(m_modifier.m_amount), apply);
    }
}

void  Aura::HandleAuraModIncreaseMaxHealth(bool apply, bool /*Real*/)
{
    Unit* target = GetTarget();
    uint32 oldhealth = target->GetHealth();
    double healthPercentage = (double)oldhealth / (double)target->GetMaxHealth();

    target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(m_modifier.m_amount), apply);

    // refresh percentage
    if (oldhealth > 0)
    {
        uint32 newhealth = uint32(ceil((double)target->GetMaxHealth() * healthPercentage));
        if (newhealth == 0)
        {
            newhealth = 1;
        }

        target->SetHealth(newhealth);
    }
}

/**
 * @brief Applies or removes a flat increase to the current power type's maximum value.
 *
 * @param apply True to apply the modifier; false to remove it.
 * @param Real Unused.
 */
void Aura::HandleAuraModIncreaseEnergy(bool apply, bool Real)
{
    Unit* target = GetTarget();
    Powers powerType = target->GetPowerType();
    if (int32(powerType) != m_modifier.m_miscvalue)
    {
        return;
    }

    UnitMods unitMod = UnitMods(UNIT_MOD_POWER_START + powerType);

    // Special case with temporary increase max/current power (percent)
    if (GetId() == 64904)                                   // Hymn of Hope
    {
        if (Real)
        {
            uint32 val = target->GetPower(powerType);
            target->HandleStatModifier(unitMod, TOTAL_PCT, float(m_modifier.m_amount), apply);
            target->SetPower(powerType, apply ? val * (100 + m_modifier.m_amount) / 100 : val * 100 / (100 + m_modifier.m_amount));
        }
        return;
    }

    // generic flat case
    target->HandleStatModifier(unitMod, TOTAL_VALUE, float(m_modifier.m_amount), apply);
}

/**
 * @brief Applies or removes a percentage increase to the current power type.
 *
 * @param apply True to apply the modifier; false to remove it.
 * @param Real Unused.
 */
void Aura::HandleAuraModIncreaseEnergyPercent(bool apply, bool /*Real*/)
{
    Powers powerType = GetTarget()->GetPowerType();
    if (int32(powerType) != m_modifier.m_miscvalue)
    {
        return;
    }

    UnitMods unitMod = UnitMods(UNIT_MOD_POWER_START + powerType);

    GetTarget()->HandleStatModifier(unitMod, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

/**
 * @brief Applies or removes a percentage increase to maximum health.
 *
 * @param apply True to apply the modifier; false to remove it.
 * @param Real Unused.
 */
void Aura::HandleAuraModIncreaseHealthPercent(bool apply, bool /*Real*/)
{
    Unit* target = GetTarget();

    target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_PCT, float(m_modifier.m_amount), apply);

    // spell special cases when current health set to max value at apply
    switch (GetId())
    {
        case 60430:                                         // Molten Fury
        case 64193:                                         // Heartbreak
        case 65737:                                         // Heartbreak
            target->SetHealth(target->GetMaxHealth());
            break;
        default:
            break;
    }
}

void Aura::HandleAuraIncreaseBaseHealthPercent(bool apply, bool /*Real*/)
{
    GetTarget()->HandleStatModifier(UNIT_MOD_HEALTH, BASE_PCT, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModParryPercent(bool /*apply*/, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    ((Player*)GetTarget())->UpdateParryPercentage();
}

/**
 * @brief Refreshes player dodge percentage after aura changes.
 *
 * @param apply Unused.
 * @param Real Unused.
 */
void Aura::HandleAuraModDodgePercent(bool /*apply*/, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    ((Player*)GetTarget())->UpdateDodgePercentage();
    // sLog.outError("BONUS DODGE CHANCE: + %f", float(m_modifier.m_amount));
}

/**
 * @brief Refreshes player mana regeneration rules after regen-interrupt changes.
 *
 * @param apply Unused.
 * @param Real True when processing the real aura state change.
 */
void Aura::HandleAuraModRegenInterrupt(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
    {
        return;
    }

    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    ((Player*)GetTarget())->UpdateManaRegen();
}

/**
 * @brief Applies or removes melee and ranged critical strike bonuses.
 *
 * @param apply True to apply the modifier; false to remove it.
 * @param Real True when processing the real aura state change.
 */
void Aura::HandleAuraModCritPercent(bool apply, bool Real)
{
    Unit* target = GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    // apply item specific bonuses for already equipped weapon
    if (Real)
    {
        for (int i = 0; i < MAX_ATTACK; ++i)
        {
            if (Item* pItem = ((Player*)target)->GetWeaponForAttack(WeaponAttackType(i), true, false))
            {
                ((Player*)target)->_ApplyWeaponDependentAuraCritMod(pItem, WeaponAttackType(i), this, apply);
            }
        }
    }

    // mods must be applied base at equipped weapon class and subclass comparison
    // with spell->EquippedItemClass and  EquippedItemSubClassMask and EquippedItemInventoryTypeMask
    // m_modifier.m_miscvalue comparison with item generated damage types

    if (GetSpellProto()->EquippedItemClass == -1)
    {
        ((Player*)target)->HandleBaseModValue(CRIT_PERCENTAGE,         FLAT_MOD, float(m_modifier.m_amount), apply);
        ((Player*)target)->HandleBaseModValue(OFFHAND_CRIT_PERCENTAGE, FLAT_MOD, float(m_modifier.m_amount), apply);
        ((Player*)target)->HandleBaseModValue(RANGED_CRIT_PERCENTAGE,  FLAT_MOD, float(m_modifier.m_amount), apply);
    }
    else
    {
        // done in Player::_ApplyWeaponDependentAuraMods
    }
}

/**
 * @brief Applies or removes melee and ranged hit chance modifiers.
 *
 * @param apply True to apply the modifier; false to remove it.
 * @param Real Unused.
 */
void Aura::HandleModHitChance(bool apply, bool /*Real*/)
{
    Unit* target = GetTarget();

    if (target->GetTypeId() == TYPEID_PLAYER)
    {
        ((Player*)target)->UpdateMeleeHitChances();
        ((Player*)target)->UpdateRangedHitChances();
    }
    else
    {
        target->m_modMeleeHitChance += apply ? m_modifier.m_amount : (-m_modifier.m_amount);
        target->m_modRangedHitChance += apply ? m_modifier.m_amount : (-m_modifier.m_amount);
    }
}

/**
 * @brief Applies or removes spell hit chance modifiers.
 *
 * @param apply True to apply the modifier; false to remove it.
 * @param Real Unused.
 */
void Aura::HandleModSpellHitChance(bool apply, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() == TYPEID_PLAYER)
    {
        ((Player*)GetTarget())->UpdateSpellHitChances();
    }
    else
    {
        GetTarget()->m_modSpellHitChance += apply ? m_modifier.m_amount : (-m_modifier.m_amount);
    }
}

/**
 * @brief Applies or removes spell critical strike chance bonuses.
 *
 * @param apply True to apply the modifier; false to remove it.
 * @param Real True when processing the real aura state change.
 */
void Aura::HandleModSpellCritChance(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
    {
        return;
    }

    if (GetTarget()->GetTypeId() == TYPEID_PLAYER)
    {
        ((Player*)GetTarget())->UpdateAllSpellCritChances();
    }
    else
    {
        GetTarget()->m_baseSpellCritChance += apply ? m_modifier.m_amount : (-m_modifier.m_amount);
    }
}

/**
 * @brief Refreshes per-school spell critical strike chance for players.
 *
 * @param apply Unused.
 * @param Real True when processing the real aura state change.
 */
void Aura::HandleModSpellCritChanceShool(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
    {
        return;
    }

    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    for (int school = SPELL_SCHOOL_NORMAL; school < MAX_SPELL_SCHOOL; ++school)
    {
        if (m_modifier.m_miscvalue & (1 << school))
        {
            ((Player*)GetTarget())->UpdateSpellCritChance(school);
        }
    }
}

void Aura::HandleModCastingSpeed(bool apply, bool /*Real*/)
{
    GetTarget()->ApplyCastTimePercentMod(float(m_modifier.m_amount), apply);
}

void Aura::HandleModMeleeRangedSpeedPct(bool apply, bool /*Real*/)
{
    Unit* target = GetTarget();
    target->ApplyAttackTimePercentMod(BASE_ATTACK, float(m_modifier.m_amount), apply);
    target->ApplyAttackTimePercentMod(OFF_ATTACK, float(m_modifier.m_amount), apply);
    target->ApplyAttackTimePercentMod(RANGED_ATTACK, float(m_modifier.m_amount), apply);
}

void Aura::HandleModCombatSpeedPct(bool apply, bool /*Real*/)
{
    Unit* target = GetTarget();
    target->ApplyCastTimePercentMod(float(m_modifier.m_amount), apply);
    target->ApplyAttackTimePercentMod(BASE_ATTACK, float(m_modifier.m_amount), apply);
    target->ApplyAttackTimePercentMod(OFF_ATTACK, float(m_modifier.m_amount), apply);
    target->ApplyAttackTimePercentMod(RANGED_ATTACK, float(m_modifier.m_amount), apply);
}

/**
 * @brief Applies or removes haste effects to main-hand attack speed.
 *
 * @param apply True to apply the modifier; false to remove it.
 * @param Real Unused.
 */
void Aura::HandleModAttackSpeed(bool apply, bool /*Real*/)
{
    GetTarget()->ApplyAttackTimePercentMod(BASE_ATTACK, float(m_modifier.m_amount), apply);
}

/**
 * @brief Applies or removes haste effects to both melee attack speeds.
 *
 * @param apply True to apply the modifier; false to remove it.
 * @param Real Unused.
 */
void Aura::HandleModMeleeSpeedPct(bool apply, bool /*Real*/)
{
    Unit* target = GetTarget();
    target->ApplyAttackTimePercentMod(BASE_ATTACK, float(m_modifier.m_amount), apply);
    target->ApplyAttackTimePercentMod(OFF_ATTACK, float(m_modifier.m_amount), apply);
}

/**
 * @brief Applies or removes haste effects to ranged attack speed.
 *
 * @param apply True to apply the modifier; false to remove it.
 * @param Real Unused.
 */
void Aura::HandleAuraModRangedHaste(bool apply, bool /*Real*/)
{
    GetTarget()->ApplyAttackTimePercentMod(RANGED_ATTACK, float(m_modifier.m_amount), apply);
}

/**
 * @brief Applies or removes ammo-based ranged haste when the equipped weapon uses ammunition.
 *
 * @param apply True to apply the modifier; false to remove it.
 * @param Real Unused.
 */
void Aura::HandleRangedAmmoHaste(bool apply, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }
    GetTarget()->ApplyAttackTimePercentMod(RANGED_ATTACK, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModAttackPower(bool apply, bool /*Real*/)
{
    GetTarget()->HandleStatModifier(UNIT_MOD_ATTACK_POWER, TOTAL_VALUE, float(m_modifier.m_amount), apply);
}

/**
 * @brief Applies or removes flat ranged attack power bonuses.
 *
 * @param apply True to apply the modifier; false to remove it.
 * @param Real Unused.
 */
void Aura::HandleAuraModRangedAttackPower(bool apply, bool /*Real*/)
{
    if ((GetTarget()->getClassMask() & CLASSMASK_WAND_USERS) != 0)
    {
        return;
    }

    GetTarget()->HandleStatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_VALUE, float(m_modifier.m_amount), apply);
}

/**
 * @brief Applies or removes percentage modifiers to melee attack power.
 *
 * @param apply True to apply the modifier; false to remove it.
 * @param Real Unused.
 */
void Aura::HandleAuraModAttackPowerPercent(bool apply, bool /*Real*/)
{
    // UNIT_FIELD_ATTACK_POWER_MULTIPLIER = multiplier - 1
    GetTarget()->HandleStatModifier(UNIT_MOD_ATTACK_POWER, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

/**
 * @brief Applies or removes percentage modifiers to ranged attack power.
 *
 * @param apply True to apply the modifier; false to remove it.
 * @param Real Unused.
 */
void Aura::HandleAuraModRangedAttackPowerPercent(bool apply, bool /*Real*/)
{
    if ((GetTarget()->getClassMask() & CLASSMASK_WAND_USERS) != 0)
    {
        return;
    }

    // UNIT_FIELD_RANGED_ATTACK_POWER_MULTIPLIER = multiplier - 1
    GetTarget()->HandleStatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModAttackPowerOfArmor(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
    {
        return;
    }

    // Recalculate bonus
    if (GetTarget()->GetTypeId() == TYPEID_PLAYER)
    {
        ((Player*)GetTarget())->UpdateAttackPowerAndDamage(false);
    }
}

/********************************/
/***        DAMAGE BONUS      ***/
/********************************/
void Aura::HandleModDamageDone(bool apply, bool Real)
{
    Unit* target = GetTarget();

    // apply item specific bonuses for already equipped weapon
    if (Real && target->GetTypeId() == TYPEID_PLAYER)
    {
        for (int i = 0; i < MAX_ATTACK; ++i)
        {
            if (Item* pItem = ((Player*)target)->GetWeaponForAttack(WeaponAttackType(i), true, false))
            {
                ((Player*)target)->_ApplyWeaponDependentAuraDamageMod(pItem, WeaponAttackType(i), this, apply);
            }
        }
    }

    // m_modifier.m_miscvalue is bitmask of spell schools
    // 1 ( 0-bit ) - normal school damage (SPELL_SCHOOL_MASK_NORMAL)
    // 126 - full bitmask all magic damages (SPELL_SCHOOL_MASK_MAGIC) including wands
    // 127 - full bitmask any damages
    //
    // mods must be applied base at equipped weapon class and subclass comparison
    // with spell->EquippedItemClass and  EquippedItemSubClassMask and EquippedItemInventoryTypeMask
    // m_modifier.m_miscvalue comparison with item generated damage types

    if ((m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL) != 0)
    {
        // apply generic physical damage bonuses including wand case
        if (GetSpellProto()->EquippedItemClass == -1 || target->GetTypeId() != TYPEID_PLAYER)
        {
            target->HandleStatModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_VALUE, float(m_modifier.m_amount), apply);
            target->HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_VALUE, float(m_modifier.m_amount), apply);
            target->HandleStatModifier(UNIT_MOD_DAMAGE_RANGED, TOTAL_VALUE, float(m_modifier.m_amount), apply);
        }
        else
        {
            // done in Player::_ApplyWeaponDependentAuraMods
        }

        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            if (m_positive)
            {
                target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS, m_modifier.m_amount, apply);
            }
            else
            {
                target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG, m_modifier.m_amount, apply);
            }
        }
    }

    // Skip non magic case for speedup
    if ((m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_MAGIC) == 0)
    {
        return;
    }

    if (GetSpellProto()->EquippedItemClass != -1 || GetSpellProto()->EquippedItemInvTypes != 0)
    {
        // wand magic case (skip generic to all item spell bonuses)
        // done in Player::_ApplyWeaponDependentAuraMods

        // Skip item specific requirements for not wand magic damage
        return;
    }

    // Magic damage modifiers implemented in Unit::SpellDamageBonusDone
    // This information for client side use only
    if (target->GetTypeId() == TYPEID_PLAYER)
    {
        if (m_positive)
        {
            for (int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
            {
                if ((m_modifier.m_miscvalue & (1 << i)) != 0)
                {
                    target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + i, m_modifier.m_amount, apply);
                }
            }
        }
        else
        {
            for (int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
            {
                if ((m_modifier.m_miscvalue & (1 << i)) != 0)
                {
                    target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG + i, m_modifier.m_amount, apply);
                }
            }
        }
        Pet* pet = target->GetPet();
        if (pet)
        {
            pet->UpdateAttackPowerAndDamage();
        }
    }
}

/**
 * @brief Applies or removes percentage damage bonuses for physical and magical damage.
 *
 * @param apply True to apply the modifier; false to remove it.
 * @param Real True when processing the real aura state change.
 */
void Aura::HandleModDamagePercentDone(bool apply, bool Real)
{
    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "AURA MOD DAMAGE type:%u negative:%u", m_modifier.m_miscvalue, m_positive ? 0 : 1);
    Unit* target = GetTarget();

    // apply item specific bonuses for already equipped weapon
    if (Real && target->GetTypeId() == TYPEID_PLAYER)
    {
        for (int i = 0; i < MAX_ATTACK; ++i)
        {
            if (Item* pItem = ((Player*)target)->GetWeaponForAttack(WeaponAttackType(i), true, false))
            {
                ((Player*)target)->_ApplyWeaponDependentAuraDamageMod(pItem, WeaponAttackType(i), this, apply);
            }
        }
    }

    // m_modifier.m_miscvalue is bitmask of spell schools
    // 1 ( 0-bit ) - normal school damage (SPELL_SCHOOL_MASK_NORMAL)
    // 126 - full bitmask all magic damages (SPELL_SCHOOL_MASK_MAGIC) including wand
    // 127 - full bitmask any damages
    //
    // mods must be applied base at equipped weapon class and subclass comparison
    // with spell->EquippedItemClass and  EquippedItemSubClassMask and EquippedItemInventoryTypeMask
    // m_modifier.m_miscvalue comparison with item generated damage types

    if ((m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL) != 0)
    {
        // apply generic physical damage bonuses including wand case
        if (GetSpellProto()->EquippedItemClass == -1 || target->GetTypeId() != TYPEID_PLAYER)
        {
            target->HandleStatModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_PCT, float(m_modifier.m_amount), apply);
            target->HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_PCT, float(m_modifier.m_amount), apply);
            target->HandleStatModifier(UNIT_MOD_DAMAGE_RANGED, TOTAL_PCT, float(m_modifier.m_amount), apply);
        }
        else
        {
            // done in Player::_ApplyWeaponDependentAuraMods
        }
        // For show in client
        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            target->ApplyModSignedFloatValue(PLAYER_FIELD_MOD_DAMAGE_DONE_PCT, m_modifier.m_amount / 100.0f, apply);
        }
    }

    // Skip non magic case for speedup
    if ((m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_MAGIC) == 0)
    {
        return;
    }

    if (GetSpellProto()->EquippedItemClass != -1 || GetSpellProto()->EquippedItemInvTypes != 0)
    {
        // wand magic case (skip generic to all item spell bonuses)
        // done in Player::_ApplyWeaponDependentAuraMods

        // Skip item specific requirements for not wand magic damage
        return;
    }

    // Magic damage percent modifiers implemented in Unit::SpellDamageBonusDone
    // Send info to client
    if (target->GetTypeId() == TYPEID_PLAYER)
        for (int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
        {
            target->ApplyModSignedFloatValue(PLAYER_FIELD_MOD_DAMAGE_DONE_PCT + i, m_modifier.m_amount / 100.0f, apply);
        }
}

/**
 * @brief Applies or removes percentage damage bonuses to offhand attacks.
 *
 * @param apply True to apply the modifier; false to remove it.
 * @param Real True when processing the real aura state change.
 */
void Aura::HandleModOffhandDamagePercent(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
    {
        return;
    }

    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "AURA MOD OFFHAND DAMAGE");

    GetTarget()->HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

void Aura::HandleModPowerCostPCT(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
    {
        return;
    }

    float amount = m_modifier.m_amount / 100.0f;
    for (int i = 0; i < MAX_SPELL_SCHOOL; ++i)
    {
        if (m_modifier.m_miscvalue & (1 << i))
        {
            GetTarget()->ApplyModSignedFloatValue(UNIT_FIELD_POWER_COST_MULTIPLIER + i, amount, apply);
        }
    }
}

/**
 * @brief Applies or removes flat spell power cost modifiers by school.
 *
 * @param apply True to apply the modifier; false to remove it.
 * @param Real True when processing the real aura state change.
 */
void Aura::HandleModPowerCost(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
    {
        return;
    }

    for (int i = 0; i < MAX_SPELL_SCHOOL; ++i)
    {
        if (m_modifier.m_miscvalue & (1 << i))
        {
            GetTarget()->ApplyModInt32Value(UNIT_FIELD_POWER_COST_MODIFIER + i, m_modifier.m_amount, apply);
        }
    }
}

void Aura::HandleNoReagentUseAura(bool /*Apply*/, bool Real)
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

    ClassFamilyMask mask;
    Unit::AuraList const& noReagent = target->GetAurasByType(SPELL_AURA_NO_REAGENT_USE);
    for (Unit::AuraList::const_iterator i = noReagent.begin(); i !=  noReagent.end(); ++i)
    {
        mask |= (*i)->GetAuraSpellClassMask();
    }

    target->SetUInt64Value(PLAYER_NO_REAGENT_COST_1 + 0, mask.Flags);
    target->SetUInt32Value(PLAYER_NO_REAGENT_COST_1 + 2, mask.Flags2);
}

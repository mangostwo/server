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
 * @file SpellAuraMisc.cpp
 * @brief Cohesion split of SpellAuras.cpp -- shapeshift-boost/absorb/misc aura handlers.
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

void Aura::HandleShapeshiftBoosts(bool apply)
{
    uint32 spellId1 = 0;
    uint32 spellId2 = 0;
    uint32 HotWSpellId = 0;
    uint32 MasterShaperSpellId = 0;

    ShapeshiftForm form = ShapeshiftForm(GetModifier()->m_miscvalue);

    Unit* target = GetTarget();

    switch (form)
    {
        case FORM_CAT:
            spellId1 = 3025;
            HotWSpellId = 24900;
            MasterShaperSpellId = 48420;
            break;
        case FORM_TREE:
            spellId1 = 5420;
            spellId2 = 34123;
            MasterShaperSpellId = 48422;
            break;
        case FORM_TRAVEL:
            spellId1 = 5419;
            break;
        case FORM_AQUA:
            spellId1 = 5421;
            break;
        case FORM_BEAR:
            spellId1 = 1178;
            spellId2 = 21178;
            HotWSpellId = 24899;
            MasterShaperSpellId = 48418;
            break;
        case FORM_DIREBEAR:
            spellId1 = 9635;
            spellId2 = 21178;
            HotWSpellId = 24899;
            MasterShaperSpellId = 48418;
            break;
        case FORM_BATTLESTANCE:
            spellId1 = 21156;
            break;
        case FORM_DEFENSIVESTANCE:
            spellId1 = 7376;
            break;
        case FORM_BERSERKERSTANCE:
            spellId1 = 7381;
            break;
        case FORM_MOONKIN:
            spellId1 = 24905;
            spellId2 = 69366;
            MasterShaperSpellId = 48421;
            break;
        case FORM_FLIGHT:
            spellId1 = 33948;
            spellId2 = 34764;
            break;
        case FORM_FLIGHT_EPIC:
            spellId1 = 40122;
            spellId2 = 40121;
            break;
        case FORM_METAMORPHOSIS:
            spellId1 = 54817;
            spellId2 = 54879;
            break;
        case FORM_SPIRITOFREDEMPTION:
            spellId1 = 27792;
            spellId2 = 27795;                               // must be second, this important at aura remove to prevent to early iterator invalidation.
            break;
        case FORM_SHADOW:
            spellId1 = 49868;

            if (target->GetTypeId() == TYPEID_PLAYER)     // Spell 49868 have same category as main form spell and share cooldown
            {
                ((Player*)target)->RemoveSpellCooldown(49868);
            }
            break;
        case FORM_GHOSTWOLF:
            spellId1 = 67116;
            break;
        case FORM_AMBIENT:
        case FORM_GHOUL:
        case FORM_STEALTH:
        case FORM_CREATURECAT:
        case FORM_CREATUREBEAR:
        case FORM_STEVES_GHOUL:
        case FORM_THARONJA_SKELETON:
        case FORM_TEST_OF_STRENGTH:
        case FORM_BLB_PLAYER:
        case FORM_SHADOW_DANCE:
        case FORM_TEST:
        case FORM_ZOMBIE:
        case FORM_UNDEAD:
        case FORM_FRENZY:
        case FORM_NONE:
            break;
    }

    if (apply)
    {
        if (spellId1)
        {
            target->CastSpell(target, spellId1, true, NULL, this);
        }
        if (spellId2)
        {
            target->CastSpell(target, spellId2, true, NULL, this);
        }

        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            const PlayerSpellMap& sp_list = ((Player*)target)->GetSpellMap();
            for (PlayerSpellMap::const_iterator itr = sp_list.begin(); itr != sp_list.end(); ++itr)
            {
                if (itr->second.state == PLAYERSPELL_REMOVED)
                {
                    continue;
                }
                if (itr->first == spellId1 || itr->first == spellId2)
                {
                    continue;
                }
                SpellEntry const* spellInfo = sSpellStore.LookupEntry(itr->first);
                if (!spellInfo || !IsNeedCastSpellAtFormApply(spellInfo, form))
                {
                    continue;
                }
                target->CastSpell(target, itr->first, true, NULL, this);
            }
            // remove auras that do not require shapeshift, but are not active in this specific form (like Improved Barkskin)
            Unit::SpellAuraHolderMap& tAuras = target->GetSpellAuraHolderMap();
            for (Unit::SpellAuraHolderMap::iterator itr = tAuras.begin(); itr != tAuras.end();)
            {
                SpellEntry const* spellInfo = itr->second->GetSpellProto();
                if (itr->second->IsPassive() && spellInfo->HasAttribute(SPELL_ATTR_EX2_NOT_NEED_SHAPESHIFT)
                        && (spellInfo->StancesNot & (1 << (form - 1))))
                {
                    target->RemoveAurasDueToSpell(itr->second->GetId());
                    itr = tAuras.begin();
                }
                else
                {
                    ++itr;
                }
            }

            // Master Shapeshifter
            if (MasterShaperSpellId)
            {
                Unit::AuraList const& ShapeShifterAuras = target->GetAurasByType(SPELL_AURA_DUMMY);
                for (Unit::AuraList::const_iterator i = ShapeShifterAuras.begin(); i != ShapeShifterAuras.end(); ++i)
                {
                    if ((*i)->GetSpellProto()->SpellIconID == 2851)
                    {
                        int32 ShiftMod = (*i)->GetModifier()->m_amount;
                        target->CastCustomSpell(target, MasterShaperSpellId, &ShiftMod, NULL, NULL, true);
                        break;
                    }
                }
            }

            // Leader of the Pack
            if (((Player*)target)->HasSpell(17007))
            {
                SpellEntry const* spellInfo = sSpellStore.LookupEntry(24932);
                if (spellInfo && spellInfo->Stances & (1 << (form - 1)))
                {
                    target->CastSpell(target, 24932, true, NULL, this);
                }
            }

            // Savage Roar
            if (form == FORM_CAT && ((Player*)target)->HasAura(52610))
            {
                target->CastSpell(target, 62071, true);
            }

            // Survival of the Fittest (Armor part)
            if (form == FORM_BEAR || form == FORM_DIREBEAR)
            {
                Unit::AuraList const& modAuras = target->GetAurasByType(SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE);
                for (Unit::AuraList::const_iterator i = modAuras.begin(); i != modAuras.end(); ++i)
                {
                    if ((*i)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_DRUID &&
                            (*i)->GetSpellProto()->SpellIconID == 961)
                    {
                        int32 bp = (*i)->GetSpellProto()->CalculateSimpleValue(EFFECT_INDEX_2);
                        if (bp)
                        {
                            target->CastCustomSpell(target, 62069, &bp, NULL, NULL, true, NULL, this);
                        }
                        break;
                    }
                }
            }

            // Improved Moonkin Form
            if (form == FORM_MOONKIN)
            {
                Unit::AuraList const& dummyAuras = target->GetAurasByType(SPELL_AURA_DUMMY);
                for (Unit::AuraList::const_iterator i = dummyAuras.begin(); i != dummyAuras.end(); ++i)
                {
                    if ((*i)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_DRUID &&
                            (*i)->GetSpellProto()->SpellIconID == 2855)
                    {
                        uint32 spell_id = 0;
                        switch ((*i)->GetId())
                        {
                            case 48384: spell_id = 50170; break; // Rank 1
                            case 48395: spell_id = 50171; break; // Rank 2
                            case 48396: spell_id = 50172; break; // Rank 3
                            default:
                                sLog.outError("Aura::HandleShapeshiftBoosts: Not handled rank of IMF (Spell: %u)", (*i)->GetId());
                                break;
                        }

                        if (spell_id)
                        {
                            target->CastSpell(target, spell_id, true, NULL, this);
                        }
                        break;
                    }
                }
            }

            // Heart of the Wild
            if (HotWSpellId)
            {
                Unit::AuraList const& mModTotalStatPct = target->GetAurasByType(SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE);
                for (Unit::AuraList::const_iterator i = mModTotalStatPct.begin(); i != mModTotalStatPct.end(); ++i)
                {
                    if ((*i)->GetSpellProto()->SpellIconID == 240 && (*i)->GetModifier()->m_miscvalue == 3)
                    {
                        int32 HotWMod = (*i)->GetModifier()->m_amount;
                        if (GetModifier()->m_miscvalue == FORM_CAT)
                        {
                            HotWMod /= 2;
                        }

                        target->CastCustomSpell(target, HotWSpellId, &HotWMod, NULL, NULL, true, NULL, this);
                        break;
                    }
                }
            }
        }
    }
    else
    {
        if (spellId1)
        {
            target->RemoveAurasDueToSpell(spellId1);
        }
        if (spellId2)
        {
            target->RemoveAurasDueToSpell(spellId2);
        }
        if (MasterShaperSpellId)
        {
            target->RemoveAurasDueToSpell(MasterShaperSpellId);
        }

        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            // re-apply passive spells that don't need shapeshift but were inactive in current form:
            const PlayerSpellMap& sp_list = ((Player*)target)->GetSpellMap();
            for (PlayerSpellMap::const_iterator itr = sp_list.begin(); itr != sp_list.end(); ++itr)
            {
                if (itr->second.state == PLAYERSPELL_REMOVED) continue;
                {
                    if (itr->first == spellId1 || itr->first == spellId2) continue;
                }
                SpellEntry const* spellInfo = sSpellStore.LookupEntry(itr->first);
                if (!spellInfo || !IsPassiveSpell(spellInfo))
                {
                    continue;
                }
                if (spellInfo->HasAttribute(SPELL_ATTR_EX2_NOT_NEED_SHAPESHIFT) && (spellInfo->StancesNot & (1 << (form - 1))))
                {
                    target->CastSpell(target, itr->first, true, NULL, this);
                }
            }
        }

        Unit::SpellAuraHolderMap& tAuras = target->GetSpellAuraHolderMap();
        for (Unit::SpellAuraHolderMap::iterator itr = tAuras.begin(); itr != tAuras.end();)
        {
            if (itr->second->IsRemovedOnShapeLost())
            {
                target->RemoveAurasDueToSpell(itr->second->GetId());
                itr = tAuras.begin();
            }
            else
            {
                ++itr;
            }
        }
    }
}

/**
 * @brief Applies or removes the empathy special-info flag for supported targets.
 *
 * @param apply True to apply the flag; false to remove it.
 * @param Real Unused.
 */
void Aura::HandleAuraEmpathy(bool apply, bool /*Real*/)
{
    Unit* target = GetTarget();

    // This aura is expected to only work with CREATURE_TYPE_BEAST or players
    CreatureInfo const* ci = ObjectMgr::GetCreatureTemplate(target->GetEntry());
    if (target->GetTypeId() == TYPEID_PLAYER || (target->GetTypeId() == TYPEID_UNIT && ci && ci->CreatureType == CREATURE_TYPE_BEAST))
    {
        target->ApplyModUInt32Value(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_SPECIALINFO, apply);
    }
}

/**
 * @brief Applies or removes the untrackable unit byte flag.
 *
 * @param apply True to apply the flag; false to remove it.
 * @param Real Unused.
 */
void Aura::HandleAuraUntrackable(bool apply, bool /*Real*/)
{
    if (apply)
    {
        GetTarget()->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_UNTRACKABLE);
    }
    else
    {
        GetTarget()->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_UNTRACKABLE);
    }
}

/**
 * @brief Applies or removes the pacified unit flag.
 *
 * @param apply True to pacify; false to remove pacify.
 * @param Real Unused.
 */
void Aura::HandleAuraModPacify(bool apply, bool /*Real*/)
{
    if (apply)
    {
        GetTarget()->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
    }
    else
    {
        GetTarget()->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
    }
}

/**
 * @brief Applies or removes both pacify and silence effects together.
 *
 * @param apply True to apply the effects; false to remove them.
 * @param Real True when processing the real aura state change.
 */
void Aura::HandleAuraModPacifyAndSilence(bool apply, bool Real)
{
    HandleAuraModPacify(apply, Real);
    HandleAuraModSilence(apply, Real);
}

/**
 * @brief Applies or removes the ghost player flag.
 *
 * @param apply True to apply the flag; false to remove it.
 * @param Real Unused.
 */
void Aura::HandleAuraGhost(bool apply, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    if (apply)
    {
        GetTarget()->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST);
    }
    else
    {
        GetTarget()->RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST);
    }
}

void Aura::HandleAuraAllowFlight(bool apply, bool Real)
{
    // all applied/removed only at real aura add/remove
    if (!Real)
    {
        return;
    }

    GetTarget()->SetCanFly(apply);
}

void Aura::HandleModRating(bool apply, bool Real)
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

    for (uint32 rating = 0; rating < MAX_COMBAT_RATING; ++rating)
    {
        if (m_modifier.m_miscvalue & (1 << rating))
        {
            ((Player*)GetTarget())->ApplyRatingMod(CombatRating(rating), m_modifier.m_amount, apply);
        }
    }
}

void Aura::HandleModRatingFromStat(bool apply, bool Real)
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
    // Just recalculate ratings
    for (uint32 rating = 0; rating < MAX_COMBAT_RATING; ++rating)
    {
        if (m_modifier.m_miscvalue & (1 << rating))
        {
            ((Player*)GetTarget())->ApplyRatingMod(CombatRating(rating), 0, apply);
        }
    }
}

void Aura::HandleForceMoveForward(bool apply, bool Real)
{
    if (!Real)
    {
        return;
    }

    if (apply)
    {
        GetTarget()->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FORCE_MOVE);
    }
    else
    {
        GetTarget()->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FORCE_MOVE);
    }
}

void Aura::HandleAuraModExpertise(bool /*apply*/, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    ((Player*)GetTarget())->UpdateExpertise(BASE_ATTACK);
    ((Player*)GetTarget())->UpdateExpertise(OFF_ATTACK);
}

void Aura::HandleModTargetResistance(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
    {
        return;
    }
    Unit* target = GetTarget();
    // applied to damage as HandleNoImmediateEffect in Unit::CalculateAbsorbAndResist and Unit::CalcArmorReducedDamage
    // show armor penetration
    if (target->GetTypeId() == TYPEID_PLAYER && (m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL))
    {
        target->ApplyModInt32Value(PLAYER_FIELD_MOD_TARGET_PHYSICAL_RESISTANCE, m_modifier.m_amount, apply);
    }

    // show as spell penetration only full spell penetration bonuses (all resistances except armor and holy
    if (target->GetTypeId() == TYPEID_PLAYER && (m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_SPELL) == SPELL_SCHOOL_MASK_SPELL)
    {
        target->ApplyModInt32Value(PLAYER_FIELD_MOD_TARGET_RESISTANCE, m_modifier.m_amount, apply);
    }
}

/**
 * @brief Preserves or removes combo points retained by the aura.
 *
 * @param apply True to apply the retention aura; false to remove it.
 * @param Real True when processing the real aura state change.
 */
void Aura::HandleAuraRetainComboPoints(bool apply, bool Real)
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

    Player* target = (Player*)GetTarget();

    // combo points was added in SPELL_EFFECT_ADD_COMBO_POINTS handler
    // remove only if aura expire by time (in case combo points amount change aura removed without combo points lost)
    if (!apply && m_removeMode == AURA_REMOVE_BY_EXPIRE && target->GetComboTargetGuid())
        if (Unit* unit = sObjectAccessor.GetUnit(*GetTarget(), target->GetComboTargetGuid()))
        {
            target->AddComboPoints(unit, -m_modifier.m_amount);
        }
}

/**
 * @brief Applies or removes the non-attackable state.
 *
 * @param Apply True to make the target unattackable; false to remove the state.
 * @param Real True when processing the real aura state change.
 */
void Aura::HandleModUnattackable(bool Apply, bool Real)
{
    if (Real && Apply)
    {
        GetTarget()->CombatStop();
        GetTarget()->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);
    }
    GetTarget()->ApplyModFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE, Apply);
}

/**
 * @brief Handles Spirit of Redemption setup and forced death on expiration.
 *
 * @param apply True to enter the spirit state; false to end it.
 * @param Real True when processing the real aura state change.
 */
void Aura::HandleSpiritOfRedemption(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
    {
        return;
    }

    Unit* target = GetTarget();

    // prepare spirit state
    if (apply)
    {
        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            // disable breath/etc timers
            ((Player*)target)->StopMirrorTimers();

            // set stand state (expected in this form)
            if (!target->IsStandState())
            {
                target->SetStandState(UNIT_STAND_STATE_STAND);
            }
        }

        target->SetHealth(1);
    }
    // die at aura end
    else
    {
        target->DealDamage(target, target->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, GetSpellProto(), false);
    }
}

/**
 * @brief Calculates absorb shield bonuses for school absorb effects.
 *
 * @param apply True to apply the absorb aura; false to remove it.
 * @param Real True when processing the real aura state change.
 */
void Aura::HandleSchoolAbsorb(bool apply, bool Real)
{
    if (!Real)
    {
        return;
    }

    Unit* caster = GetCaster();
    if (!caster)
    {
        return;
    }

    Unit* target = GetTarget();
    SpellEntry const* spellProto = GetSpellProto();
    if (apply)
    {
        // prevent double apply bonuses
        if (target->GetTypeId() != TYPEID_PLAYER || !((Player*)target)->GetSession()->PlayerLoading())
        {
            float DoneActualBenefit = 0.0f;
            switch (spellProto->SpellFamilyName)
            {
                case SPELLFAMILY_GENERIC:
                    // Stoicism
                    if (spellProto->Id == 70845)
                    {
                        DoneActualBenefit = caster->GetMaxHealth() * 0.20f;
                    }
                    break;
                case SPELLFAMILY_PRIEST:
                    // Power Word: Shield
                    if (spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000001))
                    {
                        //+80.68% from +spell bonus
                        DoneActualBenefit = caster->SpellBaseHealingBonusDone(GetSpellSchoolMask(spellProto)) * 0.8068f;
                        // Borrowed Time
                        Unit::AuraList const& borrowedTime = caster->GetAurasByType(SPELL_AURA_DUMMY);
                        for (Unit::AuraList::const_iterator itr = borrowedTime.begin(); itr != borrowedTime.end(); ++itr)
                        {
                            SpellEntry const* i_spell = (*itr)->GetSpellProto();
                            if (i_spell->SpellFamilyName == SPELLFAMILY_PRIEST && i_spell->SpellIconID == 2899 && i_spell->EffectMiscValue[(*itr)->GetEffIndex()] == 24)
                            {
                                DoneActualBenefit += DoneActualBenefit * (*itr)->GetModifier()->m_amount / 100;
                                break;
                            }
                        }
                    }

                    break;
                case SPELLFAMILY_MAGE:
                    // Frost Ward, Fire Ward
                    if (spellProto->IsFitToFamilyMask(UI64LIT(0x0000000000000108)))
                        //+10% from +spell bonus
                    {
                        DoneActualBenefit = caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(spellProto)) * 0.1f;
                    }
                    // Ice Barrier
                    else if (spellProto->IsFitToFamilyMask(UI64LIT(0x0000000100000000)))
                        //+80.67% from +spell bonus
                        DoneActualBenefit = caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(spellProto)) * 0.8067f;
                    break;
                case SPELLFAMILY_WARLOCK:
                    // Shadow Ward
                    if (spellProto->IsFitToFamilyMask(UI64LIT(0x0000000000000000), 0x00000040))
                        //+30% from +spell bonus
                        DoneActualBenefit = caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(spellProto)) * 0.30f;
                    break;
                case SPELLFAMILY_PALADIN:
                    // Sacred Shield
                    // (check not strictly needed, only Sacred Shield has SPELL_AURA_SCHOOL_ABSORB in SPELLFAMILY_PALADIN at this time)
                    if (spellProto->IsFitToFamilyMask(UI64LIT(0x0008000000000000)))
                    {
                        // +75% from spell power
                        DoneActualBenefit = caster->SpellBaseHealingBonusDone(GetSpellSchoolMask(spellProto)) * 0.75f;
                    }
                    break;
                default:
                    break;
            }

            DoneActualBenefit *= caster->CalculateLevelPenalty(GetSpellProto());

            m_modifier.m_amount += (int32)DoneActualBenefit;
        }
    }
    else
    {
        if (// Power Word: Shield
                spellProto->SpellFamilyName == SPELLFAMILY_PRIEST && spellProto->Mechanic == MECHANIC_SHIELD &&
                (spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000001)) &&
                // completely absorbed or dispelled
                (m_removeMode == AURA_REMOVE_BY_SHIELD_BREAK || m_removeMode == AURA_REMOVE_BY_DISPEL))
        {
            Unit::AuraList const& vDummyAuras = caster->GetAurasByType(SPELL_AURA_DUMMY);
            for (Unit::AuraList::const_iterator itr = vDummyAuras.begin(); itr != vDummyAuras.end(); ++itr)
            {
                SpellEntry const* vSpell = (*itr)->GetSpellProto();

                // Rapture (main spell)
                if (vSpell->SpellFamilyName == SPELLFAMILY_PRIEST && vSpell->SpellIconID == 2894 && vSpell->Effect[EFFECT_INDEX_1])
                {
                    switch ((*itr)->GetEffIndex())
                    {
                        case EFFECT_INDEX_0:
                        {
                            // energize caster
                            int32 manapct1000 = 5 * ((*itr)->GetModifier()->m_amount + sSpellMgr.GetSpellRank(vSpell->Id));
                            int32 basepoints0 = caster->GetMaxPower(POWER_MANA) * manapct1000 / 1000;
                            caster->CastCustomSpell(caster, 47755, &basepoints0, NULL, NULL, true);
                            break;
                        }
                        case EFFECT_INDEX_1:
                        {
                            // energize target
                            if (!roll_chance_i((*itr)->GetModifier()->m_amount) || caster->HasAura(63853))
                            {
                                break;
                            }

                            switch (target->GetPowerType())
                            {
                                case POWER_RUNIC_POWER:
                                    target->CastSpell(target, 63652, true, NULL, NULL, GetCasterGuid());
                                    break;
                                case POWER_RAGE:
                                    target->CastSpell(target, 63653, true, NULL, NULL, GetCasterGuid());
                                    break;
                                case POWER_MANA:
                                {
                                    int32 basepoints0 = target->GetMaxPower(POWER_MANA) * 2 / 100;
                                    target->CastCustomSpell(target, 63654, &basepoints0, NULL, NULL, true);
                                    break;
                                }
                                case POWER_ENERGY:
                                    target->CastSpell(target, 63655, true, NULL, NULL, GetCasterGuid());
                                    break;
                                default:
                                    break;
                            }

                            // cooldown aura
                            caster->CastSpell(caster, 63853, true);
                            break;
                        }
                        default:
                            sLog.outError("Changes in R-dummy spell???: effect 3");
                            break;
                    }
                }
            }
        }
    }
}

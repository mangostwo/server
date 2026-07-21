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
 * @file PlayerStats.cpp
 * @brief Cohesion split of Player.cpp -- base modifier handling, rating math,
 *        and skill progression/stat helper methods. Same `Player` class; no
 *        behaviour change.
 */

#include "Utilities/PackedValues.h"
#include "Player.h"
#include "Language.h"
#include "Database/DatabaseEnv.h"
#include "Log.h"
#include "Opcodes.h"
#include "SpellMgr.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "UpdateMask.h"
#include "CinematicFlyover.h"
#include "SkillDiscovery.h"
#include "QuestDef.h"
#include "GossipDef.h"
#include "UpdateData.h"
#include "Channel.h"
#include "ChannelMgr.h"
#include "MapManager.h"
#include "MapPersistentStateMgr.h"
#include "InstanceData.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "ObjectMgr.h"
#include "CreatureAI.h"
#include "Formulas.h"
#include "Group.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Pet.h"
#include "Util.h"
#include "Transports.h"
#include "Weather.h"
#include "BattleGround/BattleGround.h"
#include "BattleGround/BattleGroundMgr.h"
#include "BattleGround/BattleGroundAV.h"
#include "OutdoorPvP/OutdoorPvP.h"
#include "ArenaTeam.h"
#include "Chat.h"
#include "Database/DatabaseImpl.h"
#include "Spell.h"
#include "ScriptMgr.h"
#include "SocialMgr.h"
#include "AchievementMgr.h"
#include "Mail.h"
#include "SpellAuras.h"
#include "DBCStores.h"
#include "SQLStorages.h"
#include "Vehicle.h"
#include "Calendar.h"
#include "LFGMgr.h"
#include "DisableMgr.h"
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif /* ENABLE_ELUNA */

#include <cmath>

#define PLAYER_SKILL_INDEX(x)       (PLAYER_SKILL_INFO_1_1 + ((x)*3))
#define PLAYER_SKILL_VALUE_INDEX(x) (PLAYER_SKILL_INDEX(x)+1)
#define PLAYER_SKILL_BONUS_INDEX(x) (PLAYER_SKILL_INDEX(x)+2)

#define SKILL_VALUE(x)         PAIR32_LOPART(x)
#define SKILL_MAX(x)           PAIR32_HIPART(x)
#define MAKE_SKILL_VALUE(v, m) MAKE_PAIR32(v,m)

#define SKILL_TEMP_BONUS(x)    int16(PAIR32_LOPART(x))
#define SKILL_PERM_BONUS(x)    int16(PAIR32_HIPART(x))
#define MAKE_SKILL_BONUS(t, p) MAKE_PAIR32(t,p)
/**
 * @brief Applies or removes a base modifier affecting derived combat values.
 *
 * @param modGroup The modifier group to update.
 * @param modType The modifier type to apply.
 * @param amount The modifier amount.
 * @param apply True to apply the modifier; false to remove it.
 */
void Player::HandleBaseModValue(BaseModGroup modGroup, BaseModType modType, float amount, bool apply)
{
    if (modGroup >= BASEMOD_END || modType >= MOD_END)
    {
        sLog.outError("ERROR in HandleBaseModValue(): nonexistent BaseModGroup of wrong BaseModType!");
        return;
    }

    float val = 1.0f;

    switch (modType)
    {
        case FLAT_MOD:
            m_auraBaseMod[modGroup][modType] += apply ? amount : -amount;
            break;
        case PCT_MOD:
            if (amount <= -100.0f)
            {
                amount = -200.0f;
            }

            val = (100.0f + amount) / 100.0f;
            m_auraBaseMod[modGroup][modType] *= apply ? val : (1.0f / val);
            break;
    }

    if (!CanModifyStats())
    {
        return;
    }

    switch (modGroup)
    {
        case CRIT_PERCENTAGE:              UpdateCritPercentage(BASE_ATTACK);                          break;
        case RANGED_CRIT_PERCENTAGE:       UpdateCritPercentage(RANGED_ATTACK);                        break;
        case OFFHAND_CRIT_PERCENTAGE:      UpdateCritPercentage(OFF_ATTACK);                           break;
        case SHIELD_BLOCK_VALUE:           UpdateShieldBlockValue();                                   break;
        default: break;
    }
}

/**
 * @brief Gets a stored base modifier value.
 *
 * @param modGroup The modifier group to query.
 * @param modType The modifier type to query.
 * @return The stored modifier value.
 */
float Player::GetBaseModValue(BaseModGroup modGroup, BaseModType modType) const
{
    if (modGroup >= BASEMOD_END || modType > MOD_END)
    {
        sLog.outError("trial to access nonexistent BaseModGroup or wrong BaseModType!");
        return 0.0f;
    }

    if (modType == PCT_MOD && m_auraBaseMod[modGroup][PCT_MOD] <= 0.0f)
    {
        return 0.0f;
    }

    return m_auraBaseMod[modGroup][modType];
}

/**
 * @brief Gets the combined flat and percentage base modifier value for a group.
 *
 * @param modGroup The modifier group to query.
 * @return The total effective modifier value.
 */
float Player::GetTotalBaseModValue(BaseModGroup modGroup) const
{
    if (modGroup >= BASEMOD_END)
    {
        sLog.outError("wrong BaseModGroup in GetTotalBaseModValue()!");
        return 0.0f;
    }

    if (m_auraBaseMod[modGroup][PCT_MOD] <= 0.0f)
    {
        return 0.0f;
    }

    return m_auraBaseMod[modGroup][FLAT_MOD] * m_auraBaseMod[modGroup][PCT_MOD];
}

/**
 * @brief Computes the player's current shield block value.
 *
 * @return The effective shield block amount.
 */
uint32 Player::GetShieldBlockValue() const
{
    float value = (m_auraBaseMod[SHIELD_BLOCK_VALUE][FLAT_MOD] + GetStat(STAT_STRENGTH) * 0.5f - 10) * m_auraBaseMod[SHIELD_BLOCK_VALUE][PCT_MOD];

    value = (value < 0) ? 0 : value;

    return uint32(value);
}

/**
 * @brief Calculates melee critical strike chance gained from agility.
 *
 * @return The melee crit contribution from agility.
 */
float Player::GetMeleeCritFromAgility()
{
    uint32 level = getLevel();
    uint32 pclass = getClass();

    if (level > GT_MAX_LEVEL) level = GT_MAX_LEVEL;

    GtChanceToMeleeCritBaseEntry const* critBase  = sGtChanceToMeleeCritBaseStore.LookupEntry(pclass - 1);
    GtChanceToMeleeCritEntry     const* critRatio = sGtChanceToMeleeCritStore.LookupEntry((pclass - 1) * GT_MAX_LEVEL + level - 1);
    if (critBase == NULL || critRatio == NULL)
    {
        return 0.0f;
    }

    float crit = critBase->Data + GetStat(STAT_AGILITY) * critRatio->Data;
    return crit * 100.0f;
}

/**
 * @brief Calculates dodge chance gained from agility.
 */
void Player::GetDodgeFromAgility(float& diminishing, float& nondiminishing)
{
    // Table for base dodge values
    const float dodge_base[MAX_CLASSES] =
    {
        0.036640f, // Warrior
        0.034943f, // Paladin
        -0.040873f, // Hunter
        0.020957f, // Rogue
        0.034178f, // Priest
        0.036640f, // DK
        0.021080f, // Shaman
        0.036587f, // Mage
        0.024211f, // Warlock
        0.0f,      // ??
        0.056097f  // Druid
    };
    // Crit/agility to dodge/agility coefficient multipliers; 3.2.0 increased required agility by 15%
    const float crit_to_dodge[MAX_CLASSES] =
    {
        0.85f / 1.15f,  // Warrior
        1.00f / 1.15f,  // Paladin
        1.11f / 1.15f,  // Hunter
        2.00f / 1.15f,  // Rogue
        1.00f / 1.15f,  // Priest
        0.85f / 1.15f,  // DK
        1.60f / 1.15f,  // Shaman
        1.00f / 1.15f,  // Mage
        0.97f / 1.15f,  // Warlock (?)
        0.0f,           // ??
        2.00f / 1.15f   // Druid
    };

    uint32 level = getLevel();
    uint32 pclass = getClass();

    if (level > GT_MAX_LEVEL) level = GT_MAX_LEVEL;

    // Dodge per agility is proportional to crit per agility, which is available from DBC files
    GtChanceToMeleeCritEntry  const* dodgeRatio = sGtChanceToMeleeCritStore.LookupEntry((pclass - 1) * GT_MAX_LEVEL + level - 1);
    if (dodgeRatio == NULL || pclass > MAX_CLASSES)
    {
        return;
    }

    // TODO: research if talents/effects that increase total agility by x% should increase non-diminishing part
    float base_agility = GetCreateStat(STAT_AGILITY) * m_auraModifiersGroup[UNIT_MOD_STAT_START + STAT_AGILITY][BASE_PCT];
    float bonus_agility = GetStat(STAT_AGILITY) - base_agility;
    // calculate diminishing (green in char screen) and non-diminishing (white) contribution
    diminishing = 100.0f * bonus_agility * dodgeRatio->Data * crit_to_dodge[pclass - 1];
    nondiminishing = 100.0f * (dodge_base[pclass - 1] + base_agility * dodgeRatio->Data * crit_to_dodge[pclass - 1]);
}

/**
 * @brief Calculates spell critical strike chance gained from intellect.
 *
 * @return The spell crit contribution from intellect.
 */
float Player::GetSpellCritFromIntellect()
{
    uint32 level = getLevel();
    uint32 pclass = getClass();

    if (level > GT_MAX_LEVEL) level = GT_MAX_LEVEL;

    GtChanceToSpellCritBaseEntry const* critBase  = sGtChanceToSpellCritBaseStore.LookupEntry(pclass - 1);
    GtChanceToSpellCritEntry     const* critRatio = sGtChanceToSpellCritStore.LookupEntry((pclass - 1) * GT_MAX_LEVEL + level - 1);
    if (critBase == NULL || critRatio == NULL)
    {
        return 0.0f;
    }

    float crit = critBase->Data + GetStat(STAT_INTELLECT) * critRatio->Data;
    return crit * 100.0f;
}

float Player::GetRatingMultiplier(CombatRating cr) const
{
    uint32 level = getLevel();

    if (level > GT_MAX_LEVEL) level = GT_MAX_LEVEL;

    GtCombatRatingsEntry const* Rating = sGtCombatRatingsStore.LookupEntry(cr * GT_MAX_LEVEL + level - 1);
    // gtOCTClassCombatRatingScalarStore.dbc starts with 1, CombatRating with zero, so cr+1
    GtOCTClassCombatRatingScalarEntry const* classRating = sGtOCTClassCombatRatingScalarStore.LookupEntry((getClass() - 1) * GT_MAX_RATING + cr + 1);
    if (!Rating || !classRating)
    {
        return 1.0f;                                        // By default use minimum coefficient (not must be called)
    }

    return classRating->Data / Rating->Data;
}

float Player::GetRatingBonusValue(CombatRating cr) const
{
    return float(GetUInt32Value(PLAYER_FIELD_COMBAT_RATING_1 + cr)) * GetRatingMultiplier(cr);
}

float Player::GetExpertiseDodgeOrParryReduction(WeaponAttackType attType) const
{
    switch (attType)
    {
        case BASE_ATTACK:
            return GetUInt32Value(PLAYER_EXPERTISE) / 4.0f;
        case OFF_ATTACK:
            return GetUInt32Value(PLAYER_OFFHAND_EXPERTISE) / 4.0f;
        default:
            break;
    }
    return 0.0f;
}

/**
 * @brief Calculates health regeneration per spirit tick.
 *
 * @return The health regeneration value based on spirit and class.
 */
float Player::OCTRegenHPPerSpirit()
{
    uint32 level = getLevel();
    uint32 pclass = getClass();

    if (level > GT_MAX_LEVEL) level = GT_MAX_LEVEL;

    GtOCTRegenHPEntry     const* baseRatio = sGtOCTRegenHPStore.LookupEntry((pclass - 1) * GT_MAX_LEVEL + level - 1);
    GtRegenHPPerSptEntry  const* moreRatio = sGtRegenHPPerSptStore.LookupEntry((pclass - 1) * GT_MAX_LEVEL + level - 1);
    if (baseRatio == NULL || moreRatio == NULL)
    {
        return 0.0f;
    }

    // Formula from PaperDollFrame script
    float spirit = GetStat(STAT_SPIRIT);
    float baseSpirit = spirit;
    if (baseSpirit > 50)
    {
        baseSpirit = 50;
    }
    float moreSpirit = spirit - baseSpirit;
    float regen = baseSpirit * baseRatio->Data + moreSpirit * moreRatio->ratio;
    return regen;
}

/**
 * @brief Calculates mana regeneration per spirit tick.
 *
 * @return The mana regeneration value based on spirit and class.
 */
float Player::OCTRegenMPPerSpirit()
{
    uint32 level = getLevel();
    uint32 pclass = getClass();

    if (level > GT_MAX_LEVEL) level = GT_MAX_LEVEL;

//    GtOCTRegenMPEntry     const *baseRatio = sGtOCTRegenMPStore.LookupEntry((pclass-1)*GT_MAX_LEVEL + level-1);
    GtRegenMPPerSptEntry  const* moreRatio = sGtRegenMPPerSptStore.LookupEntry((pclass - 1) * GT_MAX_LEVEL + level - 1);
    if (moreRatio == NULL)
    {
        return 0.0f;
    }

    // Formula get from PaperDollFrame script
    float spirit    = GetStat(STAT_SPIRIT);
    float regen     = spirit * moreRatio->Data;
    return regen;
}

void Player::ApplyRatingMod(CombatRating cr, int32 value, bool apply)
{
    m_baseRatingValue[cr] += (apply ? value : -value);

    // explicit affected values
    switch (cr)
    {
        case CR_HASTE_MELEE:
        {
            float RatingChange = value * GetRatingMultiplier(cr);
            ApplyAttackTimePercentMod(BASE_ATTACK, RatingChange, apply);
            ApplyAttackTimePercentMod(OFF_ATTACK, RatingChange, apply);
            break;
        }
        case CR_HASTE_RANGED:
        {
            float RatingChange = value * GetRatingMultiplier(cr);
            ApplyAttackTimePercentMod(RANGED_ATTACK, RatingChange, apply);
            break;
        }
        case CR_HASTE_SPELL:
        {
            float RatingChange = value * GetRatingMultiplier(cr);
            ApplyCastTimePercentMod(RatingChange, apply);
            break;
        }
        default:
            break;
    }

    UpdateRating(cr);
}

void Player::UpdateRating(CombatRating cr)
{
    int32 amount = m_baseRatingValue[cr];
    // Apply bonus from SPELL_AURA_MOD_RATING_FROM_STAT
    // stat used stored in miscValueB for this aura
    AuraList const& modRatingFromStat = GetAurasByType(SPELL_AURA_MOD_RATING_FROM_STAT);
    for (AuraList::const_iterator i = modRatingFromStat.begin(); i != modRatingFromStat.end(); ++i)
    {
        if ((*i)->GetMiscValue() & (1 << cr))
        {
            amount += int32(GetStat(Stats((*i)->GetMiscBValue())) * (*i)->GetModifier()->m_amount / 100.0f);
        }
    }
    if (amount < 0)
    {
        amount = 0;
    }
    SetUInt32Value(PLAYER_FIELD_COMBAT_RATING_1 + cr, uint32(amount));

    bool affectStats = CanModifyStats();

    switch (cr)
    {
        case CR_WEAPON_SKILL:                               // Implemented in Unit::RollMeleeOutcomeAgainst
        case CR_DEFENSE_SKILL:
            UpdateDefenseBonusesMod();
            break;
        case CR_DODGE:
            UpdateDodgePercentage();
            break;
        case CR_PARRY:
            UpdateParryPercentage();
            break;
        case CR_BLOCK:
            UpdateBlockPercentage();
            break;
        case CR_HIT_MELEE:
            UpdateMeleeHitChances();
            break;
        case CR_HIT_RANGED:
            UpdateRangedHitChances();
            break;
        case CR_HIT_SPELL:
            UpdateSpellHitChances();
            break;
        case CR_CRIT_MELEE:
            if (affectStats)
            {
                UpdateCritPercentage(BASE_ATTACK);
                UpdateCritPercentage(OFF_ATTACK);
            }
            break;
        case CR_CRIT_RANGED:
            if (affectStats)
            {
                UpdateCritPercentage(RANGED_ATTACK);
            }
            break;
        case CR_CRIT_SPELL:
            if (affectStats)
            {
                UpdateAllSpellCritChances();
            }
            break;
        case CR_HIT_TAKEN_MELEE:                            // Implemented in Unit::MeleeMissChanceCalc
        case CR_HIT_TAKEN_RANGED:
            break;
        case CR_HIT_TAKEN_SPELL:                            // Implemented in Unit::MagicSpellHitResult
            break;
        case CR_CRIT_TAKEN_MELEE:                           // Implemented in Unit::RollMeleeOutcomeAgainst (only for chance to crit)
        case CR_CRIT_TAKEN_RANGED:
            break;
        case CR_CRIT_TAKEN_SPELL:                           // Implemented in Unit::SpellCriticalBonus (only for chance to crit)
            break;
        case CR_HASTE_MELEE:                                // Implemented in Player::ApplyRatingMod
        case CR_HASTE_RANGED:
        case CR_HASTE_SPELL:
            break;
        case CR_WEAPON_SKILL_MAINHAND:                      // Implemented in Unit::RollMeleeOutcomeAgainst
        case CR_WEAPON_SKILL_OFFHAND:
        case CR_WEAPON_SKILL_RANGED:
            break;
        case CR_EXPERTISE:
            if (affectStats)
            {
                UpdateExpertise(BASE_ATTACK);
                UpdateExpertise(OFF_ATTACK);
            }
            break;
        case CR_ARMOR_PENETRATION:
            if (affectStats)
            {
                UpdateArmorPenetration();
            }
            break;
    }
}

void Player::UpdateAllRatings()
{
    for (int cr = 0; cr < MAX_COMBAT_RATING; ++cr)
    {
        UpdateRating(CombatRating(cr));
    }
}

/**
 * @brief Restores attack timers from currently equipped weapon delays.
 */
void Player::SetRegularAttackTime()
{
    for (int i = 0; i < MAX_ATTACK; ++i)
    {
        Item* tmpitem = GetWeaponForAttack(WeaponAttackType(i), true, false);
        if (tmpitem)
        {
            ItemPrototype const* proto = tmpitem->GetProto();
            if (proto->Delay)
            {
                SetAttackTime(WeaponAttackType(i), proto->Delay);
            }
            else
            {
                SetAttackTime(WeaponAttackType(i), BASE_ATTACK_TIME);
            }
        }
    }
}

// skill+step, checking for max value
bool Player::UpdateSkill(uint32 skill_id, uint32 step)
{
    if (!skill_id)
    {
        return false;
    }

    SkillStatusMap::iterator itr = mSkillStatus.find(skill_id);
    if (itr == mSkillStatus.end())
    {
        return false;
    }

    SkillStatusData &skillStatus = itr->second;
    if (skillStatus.uState == SKILL_DELETED)
    {
        return false;
    }

    uint32 valueIndex = PLAYER_SKILL_VALUE_INDEX(skillStatus.pos);
    uint32 data = GetUInt32Value(valueIndex);
    uint32 value = SKILL_VALUE(data);
    uint32 max = SKILL_MAX(data);

    if ((!max) || (!value) || (value >= max))
    {
        return false;
    }

    if (value * 512 < max * urand(0, 512))
    {
        uint32 new_value = value + step;
        if (new_value > max)
        {
            new_value = max;
        }

        SetUInt32Value(valueIndex, MAKE_SKILL_VALUE(new_value, max));

        if (skillStatus.uState != SKILL_NEW)
        {
            skillStatus.uState = SKILL_CHANGED;
        }
        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL, skill_id);
        return true;
    }

    return false;
}

/**
 * @brief Calculates the configured chance to gain a skill point at the current value.
 *
 * @param SkillValue The player's current skill value.
 * @param GrayLevel The value at which gains become gray.
 * @param GreenLevel The value at which gains become green.
 * @param YellowLevel The value at which gains become yellow.
 * @return The gain chance scaled by ten.
 */
inline int SkillGainChance(uint32 SkillValue, uint32 GrayLevel, uint32 GreenLevel, uint32 YellowLevel)
{
    if (SkillValue >= GrayLevel)
    {
        return sWorld.getConfig(CONFIG_UINT32_SKILL_CHANCE_GREY) * 10;
    }
    if (SkillValue >= GreenLevel)
    {
        return sWorld.getConfig(CONFIG_UINT32_SKILL_CHANCE_GREEN) * 10;
    }
    if (SkillValue >= YellowLevel)
    {
        return sWorld.getConfig(CONFIG_UINT32_SKILL_CHANCE_YELLOW) * 10;
    }
    return sWorld.getConfig(CONFIG_UINT32_SKILL_CHANCE_ORANGE) * 10;
}

/**
 * @brief Attempts to increase a crafting skill based on a spell cast.
 *
 * @param spellid The crafting spell identifier.
 * @return True if the skill increased; otherwise, false.
 */
bool Player::UpdateCraftSkill(uint32 spellid)
{
    DEBUG_LOG("UpdateCraftSkill spellid %d", spellid);

    SkillLineAbilityMapBounds bounds = sSpellMgr.GetSkillLineAbilityMapBounds(spellid);

    for (SkillLineAbilityMap::const_iterator _spell_idx = bounds.first; _spell_idx != bounds.second; ++_spell_idx)
    {
        SkillLineAbilityEntry const* skill = _spell_idx->second;
        if (skill->SkillLine)
        {
            uint32 SkillValue = GetPureSkillValue(skill->SkillLine);

            // Alchemy Discoveries here
            SpellEntry const* spellEntry = sSpellStore.LookupEntry(spellid);
            if (spellEntry && spellEntry->Mechanic == MECHANIC_DISCOVERY)
            {
                if (uint32 discoveredSpell = GetSkillDiscoverySpell(skill->SkillLine, spellid, this))
                {
                    learnSpell(discoveredSpell, false);
                }
            }

            uint32 craft_skill_gain = sWorld.getConfig(CONFIG_UINT32_SKILL_GAIN_CRAFTING);

            return UpdateSkillPro(skill->SkillLine, SkillGainChance(SkillValue,
                                  skill->TrivialSkillLineRankHigh,
                                  (skill->TrivialSkillLineRankHigh + skill->TrivialSkillLineRankLow) / 2,
                                  skill->TrivialSkillLineRankLow),
                                  craft_skill_gain);
        }
    }
    return false;
}

/**
 * @brief Attempts to increase a gathering skill using profession-specific gain rules.
 *
 * @param SkillId The skill identifier to update.
 * @param SkillValue The current skill value.
 * @param RedLevel The red difficulty threshold for the source.
 * @param Multiplicator An additional gain chance multiplier.
 * @return True if the skill increased; otherwise, false.
 */
bool Player::UpdateGatherSkill(uint32 SkillId, uint32 SkillValue, uint32 RedLevel, uint32 Multiplicator)
{
    DEBUG_LOG("UpdateGatherSkill(SkillId %d SkillLevel %d RedLevel %d)", SkillId, SkillValue, RedLevel);

    uint32 gathering_skill_gain = sWorld.getConfig(CONFIG_UINT32_SKILL_GAIN_GATHERING);

    // For skinning and Mining chance decrease with level. 1-74 - no decrease, 75-149 - 2 times, 225-299 - 8 times
    switch (SkillId)
    {
        case SKILL_HERBALISM:
        case SKILL_LOCKPICKING:
        case SKILL_JEWELCRAFTING:
        case SKILL_INSCRIPTION:
            return UpdateSkillPro(SkillId, SkillGainChance(SkillValue, RedLevel + 100, RedLevel + 50, RedLevel + 25) * Multiplicator, gathering_skill_gain);
        case SKILL_SKINNING:
            if (sWorld.getConfig(CONFIG_UINT32_SKILL_CHANCE_SKINNING_STEPS) == 0)
            {
                return UpdateSkillPro(SkillId, SkillGainChance(SkillValue, RedLevel + 100, RedLevel + 50, RedLevel + 25) * Multiplicator, gathering_skill_gain);
            }
            else
            {
                return UpdateSkillPro(SkillId, (SkillGainChance(SkillValue, RedLevel + 100, RedLevel + 50, RedLevel + 25) * Multiplicator) >> (SkillValue / sWorld.getConfig(CONFIG_UINT32_SKILL_CHANCE_SKINNING_STEPS)), gathering_skill_gain);
            }
        case SKILL_MINING:
            if (sWorld.getConfig(CONFIG_UINT32_SKILL_CHANCE_MINING_STEPS) == 0)
            {
                return UpdateSkillPro(SkillId, SkillGainChance(SkillValue, RedLevel + 100, RedLevel + 50, RedLevel + 25) * Multiplicator, gathering_skill_gain);
            }
            else
            {
                return UpdateSkillPro(SkillId, (SkillGainChance(SkillValue, RedLevel + 100, RedLevel + 50, RedLevel + 25) * Multiplicator) >> (SkillValue / sWorld.getConfig(CONFIG_UINT32_SKILL_CHANCE_MINING_STEPS)), gathering_skill_gain);
            }
    }
    return false;
}

/**
 * @brief Attempts to increase the player's fishing skill.
 *
 * @return True if the skill increased; otherwise, false.
 */
bool Player::UpdateFishingSkill()
{
    DEBUG_LOG("UpdateFishingSkill");

    uint32 SkillValue = GetPureSkillValue(SKILL_FISHING);

    int32 chance = SkillValue < 75 ? 100 : 2500 / (SkillValue - 50);

    uint32 gathering_skill_gain = sWorld.getConfig(CONFIG_UINT32_SKILL_GAIN_GATHERING);

    return UpdateSkillPro(SKILL_FISHING, chance * 10, gathering_skill_gain);
}

// levels sync. with spell requirement for skill levels to learn
// bonus abilities in sSkillLineAbilityStore
// Used only to avoid scan DBC at each skill grow
static uint32 bonusSkillLevels[] = {75, 150, 225, 300, 375, 450};

/**
 * @brief Attempts to increase a skill using an explicit percentage chance.
 *
 * @param SkillId The skill identifier to update.
 * @param Chance The gain chance in tenths of a percent.
 * @param step The amount to increase the skill by.
 * @return True if the skill increased; otherwise, false.
 */
bool Player::UpdateSkillPro(uint16 SkillId, int32 Chance, uint32 step)
{
    DEBUG_LOG("UpdateSkillPro(SkillId %d, Chance %3.1f%%)", SkillId, Chance / 10.0);
    if (!SkillId)
    {
        return false;
    }

    if (Chance <= 0)                                        // speedup in 0 chance case
    {
        DEBUG_LOG("Player::UpdateSkillPro Chance=%3.1f%% missed", Chance / 10.0);
        return false;
    }

    SkillStatusMap::iterator itr = mSkillStatus.find(SkillId);
    if (itr == mSkillStatus.end())
    {
        return false;
    }

    SkillStatusData &skillStatus = itr->second;
    if (skillStatus.uState == SKILL_DELETED)
    {
        return false;
    }

    uint32 valueIndex = PLAYER_SKILL_VALUE_INDEX(skillStatus.pos);

    uint32 data = GetUInt32Value(valueIndex);
    uint16 SkillValue = SKILL_VALUE(data);
    uint16 MaxValue   = SKILL_MAX(data);

    if (!MaxValue || !SkillValue || SkillValue >= MaxValue)
    {
        return false;
    }

    int32 Roll = irand(1, 1000);

    if (Roll <= Chance)
    {
        uint32 new_value = SkillValue + step;
        if (new_value > MaxValue)
        {
            new_value = MaxValue;
        }

        SetUInt32Value(valueIndex, MAKE_SKILL_VALUE(new_value, MaxValue));

        if (skillStatus.uState != SKILL_NEW)
        {
            skillStatus.uState = SKILL_CHANGED;
        }
        for (uint32* bsl = &bonusSkillLevels[0]; *bsl; ++bsl)
        {
            if ((SkillValue < *bsl && new_value >= *bsl))
            {
                learnSkillRewardedSpells(SkillId, new_value);
                break;
            }
        }
        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL, SkillId);
        DEBUG_LOG("Player::UpdateSkillPro Chance=%3.1f%% taken", Chance / 10.0);
        return true;
    }

    DEBUG_LOG("Player::UpdateSkillPro Chance=%3.1f%% missed", Chance / 10.0);
    return false;
}

/**
 * @brief Attempts to improve the player's weapon skill for an attack type.
 *
 * @param attType The attack type whose weapon skill should be updated.
 */
void Player::UpdateWeaponSkill(WeaponAttackType attType)
{
    // no skill gain in pvp
    Unit* pVictim = getVictim();
    if (pVictim && pVictim->IsCharmerOrOwnerPlayerOrPlayerItself())
    {
        return;
    }

    if (IsInFeralForm())
    {
        return; // always maximized SKILL_FERAL_COMBAT in fact
    }

    if (GetShapeshiftForm() == FORM_TREE)
    {
        return; // use weapon but not skill up
    }

    uint32 weaponSkillGain = sWorld.getConfig(CONFIG_UINT32_SKILL_GAIN_WEAPON);

    Item* pWeapon = GetWeaponForAttack(attType, true, true);
    if (pWeapon && pWeapon->GetProto()->SubClass != ITEM_SUBCLASS_WEAPON_FISHING_POLE)
    {
        UpdateSkill(pWeapon->GetSkill(), weaponSkillGain);
    }
    else if (!pWeapon && attType == BASE_ATTACK)
    {
        UpdateSkill(SKILL_UNARMED, weaponSkillGain);
    }

    UpdateAllCritPercentages();
}

/**
 * @brief Attempts to improve weapon or defense skills from combat.
 *
 * @param pVictim The opposing unit involved in combat.
 * @param attType The attack type used for offensive skill checks.
 * @param defence True to evaluate defense gain; false for weapon skill gain.
 */
void Player::UpdateCombatSkills(Unit* pVictim, WeaponAttackType attType, bool defence)
{
    uint32 plevel = getLevel();                             // if defense than pVictim == attacker
    uint32 greylevel = MaNGOS::XP::GetGrayLevel(plevel);
    uint32 moblevel = pVictim->GetLevelForTarget(this);
    if (moblevel < greylevel)
    {
        return;
    }

    if (moblevel > plevel + 5)
    {
        moblevel = plevel + 5;
    }

    uint32 lvldif = moblevel - greylevel;
    if (lvldif < 3)
    {
        lvldif = 3;
    }

    int32 skilldif = 5 * plevel - (defence ? GetBaseDefenseSkillValue() : GetBaseWeaponSkillValue(attType));

    // Max skill reached for level.
    // Can in some cases be less than 0: having max skill and then .level -1 as example.
    if (skilldif <= 0)
    {
        return;
    }

    float chance = float(3 * lvldif * skilldif) / plevel;
    if (!defence)
    {
        chance *= 0.1f * GetStat(STAT_INTELLECT);
    }

    chance = chance < 1.0f ? 1.0f : chance;                 // minimum chance to increase skill is 1%

    if (roll_chance_f(chance))
    {
        if (defence)
        {
            UpdateDefense();
        }
        else
        {
            UpdateWeaponSkill(attType);
        }
    }
    else
    {
        return;
    }
}

/**
 * @brief Modifies the temporary or permanent bonus for a skill.
 *
 * @param skillid The skill identifier to modify.
 * @param val The bonus amount to add or remove.
 * @param talent True for permanent talent-based bonuses; false for temporary bonuses.
 */
void Player::ModifySkillBonus(uint32 skillid, int32 val, bool talent)
{
    SkillStatusMap::const_iterator itr = mSkillStatus.find(skillid);
    if (itr == mSkillStatus.end() || itr->second.uState == SKILL_DELETED)
    {
        return;
    }

    uint32 bonusIndex = PLAYER_SKILL_BONUS_INDEX(itr->second.pos);

    uint32 bonus_val = GetUInt32Value(bonusIndex);
    int16 temp_bonus = SKILL_TEMP_BONUS(bonus_val);
    int16 perm_bonus = SKILL_PERM_BONUS(bonus_val);

    if (talent)                                         // permanent bonus stored in high part
    {
        SetUInt32Value(bonusIndex, MAKE_SKILL_BONUS(temp_bonus, perm_bonus + val));
    }
    else                                                // temporary/item bonus stored in low part
    {
        SetUInt32Value(bonusIndex, MAKE_SKILL_BONUS(temp_bonus + val, perm_bonus));
    }
}

/**
 * @brief Updates level-scaled skills to match the player's current level cap.
 */
void Player::UpdateSkillsForLevel()
{
    uint16 maxconfskill = sWorld.GetConfigMaxSkillValue();
    uint32 maxSkill = GetMaxSkillValueForLevel();

    bool alwaysMaxSkill = sWorld.getConfig(CONFIG_BOOL_ALWAYS_MAX_SKILL_FOR_LEVEL);

    for (SkillStatusMap::iterator itr = mSkillStatus.begin(); itr != mSkillStatus.end(); ++itr)
    {
        SkillStatusData &skillStatus = itr->second;
        if (skillStatus.uState == SKILL_DELETED)
        {
            continue;
        }

        uint32 pskill = itr->first;

        SkillLineEntry const* pSkill = sSkillLineStore.LookupEntry(pskill);
        if (!pSkill)
        {
            continue;
        }

        if (GetSkillRangeType(pSkill, false) != SKILL_RANGE_LEVEL)
        {
            continue;
        }

        uint32 valueIndex = PLAYER_SKILL_VALUE_INDEX(skillStatus.pos);
        uint32 data = GetUInt32Value(valueIndex);
        uint32 max = SKILL_MAX(data);
        uint32 val = SKILL_VALUE(data);

        /// update only level dependent max skill values
        if (max != 1)
        {
            /// maximize skill always
            if (alwaysMaxSkill)
            {
                SetUInt32Value(valueIndex, MAKE_SKILL_VALUE(maxSkill, maxSkill));
                if (skillStatus.uState != SKILL_NEW)
                {
                    skillStatus.uState = SKILL_CHANGED;
                }
                GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL, pskill);
            }
            else if (max != maxconfskill)                   /// update max skill value if current max skill not maximized
            {
                SetUInt32Value(valueIndex, MAKE_SKILL_VALUE(val, maxSkill));
                if (skillStatus.uState != SKILL_NEW)
                {
                    skillStatus.uState = SKILL_CHANGED;
                }
            }
        }
    }
}

/**
 * @brief Raises non-profession skills to their current maximum values.
 */
void Player::UpdateSkillsToMaxSkillsForLevel()
{
    for (SkillStatusMap::iterator itr = mSkillStatus.begin(); itr != mSkillStatus.end(); ++itr)
    {
        SkillStatusData &skillStatus = itr->second;
        if (skillStatus.uState == SKILL_DELETED)
        {
            continue;
        }

        uint32 pskill = itr->first;
        if (IsProfessionOrRidingSkill(pskill))
        {
            continue;
        }
        uint32 valueIndex = PLAYER_SKILL_VALUE_INDEX(skillStatus.pos);
        uint32 data = GetUInt32Value(valueIndex);

        uint32 max = SKILL_MAX(data);

        if (max > 1)
        {
            SetUInt32Value(valueIndex, MAKE_SKILL_VALUE(max, max));
            if (skillStatus.uState != SKILL_NEW)
            {
                skillStatus.uState = SKILL_CHANGED;
            }
            GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL, pskill);
        }

        if (pskill == SKILL_DEFENSE)
        {
            UpdateDefenseBonusesMod();
        }
    }
}

// This functions sets a skill line value (and adds if doesn't exist yet)
// To "remove" a skill line, set it's values to zero
void Player::SetSkill(uint16 id, uint16 currVal, uint16 maxVal, uint16 step /*=0*/)
{
    if (!id)
    {
        return;
    }

    SkillStatusMap::iterator itr = mSkillStatus.find(id);

    // has skill
    if (itr != mSkillStatus.end() && itr->second.uState != SKILL_DELETED)
    {
        SkillStatusData &skillStatus = itr->second;
        if (currVal)
        {
            if (step)                                      // need update step
            {
                SetUInt32Value(PLAYER_SKILL_INDEX(skillStatus.pos), MAKE_PAIR32(id, step));
            }

            // update value
            SetUInt32Value(PLAYER_SKILL_VALUE_INDEX(skillStatus.pos), MAKE_SKILL_VALUE(currVal, maxVal));
            if (skillStatus.uState != SKILL_NEW)
            {
                skillStatus.uState = SKILL_CHANGED;
            }
            learnSkillRewardedSpells(id, currVal);
            GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL, id);
            GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LEVEL, id);
        }
        else                                                // remove
        {
            // clear skill fields
            SetUInt32Value(PLAYER_SKILL_INDEX(skillStatus.pos), 0);
            SetUInt32Value(PLAYER_SKILL_VALUE_INDEX(skillStatus.pos), 0);
            SetUInt32Value(PLAYER_SKILL_BONUS_INDEX(skillStatus.pos), 0);

            // mark as deleted or simply remove from map if not saved yet
            if (skillStatus.uState != SKILL_NEW)
            {
                skillStatus.uState = SKILL_DELETED;
            }
            else
            {
                mSkillStatus.erase(itr);
            }

            // remove all spells that related to this skill
            for (uint32 j = 0; j < sSkillLineAbilityStore.GetNumRows(); ++j)
            {
                if (SkillLineAbilityEntry const* pAbility = sSkillLineAbilityStore.LookupEntry(j))
                    if (pAbility->SkillLine == id)
                    {
                        removeSpell(sSpellMgr.GetFirstSpellInChain(pAbility->Spell));
                    }
            }
        }
    }
    else if (currVal)                                       // add
    {
        for (int i = 0; i < PLAYER_MAX_SKILLS; ++i)
        {
            if (!GetUInt32Value(PLAYER_SKILL_INDEX(i)))
            {
                SkillLineEntry const* pSkill = sSkillLineStore.LookupEntry(id);
                if (!pSkill)
                {
                    sLog.outError("Skill not found in SkillLineStore: skill #%u", id);
                    return;
                }

                SetUInt32Value(PLAYER_SKILL_INDEX(i), MAKE_PAIR32(id, step));
                SetUInt32Value(PLAYER_SKILL_VALUE_INDEX(i), MAKE_SKILL_VALUE(currVal, maxVal));
                GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL, id);
                GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LEVEL, id);

                // insert new entry or update if not deleted old entry yet
                if (itr != mSkillStatus.end())
                {
                    itr->second.pos = i;
                    itr->second.uState = SKILL_CHANGED;
                }
                else
                {
                    mSkillStatus.insert(SkillStatusMap::value_type(id, SkillStatusData(i, SKILL_NEW)));
                }

                // apply skill bonuses
                SetUInt32Value(PLAYER_SKILL_BONUS_INDEX(i), 0);

                // temporary bonuses
                AuraList const& mModSkill = GetAurasByType(SPELL_AURA_MOD_SKILL);
                for (AuraList::const_iterator j = mModSkill.begin(); j != mModSkill.end(); ++j)
                {
                    if ((*j)->GetModifier()->m_miscvalue == int32(id))
                    {
                        (*j)->ApplyModifier(true);
                    }
                }

                // permanent bonuses
                AuraList const& mModSkillTalent = GetAurasByType(SPELL_AURA_MOD_SKILL_TALENT);
                for (AuraList::const_iterator j = mModSkillTalent.begin(); j != mModSkillTalent.end(); ++j)
                {
                    if ((*j)->GetModifier()->m_miscvalue == int32(id))
                    {
                        (*j)->ApplyModifier(true);
                    }
                }

                // Learn all spells for skill
                learnSkillRewardedSpells(id, currVal);
                return;
            }
        }
    }
}

/**
 * @brief Checks whether the player currently has a skill line.
 *
 * @param skill The skill identifier to test.
 * @return True if the skill exists and is not deleted; otherwise, false.
 */
bool Player::HasSkill(uint32 skill) const
{
    if (!skill)
    {
        return false;
    }

    SkillStatusMap::const_iterator itr = mSkillStatus.find(skill);
    return (itr != mSkillStatus.end() && itr->second.uState != SKILL_DELETED);
}

/**
 * @brief Gets the total current value of a skill including bonuses.
 *
 * @param skill The skill identifier to query.
 * @return The current effective skill value.
 */
uint16 Player::GetSkillValue(uint32 skill) const
{
    if (!skill)
    {
        return 0;
    }

    SkillStatusMap::const_iterator itr = mSkillStatus.find(skill);
    if (itr == mSkillStatus.end())
    {
        return 0;
    }

    SkillStatusData const& skillStatus = itr->second;
    if (skillStatus.uState == SKILL_DELETED)
    {
        return 0;
    }

    uint32 bonus = GetUInt32Value(PLAYER_SKILL_BONUS_INDEX(skillStatus.pos));

    int32 result = int32(SKILL_VALUE(GetUInt32Value(PLAYER_SKILL_VALUE_INDEX(skillStatus.pos))));
    result += SKILL_TEMP_BONUS(bonus);
    result += SKILL_PERM_BONUS(bonus);
    return result < 0 ? 0 : result;
}

/**
 * @brief Gets the total maximum value of a skill including bonuses.
 *
 * @param skill The skill identifier to query.
 * @return The effective maximum skill value.
 */
uint16 Player::GetMaxSkillValue(uint32 skill) const
{
    if (!skill)
    {
        return 0;
    }

    SkillStatusMap::const_iterator itr = mSkillStatus.find(skill);
    if (itr == mSkillStatus.end())
    {
        return 0;
    }

    SkillStatusData const& skillStatus = itr->second;
    if (skillStatus.uState == SKILL_DELETED)
    {
        return 0;
    }

    uint32 bonus = GetUInt32Value(PLAYER_SKILL_BONUS_INDEX(skillStatus.pos));

    int32 result = int32(SKILL_MAX(GetUInt32Value(PLAYER_SKILL_VALUE_INDEX(skillStatus.pos))));
    result += SKILL_TEMP_BONUS(bonus);
    result += SKILL_PERM_BONUS(bonus);
    return result < 0 ? 0 : result;
}

/**
 * @brief Gets the unmodified maximum value of a skill.
 *
 * @param skill The skill identifier to query.
 * @return The stored maximum skill value without bonuses.
 */
uint16 Player::GetPureMaxSkillValue(uint32 skill) const
{
    if (!skill)
    {
        return 0;
    }

    SkillStatusMap::const_iterator itr = mSkillStatus.find(skill);
    if (itr == mSkillStatus.end())
    {
        return 0;
    }

    SkillStatusData const& skillStatus = itr->second;
    if (skillStatus.uState == SKILL_DELETED)
    {
        return 0;
    }

    return SKILL_MAX(GetUInt32Value(PLAYER_SKILL_VALUE_INDEX(skillStatus.pos)));
}

/**
 * @brief Gets the base value of a skill including permanent bonuses.
 *
 * @param skill The skill identifier to query.
 * @return The base skill value with permanent bonuses applied.
 */
uint16 Player::GetBaseSkillValue(uint32 skill) const
{
    if (!skill)
    {
        return 0;
    }

    SkillStatusMap::const_iterator itr = mSkillStatus.find(skill);
    if (itr == mSkillStatus.end())
    {
        return 0;
    }

    SkillStatusData const& skillStatus = itr->second;
    if (skillStatus.uState == SKILL_DELETED)
    {
        return 0;
    }

    int32 result = int32(SKILL_VALUE(GetUInt32Value(PLAYER_SKILL_VALUE_INDEX(skillStatus.pos))));
    result += SKILL_PERM_BONUS(GetUInt32Value(PLAYER_SKILL_BONUS_INDEX(skillStatus.pos)));
    return result < 0 ? 0 : result;
}

/**
 * @brief Gets the raw stored value of a skill without bonuses.
 *
 * @param skill The skill identifier to query.
 * @return The raw stored skill value.
 */
uint16 Player::GetPureSkillValue(uint32 skill) const
{
    if (!skill)
    {
        return 0;
    }

    SkillStatusMap::const_iterator itr = mSkillStatus.find(skill);
    if (itr == mSkillStatus.end())
    {
        return 0;
    }

    SkillStatusData const& skillStatus = itr->second;
    if (skillStatus.uState == SKILL_DELETED)
    {
        return 0;
    }

    return SKILL_VALUE(GetUInt32Value(PLAYER_SKILL_VALUE_INDEX(skillStatus.pos)));
}

/**
 * @brief Gets the permanent bonus value applied to a skill.
 *
 * @param skill The skill identifier to query.
 * @return The permanent skill bonus.
 */
int16 Player::GetSkillPermBonusValue(uint32 skill) const
{
    if (!skill)
    {
        return 0;
    }

    SkillStatusMap::const_iterator itr = mSkillStatus.find(skill);
    if (itr == mSkillStatus.end())
    {
        return 0;
    }

    SkillStatusData const& skillStatus = itr->second;
    if (skillStatus.uState == SKILL_DELETED)
    {
        return 0;
    }

    return SKILL_PERM_BONUS(GetUInt32Value(PLAYER_SKILL_BONUS_INDEX(skillStatus.pos)));
}

/**
 * @brief Gets the temporary bonus value applied to a skill.
 *
 * @param skill The skill identifier to query.
 * @return The temporary skill bonus.
 */
int16 Player::GetSkillTempBonusValue(uint32 skill) const
{
    if (!skill)
    {
        return 0;
    }

    SkillStatusMap::const_iterator itr = mSkillStatus.find(skill);
    if (itr == mSkillStatus.end())
    {
        return 0;
    }

    SkillStatusData const& skillStatus = itr->second;
    if (skillStatus.uState == SKILL_DELETED)
    {
        return 0;
    }

    return SKILL_TEMP_BONUS(GetUInt32Value(PLAYER_SKILL_BONUS_INDEX(skillStatus.pos)));
}



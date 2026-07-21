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
 * @file PlayerRegen.cpp
 * @brief Cohesion split of Player.cpp -- health / power regeneration tick.
 *        Same `Player` class; no behaviour change.
 */

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
#include "DisableMgr.h"
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif /* ENABLE_ELUNA */
#include <cmath>

/**
 * @brief Awards rage generated from dealing or receiving damage.
 *
 * @param damage The raw damage amount used for rage generation.
 * @param attacker True when the player dealt the damage; false when the player received it.
 */
void Player::RewardRage(uint32 damage, uint32 weaponSpeedHitFactor, bool attacker)
{
    float addRage;

    float rageconversion = float((0.0091107836 * getLevel() * getLevel()) + 3.225598133 * getLevel()) + 4.2652911f;

    if (attacker)
    {
        addRage = ((damage / rageconversion * 7.5f + weaponSpeedHitFactor) / 2.0f);

        // talent who gave more rage on attack
        addRage *= 1.0f + GetTotalAuraModifier(SPELL_AURA_MOD_RAGE_FROM_DAMAGE_DEALT) / 100.0f;
    }
    else
    {
        addRage = damage / rageconversion * 2.5f;

        // Berserker Rage effect
        if (HasAura(18499, EFFECT_INDEX_0))
        {
            addRage *= 1.3f;
        }
    }

    addRage *= sWorld.getConfig(CONFIG_FLOAT_RATE_POWER_RAGE_INCOME);

    ModifyPower(POWER_RAGE, uint32(addRage * 10));
}

/**
 * @brief Regenerates the player's health and power resources for the current tick.
 */
void Player::RegenerateAll(uint32 diff)
{
    // Not in combat or they have regeneration
    if (!IsInCombat() || HasAuraType(SPELL_AURA_MOD_REGEN_DURING_COMBAT) ||
        HasAuraType(SPELL_AURA_MOD_HEALTH_REGEN_IN_COMBAT) || IsPolymorphed())
    {
        RegenerateHealth(diff);
        if (!IsInCombat() && !HasAuraType(SPELL_AURA_INTERRUPT_REGEN))
        {
            Regenerate(POWER_RAGE, diff);
            if (getClass() == CLASS_DEATH_KNIGHT)
            {
                Regenerate(POWER_RUNIC_POWER, diff);
            }
        }
    }

    Regenerate(POWER_ENERGY, diff);

    Regenerate(POWER_MANA, diff);

    if (getClass() == CLASS_DEATH_KNIGHT)
    {
        Regenerate(POWER_RUNE, diff);
    }

    m_regenTimer = REGEN_TIME_FULL;
}

/**
 * @brief Regenerates or decays a specific player power type.
 *
 * @param power The power type to update.
 */
// diff contains the time in milliseconds since last regen.
void Player::Regenerate(Powers power, uint32 diff)
{
    uint32 curValue = GetPower(power);
    uint32 maxValue = GetMaxPower(power);

    float addvalue = 0.0f;

    switch (power)
    {
        case POWER_MANA:
        {
            if (HasAuraType(SPELL_AURA_STOP_NATURAL_MANA_REGEN))
            {
                break;
            }
            bool recentCast = IsUnderLastManaUseEffect();
            float ManaIncreaseRate = sWorld.getConfig(CONFIG_FLOAT_RATE_POWER_MANA);
            if (recentCast)
            {
                // Mangos Updates Mana in intervals of 2s, which is correct
                addvalue = GetFloatValue(UNIT_FIELD_POWER_REGEN_INTERRUPTED_FLAT_MODIFIER) *  ManaIncreaseRate * 2.00f;
            }
            else
            {
                addvalue = GetFloatValue(UNIT_FIELD_POWER_REGEN_FLAT_MODIFIER) * ManaIncreaseRate * 2.00f;
            }
        }   break;
        case POWER_RAGE:                                    // Regenerate rage
        {
            float RageDecreaseRate = sWorld.getConfig(CONFIG_FLOAT_RATE_POWER_RAGE_LOSS);
            addvalue = 20 * RageDecreaseRate;               // 2 rage by tick (= 2 seconds => 1 rage/sec)
        }   break;
        case POWER_ENERGY:                                  // Regenerate energy (rogue)
        {
            float EnergyRate = sWorld.getConfig(CONFIG_FLOAT_RATE_POWER_ENERGY);
            addvalue = 20 * EnergyRate;
            break;
        }
        case POWER_RUNIC_POWER:
        {
            float RunicPowerDecreaseRate = sWorld.getConfig(CONFIG_FLOAT_RATE_POWER_RUNICPOWER_LOSS);
            addvalue = 30 * RunicPowerDecreaseRate;         // 3 RunicPower by tick
            break;
        }
        case POWER_RUNE:
        {
            if (getClass() != CLASS_DEATH_KNIGHT)
            {
                break;
            }

            for (uint32 rune = 0; rune < MAX_RUNES; ++rune)
            {
                if (uint16 cd = GetRuneCooldown(rune))      // if we have cooldown, reduce it...
                {
                    uint32 cd_diff = diff;
                    AuraList const& ModPowerRegenPCTAuras = GetAurasByType(SPELL_AURA_MOD_POWER_REGEN_PERCENT);
                    for (AuraList::const_iterator i = ModPowerRegenPCTAuras.begin(); i != ModPowerRegenPCTAuras.end(); ++i)
                    {
                        if ((*i)->GetModifier()->m_miscvalue == int32(power) && (*i)->GetMiscBValue() == GetCurrentRune(rune))
                        {
                            cd_diff = cd_diff * ((*i)->GetModifier()->m_amount + 100) / 100;
                        }
                    }

                    SetRuneCooldown(rune, (cd < cd_diff) ? 0 : cd - cd_diff);
                }
            }
            break;
        }
        case POWER_FOCUS:
        case POWER_HAPPINESS:
        case POWER_HEALTH:
            break;
        default:
            break;
    }

    // Mana regen calculated in Player::UpdateManaRegen()
    // Exist only for POWER_MANA, POWER_ENERGY, POWER_FOCUS auras
    if (power != POWER_MANA)
    {
        AuraList const& ModPowerRegenPCTAuras = GetAurasByType(SPELL_AURA_MOD_POWER_REGEN_PERCENT);
        for (AuraList::const_iterator i = ModPowerRegenPCTAuras.begin(); i != ModPowerRegenPCTAuras.end(); ++i)
        {
            if ((*i)->GetModifier()->m_miscvalue == int32(power))
            {
                addvalue *= ((*i)->GetModifier()->m_amount + 100) / 100.0f;
            }
        }
    }

    // addvalue computed on a 2sec basis. => update to diff time
    addvalue *= float(diff) / REGEN_TIME_FULL;

    if (power != POWER_RAGE && power != POWER_RUNIC_POWER)
    {
        curValue += uint32(addvalue);
        if (curValue > maxValue)
        {
            curValue = maxValue;
        }
    }
    else
    {
        if (curValue <= uint32(addvalue))
        {
            curValue = 0;
        }
        else
        {
            curValue -= uint32(addvalue);
        }
    }

    SetPower(power, curValue);
}

/**
 * @brief Regenerates the player's health based on state, auras, and rates.
 */
void Player::RegenerateHealth(uint32 diff)
{
    uint32 curValue = GetHealth();
    uint32 maxValue = GetMaxHealth();

    if (curValue >= maxValue)
    {
        return;
    }

    float HealthIncreaseRate = sWorld.getConfig(CONFIG_FLOAT_RATE_HEALTH);

    float addvalue = 0.0f;

    // polymorphed case
    if (IsPolymorphed())
    {
        addvalue = (float)GetMaxHealth() / 3;
    }
    // normal regen case (maybe partly in combat case)
    else if (!IsInCombat() || HasAuraType(SPELL_AURA_MOD_REGEN_DURING_COMBAT))
    {
        addvalue = OCTRegenHPPerSpirit() * HealthIncreaseRate;
        if (!IsInCombat())
        {
            AuraList const& mModHealthRegenPct = GetAurasByType(SPELL_AURA_MOD_HEALTH_REGEN_PERCENT);
            for (AuraList::const_iterator i = mModHealthRegenPct.begin(); i != mModHealthRegenPct.end(); ++i)
            {
                addvalue *= (100.0f + (*i)->GetModifier()->m_amount) / 100.0f;
            }
        }
        else if (HasAuraType(SPELL_AURA_MOD_REGEN_DURING_COMBAT))
        {
            addvalue *= GetTotalAuraModifier(SPELL_AURA_MOD_REGEN_DURING_COMBAT) / 100.0f;
        }

        if (!IsStandState())
        {
            addvalue *= 1.5;
        }
    }

    // always regeneration bonus (including combat)
    addvalue += GetTotalAuraModifier(SPELL_AURA_MOD_HEALTH_REGEN_IN_COMBAT);

    if (addvalue < 0)
    {
        addvalue = 0;
    }

    addvalue *= (float)diff / REGEN_TIME_FULL;

    ModifyHealth(int32(addvalue));
}

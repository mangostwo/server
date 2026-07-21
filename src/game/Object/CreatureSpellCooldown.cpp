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
 * @file CreatureSpellCooldown.cpp
 * @brief Cohesion split of Creature.cpp -- creature spell-cooldown management.
 */

#include "Common/TimeConstants.h"
#include "Creature.h"
#include "DBCStores.h"
#include "SpellMgr.h"

/**
 * @brief Stores an absolute cooldown end time for a creature spell.
 *
 * @param spell_id The spell identifier.
 * @param end_time The cooldown end time.
 */
void Creature::_AddCreatureSpellCooldown(uint32 spell_id, time_t end_time)
{
    m_CreatureSpellCooldowns[spell_id] = end_time;
}

/**
 * @brief Stores the application time for a spell category cooldown.
 *
 * @param category The spell category.
 * @param apply_time The time when the category cooldown started.
 */
void Creature::_AddCreatureCategoryCooldown(uint32 category, time_t apply_time)
{
    m_CreatureCategoryCooldowns[category] = apply_time;
}

void Creature::_ProhibitSpellSchool(SpellSchoolMask idSchoolMask, time_t end_time)
{
    m_CreatureSchoolProhibition[idSchoolMask] = end_time;
}

/**
 * @brief Adds cooldown tracking for a creature spell and its category.
 *
 * @param spellid The spell identifier.
 */
void Creature::AddCreatureSpellCooldown(uint32 spellid)
{
    SpellEntry const* spellInfo = sSpellStore.LookupEntry(spellid);
    if (!spellInfo)
    {
        return;
    }

    uint32 cooldown = GetSpellRecoveryTime(spellInfo);
    if (cooldown)
    {
        _AddCreatureSpellCooldown(spellid, time(NULL) + cooldown / IN_MILLISECONDS);
    }

    if (spellInfo->Category)
    {
        _AddCreatureCategoryCooldown(spellInfo->Category, time(NULL));
    }
}

/**
 * @brief Checks whether a spell category cooldown is still active.
 *
 * @param spell_id The spell identifier.
 * @return true if the category cooldown is active; otherwise, false.
 */
bool Creature::HasCategoryCooldown(uint32 spell_id) const
{
    SpellEntry const* spellInfo = sSpellStore.LookupEntry(spell_id);
    if (!spellInfo)
    {
        return false;
    }

    CreatureSpellCooldowns::const_iterator itr = m_CreatureCategoryCooldowns.find(spellInfo->Category);
    return (itr != m_CreatureCategoryCooldowns.end() && time_t(itr->second + (spellInfo->CategoryRecoveryTime / IN_MILLISECONDS)) > time(NULL));
}

/**
 * @brief Gets the remaining cooldown delay for a creature spell.
 *
 * @param spellId The spell identifier.
 * @return Remaining cooldown in seconds.
 */
uint32 Creature::GetCreatureSpellCooldownDelay(uint32 spellId) const
{
    CreatureSpellCooldowns::const_iterator itr = m_CreatureSpellCooldowns.find(spellId);
    time_t t = time(NULL);
    return uint32(itr != m_CreatureSpellCooldowns.end() && itr->second > t ? itr->second - t : 0);
}

/**
 * @brief Checks whether a spell or its category is currently on cooldown.
 *
 * @param spell_id The spell identifier.
 * @return true if a cooldown is active; otherwise, false.
 */
bool Creature::HasSpellCooldown(uint32 spell_id) const
{
    CreatureSpellCooldowns::const_iterator itr = m_CreatureSpellCooldowns.find(spell_id);
    return (itr != m_CreatureSpellCooldowns.end() && itr->second > time(NULL)) || HasCategoryCooldown(spell_id);
}

bool Creature::HasSchoolProhibition(SpellSchoolMask idSchoolMask) const
{
    time_t curTime = time(NULL);
    for (CreatureSchoolProhibition::const_iterator itr = m_CreatureSchoolProhibition.begin(); itr != m_CreatureSchoolProhibition.end(); ++itr)
    {
        if ((idSchoolMask & itr->first) && itr->second > curTime)
        {
            return true;
        }
    }
    return false;
}

void Creature::ProhibitSpellSchool(SpellSchoolMask idSchoolMask, uint32 unTimeMs)
{
    time_t curTime = time(NULL);

    // Blanket school prohibition for spells cast via event logic
    _ProhibitSpellSchool(idSchoolMask, curTime + (unTimeMs / IN_MILLISECONDS));

    // Individual spell cooldowns for spells in the creature spell table
    for (uint32 i = 0; i < CREATURE_MAX_SPELLS; ++i)
    {
        if (!m_spells[i])
        {
            continue;
        }

        SpellEntry const* spellInfo = sSpellStore.LookupEntry(m_spells[i]);
        if (!spellInfo)
        {
            continue;
        }

        if ((idSchoolMask & GetSpellSchoolMask(spellInfo)) && GetCreatureSpellCooldownDelay(m_spells[i]) < unTimeMs)
        {
            _AddCreatureSpellCooldown(m_spells[i], curTime + (unTimeMs / IN_MILLISECONDS));
        }
    }
}

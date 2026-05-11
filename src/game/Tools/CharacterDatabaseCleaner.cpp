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
 * @file CharacterDatabaseCleaner.cpp
 * @brief Character database cleanup utility
 *
 * This file implements the CharacterDatabaseCleaner which removes invalid
 * or orphaned data from the character database. It validates data against
 * DBC stores to ensure only legitimate entries remain.
 *
 * Cleanup operations:
 * - Remove invalid skills (not in SkillLine.dbc)
 * - Remove invalid spells (not in Spell.dbc)
 *
 * The cleaner uses flags stored in the `saved_variables` table to determine
 * which cleanup operations are needed. This allows incremental cleaning
 * across server restarts.
 *
 * @see CharacterDatabaseCleaner for the cleaner interface
 * @see CLEANING_FLAG_* constants for available operations
 */

#include "Common.h"
#include "CharacterDatabaseCleaner.h"
#include "World.h"
#include "Database/DatabaseEnv.h"
#include "DBCStores.h"
#include "ProgressBar.h"

/**
 * @brief Main database cleanup entry point
 *
 * Performs database cleaning based on configuration and stored flags:
 * 1. Checks if cleaning is enabled in config
 * 2. Reads cleaning flags from saved_variables table
 * 3. Executes appropriate cleanup routines
 * 4. Resets cleaning flags when complete
 *
 * @note Can be disabled via CONFIG_BOOL_CLEAN_CHARACTER_DB
 */
void CharacterDatabaseCleaner::CleanDatabase()
{
    // Skip if disabled in config
    if (!sWorld.getConfig(CONFIG_BOOL_CLEAN_CHARACTER_DB))
    {
        return;
    }

    sLog.outString("Cleaning character database...");

    // Check which cleanups are needed
    QueryResult* result = CharacterDatabase.PQuery("SELECT `cleaning_flags` FROM `saved_variables`");
    if (!result)
    {
        return;
    }
    uint32 flags = (*result)[0].GetUInt32();
    delete result;

    // clean up
    if (flags & CLEANING_FLAG_ACHIEVEMENT_PROGRESS)
    {
        CleanCharacterAchievementProgress();
    }

    // Execute cleanup based on flags
    if (flags & CLEANING_FLAG_SKILLS)
    {
        CleanCharacterSkills();
    }
    if (flags & CLEANING_FLAG_SPELLS)
    {
        CleanCharacterSpell();
    }
    if (flags & CLEANING_FLAG_TALENTS)
    {
        CleanCharacterTalent();
    }
    CharacterDatabase.Execute("UPDATE `saved_variables` SET `cleaning_flags` = 0");
}

/**
 * @brief Validate and remove invalid entries from a table
 * @param column Column name containing the ID to check
 * @param table Table name to clean
 * @param check Validation function returning true if ID is valid
 *
 * Generic cleanup helper that:
 * 1. Queries all distinct values in the specified column
 * 2. Validates each value using the provided check function
 * 3. Deletes rows with invalid IDs in a single DELETE statement
 *
 * Uses a progress bar for visual feedback during long operations.
 */
void CharacterDatabaseCleaner::CheckUnique(const char* column, const char* table, bool (*check)(uint32))
{
    QueryResult* result = CharacterDatabase.PQuery("SELECT DISTINCT `%s` FROM `%s`", column, table);
    if (!result)
    {
        sLog.outString("Table %s is empty.", table);
        return;
    }

    bool found = false;
    std::ostringstream ss;
    BarGoLink bar(result->GetRowCount());
    do
    {
        bar.step();

        Field* fields = result->Fetch();

        uint32 id = fields[0].GetUInt32();

        if (!check(id))
        {
            if (!found)
            {
                ss << "DELETE FROM `" << table << "` WHERE `" << column << "` IN (";
                found = true;
            }
            else
            {
                ss << ",";
            }
            ss << id;
        }
    }
    while (result->NextRow());
    delete result;

    if (found)
    {
        ss << ")";
        CharacterDatabase.Execute(ss.str().c_str());
    }
}

bool CharacterDatabaseCleaner::AchievementProgressCheck(uint32 criteria)
{
    return sAchievementCriteriaStore.LookupEntry(criteria);
}

void CharacterDatabaseCleaner::CleanCharacterAchievementProgress()
{
    CheckUnique("criteria", "character_achievement_progress", &AchievementProgressCheck);
}

/**
 * @brief Check if a skill is valid
 * @param skill Skill ID to validate
 * @return true if skill exists in DBC
 *
 * Validates against SkillLine.dbc (sSkillLineStore).
 */
bool CharacterDatabaseCleaner::SkillCheck(uint32 skill)
{
    return sSkillLineStore.LookupEntry(skill);
}

/**
 * @brief Clean invalid character skills
 *
 * Removes skills from character_skills table that don't exist
 * in the SkillLine.dbc file.
 */
void CharacterDatabaseCleaner::CleanCharacterSkills()
{
    CheckUnique("skill", "character_skills", &SkillCheck);
}

/**
 * @brief Check if a spell is valid
 * @param spell_id Spell ID to validate
 * @return true if spell exists in DBC
 *
 * Validates against Spell.dbc (sSpellStore).
 */
bool CharacterDatabaseCleaner::SpellCheck(uint32 spell_id)
{
    return sSpellStore.LookupEntry(spell_id) && !GetTalentSpellPos(spell_id);
}

/**
 * @brief Clean invalid character spells
 *
 * Removes spells from character_spell table that don't exist
 * in the Spell.dbc file.
 */
void CharacterDatabaseCleaner::CleanCharacterSpell()
{
    CheckUnique("spell", "character_spell", &SpellCheck);
}

bool CharacterDatabaseCleaner::TalentCheck(uint32 talent_id)
{
    TalentEntry const* talentInfo = sTalentStore.LookupEntry(talent_id);
    if (!talentInfo)
    {
        return false;
    }

    return sTalentTabStore.LookupEntry(talentInfo->TalentTab);
}

void CharacterDatabaseCleaner::CleanCharacterTalent()
{
    CharacterDatabase.DirectPExecute("DELETE FROM `character_talent` WHERE `spec` > %u OR `current_rank` > %u", MAX_TALENT_SPEC_COUNT, MAX_TALENT_RANK);

    CheckUnique("talent_id", "character_talent", &TalentCheck);
}

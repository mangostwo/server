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
 * @file ObjectMgrSpellData.cpp
 * @brief Cohesion split of ObjectMgr.cpp -- creature spell-click, creature-template spell and fishing-skill loaders.
 */

#include "ObjectMgr.h"
#include "Database/DatabaseEnv.h"
#include "Log.h"
#include "ProgressBar.h"
#include "SQLStorages.h"
#include "DBCStores.h"
#include "SpellMgr.h"

void ObjectMgr::LoadNPCSpellClickSpells()
{
    uint32 count = 0;

    mSpellClickInfoMap.clear();
    //                                                 0            1           2              3                     4            5             6
    QueryResult* result = WorldDatabase.Query("SELECT `npc_entry`, `spell_id`, `quest_start`, `quest_start_active`, `quest_end`, `cast_flags`, `condition_id` FROM `npc_spellclick_spells`");

    if (!result)
    {
        BarGoLink bar(1);

        bar.step();

        sLog.outString();
        sLog.outErrorDb(">> Loaded 0 spellclick spells. DB table `npc_spellclick_spells` is empty.");
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();
        bar.step();

        SpellClickInfo info;
        uint32 npc_entry         = fields[0].GetUInt32();
        info.spellId             = fields[1].GetUInt32();
        info.questStart          = fields[2].GetUInt32();
        info.questStartCanActive = fields[3].GetBool();
        info.questEnd            = fields[4].GetUInt32();
        info.castFlags           = fields[5].GetUInt8();
        info.conditionId         = fields[6].GetUInt16();

        CreatureInfo const* cInfo = GetCreatureTemplate(npc_entry);
        if (!cInfo)
        {
            sLog.outErrorDb("Table npc_spellclick_spells references unknown creature_template %u. Skipping entry.", npc_entry);
            continue;
        }

        // spell can be 0 for special or custom cases
        if (info.spellId)
        {
            SpellEntry const* spellinfo = sSpellStore.LookupEntry(info.spellId);
            if (!spellinfo)
            {
                sLog.outErrorDb("Table npc_spellclick_spells references unknown spellid %u. Skipping entry.", info.spellId);
                continue;
            }
        }

        if (info.conditionId && !sConditionStorage.LookupEntry<PlayerCondition const*>(info.conditionId))
        {
            sLog.outErrorDb("Table npc_spellclick_spells references unknown condition %u. Skipping entry.", info.conditionId);
            continue;
        }
        else if (!info.conditionId)                         // TODO Drop block after finished converting
        {
            // quest might be 0 to enable spellclick independent of any quest
            if (info.questStart && mQuestTemplates.find(info.questStart) == mQuestTemplates.end())
            {
                sLog.outErrorDb("Table npc_spellclick_spells references unknown start quest %u. Skipping entry.", info.questStart);
                continue;
            }

            // quest might be 0 to enable spellclick active infinity after start quest
            if (info.questEnd && mQuestTemplates.find(info.questEnd) == mQuestTemplates.end())
            {
                sLog.outErrorDb("Table npc_spellclick_spells references unknown end quest %u. Skipping entry.", info.questEnd);
                continue;
            }
        }

        mSpellClickInfoMap.insert(SpellClickInfoMap::value_type(npc_entry, info));

        // mark creature template as spell clickable
        const_cast<CreatureInfo*>(cInfo)->NpcFlags |= UNIT_NPC_FLAG_SPELLCLICK;

        ++count;
    }
    while (result->NextRow());

    delete result;

    sLog.outString();
    sLog.outString(">> Loaded %u spellclick definitions", count);
}

/**
 * @brief Loads base fishing skill requirements for areas.
 */
void ObjectMgr::LoadFishingBaseSkillLevel()
{
    mFishingBaseForArea.clear();                            // for reload case

    uint32 count = 0;
    QueryResult* result = WorldDatabase.Query("SELECT `entry`,`skill` FROM `skill_fishing_base_level`");

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();
        sLog.outErrorDb(">> Loaded `skill_fishing_base_level`, table is empty!");
        sLog.outString();
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        bar.step();

        Field* fields = result->Fetch();
        uint32 entry  = fields[0].GetUInt32();
        int32 skill   = fields[1].GetInt32();

        AreaTableEntry const* fArea = GetAreaEntryByAreaID(entry);
        if (!fArea)
        {
            sLog.outErrorDb("AreaId %u defined in `skill_fishing_base_level` does not exist", entry);
            continue;
        }

        mFishingBaseForArea[entry] = skill;
        ++count;
    }
    while (result->NextRow());

    delete result;

    sLog.outString(">> Loaded %u areas for fishing base skill level", count);
    sLog.outString();
}

/**
 * @brief Loads creature template spell assignments and validates their spell ids.
 */
void ObjectMgr::LoadCreatureTemplateSpells()
{
    sCreatureTemplateSpellsStorage.Load();

    for (SQLStorageBase::SQLSIterator<CreatureTemplateSpells> itr = sCreatureTemplateSpellsStorage.getDataBegin<CreatureTemplateSpells>(); itr < sCreatureTemplateSpellsStorage.getDataEnd<CreatureTemplateSpells>(); ++itr)
    {
        if (!sCreatureStorage.LookupEntry<CreatureInfo>(itr->entry))
        {
            sLog.outErrorDb("LoadCreatureTemplateSpells: Spells found for creature entry %u, but creature does not exist, skipping", itr->entry);
            sCreatureTemplateSpellsStorage.EraseEntry(itr->entry);
        }
        for (uint8 i = 0; i < CREATURE_MAX_SPELLS; ++i)
        {
            if (itr->spells[i] && !sSpellStore.LookupEntry(itr->spells[i]))
            {
                sLog.outErrorDb("LoadCreatureTemplateSpells: Spells found for creature entry %u, assigned spell %u does not exist, set to 0", itr->entry, itr->spells[i]);
                const_cast<CreatureTemplateSpells*>(*itr)->spells[i] = 0;
            }
        }
    }

    sLog.outString(">> Loaded %u creature_template_spells definitions", sCreatureTemplateSpellsStorage.GetRecordCount());
    sLog.outString();
}

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
 * @file ObjectMgrDungeonFinder.cpp
 * @brief Cohesion split of ObjectMgr.cpp -- dungeon finder requirement,
 *        reward, and item loaders. Same `ObjectMgr` class; no behaviour
 *        change. CMake `file(GLOB Object/*.cpp)` picks this file up
 *        automatically; ObjectMgr.h is unchanged.
 */

#include "Utilities/PackedValues.h"
#include "ObjectMgr.h"
#include "Database/DatabaseEnv.h"
#include "Policies/Singleton.h"
#include "DBCStores.h"
#include "Log.h"
#include "ProgressBar.h"

void ObjectMgr::LoadDungeonFinderRequirements()
{
    uint32 count = 0;
    mDungeonFinderRequirementsMap.clear();    // in case of a reload

    //                                                0      1           2               3     4       5               6            7            8
    QueryResult* result = WorldDatabase.Query("SELECT `mapId`, `difficulty`, `min_item_level`, `item`, `item_2`, `alliance_quest`, `horde_quest`, `achievement`, `quest_incomplete_text` FROM `dungeonfinder_requirements`");

    if (!result)
    {
        BarGoLink bar(1);

        bar.step();

        sLog.outString();
        sLog.outErrorDb(">> Loaded 0 dungeon finder requirements. DB table `dungeonfinder_requirements`, is empty!");
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();
        bar.step();

        uint32 mapId          = fields[0].GetUInt32();
        uint32 difficulty     = fields[1].GetUInt32();
        uint32 dungeonKey     = MAKE_PAIR32(mapId, difficulty); // for unique key

        uint32 minItemLevel   = fields[2].GetUInt32();
        uint32 item           = fields[3].GetUInt32();
        uint32 item2          = fields[4].GetUInt32();
        uint32 allianceQuest  = fields[5].GetUInt32();
        uint32 hordeQuest     = fields[6].GetUInt32();
        uint32 achievement    = fields[7].GetUInt32();
        const char* questText = fields[8].GetString();

        // check that items, quests, & achievements are real
        if (item)
        {
            ItemEntry const* dbcitem = sItemStore.LookupEntry(item);
            if (!dbcitem)
            {
                sLog.outString();
                sLog.outErrorDb("Table `dungeonfinder_requirements` has invalid item entry %u for map %u ! Removing requirement.", item, mapId);
                item = 0;
            }
        }

        if (item2)
        {
            ItemEntry const* dbcitem = sItemStore.LookupEntry(item2);
            if (!dbcitem)
            {
                sLog.outString();
                sLog.outErrorDb("Table `dungeonfinder_requirements` has invalid item entry %u for map %u ! Removing requirement.", item2, mapId);
                item2 = 0;
            }
        }

        if (allianceQuest)
        {
            QuestMap::iterator qReqItr = mQuestTemplates.find(allianceQuest);
            if (qReqItr == mQuestTemplates.end())
            {
                sLog.outString();
                sLog.outErrorDb("Table `dungeonfinder_requirements` has invalid quest requirement %u for map %u ! Removing requirement.", allianceQuest, mapId);
                allianceQuest = 0;
            }
        }

        if (hordeQuest)
        {
            QuestMap::iterator qReqItr = mQuestTemplates.find(hordeQuest);
            if (qReqItr == mQuestTemplates.end())
            {
                sLog.outString();
                sLog.outErrorDb("Table `dungeonfinder_requirements` has invalid quest requirement %u for map %u ! Removing requirement.", hordeQuest, mapId);
                hordeQuest = 0;
            }
        }

        if (achievement)
        {
            if (!sAchievementStore.LookupEntry(achievement))
            {
                sLog.outString();
                sLog.outErrorDb("Table `dungeonfinder_requirements` has invalid achievement %u for map %u ! Removing requirement.", achievement, mapId);
                achievement = 0;
            }
        }

        // add to map after checks
        DungeonFinderRequirements requirement(minItemLevel, item, item2, allianceQuest, hordeQuest, achievement, questText);
        mDungeonFinderRequirementsMap[dungeonKey] = requirement;

        ++count;
    }
    while (result->NextRow());

    delete result;

    sLog.outString();
    sLog.outString(">> Loaded %u Dungeon Finder Requirements", count);
}

void ObjectMgr::LoadDungeonFinderRewards()
{
    uint32 count = 0;
    mDungeonFinderRewardsMap.clear();    // in case of a reload

    //                                                0   1      2               3
    QueryResult* result = WorldDatabase.Query("SELECT `id`, `level`, `base_xp_reward`, `base_monetary_reward` FROM `dungeonfinder_rewards`");

    if (!result)
    {
        BarGoLink bar(1);

        bar.step();

        sLog.outString();
        sLog.outErrorDb(">> Loaded 0 dungeon finder rewards. DB table `dungeonfinder_rewards`, is empty!");
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();
        bar.step();

        uint32 level     = fields[1].GetUInt32();
        uint32 baseXP    = fields[2].GetUInt32();
        int32 baseMoney  = fields[3].GetInt32();

        DungeonFinderRewards reward(baseXP, baseMoney);
        mDungeonFinderRewardsMap[level] = reward;

        ++count;
    }
    while (result->NextRow());

    delete result;

    sLog.outString();
    sLog.outString(">> Loaded %u Dungeon Finder Rewards", count);
}

void ObjectMgr::LoadDungeonFinderItems()
{
    uint32 count = 0;
    mDungeonFinderItemsMap.clear(); // in case of reload

    //                                                0   1          2          3            4            5
    QueryResult* result = WorldDatabase.Query("SELECT `id`, `min_level`, `max_level`, `item_reward`, `item_amount`, `dungeon_type` FROM `dungeonfinder_item_rewards`");

    if (!result)
    {
        BarGoLink bar(1);

        bar.step();

        sLog.outString();
        sLog.outErrorDb(">> Loaded 0 dungeon finder items. DB table `dungeonfinder_item_rewards`, is empty!");
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();
        bar.step();

        uint32 id          = fields[0].GetUInt32();
        uint32 minLevel    = fields[1].GetUInt32();
        uint32 maxLevel    = fields[2].GetUInt32();
        uint32 itemReward  = fields[3].GetInt32();
        uint32 itemAmount  = fields[4].GetUInt32();
        uint32 dungeonType = fields[5].GetUInt32();

        DungeonFinderItems rewardItems(minLevel, maxLevel, itemReward, itemAmount, dungeonType);
        mDungeonFinderItemsMap[id] = rewardItems;

        ++count;
    }
    while (result->NextRow());

    delete result;

    sLog.outString();
    sLog.outString(">> Loaded %u Dungeon Finder Items", count);
}

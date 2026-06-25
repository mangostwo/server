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
 * @file ObjectMgrInstanceData.cpp
 * @brief Cohesion split of ObjectMgr.cpp -- instance encounter, group and arena-team loaders.
 */

#include "ObjectMgr.h"
#include "Database/DatabaseEnv.h"
#include "Log.h"
#include "ProgressBar.h"
#include "SQLStorages.h"
#include "DBCStores.h"
#include "ArenaTeam.h"
#include "Group.h"

void ObjectMgr::LoadArenaTeams()
{
    uint32 count = 0;

    //                                                     0                          1      2             3      4                 5
    QueryResult* result = CharacterDatabase.Query("SELECT `arena_team`.`arenateamid`,`name`,`captainguid`,`type`,`BackgroundColor`,`EmblemStyle`,"
                          //   6          7             8              9        10           11          12             13            14
                          "`EmblemColor`,`BorderStyle`,`BorderColor`, `rating`,`games_week`,`wins_week`,`games_season`,`wins_season`,`rank` "
                          "FROM `arena_team` LEFT JOIN `arena_team_stats` ON `arena_team`.`arenateamid` = `arena_team_stats`.`arenateamid` ORDER BY `arena_team`.`arenateamid` ASC");

    if (!result)
    {
        BarGoLink bar(1);

        bar.step();

        sLog.outString();
        sLog.outString(">> Loaded %u arenateam definitions", count);
        return;
    }

    // load arena_team members
    QueryResult* arenaTeamMembersResult = CharacterDatabase.Query(
            //          0                   1      2             3           4               5             6                 7      8
            "SELECT `arenateamid`,`member`.`guid`,`played_week`,`wons_week`,`played_season`,`wons_season`,`personal_rating`,`name`,`class` "
            "FROM `arena_team_member` member LEFT JOIN `characters` chars on member.`guid` = chars.`guid` ORDER BY member.`arenateamid` ASC");

    BarGoLink bar(result->GetRowCount());

    do
    {
        bar.step();
        ++count;

        ArenaTeam* newArenaTeam = new ArenaTeam;
        if (!newArenaTeam->LoadArenaTeamFromDB(result) ||
                !newArenaTeam->LoadMembersFromDB(arenaTeamMembersResult))
        {
            newArenaTeam->Disband(NULL);
            delete newArenaTeam;
            continue;
        }
        AddArenaTeam(newArenaTeam);
    }
    while (result->NextRow());

    delete result;
    delete arenaTeamMembersResult;

    sLog.outString();
    sLog.outString(">> Loaded %u arenateam definitions", count);
}

/**
 * @brief Loads groups, group members, and group instance bindings from the database.
 */
void ObjectMgr::LoadGroups()
{
    // -- loading groups --
    uint32 count = 0;
    //                                                     0           1                2             3             4                5        6        7        8        9        10       11       12       13           14            15                16            17
    QueryResult* result = CharacterDatabase.Query("SELECT `mainTank`, `mainAssistant`, `lootMethod`, `looterGuid`, `lootThreshold`, `icon1`, `icon2`, `icon3`, `icon4`, `icon5`, `icon6`, `icon7`, `icon8`, `groupType`, `difficulty`, `raiddifficulty`, `leaderGuid`, `groupId` FROM `groups`");

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();
        sLog.outString(">> Loaded %u group definitions", count);
        sLog.outString();
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        bar.step();
        Field* fields = result->Fetch();
        ++count;
        Group* group = new Group;
        if (!group->LoadGroupFromDB(fields))
        {
            group->Disband();
            delete group;
            continue;
        }
        AddGroup(group);
    }
    while (result->NextRow());

    delete result;

    sLog.outString(">> Loaded %u group definitions", count);
    sLog.outString();

    // -- loading members --
    count = 0;
    //                                       0           1          2         3
    result = CharacterDatabase.Query("SELECT `memberGuid`, `assistant`, `subgroup`, `groupId` FROM `group_member` ORDER BY `groupId`");
    if (!result)
    {
        BarGoLink bar2(1);
        bar2.step();
    }
    else
    {
        Group* group = NULL;                                // used as cached pointer for avoid relookup group for each member

        BarGoLink bar2(result->GetRowCount());
        do
        {
            bar2.step();
            Field* fields = result->Fetch();
            ++count;

            uint32 memberGuidlow = fields[0].GetUInt32();
            ObjectGuid memberGuid = ObjectGuid(HIGHGUID_PLAYER, memberGuidlow);
            bool   assistent     = fields[1].GetBool();
            uint8  subgroup      = fields[2].GetUInt8();
            uint32 groupId       = fields[3].GetUInt32();
            if (!group || group->GetId() != groupId)
            {
                group = GetGroupById(groupId);
                if (!group)
                {
                    sLog.outErrorDb("Incorrect entry in group_member table : no group with Id %d for member %s!",
                                    groupId, memberGuid.GetString().c_str());
                    CharacterDatabase.PExecute("DELETE FROM `group_member` WHERE `memberGuid` = '%u'", memberGuidlow);
                    continue;
                }
            }

            if (!group->LoadMemberFromDB(memberGuidlow, subgroup, assistent))
            {
                sLog.outErrorDb("Incorrect entry in group_member table : member %s can not be added to group (Id: %u)!",
                                memberGuid.GetString().c_str(), groupId);
                CharacterDatabase.PExecute("DELETE FROM `group_member` WHERE `memberGuid` = '%u'", memberGuidlow);
            }
        }
        while (result->NextRow());
        delete result;
    }

    // clean groups
    // TODO: maybe delete from the DB before loading in this case
    for (GroupMap::iterator itr = mGroupMap.begin(); itr != mGroupMap.end();)
    {
        if (itr->second->GetMembersCount() < 2)
        {
            itr->second->Disband();
            delete itr->second;
            mGroupMap.erase(itr++);
        }
        else
        {
            ++itr;
        }
    }

    // -- loading instances --
    count = 0;
    result = CharacterDatabase.Query(
                 //                        0             1      2           3                       4             5
                 "SELECT `group_instance`.`leaderGuid`, `map`, `instance`, `permanent`, `instance`.`difficulty`, `resettime`, "
                 // 6
                 "(SELECT COUNT(*) FROM `character_instance` WHERE `guid` = `group_instance`.`leaderGuid` AND `instance` = `group_instance`.`instance` AND `permanent` = 1 LIMIT 1), "
                 // 7                              8
                 " `groups`.`groupId`, `instance`.`encountersMask` "
                 "FROM `group_instance` LEFT JOIN `instance` ON `instance` = `id` LEFT JOIN `groups` ON `groups`.`leaderGUID` = `group_instance`.`leaderGUID` ORDER BY `leaderGuid`"
             );

    if (!result)
    {
        BarGoLink bar2(1);
        bar2.step();
    }
    else
    {
        Group* group = NULL;                                // used as cached pointer for avoid relookup group for each member

        BarGoLink bar2(result->GetRowCount());
        do
        {
            bar2.step();
            Field* fields = result->Fetch();
            ++count;

            uint32 leaderGuidLow = fields[0].GetUInt32();
            uint32 mapId = fields[1].GetUInt32();
            Difficulty diff = (Difficulty)fields[4].GetUInt8();
            uint32 groupId = fields[7].GetUInt32();

            if (!group || group->GetId() != groupId)
            {
                // find group id in map by leader low guid
                group = GetGroupById(groupId);
                if (!group)
                {
                    sLog.outErrorDb("Incorrect entry in group_instance table : no group with leader %d", leaderGuidLow);
                    continue;
                }
            }

            MapEntry const* mapEntry = sMapStore.LookupEntry(mapId);
            if (!mapEntry || !mapEntry->IsDungeon())
            {
                sLog.outErrorDb("Incorrect entry in group_instance table : no dungeon map %d", mapId);
                continue;
            }

            if (diff >= (mapEntry->IsRaid() ? MAX_RAID_DIFFICULTY : MAX_DUNGEON_DIFFICULTY))
            {
                sLog.outErrorDb("Wrong dungeon difficulty use in group_instance table: %d", diff + 1);
                diff = REGULAR_DIFFICULTY;                  // default for both difficaly types
            }

            DungeonPersistentState* state = (DungeonPersistentState*)sMapPersistentStateMgr.AddPersistentState(mapEntry, fields[2].GetUInt32(), Difficulty(diff), (time_t)fields[5].GetUInt64(), (fields[6].GetUInt32() == 0), true, true, fields[8].GetUInt32());
            group->BindToInstance(state, fields[3].GetBool(), true);
        }
        while (result->NextRow());
        delete result;
    }

    sLog.outString(">> Loaded %u group-instance binds total", count);
    sLog.outString();

    sLog.outString(">> Loaded %u group members total", count);
    sLog.outString();
}

void ObjectMgr::LoadInstanceEncounters()
{
    m_DungeonEncounters.clear();         // need for reload case

    QueryResult* result = WorldDatabase.Query("SELECT `entry`, `creditType`, `creditEntry`, `lastEncounterDungeon` FROM `instance_encounters`");

    if (!result)
    {
        BarGoLink bar(1);

        bar.step();

        sLog.outString();
        sLog.outString(">> Loaded 0 Instance Encounters. DB table `instance_encounters` is empty.");
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();
        bar.step();

        uint32 entry = fields[0].GetUInt32();
        DungeonEncounterEntry const* dungeonEncounter = sDungeonEncounterStore.LookupEntry(entry);

        if (!dungeonEncounter)
        {
            sLog.outErrorDb("Table `instance_encounters` has an invalid encounter id %u, skipped!", entry);
            continue;
        }

        uint8 creditType = fields[1].GetUInt8();
        uint32 creditEntry = fields[2].GetUInt32();
        switch (creditType)
        {
            case ENCOUNTER_CREDIT_KILL_CREATURE:
            {
                CreatureInfo const* cInfo = sCreatureStorage.LookupEntry<CreatureInfo>(creditEntry);
                if (!cInfo)
                {
                    sLog.outErrorDb("Table `instance_encounters` has an invalid creature (entry %u) linked to the encounter %u (%s), skipped!", creditEntry, entry, dungeonEncounter->encounterName[0]);
                    continue;
                }
                break;
            }
            case ENCOUNTER_CREDIT_CAST_SPELL:
            {
                if (!sSpellStore.LookupEntry(creditEntry))
                {
                    // skip spells that aren't in dbc for now
                    // sLog.outErrorDb("Table `instance_encounters` has an invalid spell (entry %u) linked to the encounter %u (%s), skipped!", creditEntry, entry, dungeonEncounter->encounterName[0]);
                    continue;
                }
                break;
            }
            default:
                sLog.outErrorDb("Table `instance_encounters` has an invalid credit type (%u) for encounter %u (%s), skipped!", creditType, entry, dungeonEncounter->encounterName[0]);
                continue;
        }
        uint32 lastEncounterDungeon = fields[3].GetUInt32();

        m_DungeonEncounters.insert(DungeonEncounterMap::value_type(creditEntry, new DungeonEncounter(dungeonEncounter, EncounterCreditType(creditType), creditEntry, lastEncounterDungeon)));
    }
    while (result->NextRow());

    delete result;

    sLog.outString();
    sLog.outString(">> Loaded %zu Instance Encounters", m_DungeonEncounters.size());
}

/*
 * This code is part of MaNGOS. Contributor & Copyright details are in AUTHORS/THANKS.
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
 */

#include "DBCEnums.h"
#include "DBCStores.h"
#include "DBCStructure.h"
#include "GameEventMgr.h"
#include "LFGMgr.h"
#include "ObjectMgr.h"
#include "SharedDefines.h"
#include "WorldSession.h"

LFGMgr::LFGMgr() { }
LFGMgr::~LFGMgr() { }

ItemRewards LFGMgr::GetDungeonItemRewards(uint32 dungeonId, DungeonTypes type)
{
    ItemRewards rewards();
    LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(dungeonId);
    if (dungeon)
    {
        // Here we're using the target levels rather than the 
        // actual minimum and maximum levels to avoid potential
        // conflicts with the level ranges in the database
        uint32 minLevel = dungeon->targetLevelMin;
        uint32 maxLevel = dungeon->targetLevelMax;
        
        DungeonFinderItemsMap itemBuffer = sObjectMgr.GetDungeonFinderItemsMap();
        for (DungeonFinderItemsMap::iterator it = itemBuffer.start(); it != itemBuffer.end(); ++it)
        {
            DungeonFinderItems itemCache = *it.second;
            if (itemCache.dungeonType == (uint32)type)
            {
                // should only be one of this inequality in the map
                if ((itemCache.minLevel <= minLevel) && (maxLevel <= itemCache.maxLevel))
                {
                    rewards.itemId = itemCache.itemReward;
                    rewards.itemAmount = itemCache.itemAmount;
                    return rewards;
                }
            }
        }
    }
    return rewards;
}

DungeonTypes LFGMgr::GetDungeonType(uint32 dungeonId)
{
    LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(dungeonId);
    if (dungeon)
    {
        switch (dungeon->expansionLevel)
            case 0:
                return DUNGEON_CLASSIC;
            case 1:
            {
                if (dungeon->difficulty == DUNGEON_DIFFICULTY_NORMAL)
                    return DUNGEON_TBC;
                else if (dungeon->difficulty == DUNGEON_DIFFICULTY_HEROIC)
                    return DUNGEON_TBC_HEROIC;
            }
            case 2:
            {
                if (dungeon->difficulty == DUNGEON_DIFFICULTY_NORMAL)
                    return DUNGEON_WOTLK;
                else if (dungeon->difficulty == DUNGEON_DIFFICULTY_HEROIC)
                    return DUNGEON_WOTLK_HEROIC;
            }
            default:
                return NULL;
    }
}

void LFGMgr::RegisterPlayerDaily(uint32 guidLow, DungeonTypes dungeon)
{
    switch (dungeon)
    {
        case DUNGEON_CLASSIC:
        case DUNGEON_TBC:
            m_dailyAny.insert(guidLow);
            break;
        case DUNGEON_TBC_HEROIC:
            m_dailyTBCHeroic.insert(guidLow);
            break;
        case DUNGEON_WOTLK:
            m_dailyLKNormal.insert(guidLow);
            break;
        case DUNGEON_WOTLK_HEROIC:
            m_dailyLKHeroic.insert(guidLow);
            break;
        default:
            break;
    }
}

bool LFGMgr::HasPlayerDoneDaily(uint32 guidLow, DungeonTypes dungeon)
{
    switch (dungeon)
    {
        case DUNGEON_CLASSIC:
        case DUNGEON_TBC:
            return (m_dailyAny.find(guidLow) != m_dailyAny.end()) ? true : false;
        case DUNGEON_TBC_HEROIC:
            return (m_dailyTBCHeroic.find(guidLow) != m_dailyTBCHeroic.end()) ? true : false;
        case DUNGEON_WOTLK:
            return (m_dailyLKNormal.find(guidLow) != m_dailyLKNormal.end()) ? true : false;
        case DUNGEON_WOTLK_HEROIC:
            return (m_dailyLKHeroic.find(guidLow) != m_dailyLKHeroic.end()) ? true : false;
        default:
            return false;
    }
    return false;
}

void LFGMgr::ResetDailyRecords()
{
    m_dailyAny.clear();
    m_dailyTBCHeroic.clear();
    m_dailyLKNormal.clear();
    m_dailyLKHeroic.clear();
}

bool LFGMgr::IsSeasonActive(uint32 dungeonId)
{
    switch (dungeonId)
    {
        case 285:
            return IsHolidayActive(HOLIDAY_HALLOWS_END);
        case 286:
            return IsHolidayActive(HOLIDAY_FIRE_FESTIVAL);
        case 287:
            return IsHolidayActive(HOLIDAY_BREWFEST);
        case 288:
            return IsHolidayActive(HOLIDAY_LOVE_IS_IN_THE_AIR);
        default:
            return false;
    }
    return false;
}

dungeonEntries LFGMgr::FindRandomDungeonsForPlayer(uint32 level, uint8 expansion)
{
    dungeonEntries randomDungeons;
    
    // go through the dungeon dbc and select the applicable dungeons
    for (uint32 id = 0; id < sLfgDungeonsStore.GetNumRows(); ++id)
    {
        LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(id);
        if (dungeon)
        {
            if ( (dungeon->typeID == LFG_TYPE_RANDOM_DUNGEON)
                || (IsSeasonal(dungeon->flags) && IsSeasonActive(dungeon->ID)) )
                if (dungeon->expansionLevel <= expansion && dungeon->minLevel <= level
                    && dungeon->maxLevel >= level)
                    randomDungeons[dungeon->ID] = dungeon->Entry();
        }
    }
    return randomDungeons;
}

dungeonForbidden LFGMgr::FindRandomDungeonsNotForPlayer(uint32 level, uint8 expansion)
{
    dungeonForbidden randomDungeons;
    /*
     * Reasons a player cannot enter a dungeon...
     *     - level < dungeon->minLevel
     *     - level > dungeon->maxLevel
     *     - expansion < dungeon->expansionLevel
     *     - gear score < requirements [todo: mangos lacks a system to check for this]
     *                                      (only  Player::GetEquipGearScore(false,false))
     *     - dungeon->typeID == LFG_TYPE_RAID (dungeon finder, not raid finder)
     */
    for (uint32 id = 0; id < sLfgDungeonsStore.GetNumRows(); ++id)
    {
        LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(id);
        if (dungeon)
        {
            
        }
    }
    return dungeonEntries;
}

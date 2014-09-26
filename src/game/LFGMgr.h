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

#ifndef __MANGOS_LFGMGR_H
#define __MANGOS_LFGMGR_H

#include "Common.h"
#include "Policies/Singleton.h"

#include <set>
#include <vector>

enum LFGFlags
{
    LFG_FLAG_UNK1        = 0x1,
    LFG_FLAG_UNK2        = 0x2,
    LFG_FLAG_SEASONAL    = 0x4,
    LFG_FLAG_UNK3        = 0x8
};

/// Reasons a player cannot enter a dungeon
enum LFGForbiddenTypes
{
    LFG_FORBIDDEN_EXPANSION             = 1,
    LFG_FORBIDDEN_LOW_LEVEL             = 2,
    LFG_FORBIDDEN_HIGH_LEVEL            = 3,
    LFG_FORBIDDEN_LOW_GEAR_SCORE        = 4,
    LFG_FORBIDDEN_HIGH_GEAR_SCORE       = 5,
    LFG_FORBIDDEN_RAID                  = 6,
    LFG_FORBIDDEN_ATTUNEMENT_LOW_LEVEL  = 1001,
    LFG_FORBIDDEN_ATTUNEMENT_HIGH_LEVEL = 1002,
    LFG_FORBIDDEN_QUEST_INCOMPLETE      = 1022,
    LFG_FORBIDDEN_MISSING_ITEM          = 1025,
    LFG_FORBIDDEN_NOT_IN_SEASON         = 1031,
    LFG_FORBIDDEN_MISSING_ACHIEVEMENT   = 1034
};

enum DungeonTypes
{
    DUNGEON_CLASSIC      = 0,
    DUNGEON_TBC          = 1,
    DUNGEON_TBC_HEROIC   = 2,
    DUNGEON_WOTLK        = 3,
    DUNGEON_WOTLK_HEROIC = 4
};

/// Item rewards taken from DungeonFinderItems in ObjectMgr, parsed by dbc values
struct ItemRewards
{
    uint32 itemId;
    uint32 itemAmount;
    
    ItemRewards() : itemId(0), itemAmount(0) {}
    ItemRewards(uint32 ItemId, uint32 ItemAmount) : itemId(ItemId), itemAmount(ItemAmount) {}
};

typedef std::set<uint32> dailyEntries; // for players who did one of X type instance per day
typedef UNORDERED_MAP<uint32, uint32> dungeonEntries; // ID, Entry
typedef UNORDERED_MAP<uint32, uint32> dungeonForbidden; // Entry, Why it's forbidden

class LFGMgr
{
public:
    LFGMgr();
    ~LFGMgr();
    
    /**
     * @brief Used to fetch the item rewards of a dungeon from the database
     * 
     * @param dungeonId the dungeon ID used in the DBCs
     * @param type the type of dungeon
     */
    ItemRewards GetDungeonItemRewards(uint32 dungeonId, DungeonTypes type);
    
    /**
     * @brief Used to determine the type of dungeon for ease of use.
     * 
     * @param dungeonId the dungeon ID used in the DBCs
     */
    DungeonTypes GetDungeonType(uint32 dungeonId);
    
    /**
     * @brief Used to record the first time a player has entered x type of dungeon in the day.
     * 
     * @param guidLow the player's guidLow
     * @param dungeon the specific type/expansion of dungeon
     */
    void RegisterPlayerDaily(uint32 guidLow, DungeonTypes dungeon);
    
    /**
     * @brief Used to find whether or not the player has done x type of dungeon today.
     * 
     * @param guidLow the player's guidLow
     * @param dungeon the specific type/expansion of dungeon
     */
    bool HasPlayerDoneDaily(uint32 guidLow, DungeonTypes dungeon);
    
     /// Reset accounts of players completing a/any dungeon for the day for new rewards
    void ResetDailyRecords();
    
    /**
     * @brief Find out whether or not a special dungeon is available for that season 
     * 
     * @param dungeonId the ID of the dungeon in question
     */
    bool IsSeasonActive(uint32 dungeonId);
    
    /**
     * @brief Find the random dungeons applicable for a player 
     * 
     * @param level The level of said player
     * @param expansion The player's expansion
     */
    dungeonEntries FindRandomDungeonsForPlayer(uint32 level, uint8 expansion);
    
    /**
     * @brief Find the random dungeons not applicable for a player
     * 
     * @param level The level of said player
     * @param expansion The player's expansion
     */
    dungeonForbidden FindRandomDungeonsNotForPlayer(uint32 level, uint8 expansion);
    
protected:
    bool IsSeasonal(uint32 dbcFlags) { return ((dbcFlags & LFG_FLAG_SEASONAL) != 0) ? true : false; }
    
private:
    dailyEntries m_dailyAny;
    dailyEntries m_dailyTBCHeroic;
    dailyEntries m_dailyLKNormal;
    dailyEntries m_dailyLKHeroic;
};

#define sLFGMgr MaNGOS::Singleton<LFGMgr>::Instance()

#endif

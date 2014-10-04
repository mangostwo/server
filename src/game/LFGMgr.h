/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2014  MaNGOS project <http://getmangos.eu>
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

#ifndef __MANGOS_LFGMGR_H
#define __MANGOS_LFGMGR_H

#include "Common.h"
#include "Policies/Singleton.h"

#include <set>
#include <vector>

class Object;
class ObjectGuid;
class Player;

// Begin Section: Constants & Definitions

/// Heroic dungeon rewards in WoTLK after already doing a dungeon
const uint32 WOTLK_SPECIAL_HEROIC_ITEM = 47241;
const uint32 WOTLK_SPECIAL_HEROIC_AMNT = 2;

typedef std::set<uint32> dailyEntries; // for players who did one of X type instance per day
typedef UNORDERED_MAP<uint32, uint32> dungeonEntries; // ID, Entry
typedef UNORDERED_MAP<uint32, uint32> dungeonForbidden; // Entry, LFGForbiddenTypes
typedef UNORDERED_MAP<uint64, dungeonForbidden> partyForbidden; // ObjectGuid (raw), map of locked dungeons

// End Section: Constants & Definitions

// Begin Section: Enumerations & Structures
enum LFGFlags
{
    LFG_FLAG_UNK1        = 0x1,
    LFG_FLAG_UNK2        = 0x2,
    LFG_FLAG_SEASONAL    = 0x4,
    LFG_FLAG_UNK3        = 0x8
};

/// Possible statuses to send after a request to join the dungeon finder
enum LfgJoinResult
{
    ERR_LFG_OK                                  = 0x00,
    ERR_LFG_ROLE_CHECK_FAILED                   = 0x01,
    ERR_LFG_GROUP_FULL                          = 0x02,
    ERR_LFG_NO_LFG_OBJECT                       = 0x04,
    ERR_LFG_NO_SLOTS_PLAYER                     = 0x05,
    ERR_LFG_NO_SLOTS_PARTY                      = 0x06,
    ERR_LFG_MISMATCHED_SLOTS                    = 0x07,
    ERR_LFG_PARTY_PLAYERS_FROM_DIFFERENT_REALMS = 0x08,
    ERR_LFG_MEMBERS_NOT_PRESENT                 = 0x09,
    ERR_LFG_GET_INFO_TIMEOUT                    = 0x0A,
    ERR_LFG_INVALID_SLOT                        = 0x0B,
    ERR_LFG_DESERTER_PLAYER                     = 0x0C,
    ERR_LFG_DESERTER_PARTY                      = 0x0D,
    ERR_LFG_RANDOM_COOLDOWN_PLAYER              = 0x0E,
    ERR_LFG_RANDOM_COOLDOWN_PARTY               = 0x0F,
    ERR_LFG_TOO_MANY_MEMBERS                    = 0x10,
    ERR_LFG_CANT_USE_DUNGEONS                   = 0x11,
    ERR_LFG_ROLE_CHECK_FAILED2                  = 0x12,
};

enum LfgUpdateType
{
    LFG_UPDATE_DEFAULT              = 0,
    LFG_UPDATE_LEADER_LEAVE         = 1,
    LFG_UPDATE_ROLECHECK_ABORTED    = 4,
    LFG_UPDATE_JOIN                 = 5,
    LFG_UPDATE_ROLECHECK_FAILED     = 6,
    LFG_UPDATE_LEAVE                = 7,
    LFG_UPDATE_PROPOSAL_FAILED      = 8,
    LFG_UPDATE_PROPOSAL_DECLINED    = 9,
    LFG_UPDATE_GROUP_FOUND          = 10,
    LFG_UPDATE_ADDED_TO_QUEUE       = 12,
    LFG_UPDATE_PROPOSAL_BEGIN       = 13,
    LFG_UPDATE_STATUS               = 14,
    LFG_UPDATE_GROUP_MEMBER_OFFLINE = 15,
    LFG_UPDATE_GROUP_DISBAND        = 16,
};

enum LfgType
{
    LFG_TYPE_NONE                 = 0,
    LFG_TYPE_DUNGEON              = 1,
    LFG_TYPE_RAID                 = 2,
    LFG_TYPE_QUEST                = 3,
    LFG_TYPE_ZONE                 = 4,
    LFG_TYPE_HEROIC_DUNGEON       = 5,
    LFG_TYPE_RANDOM_DUNGEON       = 6
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

/// Spells that affect the mechanisms of the dungeon finder
enum LFGSpells
{
    LFG_DESERTER_SPELL = 71041,
    LFG_COOLDOWN_SPELL = 71328,
};

enum LFGState
{
    LFG_STATE_NONE,
    LFG_STATE_ROLECHECK,
    LFG_STATE_QUEUED,
    LFG_STATE_PROPOSAL,
    LFG_STATE_BOOT,
    LFG_STATE_IN_DUNGEON,
    LFG_STATE_FINISHED_DUNGEON,
    LFG_STATE_RAIDBROWSER
};

/// Role check states
enum LFGRoleCheckState
{
    LFG_ROLECHECK_DEFAULT                        = 0,      // Internal use = Not initialized.
    LFG_ROLECHECK_FINISHED                       = 1,      // Role check finished
    LFG_ROLECHECK_INITIALITING                   = 2,      // Role check begins
    LFG_ROLECHECK_MISSING_ROLE                   = 3,      // Someone hasn't selected a role after 2 mins
    LFG_ROLECHECK_WRONG_ROLES                    = 4,      // Can't form a group with the role selection
    LFG_ROLECHECK_ABORTED                        = 5,      // Someone left the group
    LFG_ROLECHECK_NO_ROLE                        = 6       // Someone didn't select a role
};

enum DungeonTypes
{
    DUNGEON_CLASSIC      = 0,
    DUNGEON_TBC          = 1,
    DUNGEON_TBC_HEROIC   = 2,
    DUNGEON_WOTLK        = 3,
    DUNGEON_WOTLK_HEROIC = 4,
    DUNGEON_UNKNOWN
};

/// Item rewards taken from DungeonFinderItems in ObjectMgr, parsed by dbc values
struct ItemRewards
{
    uint32 itemId;
    uint32 itemAmount;
    
    ItemRewards() : itemId(0), itemAmount(0) {}
    ItemRewards(uint32 ItemId, uint32 ItemAmount) : itemId(ItemId), itemAmount(ItemAmount) {}
};

/// Information the dungeon finder needs about each player
struct LFGPlayers
{
    LFGState currentState; // where the player is at with the dungeon finder
    dungeonEntries currentDungeonSelection; // what dungeon(s) have they selected
    uint8 currentRoles; // tank, dps, healer, etc..
    std::string comments;
};

typedef UNORDERED_MAP<uint64, LFGPlayers> playerData; // ObjectGuid(raw), info on specific player

// End Section: Enumerations & Structures

class LFGMgr
{
public:
    LFGMgr();
    ~LFGMgr();
    
    /**
     * @brief Attempt to join the dungeon finder queue, as long as the player(s)
     *        fit the criteria.
     * 
     * @param roles Roles selected in lfg window
     * @param dungeons List of dungeon(s) selected
     * @param comments Comments made by the player
     * @param plr Pointer to the player sending the packet
     */
    void JoinLFG(uint32 roles, std::set<uint32> dungeons, std::string comments, Player* plr);
    
    void LeaveLFG();
    
    /**
     * @brief Go through a number of checks to see if the player/group can join
     *        the LFG queue
     * 
     * @param plr The pointer to the player
     */
    LfgJoinResult GetJoinResult(Player* plr);
    
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
    dungeonForbidden FindRandomDungeonsNotForPlayer(Player* plr);
    
protected:
    bool IsSeasonal(uint32 dbcFlags) { return ((dbcFlags & LFG_FLAG_SEASONAL) != 0) ? true : false; }
    
    /// Check if player/party is already in the system, return that data
    LFGPlayers* GetPlayerOrPartyData(uint64 rawGuid);
    
private:
    dailyEntries m_dailyAny;
    dailyEntries m_dailyTBCHeroic;
    dailyEntries m_dailyLKNormal;
    dailyEntries m_dailyLKHeroic;
    
    playerData m_playerData;
};

#define sLFGMgr MaNGOS::Singleton<LFGMgr>::Instance()

#endif

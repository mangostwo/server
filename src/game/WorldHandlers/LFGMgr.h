/**
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2026 MaNGOS <https://www.getmangos.eu>
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
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

#ifndef __MANGOS_LFGMGR_H
#define __MANGOS_LFGMGR_H

#include <unordered_map>
#include "Platform/Define.h"
#include "Common/TimeConstants.h"
#include <ctime>
#include <string>
#include "Policies/Singleton.h"
#include "Group.h"
#include <set>
#include <vector>

class Object;
class ObjectGuid;
class Player;
class Group;

struct LFGBoot;
struct LFGGroupStatus;
struct LFGPlayers;
struct LFGPlayerStatus;
struct LFGProposal;
struct LFGRoleCheck;
struct LFGWait;

// Begin Section: Enumerations

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

enum LFGTimes
{
    LFG_TIME_ROLECHECK                           = 45,
    LFG_TIME_BOOT                                = 120,
    LFG_TIME_PROPOSAL                            = 45,
};

/// Proposal answers
enum LFGProposalAnswer
{
    LFG_ANSWER_PENDING                           = -1,
    LFG_ANSWER_DENY                              = 0,
    LFG_ANSWER_AGREE                             = 1
};

/// Player states in the lfg system
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

/// Proposal states
enum LFGProposalState
{
    LFG_PROPOSAL_INITIATING                      = 0,
    LFG_PROPOSAL_FAILED                          = 1,
    LFG_PROPOSAL_SUCCESS                         = 2
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

/// Role types
enum LFGRoles
{
    PLAYER_ROLE_NONE                             = 0x00,
    PLAYER_ROLE_LEADER                           = 0x01,
    PLAYER_ROLE_TANK                             = 0x02,
    PLAYER_ROLE_HEALER                           = 0x04,
    PLAYER_ROLE_DAMAGE                           = 0x08
};

/// Role amounts
enum LFGRoleCount
{
    NORMAL_TANK_OR_HEALER_COUNT                  = 1,      // Tanks / Heals
    NORMAL_DAMAGE_COUNT                          = 3,      // DPS
    NORMAL_TOTAL_ROLE_COUNT                      = 5       // Amount of players total per normal dungeon
};

/// Teleport errors
enum LFGTeleportError
{
    // 7 = "You can't do that right now" | 5 = No client reaction
    LFG_TELEPORTERROR_OK                         = 0,
    LFG_TELEPORTERROR_PLAYER_DEAD                = 1,
    LFG_TELEPORTERROR_FALLING                    = 2,
    LFG_TELEPORTERROR_IN_VEHICLE                 = 3,
    LFG_TELEPORTERROR_FATIGUE                    = 4,
    LFG_TELEPORTERROR_INVALID_LOCATION           = 6,
    LFG_TELEPORTERROR_CHARMING                   = 8
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

// End Section: Enumerations

// Begin Section: Constants & Definitions

/// Heroic dungeon rewards in WoTLK after already doing a dungeon
const uint32 WOTLK_SPECIAL_HEROIC_ITEM = 47241;
const uint32 WOTLK_SPECIAL_HEROIC_AMNT = 2;

/// Default average queue time (in case we don't have data to base calculations on)
const int32 QUEUE_DEFAULT_TIME = 15*MINUTE;                              // 15 minutes [system is measured in seconds]

/// Amount of votes needed to kick a player out of a group
const int32 REQUIRED_VOTES_FOR_BOOT = 3;

typedef std::set<uint32> dailyEntries;                                   // for players who did one of X type instance per day
typedef std::set<ObjectGuid> queueSet;                                   // List of players / groups in the queue
typedef std::set<ObjectGuid> groupSet;                                   // List of groups doing a dungeon via the finder

typedef std::unordered_map<uint32, uint32> dungeonEntries;                    // ID, Entry
typedef std::unordered_map<uint32, uint32> dungeonForbidden;                  // Entry, LFGForbiddenTypes
typedef std::unordered_map<uint32, LFGProposal> proposalMap;                  // Proposal ID, info on a proposal
typedef std::unordered_map<uint32, LFGWait> waitTimeMap;                      // DungeonID, wait info
typedef std::unordered_map<ObjectGuid, dungeonForbidden> partyForbidden;      // ObjectGuid of player, map of locked dungeons
typedef std::unordered_map<ObjectGuid, uint8> roleMap;                        // ObjectGuid of player, role(s) selected
typedef std::unordered_map<ObjectGuid, uint32> playerDungeonMap;              // Player, requested random category (zero for specific)
typedef std::unordered_map<ObjectGuid, LFGRoleCheck> roleCheckMap;            // ObjectGuid of group, role information
typedef std::unordered_map<ObjectGuid, LFGPlayerStatus> playerStatusMap;      // ObjectGuid of player, info on specific players only
typedef std::unordered_map<ObjectGuid, LFGPlayers> playerData;                // ObjectGuid of plr/group, info on specific player or group. TODO: rename to queueData
typedef std::unordered_map<ObjectGuid, LFGProposalAnswer> proposalAnswerMap;  // ObjectGuid of player, answer to proposal
typedef std::unordered_map<ObjectGuid, ObjectGuid> playerGroupMap;            // ObjectGuid of player, ObjectGuid of group
typedef std::unordered_map<ObjectGuid, uint32> ownerProposalMap;               // Queue-source owner, active proposal ID
typedef std::unordered_map<ObjectGuid, LFGGroupStatus> groupStatusMap;        // ObjectGuid of group, group status structure
typedef std::unordered_map<ObjectGuid, LFGBoot> bootStatusMap;                // ObjectGuid of group, boot vote status

// End Section: Constants & Definitions

// Begin Section: Structures

struct LFGQueueSource
{
    ObjectGuid ownerGuid;
    std::set<uint32> dungeonList;
    playerDungeonMap randomDungeonByPlayer;
    roleMap selectedRoles;
    std::string comment;
    TeamId team;
    bool isGroup;
    time_t joinedTime;

    LFGQueueSource() : team(TEAM_NEUTRAL), isGroup(false), joinedTime(0) {}
    LFGQueueSource(ObjectGuid OwnerGuid, std::set<uint32> const& DungeonList,
        playerDungeonMap const& RandomDungeonByPlayer,
        roleMap const& SelectedRoles, std::string const& Comment, TeamId Team,
        bool IsGroup, time_t JoinedTime) : ownerGuid(OwnerGuid),
        dungeonList(DungeonList), randomDungeonByPlayer(RandomDungeonByPlayer),
        selectedRoles(SelectedRoles), comment(Comment), team(Team),
        isGroup(IsGroup), joinedTime(JoinedTime) {}
};

typedef std::unordered_map<ObjectGuid, LFGQueueSource> queueSourceMap;

/// Item rewards taken from DungeonFinderItems in ObjectMgr, parsed by dbc values
struct ItemRewards
{
    uint32 itemId;
    uint32 itemAmount;

    ItemRewards() : itemId(0), itemAmount(0) {}
    ItemRewards(uint32 ItemId, uint32 ItemAmount) : itemId(ItemId), itemAmount(ItemAmount) {}
};

/// Information the dungeon finder needs about each player (or group)
struct LFGPlayers //TODO: rename to LFGQueueData
{
    LFGState currentState;                  // where the player is at with the dungeon finder
    std::set<uint32> dungeonList;           // The dungeons this player or group are queued for (ID, not entry)
    roleMap currentRoles;                   // tank, dps, healer, etc..
    std::string comments;
    playerDungeonMap randomDungeonByPlayer;
    queueSourceMap sourceUnits;
    TeamId team;
    bool isGroup;

    time_t joinedTime;
    uint8 neededTanks;
    uint8 neededHealers;
    uint8 neededDps;

    LFGPlayers() : currentState(LFG_STATE_NONE), team(TEAM_NEUTRAL),
        isGroup(false), joinedTime(0), neededTanks(0), neededHealers(0),
        neededDps(0) {}
    LFGPlayers(LFGState state, std::set<uint32> const& dungeonSelection,
        roleMap const& CurrentRoles, std::string const& comment, bool IsGroup,
        time_t JoinedTime, uint8 NeededTanks, uint8 NeededHealers,
        uint8 NeededDps, playerDungeonMap const& RandomDungeonByPlayer,
        TeamId Team, queueSourceMap const& SourceUnits) : currentState(state),
        dungeonList(dungeonSelection), currentRoles(CurrentRoles),
        comments(comment), randomDungeonByPlayer(RandomDungeonByPlayer),
        sourceUnits(SourceUnits), team(Team), isGroup(IsGroup),
        joinedTime(JoinedTime), neededTanks(NeededTanks),
        neededHealers(NeededHealers), neededDps(NeededDps) {}
};

struct LFGRoleCheck
{
    LFGRoleCheckState state;      // current status of the role check
    roleMap currentRoles;         // map of players to roles
    std::set<uint32> dungeonList; // The dungeons this player or group are queued for
    uint32 randomDungeonID;       // The random dungeon ID
    playerDungeonMap randomDungeonByPlayer;
    uint64 leaderGuidRaw;         // ObjectGuid(raw) of leader
    time_t waitForRoleTime;       // How long we'll wait for the players to confirm their roles

    LFGRoleCheck() : state(LFG_ROLECHECK_DEFAULT), randomDungeonID(0),
        leaderGuidRaw(0), waitForRoleTime(0) {}
};

struct LFGWait
{
    int32 time;                   // current wait time for x (in seconds, so (time_t x / IN_MILLISECONDS)
    int32 previousTime;           // how long it took for the last person to go from queue to instance
    uint32 playerCount;           // amount of players in x queue for calculations [not sure if needed when finished implementing system]
    bool doAverage;               // tells the lfgmgr during a world update whether or not to recalculate waiting time

    LFGWait() : time(-1), previousTime(-1), playerCount(0), doAverage(false) {}
    LFGWait(int32 currentTime, int32 lastTime, uint32 currentPlayerCount, bool shouldRecalculate)
        : time(currentTime), previousTime(lastTime), playerCount(currentPlayerCount), doAverage(shouldRecalculate) {}
};

/// For SMSG_LFG_QUEUE_STATUS
struct LFGQueueStatus
{
    uint32 dungeonID;             // queue info for x dungeon
    int32  playerAvgWaitTime;     // average wait time for the current player
    int32  avgWaitTime;           // average wait time for the dungeon
    int32  tankAvgWaitTime;       // average wait time for the tank(s)
    int32  healerAvgWaitTime;     // average wait time for the healer(s)
    int32  dpsAvgWaitTime;        // average wait time for the dps'
    uint8  neededTanks;           // amount of tanks needed
    uint8  neededHeals;           // amount of healers needed
    uint8  neededDps;             // amount of dps needed
    uint32 timeSpentInQueue;      // time already spent in the queue
};

struct LFGGroupUpdateData
{
    uint8 role;
    uint8 state;
    uint32 dungeonEntry;

    LFGGroupUpdateData() : role(PLAYER_ROLE_NONE), state(0), dungeonEntry(0) { }
};

/// For CMSG_LFG_GET_STATUS, SMSG_LFG_UPDATE_PARTY, and SMSG_LFG_UPDATE_PLAYER
struct LFGPlayerStatus
{
    LFGState state;
    LfgUpdateType updateType;
    std::set<uint32> dungeonList;
    std::string comment;

    LFGPlayerStatus() : state(LFG_STATE_NONE), updateType(LFG_UPDATE_DEFAULT) { }
    LFGPlayerStatus(LFGState State, LfgUpdateType UpdateType, std::set<uint32> DungeonList, std::string Comment)
        : state(State), updateType(UpdateType), dungeonList(DungeonList), comment(Comment) { }
};

/// Information on a group currently in a dungeon
struct LFGGroupStatus //todo: check for this in joinlfg function, not lfgplayers struct
{
    LFGState state;        // State of the group
    uint32 dungeonID;      // ID of the dungeon the group should be in
    roleMap playerRoles;   // Container holding each player's objectguid and their roles
    playerDungeonMap randomDungeonByPlayer;
    ObjectGuid leaderGuid; // The group leader's object guid

    LFGGroupStatus() : state(LFG_STATE_NONE), dungeonID(0) { }
    LFGGroupStatus(LFGState State, uint32 DungeonID,
        roleMap const& PlayerRoles,
        playerDungeonMap const& RandomDungeonByPlayer, ObjectGuid LeaderGuid)
        : state(State), dungeonID(DungeonID), playerRoles(PlayerRoles),
        randomDungeonByPlayer(RandomDungeonByPlayer), leaderGuid(LeaderGuid) { }
};

/// For SMSG_LFG_PROPOSAL_UPDATE
struct LFGProposal
{
    uint32 id;                 // proposal id
    uint32 dungeonID;          // dungeon id
    LFGProposalState state;    // proposal state
    uint32 encounters;         // encounters done
    uint64 groupRawGuid;       // group raw guid value
    uint64 groupLeaderGuid;    // group leader's guid
    bool isNew;                // is new or old group
    roleMap currentRoles;      // group player's roles
    playerDungeonMap randomDungeonByPlayer;
    queueSourceMap sourceUnits;
    proposalAnswerMap answers; // answers to a proposal
    playerGroupMap groups;     // data on which groups players belong/belonged to
    time_t joinedQueue;        // time from when the players joined the queue
    time_t expiresAt;

    LFGProposal() : id(0), dungeonID(0), state(LFG_PROPOSAL_INITIATING),
        encounters(0), groupRawGuid(0), groupLeaderGuid(0), isNew(true),
        joinedQueue(0), expiresAt(0) {}
};

// For SMSG_LFG_PLAYER_REWARD
struct LFGRewards
{
    uint32 randomDungeonEntry;  // Entry of the random dungeon done (0 if not random)
    uint32 groupDungeonEntry;   // Entry of the dungeon done by your group
    bool hasDoneDaily;          // First dungeon of the day?
    uint32 moneyReward;         // Amount of money rewarded
    uint32 expReward;           // Amount of experience rewarded
    uint32 itemID;              // ID of item reward
    uint32 itemAmount;          // How many of x item is rewarded

    LFGRewards() { }
    LFGRewards(uint32 RandomDungeonEntry, uint32 GroupDungeonEntry, bool HasDoneDaily,
        uint32 MoneyReward, uint32 ExpReward, uint32 ItemID, uint32 ItemAmount) :
        randomDungeonEntry(RandomDungeonEntry), groupDungeonEntry(GroupDungeonEntry),
        hasDoneDaily(HasDoneDaily), moneyReward(MoneyReward), expReward(ExpReward),
        itemID(ItemID), itemAmount(ItemAmount) { }
};

// For SMSG_LFG_BOOT_PLAYER
struct LFGBoot
{
    bool inProgress;           // Is the boot vote still occurring?
    LFGState previousState;    // Group state restored when the vote ends
    ObjectGuid playerVotedOn;  // ObjectGuid of the player being voted on
    std::string reason;        // Reason stated for the vote
    proposalAnswerMap answers; // Player's votes
    time_t startTime;          // When the vote started

    LFGBoot() : inProgress(false), previousState(LFG_STATE_NONE), startTime(0) { }
    LFGBoot(bool InProgress, LFGState PreviousState,
        ObjectGuid PlayerVotedOn, std::string const& Reason,
        proposalAnswerMap const& Answers, time_t StartTime)
        : inProgress(InProgress), previousState(PreviousState),
        playerVotedOn(PlayerVotedOn), reason(Reason), answers(Answers),
        startTime(StartTime) { }
};

// End Section: Structures

class LFGMgr
{
public:
    LFGMgr();
    ~LFGMgr();

    /// Update queue information and such
    void Update();

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

    /**
     * @brief Leave the lfg/dungeon finder system.
     *
     * @param plr The pointer to the player sending the request
     * @param isGroup Whether or not they are the leader of a group / in a group
     */
    void LeaveLFG(Player* plr, bool isGroup);

    /**
     * @brief Go through a number of checks to see if the player/group can join
     *        the LFG queue
     *
     * @param plr The pointer to the player
     */
    LfgJoinResult GetJoinResult(Player* plr);

    /**
     * @brief Fetch the playerstatus struct of a player on request, if existant
     *
     * @param guid the player's objectguid
     */
    LFGPlayerStatus GetPlayerStatus(ObjectGuid guid);

    /**
     * @brief Set the player's comment string
     *
     * @param guid The player's objectguid
     * @param comment Their comments
     */
    void SetPlayerComment(ObjectGuid guid, std::string comment);

    /**
     * @brief Set the player's LFG state
     *
     * @param guid The player's objectguid
     * @param state the LFGState value
     */
    void SetPlayerState(ObjectGuid guid, LFGState state);

    /**
     * @brief Set the player's LFG update type
     *
     * @param guid The player's objectguid
     * @param updateType The LfgUpdateType value
     */
    void SetPlayerUpdateType(ObjectGuid guid, LfgUpdateType updateType);

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

    /// Given the ID of a dungeon, spit out its entry
    uint32 GetDungeonEntry(uint32 ID) const;

    /// Fetch the client-facing LFD fields appended to SMSG_GROUP_LIST.
    bool GetGroupUpdateData(ObjectGuid groupGuid, ObjectGuid playerGuid,
        LFGGroupUpdateData& data) const;

    /// Enter or leave the group's validated LFD dungeon.
    void TeleportPlayer(Player* pPlayer, bool out, bool automatic);

    // TEMPORARY LFD SMOKE TEST: remove after the one-player live test.
    bool IsTesting() const { return m_testing; }
    void SetTesting(bool testing) { m_testing = testing; }

    /// Queue Functions Below

    /**
     * Find the player's or group's information and update the system with
     *     the amount of each role they need to find.
     *
     * @param guid The guid assigned to the structure
     * @param information The LFGPlayers structure containing their information
     */
    void UpdateNeededRoles(ObjectGuid guid, LFGPlayers* information);

    /**
     * @brief Add the player or group to the Dungeon Finder queue
     *
     * @param guid the player/group's ObjectGuid
     */
    void AddToQueue(ObjectGuid guid);

    /**
     * @brief Remove the player or group from the Dungeon Finder queue
     *
     * @param guid the player/group's ObjectGuid
     */
    void RemoveFromQueue(ObjectGuid guid);

    /// Search the queue for compatible matches
    void FindQueueMatches();

    /**
     * @brief Search the queue for matches based off of one's guid
     *
     * @param guid The player or group's guid
     */
    void FindSpecificQueueMatches(ObjectGuid guid);

    /// Send a periodic status update for queued players
    void SendQueueStatus();

    /// Role-Related Functions

    /**
     * @brief Set and/or confirm roles for a group.
     *
     * @param pPlayer The pointer to the player issuing the request
     * @param pGroup The pointer to that player's group
     * @param roles The group leader's role(s)
     */
    void PerformRoleCheck(Player* pPlayer, Group* pGroup, uint8 roles);

    /// Make sure role selections are okay
    bool ValidateGroupRoles(roleMap groupMap);

    /// Proposal-Related Functions

    void ProposalUpdate(uint32 proposalID, ObjectGuid plrGuid, bool accepted);

    /// Handles reward hooks -- called by achievement manager
    void HandleBossKilled(Player* pPlayer);

    /// Group kick hook
    void AttemptToKickPlayer(Group* pGroup, ObjectGuid guid, ObjectGuid kicker, std::string reason);

    // Called when a player votes yes or no on a boot vote
    void CastVote(Player* pPlayer, bool vote);

    /// Group lifecycle hooks used by the core group implementation.
    void OnGroupMemberRemoved(ObjectGuid groupGuid, ObjectGuid playerGuid);
    void OnGroupDisband(ObjectGuid groupGuid);
    void OnGroupLeaderChanged(ObjectGuid groupGuid, ObjectGuid newLeaderGuid);

    /// Returns true when logout must retain active LFD group membership.
    bool OnPlayerLogout(Player* player);

protected:
    bool IsSeasonal(uint32 dbcFlags) { return ((dbcFlags & LFG_FLAG_SEASONAL) != 0) ? true : false; }

    /// Check if player/party is already in the system, return that data
    LFGPlayers* GetPlayerOrPartyData(ObjectGuid guid);

    /// Get a proposal structure given its id
    LFGProposal* GetProposalData(uint32 proposalID);

    /// Get information on a group currently in a dungeon
    LFGGroupStatus* GetGroupStatus(ObjectGuid guid);

    /// Add the player to their respective waiting map for their dungeon
    void AddToWaitMap(uint8 role, std::set<uint32> dungeons);

    /// Compares two groups/players to see if their role combinations are compatible
    bool RoleMapsAreCompatible(LFGPlayers* groupOne, LFGPlayers* groupTwo);

    /// Checks whether or not two combinations of players/groups are on the same team (alliance/horde)
    bool MatchesAreOfSameTeam(LFGPlayers* groupOne, LFGPlayers* groupTwo);

    /// Updates a wait map with the amount of time it took the last player to join
    void UpdateWaitMap(LFGRoles role, uint32 dungeonID, time_t waitTime);

    /// Creates a group so they can enter a dungeon together
    bool CreateDungeonGroup(LFGProposal* proposal);

    /**
     * @brief Merges two players/groups/etc into one for dungeon assignment.
     *
     * @param guidOne The guid assigned to the first group in m_playerData
     * @param guidTwo The guid assigned to the second group in m_playerData
     * @param compatibleDungeons The dungeons that both players or groups agreed to doing
     */
    void MergeGroups(ObjectGuid guidOne, ObjectGuid guidTwo,
        std::set<uint32> const& compatibleDungeons);

    /// Consume a complete queue aggregate and publish one proposal atomically.
    bool BeginProposal(ObjectGuid ownerGuid);

    /// Remove one failed proposal and restore each still-valid source at most once.
    void UnwindProposal(uint32 proposalId, std::set<ObjectGuid> const& failedPlayers);

    /// Revalidate and republish one immutable queue source.
    bool RestoreQueueSource(LFGQueueSource const& source);

    /// Tell a group member that someone else just confirmed their role
    void SendRoleChosen(ObjectGuid plrGuid, ObjectGuid confirmedGuid, uint8 roles);

    /// Send SMSG_LFG_ROLE_CHECK_UPDATE to a specific player
    void SendRoleCheckUpdate(ObjectGuid plrGuid, LFGRoleCheck const& roleCheck);

    /// Send SMSG_LFG_UPDATE_PARTY or SMSG_LFG_UPDATE_PLAYER
    void SendLfgUpdate(ObjectGuid plrGuid, LFGPlayerStatus status, bool isGroup);

    /// Send SMSG_LFG_JOIN_RESULT
    void SendLfgJoinResult(ObjectGuid plrGuid, LfgJoinResult result, LFGState state, partyForbidden const& lockedDungeons);

    /// Get rid of expired role checks
    void RemoveOldRoleChecks();

    /// Fail proposals whose client-response window expired.
    void RemoveOldProposals();

    /// Resolve one boot vote and restore the group's previous state.
    void FinishBootVote(ObjectGuid groupGuid, bool succeeded);

    /// Fail expired boot votes.
    void RemoveOldBoots();

    /// Cancel one immutable queue source and restore unaffected merged sources.
    void CancelQueueSource(ObjectGuid sourceOwner, LfgUpdateType updateType);

    /// Abort and remove one pending party role check.
    void CancelRoleCheck(ObjectGuid groupGuid, LfgUpdateType updateType);

    /// Keep an aggregate queue record and every member's client status in sync.
    bool TransitionQueueUnit(ObjectGuid ownerGuid, LFGState state, LfgUpdateType updateType);

    /// Update client-visible state after the aggregate queue record is consumed.
    bool TransitionPlayer(ObjectGuid playerGuid, LFGState state, LfgUpdateType updateType);

private:
    /// Daily occurences of a player doing X type dungeon
    dailyEntries m_dailyAny;
    dailyEntries m_dailyTBCHeroic;
    dailyEntries m_dailyLKNormal;
    dailyEntries m_dailyLKHeroic;

    /// General info related to joining / leaving the dungeon finder
    playerData m_playerData;
    queueSet   m_queueSet;

    /// Dungeon Finder Status for players
    playerStatusMap m_playerStatusMap;

    /// Current queue owner for every player participating in an LFD flow.
    playerGroupMap m_playerQueueOwners;

    groupSet m_groupSet;
    groupStatusMap m_groupStatusMap;

    /// Role check information
    roleCheckMap m_roleCheckMap;

    /// Boot vote information
    bootStatusMap m_bootStatusMap;

    /// Wait times for the queue
    waitTimeMap m_tankWaitTime;
    waitTimeMap m_healerWaitTime;
    waitTimeMap m_dpsWaitTime;
    waitTimeMap m_avgWaitTime;

    /// Proposal information
    uint32 m_proposalId;
    proposalMap m_proposalMap;
    ownerProposalMap m_ownerProposalIds;

    // TEMPORARY LFD SMOKE TEST: never persist or enable by default.
    bool m_testing;
};

#define sLFGMgr MaNGOS::Singleton<LFGMgr>::Instance()

#endif

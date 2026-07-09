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

#ifndef MANGOS_DBCSTRUCTURE_H
#define MANGOS_DBCSTRUCTURE_H

#include "Common.h"
#include "DBCEnums.h"
#include "Path.h"
#include "Platform/Define.h"
#include "SharedDefines.h"

#include <map>
#include <set>
#include <vector>

// Structures using to access raw DBC data and required packing to portability

// GCC have alternative #pragma pack(N) syntax and old gcc version not support pack(push,N), also any gcc version not support it at some platform
#if defined( __GNUC__ )
#pragma pack(1)
#else
#pragma pack(push,1)
#endif

struct AchievementEntry
{
    uint32    ID;                                           // 0        m_ID
    uint32    Faction;                                  // 1        m_faction -1=all, 0=horde, 1=alliance
    uint32    Instance_ID;                                        // 2        m_instance_id -1=none
    // uint32 parentAchievement;                            // 3        m_supercedes its Achievement parent (can`t start while parent uncomplete, use its Criteria if don`t have own, use its progress on begin)
    char* Title_lang[16];                                         // 4-19     m_title_lang
    // uint32 name_flags;                                   // 20 string Flags
    // char *description[16];                               // 21-36    m_description_lang
    // uint32 desc_flags;                                   // 37 string Flags
    uint32    Category;                                   // 38       m_category
    uint32    Points;                                       // 39       m_points
    // uint32 OrderInCategory;                              // 40       m_ui_order
    uint32    Flags;                                        // 41       m_flags
    // uint32    icon;                                      // 42       m_iconID
    // char *titleReward[16];                               // 43-58    m_reward_lang
    // uint32 titleReward_flags;                            // 59 string Flags
    uint32 Minimum_criteria;                                           // 60       m_minimum_criteria - need this Minimum_criteria of completed criterias (own or referenced achievement criterias)
    uint32 Shares_criteria;                                  // 61       m_shares_criteria - referenced achievement (counting of all completed criterias)
};

struct AchievementCategoryEntry
{
    uint32    ID;                                           // 0        m_ID
    uint32    Parent;                               // 1        m_parent -1 for main category
    // char *name[16];                                      // 2-17     m_name_lang
    // uint32 name_flags;                                   // 18 string flags
    // uint32    sortOrder;                                 // 19       m_ui_order
};

struct AchievementCriteriaEntry
{
    uint32  ID;                                             // 0        m_ID
    uint32  Achievement_ID;                            // 1        m_achievement_id
    uint32  Type;                                   // 2        m_type
    union
    {
        // ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE          = 0
        // TODO: also used for player deaths..
        struct
        {
            uint32  creatureID;                             // 3
            uint32  creatureCount;                          // 4
        } kill_creature;

        // ACHIEVEMENT_CRITERIA_TYPE_WIN_BG                 = 1
        struct
        {
            uint32  bgMapID;                                // 3
            uint32  winCount;                               // 4
            uint32  additionalRequirement1_type;            // 5
            uint32  additionalRequirement1_value;           // 6
            uint32  additionalRequirement2_type;            // 7
            uint32  additionalRequirement2_value;           // 8
        } win_bg;

        // ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL            = 5
        struct
        {
            uint32  unused;                                 // 3
            uint32  level;                                  // 4
        } reach_level;

        // ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL      = 7
        struct
        {
            uint32  skillID;                                // 3
            uint32  skillLevel;                             // 4
        } reach_skill_level;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT   = 8
        struct
        {
            uint32  linkedAchievement;                      // 3
        } complete_achievement;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST_COUNT   = 9
        struct
        {
            uint32  unused;                                 // 3
            uint32  totalQuestCount;                        // 4
        } complete_quest_count;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST_DAILY = 10
        struct
        {
            uint32  unused;                                 // 3
            uint32  numberOfDays;                           // 4
        } complete_daily_quest_daily;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE = 11
        struct
        {
            uint32  zoneID;                                 // 3
            uint32  questCount;                             // 4
        } complete_quests_in_zone;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST   = 14
        struct
        {
            uint32  unused;                                 // 3
            uint32  questCount;                             // 4
        } complete_daily_quest;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_BATTLEGROUND  = 15
        struct
        {
            uint32  mapID;                                  // 3
        } complete_battleground;

        // ACHIEVEMENT_CRITERIA_TYPE_DEATH_AT_MAP           = 16
        struct
        {
            uint32  mapID;                                  // 3
        } death_at_map;

        // ACHIEVEMENT_CRITERIA_TYPE_DEATH_IN_DUNGEON       = 18
        struct
        {
            uint32  manLimit;                               // 3
        } death_in_dungeon;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_RAID          = 19
        struct
        {
            uint32  groupSize;                              // 3 can be 5, 10 or 25
        } complete_raid;

        // ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_CREATURE     = 20
        struct
        {
            uint32  creatureEntry;                          // 3
        } killed_by_creature;

        // ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING     = 24
        struct
        {
            uint32  unused;                                 // 3
            uint32  fallHeight;                             // 4
        } fall_without_dying;

        // ACHIEVEMENT_CRITERIA_TYPE_DEATHS_FROM            = 26
        struct
        {
            uint32 type;                                    // 3, see enum EnviromentalDamage
        } death_from;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST         = 27
        struct
        {
            uint32  questID;                                // 3
            uint32  questCount;                             // 4
        } complete_quest;

        // ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET        = 28
        // ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2       = 69
        struct
        {
            uint32  spellID;                                // 3
            uint32  spellCount;                             // 4
        } be_spell_target;

        // ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL             = 29
        // ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2            = 110
        struct
        {
            uint32  spellID;                                // 3
            uint32  castCount;                              // 4
        } cast_spell;

        // ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL_AT_AREA = 31
        struct
        {
            uint32  areaID;                                 // 3 Reference to AreaTable.dbc
            uint32  killCount;                              // 4
        } honorable_kill_at_area;

        // ACHIEVEMENT_CRITERIA_TYPE_WIN_ARENA              = 32
        struct
        {
            uint32  mapID;                                  // 3 Reference to Map.dbc
        } win_arena;

        // ACHIEVEMENT_CRITERIA_TYPE_PLAY_ARENA             = 33
        struct
        {
            uint32  mapID;                                  // 3 Reference to Map.dbc
        } play_arena;

        // ACHIEVEMENT_CRITERIA_TYPE_LEARN_SPELL            = 34
        struct
        {
            uint32  spellID;                                // 3 Reference to Map.dbc
        } learn_spell;

        // ACHIEVEMENT_CRITERIA_TYPE_OWN_ITEM               = 36
        struct
        {
            uint32  itemID;                                 // 3
            uint32  itemCount;                              // 4
        } own_item;

        // ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA        = 37
        struct
        {
            uint32  unused;                                 // 3
            uint32  count;                                  // 4
            uint32  flag;                                   // 5 4=in a row
        } win_rated_arena;

        // ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_TEAM_RATING    = 38
        struct
        {
            uint32  teamtype;                               // 3 {2,3,5}
        } highest_team_rating;

        // ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_PERSONAL_RATING= 39
        struct
        {
            uint32  teamtype;                               // 3 {2,3,5}
            uint32  teamrating;                             // 4
        } highest_personal_rating;

        // ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LEVEL      = 40
        struct
        {
            uint32  skillID;                                // 3
            uint32  skillLevel;                             // 4 apprentice=1, journeyman=2, expert=3, artisan=4, master=5, grand master=6
        } learn_skill_level;

        // ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM               = 41
        struct
        {
            uint32  itemID;                                 // 3
            uint32  itemCount;                              // 4
        } use_item;

        // ACHIEVEMENT_CRITERIA_TYPE_LOOT_ITEM              = 42
        struct
        {
            uint32  itemID;                                 // 3
            uint32  itemCount;                              // 4
        } loot_item;

        // ACHIEVEMENT_CRITERIA_TYPE_EXPLORE_AREA           = 43
        struct
        {
            // TODO: This rank is _NOT_ the index from AreaTable.dbc
            uint32  areaReference;                          // 3
        } explore_area;

        // ACHIEVEMENT_CRITERIA_TYPE_OWN_RANK               = 44
        struct
        {
            // TODO: This rank is _NOT_ the index from CharTitles.dbc
            uint32  rank;                                   // 3
        } own_rank;

        // ACHIEVEMENT_CRITERIA_TYPE_BUY_BANK_SLOT          = 45
        struct
        {
            uint32  unused;                                 // 3
            uint32  numberOfSlots;                          // 4
        } buy_bank_slot;

        // ACHIEVEMENT_CRITERIA_TYPE_GAIN_REPUTATION        = 46
        struct
        {
            uint32  factionID;                              // 3
            uint32  reputationAmount;                       // 4 Total reputation amount, so 42000 = exalted
        } gain_reputation;

        // ACHIEVEMENT_CRITERIA_TYPE_GAIN_EXALTED_REPUTATION= 47
        struct
        {
            uint32  unused;                                 // 3
            uint32  numberOfExaltedFactions;                // 4
        } gain_exalted_reputation;

        // ACHIEVEMENT_CRITERIA_TYPE_VISIT_BARBER_SHOP      = 48
        struct
        {
            uint32 unused;                                  // 3
            uint32 numberOfVisits;                          // 4
        } visit_barber;

        // ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM        = 49
        // TODO: where is the required itemlevel stored?
        struct
        {
            uint32  itemSlot;                               // 3
            uint32  count;                                  // 4
        } equip_epic_item;

        // ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED_ON_LOOT      = 50
        struct
        {
            uint32  rollValue;                              // 3
            uint32  count;                                  // 4
        } roll_need_on_loot;
        // ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED_ON_LOOT      = 51
        struct
        {
            uint32  rollValue;                              // 3
            uint32  count;                                  // 4
        } roll_greed_on_loot;

        // ACHIEVEMENT_CRITERIA_TYPE_HK_CLASS               = 52
        struct
        {
            uint32  classID;                                // 3
            uint32  count;                                  // 4
        } hk_class;

        // ACHIEVEMENT_CRITERIA_TYPE_HK_RACE                = 53
        struct
        {
            uint32  raceID;                                 // 3
            uint32  count;                                  // 4
        } hk_race;

        // ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE               = 54
        // TODO: where is the information about the target stored?
        struct
        {
            uint32  emoteID;                                // 3 enum TextEmotes
            uint32  count;                                  // 4 count of emotes, always required special target or requirements
        } do_emote;
        // ACHIEVEMENT_CRITERIA_TYPE_DAMAGE_DONE            = 13
        // ACHIEVEMENT_CRITERIA_TYPE_HEALING_DONE           = 55
        // ACHIEVEMENT_CRITERIA_TYPE_GET_KILLING_BLOWS      = 56
        struct
        {
            uint32  unused;                                 // 3
            uint32  count;                                  // 4
            uint32  flag;                                   // 5 =3 for battleground healing
            uint32  mapid;                                  // 6
        } healing_done;

        // ACHIEVEMENT_CRITERIA_TYPE_EQUIP_ITEM             = 57
        struct
        {
            uint32  itemID;                                 // 3
            uint32  count;                                  // 4
        } equip_item;

        // ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_QUEST_REWARD= 62
        struct
        {
            uint32  unused;                                 // 3
            uint32  goldInCopper;                           // 4
        } quest_reward_money;

        // ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY             = 67
        struct
        {
            uint32  unused;                                 // 3
            uint32  goldInCopper;                           // 4
        } loot_money;

        // ACHIEVEMENT_CRITERIA_TYPE_USE_GAMEOBJECT         = 68
        struct
        {
            uint32  goEntry;                                // 3
            uint32  useCount;                               // 4
        } use_gameobject;

        // ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL       = 70
        // TODO: are those special criteria stored in the dbc or do we have to add another sql table?
        struct
        {
            uint32  unused;                                 // 3
            uint32  killCount;                              // 4
        } special_pvp_kill;

        // ACHIEVEMENT_CRITERIA_TYPE_FISH_IN_GAMEOBJECT     = 72
        struct
        {
            uint32  goEntry;                                // 3
            uint32  lootCount;                              // 4
        } fish_in_gameobject;

        // ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILLLINE_SPELLS = 75
        struct
        {
            uint32  skillLine;                              // 3
            uint32  spellCount;                             // 4
        } learn_skillline_spell;

        // ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL               = 76
        struct
        {
            uint32  unused;                                 // 3
            uint32  duelCount;                              // 4
        } win_duel;

        // ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_POWER          = 96
        struct
        {
            uint32  powerType;                              // 3 mana=0, 1=rage, 3=energy, 6=runic power
        } highest_power;

        // ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_STAT           = 97
        struct
        {
            uint32  statType;                               // 3 4=spirit, 3=int, 2=stamina, 1=agi, 0=strength
        } highest_stat;

        // ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_SPELLPOWER     = 98
        struct
        {
            uint32  spellSchool;                            // 3
        } highest_spellpower;

        // ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_RATING         = 100
        struct
        {
            uint32  ratingType;                             // 3
        } highest_rating;

        // ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE              = 109
        struct
        {
            uint32  lootType;                               // 3 3=fishing, 2=pickpocket, 4=disentchant
            uint32  lootTypeCount;                          // 4
        } loot_type;

        // ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LINE       = 112
        struct
        {
            uint32  skillLine;                              // 3
            uint32  spellCount;                             // 4
        } learn_skill_line;

        // ACHIEVEMENT_CRITERIA_TYPE_EARN_HONORABLE_KILL    = 113
        struct
        {
            uint32  unused;                                 // 3
            uint32  killCount;                              // 4
        } honorable_kill;

        struct
        {
            uint32  value;                                  // 3        m_asset_id
            uint32  count;                                  // 4        m_quantity
            uint32  additionalRequirement1_type;            // 5        m_start_event
            uint32  additionalRequirement1_value;           // 6        m_start_asset
            uint32  additionalRequirement2_type;            // 7        m_fail_event
            uint32  additionalRequirement2_value;           // 8        m_fail_asset
        } raw;
    };
    char*  name[16];                                        // 9-24     m_description_lang
    // uint32 name_flags;                                   // 25
    uint32  completionFlag;                                 // 26       m_flags
    // uint32  timedCriteriaStartType;                      // 27       m_timer_start_event Only appears with timed achievements, seems to be the type of starting a timed Achievement, only type 1 and some of type 6 need manual starting: 1: ByEventId(?) (serverside IDs), 2: ByQuestId, 5: ByCastSpellId(?), 6: BySpellIdTarget(some of these are unknown spells, some not, some maybe spells), 7: ByKillNpcId,  9: ByUseItemId
    uint32  timedCriteriaMiscId;                            // 28       m_timer_asset_id Alway appears with timed events, used internally to start the achievement, store
    uint32  timeLimit;                                      // 29       m_timer_time
    uint32  showOrder;                                      // 30       m_ui_order also used in achievement shift-links as index in state bitmask

    // helpers
    bool IsExplicitlyStartedTimedCriteria() const
    {
        if (!timeLimit)
        {
            return false;
        }

        // in case raw.value == timedCriteriaMiscId in timedCriteriaMiscId stored spellid/itemids for cast/use, so repeating aura start at first cast/use until fails
        return Type == ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST || raw.value != timedCriteriaMiscId;
    }
};

struct AreaTableEntry
{
    uint32  ID;                                             // 0        m_ID - ID of the Area within the DBC.
    uint32  ContinentID;                                          // 1        m_ContinentID - ID of the Continent in DBC (0 = Azeroth, 1 = Kalimdor, ...)
    uint32  ParentAreaID;                                           // 2        m_ParentAreaID - ID of the parent area.
    uint32  AreaBit;                                    // 3        m_AreaBit -
    uint32  Flags;                                          // 4        m_flags -
    // 5        m_SoundProviderPref
    // 6        m_SoundProviderPrefUnderwater
    // 7        m_AmbienceID
    // 8        m_ZoneMusic
    // 9        m_IntroSound
    int32   ExplorationLevel;                                     // 10       m_ExplorationLevel
    char*   AreaName_lang[16];                                  // 11-26    m_AreaName_lang
    // 27 string Flags
    uint32  FactionGroupMask;                                           // 28       m_factionGroupMask
    uint32  LiquidTypeID[4];                          // 29-32    m_liquidTypeID[4]
    // 33       m_minElevation
    // 34       m_ambient_multiplier
    // 35       m_lightid
};

struct AreaGroupEntry
{
    uint32  AreaGroupId;                                    // 0        m_ID
    uint32  AreaId[6];                                      // 1-6      m_areaID
    uint32  NextAreaID;                                      // 7        m_nextAreaID
};

/**
* \struct AreaTriggerEntry
* \brief Entry representing an area which need to send a specific trigger for quest/resting/..
*/
struct AreaTriggerEntry
{
    uint32    ID;                                           // 0 - ID of the Area within the DBC.
    uint32    ContinentID;                                        // 1 - ID of the Continent in DBC (0 = Azeroth, 1 = Kalimdor, ...)
    float     Pos_0;                                            // 2 - X position of the Area Trigger Entry.
    float     Pos_1;                                            // 3 - Y position of the Area Trigger Entry.
    float     Pos_2;                                            // 4 - Z position of the Area Trigger Entry.
    float     Radius;                                       // 5 - Radius around the Area Trigger point.
    float     Box_length;                                        // 6 - extent Pos_0 edge
    float     Box_width;                                        // 7 - extent Pos_1 edge
    float     Box_height;                                        // 8 - extent Pos_2 edge
    float     Box_yaw;                              // 9 - extent rotation by about Pos_2 axis
};

/**
* \struct AuctionHouseEntry
* \brief Entry representing the different type of Auction House existing within the game and their comission.
*/
struct AuctionHouseEntry
{
    uint32    houseId;                                      // 0        m_ID - ID of the Auction House in the DBC.
    uint32    faction;                                      // 1        m_factionID - ID of the Faction (see faction.dbc).
    uint32    depositPercent;                               // 2        m_depositRate - Percentage taken for any deposit.
    uint32    cutPercent;                                   // 3        m_consignmentRate - Percentage taken for any sell.
    // char*     name[16];                                  // 4-19     m_name_lang
    // 20 string flags
};

/**
* \struct BankBagSlotProcesEntry
* \brief Entry representing the bank bag slot price.
*/
struct BankBagSlotPricesEntry
{
    uint32  ID;                                             // 0        m_ID - ID of the Bank Bag Slot in the DBC.
    uint32  Cost;                                          // 1        m_Cost - Price of the Bank Bag Slot.
};

struct BarberShopStyleEntry
{
    uint32  ID;                                             // 0        m_ID
    uint32  Type;                                           // 1        m_type
    // char*   name[16];                                    // 2-17     m_DisplayName_lang
    // uint32  name_flags;                                  // 18 string flags
    // uint32  unk_name[16];                                // 19-34    m_Description_lang
    // uint32  unk_flags;                                   // 35 string flags
    // float   CostMultiplier;                              // 36       m_Cost_Modifier
    uint32  Race;                                           // 37       m_race
    uint32  Sex;                                         // 38       m_sex
    uint32  Data;                                        // 39       m_data (real ID to hair/facial hair)
};

struct BattlemasterListEntry
{
    uint32  id;                                             // 0        m_ID
    int32   mapid[8];                                       // 1-8      m_mapID[8]
    uint32  instance_type;                                           // 9        m_instanceType
    // uint32 canJoinAsGroup;                               // 10       m_groupsAllowed
    char*   name_lang[16];                                       // 11-26    m_name_lang
    // uint32 nameFlags                                     // 27 string flags
    uint32 max_group_size;                                    // 28       m_maxGroupSize
    uint32 holiday_world_state;                             // 29       m_holidayWorldState
    uint32 min_level;                                        // 30       m_minlevel (sync with PvPDifficulty.dbc content)
    uint32 max_level;                                        // 31       m_maxlevel (sync with PvPDifficulty.dbc content)
};

/*struct Cfg_CategoriesEntry
{
    uint32 Index;                                           //          m_ID categoryId (sent in RealmList packet)
    uint32 Unk1;                                            //          m_localeMask
    uint32 Unk2;                                            //          m_charsetMask
    uint32 IsTournamentRealm;                               //          m_flags
    char *categoryName[16];                                 //          m_name_lang
    uint32 categoryNameFlags;
}*/

/*struct Cfg_ConfigsEntry
{
    uint32 Id;                                              //          m_ID
    uint32 Type;                                            //          m_realmType (sent in RealmList packet)
    uint32 IsPvp;                                           //          m_playerKillingAllowed
    uint32 IsRp;                                            //          m_roleplaying
};*/

#define MAX_OUTFIT_ITEMS 24

/**
* \struct CharStartOutfitEntry
* \brief
*
*/
struct CharStartOutfitEntry
{
    // uint32 ID;                                           // 0        m_ID ('d' sort key, not stored)
    uint8  RaceID;                                          // 1        m_raceID
    uint8  ClassID;                                         // 2        m_classID
    uint8  SexID;                                           // 3        m_sexID
    uint8  OutfitID;                                        // 4        m_outfitID (kept active to 4-align the byte group; server keys on race/class/sex)
    int32  ItemID[MAX_OUTFIT_ITEMS];                        // 5-28     m_ItemID (was ItemId)
    // int32 DisplayItemID[MAX_OUTFIT_ITEMS];               // 29-52    m_DisplayItemID - server-unused ('x')
    // int32 InventoryType[MAX_OUTFIT_ITEMS];               // 53-76    m_InventoryType - server-unused ('x')
    // 77 fields / 296 bytes. RaceClassGender formerly packed fields 1-4; the "Unknown1-3" tail was the 4-byte-modeling artifact.
};

struct CharTitlesEntry
{
    uint32  ID;                                             // 0,       m_ID
    // uint32      unk1;                                    // 1        m_Condition_ID
    char*   Name_lang[16];                                       // 2-17     m_name_lang
    // 18 string flags
    // char*       name2[16];                               // 19-34    m_name1_lang
    // 35 string flags
    uint32  Mask_ID;                                      // 36       m_mask_ID used in PLAYER_CHOSEN_TITLE and 1<<index in PLAYER__FIELD_KNOWN_TITLES
};

/**
* \struct ChatChannelsEntry
* \brief Entry representing default chat channels available in game.
*/
struct ChatChannelsEntry
{
    uint32  ID;                                      // 0        m_ID - ID of the Channel in DBC.
    uint32  Flags;                                          // 1        m_flags - Flags indicating the type of channel (trading, guid recruitment, ...).
    // 2        m_factionGroup
    const char*   Name_lang[16];                              // 3-18     m_name_lang
    // 19 string Flags
    // char*       name[16];                                // 20-35    m_shortcut_lang
    // 36 string Flags
};

/**
* \struct ChrClassesEntry
* \brief Entry representing the classes available in game.
*/
struct ChrClassesEntry
{
    uint32  ID;                                        // 0        m_ID - ID of the Char Class in DBC.
    // uint32 flags;                                        // 1 unknown
    uint32  DisplayPower;                                      // 2        m_DisplayPower
    // 3        m_petNameToken
    char const* Name_lang[16];                                   // 4-19     m_name_lang
    // 20 string flags
    // char*       nameFemale[16];                          // 21-36    m_name_female_lang
    // 37 string flags
    // char*       nameNeutralGender[16];                   // 38-53    m_name_male_lang
    // 54 string flags
    // 55       m_filename
    uint32  SpellClassSet;                                    // 56       m_spellClassSet
    // uint32 flags2;                                       // 57       m_flags (0x08 HasRelicSlot)
    uint32  CinematicSequenceID;                              // 58       m_cinematicSequenceID
    uint32  Required_expansion;                                      // 59       m_required_expansion
};

/**
* \struct ChrRacesEntry
* \brief Entry rerepsenting
*/
struct ChrRacesEntry
{
    uint32      ID;                                     // 0        m_ID - ID of the Char Race in DBC.
    // 1        m_flags
    uint32      FactionID;                                  // 2        m_factionID - ID of the faction in DBC. (See Faction.dbc)
    // 3        m_ExplorationSoundID
    uint32      MaleDisplayID;                                    // 4        m_MaleDisplayId - ID of the Male Display.
    uint32      FemaleDisplayID;                                    // 5        m_FemaleDisplayId - ID of the Female Display.
    // 6        m_ClientPrefix
    uint32      BaseLanguage;                                     // 7        m_BaseLanguage (7-Alliance 1-Horde)
    // 8        m_creatureType
    // 9        m_ResSicknessSpellID
    // 10       m_SplashSoundID
    // 11       m_clientFileString
    uint32      CinematicSequenceID;                          // 12       m_cinematicSequenceID
    // uint32    unk_322;                                   // 13       m_alliance (0 alliance, 1 horde, 2 not available?)
    char*       Name_lang[16];                                   // 14-29    m_name_lang used for DBC language detection/selection
    // 30 string flags
    // char*       nameFemale[16];                          // 31-46    m_name_female_lang
    // 47 string flags
    // char*       nameNeutralGender[16];                   // 48-63    m_name_male_lang
    // 64 string flags
    // 65-66    m_facialHairCustomization[2]
    // 67       m_hairCustomization
    uint32      Required_expansion;                                  // 68       m_required_expansion
};

/*struct CinematicCameraEntry
{
    uint32      id;                                         // 0        m_ID
    char*       filename;                                   // 1        m_model
    uint32      soundid;                                    // 2        m_soundID
    float       start_x;                                    // 3        m_originX
    float       start_y;                                    // 4        m_originY
    float       start_z;                                    // 5        m_originZ
    float       unk6;                                       // 6        m_originFacing
};*/

/**
* \struct CinematicSequencesEntry
*/
struct CinematicSequencesEntry
{
    uint32      ID;                                         // 0        m_ID - ID in DBC.
    // uint32      unk1;                                    // 1        m_soundID
    // uint32      cinematicCamera;                         // 2        m_camera[8]
};

/**
* \struct CreatureDisplayInfoEntry
* \brief Entry representing the display info.
*/
struct CreatureDisplayInfoEntry
{
    uint32      ID;                                  // 0        m_ID - ID in DBC.
    uint32      ModelID;                                    // 1        m_modelID
    // 2        m_soundID
    uint32      ExtendedDisplayInfoID;                      // 3        m_extendedDisplayInfoID - Extended info (see CreatureDisplayInfoExtraEntry).
    float       CreatureModelScale;                                      // 4        m_creatureModelScale - Scale of the Creature.
    // 5        m_creatureModelAlpha
    // 6-8      m_textureVariation[3]
    // 9        m_portraitTextureName
    // 10       m_sizeClass
    // 11       m_bloodID
    // 12       m_NPCSoundID
    // 13       m_particleColorID
    // 14       m_creatureGeosetData
    // 15       m_objectEffectPackageID
};

struct CreatureModelDataEntry
{
    uint32  ID;
    uint32  Flags;
    //char*  ModelPath[16]
    //uint32 Unk1;
    float    ModelScale; // Used in calculation of unit collision data
    //int32  Unk2
    //int32  Unk3
    //uint32 Unk4
    //uint32 Unk5
    //float  Unk6
    //uint32 Unk7
    //float  Unk8
    //uint32 Unk9
    //uint32 Unk10
    //float  CollisionWidth;
    float    CollisionHeight;
    float    MountHeight; // Used in calculation of unit collision data when mounted
    //float  Unks[11]
};

/**
* \struct CreatureDisplayInfoExtraEntry
* \brief Entry extending the CreatureDisplayInfoEntry.
*/
struct CreatureDisplayInfoExtraEntry
{
    uint32      DisplayExtraId;                             // 0        m_ID CreatureDisplayInfoEntry::m_extendedDisplayInfoID
    uint32      DisplayRaceID;                                       // 1        m_DisplayRaceID - DisplayRaceID to which it's applicable.
    // uint32    Gender;                                    // 2        m_DisplaySexID
    // uint32    SkinColor;                                 // 3        m_SkinID
    // uint32    FaceType;                                  // 4        m_FaceID
    // uint32    HairType;                                  // 5        m_HairStyleID
    // uint32    HairStyle;                                 // 6        m_HairColorID
    // uint32    BeardStyle;                                // 7        m_FacialHairID
    // uint32    Equipment[11];                             // 8-18     m_NPCItemDisplay equipped static items EQUIPMENT_SLOT_HEAD..EQUIPMENT_SLOT_HANDS, client show its by self
    // uint32    CanEquip;                                  // 19       m_flags 0..1 Can equip additional things when used for players
    // char*                                                // 20       m_BakeName CreatureDisplayExtra-*.blp
};

/**
* \struct CreatureFamilyEntry
* \brief Entry representing the different pet available for players.
*/
struct CreatureFamilyEntry
{
    uint32    ID;                                           // 0 - ID in DBC.
    float     MinScale;                                     // 1 - Min Scale of creature within the game.
    uint32    MinScaleLevel;                                // 2 0/1 - Minimum level for which the MinScale is applicable.
    float     MaxScale;                                     // 3 - Max Scale of creature within the game.
    uint32    MaxScaleLevel;                                // 4 0/60 - Maximum level for which the MaxScale is applicable.
    uint32    SkillLine[2];                                 // 5-6 - Skill Lines (See SkillLine.dbc).
    uint32    PetFoodMask;                                  // 7 - Food Mask for the given pet.
    int32   PetTalentType;                                  // 8        m_petTalentType
    // 9        m_categoryEnumID
    char*   Name_lang[16];                                       // 10-25    m_name_lang
    // 26 string flags
    // 27       m_iconFile
};

#define MAX_CREATURE_SPELL_DATA_SLOT 4

/**
* \struct CreatureSpellDataEntry
* \brief Entry representing the different spell available for player's pet.
*/
struct CreatureSpellDataEntry
{
    uint32    ID;                                           // 0        m_ID - ID in DBC.
    uint32    Spells[MAX_CREATURE_SPELL_DATA_SLOT];        // 1-4      m_spells[4] - Spell ID's (see Spell.dbc).
    // uint32    availability[MAX_CREATURE_SPELL_DATA_SLOT];// 4-7      m_availability[4]
};

/**
* \struct CreatureTypeEntry
* \brief Entry representing the different creature type available for player's pet.
*/
struct CreatureTypeEntry
{
    uint32    ID;                                           // 0        m_ID
    // char*   Name[16];                                    // 1-16     m_name_lang
    // 17 string flags
    // uint32    no_expirience;                             // 18       m_flags
};

struct CurrencyCategoryEntry
{
    uint32    ID;                                           // 0        m_ID
    uint32    Flags;                                         // 1        m_flags 0 for known categories and 3 for unknown one (3.0.9)
    char*   Name_lang[16];                                       // 2-17     m_name_lang
    //                                                      // 18 string flags
};

struct CurrencyTypesEntry
{
    // uint32    ID;                                        // 0        m_ID
    uint32    ItemID;                                       // 1        m_itemID used as real index
    // uint32    Category;                                  // 2        m_categoryID may be category
    uint32    BitIndex;                                     // 3        m_bitIndex bit index in PLAYER_FIELD_KNOWN_CURRENCIES (1 << (index-1))
};

struct DestructibleModelDataEntry
{
    uint32 m_ID;                                            // 0        m_ID
    // uint32 unk1;                                         // 1
    // uint32 unk2;                                         // 2
    uint32 damagedDisplayId;                                // 3
    // uint32 unk4;                                         // 4
    // uint32 unk5;                                         // 5
    // uint32 unk6;                                         // 6
    uint32 destroyedDisplayId;                              // 7
    // uint32 unk8;                                         // 8
    // uint32 unk9;                                         // 9
    // uint32 unk10;                                        // 10
    uint32 rebuildingDisplayId;                             // 11       // Maybe rebuildingDisplayIdWhileDestroyed
    // uint32 unk12;                                        // 12
    // uint32 unk13;                                        // 13
    // uint32 unk14;                                        // 14
    // uint32 unk15;                                        // 15
    // uint32 unk16;                                        // 16
    // uint32 unk17;                                        // 17
    // uint32 unk18;                                        // 18
};

struct DungeonEncounterEntry
{
    uint32 ID;                                              // 0        m_ID
    uint32 MapID;                                           // 1        m_mapID
    uint32 Difficulty;                                      // 2        m_difficulty
    uint32 OrderIndex;                                   // 3        m_orderIndex
    uint32 Bit;                                  // 4        m_Bit
    char*  Name_lang[16];                               // 5-20     m_name_lang
    // uint32 nameLangFlags;                                // 21       m_name_lang_flags
    // uint32 spellIconID;                                  // 22       m_spellIconID
};

/**
* \struct DurabilityCostsEntry
* \brief Entry representing the multipliers for item reparation cost.
*/
struct DurabilityCostsEntry
{
    uint32    ID;                                      // 0        m_ID - ID in DBC.
    uint32    WeaponSubClassCost[29];                               // 1-29     m_weaponSubClassCost m_armorSubClassCost
};

/**
* \struct DurabilityQualityEntry
* \brief Entry representing the quality modifier for item reparation cost.
*/
struct DurabilityQualityEntry
{
    uint32    ID;                                           // 0        m_ID - ID in DBC.
    float     Data;                                  // 1        m_data - Quality modifier values.
};

/**
* \struct EmotesEntry
* \brief Entry representing the emotes available.
*/
struct EmotesEntry
{
    uint32  Id;                                             // 0        m_ID - ID in DBC.
    // char*   Name;                                        // 1        m_EmoteSlashCommand
    // uint32  AnimationId;                                 // 2        m_AnimID
    uint32  EmoteFlags;                                          // 3        m_EmoteFlags
    uint32  EmoteSpecProc;                                      // 4        m_EmoteSpecProc (determine how emote are shown)
    uint32  EmoteSpecProcParam;                                 // 5        m_EmoteSpecProcParam
    // uint32  SoundId;                                     // 6        m_EventSoundID
};

/**
* \struct EmotesTextEntry
* \brief Entry repsenting the text for given emote.
*/
struct EmotesTextEntry
{
    uint32  ID;                                             //          m_ID - ID in DBC.
    //          m_name
    uint32  EmoteID;                                         //          m_emoteID - ID of the text.
    //          m_emoteText
};

/**
* \struct FactionEntry
* \brief Entry representing all the factions available.
*/
struct FactionEntry
{
    uint32      ID;                                         // 0        m_ID - ID in DBC.
    int32       ReputationIndex;                           // 1        m_reputationIndex - ID of the Reputation List.
    uint32      ReputationRaceMask[4];                         // 2-5      m_reputationRaceMask -
    uint32      ReputationClassMask[4];                        // 6-9      m_reputationClassMask
    int32       ReputationBase[4];                            // 10-13    m_reputationBase
    uint32      ReputationFlags[4];                         // 14-17    m_reputationFlags
    uint32      ParentFactionID;                                       // 18       m_parentFactionID
    float       ParentFactionMod_0;                            // 19       m_parentFactionMod[2] Faction gains incoming rep * ParentFactionMod_0
    float       ParentFactionMod_1;                           // 20       Faction outputs rep * ParentFactionMod_1 as spillover reputation
    uint32      ParentFactionCap_0;                         // 21       m_parentFactionCap[2] The highest rank the faction will profit from incoming spillover
    // uint32    spilloverRank_unk;                         // 22       It does not seem to be the max standing at which a faction outputs spillover ...so no idea
    char*       Name_lang[16];                                   // 23-38    m_name_lang
    // 39 string flags
    // char*     description[16];                           // 40-55    m_description_lang
    // 56 string flags

    // helpers

    int GetIndexFitTo(uint32 raceMask, uint32 classMask) const
    {
        for (int i = 0; i < 4; ++i)
        {
            if ((ReputationRaceMask[i] == 0 || (ReputationRaceMask[i] & raceMask)) &&
                (ReputationClassMask[i] == 0 || (ReputationClassMask[i] & classMask)))
            {
                return i;
            }
        }

        return -1;
    }
};

/**
* \struct FactionTemplateEntry
* \brief Entry representing the type of faction that exists.
*/
struct FactionTemplateEntry
{
    /// 0
    uint32      ID;
    /// 1
    uint32      Faction;
    /// 2 specific flags for that Faction
    uint32      Flags;
    /// 3 if mask set (see FactionMasks) then Faction included in masked team
    uint32      FactionGroup;
    /// 4 if mask set (see FactionMasks) then Faction friendly to masked team
    uint32      FriendGroup;
    /// 5 if mask set (see FactionMasks) then Faction hostile to masked team
    uint32      EnemyGroup;
    /// 6-9
    uint32      Enemies[4];
    /// 10-13
    uint32      Friend[4];
    //-------------------------------------------------------  end structure

    // helpers
    bool IsFriendlyTo(FactionTemplateEntry const& entry) const
    {
        if (entry.Faction)
        {
            for (int i = 0; i < 4; ++i)
            {
                if (Enemies[i]  == entry.Faction)
                {
                    return false;
                }
            }

            for (int i = 0; i < 4; ++i)
            {
                if (Friend[i] == entry.Faction)
                {
                    return true;
                }
            }
        }
        return (FriendGroup & entry.FactionGroup) || (FactionGroup & entry.FriendGroup);
    }
    bool IsHostileTo(FactionTemplateEntry const& entry) const
    {
        if (entry.Faction)
        {
            for (int i = 0; i < 4; ++i)
            {
                if (Enemies[i]  == entry.Faction)
                {
                    return true;
                }
            }

            for (int i = 0; i < 4; ++i)
            {
                if (Friend[i] == entry.Faction)
                {
                    return false;
                }
            }
        }
        return (EnemyGroup & entry.FactionGroup) != 0;
    }
    bool IsHostileToPlayers() const { return (EnemyGroup & FACTION_MASK_PLAYER) != 0; }
    bool IsNeutralToAll() const
    {
        for (int i = 0; i < 4; ++i)
        {
            if (Enemies[i] != 0)
            {
                return false;
            }
        }
        return EnemyGroup == 0 && FriendGroup == 0;
    }
    bool IsContestedGuardFaction() const { return (Flags & FACTION_TEMPLATE_FLAG_CONTESTED_GUARD) != 0; }
};

/**
* \struct GameObjectDisplayInfoEntry
* \brief Entry representing the info for the game object to be displayed on the client.
*/
struct GameObjectDisplayInfoEntry
{
    uint32      ID;                                  // 0        m_ID - ID in DBC.
    char*       ModelName;                                   // 1        m_modelName - File name for  the object.
    // uint32   m_Sound[10];                                // 2-11     m_Sound
    float GeoBoxMin_0;                                       // 12 m_geoBoxMinX (use first value as interact dist, mostly in hacks way)
    float GeoBoxMin_1;                                       // 13 m_geoBoxMinY
    float GeoBoxMin_2;                                       // 14 m_geoBoxMinZ
    float GeoBoxMax_0;                                       // 15 m_geoBoxMaxX
    float GeoBoxMax_1;                                       // 16 m_geoBoxMaxY
    float GeoBoxMax_2;                                       // 17 m_geoBoxMaxZ
    // uint32 unknown18; // 18 m_objectEffectPackageID
};

struct GemPropertiesEntry
{
    uint32      ID;                                         //          m_id
    uint32      EnchantID;                      //          m_enchant_id
    //          m_maxcount_inv
    //          m_maxcount_item
    uint32      Type;                                      //          m_type
};

struct GlyphPropertiesEntry
{
    uint32  Id;                                             //          m_id
    uint32  SpellID;                                        //          m_spellID
    uint32  GlyphSlotFlags;                                      //          m_glyphSlotFlags
    uint32  SpellIconID;                                           //          m_spellIconID
};

struct GlyphSlotEntry
{
    uint32  Id;                                             //          m_id
    uint32  Type;                                      //          m_type
    uint32  Tooltip;                                          //          m_tooltip
};

// All Gt* DBC store data for 100 levels, some by 100 per class/race
#define GT_MAX_LEVEL    100
// gtOCTClassCombatRatingScalar.dbc stores data for 32 ratings, look at MAX_COMBAT_RATING for real used amount
#define GT_MAX_RATING   32

struct GtBarberShopCostBaseEntry
{
    float   Data;
};

struct GtCombatRatingsEntry
{
    float    Data;
};

struct GtChanceToMeleeCritBaseEntry
{
    float    Data;
};

struct GtChanceToMeleeCritEntry
{
    float    Data;
};

struct GtChanceToSpellCritBaseEntry
{
    float    Data;
};

struct GtChanceToSpellCritEntry
{
    float    Data;
};

struct GtOCTClassCombatRatingScalarEntry
{
    float    Data;
};

struct GtOCTRegenHPEntry
{
    float    Data;
};

// struct GtOCTRegenMPEntry
//{
//    float    ratio;
//};

struct GtRegenHPPerSptEntry
{
    float    ratio;
};

struct GtRegenMPPerSptEntry
{
    float    Data;
};

/*struct HolidayDescriptionsEntry
{
    uint32 ID;                                              // 0        m_ID this is NOT holiday id
    // char*     name[16]                                   // 1-16     m_name_lang
                                                            // 17 string flags
};*/

struct HolidayNamesEntry
{
    uint32 ID;                                              // 0        m_ID this is NOT holiday id
    // char*     name[16]                                   // 1-16     m_name_lang
                                                            // 17 string flags
};

struct HolidaysEntry
{
    uint32 ID;                                              // 0        m_ID
    // uint32 duration[10];                                 // 1-10     m_duration
    // uint32 date[26];                                     // 11-36    m_date (dates in unix time starting at January, 1, 2000)
    // uint32 region;                                       // 37       m_region (wow region)
    // uint32 looping;                                      // 38       m_looping
    // uint32 calendarFlags[10];                            // 39-48    m_calendarFlags
    // uint32 holidayNameId;                                // 49       m_holidayNameID (HolidayNames.dbc)
    // uint32 holidayDescriptionId;                         // 50       m_holidayDescriptionID (HolidayDescriptions.dbc)
    // char *textureFilename;                               // 51       m_textureFilename
    // uint32 priority;                                     // 52       m_priority
    // uint32 calendarFilterType;                           // 53       m_calendarFilterType (-1,0,1 or 2)
    // uint32 flags;                                        // 54       m_flags
};

struct ItemEntry
{
    uint32   ID;                                            // 0        m_ID
    uint32   ClassID;                                         // 1        m_classID
    uint32   SubclassID;                                      // 2        m_subclassID (some items have strange subclasses)
    int32    SoundOverrideSubclassID;                                          // 3        m_sound_override_subclassid
    int32    Material;                                      // 4        m_material
    uint32   DisplayInfoID;                                     // 5        m_displayInfoID
    uint32   InventoryType;                                 // 6        m_inventoryType
    uint32   SheatheType;                                        // 7        m_sheatheType
};

/**
* \struct ItemBagFamilyEntry
* \brief Entry representing the existing bag family.
*/
struct ItemBagFamilyEntry
{
    uint32   ID;                                            // 0        m_ID
    // char*     name[16]                                   // 1-16     m_name_lang
    //                                                      // 17       name flags
};

/**
* \struct ItemClassEntry
* \brief Entry representing the item class type.
*/
struct ItemClassEntry
{
    uint32   ClassID;                                            // 0        m_ID
    // uint32   unk1;                                       // 1
    // uint32   unk2;                                       // 2        only weapon have 1 in field, other 0
    char*    ClassName_lang[16];                                      // 3-19     m_name_lang
    //                                                      // 20       ClassName_lang flags
};

struct ItemDisplayInfoEntry
{
    uint32      ID;                                         // 0        m_ID
    // 1        m_modelName[2]
    // 2        m_modelTexture[2]
    // 3        m_inventoryIcon
    // 4        m_geosetGroup[3]
    // 5        m_flags
    // 6        m_spellVisualID
    // 7        m_groupSoundIndex
    // 8        m_helmetGeosetVis[2]
    // 9        m_texture[2]
    // 10       m_itemVisual[8]
    // 11       m_particleColorID
};

// struct ItemCondExtCostsEntry
//{
//    uint32      ID;
//    uint32      condExtendedCost;                         // ItemPrototype::CondExtendedCost
//    uint32      itemextendedcostentry;                    // ItemPrototype::ExtendedCost
//    uint32      arenaseason;                              // arena season number(1-4)
//};

#define MAX_EXTENDED_COST_ITEMS 5

struct ItemExtendedCostEntry
{
    uint32      ID;                                         // 0        m_ID
    uint32      HonorPoints;                             // 1        m_honorPoints
    uint32      ArenaPoints;                             // 2        m_arenaPoints
    uint32      ArenaBracket;                               // 4        m_arenaBracket
    uint32      ItemID[MAX_EXTENDED_COST_ITEMS];           // 5-8      m_itemID
    uint32      ItemCount[MAX_EXTENDED_COST_ITEMS];      // 9-13     m_itemCount
    uint32      RequiredArenaRating;                     // 14       m_requiredArenaRating
    // 15       m_itemPurchaseGroup
};

struct ItemLimitCategoryEntry
{
    uint32      ID;                                         // 0 Id     m_ID
    // char*     name[16]                                   // 1-16     m_name_lang
    // 17 string flags
    uint32      Quantity;                                   // 18       m_quantity max allowed equipped as item or in gem slot
    uint32      Flags;                                       // 19       m_flags 0 = have, 1 = equip (enum ItemLimitCategoryMode)
};

/**
* \struct ItemRandomPropertiesEntry
* \brief Entry representing the random enchant for Items.
*/
struct ItemRandomPropertiesEntry
{
    uint32    ID;                                           // 0        m_ID
    // char*     internalName                               // 1        m_Name
    uint32    Enchantment[5];                                // 2-6      m_Enchantment
    char*     Name_lang[16];                               // 7-22     m_name_lang
    // 23 string flags
};

struct ItemRandomSuffixEntry
{
    uint32    ID;                                           // 0        m_ID
    char*     Name_lang[16];                               // 1-16     m_name_lang
    // 17 string flags
    // 18       m_internalName
    uint32    Enchantment[5];                                // 19-21    m_enchantment
    uint32    AllocationPct[5];                                    // 22-24    m_allocationPct
};

/**
* \struct ItemSetEntry
* \brief Entry representing the Set of items within the game.
*/
struct ItemSetEntry
{
    // uint32    id                                         // 0        m_ID
    char*     Name_lang[16];                                     // 1-16     m_name_lang
    // 17 string flags
    // uint32    itemId[17];                                // 18-34    m_itemID
    uint32    SetSpellID[8];                                    // 35-42    m_setSpellID
    uint32    SetThreshold[8];                     // 43-50    m_setThreshold
    uint32    RequiredSkill;                            // 51       m_requiredSkill
    uint32    RequiredSkillRank;                         // 52       m_requiredSkillRank
};

struct LfgDungeonsEntry
{
    uint32 ID;                                              // 0    m_ID
    char*  Name_lang[16];                                        // 1-16 m_name_lang
    uint32 MinLevel;                                        // 18    m_minLevel
    uint32 MaxLevel;                                        // 19    m_maxLevel
    uint32 Target_level;                                     // 20    m_target_level
    uint32 Target_level_min;                                  // 21    m_target_level_min
    uint32 Target_level_max;                                  // 22    m_target_level_max
    int32  MapID;                                           // 23    m_mapID
    uint32 Difficulty;                                      // 24    m_difficulty
    uint32 Flags;                                           // 25    m_flags
    uint32 TypeID;                                          // 26    m_typeID
    //uint32 faction;                                       // 27    m_faction
    //char* textureFilename;                                // 28    m_textureFilename
    uint32 ExpansionLevel;                                  // 29    m_expansionLevel
    uint32 Order_index;                                      // 30    m_order_index
    uint32 Group_ID;                                         // 31    m_group_id
    //char* description[16]; // 32-49 m_Description_lang

    uint32 Entry() const { return ID + ((uint8)TypeID << 24); }
};

/*struct LfgDungeonGroupEntry
{
    m_ID
    m_name_lang
    m_order_index
    m_parent_group_id
    m_typeid
};*/

/*struct LfgDungeonExpansionEntry
{
    m_ID
    m_lfg_id
    m_expansion_level
    m_random_id
    m_hard_level_min
    m_hard_level_max
    m_target_level_min
    m_target_level_max
};*/

/**
* \struct LiquidTypeEntry
* \brief Entry representing the type of liquid within the game.
*/
struct LiquidTypeEntry
{
    uint32 ID;                                              // 0
    char* Name;                                            // 1        m_Name - liquid name string (opaque-now-typed: was skipped 'x')
    //uint32 Flags;                                         // 2 Water: 1|2|4|8, Magma: 8|16|32|64, Slime: 2|64|256, WMO Ocean: 1|2|4|8|512
    uint32 Type;                                            // 3 0: Water, 1: Ocean, 2: Magma, 3: Slime
    //uint32 SoundId;                                       // 4 Reference to SoundEntries.dbc
    uint32 SpellID;                                         // 5 Reference to Spell.dbc
    //float MaxDarkenDepth;                                 // 6 Only oceans got values here!
    //float FogDarkenIntensity;                             // 7 Only oceans got values here!
    //float AmbDarkenIntensity;                             // 8 Only oceans got values here!
    //float DirDarkenIntensity;                             // 9 Only oceans got values here!
    //uint32 LightID;                                       // 10 Only Slime (6) and Magma (7)
    //float ParticleScale;                                  // 11 0: Slime, 1: Water/Ocean, 4: Magma
    //uint32 ParticleMovement;                              // 12
    //uint32 ParticleTexSlots;                              // 13
    //uint32 LiquidMaterialID;                              // 14
    //char* Texture[6];                                     // 15-20
    //uint32 Color[2];                                      // 21-22
    //float Unk1[18];                                       // 23-40 Most likely these are attributes for the shaders. Water: (23, TextureTilesPerBlock),(24, Rotation) Magma: (23, AnimationX),(24, AnimationY)
    //uint32 Unk2[4];                                       // 41-44
};

#define MAX_LOCK_CASE 8

/**
* \struct LockEntry
* \brief Entry representing the different "locks" existing in game (chest, veins, herbs, ...).
*/
struct LockEntry
{
    uint32      ID;                                         // 0        m_ID
    uint32      Type[MAX_LOCK_CASE];                        // 1-8      m_Type
    uint32      Index[MAX_LOCK_CASE];                       // 9-16     m_Index
    uint32      Skill[MAX_LOCK_CASE];                       // 17-24    m_Skill
    // uint32      Action[MAX_LOCK_CASE];                   // 25-32    m_Action
};


/**
* \struct MailTemplateEntry
* \brief Entry representing a mail template for quest result.
*/
struct MailTemplateEntry
{
    uint32      ID;                                         // 0        m_ID
    // char*       subject[16];                             // 1-16     m_subject_lang
    // 17 string flags
    char*       Body_lang[16];                                // 18-33    m_body_lang
};

/**
* \struct MapEntry
* \brief Entry representing maps existing within the game.
*/
struct MapEntry
{
    uint32  ID;                                          // 0        m_ID
    // char*       internalname;                            // 1        m_Directory
    uint32  InstanceType;                                       // 2        m_InstanceType
    // uint32 mapFlags;                                     // 3        m_Flags (0x100 - CAN_CHANGE_PLAYER_DIFFICULTY)
    // uint32 isPvP;                                        // 4        m_PVP 0 or 1 for battlegrounds (not arenas)
    char*   MapName_lang[16];                                       // 5-20     m_MapName_lang
    // 21 string flags
    uint32  AreaTableID;                                    // 22       m_areaTableID
    // char*     hordeIntro[16];                            // 23-38    m_MapDescription0_lang
    // 39 string flags
    // char*     allianceIntro[16];                         // 40-55    m_MapDescription1_lang
    // 56 string flags
    uint32  LoadingScreenID;                                    // 57       m_LoadingScreenID (LoadingScreens.dbc)
    // float   BattlefieldMapIconScale;                     // 58       m_minimapIconScale
    int32   CorpseMapID;                             // 59       m_corpseMapID map_id of entrance map in ghost mode (continent always and in most cases = normal entrance)
    float   Corpse_0;                               // 60       m_corpseX entrance x coordinate in ghost mode  (in most cases = normal entrance)
    float   Corpse_1;                               // 61       m_corpseY entrance y coordinate in ghost mode  (in most cases = normal entrance)
    // uint32  timeOfDayOverride;                           // 62       m_timeOfDayOverride
    uint32  ExpansionID;                                          // 63       m_expansionID
    // 64       m_raidOffset
    // uint32 maxPlayers;                                   // 65       m_maxPlayers

    // Helpers
    uint32 Expansion() const { return ExpansionID; }

    bool IsDungeon() const { return InstanceType == MAP_INSTANCE || InstanceType == MAP_RAID; }
    bool IsNonRaidDungeon() const { return InstanceType == MAP_INSTANCE; }
    bool Instanceable() const { return InstanceType == MAP_INSTANCE || InstanceType == MAP_RAID || InstanceType == MAP_BATTLEGROUND || InstanceType == MAP_ARENA; }
    bool IsRaid() const { return InstanceType == MAP_RAID; }
    bool IsBattleGround() const { return InstanceType == MAP_BATTLEGROUND; }
    bool IsBattleArena() const { return InstanceType == MAP_ARENA; }
    bool IsBattleGroundOrArena() const { return InstanceType == MAP_BATTLEGROUND || InstanceType == MAP_ARENA; }

    bool IsMountAllowed() const
    {
        return !IsDungeon() ||
               ID == 209 || ID == 269 || ID == 309 || // TanarisInstance, CavernsOfTime, Zul'gurub
               ID == 509 || ID == 534 || ID == 560 || // AhnQiraj, HyjalPast, HillsbradPast
               ID == 568 || ID == 580 || ID == 595 || // ZulAman, Sunwell Plateau, Culling of Stratholme
               ID == 603 || ID == 615 || ID == 616;// Ulduar, The Obsidian Sanctum, The Eye Of Eternity
    }

    bool IsContinent() const
    {
        return ID == 0 || ID == 1 || ID == 530 || ID == 571;
    }
};

struct MapDifficultyEntry
{
    // uint32      Id;                                      // 0        m_ID
    uint32      MapId;                                      // 1        m_mapID
    uint32      Difficulty;                                 // 2        m_difficulty (for arenas: arena slot)
    // char*       areaTriggerText[16];                     // 3-18     m_message_lang (text showed when transfer to map failed)
    // uint32      textFlags;                               // 19
    uint32      RaidDuration;                                  // 20       m_raidDuration in secs, 0 if no fixed reset time
    uint32      MaxPlayers;                                 // 21       m_maxPlayers some heroic versions have 0 when expected same amount as in normal version
    // char*       difficultyString;                        // 22       m_difficultystring
};

struct MovieEntry
{
    uint32      Id;                                         // 0        m_ID
    // char*       filename;                                // 1        m_filename
    // uint32      unk2;                                    // 2        m_volume
};

#define MAX_OVERRIDE_SPELLS     10

struct OverrideSpellDataEntry
{
    uint32      Id;                                         // 0        m_ID
    uint32      Spells[MAX_OVERRIDE_SPELLS];                // 1-10     m_spells
    // uint32      unk2;                                    // 11       m_flags
};

struct PowerDisplayEntry
{
    uint32 ID;                                              // 0        m_ID (was id)
    uint32 ActualType;                                      // 1        m_actualType (was power)
    // char* GlobalStringBaseTag;                           // 2        m_globalStringBaseTag - server-unused ('x')
    // uint8 Red;                                           // 3        m_red   - server-unused ('X', 1 byte)
    // uint8 Green;                                          // 4        m_green - server-unused ('X', 1 byte)
    // uint8 Blue;                                           // 5        m_blue  - server-unused ('X', 1 byte)
};

struct PvPDifficultyEntry
{
    // uint32      id;                                      // 0        m_ID
    uint32      MapID;                                      // 1        m_mapID
    uint32      RangeIndex;                                  // 2        m_rangeIndex
    uint32      MinLevel;                                   // 3        m_minLevel
    uint32      MaxLevel;                                   // 4        m_maxLevel
    uint32      Difficulty;                                 // 5        m_difficulty

    // helpers
    BattleGroundBracketId GetBracketId() const { return BattleGroundBracketId(RangeIndex); }
};

struct QuestFactionRewardEntry
{
    uint32      ID;                                         // 0        m_ID
    int32       Difficulty[10];                            // 1-10     m_Difficulty
};
/**
* \struct QuestSortEntry
* \brief Entry representing the type of quest within the game.
*/
struct QuestSortEntry
{
    uint32      ID;                                         // 0        m_ID
    // char*       name[16];                                // 1-16     m_SortName_lang
    // 17 string flags
};

struct QuestXPLevel
{
    uint32      ID;                                 // 0        m_ID
    uint32      Difficulty[10];                                // 1-10     m_difficulty[10]
};

struct RandomPropertiesPointsEntry
{
    // uint32  Id;                                          // 0        m_ID
    uint32    ID;                                    // 1        m_ItemLevel
    uint32    Epic[5];                      // 2-6      m_Epic
    uint32    Superior[5];                      // 7-11     m_Superior
    uint32    Good[5];                  // 12-16    m_Good
};

struct ScalingStatDistributionEntry
{
    uint32  ID;                                             // 0        m_ID
    int32   StatID[10];                                    // 1-10     m_statID
    uint32  Bonus[10];                                   // 11-20    m_bonus
    uint32  Maxlevel;                                       // 21       m_maxlevel
};

struct ScalingStatValuesEntry
{
    uint32  ID;                                             // 0        m_ID
    uint32  Charlevel;                                          // 1        m_charlevel
    uint32  ShoulderBudget[4];                               // 2-5 Multiplier for ScalingStatDistribution
    uint32  ClothShoulderArmor[4];                                    // 6-9 Armor for level
    uint32  WeaponDPS1H[6];                                      // 10-15 DPS mod for level
    uint32  SpellPower;                                     // 16 spell power for level
    uint32  PrimaryBudget;                                 // 17 there's data from 3.1 dbc ShoulderBudget[3]
    uint32  TertiaryBudget;                                 // 18 3.3
    // uint32 unk2;                                         // 19 unk, probably also Armor for level (flag 0x80000?)
    uint32  ClothChestArmor[4];                                   // 20-23 Armor for level

    /*struct ScalingStatValuesEntry
    {
        m_ID
        m_charlevel
        m_shoulderBudget
        m_trinketBudget
        m_weaponBudget1H
        m_rangedBudget
        m_clothShoulderArmor
        m_leatherShoulderArmor
        m_mailShoulderArmor
        m_plateShoulderArmor
        m_weaponDPS1H
        m_weaponDPS2H
        m_spellcasterDPS1H
        m_spellcasterDPS2H
        m_rangedDPS
        m_wandDPS
        m_spellPower
        m_primaryBudget
        m_tertiaryBudget
        m_clothCloakArmor
        m_clothChestArmor
        m_leatherChestArmor
        m_mailChestArmor
        m_plateChestArmor
    };*/
    uint32  getssdMultiplier(uint32 mask) const
    {
        if (mask & 0x4001F)
        {
            if (mask & 0x00000001)
            {
                return ShoulderBudget[0];
            }
            if (mask & 0x00000002)
            {
                return ShoulderBudget[1];
            }
            if (mask & 0x00000004)
            {
                return ShoulderBudget[2];
            }
            if (mask & 0x00000008)
            {
                return PrimaryBudget;
            }
            if (mask & 0x00000010)
            {
                return ShoulderBudget[3];
            }
            if (mask & 0x00040000)
            {
                return TertiaryBudget;
            }
        }
        return 0;
    }

    uint32  getArmorMod(uint32 mask) const
    {
        if (mask & 0x00F001E0)
        {
            if (mask & 0x00000020)
            {
                return ClothShoulderArmor[0];
            }
            if (mask & 0x00000040)
            {
                return ClothShoulderArmor[1];
            }
            if (mask & 0x00000080)
            {
                return ClothShoulderArmor[2];
            }
            if (mask & 0x00000100)
            {
                return ClothShoulderArmor[3];
            }
            if (mask & 0x00100000)
            {
                return ClothChestArmor[0];     // cloth
            }
            if (mask & 0x00200000)
            {
                return ClothChestArmor[1];     // leather
            }
            if (mask & 0x00400000)
            {
                return ClothChestArmor[2];     // mail
            }
            if (mask & 0x00800000)
            {
                return ClothChestArmor[3];     // plate
            }
        }
        return 0;
    }

    uint32 getDPSMod(uint32 mask) const
    {
        if (mask & 0x7E00)
        {
            if (mask & 0x00000200)
            {
                return WeaponDPS1H[0];
            }
            if (mask & 0x00000400)
            {
                return WeaponDPS1H[1];
            }
            if (mask & 0x00000800)
            {
                return WeaponDPS1H[2];
            }
            if (mask & 0x00001000)
            {
                return WeaponDPS1H[3];
            }
            if (mask & 0x00002000)
            {
                return WeaponDPS1H[4];
            }
            if (mask & 0x00004000)
            {
                return WeaponDPS1H[5];        // not used?
            }
        }
        return 0;
    }

    uint32 getSpellBonus(uint32 mask) const
    {
        if (mask & 0x00008000)
        {
            return SpellPower;
        }
        return 0;
    }

    uint32 getFeralBonus(uint32 mask) const                 // removed in 3.2.x?
    {
        if (mask & 0x00010000)                              // not used?
        {
            return 0;
        }
        return 0;
    }
};

/*struct SkillLineCategoryEntry
{
    uint32    id;                                           // 0        m_ID
    char*     name[16];                                     // 1-17     m_name_lang
                                                            // 18 string flags
    uint32    displayOrder;                                 // 19       m_sortIndex
};*/

/**
* \struct SkillRaceClassInfoEntry
* \brief Entry representing the available skills for classes (weapons, gear, ..)
*/
struct SkillRaceClassInfoEntry
{
    // uint32    id;                                        // 0        m_ID
    uint32    skillId;                                      // 1        m_skillID
    uint32    raceMask;                                     // 2        m_raceMask
    uint32    classMask;                                    // 3        m_classMask
    uint32    flags;                                        // 4        m_flags
    uint32    reqLevel;                                     // 5        m_minLevel
    // uint32    skillTierId;                               // 6        m_skillTierID
    // uint32    skillCostID;                               // 7        m_skillCostIndex
};

/*struct SkillTiersEntry{
    uint32    id;                                           // 0        m_ID
    uint32    skillValue[16];                               // 1-17     m_cost
    uint32    maxSkillValue[16];                            // 18-3     m_valueMax
};*/

/**
* \struct SkillLineEntry
* \brief Entry representing the type of skill line (fire, frost, racial, ...).
*/
struct SkillLineEntry
{
    uint32    ID;                                           // 0        m_ID
    int32     CategoryID;                                   // 1        m_categoryID
    // uint32    skillCostID;                               // 2        m_skillCostsID
    char*     DisplayName_lang[16];                                     // 3-18     m_displayName_lang
    // 19 string flags
    // char*     description[16];                           // 20-35    m_description_lang
    // 36 string flags
    uint32    SpellIconID;                                    // 37       m_spellIconID
    // char*     alternateVerb[16];                         // 38-53    m_alternateVerb_lang
    // 54 string flags
    uint32    CanLink;                                      // 55       m_canLink (prof. with recipes)
};

/**
* \struct SkillLineAbilityEntry
* \brief Entry representing the skill line abilities, also contains information about learning conditions.
*/
struct SkillLineAbilityEntry
{
    uint32    ID;                                           // 0        m_ID
    uint32    SkillLine;                                      // 1        m_skillLine
    uint32    Spell;                                      // 2        m_spell
    uint32    RaceMask;                                     // 3        m_raceMask
    uint32    ClassMask;                                    // 4        m_classMask
    // uint32    racemaskNot;                               // 5        m_excludeRace
    // uint32    classmaskNot;                              // 6        m_excludeClass
    uint32    MinSkillLineRank;                              // 7        m_minSkillLineRank
    uint32    SupercededBySpell;                              // 8        m_supercededBySpell
    uint32    AcquireMethod;                              // 9        m_acquireMethod
    uint32    TrivialSkillLineRankHigh;                                    // 10       m_trivialSkillLineRankHigh
    uint32    TrivialSkillLineRankLow;                                    // 11       m_trivialSkillLineRankLow
    // uint32    characterPoints[2];                        // 12-13    m_characterPoints[2]
};

/**
* \struct SoundEntriesEntry
* \brief Entry representing sound for client, used for validation.
*/
struct SoundEntriesEntry
{
    uint32    Id;                                           // 0        m_ID
    // uint32    Type;                                      // 1        m_soundType
    // char*     InternalName;                              // 2        m_name
    // char*     FileName[10];                              // 3-12     m_File[10]
    // uint32    Unk13[10];                                 // 13-22    m_Freq[10]
    // char*     Path;                                      // 23       m_DirectoryBase
    // 24       m_volumeFloat
    // 25       m_flags
    // 26       m_minDistance
    // 27       m_distanceCutoff
    // 28       m_EAXDef
    // 29       m_soundEntriesAdvancedID
};

/**
* \struct ClassFamilyMask
* \brief Used to compare spells and determine if they belong to the same family.
*/
struct ClassFamilyMask
{
    // Flags of the class family.
    uint64 Flags;
    uint32 Flags2;

    /**
    * Default constructor.
    */
    ClassFamilyMask() : Flags(0), Flags2(0) {}

    /**
    * Constructor taking familyFlags as parameter.
    */
    explicit ClassFamilyMask(uint64 familyFlags, uint32 familyFlags2 = 0) : Flags(familyFlags), Flags2(familyFlags2) {}

    /**
    * function indicating whether the class is empty ( = 0) or not.
    * Returns a boolean value.
    */
    bool Empty() const { return Flags == 0 && Flags2 == 0; }

    /**
    * function overloading the operator !
    * Returns a boolean value.
    */
    bool operator!() const { return Empty(); }

    operator void const* () const { return Empty() ? NULL : this; } // for allow normal use in if (mask)

    /**
    * function indicating whether a familyFlags belongs to a Spell Family.
    * Does a bitwise comparison between current Flags and familyFlags given in parameter.
    * Returns a boolean value.
    * \param familyFlags The familyFlags to compare.
    * \param familyFlags2.
    */
    bool IsFitToFamilyMask(uint64 familyFlags, uint32 familyFlags2 = 0) const
    {
        return (Flags & familyFlags) || (Flags2 & familyFlags2);
    }

    /**
    * function indicating whether a ClassFamilyMask belongs to a Spell Family.
    * Does a bitwise comparison between current Flags and mask's flags.
    * Returns a boolean value.
    * \param mask The ClassFamilyMask to compare.
    */
    bool IsFitToFamilyMask(ClassFamilyMask const& mask) const
    {
        return (Flags & mask.Flags) || (Flags2 & mask.Flags2);
    }

    /**
    * function overloading the operator & for bitwise comparison.
    */
    uint64 operator& (uint64 mask) const                    // possible will removed at finish convertion code use IsFitToFamilyMask
    {
        return Flags & mask;
    }

    /**
    * function overloading operator |=.
    */
    ClassFamilyMask& operator|= (ClassFamilyMask const& mask)
    {
        Flags |= mask.Flags;
        Flags2 |= mask.Flags2;
        return *this;
    }
};

#define MAX_SPELL_REAGENTS 8
#define MAX_SPELL_TOTEMS 2
#define MAX_SPELL_TOTEM_CATEGORIES 2

/**
* \struct SpellEntry
* \brief Entry representing each spell of the game.
*
* This structure also contains flags about spell family, attributes, spell effects
* enchantement, cast conditions, proc conditions, mechanic, cast time, damage range, ...
*
* All we need to know about spells is represented by such entry and used for every effect within the game
* such as elixir, potion, buff, heal, damage, ..
*/
struct SpellEntry
{
        uint32    ID;                                       // 0 normally counted from 0 field (but some tools start counting from 1, check this before tool use for data view!)
        uint32    Category;                                 // 1        m_category
        uint32    DispelType;                                   // 2        m_dispelType
        uint32    Mechanic;                                 // 3        m_mechanic
        uint32    Attributes;                               // 4        m_attributes
        uint32    AttributesEx;                             // 5        m_attributesEx
        uint32    AttributesExB;                            // 6        m_attributesExB
        uint32    AttributesExC;                            // 7        m_attributesExC
        uint32    AttributesExD;                            // 8        m_attributesExD
        uint32    AttributesExE;                            // 9        m_attributesExE
        uint32    AttributesExF;                            // 10       m_attributesExF
        uint32    AttributesExG;                            // 11       m_attributesExG (0x20 - totems, 0x4 - paladin auras, etc...)
        uint32    ShapeshiftMask;                                  // 12       m_shapeshiftMask
        // uint32 ShapeshiftMask;                                // 13       3.2.0
        uint32    ShapeshiftExclude;                               // 14       m_shapeshiftExclude
        // uint32 ShapeshiftExclude;                                // 15       3.2.0
        uint32    Targets;                                  // 16       m_targets
        uint32    TargetCreatureType;                       // 17       m_targetCreatureType
        uint32    RequiresSpellFocus;                       // 18       m_requiresSpellFocus
        uint32    FacingCasterFlags;                        // 19       m_facingCasterFlags
        uint32    CasterAuraState;                          // 20       m_casterAuraState
        uint32    TargetAuraState;                          // 21       m_targetAuraState
        uint32    ExcludeCasterAuraState;                       // 22       m_excludeCasterAuraState
        uint32    ExcludeTargetAuraState;                       // 23       m_excludeTargetAuraState
        uint32    CasterAuraSpell;                          // 24       m_casterAuraSpell
        uint32    TargetAuraSpell;                          // 25       m_targetAuraSpell
        uint32    ExcludeCasterAuraSpell;                   // 26       m_excludeCasterAuraSpell
        uint32    ExcludeTargetAuraSpell;                   // 27       m_excludeTargetAuraSpell
        uint32    CastingTimeIndex;                         // 28       m_castingTimeIndex
        uint32    RecoveryTime;                             // 29       m_recoveryTime
        uint32    CategoryRecoveryTime;                     // 30       m_categoryRecoveryTime
        uint32    InterruptFlags;                           // 31       m_interruptFlags
        uint32    AuraInterruptFlags;                       // 32       m_auraInterruptFlags
        uint32    ChannelInterruptFlags;                    // 33       m_channelInterruptFlags
        uint32    ProcTypeMask;                                // 34       m_procTypeMask
        uint32    ProcChance;                               // 35       m_procChance
        uint32    ProcCharges;                              // 36       m_procCharges
        uint32    MaxLevel;                                 // 37       m_maxLevel
        uint32    BaseLevel;                                // 38       m_baseLevel
        uint32    SpellLevel;                               // 39       m_spellLevel
        uint32    DurationIndex;                            // 40       m_durationIndex
        uint32    PowerType;                                // 41       m_powerType
        uint32    ManaCost;                                 // 42       m_manaCost
        uint32    ManaCostPerLevel;                         // 43       m_manaCostPerLevel
        uint32    ManaPerSecond;                            // 44       m_manaPerSecond
        uint32    ManaPerSecondPerLevel;                    // 45       m_manaPerSecondPerLevel
        uint32    RangeIndex;                               // 46       m_rangeIndex
        float     Speed;                                    // 47       m_speed
        // uint32    ModalNextSpell;                        // 48       m_modalNextSpell not used
        uint32    CumulativeAura;                              // 49       m_cumulativeAura
        uint32    Totem[MAX_SPELL_TOTEMS];                  // 50-51    m_totem
        int32     Reagent[MAX_SPELL_REAGENTS];              // 52-59    m_reagent
        uint32    ReagentCount[MAX_SPELL_REAGENTS];         // 60-67    m_reagentCount
        int32     EquippedItemClass;                        // 68       m_equippedItemClass (value)
        int32     EquippedItemSubclass;                 // 69       m_equippedItemSubclass (mask)
        int32     EquippedItemInvTypes;            // 70       m_equippedItemInvTypes (mask)
        uint32    Effect[MAX_EFFECT_INDEX];                 // 71-73    m_effect
        int32     EffectDieSides[MAX_EFFECT_INDEX];         // 74-76    m_effectDieSides
        float     EffectRealPointsPerLevel[MAX_EFFECT_INDEX];   // 77-79    m_effectRealPointsPerLevel
        int32     EffectBasePoints[MAX_EFFECT_INDEX];       // 80-82    m_effectBasePoints (don't must be used in spell/auras explicitly, must be used cached Spell::m_currentBasePoints)
        uint32    EffectMechanic[MAX_EFFECT_INDEX];         // 83-85    m_effectMechanic
        uint32    ImplicitTargetA[MAX_EFFECT_INDEX];  // 86-88    m_implicitTargetA
        uint32    ImplicitTargetB[MAX_EFFECT_INDEX];  // 89-91    m_implicitTargetB
        uint32    EffectRadiusIndex[MAX_EFFECT_INDEX];      // 92-94    m_effectRadiusIndex - spellradius.dbc
        uint32    EffectAura[MAX_EFFECT_INDEX];    // 95-97    m_effectAura
        uint32    EffectAuraPeriod[MAX_EFFECT_INDEX];        // 98-100   m_effectAuraPeriod
        float     EffectAmplitude[MAX_EFFECT_INDEX];    // 101-103  m_effectAmplitude
        uint32    EffectChainTargets[MAX_EFFECT_INDEX];      // 104-106  m_effectChainTargets
        uint32    EffectItemType[MAX_EFFECT_INDEX];         // 107-109  m_effectItemType
        int32     EffectMiscValue[MAX_EFFECT_INDEX];        // 110-112  m_effectMiscValue
        int32     EffectMiscValueB[MAX_EFFECT_INDEX];       // 113-115  m_effectMiscValueB
        uint32    EffectTriggerSpell[MAX_EFFECT_INDEX];     // 116-118  m_effectTriggerSpell
        float     EffectPointsPerCombo[MAX_EFFECT_INDEX];  // 119-121  m_effectPointsPerCombo
        ClassFamilyMask EffectSpellClassMask[MAX_EFFECT_INDEX]; // 122-130  m_effectSpellClassMaskA/B/C, effect 0/1/2
        uint32    SpellVisualID[2];                           // 131-132  m_spellVisualID
        uint32    SpellIconID;                              // 133      m_spellIconID
        uint32    ActiveIconID;                             // 134      m_activeIconID
        // uint32    SpellPriority;                         // 135      m_spellPriority not used
        char*     Name_lang[16];                            // 136-151  m_name_lang
        // uint32    SpellNameFlag;                         // 152      m_name_flag not used
        char*     NameSubtext_lang[16];                                 // 153-168  m_nameSubtext_lang
        // uint32    RankFlags;                             // 169      m_nameSubtext_flag not used
        // char*     Description_lang[16];                       // 170-185  m_description_lang not used
        // uint32    DescriptionFlags;                      // 186      m_description_flag not used
        // char*     AuraDescription_lang[16];                           // 187-202  m_auraDescription_lang not used
        // uint32    ToolTipFlags;                          // 203      m_auraDescription_flag not used
        uint32    ManaCostPct;                       // 204      m_manaCostPct
        uint32    StartRecoveryCategory;                    // 205      m_startRecoveryCategory
        uint32    StartRecoveryTime;                        // 206      m_startRecoveryTime
        uint32    MaxTargetLevel;                           // 207      m_maxTargetLevel
        uint32    SpellClassSet;                          // 208      m_spellClassSet
        ClassFamilyMask SpellClassMask;                   // 209-211  m_spellClassMask NOTE: size is 12 bytes!!!
        uint32    MaxTargets;                       // 212      m_maxTargets
        uint32    DefenseType;                                 // 213      m_defenseType
        uint32    PreventionType;                           // 214      m_preventionType
        // uint32    StanceBarOrder;                        // 215      m_stanceBarOrder not used
        float     EffectChainAmplitude[MAX_EFFECT_INDEX];          // 216-218  m_effectChainAmplitude
        // uint32    MinFactionID;                          // 219      m_minFactionID not used
        // uint32    MinReputation;                         // 220      m_minReputation not used
        // uint32    RequiredAuraVision;                    // 221      m_requiredAuraVision not used
        uint32    RequiredTotemCategoryID[MAX_SPELL_TOTEM_CATEGORIES];// 222-223  m_requiredTotemCategoryID
        int32     RequiredAreasID;                              // 224      m_requiredAreasId
        uint32    SchoolMask;                               // 225      m_schoolMask
        uint32    RuneCostID;                               // 226      m_runeCostID
        // uint32    SpellMissileID;                        // 227      m_spellMissileID
        // uint32  PowerDisplayID;                          // 228      m_powerDisplayID (PowerDisplay.dbc)
        // float   EffectBonusCoefficient[3];               // 229-231  m_effectBonusCoefficient
        // uint32  DescriptionVariablesID;              // 232      m_descriptionVariablesID
        uint32  Difficulty;                          // 233      m_difficulty (SpellDifficulty.dbc)

        /**
        * function calculating the basic damage/snare/... points for a given Spell Effect.
        * Returns an int32 value representing the basic points.
        * \param eff INDEX of the Spell Effect.
        */
        int32 CalculateSimpleValue(SpellEffectIndex eff) const { return EffectBasePoints[eff] + int32(1); }
        ClassFamilyMask const& GetEffectSpellClassMask(SpellEffectIndex effect) const
        {
            return EffectSpellClassMask[effect];
        }

        /**
        * function indicating whether a spell fits to a spell family.
        * Returns a bool value.
        * \param familyFlags The uint64 value of Spell Family Flags.
        * \param familyFlags2.
        */
        bool IsFitToFamilyMask(uint64 familyFlags, uint32 familyFlags2 = 0) const
        {
            return SpellClassMask.IsFitToFamilyMask(familyFlags, familyFlags2);
        }

        /**
        * function indicating whether a spell fits to a spell family based on arguments.
        * Returns a bool value.
        * \param family SpellFamily to which the spell should belong to.
        * \param familyFlags The uint64 value of Spell Family Flags.
        * \param familyFlags2.
        */
        bool IsFitToFamily(SpellFamily family, uint64 familyFlags, uint32 familyFlags2 = 0) const
        {
            return SpellFamily(SpellClassSet) == family && IsFitToFamilyMask(familyFlags, familyFlags2);
        }

        /**
        * function indicating whether a spell fits to a spell class family based on a ClassFamilyMask.
        * Returns a bool value.
        * \param mask ClassFamilyMask representing the class family.
        */
        bool IsFitToFamilyMask(ClassFamilyMask const& mask) const
        {
            return SpellClassMask.IsFitToFamilyMask(mask);
        }

        /**
        * function indicating whether a spell fits to a spell class family based on arguments.
        * Returns a bool value.
        * \param family SpellFamily to which the spell should belong to.
        * \param masl ClassFamilyMask representing the class family.
        */
        bool IsFitToFamily(SpellFamily family, ClassFamilyMask const& mask) const
        {
            return SpellFamily(SpellClassSet) == family && IsFitToFamilyMask(mask);
        }

        /**
        * function indicating whether a spell has an attribute doing bitwise comparison.
        * Returns a bool value.
        * \param attribute SpellAttributes to compare to actual attribute.
        */
        inline bool HasAttribute(SpellAttributes attribute) const { return Attributes & attribute; }

        /**
        * function indicating whether a spell has an attribute doing bitwise comparison.
        * Returns a bool value.
        * \param attribute SpellAttributesEx to compare to actual attributeEx.
        */
        inline bool HasAttribute(SpellAttributesEx attribute) const { return AttributesEx & attribute; }

        /**
        * function indicating whether a spell has an attribute doing bitwise comparison.
        * Returns a bool value.
        * \param attribute SpellAttributesEx2 to compare to actual attributeEx2.
        */
        inline bool HasAttribute(SpellAttributesEx2 attribute) const { return AttributesExB & attribute; }

        /**
        * function indicating whether a spell has an attribute doing bitwise comparison.
        * Returns a bool value.
        * \param attribute SpellAttributesEx3 to compare to actual attributeEx3.
        */
        inline bool HasAttribute(SpellAttributesEx3 attribute) const { return AttributesExC & attribute; }

        /**
        * function indicating whether a spell has an attribute doing bitwise comparison.
        * Returns a bool value.
        * \param attribute SpellAttributesEx4 to compare to actual attributeEx4.
        */
        inline bool HasAttribute(SpellAttributesEx4 attribute) const { return AttributesExD & attribute; }
        inline bool HasAttribute(SpellAttributesEx5 attribute) const { return AttributesExE & attribute; }
        inline bool HasAttribute(SpellAttributesEx6 attribute) const { return AttributesExF & attribute; }
        inline bool HasAttribute(SpellAttributesEx7 attribute) const { return AttributesExG & attribute; }

    private:
        // prevent creating custom entries (copy data from original in fact)
        SpellEntry(SpellEntry const&);                      // DON'T must have implementation

        // catch wrong uses
        template<typename T>
        bool IsFitToFamilyMask(SpellFamily family, T t) const;
};

// A few fields which are required for automated convertion
// NOTE that these fields are count by _skipping_ the fields that are unused!
#define LOADED_SPELLDBC_FIELD_POS_EQUIPPED_ITEM_CLASS  65   // Must be converted to -1
#define LOADED_SPELLDBC_FIELD_POS_SPELLNAME_0          132  // Links to "MaNGOS server-side spell"

/**
* \struct SpellCastTimesEntry
* \brief Entry representing the spell cast time for a given spell.
*/
struct SpellCastTimesEntry
{
    uint32    ID;                                           // 0        m_ID
    int32     Base;                                     // 1        m_base
    // float     CastTimePerLevel;                          // 2        m_perLevel
    // int32     MinCastTime;                               // 3        m_minimum
};

struct SpellFocusObjectEntry
{
    uint32    ID;                                           // 0        m_ID
    // char*     Name[16];                                  // 1-15     m_name_lang
    // 16 string flags
};

/**
* \struct SpellRadiusEntry
* \brief Entry representing the radius of action of some spells.
*/
struct SpellRadiusEntry
{
    uint32    ID;                                           //          m_ID
    float     Radius;                                       //          m_radius
    //          m_radiusPerLevel
    // float     RadiusMax;                                 //          m_radiusMax
};

/**
* \struct SpellRangeEntry
* \brief Entry representing the spell range of spells between which the spellcast is possible.
*/
struct SpellRangeEntry
{
    uint32    ID;                                           // 0        m_ID
    float     RangeMin_0;                                     // 1        m_rangeMin[2]
    float     RangeMin_1;                             // 2
    float     RangeMax_0;                                     // 3        m_rangeMax[2]
    float     RangeMax_1;                             // 4
    // uint32  Flags;                                       // 5        m_flags
    // char*   Name[16];                                    // 6-21     m_displayName_lang
    // uint32  NameFlags;                                   // 22 string flags
    // char*   ShortName[16];                               // 23-38    m_displayNameShort_lang
    // uint32  NameFlags;                                   // 39 string flags
};

struct SpellRuneCostEntry
{
    uint32  ID;                                             // 0        m_ID
    uint32  Blood[3];                                    // 1-3      m_blood m_unholy m_frost (0=blood, 1=frost, 2=unholy)
    uint32  RunicPower;                                  // 4        m_runicPower

    bool NoRuneCost() const { return Blood[0] == 0 && Blood[1] == 0 && Blood[2] == 0; }
    bool NoRunicPowerGain() const { return RunicPower == 0; }
};

/**
* \struct SpellShapeshiftFormEntry
* \brief Entry representing the valid shape shift within the game (stealth, bear, ...).
*/
struct SpellShapeshiftFormEntry
{
    uint32 ID;                                              // 0        m_ID
    // uint32 buttonPosition;                               // 1        m_bonusActionBar
    // char*  Name[16];                                     // 2-17     m_name_lang
    // uint32 NameFlags;                                    // 18 string flags
    uint32 Flags;                                          // 19       m_flags
    int32  CreatureType;                                    // 20       m_creatureType <=0 humanoid, other normal creature types
    // uint32 unk1;                                         // 21       m_attackIconID
    uint32 CombatRoundTime;                                     // 22       m_combatRoundTime
    uint32 CreatureDisplayID_0;                                       // 23       m_creatureDisplayID[4]
    uint32 CreatureDisplayID_1;                                       // 24
    // uint32 unk3;                                         // 25
    // uint32 unk4;                                         // 26
    uint32 PresetSpellID[8];                                      // 27-34    m_presetSpellID[8]
};

struct SpellDifficultyEntry
{
    uint32 ID;                                              // 0        m_ID
    uint32 DifficultySpellID[MAX_DIFFICULTY];                         // 1-4      m_difficultySpellID[4]
};

/**
* \struct SpellDurationEntry
* \brief Entry representing the spell duration.
*/
struct SpellDurationEntry
{
    uint32    ID;                                           //          m_ID
    int32     Duration[3];                                  //          m_duration, m_durationPerLevel, m_maxDuration
};

/**
* \struct SpellItemEnchantmentEntry
* \brief Entry representing the link between a Spell Trigger Enchantement and its enchant.
*/
struct SpellItemEnchantmentEntry
{
    uint32      ID;                                         // 0        m_ID
    // uint32      charges;                                 // 1        m_charges
    uint32      Effect[3];                                    // 2-4      m_effect[3]
    uint32      EffectPointsMin[3];                                  // 5-7      m_effectPointsMin[3]
    // uint32      amount2[3]                               // 8-10     m_effectPointsMax[3]
    uint32      EffectArg[3];                                 // 11-13    m_effectArg[3]
    char*       Name_lang[16];                            // 14-29    m_name_lang[16]
    // uint32      descriptionFlags;                        // 30 string flags
    uint32      ItemVisual;                                    // 31       m_itemVisual
    uint32      Flags;                                       // 32       m_flags
    uint32      SrcItemID;                                      // 33       m_src_itemID
    uint32      ConditionID;                       // 34       m_condition_id
    // uint32      requiredSkill;                           // 35       m_requiredSkillID
    // uint32      requiredSkillValue;                      // 36       m_requiredSkillRank
    // 37       m_minLevel
};

struct SpellItemEnchantmentConditionEntry
{
    uint32  ID;                                             // 0        m_ID
    uint8   Lt_OperandType[5];                                       // 1-5      m_lt_operandType[5]
    // uint32  LT_Operand[5];                               // 6-10     m_lt_operand[5]
    uint8   Operator[5];                                  // 11-15    m_operator[5]
    uint8   Rt_OperandType[5];                                // 15-20    m_rt_operandType[5]
    uint32  Rt_Operand[5];                                       // 21-25    m_rt_operand[5]
    // uint8   Logic[5]                                     // 25-30    m_logic[5]
};

/**
* \struct StableSlotPricesEntry
* \brief Entry representing the price for a stable slot.
*/
struct StableSlotPricesEntry
{
    uint32 ID;                                            //          m_ID
    uint32 Cost;                                           //          m_cost
};

struct SummonPropertiesEntry
{
    uint32  ID;                                             // 0        m_id
    uint32  Control;                                          // 1        m_control (enum SummonPropGroup)
    uint32  Faction;                                      // 2        m_faction
    uint32  Title;                                          // 3        m_title (enum UnitNameSummonTitle)
    uint32  Slot;                                           // 4        m_slot if title = UNITNAME_SUMMON_TITLE_TOTEM, its actual slot (0-6).
    //      if title = UNITNAME_SUMMON_TITLE_COMPANION, slot=6 -> defensive guardian, in other cases criter/minipet
    //      Slot may have other uses, selection of pet type in some cases?
    uint32  Flags;                                          // 5        m_flags (enum SummonPropFlags)
};

#define MAX_TALENT_RANK 5
#define MAX_PET_TALENT_RANK 3                               // use in calculations, expected <= MAX_TALENT_RANK

/**
* \struct TalentEntry
* \brief Entry representing the talent tree and the links between each of them (conditions, ..)
*/
struct TalentEntry
{
    uint32    ID;                                     // 0        m_ID
    uint32    TabID;                                    // 1        m_tabID (TabID.dbc)
    uint32    TierID;                                          // 2        m_tierID
    uint32    ColumnIndex;                                          // 3        m_columnIndex
    uint32    SpellRank[MAX_TALENT_RANK];                      // 4-8      m_spellRank
    // 9-12 part of prev field
    uint32    PrereqTalent_0;                                    // 13       m_prereqTalent (Talent.dbc)
    // 14-15 part of prev field
    uint32    PrereqRank_0;                                // 16       m_prereqRank
    // 17-18 part of prev field
    // uint32  needAddInSpellBook;                          // 19       m_flags also need disable higest ranks on reset talent tree
    // uint32  unk2;                                        // 20       m_requiredSpellID
    // uint64  allowForPet;                                 // 21       m_categoryMask its a 64 bit mask for pet 1<<m_categoryEnumID in CreatureFamily.dbc
};

/**
* \struct TalentTabEntry
* \brief Entry representing the available talents tab for each classes.
*/
struct TalentTabEntry
{
    uint32  ID;                                    // 0        m_ID
    // char* name[16];                                      // 1-16     m_name_lang
    // uint32  nameFlags;                                   // 17 string flags
    // unit32  spellicon;                                   // 18       m_spellIconID
    // 19       m_raceMask
    uint32  ClassMask;                                      // 20       m_classMask
    uint32  PetTalentMask;                                  // 21       m_petTalentMask
    uint32  OrderIndex;                                        // 22       m_orderIndex
    // char* internalname;                                  // 23       m_backgroundFile
};

/**
* \struct TaxiNodesEntry
* \brief Entry representing a taxi node point coming from DBC.
*
* Each Taxi Node is used to be stored as a location for a taxi node NPC inside the game.
* The Taxi Node ID is used within a bitwise comparison with Character.taximask to determine whether the
* nearby Node is known by the player.
*
*/
struct TaxiNodesEntry
{
    uint32    ID;                                           // 0        ID - ID of the Taxi Node in DBC.
    uint32    ContinentID;                                       // 1        m_ContinentID - ID of the Continent in DBC (0 = Azeroth, 1 = Kalimdor, 30 = Alterac Valley)
    float     x;                                            // 2        m_x - X position of the Taxi Node.
    float     y;                                            // 3        m_y - Y position of the Taxi Node.
    float     z;                                            // 4        m_z - Z position of the Taxi Node.
    char*     Name_lang[16];                                     // 5-21     m_Name_lang
    // 22 string flags
    uint32    MountCreatureID[2];                           // 23-24    m_MountCreatureID[2]
};


/**
* \struct TaxiPathEntry
* \brief Entry representing a taxi path between two taxi nodes.
*
* Each Taxi Path is used within the game to determine the price between 2 taxi nodes.
*/
struct TaxiPathEntry
{
    uint32    ID;                                            // 0        ID - ID of the Taxi Path in DBC.
    uint32    FromTaxiNode;                                          // 1        m_from - ID of the Starting Taxi Node of the travel.
    uint32    ToTaxiNode;                                            // 2        m_to - ID of the Ending Taxi Node of the travel.
    uint32    Cost;                                         // 3        m_price - Basic Price of the travel (Unit : Copper).
};

/**
* \struct TaxiPathNodeEntry
* \brief Entry representing a Taxi Path Node - It is not loaded from the DBC but generated from it.
*/
struct TaxiPathNodeEntry
{
    // 0        m_ID - ID in the DBC.
    uint32    PathID;                                         // 1        m_PathID - ID of the PathID in the DBC.
    uint32    NodeIndex;                                        // 2        m_NodeIndex - Index of the Node in the PathID.
    uint32    ContinentID;                                        // 3        m_ContinentID - ID of the Continent in DBC (0 = Azeroth, 1 = Kalimdor, 30 = Alterac Valley)
    float     LocX;                                            // 4        m_LocX - X position of the Node.
    float     LocY;                                            // 5        m_LocY - Y position of the Node.
    float     LocZ;                                            // 6        m_LocZ - Z position of the Node.
    uint32    Flags;                                   // 7        m_flags - Unknown usage.
    uint32    Delay;                                        // 8        m_delay - Unknown usage.
    uint32    ArrivalEventID;                               // 9        m_arrivalEventID
    uint32    DepartureEventID;                             // 10       m_departureEventID
};

struct TeamContributionPoints
{
    // uint32    Entry;                                     // 0        m_ID
    float     Data;                                        // 1        m_data
};

struct TotemCategoryEntry
{
    uint32    ID;                                           // 0        m_ID
    // char*   name[16];                                    // 1-16     m_name_lang
    // 17 string flags
    uint32    TotemCategoryType;                                 // 18       m_totemCategoryType (one for specialization)
    uint32    TotemCategoryMask;                                 // 19       m_totemCategoryMask (compatibility mask for same type: different for totems, compatible from high to low for rods)
};

#define MAX_VEHICLE_SEAT 8

struct VehicleEntry
{
    uint32  ID;                                           // 0
    uint32  Flags;                                        // 1
    float   TurnSpeed;                                    // 2
    float   PitchSpeed;                                   // 3
    float   PitchMin;                                     // 4
    float   PitchMax;                                     // 5
    uint32  SeatID[MAX_VEHICLE_SEAT];                     // 6-13
    float   MouseLookOffsetPitch;                         // 14
    float   CameraFadeDistScalarMin;                      // 15
    float   CameraFadeDistScalarMax;                      // 16
    float   CameraPitchOffset;                            // 17
    float   FacingLimitRight;                             // 18
    float   FacingLimitLeft;                              // 19
    float   MsslTrgtTurnLingering;                        // 20
    float   MsslTrgtPitchLingering;                       // 21
    float   MsslTrgtMouseLingering;                       // 22
    float   MsslTrgtEndOpacity;                           // 23
    float   MsslTrgtArcSpeed;                             // 24
    float   MsslTrgtArcRepeat;                            // 25
    float   MsslTrgtArcWidth;                             // 26
    float   MsslTrgtImpactRadius[2];                      // 27-28
    char*   MsslTrgtArcTexture;                           // 29
    char*   MsslTrgtImpactTexture;                        // 30
    char*   MsslTrgtImpactModel[2];                       // 31-32
    float   CameraYawOffset;                              // 33
    uint32  UiLocomotionType;                             // 34
    float   MsslTrgtImpactTexRadius;                      // 35
    uint32  VehicleUIIndicatorID;                          // 36       m_vehicleUIIndicatorID
    uint32  PowerDisplayID_0;                               // 37
    // 38 new in 3.1
    // 39 new in 3.1
};

struct VehicleSeatEntry
{
    uint32  ID;                                           // 0
    uint32  Flags;                                        // 1
    int32   AttachmentID;                                 // 2
    float   AttachmentOffsetX;                            // 3
    float   AttachmentOffsetY;                            // 4
    float   AttachmentOffsetZ;                            // 5
    float   EnterPreDelay;                                // 6
    float   EnterSpeed;                                   // 7
    float   EnterGravity;                                 // 8
    float   EnterMinDuration;                             // 9
    float   EnterMaxDuration;                             // 10
    float   EnterMinArcHeight;                            // 11
    float   EnterMaxArcHeight;                            // 12
    int32   EnterAnimStart;                               // 13
    int32   EnterAnimLoop;                                // 14
    int32   RideAnimStart;                                // 15
    int32   RideAnimLoop;                                 // 16
    int32   RideUpperAnimStart;                           // 17
    int32   RideUpperAnimLoop;                            // 18
    float   ExitPreDelay;                                 // 19
    float   ExitSpeed;                                    // 20
    float   ExitGravity;                                  // 21
    float   ExitMinDuration;                              // 22
    float   ExitMaxDuration;                              // 23
    float   ExitMinArcHeight;                             // 24
    float   ExitMaxArcHeight;                             // 25
    int32   ExitAnimStart;                                // 26
    int32   ExitAnimLoop;                                 // 27
    int32   ExitAnimEnd;                                  // 28
    float   PassengerYaw;                                 // 29
    float   PassengerPitch;                               // 30
    float   PassengerRoll;                                // 31
    int32   PassengerAttachmentID;                        // 32
    int32   VehicleEnterAnim;                             // 33
    int32   VehicleExitAnim;                              // 34
    int32   VehicleRideAnimLoop;                          // 35
    int32   VehicleEnterAnimBone;                         // 36
    int32   VehicleExitAnimBone;                          // 37
    int32   VehicleRideAnimLoopBone;                      // 38
    float   VehicleEnterAnimDelay;                        // 39
    float   VehicleExitAnimDelay;                         // 40
    uint32  VehicleAbilityDisplay;                        // 41
    uint32  EnterUISoundID;                               // 42
    uint32  ExitUISoundID;                                // 43
    int32   UiSkin;                                       // 44
    uint32  FlagsB;                                       // 45
    // 46       m_cameraEnteringDelay
    // 47       m_cameraEnteringDuration
    // 48       m_cameraExitingDelay
    // 49       m_cameraExitingDuration
    // 50       m_cameraOffsetX
    // 51       m_cameraOffsetY
    // 52       m_cameraOffsetZ
    // 53       m_cameraPosChaseRate
    // 54       m_cameraFacingChaseRate
    // 55       m_cameraEnteringZoom"
    // 56       m_cameraSeatZoomMin
    // 57       m_cameraSeatZoomMax
};

/**
* \struct WMOAreaTableEntry
* \brief Entry representing the links between area, area's name, area's location, ...
*/
struct WMOAreaTableEntry
{
    uint32 ID;                                              // 0        m_ID index
    int32 WMOID;                                           // 1        m_WMOID used in root WMO
    int32 NameSetID;                                            // 2        m_NameSetID used in adt file
    int32 WMOGroupID;                                          // 3        m_WMOGroupID used in group WMO
    // uint32 field4;                                       // 4        m_SoundProviderPref
    // uint32 field5;                                       // 5        m_SoundProviderPrefUnderwater
    // uint32 field6;                                       // 6        m_AmbienceID
    // uint32 field7;                                       // 7        m_ZoneMusic
    // uint32 field8;                                       // 8        m_IntroSound
    uint32 Flags;                                           // 9        m_flags (used for indoor/outdoor determination)
    uint32 AreaTableID;                                          // 10       m_AreaTableID (AreaTable.dbc)
    // char *Name[16];                                      //          m_AreaName_lang
    // uint32 nameFlags;
};

/**
* \struct WorldMapAreaEntry
* \brief Entry representing the location of World Map Area.
*/
struct WorldMapAreaEntry
{
    // uint32  ID;                                          // 0        m_ID
    uint32  MapID;                                         // 1        m_mapID
    uint32  AreaID;                                        // 2        m_areaID index (continent 0 areas ignored)
    // char* internal_name                                  // 3        m_areaName
    float   LocLeft;                                             // 4        m_locLeft
    float   LocRight;                                             // 5        m_locRight
    float   LocTop;                                             // 6        m_locTop
    float   LocBottom;                                             // 7        m_locBottom
    int32   DisplayMapID;                                 // 8        m_displayMapID -1 (MapID have correct map) other: virtual map where zone show (MapID - where zone in fact internally)
    // int32   dungeonMap_id;                               // 9        m_defaultDungeonFloor (DungeonMap.dbc)
    // uint32  someMapID;                                   // 10       m_parentWorldMapID
};

#define MAX_WORLD_MAP_OVERLAY_AREA_IDX 4

struct WorldMapOverlayEntry
{
    uint32    ID;                                           // 0        m_ID
    // uint32    worldMapAreaId;                            // 1        m_mapAreaID (WorldMapArea.dbc)
    uint32    AreaID[MAX_WORLD_MAP_OVERLAY_AREA_IDX];  // 2-5      m_areaID
    // 6        m_mapPointX
    // 7        m_mapPointY
    // char* internal_name                                  // 8        m_textureName
    // 9        m_textureWidth
    // 10       m_textureHeight
    // 11       m_offsetX
    // 12       m_offsetY
    // 13       m_hitRectTop
    // 14       m_hitRectLeft
    // 15       m_hitRectBottom
    // 16       m_hitRectRight
};

/**
* \struct WorldSafeLocsEntry
* \brief Entry representing safe location within the world.
*/
struct WorldSafeLocsEntry
{
    uint32    ID;                                           // 0        m_ID
    uint32    Continent;                                       // 1        m_continent
    float     LocX;                                            // 2        m_locX
    float     LocY;                                            // 3        m_locY
    float     LocZ;                                            // 4        m_locZ
    // char*   name[16]                                     // 5-20     m_AreaName_lang
    // 21 string flags
};

// GCC have alternative #pragma pack() syntax and old gcc version not support pack(pop), also any gcc version not support it at some platform
#if defined( __GNUC__ )
#pragma pack()
#else
#pragma pack(pop)
#endif

typedef std::set<uint32> SpellCategorySet;
typedef std::map<uint32, SpellCategorySet > SpellCategoryStore;
typedef std::set<uint32> PetFamilySpellsSet;
typedef std::map<uint32, PetFamilySpellsSet > PetFamilySpellsStore;

// Structures not used for casting to loaded DBC data and not required then packing
struct TalentSpellPos
{
    TalentSpellPos() : talent_id(0), rank(0) {}
    TalentSpellPos(uint16 _talent_id, uint8 _rank) : talent_id(_talent_id), rank(_rank) {}

    uint16 talent_id;
    uint8  rank;
};

typedef std::map<uint32, TalentSpellPos> TalentSpellPosMap;

struct TaxiPathBySourceAndDestination
{
    TaxiPathBySourceAndDestination() : ID(0), price(0) {}
    TaxiPathBySourceAndDestination(uint32 _id, uint32 _price) : ID(_id), price(_price) {}

    uint32    ID;
    uint32    price;
};
typedef std::map<uint32, TaxiPathBySourceAndDestination> TaxiPathSetForSource;
typedef std::map<uint32, TaxiPathSetForSource> TaxiPathSetBySource;

struct TaxiPathNodePtr
{
    TaxiPathNodePtr() : i_ptr(NULL) {}
    TaxiPathNodePtr(TaxiPathNodeEntry const* ptr) : i_ptr(ptr) {}

    TaxiPathNodeEntry const* i_ptr;

    operator TaxiPathNodeEntry const& () const { return *i_ptr; }
};

typedef Path<TaxiPathNodePtr, TaxiPathNodeEntry const> TaxiPathNodeList;
typedef std::vector<TaxiPathNodeList> TaxiPathNodesByPath;

#define TaxiMaskSize 14
typedef uint32 TaxiMask[TaxiMaskSize];
#endif

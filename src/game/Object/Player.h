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

#ifndef MANGOS_H_PLAYER
#define MANGOS_H_PLAYER

#include "Common.h"
#include "ItemPrototype.h"
#include "Unit.h"
#include "Item.h"

#include "Database/DatabaseEnv.h"
#include "NPCHandler.h"
#include "QuestDef.h"
#include "Group.h"
#include "Bag.h"
#include "WorldSession.h"
#include "Pet.h"
#include "MapReference.h"
#include "Util.h"                                           // for Tokens typedef
#include "AchievementMgr.h"
#include "ReputationMgr.h"
#include "BattleGround.h"
#include "DBCStores.h"
#include "SharedDefines.h"
#include "Chat.h"
#include "GMTicketMgr.h"

#include<string>
#include<vector>

struct Mail;
class Channel;
class DynamicObject;
class Creature;
class PlayerMenu;
class Transport;
class UpdateMask;
class SpellCastTargets;
class PlayerSocial;
class DungeonPersistentState;
class Spell;
class Item;

struct AreaTrigger;

#ifdef ENABLE_PLAYERBOTS
class PlayerbotAI;
class PlayerbotMgr;
#endif

typedef std::deque<Mail*> PlayerMails;

#define PLAYER_MAX_SKILLS           127
#define PLAYER_MAX_DAILY_QUESTS     25
#define PLAYER_EXPLORED_ZONES_SIZE  128

// Note: SPELLMOD_* values is aura types in fact
enum SpellModType
{
    SPELLMOD_FLAT               = 107,                      // SPELL_AURA_ADD_FLAT_MODIFIER
    SPELLMOD_PCT                = 108                       // SPELL_AURA_ADD_PCT_MODIFIER
};

// 2^n internal values, they are never sent to the client
enum PlayerUnderwaterState
{
    UNDERWATER_NONE             = 0x00,
    UNDERWATER_INWATER          = 0x01,                     // terrain type is water and player is afflicted by it
    UNDERWATER_INLAVA           = 0x02,                     // terrain type is lava and player is afflicted by it
    UNDERWATER_INSLIME          = 0x04,                     // terrain type is lava and player is afflicted by it
    UNDERWATER_INDARKWATER      = 0x08,                     // terrain type is dark water and player is afflicted by it

    UNDERWATER_EXIST_TIMERS     = 0x10
};

enum BuyBankSlotResult
{
    ERR_BANKSLOT_FAILED_TOO_MANY    = 0,
    ERR_BANKSLOT_INSUFFICIENT_FUNDS = 1,
    ERR_BANKSLOT_NOTBANKER          = 2,
    ERR_BANKSLOT_OK                 = 3
};

enum PlayerSpellState
{
    PLAYERSPELL_UNCHANGED       = 0,
    PLAYERSPELL_CHANGED         = 1,
    PLAYERSPELL_NEW             = 2,
    PLAYERSPELL_REMOVED         = 3
};

// Structure to hold player spell information
struct PlayerSpell
{
    PlayerSpellState state : 8;  // State of the spell
    bool active            : 1;  // Show in spellbook
    bool dependent         : 1;  // Learned as result of another spell learn, skill grow, quest reward, etc
    bool disabled          : 1;  // First rank has been learned as a result of talent learn but currently talent unlearned, save max learned ranks
};

struct PlayerTalent
{
    TalentEntry const* talentEntry;
    uint32 currentRank;
    PlayerSpellState state;
};

typedef UNORDERED_MAP<uint32, PlayerSpell> PlayerSpellMap;
typedef UNORDERED_MAP<uint32, PlayerTalent> PlayerTalentMap;

// Structure to hold spell cooldown information
struct SpellCooldown
{
    time_t end;    // End time of the cooldown
    uint16 itemid; // Item ID associated with the cooldown
};

typedef std::map<uint32, SpellCooldown> SpellCooldowns;

enum TrainerSpellState
{
    TRAINER_SPELL_GREEN          = 0,
    TRAINER_SPELL_RED            = 1,
    TRAINER_SPELL_GRAY           = 2,
    TRAINER_SPELL_GREEN_DISABLED = 10 // Custom value, not sent to client: formally green but learn not allowed
};

enum ActionButtonUpdateState
{
    ACTIONBUTTON_UNCHANGED      = 0,
    ACTIONBUTTON_CHANGED        = 1,
    ACTIONBUTTON_NEW            = 2,
    ACTIONBUTTON_DELETED        = 3
};

enum ActionButtonType
{
    ACTION_BUTTON_SPELL         = 0x00,
    ACTION_BUTTON_C             = 0x01,                     // click?
    ACTION_BUTTON_EQSET         = 0x20,
    ACTION_BUTTON_MACRO         = 0x40,
    ACTION_BUTTON_CMACRO        = ACTION_BUTTON_C | ACTION_BUTTON_MACRO,
    ACTION_BUTTON_ITEM          = 0x80
};

#define ACTION_BUTTON_ACTION(X) (uint32(X) & 0x00FFFFFF)
#define ACTION_BUTTON_TYPE(X)   ((uint32(X) & 0xFF000000) >> 24)
#define MAX_ACTION_BUTTON_ACTION_VALUE (0x00FFFFFF+1)

// Structure to hold action button information
struct ActionButton
{
    ActionButton() : packedData(0), uState(ACTIONBUTTON_NEW) {}

    uint32 packedData;               // Packed data containing action and type
    ActionButtonUpdateState uState;  // Update state of the action button

    // Helpers
    ActionButtonType GetType() const
    {
        return ActionButtonType(ACTION_BUTTON_TYPE(packedData));
    }
    uint32 GetAction() const
    {
        return ACTION_BUTTON_ACTION(packedData);
    }
    void SetActionAndType(uint32 action, ActionButtonType type)
    {
        uint32 newData = action | (uint32(type) << 24);
        if (newData != packedData || uState == ACTIONBUTTON_DELETED)
        {
            packedData = newData;
            if (uState != ACTIONBUTTON_NEW)
            {
                uState = ACTIONBUTTON_CHANGED;
            }
        }
    }
};

// some action button indexes used in code or clarify structure
enum ActionButtonIndex
{
    ACTION_BUTTON_SHAMAN_TOTEMS_BAR = 132,
};

#define  MAX_ACTION_BUTTONS     144                         // checked in 3.2.0

typedef std::map<uint8, ActionButton> ActionButtonList;

enum GlyphUpdateState
{
    GLYPH_UNCHANGED             = 0,
    GLYPH_CHANGED               = 1,
    GLYPH_NEW                   = 2,
    GLYPH_DELETED               = 3
};

struct Glyph
{
    uint32 id;
    GlyphUpdateState uState;

    Glyph() : id(0), uState(GLYPH_UNCHANGED) { }

    uint32 GetId() { return id; }

    void SetId(uint32 newId)
    {
        if (newId == id)
        {
            return;
        }

        if (id == 0 && uState == GLYPH_UNCHANGED)           // not exist yet in db and already saved
        {
            uState = GLYPH_NEW;
        }
        else if (newId == 0)
        {
            if (uState == GLYPH_NEW)                        // delete before add new -> no change
            {
                uState = GLYPH_UNCHANGED;
            }
            else                                            // delete existing data
            {
                uState = GLYPH_DELETED;
            }
        }
        else if (uState != GLYPH_NEW)                       // if not new data, change current data
        {
            uState = GLYPH_CHANGED;
        }

        id = newId;
    }
};

struct PlayerCreateInfoItem
{
    PlayerCreateInfoItem(uint32 id, uint32 amount) : item_id(id), item_amount(amount) {}

    uint32 item_id;     // Item ID
    uint32 item_amount; // Item amount
};

typedef std::list<PlayerCreateInfoItem> PlayerCreateInfoItems;

// Structure to hold player class level info
struct PlayerClassLevelInfo
{
    PlayerClassLevelInfo() : basehealth(0), basemana(0) {}
    uint16 basehealth; // Base health
    uint16 basemana;   // Base mana
};

// Structure to hold player class info
struct PlayerClassInfo
{
    PlayerClassInfo() : levelInfo(NULL) { }

    PlayerClassLevelInfo* levelInfo; // Level info array [level-1] 0..MaxPlayerLevel-1
};

// Structure to hold player level info
struct PlayerLevelInfo
{
    PlayerLevelInfo()
    {
        for (int i = 0; i < MAX_STATS; ++i)
        {
            stats[i] = 0;
        }
    }

    uint8 stats[MAX_STATS]; // Stats array
};

typedef std::list<uint32> PlayerCreateInfoSpells;

// Structure to hold player create info action
struct PlayerCreateInfoAction
{
    PlayerCreateInfoAction() : button(0), type(0), action(0) {}
    PlayerCreateInfoAction(uint8 _button, uint32 _action, uint8 _type) : button(_button), type(_type), action(_action) {}

    uint8 button;  // Button index
    uint8 type;    // Action type
    uint32 action; // Action ID
};

typedef std::list<PlayerCreateInfoAction> PlayerCreateInfoActions;

// Structure to hold player info
struct PlayerInfo
{
    // existence checked by displayId != 0             // existence checked by displayId != 0
    PlayerInfo() : displayId_m(0), displayId_f(0), levelInfo(NULL), areaId(0), mapId(0), orientation(0.0f), positionX(0.0f), positionY(0.0f), positionZ(0.0f) {}

    uint32 mapId;             // Map ID
    uint32 areaId;            // Area ID
    float positionX;          // Position X
    float positionY;          // Position Y
    float positionZ;          // Position Z
    float orientation;        // Orientation
    uint16 displayId_m;       // Display ID for male
    uint16 displayId_f;       // Display ID for female
    PlayerCreateInfoItems item; // Create info items
    PlayerCreateInfoSpells spell; // Create info spells
    PlayerCreateInfoActions action; // Create info actions

    PlayerLevelInfo* levelInfo; // Level info array [level-1] 0..MaxPlayerLevel-1
};

// Structure to hold PvP info
struct PvPInfo
{
    PvPInfo() : inHostileArea(false), endTimer(0) {}

    bool inHostileArea; // Is in hostile area
    time_t endTimer;    // End timer
};

// Structure to hold duel info
struct DuelInfo
{
    DuelInfo() : initiator(NULL), opponent(NULL), startTimer(0), startTime(0), outOfBound(0) {}

    Player* initiator; // Initiator player
    Player* opponent;  // Opponent player
    time_t startTimer; // Start timer
    time_t startTime;  // Start time
    time_t outOfBound; // Out of bound timer
};

// Structure to hold area information
struct Areas
{
    uint32 areaID;   // Area ID
    uint32 areaFlag; // Area flag
    float x1;        // X1 coordinate
    float x2;        // X2 coordinate
    float y1;        // Y1 coordinate
    float y2;        // Y2 coordinate
};

#define MAX_RUNES               6
#define RUNE_COOLDOWN           (2*5*IN_MILLISECONDS)       // msec

enum RuneType
{
    RUNE_BLOOD                  = 0,
    RUNE_UNHOLY                 = 1,
    RUNE_FROST                  = 2,
    RUNE_DEATH                  = 3,
    NUM_RUNE_TYPES              = 4
};

struct RuneInfo
{
    uint8  BaseRune;
    uint8  CurrentRune;
    uint16 Cooldown;                                        // msec
};

struct Runes
{
    RuneInfo runes[MAX_RUNES];
    uint8 runeState;                                        // mask of available runes

    void SetRuneState(uint8 index, bool set = true)
    {
        if (set)
        {
            runeState |= (1 << index);                      // usable
        }
        else
        {
            runeState &= ~(1 << index);                     // on cooldown
        }
    }
};

struct EnchantDuration
{
    EnchantDuration() : item(NULL), slot(MAX_ENCHANTMENT_SLOT), leftduration(0) {};
    EnchantDuration(Item* _item, EnchantmentSlot _slot, uint32 _leftduration) : item(_item), slot(_slot), leftduration(_leftduration) { MANGOS_ASSERT(item); };

    Item* item;             // Item pointer
    EnchantmentSlot slot;   // Enchantment slot
    uint32 leftduration;    // Left duration
};

typedef std::list<EnchantDuration> EnchantDurationList;
typedef std::list<Item*> ItemDurationList;

enum LfgRoles
{
    LEADER                      = 0x01,
    TANK                        = 0x02,
    HEALER                      = 0x04,
    DAMAGE                      = 0x08
};

enum RaidGroupError
{
    ERR_RAID_GROUP_NONE                 = 0,
    ERR_RAID_GROUP_LOWLEVEL             = 1,
    ERR_RAID_GROUP_ONLY                 = 2,
    ERR_RAID_GROUP_FULL                 = 3,
    ERR_RAID_GROUP_REQUIREMENTS_UNMATCH = 4
};

enum DrunkenState
{
    DRUNKEN_SOBER               = 0,
    DRUNKEN_TIPSY               = 1,
    DRUNKEN_DRUNK               = 2,
    DRUNKEN_SMASHED             = 3
};

#define MAX_DRUNKEN             4

enum PlayerFlags
{
    PLAYER_FLAGS_NONE                   = 0x00000000,
    PLAYER_FLAGS_GROUP_LEADER           = 0x00000001,
    PLAYER_FLAGS_AFK                    = 0x00000002,
    PLAYER_FLAGS_DND                    = 0x00000004,
    PLAYER_FLAGS_GM                     = 0x00000008,
    PLAYER_FLAGS_GHOST                  = 0x00000010,
    PLAYER_FLAGS_RESTING                = 0x00000020,
    PLAYER_FLAGS_UNK7                   = 0x00000040,       // admin?
    PLAYER_FLAGS_UNK8                   = 0x00000080,       // pre-3.0.3 PLAYER_FLAGS_FFA_PVP flag for FFA PVP state
    PLAYER_FLAGS_CONTESTED_PVP          = 0x00000100,       // Player has been involved in a PvP combat and will be attacked by contested guards
    PLAYER_FLAGS_IN_PVP                 = 0x00000200,
    PLAYER_FLAGS_HIDE_HELM              = 0x00000400,
    PLAYER_FLAGS_HIDE_CLOAK             = 0x00000800,
    PLAYER_FLAGS_PARTIAL_PLAY_TIME      = 0x00001000,       // played long time
    PLAYER_FLAGS_NO_PLAY_TIME           = 0x00002000,       // played too long time
    PLAYER_FLAGS_IS_OUT_OF_BOUNDS       = 0x00004000,       // Lua_IsOutOfBounds
    PLAYER_FLAGS_DEVELOPER              = 0x00008000,       // <Dev> chat tag, name prefix
    PLAYER_FLAGS_ENABLE_LOW_LEVEL_RAID  = 0x00010000,       // triggers lua event EVENT_ENABLE_LOW_LEVEL_RAID
    PLAYER_FLAGS_TAXI_BENCHMARK         = 0x00020000,       // taxi benchmark mode (on/off) (2.0.1)
    PLAYER_FLAGS_PVP_TIMER              = 0x00040000,       // 3.0.2, pvp timer active (after you disable pvp manually)
    PLAYER_FLAGS_COMMENTATOR            = 0x00080000,
    PLAYER_FLAGS_UNK21                  = 0x00100000,
    PLAYER_FLAGS_UNK22                  = 0x00200000,
    PLAYER_FLAGS_COMMENTATOR_UBER       = 0x00400000,       // something like COMMENTATOR_CAN_USE_INSTANCE_COMMAND
    PLAYER_FLAGS_UNK24                  = 0x00800000,       // EVENT_SPELL_UPDATE_USABLE and EVENT_UPDATE_SHAPESHIFT_USABLE, disabled all abilitys on tab except autoattack
    PLAYER_FLAGS_UNK25                  = 0x01000000,       // EVENT_SPELL_UPDATE_USABLE and EVENT_UPDATE_SHAPESHIFT_USABLE, disabled all melee ability on tab include autoattack
    PLAYER_FLAGS_XP_USER_DISABLED       = 0x02000000,
};

// Used for PLAYER__FIELD_KNOWN_TITLES field (uint64), (1<<bit_index) without (-1)
// Can't use enum for uint64 values
#define PLAYER_TITLE_DISABLED              UI64LIT(0x0000000000000000)
#define PLAYER_TITLE_NONE                  UI64LIT(0x0000000000000001)
#define PLAYER_TITLE_PRIVATE               UI64LIT(0x0000000000000002) // 1
#define PLAYER_TITLE_CORPORAL              UI64LIT(0x0000000000000004) // 2
#define PLAYER_TITLE_SERGEANT_A            UI64LIT(0x0000000000000008) // 3
#define PLAYER_TITLE_MASTER_SERGEANT       UI64LIT(0x0000000000000010) // 4
#define PLAYER_TITLE_SERGEANT_MAJOR        UI64LIT(0x0000000000000020) // 5
#define PLAYER_TITLE_KNIGHT                UI64LIT(0x0000000000000040) // 6
#define PLAYER_TITLE_KNIGHT_LIEUTENANT     UI64LIT(0x0000000000000080) // 7
#define PLAYER_TITLE_KNIGHT_CAPTAIN        UI64LIT(0x0000000000000100) // 8
#define PLAYER_TITLE_KNIGHT_CHAMPION       UI64LIT(0x0000000000000200) // 9
#define PLAYER_TITLE_LIEUTENANT_COMMANDER  UI64LIT(0x0000000000000400) // 10
#define PLAYER_TITLE_COMMANDER             UI64LIT(0x0000000000000800) // 11
#define PLAYER_TITLE_MARSHAL               UI64LIT(0x0000000000001000) // 12
#define PLAYER_TITLE_FIELD_MARSHAL         UI64LIT(0x0000000000002000) // 13
#define PLAYER_TITLE_GRAND_MARSHAL         UI64LIT(0x0000000000004000) // 14
#define PLAYER_TITLE_SCOUT                 UI64LIT(0x0000000000008000) // 15
#define PLAYER_TITLE_GRUNT                 UI64LIT(0x0000000000010000) // 16
#define PLAYER_TITLE_SERGEANT_H            UI64LIT(0x0000000000020000) // 17
#define PLAYER_TITLE_SENIOR_SERGEANT       UI64LIT(0x0000000000040000) // 18
#define PLAYER_TITLE_FIRST_SERGEANT        UI64LIT(0x0000000000080000) // 19
#define PLAYER_TITLE_STONE_GUARD           UI64LIT(0x0000000000100000) // 20
#define PLAYER_TITLE_BLOOD_GUARD           UI64LIT(0x0000000000200000) // 21
#define PLAYER_TITLE_LEGIONNAIRE           UI64LIT(0x0000000000400000) // 22
#define PLAYER_TITLE_CENTURION             UI64LIT(0x0000000000800000) // 23
#define PLAYER_TITLE_CHAMPION              UI64LIT(0x0000000001000000) // 24
#define PLAYER_TITLE_LIEUTENANT_GENERAL    UI64LIT(0x0000000002000000) // 25
#define PLAYER_TITLE_GENERAL               UI64LIT(0x0000000004000000) // 26
#define PLAYER_TITLE_WARLORD               UI64LIT(0x0000000008000000) // 27
#define PLAYER_TITLE_HIGH_WARLORD          UI64LIT(0x0000000010000000) // 28
#define PLAYER_TITLE_GLADIATOR             UI64LIT(0x0000000020000000) // 29
#define PLAYER_TITLE_DUELIST               UI64LIT(0x0000000040000000) // 30
#define PLAYER_TITLE_RIVAL                 UI64LIT(0x0000000080000000) // 31
#define PLAYER_TITLE_CHALLENGER            UI64LIT(0x0000000100000000) // 32
#define PLAYER_TITLE_SCARAB_LORD           UI64LIT(0x0000000200000000) // 33
#define PLAYER_TITLE_CONQUEROR             UI64LIT(0x0000000400000000) // 34
#define PLAYER_TITLE_JUSTICAR              UI64LIT(0x0000000800000000) // 35
#define PLAYER_TITLE_CHAMPION_OF_THE_NAARU UI64LIT(0x0000001000000000) // 36
#define PLAYER_TITLE_MERCILESS_GLADIATOR   UI64LIT(0x0000002000000000) // 37
#define PLAYER_TITLE_OF_THE_SHATTERED_SUN  UI64LIT(0x0000004000000000) // 38
#define PLAYER_TITLE_HAND_OF_ADAL          UI64LIT(0x0000008000000000) // 39
#define PLAYER_TITLE_VENGEFUL_GLADIATOR    UI64LIT(0x0000010000000000) // 40

#define KNOWN_TITLES_SIZE       3
#define MAX_TITLE_INDEX         (KNOWN_TITLES_SIZE*64)      // 3 uint64 fields

// used in (PLAYER_FIELD_BYTES, 0) byte values
enum PlayerFieldByteFlags
{
    PLAYER_FIELD_BYTE_TRACK_STEALTHED   = 0x02, // Track stealthed units
    PLAYER_FIELD_BYTE_RELEASE_TIMER     = 0x08, // Display time till auto release spirit
    PLAYER_FIELD_BYTE_NO_RELEASE_WINDOW = 0x10  // Display no "release spirit" window at all
};

// used in byte (PLAYER_FIELD_BYTES2,3) values
enum PlayerFieldByte2Flags
{
    PLAYER_FIELD_BYTE2_NONE              = 0x00, // No flags
    PLAYER_FIELD_BYTE2_DETECT_AMORE_0    = 0x02, // SPELL_AURA_DETECT_AMORE, not used as value and maybe not related to, but used in code as base for mask apply
    PLAYER_FIELD_BYTE2_DETECT_AMORE_1    = 0x04, // SPELL_AURA_DETECT_AMORE value 1
    PLAYER_FIELD_BYTE2_DETECT_AMORE_2    = 0x08, // SPELL_AURA_DETECT_AMORE value 2
    PLAYER_FIELD_BYTE2_DETECT_AMORE_3    = 0x10, // SPELL_AURA_DETECT_AMORE value 3
    PLAYER_FIELD_BYTE2_STEALTH           = 0x20, // Stealth mode
    PLAYER_FIELD_BYTE2_INVISIBILITY_GLOW = 0x40  // Invisibility glow effect
};

// Mirror timer types
enum MirrorTimerType
{
    FATIGUE_TIMER               = 0,
    BREATH_TIMER                = 1,
    FIRE_TIMER                  = 2
};

#define MAX_TIMERS              3
#define DISABLED_MIRROR_TIMER   -1

// 2^n values for player extra flags
enum PlayerExtraFlags
{
    // GM abilities
    PLAYER_EXTRA_GM_ON              = 0x0001, // GM mode on
    PLAYER_EXTRA_GM_ACCEPT_TICKETS  = 0x0002, // GM accepts tickets
    PLAYER_EXTRA_ACCEPT_WHISPERS    = 0x0004, // Accept whispers
    PLAYER_EXTRA_TAXICHEAT          = 0x0008, // Taxi cheat mode
    PLAYER_EXTRA_GM_INVISIBLE       = 0x0010, // GM invisible mode
    PLAYER_EXTRA_GM_CHAT            = 0x0020, // Show GM badge in chat messages
    PLAYER_EXTRA_AUCTION_NEUTRAL    = 0x0040, // Neutral auction access
    PLAYER_EXTRA_AUCTION_ENEMY      = 0x0080, // Enemy auction access, overwrites PLAYER_EXTRA_AUCTION_NEUTRAL

    // Other states
    PLAYER_EXTRA_PVP_DEATH          = 0x0100  // Store PvP death status until corpse creation
};

// 2^n values for at login flags
enum AtLoginFlags
{
    AT_LOGIN_NONE                 = 0x00,
    AT_LOGIN_RENAME               = 0x01,
    AT_LOGIN_RESET_SPELLS         = 0x02,
    AT_LOGIN_RESET_TALENTS        = 0x04,
    AT_LOGIN_CUSTOMIZE            = 0x08,
    AT_LOGIN_RESET_PET_TALENTS    = 0x10,
    AT_LOGIN_FIRST                = 0x20,
};

typedef std::map<uint32, QuestStatusData> QuestStatusMap;

// Offsets for quest slots
enum QuestSlotOffsets
{
    QUEST_ID_OFFSET             = 0,
    QUEST_STATE_OFFSET          = 1,
    QUEST_COUNTS_OFFSET         = 2,                        // 2 and 3
    QUEST_TIME_OFFSET           = 4
};

#define MAX_QUEST_OFFSET 5

// State mask for quest slots
enum QuestSlotStateMask
{
    QUEST_STATE_NONE            = 0x0000, // No state
    QUEST_STATE_COMPLETE        = 0x0001, // Quest complete
    QUEST_STATE_FAIL            = 0x0002  // Quest failed
};

// States for skill updates
enum SkillUpdateState
{
    SKILL_UNCHANGED             = 0, // Skill unchanged
    SKILL_CHANGED               = 1, // Skill changed
    SKILL_NEW                   = 2, // New skill
    SKILL_DELETED               = 3  // Skill deleted
};

// Structure to hold skill status data
struct SkillStatusData
{
    SkillStatusData(uint8 _pos, SkillUpdateState _uState) : pos(_pos), uState(_uState) {}

    uint8 pos;              // Position of the skill
    SkillUpdateState uState; // Update state of the skill
};

typedef UNORDERED_MAP<uint32, SkillStatusData> SkillStatusMap;

// Player slots for items
enum PlayerSlots
{
    // First slot for item stored (in any way in player m_items data)
    PLAYER_SLOT_START           = 0,
    // last+1 slot for item stored (in any way in player m_items data)
    PLAYER_SLOT_END             = 150,
    PLAYER_SLOTS_COUNT          = (PLAYER_SLOT_END - PLAYER_SLOT_START)
};

#define INVENTORY_SLOT_BAG_0    255

// Equipment slots (19 slots)
enum EquipmentSlots
{
    EQUIPMENT_SLOT_START        = 0,
    EQUIPMENT_SLOT_HEAD         = 0,  // Head slot
    EQUIPMENT_SLOT_NECK         = 1,  // Neck slot
    EQUIPMENT_SLOT_SHOULDERS    = 2,  // Shoulders slot
    EQUIPMENT_SLOT_BODY         = 3,  // Body slot
    EQUIPMENT_SLOT_CHEST        = 4,  // Chest slot
    EQUIPMENT_SLOT_WAIST        = 5,  // Waist slot
    EQUIPMENT_SLOT_LEGS         = 6,  // Legs slot
    EQUIPMENT_SLOT_FEET         = 7,  // Feet slot
    EQUIPMENT_SLOT_WRISTS       = 8,  // Wrists slot
    EQUIPMENT_SLOT_HANDS        = 9,  // Hands slot
    EQUIPMENT_SLOT_FINGER1      = 10, // First finger slot
    EQUIPMENT_SLOT_FINGER2      = 11, // Second finger slot
    EQUIPMENT_SLOT_TRINKET1     = 12, // First trinket slot
    EQUIPMENT_SLOT_TRINKET2     = 13, // Second trinket slot
    EQUIPMENT_SLOT_BACK         = 14, // Back slot
    EQUIPMENT_SLOT_MAINHAND     = 15, // Main hand slot
    EQUIPMENT_SLOT_OFFHAND      = 16, // Off hand slot
    EQUIPMENT_SLOT_RANGED       = 17, // Ranged slot
    EQUIPMENT_SLOT_TABARD       = 18, // Tabard slot
    EQUIPMENT_SLOT_END          = 19  // End of equipment slots
};

// Inventory slots (4 slots)
enum InventorySlots
{
    INVENTORY_SLOT_BAG_START    = 19, // Start of bag slots
    INVENTORY_SLOT_BAG_END      = 23  // End of bag slots
};

// Inventory pack slots (16 slots)
enum InventoryPackSlots
{
    INVENTORY_SLOT_ITEM_START   = 23, // Start of item slots
    INVENTORY_SLOT_ITEM_END     = 39  // End of item slots
};

// Bank item slots (28 slots)
enum BankItemSlots
{
    BANK_SLOT_ITEM_START        = 39, // Start of bank item slots
    BANK_SLOT_ITEM_END          = 67  // End of bank item slots
};

// Bank bag slots (7 slots)
enum BankBagSlots
{
    BANK_SLOT_BAG_START         = 67, // Start of bank bag slots
    BANK_SLOT_BAG_END           = 74  // End of bank bag slots
};

// Buy back slots (12 slots)
enum BuyBackSlots
{
    // Stored in m_buybackitems
    BUYBACK_SLOT_START          = 74, // Start of buy back slots
    BUYBACK_SLOT_END            = 86  // End of buy back slots
};

// Key ring slots (32 slots)
enum KeyRingSlots
{
    KEYRING_SLOT_START          = 86,  // Start of key ring slots
    KEYRING_SLOT_END            = 118  // End of key ring slots
};

enum CurrencyTokenSlots                                     // 32 slots
{
    CURRENCYTOKEN_SLOT_START    = 118,
    CURRENCYTOKEN_SLOT_END      = 150
};

enum EquipmentSetUpdateState
{
    EQUIPMENT_SET_UNCHANGED     = 0,
    EQUIPMENT_SET_CHANGED       = 1,
    EQUIPMENT_SET_NEW           = 2,
    EQUIPMENT_SET_DELETED       = 3
};

struct EquipmentSet
{
    EquipmentSet() : Guid(0), IgnoreMask(0), state(EQUIPMENT_SET_NEW)
    {
        for (int i = 0; i < EQUIPMENT_SLOT_END; ++i)
        {
            Items[i] = 0;
        }
    }

    uint64 Guid;
    std::string Name;
    std::string IconName;
    uint32 IgnoreMask;
    uint32 Items[EQUIPMENT_SLOT_END];
    EquipmentSetUpdateState state;
};

#define MAX_EQUIPMENT_SET_INDEX 10                          // client limit

typedef std::map<uint32, EquipmentSet> EquipmentSets;

struct ItemPosCount
{
    ItemPosCount(uint16 _pos, uint32 _count) : pos(_pos), count(_count) {}
    bool isContainedIn(std::vector<ItemPosCount> const& vec) const;
    uint16 pos;
    uint32 count;
};

typedef std::vector<ItemPosCount> ItemPosCountVec;

// Trade slots
enum TradeSlots
{
    TRADE_SLOT_COUNT            = 7, // Total trade slots
    TRADE_SLOT_TRADED_COUNT     = 6, // Traded slots count
    TRADE_SLOT_NONTRADED        = 6  // Non-traded slots count
};

// Reasons for transfer abort
enum TransferAbortReason
{
    TRANSFER_ABORT_NONE                         = 0x00,
    TRANSFER_ABORT_ERROR                        = 0x01,
    TRANSFER_ABORT_MAX_PLAYERS                  = 0x02,     // Transfer Aborted: instance is full
    TRANSFER_ABORT_NOT_FOUND                    = 0x03,     // Transfer Aborted: instance not found
    TRANSFER_ABORT_TOO_MANY_INSTANCES           = 0x04,     // You have entered too many instances recently.
    TRANSFER_ABORT_ZONE_IN_COMBAT               = 0x06,     // Unable to zone in while an encounter is in progress.
    TRANSFER_ABORT_INSUF_EXPAN_LVL              = 0x07,     // You must have <TBC,WotLK> expansion installed to access this area.
    TRANSFER_ABORT_DIFFICULTY                   = 0x08,     // <Normal,Heroic,Epic> difficulty mode is not available for %s.
    TRANSFER_ABORT_UNIQUE_MESSAGE               = 0x09,     // Until you've escaped TLK's grasp, you cannot leave this place!
    TRANSFER_ABORT_TOO_MANY_REALM_INSTANCES     = 0x0A,     // Additional instances cannot be launched, please try again later.
    TRANSFER_ABORT_NEED_GROUP                   = 0x0B,     // 3.1
    TRANSFER_ABORT_NOT_FOUND2                   = 0x0C,     // 3.1
    TRANSFER_ABORT_NOT_FOUND3                   = 0x0D,     // 3.1
    TRANSFER_ABORT_NOT_FOUND4                   = 0x0E,     // 3.2
    TRANSFER_ABORT_REALM_ONLY                   = 0x0F,     // All players on party must be from the same realm.
    TRANSFER_ABORT_MAP_NOT_ALLOWED              = 0x10,     // Map can't be entered at this time.
};

// Instance reset warning types
enum InstanceResetWarningType
{
    RAID_INSTANCE_WARNING_HOURS     = 1,                    // WARNING! %s is scheduled to reset in %d hour(s).
    RAID_INSTANCE_WARNING_MIN       = 2,                    // WARNING! %s is scheduled to reset in %d minute(s)!
    RAID_INSTANCE_WARNING_MIN_SOON  = 3,                    // WARNING! %s is scheduled to reset in %d minute(s). Please exit the zone or you will be returned to your bind location!
    RAID_INSTANCE_WELCOME           = 4,                    // Welcome to %s. This raid instance is scheduled to reset in %s.
    RAID_INSTANCE_EXPIRED           = 5
};

// PLAYER_FIELD_ARENA_TEAM_INFO_1_1 offsets
enum ArenaTeamInfoType
{
    ARENA_TEAM_ID               = 0,
    ARENA_TEAM_TYPE             = 1,                        // new in 3.2 - team type?
    ARENA_TEAM_MEMBER           = 2,                        // 0 - captain, 1 - member
    ARENA_TEAM_GAMES_WEEK       = 3,
    ARENA_TEAM_GAMES_SEASON     = 4,
    ARENA_TEAM_WINS_SEASON      = 5,
    ARENA_TEAM_PERSONAL_RATING  = 6,
    ARENA_TEAM_END              = 7
};

// Rest types
enum RestType
{
    REST_TYPE_NO                = 0, // No rest
    REST_TYPE_IN_TAVERN         = 1, // Resting in a tavern
    REST_TYPE_IN_CITY           = 2  // Resting in a city
};

// Duel completion types
enum DuelCompleteType
{
    DUEL_INTERRUPTED            = 0, // Duel interrupted
    DUEL_WON                    = 1, // Duel won
    DUEL_FLED                   = 2  // Duel fled
};

// Teleport options
enum TeleportToOptions
{
    TELE_TO_GM_MODE             = 0x01, // GM mode teleport
    TELE_TO_NOT_LEAVE_TRANSPORT = 0x02, // Do not leave transport
    TELE_TO_NOT_LEAVE_COMBAT    = 0x04, // Do not leave combat
    TELE_TO_NOT_UNSUMMON_PET    = 0x08, // Do not unsummon pet
    TELE_TO_SPELL               = 0x10  // Teleport by spell
};

// Types of environmental damages
enum EnviromentalDamage
{
    DAMAGE_EXHAUSTED            = 0, // Exhausted damage
    DAMAGE_DROWNING             = 1, // Drowning damage
    DAMAGE_FALL                 = 2, // Fall damage
    DAMAGE_LAVA                 = 3, // Lava damage
    DAMAGE_SLIME                = 4, // Slime damage
    DAMAGE_FIRE                 = 5, // Fire damage
    DAMAGE_FALL_TO_VOID         = 6  // Custom case for fall without durability loss
};

// Played time indices
enum PlayedTimeIndex
{
    PLAYED_TIME_TOTAL           = 0, // Total played time
    PLAYED_TIME_LEVEL           = 1  // Played time at current level
};

#define MAX_PLAYED_TIME_INDEX   2

// Used at player loading query list preparing, and later result selection
enum PlayerLoginQueryIndex
{
    PLAYER_LOGIN_QUERY_LOADFROM,
    PLAYER_LOGIN_QUERY_LOADGROUP,
    PLAYER_LOGIN_QUERY_LOADBOUNDINSTANCES,
    PLAYER_LOGIN_QUERY_LOADAURAS,
    PLAYER_LOGIN_QUERY_LOADSPELLS,
    PLAYER_LOGIN_QUERY_LOADQUESTSTATUS,
    PLAYER_LOGIN_QUERY_LOADDAILYQUESTSTATUS,
    PLAYER_LOGIN_QUERY_LOADREPUTATION,
    PLAYER_LOGIN_QUERY_LOADINVENTORY,
    PLAYER_LOGIN_QUERY_LOADITEMLOOT,
    PLAYER_LOGIN_QUERY_LOADACTIONS,
    PLAYER_LOGIN_QUERY_LOADSOCIALLIST,
    PLAYER_LOGIN_QUERY_LOADHOMEBIND,
    PLAYER_LOGIN_QUERY_LOADSPELLCOOLDOWNS,
    PLAYER_LOGIN_QUERY_LOADDECLINEDNAMES,
    PLAYER_LOGIN_QUERY_LOADGUILD,
    PLAYER_LOGIN_QUERY_LOADARENAINFO,
    PLAYER_LOGIN_QUERY_LOADACHIEVEMENTS,
    PLAYER_LOGIN_QUERY_LOADCRITERIAPROGRESS,
    PLAYER_LOGIN_QUERY_LOADEQUIPMENTSETS,
    PLAYER_LOGIN_QUERY_LOADBGDATA,
    PLAYER_LOGIN_QUERY_LOADACCOUNTDATA,
    PLAYER_LOGIN_QUERY_LOADSKILLS,
    PLAYER_LOGIN_QUERY_LOADGLYPHS,
    PLAYER_LOGIN_QUERY_LOADMAILS,
    PLAYER_LOGIN_QUERY_LOADMAILEDITEMS,
    PLAYER_LOGIN_QUERY_LOADTALENTS,
    PLAYER_LOGIN_QUERY_LOADRANDOMBG,
    PLAYER_LOGIN_QUERY_LOADWEEKLYQUESTSTATUS,
    PLAYER_LOGIN_QUERY_LOADMONTHLYQUESTSTATUS,

    MAX_PLAYER_LOGIN_QUERY
};

// Delayed operations for players
enum PlayerDelayedOperations
{
    DELAYED_SAVE_PLAYER         = 0x01,
    DELAYED_RESURRECT_PLAYER    = 0x02,
    DELAYED_SPELL_CAST_DESERTER = 0x04,
    DELAYED_BG_MOUNT_RESTORE    = 0x08,                     ///< Flag to restore mount state after teleport from BG
    DELAYED_BG_TAXI_RESTORE     = 0x10,                     ///< Flag to restore taxi state after teleport from BG
    DELAYED_END
};

// Sources of reputation
enum ReputationSource
{
    REPUTATION_SOURCE_KILL,   // Reputation from kills
    REPUTATION_SOURCE_QUEST,  // Reputation from quests
    REPUTATION_SOURCE_SPELL   // Reputation from spells
};

// Player summoning auto-decline time (in seconds)
#define MAX_PLAYER_SUMMON_DELAY (2*MINUTE)
#define MAX_MONEY_AMOUNT        (0x7FFFFFFF-1) // Maximum money amount

// Structure to hold instance player bind information
struct InstancePlayerBind
{
    DungeonPersistentState* state; // Pointer to the persistent state of the dungeon
    bool perm; // Indicates if the bind is permanent

    /* Permanent PlayerInstanceBinds are created in Raid/Heroic instances for players
       that aren't already permanently bound when they are inside when a boss is killed
       or when they enter an instance that the group leader is permanently bound to. */
    InstancePlayerBind() : state(NULL), perm(false) {}
};

// Enum to represent player rest states
enum PlayerRestState
{
    REST_STATE_RESTED           = 0x01, // Player is rested
    REST_STATE_NORMAL           = 0x02, // Player is in a normal state
    REST_STATE_RAF_LINKED       = 0x04  // Exact use unknown
};

// Class to manage player taxi information
class PlayerTaxi
{
    public:
        PlayerTaxi();
        ~PlayerTaxi() {}
        // Nodes
        void InitTaxiNodesForLevel(uint32 race, uint32 chrClass, uint32 level);
        void LoadTaxiMask(const char* data);

        // Check if a taxi node is known
        bool IsTaximaskNodeKnown(uint32 nodeidx) const
        {
            uint8  field   = uint8((nodeidx - 1) / 32);
            uint32 submask = 1 << ((nodeidx - 1) % 32);
            return (m_taximask[field] & submask) == submask;
        }

        // Set a taxi node as known
        bool SetTaximaskNode(uint32 nodeidx)
        {
            uint8  field   = uint8((nodeidx - 1) / 32);
            uint32 submask = 1 << ((nodeidx - 1) % 32);
            if ((m_taximask[field] & submask) != submask)
            {
                m_taximask[field] |= submask;
                return true;
            }
            else
            {
                return false;
            }
        }

        // Append taxi mask to data
        void AppendTaximaskTo(ByteBuffer& data, bool all);

        // Load taxi destinations from string
        bool LoadTaxiDestinationsFromString(const std::string& values, Team team);

        // Save taxi destinations to string
        std::string SaveTaxiDestinationsToString();

        // Clear taxi destinations
        void ClearTaxiDestinations()
        {
            m_TaxiDestinations.clear();
        }

        // Add a taxi destination
        void AddTaxiDestination(uint32 dest)
        {
            m_TaxiDestinations.push_back(dest);
        }

        // Get the source of the taxi
        uint32 GetTaxiSource() const
        {
            return m_TaxiDestinations.empty() ? 0 : m_TaxiDestinations.front();
        }

        // Get the destination of the taxi
        uint32 GetTaxiDestination() const
        {
            return m_TaxiDestinations.size() < 2 ? 0 : m_TaxiDestinations[1];
        }

        // Get the current taxi path
        uint32 GetCurrentTaxiPath() const;

        // Get the next taxi destination
        uint32 NextTaxiDestination()
        {
            m_TaxiDestinations.pop_front();
            return GetTaxiDestination();
        }

        // Check if there are no taxi destinations
        bool empty() const
        {
            return m_TaxiDestinations.empty();
        }

        // Friend function to output taxi information
        friend std::ostringstream& operator<< (std::ostringstream& ss, PlayerTaxi const& taxi);

    private:
        TaxiMask m_taximask; // Mask of known taxi nodes
        std::deque<uint32> m_TaxiDestinations; // Queue of taxi destinations
};

std::ostringstream& operator<< (std::ostringstream& ss, PlayerTaxi const& taxi);

/// Structure to hold battleground data
struct BGData
{
    BGData() : bgInstanceID(0), bgTypeID(BATTLEGROUND_TYPE_NONE), bgAfkReportedCount(0), bgAfkReportedTimer(0),
        bgTeam(TEAM_NONE), mountSpell(0), m_needSave(false) { ClearTaxiPath(); }

    uint32 bgInstanceID; // Battleground instance ID
    ///  when player is teleported to BG - (it is battleground's GUID)
    BattleGroundTypeId bgTypeID; // Battleground type ID

    std::set<uint32> bgAfkReporter; // Set of players who reported AFK
    uint8 bgAfkReportedCount; // Count of AFK reports
    time_t bgAfkReportedTimer; // Timer for AFK reports

    Team bgTeam; // Team of the player in the battleground

    uint32 mountSpell;                                      ///< Mount used before join to bg, saved
    uint32 taxiPath[2];                                     ///< Current taxi active path start/end nodes, saved

    WorldLocation joinPos;                                  ///< From where player entered BG, saved

    bool m_needSave;                                        ///< true, if saved to DB fields modified after prev. save (marked as "saved" above)

    void ClearTaxiPath()     { taxiPath[0] = taxiPath[1] = 0; }
    bool HasTaxiPath() const { return taxiPath[0] && taxiPath[1]; }
};

// Structure to hold trade status information
struct TradeStatusInfo
{
    TradeStatusInfo() : Status(TRADE_STATUS_BUSY), TraderGuid(), Result(EQUIP_ERR_OK),
        IsTargetResult(false), ItemLimitCategoryId(0), Slot(0) { }

    TradeStatus Status; // Status of the trade
    ObjectGuid TraderGuid; // GUID of the trader
    InventoryResult Result; // Result of the trade
    bool IsTargetResult; // Indicates if the result is for the target
    uint32 ItemLimitCategoryId; // Item limit category ID
    uint8 Slot; // Slot of the item
};

// Class to manage trade data
class TradeData
{
    public: // Constructors
        TradeData(Player* player, Player* trader) :
            m_player(player),  m_trader(trader), m_accepted(false), m_acceptProccess(false),
            m_money(0), m_spell(0) {}

    public: // Access functions

        // Get the trader
        Player* GetTrader() const
        {
            return m_trader;
        }

        // Get the trade data of the trader
        TradeData* GetTraderData() const;

        // Get the item in the specified trade slot
        Item* GetItem(TradeSlots slot) const;

        // Check if the trade has the specified item
        bool HasItem(ObjectGuid item_guid) const;

        // Get the spell applied to the trade
        uint32 GetSpell() const
        {
            return m_spell;
        }

        // Get the item used to cast the spell
        Item* GetSpellCastItem() const;

        // Check if there is a spell cast item
        bool HasSpellCastItem() const
        {
            return !m_spellCastItem.IsEmpty();
        }

        // Get the money placed in the trade
        uint32 GetMoney() const
        {
            return m_money;
        }

        // Check if the trade is accepted
        bool IsAccepted() const
        {
            return m_accepted;
        }

        // Check if the trade is in the accept process
        bool IsInAcceptProcess() const
        {
            return m_acceptProccess;
        }

    public: // Access functions

        // Set the item in the specified trade slot
        void SetItem(TradeSlots slot, Item* item);

        // Set the spell applied to the trade
        void SetSpell(uint32 spell_id, Item* castItem = NULL);

        // Set the money placed in the trade
        void SetMoney(uint32 money);

        // Set the accepted state of the trade
        void SetAccepted(bool state, bool crosssend = false);

        // Set the accept process state of the trade
        void SetInAcceptProcess(bool state)
        {
            m_acceptProccess = state;
        }

    private: // Internal functions

        // Update the trade data
        void Update(bool for_trader = true);

    private: // Fields

        Player* m_player; // Player who owns this TradeData
        Player* m_trader; // Player who trades with m_player

        bool m_accepted; // Indicates if m_player has accepted the trade
        bool m_acceptProccess; // Indicates if the accept process is ongoing

        uint32 m_money; // Money placed in the trade

        uint32 m_spell; // Spell applied to the non-traded slot item
        ObjectGuid m_spellCastItem; // Item used to cast the spell

        ObjectGuid m_items[TRADE_SLOT_COUNT]; // Items traded from m_player's side, including non-traded slot
};

class Player : public Unit
{
        friend class WorldSession;
        friend void Item::AddToUpdateQueueOf(Player* player);
        friend void Item::RemoveFromUpdateQueueOf(Player* player);
    public:
        explicit Player(WorldSession* session);
        ~Player();

        void CleanupsBeforeDelete() override;


        static UpdateMask updateVisualBits; // Update mask for visual bits
        static void InitVisibleBits(); // Initialize visible bits

        void AddToWorld() override; // Add the player to the world
        void RemoveFromWorld() override; // Remove the player from the world

        // Teleport the player to a specific location
        bool TeleportTo(uint32 mapid, float x, float y, float z, float orientation, uint32 options = 0, AreaTrigger const* at = NULL);

        // Teleport the player to a specific location using WorldLocation
        bool TeleportTo(WorldLocation const& loc, uint32 options = 0)
        {
            return TeleportTo(loc.mapid, loc.coord_x, loc.coord_y, loc.coord_z, loc.orientation, options);
        }

        bool TeleportToBGEntryPoint(); // Teleport the player to the battleground entry point

        // Set the summon point for the player
        void SetSummonPoint(uint32 mapid, float x, float y, float z)
        {
            m_summon_expire = time(NULL) + MAX_PLAYER_SUMMON_DELAY;
            m_summon_mapid = mapid;
            m_summon_x = x;
            m_summon_y = y;
            m_summon_z = z;
        }
        void SummonIfPossible(bool agree); // Summon the player if possible

        // Create a new player
        bool Create(uint32 guidlow, const std::string& name, uint8 race, uint8 class_, uint8 gender, uint8 skin, uint8 face, uint8 hairStyle, uint8 hairColor, uint8 facialHair, uint8 outfitId);

        void Update(uint32 update_diff, uint32 time) override; // Update the player

        static bool BuildEnumData(QueryResult* result,  WorldPacket* p_data); // Build enumeration data

        void SetInWater(bool apply); // Set the player in water

        bool IsInWater() const override // Check if the player is in water
        {
            return m_isInWater;
        }
        bool IsUnderWater() const override; // Check if the player is underwater
        bool IsFalling() // Check if the player is falling
        {
            return GetPositionZ() < m_lastFallZ;
        }

        void SendInitialPacketsBeforeAddToMap();
        void SendInitialPacketsAfterAddToMap();
        void SendInstanceResetWarning(uint32 mapid, Difficulty difficulty, uint32 time);

        // Get the NPC if the player can interact with it
        Creature* GetNPCIfCanInteractWith(ObjectGuid guid, uint32 npcflagmask);
        // Get the game object if the player can interact with it
        GameObject* GetGameObjectIfCanInteractWith(ObjectGuid guid, uint32 gameobject_type = MAX_GAMEOBJECT_TYPE) const;

        void ToggleAFK(); // Toggle AFK status
        void ToggleDND(); // Toggle DND status
        bool isAFK() const // Check if the player is AFK
        {
            return HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_AFK);
        }
        bool isDND() const // Check if the player is DND
        {
            return HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_DND);
        }
        ChatTagFlags GetChatTag() const; // Get the chat tag flags
        std::string autoReplyMsg; // Auto-reply message

        uint32 GetBarberShopCost(uint8 newhairstyle, uint8 newhaircolor, uint8 newfacialhair, uint32 newskintone);

        PlayerSocial* GetSocial()
        {
            return m_social;
        }

        void SetCreatedDate(uint32 createdDate) // Set the created date of the player
        {
            m_created_date = createdDate;
        }

        uint32 GetCreatedDate() // Get the created date of the player
        {
            return m_created_date;
        }

        PlayerTaxi m_taxi; // Player's taxi information

        // Initialize taxi nodes for the player's level
        void InitTaxiNodesForLevel()
        {
            m_taxi.InitTaxiNodesForLevel(getRace(), getClass(), getLevel());
        }

        // Activate taxi path to specified nodes
        bool ActivateTaxiPathTo(std::vector<uint32> const& nodes, Creature* npc = NULL, uint32 spellid = 0);

        // Activate taxi path to specified taxi path ID
        bool ActivateTaxiPathTo(uint32 taxi_path_id, uint32 spellid = 0);

        // Continue the taxi flight
        void ContinueTaxiFlight();

        // Check if the player accepts tickets
        bool isAcceptTickets() const { return GetSession()->GetSecurity() >= SEC_GAMEMASTER && (m_ExtraFlags & PLAYER_EXTRA_GM_ACCEPT_TICKETS); }

        // Set the accept ticket state
        void SetAcceptTicket(bool on) { if (on) { m_ExtraFlags |= PLAYER_EXTRA_GM_ACCEPT_TICKETS; } else { m_ExtraFlags &= ~PLAYER_EXTRA_GM_ACCEPT_TICKETS; } }

        // Check if the player accepts whispers
        bool isAcceptWhispers() const { return m_ExtraFlags & PLAYER_EXTRA_ACCEPT_WHISPERS; }

        // Set the accept whispers state
        void SetAcceptWhispers(bool on) { if (on) { m_ExtraFlags |= PLAYER_EXTRA_ACCEPT_WHISPERS; } else { m_ExtraFlags &= ~PLAYER_EXTRA_ACCEPT_WHISPERS; } }

        // Check if the player is a game master
        bool isGameMaster() const { return m_ExtraFlags & PLAYER_EXTRA_GM_ON; }

        // Set the game master state
        void SetGameMaster(bool on);

        // Check if the player has GM chat enabled
        bool isGMChat() const { return GetSession()->GetSecurity() >= SEC_MODERATOR && (m_ExtraFlags & PLAYER_EXTRA_GM_CHAT); }

        // Set the GM chat state
        void SetGMChat(bool on) { if (on) { m_ExtraFlags |= PLAYER_EXTRA_GM_CHAT; } else { m_ExtraFlags &= ~PLAYER_EXTRA_GM_CHAT; } }

        // Check if the player is a taxi cheater
        bool IsTaxiCheater() const { return m_ExtraFlags & PLAYER_EXTRA_TAXICHEAT; }

        // Set the taxi cheater state
        void SetTaxiCheater(bool on) { if (on) { m_ExtraFlags |= PLAYER_EXTRA_TAXICHEAT; } else { m_ExtraFlags &= ~PLAYER_EXTRA_TAXICHEAT; } }

        // Check if the player is visible as a GM
        bool isGMVisible() const { return !(m_ExtraFlags & PLAYER_EXTRA_GM_INVISIBLE); }

        // Set the GM visibility state
        void SetGMVisible(bool on);

        // Set the PvP death state
        void SetPvPDeath(bool on)
        {
            if (on)
            {
                m_ExtraFlags |= PLAYER_EXTRA_PVP_DEATH;
            }
            else
            {
                m_ExtraFlags &= ~PLAYER_EXTRA_PVP_DEATH;
            }
        }

        // Get the auction access mode
        // 0 = own auction, -1 = enemy auction, 1 = goblin auction
        int GetAuctionAccessMode() const
        {
            return m_ExtraFlags & PLAYER_EXTRA_AUCTION_ENEMY ? -1 : (m_ExtraFlags & PLAYER_EXTRA_AUCTION_NEUTRAL ? 1 : 0);
        }

        // Set the auction access mode
        void SetAuctionAccessMode(int state)
        {
            m_ExtraFlags &= ~(PLAYER_EXTRA_AUCTION_ENEMY | PLAYER_EXTRA_AUCTION_NEUTRAL);

            if (state < 0)
            {
                m_ExtraFlags |= PLAYER_EXTRA_AUCTION_ENEMY;
            }
            else if (state > 0)
            {
                m_ExtraFlags |= PLAYER_EXTRA_AUCTION_NEUTRAL;
            }
        }

        // Give experience points to the player
        void GiveXP(uint32 xp, Unit* victim);
        void GiveLevel(uint32 level);   /* DO NOT REMOVE: Used for Eluna compatibility */
        void SetLevel(uint32 level);


        // Initialize stats for the player's level
        void InitStatsForLevel(bool reapplyMods = false);

        // Played Time Stuff
        time_t m_logintime; // Login time
        time_t m_Last_tick; // Last tick time

        uint32 m_Played_time[MAX_PLAYED_TIME_INDEX]; // Played time array

        // Get the total played time
        uint32 GetTotalPlayedTime()
        {
            return m_Played_time[PLAYED_TIME_TOTAL];
        }

        // Get the played time at the current level
        uint32 GetLevelPlayedTime()
        {
            return m_Played_time[PLAYED_TIME_LEVEL];
        }

        // Reset time synchronization
        void ResetTimeSync();

        // Send time synchronization
        void SendTimeSync();

        // Set the death state of the player
        void SetDeathState(DeathState s) override; // overwrite Unit::SetDeathState

        // Get the rest bonus
        float GetRestBonus() const
        {
            return m_rest_bonus;
        }

        // Set the rest bonus
        void SetRestBonus(float rest_bonus_new);

        /**
         * \brief: compute rest bonus
         * \param: time_t timePassed > time from last check
         * \param: bool offline > is the player was offline?
         * \param: bool inRestPlace > if it was offline, is the player was in city/tavern/inn?
         * \returns: float
         **/
        float ComputeRest(time_t timePassed, bool offline = false, bool inRestPlace = false);

        // Get the rest type
        RestType GetRestType() const
        {
            return rest_type;
        }

        // Set the rest type
        void SetRestType(RestType n_r_type, uint32 areaTriggerId = 0);

        // Get the time the player entered the inn
        time_t GetTimeInnEnter() const
        {
            return time_inn_enter;
        }

        // Update the inn enter time
        void UpdateInnerTime(time_t time)
        {
            time_inn_enter = time;
        }

        // Remove the player's pet
        void RemovePet(PetSaveMode mode);

        uint32 GetPhaseMaskForSpawn() const;                // used for proper set phase for DB at GM-mode creature/GO spawn


        // Player communication methods
        void Say(const std::string& text, const uint32 language);
        void Yell(const std::string& text, const uint32 language);
        void TextEmote(const std::string& text);

        /**
         * This will log a whisper depending on the setting LogWhispers in mangosd.conf, for a list
         * of available levels please see \ref WhisperLoggingLevels. The logging is done to database
         * in the table characters.character_whispers and includes to/from, text and when the whisper
         * was sent.
         *
         * @param text the text that was sent
         * @param receiver guid of the receiver of the message
         * \see WhisperLoggingLevels
         * \see eConfigUInt32Values::CONFIG_UINT32_LOG_WHISPERS
         */
        void LogWhisper(const std::string& text, ObjectGuid receiver);
        void Whisper(const std::string& text, const uint32 language, ObjectGuid receiver);

        /*********************************************************/
        /***                    STORAGE SYSTEM                 ***/
        /*********************************************************/

        // Set the virtual item slot
        void SetVirtualItemSlot(uint8 i, Item* item);

        // Set the sheath state (override Unit version)
        void SetSheath(SheathState sheathed) override;

        // Find the equipment slot for the specified item
        uint8 FindEquipSlot(ItemPrototype const* proto, uint32 slot, bool swap) const;

        // Get the count of the specified item
        uint32 GetItemCount(uint32 item, bool inBankAlso = false, Item* skipItem = NULL) const;
        uint32 GetItemCountWithLimitCategory(uint32 limitCategory, Item* skipItem = NULL) const;
        Item* GetItemByGuid(ObjectGuid guid) const;
        Item* GetItemByEntry(uint32 item) const;            // only for special cases
        Item* GetItemByLimitedCategory(uint32 limitedCategory) const;
        Item* GetItemByPos(uint16 pos) const;

        // Get the item by its bag and slot
        Item* GetItemByPos(uint8 bag, uint8 slot) const;

        // Get the display ID of the item in the specified slot
        uint32 GetItemDisplayIdInSlot(uint8 bag, uint8 slot) const;

        // Get the weapon for the specified attack type
        Item* GetWeaponForAttack(WeaponAttackType attackType) const
        {
            return GetWeaponForAttack(attackType, false, false);
        }

        // Get the weapon for the specified attack type with additional options
        Item* GetWeaponForAttack(WeaponAttackType attackType, bool nonbroken, bool useable) const;

        // Get the shield (if usable)
        Item* GetShield(bool useable = false) const;

        // Get the attack type by the slot
        static uint32 GetAttackBySlot(uint8 slot);

        // Get the item update queue
        std::vector<Item*>& GetItemUpdateQueue() { return m_itemUpdateQueue; }

        // Check if the position is an inventory position
        static bool IsInventoryPos(uint16 pos) { return IsInventoryPos(pos >> 8, pos & 255); }

        // Check if the position is an inventory position (overloaded)
        static bool IsInventoryPos(uint8 bag, uint8 slot);

        // Check if the position is an equipment position
        static bool IsEquipmentPos(uint16 pos) { return IsEquipmentPos(pos >> 8, pos & 255); }

        // Check if the position is an equipment position (overloaded)
        static bool IsEquipmentPos(uint8 bag, uint8 slot);

        // Check if the position is a bag position
        static bool IsBagPos(uint16 pos);

        // Check if the position is a bank position
        static bool IsBankPos(uint16 pos) { return IsBankPos(pos >> 8, pos & 255); }

        // Check if the position is a bank position (overloaded)
        static bool IsBankPos(uint8 bag, uint8 slot);

        // Check if the position is valid
        bool IsValidPos(uint16 pos, bool explicit_pos) const { return IsValidPos(pos >> 8, pos & 255, explicit_pos); }

        // Check if the position is valid (overloaded)
        bool IsValidPos(uint8 bag, uint8 slot, bool explicit_pos) const;

        // Get the count of bank bag slots
        uint8 GetBankBagSlotCount() const { return GetByteValue(PLAYER_BYTES_2, 2); }

        // Set the count of bank bag slots
        void SetBankBagSlotCount(uint8 count) { SetByteValue(PLAYER_BYTES_2, 2, count); }

        // Check if the player has the specified item count
        bool HasItemCount(uint32 item, uint32 count, bool inBankAlso = false) const;

        // Check if the player has an item that fits the spell requirements
        bool HasItemFitToSpellReqirements(SpellEntry const* spellInfo, Item const* ignoreItem = NULL);

        // Check if the player can cast the spell without reagents
        bool CanNoReagentCast(SpellEntry const* spellInfo) const;

        // Check if the player has the specified item or gem equipped
        bool HasItemOrGemWithIdEquipped(uint32 item, uint32 count, uint8 except_slot = NULL_SLOT) const;
        bool HasItemOrGemWithLimitCategoryEquipped(uint32 limitCategory, uint32 count, uint8 except_slot = NULL_SLOT) const;
        InventoryResult CanTakeMoreSimilarItems(Item* pItem) const { return _CanTakeMoreSimilarItems(pItem->GetEntry(), pItem->GetCount(), pItem); }

        // Check if the player can take more similar items (overloaded)
        InventoryResult CanTakeMoreSimilarItems(uint32 entry, uint32 count) const { return _CanTakeMoreSimilarItems(entry, count, NULL); }

        // Check if the player can store a new item
        InventoryResult CanStoreNewItem(uint8 bag, uint8 slot, ItemPosCountVec& dest, uint32 item, uint32 count, uint32* no_space_count = NULL) const
        {
            return _CanStoreItem(bag, slot, dest, item, count, NULL, false, no_space_count);
        }

        // Check if the player can store an item
        InventoryResult CanStoreItem(uint8 bag, uint8 slot, ItemPosCountVec& dest, Item* pItem, bool swap = false) const
        {
            if (!pItem)
            {
                return EQUIP_ERR_ITEM_NOT_FOUND;
            }
            uint32 count = pItem->GetCount();
            return _CanStoreItem(bag, slot, dest, pItem->GetEntry(), count, pItem, swap, NULL);
        }

        // Check if the player can store multiple items
        InventoryResult CanStoreItems(Item** pItem, int count) const;

        // Check if the player can equip a new item
        InventoryResult CanEquipNewItem(uint8 slot, uint16& dest, uint32 item, bool swap) const;

        // Check if the player can equip an item
        InventoryResult CanEquipItem(uint8 slot, uint16& dest, Item* pItem, bool swap, bool direct_action = true) const;

        InventoryResult CanEquipUniqueItem(Item* pItem, uint8 except_slot = NULL_SLOT, uint32 limit_count = 1) const;
        InventoryResult CanEquipUniqueItem(ItemPrototype const* itemProto, uint8 except_slot = NULL_SLOT, uint32 limit_count = 1) const;
        InventoryResult CanUnequipItems(uint32 item, uint32 count) const;

        // Check if the player can unequip an item
        InventoryResult CanUnequipItem(uint16 src, bool swap) const;

        // Check if the player can bank an item
        InventoryResult CanBankItem(uint8 bag, uint8 slot, ItemPosCountVec& dest, Item* pItem, bool swap, bool not_loading = true) const;

        // Check if the player can use an item
        InventoryResult CanUseItem(Item* pItem, bool direct_action = true) const;

        // Check if the player has an item with the specified totem category
        bool HasItemTotemCategory(uint32 TotemCategory) const;

        // Check if the player can use an item (overloaded)
        InventoryResult CanUseItem(ItemPrototype const* pItem) const;

        // Check if the player can use ammo
        InventoryResult CanUseAmmo(uint32 item) const;

        // Store a new item
        Item* StoreNewItem(ItemPosCountVec const& pos, uint32 item, bool update, int32 randomPropertyId = 0);

        // Store an item
        Item* StoreItem(ItemPosCountVec const& pos, Item* pItem, bool update);

        // Equip a new item
        Item* EquipNewItem(uint16 pos, uint32 item, bool update);

        // Equip an item
        Item* EquipItem(uint16 pos, Item* pItem, bool update);

        // Automatically unequip the offhand item if needed
        void AutoUnequipOffhandIfNeed();

        // Store a new item in the best slots
        bool StoreNewItemInBestSlots(uint32 item_id, uint32 item_count);

        // Store a new item in the inventory slot
        Item* StoreNewItemInInventorySlot(uint32 itemEntry, uint32 amount);

        // Automatically store loot
        void AutoStoreLoot(WorldObject const* lootTarget, uint32 loot_id, LootStore const& store, bool broadcast = false, uint8 bag = NULL_BAG, uint8 slot = NULL_SLOT);

        // Automatically store loot (overloaded)
        void AutoStoreLoot(Loot& loot, bool broadcast = false, uint8 bag = NULL_BAG, uint8 slot = NULL_SLOT);

        // Convert an item to a new item ID
        Item* ConvertItem(Item* item, uint32 newItemId);

        // Internal methods for storing items
        InventoryResult _CanTakeMoreSimilarItems(uint32 entry, uint32 count, Item* pItem, uint32* no_space_count = NULL) const;
        InventoryResult _CanStoreItem(uint8 bag, uint8 slot, ItemPosCountVec& dest, uint32 entry, uint32 count, Item* pItem = NULL, bool swap = false, uint32* no_space_count = NULL) const;

        // Apply equipment cooldown
        void ApplyEquipCooldown(Item* pItem);

        // Set the ammo
        void SetAmmo(uint32 item);

        // Remove the ammo
        void RemoveAmmo();

        // Get the ammo DPS
        float GetAmmoDPS() const
        {
            return m_ammoDPS;
        }

        // Check if the ammo is compatible
        bool CheckAmmoCompatibility(const ItemPrototype* ammo_proto) const;

        // Quickly equip an item
        void QuickEquipItem(uint16 pos, Item* pItem);

        // Visualize an item
        void VisualizeItem(uint8 slot, Item* pItem);

        // Set the visible item slot
        void SetVisibleItemSlot(uint8 slot, Item* pItem);

        // Bank an item
        Item* BankItem(ItemPosCountVec const& dest, Item* pItem, bool update)
        {
            return StoreItem(dest, pItem, update);
        }

        // Bank an item (overloaded)
        Item* BankItem(uint16 pos, Item* pItem, bool update);

        // Remove an item
        void RemoveItem(uint8 bag, uint8 slot, bool update);

        // Move an item from the inventory
        void MoveItemFromInventory(uint8 bag, uint8 slot, bool update);

        // Move an item to the inventory
        void MoveItemToInventory(ItemPosCountVec const& dest, Item* pItem, bool update, bool in_characterInventoryDB = false);

        // Remove item-dependent auras and casts
        void RemoveItemDependentAurasAndCasts(Item* pItem);

        // Destroy an item
        void DestroyItem(uint8 bag, uint8 slot, bool update);
        void DestroyItemCount(uint32 item, uint32 count, bool update, bool unequip_check = false, bool inBankAlso = false);
        void DestroyItemCount(Item* item, uint32& count, bool update);
        // Destroy all conjured items
        void DestroyConjuredItems(bool update);

        // Destroy items limited to a specific zone
        void DestroyZoneLimitedItem(bool update, uint32 new_zone);

        // Split an item stack into two stacks
        void SplitItem(uint16 src, uint16 dst, uint32 count);

        // Swap two items
        void SwapItem(uint16 src, uint16 dst);

        // Add an item to the buyback slot
        void AddItemToBuyBackSlot(Item* pItem);

        // Get an item from the buyback slot
        Item* GetItemFromBuyBackSlot(uint32 slot);

        // Remove an item from the buyback slot
        void RemoveItemFromBuyBackSlot(uint32 slot, bool del);

        // Take extended cost for an item
        void TakeExtendedCost(uint32 extendedCostId, uint32 count);

        // Get the maximum size of the keyring
        uint32 GetMaxKeyringSize() const
        {
            return KEYRING_SLOT_END - KEYRING_SLOT_START;
        }

        // Send an equipment error message
        void SendEquipError(InventoryResult msg, Item* pItem, Item* pItem2 = NULL, uint32 itemid = 0) const;

        // Send a buy error message
        void SendBuyError(BuyResult msg, Creature* pCreature, uint32 item, uint32 param);

        // Send a sell error message
        void SendSellError(SellResult msg, Creature* pCreature, ObjectGuid itemGuid, uint32 param);

        // Add a weapon proficiency
        void AddWeaponProficiency(uint32 newflag)
        {
            m_WeaponProficiency |= newflag;
        }

        // Add an armor proficiency
        void AddArmorProficiency(uint32 newflag)
        {
            m_ArmorProficiency |= newflag;
        }

        // Get the weapon proficiency
        uint32 GetWeaponProficiency() const
        {
            return m_WeaponProficiency;
        }

        // Get the armor proficiency
        uint32 GetArmorProficiency() const
        {
            return m_ArmorProficiency;
        }

        // Check if a two-handed weapon is used
        bool IsTwoHandUsed() const
        {
            Item* mainItem = GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
            return mainItem && mainItem->GetProto()->InventoryType == INVTYPE_2HWEAPON && !CanTitanGrip();
        }
        bool HasTwoHandWeaponInOneHand() const
        {
            Item* offItem = GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
            Item* mainItem = GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
            return offItem && ((mainItem && mainItem->GetProto()->InventoryType == INVTYPE_2HWEAPON) || offItem->GetProto()->InventoryType == INVTYPE_2HWEAPON);
        }

        // Send a new item notification
        void SendNewItem(Item* item, uint32 count, bool received, bool created, bool broadcast = false, bool showInChat = true);
        bool BuyItemFromVendorSlot(ObjectGuid vendorGuid, uint32 vendorslot, uint32 item, uint8 count, uint8 bag, uint8 slot);
        bool BuyItemFromVendor(ObjectGuid vendorGuid, uint32 item, uint8 count, uint8 bag, uint8 slot);

        // Get the reputation price discount
        float GetReputationPriceDiscount(Creature const* pCreature) const;

        // Get the trader
        Player* GetTrader() const { return m_trade ? m_trade->GetTrader() : NULL; }

        // Get the trade data
        TradeData* GetTradeData() const { return m_trade; }

        // Cancel the trade
        void TradeCancel(bool sendback);

        // Update enchantment time
        void UpdateEnchantTime(uint32 time);

        // Update item duration
        void UpdateItemDuration(uint32 time, bool realtimeonly = false);

        // Add enchantment durations
        void AddEnchantmentDurations(Item* item);

        // Remove enchantment durations
        void RemoveEnchantmentDurations(Item* item);

        // Remove all enchantments from a slot
        void RemoveAllEnchantments(EnchantmentSlot slot);

        // Add enchantment duration to an item
        void AddEnchantmentDuration(Item* item, EnchantmentSlot slot, uint32 duration);

        // Apply or remove an enchantment
        void ApplyEnchantment(Item* item, EnchantmentSlot slot, bool apply, bool apply_dur = true, bool ignore_condition = false);

        // Apply or remove all enchantments from an item
        void ApplyEnchantment(Item* item, bool apply);

        // Send enchantment durations to the client
        void SendEnchantmentDurations();
        void BuildEnchantmentsInfoData(WorldPacket* data);
        void AddItemDurations(Item* item);

        // Remove item durations
        void RemoveItemDurations(Item* item);

        // Send item durations to the client
        void SendItemDurations();

        // Load the player's corpse
        void LoadCorpse();

        // Load the player's pet
        void LoadPet();

        uint32 m_stableSlots; // Number of stable slots

        uint32 GetEquipGearScore(bool withBags = true, bool withBank = false);
        void ResetCachedGearScore() { m_cachedGS = 0; }
        typedef std::vector < uint32/*item level*/ > GearScoreVec;

        /*********************************************************/
        /***                    GOSSIP SYSTEM                  ***/
        /*********************************************************/

        // Prepare the gossip menu
        void PrepareGossipMenu(WorldObject* pSource, uint32 menuId = 0);

        // Send the prepared gossip menu
        void SendPreparedGossip(WorldObject* pSource);

        // Handle gossip selection
        void OnGossipSelect(WorldObject* pSource, uint32 gossipListId, uint32 menuId);

        // Get the gossip text ID for a menu
        uint32 GetGossipTextId(uint32 menuId, WorldObject* pSource);

        // Get the gossip text ID for a source
        uint32 GetGossipTextId(WorldObject* pSource);

        // Get the default gossip menu for a source
        uint32 GetDefaultGossipMenuForSource(WorldObject* pSource);

        /*********************************************************/
        /***                    QUEST SYSTEM                   ***/
        /*********************************************************/

        // Return player level when QuestLevel is dynamic (-1)
        uint32 GetQuestLevelForPlayer(Quest const* pQuest) const { return pQuest && (pQuest->GetQuestLevel() > 0) ? (uint32)pQuest->GetQuestLevel() : getLevel(); }

        // Prepare the quest menu
        void PrepareQuestMenu(ObjectGuid guid);

        // Send the prepared quest menu
        void SendPreparedQuest(ObjectGuid guid);

        // Check if a quest is active
        bool IsActiveQuest(uint32 quest_id) const; // can be taken or taken

        // Quest is taken and not yet rewarded
        // if completed_or_not = 0 (or any other value except 1 or 2) - returns true, if quest is taken and doesn't depend if quest is completed or not
        // if completed_or_not = 1 - returns true, if quest is taken but not completed
        // if completed_or_not = 2 - returns true, if quest is taken and already completed
        bool IsCurrentQuest(uint32 quest_id, uint8 completed_or_not = 0) const; // taken and not yet rewarded

        // Get the next quest in a chain
        Quest const* GetNextQuest(ObjectGuid guid, Quest const* pQuest);

        // Check if the player can see the start of a quest
        bool CanSeeStartQuest(Quest const* pQuest) const;

        // Check if the player can take a quest
        bool CanTakeQuest(Quest const* pQuest, bool msg) const;

        // Check if the player can add a quest
        bool CanAddQuest(Quest const* pQuest, bool msg) const;

        // Check if the player can complete a quest
        bool CanCompleteQuest(uint32 quest_id) const;

        // Check if the player can complete a repeatable quest
        bool CanCompleteRepeatableQuest(Quest const* pQuest) const;

        // Check if the player can reward a quest
        bool CanRewardQuest(Quest const* pQuest, bool msg) const;

        // Check if the player can reward a quest with a specific reward
        bool CanRewardQuest(Quest const* pQuest, uint32 reward, bool msg) const;

        // Add a quest to the player's quest log
        void AddQuest(Quest const* pQuest, Object* questGiver);

        // Complete a quest
        void CompleteQuest(uint32 quest_id, QuestStatus status = QUEST_STATUS_COMPLETE);

        // Mark a quest as incomplete
        void IncompleteQuest(uint32 quest_id);

        // Reward a quest
        void RewardQuest(Quest const* pQuest, uint32 reward, Object* questGiver, bool announce = true);

        // Fail a quest
        void FailQuest(uint32 quest_id);

        // Check if the player satisfies the skill requirements for a quest
        bool SatisfyQuestSkill(Quest const* qInfo, bool msg) const;

        // Check if the player satisfies the level requirements for a quest
        bool SatisfyQuestLevel(Quest const* qInfo, bool msg) const;

        // Check if the player satisfies the quest log requirements
        bool SatisfyQuestLog(bool msg) const;

        // Check if the player satisfies the previous quest requirements
        bool SatisfyQuestPreviousQuest(Quest const* qInfo, bool msg) const;

        // Check if the player satisfies the class requirements for a quest
        bool SatisfyQuestClass(Quest const* qInfo, bool msg) const;

        // Check if the player satisfies the race requirements for a quest
        bool SatisfyQuestRace(Quest const* qInfo, bool msg) const;

        // Check if the player satisfies the reputation requirements for a quest
        bool SatisfyQuestReputation(Quest const* qInfo, bool msg) const;

        // Check if the player satisfies the status requirements for a quest
        bool SatisfyQuestStatus(Quest const* qInfo, bool msg) const;

        // Check if the player satisfies the timed requirements for a quest
        bool SatisfyQuestTimed(Quest const* qInfo, bool msg) const;

        // Check if the player satisfies the exclusive group requirements for a quest
        bool SatisfyQuestExclusiveGroup(Quest const* qInfo, bool msg) const;

        // Check if the player satisfies the next chain requirements for a quest
        bool SatisfyQuestNextChain(Quest const* qInfo, bool msg) const;

        // Check if the player satisfies the previous chain requirements for a quest
        bool SatisfyQuestPrevChain(Quest const* qInfo, bool msg) const;

        // Check if the player satisfies the daily requirements for a quest
        bool SatisfyQuestDay(Quest const* qInfo, bool msg) const;
        bool SatisfyQuestWeek(Quest const* qInfo) const;
        bool SatisfyQuestMonth(Quest const* qInfo) const;
        bool CanGiveQuestSourceItemIfNeed(Quest const* pQuest, ItemPosCountVec* dest = NULL) const;

        // Give the quest source item if needed
        void GiveQuestSourceItemIfNeed(Quest const* pQuest);

        // Take the quest source item
        bool TakeQuestSourceItem(uint32 quest_id, bool msg);

        // Check if the player has the quest reward status
        bool GetQuestRewardStatus(uint32 quest_id) const;

        // Get the quest status
        QuestStatus GetQuestStatus(uint32 quest_id) const;

        // Set the quest status
        void SetQuestStatus(uint32 quest_id, QuestStatus status);

        // Set the daily quest status
        void SetDailyQuestStatus(uint32 quest_id);
        void SetWeeklyQuestStatus(uint32 quest_id);
        void SetMonthlyQuestStatus(uint32 quest_id);
        void ResetDailyQuestStatus();
        void ResetWeeklyQuestStatus();
        void ResetMonthlyQuestStatus();

        // Find the quest slot for a quest
        uint16 FindQuestSlot(uint32 quest_id) const;

        // Get the quest ID from a quest slot
        uint32 GetQuestSlotQuestId(uint16 slot) const { return GetUInt32Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_ID_OFFSET); }

        // Set the quest slot
        void SetQuestSlot(uint16 slot, uint32 quest_id, uint32 timer = 0)
        {
            SetUInt32Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_ID_OFFSET, quest_id);
            SetUInt32Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_STATE_OFFSET, 0);
            SetUInt32Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_COUNTS_OFFSET, 0);
            SetUInt32Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_COUNTS_OFFSET + 1, 0);
            SetUInt32Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_TIME_OFFSET, timer);
        }
        void SetQuestSlotCounter(uint16 slot, uint8 counter, uint16 count)
        {
            uint64 val = GetUInt64Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_COUNTS_OFFSET);
            val &= ~((uint64)0xFFFF << (counter * 16));
            val |= ((uint64)count << (counter * 16));
            SetUInt64Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_COUNTS_OFFSET, val);
        }

        // Set the quest slot state
        void SetQuestSlotState(uint16 slot, uint32 state) { SetFlag(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_STATE_OFFSET, state); }

        // Remove the quest slot state
        void RemoveQuestSlotState(uint16 slot, uint32 state) { RemoveFlag(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_STATE_OFFSET, state); }

        // Set the quest slot timer
        void SetQuestSlotTimer(uint16 slot, uint32 timer) { SetUInt32Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_TIME_OFFSET, timer); }

        // Swap two quest slots
        void SwapQuestSlot(uint16 slot1, uint16 slot2)
        {
            for (int i = 0; i < MAX_QUEST_OFFSET; ++i)
            {
                uint32 temp1 = GetUInt32Value(PLAYER_QUEST_LOG_1_1 + MAX_QUEST_OFFSET * slot1 + i);
                uint32 temp2 = GetUInt32Value(PLAYER_QUEST_LOG_1_1 + MAX_QUEST_OFFSET * slot2 + i);

                SetUInt32Value(PLAYER_QUEST_LOG_1_1 + MAX_QUEST_OFFSET * slot1 + i, temp2);
                SetUInt32Value(PLAYER_QUEST_LOG_1_1 + MAX_QUEST_OFFSET * slot2 + i, temp1);
            }
        }

        // Get the current count for a required kill or cast
        uint32 GetReqKillOrCastCurrentCount(uint32 quest_id, int32 entry);

        // Mark an area as explored or an event as happened for a quest
        void AreaExploredOrEventHappens(uint32 questId);

        // Mark a group event as happened for a quest
        void GroupEventHappens(uint32 questId, WorldObject const* pEventObject);

        // Check if an item added satisfies a quest requirement
        void ItemAddedQuestCheck(uint32 entry, uint32 count);

        // Check if an item removed satisfies a quest requirement
        void ItemRemovedQuestCheck(uint32 entry, uint32 count);

        // Mark a monster as killed for a quest
        void KilledMonster(CreatureInfo const* cInfo, ObjectGuid guid);

        // Mark a monster as killed for a quest (with credit)
        void KilledMonsterCredit(uint32 entry, ObjectGuid guid = ObjectGuid());

        // Mark a creature or game object as casted for a quest
        void CastedCreatureOrGO(uint32 entry, ObjectGuid guid, uint32 spell_id, bool original_caster = true);

        // Mark a creature as talked to for a quest
        void TalkedToCreature(uint32 entry, ObjectGuid guid);

        // Handle money change for a quest
        void MoneyChanged(uint32 value);

        // Handle reputation change for a quest
        void ReputationChanged(FactionEntry const* factionEntry);

        // Check if the player has a quest for an item
        bool HasQuestForItem(uint32 itemid) const;

        // Check if the player has a quest for a game object
        bool HasQuestForGO(int32 GOId) const;

        // Update the world objects for quests
        void UpdateForQuestWorldObjects();

        // Check if the player can share a quest
        bool CanShareQuest(uint32 quest_id) const;

        // Send a quest complete event
        void SendQuestCompleteEvent(uint32 quest_id);

        // Send a quest reward notification
        void SendQuestReward(Quest const* pQuest, uint32 XP);
        void SendQuestFailed(uint32 quest_id, InventoryResult reason = EQUIP_ERR_OK);
        void SendQuestTimerFailed(uint32 quest_id);

        // Send a response for the ability to take a quest
        void SendCanTakeQuestResponse(uint32 msg) const;

        // Send a quest confirm accept notification
        void SendQuestConfirmAccept(Quest const* pQuest, Player* pReceiver);

        // Send a response for pushing a quest to the party
        void SendPushToPartyResponse(Player* pPlayer, uint32 msg);

        // Send a quest update for adding an item
        void SendQuestUpdateAddItem(Quest const* pQuest, uint32 item_idx, uint32 count);

        // Send a quest update for adding a creature or game object
        void SendQuestUpdateAddCreatureOrGo(Quest const* pQuest, ObjectGuid guid, uint32 creatureOrGO_idx, uint32 count);

        // Get the divider GUID
        ObjectGuid GetDividerGuid() const { return m_dividerGuid; }

        // Set the divider GUID
        void SetDividerGuid(ObjectGuid guid) { m_dividerGuid = guid; }

        // Clear the divider GUID
        void ClearDividerGuid() { m_dividerGuid.Clear(); }

        // Get the in-game time
        uint32 GetInGameTime() { return m_ingametime; }

        // Set the in-game time
        void SetInGameTime(uint32 time) { m_ingametime = time; }

        // Add a timed quest
        void AddTimedQuest(uint32 quest_id) { m_timedquests.insert(quest_id); }

        // Remove a timed quest
        void RemoveTimedQuest(uint32 quest_id) { m_timedquests.erase(quest_id); }

        /// Return collision height sent to client
        float GetCollisionHeight(bool mounted) const;

        /*********************************************************/
        /***                   LOAD SYSTEM                     ***/
        /*********************************************************/

        // Load the player from the database
        bool LoadFromDB(ObjectGuid guid, SqlQueryHolder* holder);

        // Get the zone ID from the database
        static uint32 GetZoneIdFromDB(ObjectGuid guid);

        // Get the level from the database
        static uint32 GetLevelFromDB(ObjectGuid guid);

        // Load the position from the database
        static bool LoadPositionFromDB(ObjectGuid guid, uint32& mapid, float& x, float& y, float& z, float& o, bool& in_flight);

        /*********************************************************/
        /***                   SAVE SYSTEM                     ***/
        /*********************************************************/

        // Save the player to the database
        void SaveToDB();

        // Save the inventory and gold to the database
        void SaveInventoryAndGoldToDB(); // fast save function for item/money cheating preventing

        // Save the gold to the database
        void SaveGoldToDB();

        // Set a uint32 value in an array
        static void SetUInt32ValueInArray(Tokens& data, uint16 index, uint32 value);
        static void SetFloatValueInArray(Tokens& data, uint16 index, float value);
        static void Customize(ObjectGuid guid, uint8 gender, uint8 skin, uint8 face, uint8 hairStyle, uint8 hairColor, uint8 facialHair);
        static void SavePositionInDB(ObjectGuid guid, uint32 mapid, float x, float y, float z, float o, uint32 zone);

        // Delete a player from the database
        static void DeleteFromDB(ObjectGuid playerguid, uint32 accountId, bool updateRealmChars = true, bool deleteFinally = false);

        // Delete old characters from the database
        static void DeleteOldCharacters();

        // Delete old characters from the database, keeping characters for a specified number of days
        static void DeleteOldCharacters(uint32 keepDays);

        bool m_mailsUpdated; // Indicates if mails have been updated

        // Send a pet tame failure message
        void SendPetTameFailure(PetTameFailureReason reason);

        // Set the player's bind point
        void SetBindPoint(ObjectGuid guid);

        // Send a talent wipe confirmation message
        void SendTalentWipeConfirm(ObjectGuid guid);

        // Reward rage to the player
        void RewardRage(uint32 damage, uint32 weaponSpeedHitFactor, bool attacker);

        // Send a pet skill wipe confirmation message
        void SendPetSkillWipeConfirm();
        void CalcRage(uint32 damage, bool attacker);
        void RegenerateAll(uint32 diff = REGEN_TIME_FULL);
        void Regenerate(Powers power, uint32 diff);
        void RegenerateHealth(uint32 diff);
        void setRegenTimer(uint32 time)
        {
            m_regenTimer = time;
        }

        // Set the weapon change timer
        void setWeaponChangeTimer(uint32 time)
        {
            m_weaponChangeTimer = time;
        }

        // Get the player's money
        uint32 GetMoney() const
        {
            return GetUInt32Value(PLAYER_FIELD_COINAGE);
        }

        // Modify the player's money
        void ModifyMoney(int32 d);

        // Set the player's money
        void SetMoney(uint32 value)
        {
            SetUInt32Value(PLAYER_FIELD_COINAGE, value);
            MoneyChanged(value);
            UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_GOLD_VALUE_OWNED);
        }

        // Get the player's quest status map
        QuestStatusMap& getQuestStatusMap()
        {
            return mQuestStatus;
        };

        // Get the player's current selection GUID
        ObjectGuid const& GetSelectionGuid() const { return m_curSelectionGuid; }

        // Set the player's current selection GUID
        void SetSelectionGuid(ObjectGuid guid) { m_curSelectionGuid = guid; SetTargetGuid(guid); }

        // Get the player's combo points
        uint8 GetComboPoints() const { return m_comboPoints; }

        // Get the player's combo target GUID
        ObjectGuid const& GetComboTargetGuid() const { return m_comboTargetGuid; }

        // Add combo points to the player
        void AddComboPoints(Unit* target, int8 count);

        // Clear the player's combo points
        void ClearComboPoints();

        // Send the player's combo points to the client
        void SendComboPoints();

        // Send a mail result message
        void SendMailResult(uint32 mailId, MailResponseType mailAction, MailResponseResult mailError, uint32 equipError = 0, uint32 item_guid = 0, uint32 item_count = 0);

        // Send a new mail notification
        void SendNewMail();

        // Update the next mail delivery time and unread mails
        void UpdateNextMailTimeAndUnreads();

        // Add a new mail delivery time
        void AddNewMailDeliverTime(time_t deliver_time);

        // Remove a mail by ID
        void RemoveMail(uint32 id);

        // Add a mail to the player's mail list
        void AddMail(Mail* mail)
        {
            m_mail.push_front(mail);   // for call from WorldSession::SendMailTo
        }

        // Get the size of the player's mail list
        uint32 GetMailSize()
        {
            return m_mail.size();
        }

        // Get a mail by ID
        Mail* GetMail(uint32 id);

        // Get the beginning iterator of the player's mail list
        PlayerMails::iterator GetMailBegin()
        {
            return m_mail.begin();
        }

        // Get the end iterator of the player's mail list
        PlayerMails::iterator GetMailEnd()
        {
            return m_mail.end();
        }

        /*********************************************************/
        /*** MAILED ITEMS SYSTEM ***/
        /*********************************************************/

        uint8 unReadMails; // Number of unread mails
        time_t m_nextMailDelivereTime; // Time of the next mail delivery

        typedef UNORDERED_MAP<uint32, Item*> ItemMap;

        ItemMap mMitems; // Map of mailed items

        // Get a mailed item by ID
        Item* GetMItem(uint32 id)
        {
            ItemMap::const_iterator itr = mMitems.find(id);
            return itr != mMitems.end() ? itr->second : NULL;
        }

        // Add a mailed item
        void AddMItem(Item* it)
        {
            MANGOS_ASSERT(it);
            // ASSERT deleted, because items can be added before loading
            mMitems[it->GetGUIDLow()] = it;
        }

        // Remove a mailed item by ID
        bool RemoveMItem(uint32 id)
        {
            return mMitems.erase(id) ? true : false;
        }

        // Initialize pet spells
        void PetSpellInitialize();
        void SendPetGUIDs();
        void CharmSpellInitialize();

        // Initialize possess spells
        void PossessSpellInitialize();

        // Remove the pet action bar
        void RemovePetActionBar();

        // Check if the player has a specific spell
        bool HasSpell(uint32 spell) const override;

        // Check if the player has an active spell
        bool HasActiveSpell(uint32 spell) const; // show in spellbook

        // Get the state of a trainer spell
        TrainerSpellState GetTrainerSpellState(TrainerSpell const* trainer_spell, uint32 reqLevel) const;

        // Check if a spell fits the player's class and race
        bool IsSpellFitByClassAndRace(uint32 spell_id, uint32* pReqlevel = NULL) const;

        // Check if a passive-like spell needs to be cast when learned
        bool IsNeedCastPassiveLikeSpellAtLearn(SpellEntry const* spellInfo) const;

        // Check if the player is immune to a spell effect
        bool IsImmuneToSpellEffect(SpellEntry const* spellInfo, SpellEffectIndex index, bool castOnSelf) const override;

        void SendProficiency(ItemClass itemClass, uint32 itemSubclassMask);

        // Send initial spells to the client
        void SendInitialSpells();

        // Add a spell to the player
        bool addSpell(uint32 spell_id, bool active, bool learning, bool dependent, bool disabled);

        // Learn a spell
        void learnSpell(uint32 spell_id, bool dependent);

        // Remove a spell from the player
        void removeSpell(uint32 spell_id, bool disabled = false, bool learn_low_rank = true, bool sendUpdate = true);

        // Reset the player's spells
        void resetSpells();

        // Learn the player's default spells
        void learnDefaultSpells();

        // Learn quest-rewarded spells
        void learnQuestRewardedSpells();

        // Learn quest-rewarded spells for a specific quest
        void learnQuestRewardedSpells(Quest const* quest);

        // Learn a high-rank spell
        void learnSpellHighRank(uint32 spellid);

        // Get the player's free talent points
        uint32 GetFreeTalentPoints() const
        {
            return GetUInt32Value(PLAYER_CHARACTER_POINTS1);
        }

        // Set the player's free talent points
        void SetFreeTalentPoints(uint32 points);

        // Update the player's free talent points
        void UpdateFreeTalentPoints(bool resetIfNeed = true);
        bool resetTalents(bool no_cost = false, bool all_specs = false);
        uint32 resetTalentsCost() const;
        void InitTalentForLevel();
        void BuildPlayerTalentsInfoData(WorldPacket* data);
        void BuildPetTalentsInfoData(WorldPacket* data);
        void SendTalentsInfoData(bool pet);
        void LearnTalent(uint32 talentId, uint32 talentRank);
        void LearnPetTalent(ObjectGuid petGuid, uint32 talentId, uint32 talentRank);
        bool HasTalent(uint32 spell_id, uint8 spec) const;

        uint32 CalculateTalentsPoints() const;

        // Dual Spec
        uint8 GetActiveSpec() { return m_activeSpec; }
        void SetActiveSpec(uint8 spec) { m_activeSpec = spec; }
        uint8 GetSpecsCount() { return m_specsCount; }
        void SetSpecsCount(uint8 count) { m_specsCount = count; }
        void ActivateSpec(uint8 specNum);
        void UpdateSpecCount(uint8 count);

        void InitGlyphsForLevel();
        void SetGlyphSlot(uint8 slot, uint32 slottype) { SetUInt32Value(PLAYER_FIELD_GLYPH_SLOTS_1 + slot, slottype); }
        uint32 GetGlyphSlot(uint8 slot) { return GetUInt32Value(PLAYER_FIELD_GLYPH_SLOTS_1 + slot); }
        void SetGlyph(uint8 slot, uint32 glyph) { m_glyphs[m_activeSpec][slot].SetId(glyph); }
        uint32 GetGlyph(uint8 slot) { return m_glyphs[m_activeSpec][slot].GetId(); }
        void ApplyGlyph(uint8 slot, bool apply);
        void ApplyGlyphs(bool apply);

        uint32 GetFreePrimaryProfessionPoints() const
        {
            return GetUInt32Value(PLAYER_CHARACTER_POINTS2);
        }

        // Set the player's free primary professions
        void SetFreePrimaryProfessions(uint16 profs)
        {
            SetUInt32Value(PLAYER_CHARACTER_POINTS2, profs);
        }

        // Initialize the player's primary professions
        void InitPrimaryProfessions();

        // Get the player's spell map
        PlayerSpellMap const& GetSpellMap() const
        {
            return m_spells;
        }

        // Get the player's spell map (non-const)
        PlayerSpellMap& GetSpellMap()
        {
            return m_spells;
        }

        // Get the player's spell cooldown map
        SpellCooldowns const& GetSpellCooldownMap() const
        {
            return m_spellCooldowns;
        }

        PlayerTalent const* GetKnownTalentById(int32 talentId) const;
        SpellEntry const* GetKnownTalentRankById(int32 talentId) const;

        void AddSpellMod(Aura* aura, bool apply);
        template <class T> T ApplySpellMod(uint32 spellId, SpellModOp op, T& basevalue);

        static uint32 const infinityCooldownDelay = MONTH; // used for set "infinity cooldowns" for spells and check
        static uint32 const infinityCooldownDelayCheck = MONTH / 2;

        // Check if the player has a spell cooldown
        bool HasSpellCooldown(uint32 spell_id) const
        {
            SpellCooldowns::const_iterator itr = m_spellCooldowns.find(spell_id);
            return itr != m_spellCooldowns.end() && itr->second.end > time(NULL);
        }

        // Get the delay for a spell cooldown
        time_t GetSpellCooldownDelay(uint32 spell_id) const
        {
            SpellCooldowns::const_iterator itr = m_spellCooldowns.find(spell_id);
            time_t t = time(NULL);
            return itr != m_spellCooldowns.end() && itr->second.end > t ? itr->second.end - t : 0;
        }

        // Add spell and category cooldowns
        void AddSpellAndCategoryCooldowns(SpellEntry const* spellInfo, uint32 itemId, Spell* spell = NULL, bool infinityCooldown = false);

        // Add a spell cooldown
        void AddSpellCooldown(uint32 spell_id, uint32 itemid, time_t end_time);

        // Send a cooldown event to the client
        void SendCooldownEvent(SpellEntry const* spellInfo, uint32 itemId = 0, Spell* spell = NULL);

        // Prohibit a spell school for a specific duration
        void ProhibitSpellSchool(SpellSchoolMask idSchoolMask, uint32 unTimeMs) override;

        // Remove a spell cooldown
        void RemoveSpellCooldown(uint32 spell_id, bool update = false);

        // Remove a spell category cooldown
        void RemoveSpellCategoryCooldown(uint32 cat, bool update = false);

        // Send a clear cooldown message to the client
        void SendClearCooldown(uint32 spell_id, Unit* target);

        // Get the global cooldown manager
        GlobalCooldownMgr& GetGlobalCooldownMgr()
        {
            return m_GlobalCooldownMgr;
        }

        // Remove all arena spell cooldowns
        void RemoveArenaSpellCooldowns();

        // Remove all spell cooldowns
        void RemoveAllSpellCooldown();

        // Load spell cooldowns from the database
        void _LoadSpellCooldowns(QueryResult* result);

        // Save spell cooldowns to the database
        void _SaveSpellCooldowns();
        void SetLastPotionId(uint32 item_id) { m_lastPotionId = item_id; }
        uint32 GetLastPotionId() { return m_lastPotionId; }
        void UpdatePotionCooldown(Spell* spell = NULL);

        // Set resurrect request data
        void setResurrectRequestData(ObjectGuid guid, uint32 mapId, float X, float Y, float Z, uint32 health, uint32 mana)
        {
            m_resurrectGuid = guid;
            m_resurrectMap = mapId;
            m_resurrectX = X;
            m_resurrectY = Y;
            m_resurrectZ = Z;
            m_resurrectHealth = health;
            m_resurrectMana = mana;
        }

        // Clear resurrect request data
        void clearResurrectRequestData() { setResurrectRequestData(ObjectGuid(), 0, 0.0f, 0.0f, 0.0f, 0, 0); }

        // Check if resurrect is requested by a specific GUID
        bool isRessurectRequestedBy(ObjectGuid guid) const { return m_resurrectGuid == guid; }

        // Check if resurrect is requested
        bool isRessurectRequested() const { return !m_resurrectGuid.IsEmpty(); }

        // Resurrect using request data
        void ResurectUsingRequestData();

        // Get the cinematic ID
        uint32 getCinematic()
        {
            return m_cinematic;
        }

        // Set the cinematic ID
        void setCinematic(uint32 cine)
        {
            m_cinematic = cine;
        }

        static bool IsActionButtonDataValid(uint8 button, uint32 action, uint8 type, Player* player, bool msg = true);
        ActionButton* addActionButton(uint8 spec, uint8 button, uint32 action, uint8 type);
        void removeActionButton(uint8 spec, uint8 button);
        void SendInitialActionButtons() const;
        void SendLockActionButtons() const;
        ActionButton const* GetActionButton(uint8 button);

        PvPInfo pvpInfo;
        // Update PvP state
        void UpdatePvP(bool state, bool ovrride = false);
        void UpdateZone(uint32 newZone, uint32 newArea);

        // Update the player's area
        void UpdateArea(uint32 newArea);

        // Get the cached zone ID
        uint32 GetCachedZoneId() const
        {
            return m_zoneUpdateId;
        }

        // Update zone-dependent auras
        void UpdateZoneDependentAuras();

        // Update area-dependent auras
        void UpdateAreaDependentAuras(); // subzones

        // Update zone-dependent pets
        void UpdateZoneDependentPets();

        // Update AFK report
        void UpdateAfkReport(time_t currTime);

        // Update PvP flag
        void UpdatePvPFlag(time_t currTime);

        // Update contested PvP state
        void UpdateContestedPvP(uint32 currTime);

        // Set the contested PvP timer
        void SetContestedPvPTimer(uint32 newTime)
        {
            m_contestedPvPTimer = newTime;
        }

        // Reset contested PvP state
        void ResetContestedPvP()
        {
            clearUnitState(UNIT_STAT_ATTACK_PLAYER);
            RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_CONTESTED_PVP);
            m_contestedPvPTimer = 0;
        }

        // Check if the player is in a duel with another player
            /** todo: -maybe move UpdateDuelFlag+DuelComplete to independent DuelHandler.. **/
        DuelInfo* duel;
        bool IsInDuelWith(Player const* player) const
        {
            return duel && duel->opponent == player && duel->startTime != 0;
        }

        // Update duel flag
        void UpdateDuelFlag(time_t currTime);

        // Check duel distance
        void CheckDuelDistance(time_t currTime);

        // Complete the duel
        void DuelComplete(DuelCompleteType type);

        // Send duel countdown
        void SendDuelCountdown(uint32 counter);

        // Check if the player is visible for another player in the group
        bool IsGroupVisibleFor(Player* p) const;

        // Check if the player is in the same group with another player
        bool IsInSameGroupWith(Player const* p) const;

        // Check if the player is in the same raid with another player
        bool IsInSameRaidWith(Player const* p) const
        {
            return p == this || (GetGroup() != NULL && GetGroup() == p->GetGroup());
        }

        // Uninvite the player from the group
        void UninviteFromGroup();
        static void RemoveFromGroup(Group* group, ObjectGuid guid, ObjectGuid kicker, std::string reason);
        void RemoveFromGroup() { RemoveFromGroup(GetGroup(), GetObjectGuid(), GetObjectGuid(), ""); }
        void SendUpdateToOutOfRangeGroupMembers();
        void SetAllowLowLevelRaid(bool allow) { ApplyModFlag(PLAYER_FLAGS, PLAYER_FLAGS_ENABLE_LOW_LEVEL_RAID, allow); }
        bool GetAllowLowLevelRaid() const { return HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_ENABLE_LOW_LEVEL_RAID); }

        // Set the player's guild ID
        void SetInGuild(uint32 GuildId)
        {
            SetUInt32Value(PLAYER_GUILDID, GuildId);
        }

        // Set the player's guild rank
        void SetRank(uint32 rankId)
        {
            SetUInt32Value(PLAYER_GUILDRANK, rankId);
        }

        // Set the guild ID the player is invited to
        void SetGuildIdInvited(uint32 GuildId)
        {
            m_GuildIdInvited = GuildId;
        }

        // Get the player's guild ID
        uint32 GetGuildId()
        {
            return GetUInt32Value(PLAYER_GUILDID);
        }

        // Get the player's guild ID from the database
        static uint32 GetGuildIdFromDB(ObjectGuid guid);

        // Get the player's guild rank
        uint32 GetRank()
        {
            return GetUInt32Value(PLAYER_GUILDRANK);
        }

        // Get the player's guild rank from the database
        static uint32 GetRankFromDB(ObjectGuid guid);

        // Get the guild ID the player is invited to
        int GetGuildIdInvited()
        {
            return m_GuildIdInvited;
        }

        // Remove petitions and signs for the player
        static void RemovePetitionsAndSigns(ObjectGuid guid, uint32 type);

        // Arena Team
        void SetInArenaTeam(uint32 ArenaTeamId, uint8 slot, ArenaType type)
        {
            SetArenaTeamInfoField(slot, ARENA_TEAM_ID, ArenaTeamId);
            SetArenaTeamInfoField(slot, ARENA_TEAM_TYPE, type);
        }

        // Set the player's arena team info field
        void SetArenaTeamInfoField(uint8 slot, ArenaTeamInfoType type, uint32 value)
        {
            SetUInt32Value(PLAYER_FIELD_ARENA_TEAM_INFO_1_1 + (slot * ARENA_TEAM_END) + type, value);
        }

        // Get the player's arena team ID
        uint32 GetArenaTeamId(uint8 slot) { return GetUInt32Value(PLAYER_FIELD_ARENA_TEAM_INFO_1_1 + (slot * ARENA_TEAM_END) + ARENA_TEAM_ID); }

        // Get the player's arena personal rating
        uint32 GetArenaPersonalRating(uint8 slot) { return GetUInt32Value(PLAYER_FIELD_ARENA_TEAM_INFO_1_1 + (slot * ARENA_TEAM_END) + ARENA_TEAM_PERSONAL_RATING); }

        // Get the player's arena team ID from the database
        static uint32 GetArenaTeamIdFromDB(ObjectGuid guid, ArenaType type);

        // Set the arena team ID the player is invited to
        void SetArenaTeamIdInvited(uint32 ArenaTeamId) { m_ArenaTeamIdInvited = ArenaTeamId; }

        // Get the arena team ID the player is invited to
        uint32 GetArenaTeamIdInvited() { return m_ArenaTeamIdInvited; }

        // Leave all arena teams
        static void LeaveAllArenaTeams(ObjectGuid guid);

        Difficulty GetDifficulty(bool isRaid) const { return isRaid ? m_raidDifficulty : m_dungeonDifficulty; }
        Difficulty GetDungeonDifficulty() const { return m_dungeonDifficulty; }
        Difficulty GetRaidDifficulty() const { return m_raidDifficulty; }
        void SetDungeonDifficulty(Difficulty dungeon_difficulty) { m_dungeonDifficulty = dungeon_difficulty; }
        void SetRaidDifficulty(Difficulty raid_difficulty) { m_raidDifficulty = raid_difficulty; }


        // Update the player's skill
        bool UpdateSkill(uint32 skill_id, uint32 step);

        // Update the player's skill proficiency
        bool UpdateSkillPro(uint16 SkillId, int32 Chance, uint32 step);

        // Update the player's crafting skill
        bool UpdateCraftSkill(uint32 spellid);

        // Update the player's gathering skill
        bool UpdateGatherSkill(uint32 SkillId, uint32 SkillValue, uint32 RedLevel, uint32 Multiplicator = 1);

        // Update the player's fishing skill
        bool UpdateFishingSkill();

        // Get the player's base defense skill value
        uint32 GetBaseDefenseSkillValue() const
        {
            return GetBaseSkillValue(SKILL_DEFENSE);
        }

        // Get the player's base weapon skill value
        uint32 GetBaseWeaponSkillValue(WeaponAttackType attType) const;

        uint32 GetSpellByProto(ItemPrototype* proto);

        float GetHealthBonusFromStamina();

        // Get the mana bonus from intellect
        float GetManaBonusFromIntellect();

        // Update the player's stats
        bool UpdateStats(Stats stat) override;

        // Update all of the player's stats
        bool UpdateAllStats() override;

        // Update the player's resistances
        void UpdateResistances(uint32 school) override;

        // Update the player's armor
        void UpdateArmor() override;

        // Update the player's maximum health
        void UpdateMaxHealth() override;

        // Update the player's maximum power
        void UpdateMaxPower(Powers power) override;
        void ApplyFeralAPBonus(int32 amount, bool apply);
        void UpdateAttackPowerAndDamage(bool ranged = false) override;

        // Update the player's shield block value
        void UpdateShieldBlockValue();

        // Update the player's physical damage
        void UpdateDamagePhysical(WeaponAttackType attType) override;
        void ApplySpellPowerBonus(int32 amount, bool apply);
        void UpdateSpellDamageAndHealingBonus();

        // Apply a rating modifier
        void ApplyRatingMod(CombatRating cr, int32 value, bool apply);

        // Update a rating
        void UpdateRating(CombatRating cr);

        // Update all ratings
        void UpdateAllRatings();

        // Calculate the minimum and maximum damage
        void CalculateMinMaxDamage(WeaponAttackType attType, bool normalized, float& min_damage, float& max_damage);

        // Update defense bonuses modifier
        void UpdateDefenseBonusesMod();

        // Get melee critical chance from agility
        float GetMeleeCritFromAgility();
        void GetDodgeFromAgility(float& diminishing, float& nondiminishing);
        float GetSpellCritFromIntellect();

        // Get health regeneration per spirit
        float OCTRegenHPPerSpirit();

        // Get mana regeneration per spirit
        float OCTRegenMPPerSpirit();

        // Get rating multiplier
        float GetRatingMultiplier(CombatRating cr) const;

        // Get rating bonus value
        float GetRatingBonusValue(CombatRating cr) const;
        uint32 GetBaseSpellPowerBonus() { return m_baseSpellPower; }

        // Get expertise dodge or parry reduction
        float GetExpertiseDodgeOrParryReduction(WeaponAttackType attType) const;

        // Update block percentage
        void UpdateBlockPercentage();

        // Update critical percentage
        void UpdateCritPercentage(WeaponAttackType attType);

        // Update all critical percentages
        void UpdateAllCritPercentages();

        // Update parry percentage
        void UpdateParryPercentage();

        // Update dodge percentage
        void UpdateDodgePercentage();

        // Update melee hit chances
        void UpdateMeleeHitChances();

        // Update ranged hit chances
        void UpdateRangedHitChances();

        // Update spell hit chances
        void UpdateSpellHitChances();

        // Update all spell critical chances
        void UpdateAllSpellCritChances();

        // Update spell critical chance for a specific school
        void UpdateSpellCritChance(uint32 school);

        // Update expertise
        void UpdateExpertise(WeaponAttackType attType);
        void UpdateArmorPenetration();
        void ApplyManaRegenBonus(int32 amount, bool apply);
        void UpdateManaRegen();

        // Get the GUID of the loot
        ObjectGuid const& GetLootGuid() const
        {
            return m_lootGuid;
        }

        // Set the GUID of the loot
        void SetLootGuid(ObjectGuid const& guid)
        {
            m_lootGuid = guid;
        }

        // Remove the insignia of the player
        void RemovedInsignia(Player* looterPlr);

        // Get the player's session
        WorldSession* GetSession() const
        {
            return m_session;
        }

        // Set the player's session
        void SetSession(WorldSession* s)
        {
            m_session = s;
        }

        // Build the create update block for the player
        void BuildCreateUpdateBlockForPlayer(UpdateData* data, Player* target) const override;
        void DestroyForPlayer(Player* target, bool anim = false) const override;
        void SendLogXPGain(uint32 GivenXP, Unit* victim, uint32 RestXP);

        // Get the last swing error message
        uint8 LastSwingErrorMsg() const
        {
            return m_swingErrorMsg;
        }

        // Set the swing error message
        void SwingErrorMsg(uint8 val)
        {
            m_swingErrorMsg = val;
        }

        // Notifiers for various attack swing errors
        void SendAttackSwingCantAttack();
        void SendAttackSwingCancelAttack();
        void SendAttackSwingDeadTarget();
        void SendAttackSwingNotInRange();
        void SendAttackSwingBadFacingAttack();
        void SendAutoRepeatCancel(Unit* target);
        void SendExplorationExperience(uint32 Area, uint32 Experience);

        // Send dungeon difficulty
        void SendDungeonDifficulty(bool IsInGroup);
        void SendRaidDifficulty(bool IsInGroup);
        void ResetInstances(InstanceResetMethod method, bool isRaid);
        void SendResetInstanceSuccess(uint32 MapId);

        // Send reset instance failed
        void SendResetInstanceFailed(uint32 reason, uint32 MapId);

        // Send reset failed notification
        void SendResetFailedNotify(uint32 mapid);

        // Set the player's position
        bool SetPosition(float x, float y, float z, float orientation, bool teleport = false);

        // Update the player's underwater state
        void UpdateUnderwaterState(Map* m, float x, float y, float z);

        // Send a message to the set of players
        void SendMessageToSet(WorldPacket* data, bool self) const override;

        // Send a message to the set of players within a range
        void SendMessageToSetInRange(WorldPacket* data, float dist, bool self) const override;

        // Send a message to the set of players within a range, with an option for own team only
        void SendMessageToSetInRange(WorldPacket* data, float dist, bool self, bool own_team_only) const;

        // Get the player's corpse
        Corpse* GetCorpse() const;

        // Spawn the player's corpse bones
        void SpawnCorpseBones();

        // Create a corpse for the player
        Corpse* CreateCorpse();

        // Kill the player
        void KillPlayer();

        // Get the resurrection spell ID
        uint32 GetResurrectionSpellId();

        // Resurrect the player
        void ResurrectPlayer(float restore_percent, bool applySickness = false);

        // Build the player repopulation
        void BuildPlayerRepop();

        // Repopulate the player at the graveyard
        void RepopAtGraveyard();

        // Handle durability loss for all items
        void DurabilityLossAll(double percent, bool inventory);

        // Handle durability loss for a specific item
        void DurabilityLoss(Item* item, double percent);

        // Handle durability points loss for all items
        void DurabilityPointsLossAll(int32 points, bool inventory);

        // Handle durability points loss for a specific item
        void DurabilityPointsLoss(Item* item, int32 points);

        // Handle durability point loss for a specific equipment slot
        void DurabilityPointLossForEquipSlot(EquipmentSlots slot);

        // Repair all items' durability
        uint32 DurabilityRepairAll(bool cost, float discountMod, bool guildBank);

        // Repair a specific item's durability
        uint32 DurabilityRepair(uint16 pos, bool cost, float discountMod, bool guildBank);

        // Update mirror timers
        void UpdateMirrorTimers();

        // Stop all mirror timers
        void StopMirrorTimers()
        {
            StopMirrorTimer(FATIGUE_TIMER);
            StopMirrorTimer(BREATH_TIMER);
            StopMirrorTimer(FIRE_TIMER);
        }

        // Set levitate state
        void SetLevitate(bool enable) override;

        // Set can fly state
        void SetCanFly(bool enable) override;

        // Set feather fall state
        void SetFeatherFall(bool enable) override;

        // Set hover state
        void SetHover(bool enable) override;

        // Set root state
        void SetRoot(bool enable) override;

        // Set water walk state
        void SetWaterWalk(bool enable) override;

        // Handle joining a channel
        void JoinedChannel(Channel* c);

        // Handle leaving a channel
        void LeftChannel(Channel* c);

        // Cleanup channels
        void CleanupChannels();

        // Update local channels based on the new zone
        void UpdateLocalChannels(uint32 newZone);

        // Leave the Looking For Group (LFG) channel
        void LeaveLFGChannel();

        // Update the player's defense
        void UpdateDefense();

        // Update the player's weapon skill
        void UpdateWeaponSkill(WeaponAttackType attType);

        // Update the player's combat skills
        void UpdateCombatSkills(Unit* pVictim, WeaponAttackType attType, bool defence);

        // Set the player's skill
        void SetSkill(uint16 id, uint16 currVal, uint16 maxVal, uint16 step = 0);

        // Get the maximum skill value
        uint16 GetMaxSkillValue(uint32 skill) const;

        // Get the pure maximum skill value
        uint16 GetPureMaxSkillValue(uint32 skill) const;

        // Get the skill value
        uint16 GetSkillValue(uint32 skill) const;

        // Get the base skill value
        uint16 GetBaseSkillValue(uint32 skill) const;

        // Get the pure skill value
        uint16 GetPureSkillValue(uint32 skill) const;

        // Get the permanent bonus value for a skill
        int16 GetSkillPermBonusValue(uint32 skill) const;

        // Get the temporary bonus value for a skill
        int16 GetSkillTempBonusValue(uint32 skill) const;

        // Check if the player has a specific skill
        bool HasSkill(uint32 skill) const;

        // Learn spells rewarded by a skill
        void learnSkillRewardedSpells(uint32 id, uint32 value);

        // Get the teleport destination
        WorldLocation& GetTeleportDest() { return m_teleport_dest; }

        // Check if the player is being teleported
        bool IsBeingTeleported() const { return mSemaphoreTeleport_Near || mSemaphoreTeleport_Far; }

        // Check if the player is being teleported near
        bool IsBeingTeleportedNear() const { return mSemaphoreTeleport_Near; }

        // Check if the player is being teleported far
        bool IsBeingTeleportedFar() const { return mSemaphoreTeleport_Far; }

        // Set the semaphore for near teleportation
        void SetSemaphoreTeleportNear(bool semphsetting) { mSemaphoreTeleport_Near = semphsetting; }

        // Set the semaphore for far teleportation
        void SetSemaphoreTeleportFar(bool semphsetting) { mSemaphoreTeleport_Far = semphsetting; }

        // Process delayed operations
        void ProcessDelayedOperations();

        // Check area exploration and outdoor status
        void CheckAreaExploreAndOutdoor();

        // Get the team for a specific race
        static Team TeamForRace(uint8 race);

        // Get the player's team
        Team GetTeam() const { return m_team; }
        TeamId GetTeamId() const { return m_team == ALLIANCE ? TEAM_ALLIANCE : TEAM_HORDE; }
        static uint32 getFactionForRace(uint8 race);

        // Set the faction for a specific race
        void setFactionForRace(uint8 race);

        // Initialize display IDs
        void InitDisplayIds();

        // Check if the player is at group reward distance
        bool IsAtGroupRewardDistance(WorldObject const* pRewardSource) const;

        // Reward a single player at a kill
        void RewardSinglePlayerAtKill(Unit* pVictim);

        // Reward the player and group at an event
        void RewardPlayerAndGroupAtEvent(uint32 creature_id, WorldObject* pRewardSource);

        // Reward the player and group at a cast
        void RewardPlayerAndGroupAtCast(WorldObject* pRewardSource, uint32 spellid = 0);

        // Check if the player is an honor or XP target
        bool isHonorOrXPTarget(Unit* pVictim) const;

        // Get the player's reputation manager
        ReputationMgr& GetReputationMgr() { return m_reputationMgr; }

        // Get the player's reputation manager (const version)
        ReputationMgr const& GetReputationMgr() const { return m_reputationMgr; }

        // Get the player's reputation rank for a specific faction
        ReputationRank GetReputationRank(uint32 faction_id) const;

        // Reward reputation for killing a unit
        void RewardReputation(Unit* pVictim, float rate);

        // Reward reputation for completing a quest
        void RewardReputation(Quest const* pQuest);

        // Calculate the reputation gain
        int32 CalculateReputationGain(ReputationSource source, int32 rep, int32 faction, uint32 creatureOrQuestLevel = 0, bool noAuraBonus = false);

        // Update skills for the player's level
        void UpdateSkillsForLevel();

        // Update skills to the maximum for the player's level
        void UpdateSkillsToMaxSkillsForLevel();

        // Modify the skill bonus
        void ModifySkillBonus(uint32 skillid, int32 val, bool talent);

        /*********************************************************/
        /***                  PVP SYSTEM                       ***/
        /*********************************************************/

        // Update arena fields
        void UpdateArenaFields();

        // Update honor fields
        void UpdateHonorFields();

        // Reward honor for killing a unit
        bool RewardHonor(Unit* pVictim, uint32 groupsize, float honor = -1);

        // Get the player's honor points
        uint32 GetHonorPoints() const { return GetUInt32Value(PLAYER_FIELD_HONOR_CURRENCY); }

        // Get the player's arena points
        uint32 GetArenaPoints() const { return GetUInt32Value(PLAYER_FIELD_ARENA_CURRENCY); }

        // Set the player's honor points
        void SetHonorPoints(uint32 value);

        // Set the player's arena points
        void SetArenaPoints(uint32 value);

        // Modify the player's honor points
        void ModifyHonorPoints(int32 value);

        // Modify the player's arena points
        void ModifyArenaPoints(int32 value);

        uint32 GetMaxPersonalArenaRatingRequirement(uint32 minarenaslot);

        // End of PvP System

        void SetDrunkValue(uint8 newDrunkValue, uint32 itemId = 0);
        uint8 GetDrunkValue() const { return GetByteValue(PLAYER_BYTES_3, 1); }
        static DrunkenState GetDrunkenstateByValue(uint8 value);


        // Get the player's death timer
        uint32 GetDeathTimer() const { return m_deathTimer; }

        // Get the corpse reclaim delay
        uint32 GetCorpseReclaimDelay(bool pvp) const;

        // Update the corpse reclaim delay
        void UpdateCorpseReclaimDelay();

        // Send the corpse reclaim delay
        void SendCorpseReclaimDelay(bool load = false);

        // Get the player's shield block value
        uint32 GetShieldBlockValue() const override;

        // Check if the player can parry
        bool CanParry() const { return m_canParry; }

        // Set the player's ability to parry
        void SetCanParry(bool value);

        // Check if the player can block
        bool CanBlock() const { return m_canBlock; }

        // Set the player's ability to block
        void SetCanBlock(bool value);

        // Check if the player can dual wield
        bool CanDualWield() const { return m_canDualWield; }

        // Set the player's ability to dual wield
        void SetCanDualWield(bool value) { m_canDualWield = value; }
        bool CanTitanGrip() const { return m_canTitanGrip; }
        void SetCanTitanGrip(bool value) { m_canTitanGrip = value; }
        bool CanTameExoticPets() const { return isGameMaster() || HasAuraType(SPELL_AURA_ALLOW_TAME_PET_TYPE); }

        // Set the regular attack time
        void SetRegularAttackTime();

        // Set the base modifier value
        void SetBaseModValue(BaseModGroup modGroup, BaseModType modType, float value) { m_auraBaseMod[modGroup][modType] = value; }

        // Handle the base modifier value
        void HandleBaseModValue(BaseModGroup modGroup, BaseModType modType, float amount, bool apply);

        // Get the base modifier value
        float GetBaseModValue(BaseModGroup modGroup, BaseModType modType) const;

        // Get the total base modifier value
        float GetTotalBaseModValue(BaseModGroup modGroup) const;

        // Get the total percentage modifier value
        float GetTotalPercentageModValue(BaseModGroup modGroup) const { return m_auraBaseMod[modGroup][FLAT_MOD] + m_auraBaseMod[modGroup][PCT_MOD]; }

        // Apply all stat bonuses
        void _ApplyAllStatBonuses();

        // Remove all stat bonuses
        void _RemoveAllStatBonuses();
        float GetArmorPenetrationPct() const { return m_armorPenetrationPct; }
        int32 GetSpellPenetrationItemMod() const { return m_spellPenetrationItemMod; }

        // Apply weapon-dependent aura mods
        void _ApplyWeaponDependentAuraMods(Item* item, WeaponAttackType attackType, bool apply);

        // Apply weapon-dependent aura crit mod
        void _ApplyWeaponDependentAuraCritMod(Item* item, WeaponAttackType attackType, Aura* aura, bool apply);

        // Apply weapon-dependent aura damage mod
        void _ApplyWeaponDependentAuraDamageMod(Item* item, WeaponAttackType attackType, Aura* aura, bool apply);

        // Apply item mods
        void _ApplyItemMods(Item* item, uint8 slot, bool apply);

        // Remove all item mods
        void _RemoveAllItemMods();

        // Apply all item mods
        void _ApplyAllItemMods();
        void _ApplyAllLevelScaleItemMods(bool apply);
        void _ApplyItemBonuses(ItemPrototype const* proto, uint8 slot, bool apply, bool only_level_scale = false);
        void _ApplyAmmoBonuses();

        // Check if an enchantment fits the requirements
        bool EnchantmentFitsRequirements(uint32 enchantmentcondition, int8 slot);

        // Toggle meta gems active state
        void ToggleMetaGemsActive(uint8 exceptslot, bool apply);

        // Correct meta gem enchants
        void CorrectMetaGemEnchants(uint8 slot, bool apply);

        // Initialize data for form
        void InitDataForForm(bool reapplyMods = false);

        // Apply or remove an equip spell from an item
        void ApplyItemEquipSpell(Item* item, bool apply, bool form_change = false);

        // Apply or remove an equip spell from a spell entry
        void ApplyEquipSpell(SpellEntry const* spellInfo, Item* item, bool apply, bool form_change = false);

        // Update equip spells when the player's form changes
        void UpdateEquipSpellsAtFormChange();

        // Cast a combat spell from an item
        void CastItemCombatSpell(Unit* Target, WeaponAttackType attType);
        void CastItemUseSpell(Item* item, SpellCastTargets const& targets, uint8 cast_count, uint32 glyphIndex);

        // Apply or remove a spell from an item when it is stored
        void ApplyItemOnStoreSpell(Item* item, bool apply);

        // Destroy an item with an on-store spell
        void DestroyItemWithOnStoreSpell(Item* item, uint32 spellId);

        void SendEquipmentSetList();
        void SetEquipmentSet(uint32 index, EquipmentSet eqset);
        void DeleteEquipmentSet(uint64 setGuid);

        void SendInitWorldStates(uint32 zone, uint32 area);

        // Send an update for a world state to the client
        void SendUpdateWorldState(uint32 Field, uint32 Value);

        // Send a direct message to the client
        void SendDirectMessage(WorldPacket* data) const;
        void FillBGWeekendWorldStates(WorldPacket& data, uint32& count);

        void SendAurasForTarget(Unit* target);

        // Player menu for interactions
        PlayerMenu* PlayerTalkClass;

        // List of item set effects
        std::vector<ItemSetEffect*> ItemSetEff;

        // Send loot information to the client
        void SendLoot(ObjectGuid guid, LootType loot_type);

        // Send loot release information to the client
        void SendLootRelease(ObjectGuid guid);

        // Notify the client that a loot item was removed
        void SendNotifyLootItemRemoved(uint8 lootSlot);

        // Notify the client that loot money was removed
        void SendNotifyLootMoneyRemoved();

        /*********************************************************/
        /***               BATTLEGROUND SYSTEM                 ***/
        /*********************************************************/

        // Check if the player is in a battleground
        bool InBattleGround() const { return m_bgData.bgInstanceID != 0; }

        // Check if the player is in an arena
        bool InArena() const;

        // Get the battleground ID
        uint32 GetBattleGroundId() const { return m_bgData.bgInstanceID; }

        // Get the battleground type ID
        BattleGroundTypeId GetBattleGroundTypeId() const { return m_bgData.bgTypeID; }

        // Get the battleground instance
        BattleGround* GetBattleGround() const;

        bool InBattleGroundQueue() const
        {
            for (int i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
                if (m_bgBattleGroundQueueID[i].bgQueueTypeId != BATTLEGROUND_QUEUE_NONE)
                {
                    return true;
                }
            return false;
        }

        // Get the battleground queue type ID
        BattleGroundQueueTypeId GetBattleGroundQueueTypeId(uint32 index) const { return m_bgBattleGroundQueueID[index].bgQueueTypeId; }

        // Get the battleground queue index
        uint32 GetBattleGroundQueueIndex(BattleGroundQueueTypeId bgQueueTypeId) const
        {
            for (int i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
                if (m_bgBattleGroundQueueID[i].bgQueueTypeId == bgQueueTypeId)
                {
                    return i;
                }
            return PLAYER_MAX_BATTLEGROUND_QUEUES;
        }

        // Check if the player is invited for a battleground queue type
        bool IsInvitedForBattleGroundQueueType(BattleGroundQueueTypeId bgQueueTypeId) const
        {
            for (int i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
                if (m_bgBattleGroundQueueID[i].bgQueueTypeId == bgQueueTypeId)
                {
                    return m_bgBattleGroundQueueID[i].invitedToInstance != 0;
                }
            return false;
        }

        // Check if the player is in a battleground queue for a specific queue type
        bool InBattleGroundQueueForBattleGroundQueueType(BattleGroundQueueTypeId bgQueueTypeId) const
        {
            return GetBattleGroundQueueIndex(bgQueueTypeId) < PLAYER_MAX_BATTLEGROUND_QUEUES;
        }

        // Set the battleground ID and type
        void SetBattleGroundId(uint32 val, BattleGroundTypeId bgTypeId)
        {
            m_bgData.bgInstanceID = val;
            m_bgData.bgTypeID = bgTypeId;
            m_bgData.m_needSave = true;
        }

        // Add a battleground queue ID
        uint32 AddBattleGroundQueueId(BattleGroundQueueTypeId val)
        {
            for (int i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
            {
                if (m_bgBattleGroundQueueID[i].bgQueueTypeId == BATTLEGROUND_QUEUE_NONE || m_bgBattleGroundQueueID[i].bgQueueTypeId == val)
                {
                    m_bgBattleGroundQueueID[i].bgQueueTypeId = val;
                    m_bgBattleGroundQueueID[i].invitedToInstance = 0;
                    return i;
                }
            }
            return PLAYER_MAX_BATTLEGROUND_QUEUES;
        }

        // Check if the player has a free battleground queue ID
        bool HasFreeBattleGroundQueueId()
        {
            for (int i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
                if (m_bgBattleGroundQueueID[i].bgQueueTypeId == BATTLEGROUND_QUEUE_NONE)
                {
                    return true;
                }
            return false;
        }

        // Remove a battleground queue ID
        void RemoveBattleGroundQueueId(BattleGroundQueueTypeId val)
        {
            for (int i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
            {
                if (m_bgBattleGroundQueueID[i].bgQueueTypeId == val)
                {
                    m_bgBattleGroundQueueID[i].bgQueueTypeId = BATTLEGROUND_QUEUE_NONE;
                    m_bgBattleGroundQueueID[i].invitedToInstance = 0;
                    return;
                }
            }
        }

        // Set the invite for a battleground queue type
        void SetInviteForBattleGroundQueueType(BattleGroundQueueTypeId bgQueueTypeId, uint32 instanceId)
        {
            for (int i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
                if (m_bgBattleGroundQueueID[i].bgQueueTypeId == bgQueueTypeId)
                {
                    m_bgBattleGroundQueueID[i].invitedToInstance = instanceId;
                }
        }

        // Check if the player is invited for a specific battleground instance
        bool IsInvitedForBattleGroundInstance(uint32 instanceId) const
        {
            for (int i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
                if (m_bgBattleGroundQueueID[i].invitedToInstance == instanceId)
                {
                    return true;
                }
            return false;
        }

        // Get the battleground entry point
        WorldLocation const& GetBattleGroundEntryPoint() const { return m_bgData.joinPos; }
        void SetBattleGroundEntryPoint();

        // Set the battleground team
        void SetBGTeam(Team team) { m_bgData.bgTeam = team; m_bgData.m_needSave = true; }

        // Get the battleground team
        Team GetBGTeam() const { return m_bgData.bgTeam ? m_bgData.bgTeam : GetTeam(); }

        // Leave the battleground
        void LeaveBattleground(bool teleportToEntryPoint = true);

        // Check if the player can join a battleground
        bool CanJoinToBattleground() const;

        // Check if the player can report AFK due to limit
        bool CanReportAfkDueToLimit();

        // Report the player as AFK
        void ReportedAfkBy(Player* reporter);

        // Clear AFK reports
        void ClearAfkReports() { m_bgData.bgAfkReporter.clear(); }

        // Check if the player has access to a battleground by level
        bool GetBGAccessByLevel(BattleGroundTypeId bgTypeId) const;

        // Check if the player can use a battleground object
        bool CanUseBattleGroundObject();

        // Check if the player is totally immune
        bool isTotalImmune();

        // Check if the player is in an active state for capture point capturing
        bool CanUseCapturePoint();

        bool GetRandomWinner() { return m_IsBGRandomWinner; }
        void SetRandomWinner(bool isWinner);

        /*********************************************************/
        /***                    REST SYSTEM                    ***/
        /*********************************************************/

        // Check if the player is rested
        bool isRested() const { return GetRestTime() >= 10 * IN_MILLISECONDS; }

        // Get the XP rest bonus
        uint32 GetXPRestBonus(uint32 xp);

        // Get the rest time
        uint32 GetRestTime() const { return m_restTime; }

        // Set the rest time
        void SetRestTime(uint32 v) { m_restTime = v; }

        /*********************************************************/
        /***              ENVIRONMENTAL SYSTEM                  ***/
        /*********************************************************/

        // Handle environmental damage
        uint32 EnvironmentalDamage(EnviromentalDamage type, uint32 damage);

        /*********************************************************/
        /***               FLOOD FILTER SYSTEM                 ***/
        /*********************************************************/

        // Update the speak time
        void UpdateSpeakTime();

        // Check if the player can speak
        bool CanSpeak() const;
        void ChangeSpeakTime(int utime);

        /*********************************************************/
        /***                 VARIOUS SYSTEMS                   ***/
        /*********************************************************/

        // Check if the player has a specific movement flag
        // for script access to m_movementInfo.HasMovementFlag
        bool HasMovementFlag(MovementFlags f) const;

        // Update fall information if needed
        void UpdateFallInformationIfNeed(MovementInfo const& minfo, uint16 opcode);

        // Set fall information
        void SetFallInformation(uint32 time, float z)
        {
            m_lastFallTime = time;
            m_lastFallZ = z;
        }

        // Handle fall
        void HandleFall(MovementInfo const& movementInfo);

        // Build a teleport acknowledgment message
        void BuildTeleportAckMsg(WorldPacket& data, float x, float y, float z, float ang) const;

        // Check if the player is moving
        bool isMoving() const { return m_movementInfo.HasMovementFlag(movementFlagsMask); }

        // Check if the player is moving or turning
        bool isMovingOrTurning() const { return m_movementInfo.HasMovementFlag(movementOrTurningFlagsMask); }

        // Check if the player can swim
        bool CanSwim() const { return true; }

        // Check if the player can fly
        bool CanFly() const { return m_movementInfo.HasMovementFlag(MOVEFLAG_CAN_FLY); }

        // Check if the player is flying
        bool IsFlying() const { return m_movementInfo.HasMovementFlag(MOVEFLAG_FLYING); }

        // Check if the player is free flying
        bool IsFreeFlying() const { return HasAuraType(SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED) || HasAuraType(SPELL_AURA_FLY); }
        bool CanStartFlyInArea(uint32 mapid, uint32 zone, uint32 area) const;

        // Set client control for a target
        void SetClientControl(Unit* target, uint8 allowMove);

        // Set the mover for the player
        void SetMover(Unit* target) { m_mover = target ? target : this; }

        // Get the mover for the player
        Unit* GetMover() const { return m_mover; }

        // Check if the player is the self mover
        bool IsSelfMover() const { return m_mover == this; }

        // Get the far sight GUID
        ObjectGuid const& GetFarSightGuid() const { return GetGuidValue(PLAYER_FARSIGHT); }

        // Get the transport for the player
        Transport* GetTransport() const { return m_transport; }

        // Set the transport for the player
        void SetTransport(Transport* t) { m_transport = t; }

        // Get the X offset of the player's position on the transport
        float GetTransOffsetX() const { return m_movementInfo.GetTransportPos()->x; }

        // Get the Y offset of the player's position on the transport
        float GetTransOffsetY() const { return m_movementInfo.GetTransportPos()->y; }

        // Get the Z offset of the player's position on the transport
        float GetTransOffsetZ() const { return m_movementInfo.GetTransportPos()->z; }

        // Get the orientation offset of the player's position on the transport
        float GetTransOffsetO() const { return m_movementInfo.GetTransportPos()->o; }

        // Get the transport time
        uint32 GetTransTime() const { return m_movementInfo.GetTransportTime(); }
        int8 GetTransSeat() const { return m_movementInfo.GetTransportSeat(); }

        // Get the save timer
        uint32 GetSaveTimer() const { return m_nextSave; }

        // Set the save timer
        void SetSaveTimer(uint32 timer) { m_nextSave = timer; }

        // Recall position
        uint32 m_recallMap; // Map ID of the recall position
        float  m_recallX;   // X coordinate of the recall position
        float  m_recallY;   // Y coordinate of the recall position
        float  m_recallZ;   // Z coordinate of the recall position
        float  m_recallO;   // Orientation of the recall position

        // Save the recall position
        void SaveRecallPosition();

        // Set the homebind location
        void SetHomebindToLocation(WorldLocation const& loc, uint32 area_id);

        // Relocate the player to the homebind location
        void RelocateToHomebind() { SetLocationMapId(m_homebindMapId); Relocate(m_homebindX, m_homebindY, m_homebindZ); }

        // Teleport the player to the homebind location
        bool TeleportToHomebind(uint32 options = 0) { return TeleportTo(m_homebindMapId, m_homebindX, m_homebindY, m_homebindZ, GetOrientation(), options); }

        // Get an object by type mask
        Object* GetObjectByTypeMask(ObjectGuid guid, TypeMask typemask);

        // Currently visible objects at the player's client
        GuidSet m_clientGUIDs;

        // Check if an object is visible to the client
        bool HaveAtClient(WorldObject const* u) { return u == this || m_clientGUIDs.find(u->GetObjectGuid()) != m_clientGUIDs.end(); }

        // Check if the player is visible in the grid for another player
        bool IsVisibleInGridForPlayer(Player* pl) const override;

        // Check if the player is visible globally for another player
        bool IsVisibleGloballyFor(Player* pl) const;

        // Update the visibility of a target from a viewpoint
        void UpdateVisibilityOf(WorldObject const* viewPoint, WorldObject* target);

        // Template function to update the visibility of a target from a viewpoint
        template<class T>
        void UpdateVisibilityOf(WorldObject const* viewPoint, T* target, UpdateData& data, std::set<WorldObject*>& visibleNow);

        // Handle detection of stealthed units
        void HandleStealthedUnitsDetection();

        // Get the player's camera
        Camera& GetCamera() { return m_camera; }

        void SetPhaseMask(uint32 newPhaseMask, bool update) override;// overwrite Unit::SetPhaseMask

        uint8 m_forced_speed_changes[MAX_MOVE_TYPE];

        // Check if the player has a specific at-login flag
        bool HasAtLoginFlag(AtLoginFlags f) const { return m_atLoginFlags & f; }

        // Set an at-login flag for the player
        void SetAtLoginFlag(AtLoginFlags f) { m_atLoginFlags |= f; }

        // Remove an at-login flag for the player
        void RemoveAtLoginFlag(AtLoginFlags f, bool in_db_also = false);

        // Temporarily removed pet cache
        uint32 GetTemporaryUnsummonedPetNumber() const { return m_temporaryUnsummonedPetNumber; }
        void SetTemporaryUnsummonedPetNumber(uint32 petnumber) { m_temporaryUnsummonedPetNumber = petnumber; }
        void UnsummonPetTemporaryIfAny();
        void ResummonPetTemporaryUnSummonedIfAny();
        bool IsPetNeedBeTemporaryUnsummoned() const { return !IsInWorld() || !IsAlive() || IsMounted() /*+in flight*/; }

        // Send cinematic start to the client
        void SendCinematicStart(uint32 CinematicSequenceId);
#if defined(WOTLK) || defined(CATA) || defined(MISTS)
        // Send movie start to the client
        void SendMovieStart(uint32 MovieId);
#endif

        /*********************************************************/
        /***                 INSTANCE SYSTEM                   ***/
        /*********************************************************/

        typedef UNORDERED_MAP < uint32 /*mapId*/, InstancePlayerBind > BoundInstancesMap;

        // Update the homebind time
        void UpdateHomebindTime(uint32 time);

        uint32 m_HomebindTimer; // Homebind timer
        bool m_InstanceValid;   // Instance validity flag

        // Permanent binds and solo binds by difficulty
        BoundInstancesMap m_boundInstances[MAX_DIFFICULTY];

        // Get the bound instance for a specific map and difficulty
        InstancePlayerBind* GetBoundInstance(uint32 mapid, Difficulty difficulty);

        // Get the bound instances for a specific difficulty
        BoundInstancesMap& GetBoundInstances(Difficulty difficulty) { return m_boundInstances[difficulty]; }

        // Unbind an instance
        void UnbindInstance(uint32 mapid, Difficulty difficulty, bool unload = false);

        // Unbind an instance (iterator version)
        void UnbindInstance(BoundInstancesMap::iterator& itr, Difficulty difficulty, bool unload = false);

        // Bind the player to an instance
        InstancePlayerBind* BindToInstance(DungeonPersistentState* save, bool permanent, bool load = false);

        // Send raid information to the client
        void SendRaidInfo();

        // Send saved instances to the client
        void SendSavedInstances();

        // Convert instances to group
        static void ConvertInstancesToGroup(Player* player, Group* group = NULL, ObjectGuid player_guid = ObjectGuid());

        // Get the bound instance save for self or group
        DungeonPersistentState* GetBoundInstanceSaveForSelfOrGroup(uint32 mapid);

        AreaLockStatus GetAreaTriggerLockStatus(AreaTrigger const* at, Difficulty difficulty, uint32& miscRequirement);
        void SendTransferAbortedByLockStatus(MapEntry const* mapEntry, AreaLockStatus lockStatus, uint32 miscRequirement = 0);

        /*********************************************************/
        /***                   GROUP SYSTEM                    ***/
        /*********************************************************/

        // Get the group invite
        Group* GetGroupInvite() { return m_groupInvite; }

        // Set the group invite
        void SetGroupInvite(Group* group) { m_groupInvite = group; }

        // Get the group
        Group* GetGroup() { return m_group.getTarget(); }

        // Get the group (const version)
        const Group* GetGroup() const { return (const Group*)m_group.getTarget(); }

        // Get the group reference
        GroupReference& GetGroupRef() { return m_group; }

        // Set the group
        void SetGroup(Group* group, int8 subgroup = -1);

        // Get the subgroup
        uint8 GetSubGroup() const { return m_group.getSubGroup(); }

        // Get the group update flag
        uint32 GetGroupUpdateFlag() const { return m_groupUpdateMask; }

        // Set the group update flag
        void SetGroupUpdateFlag(uint32 flag) { m_groupUpdateMask |= flag; }

        // Get the aura update mask
        const uint64& GetAuraUpdateMask() const { return m_auraUpdateMask; }

        // Set the aura update mask
        void SetAuraUpdateMask(uint8 slot) { m_auraUpdateMask |= (uint64(1) << slot); }

        // Get the next random raid member within a radius
        Player* GetNextRandomRaidMember(float radius);

        // Check if the player can be uninvited from the group
        PartyResult CanUninviteFromGroup() const;

        // Set the battleground raid group
        void SetBattleGroundRaid(Group* group, int8 subgroup = -1);

        // Remove the player from the battleground raid group
        void RemoveFromBattleGroundRaid();

        // Get the original group
        Group* GetOriginalGroup() { return m_originalGroup.getTarget(); }

        // Get the original group reference
        GroupReference& GetOriginalGroupRef() { return m_originalGroup; }

        // Get the original subgroup
        uint8 GetOriginalSubGroup() const { return m_originalGroup.getSubGroup(); }

        // Set the original group
        void SetOriginalGroup(Group* group, int8 subgroup = -1);

        // Get the grid reference
        GridReference<Player>& GetGridRef() { return m_gridRef; }

        // Get the map reference
        MapReference& GetMapRef() { return m_mapRef; }

        bool isAllowedToLoot(Creature* creature);

        DeclinedName const* GetDeclinedNames() const { return m_declinedname; }

        // Rune functions, need check  getClass() == CLASS_DEATH_KNIGHT before access
        uint8 GetRunesState() const { return m_runes->runeState; }
        RuneType GetBaseRune(uint8 index) const { return RuneType(m_runes->runes[index].BaseRune); }
        RuneType GetCurrentRune(uint8 index) const { return RuneType(m_runes->runes[index].CurrentRune); }
        uint16 GetRuneCooldown(uint8 index) const { return m_runes->runes[index].Cooldown; }
        bool IsBaseRuneSlotsOnCooldown(RuneType runeType) const;
        void SetBaseRune(uint8 index, RuneType baseRune) { m_runes->runes[index].BaseRune = baseRune; }
        void SetCurrentRune(uint8 index, RuneType currentRune) { m_runes->runes[index].CurrentRune = currentRune; }
        void SetRuneCooldown(uint8 index, uint16 cooldown) { m_runes->runes[index].Cooldown = cooldown; m_runes->SetRuneState(index, (cooldown == 0) ? true : false); }
        void ConvertRune(uint8 index, RuneType newType);
        bool ActivateRunes(RuneType type, uint32 count);
        void ResyncRunes();
        void AddRunePower(uint8 index);
        void InitRunes();

        AchievementMgr const& GetAchievementMgr() const { return m_achievementMgr; }
        AchievementMgr& GetAchievementMgr() { return m_achievementMgr; }
        void UpdateAchievementCriteria(AchievementCriteriaTypes type, uint32 miscvalue1 = 0, uint32 miscvalue2 = 0, Unit* unit = NULL, uint32 time = 0);
        void StartTimedAchievementCriteria(AchievementCriteriaTypes type, uint32 timedRequirementId, time_t startTime = 0);

        bool HasTitle(uint32 bitIndex) const;

        // Check if the player has a specific title (overloaded)
        bool HasTitle(CharTitlesEntry const* title) const { return HasTitle(title->bit_index); }

        // Set the player's title
        void SetTitle(CharTitlesEntry const* title, bool lost = false);

        // Check if the player can see a spell click on a creature
        bool canSeeSpellClickOn(Creature const* creature) const;

#ifdef ENABLE_PLAYERBOTS
        //EquipmentSets& GetEquipmentSets() { return m_EquipmentSets; }
        void SetPlayerbotAI(PlayerbotAI* ai) { assert(!m_playerbotAI && !m_playerbotMgr); m_playerbotAI = ai; }
        PlayerbotAI* GetPlayerbotAI() { return m_playerbotAI; }
        void SetPlayerbotMgr(PlayerbotMgr* mgr) { assert(!m_playerbotAI && !m_playerbotMgr); m_playerbotMgr = mgr; }
        PlayerbotMgr* GetPlayerbotMgr() { return m_playerbotMgr; }
        void SetBotDeathTimer() { m_deathTimer = 0; }
        //PlayerTalentMap& GetTalentMap(uint8 spec) { return m_talents[spec]; }
        std::list<Channel*> GetJoinedChannels() { return m_channels; }
#endif

    protected:

        uint32 m_contestedPvPTimer; // Timer for contested PvP state

        /*********************************************************/
        /***               BATTLEGROUND SYSTEM                 ***/
        /*********************************************************/

        /*
        This is an array of BG queues (BgTypeIDs) in which the player is queued
        */
        struct BgBattleGroundQueueID_Rec
        {
            BattleGroundQueueTypeId bgQueueTypeId; // Battleground queue type ID
            uint32 invitedToInstance; // Instance ID the player is invited to
        };

        BgBattleGroundQueueID_Rec m_bgBattleGroundQueueID[PLAYER_MAX_BATTLEGROUND_QUEUES];
        BGData                    m_bgData;
        bool m_IsBGRandomWinner;

        /*********************************************************/
        /***                    QUEST SYSTEM                   ***/
        /*********************************************************/

        // We allow only one timed quest active at the same time. Below can then be simple value instead of set.
        typedef std::set<uint32> QuestSet;
        QuestSet m_timedquests;
        QuestSet m_weeklyquests;
        QuestSet m_monthlyquests;

        ObjectGuid m_dividerGuid; // Divider GUID
        uint32 m_ingametime; // In-game time

        /*********************************************************/
        /***                   LOAD SYSTEM                     ***/
        /*********************************************************/

        // Load player actions from the database
        void _LoadActions(QueryResult* result);

        // Load player auras from the database
        void _LoadAuras(QueryResult* result, uint32 timediff);

        // Load bound instances from the database
        void _LoadBoundInstances(QueryResult* result);

        // Load player inventory from the database
        void _LoadInventory(QueryResult* result, uint32 timediff);

        // Load item loot from the database
        void _LoadItemLoot(QueryResult* result);

        // Load player mails from the database
        void _LoadMails(QueryResult* result);

        // Load mailed items from the database
        void _LoadMailedItems(QueryResult* result);

        // Load quest status from the database
        void _LoadQuestStatus(QueryResult* result);

        // Load daily quest status from the database
        void _LoadDailyQuestStatus(QueryResult* result);
        void _LoadRandomBGStatus(QueryResult* result);
        void _LoadWeeklyQuestStatus(QueryResult* result);
        void _LoadMonthlyQuestStatus(QueryResult* result);
        void _LoadGroup(QueryResult* result);

        // Load player skills from the database
        void _LoadSkills(QueryResult* result);

        // Load player spells from the database
        void _LoadSpells(QueryResult* result);
        void _LoadTalents(QueryResult* result);
        void _LoadFriendList(QueryResult* result);
        bool _LoadHomeBind(QueryResult* result);

        // Load declined names from the database
        void _LoadDeclinedNames(QueryResult* result);

        // Load arena team information from the database
        void _LoadArenaTeamInfo(QueryResult* result);
        void _LoadEquipmentSets(QueryResult* result);
        void _LoadBGData(QueryResult* result);
        void _LoadGlyphs(QueryResult* result);
        void _LoadIntoDataField(const char* data, uint32 startOffset, uint32 count);

        /*********************************************************/
        /***                   SAVE SYSTEM                     ***/
        /*********************************************************/

        // Save player actions to the database
        void _SaveActions();

        // Save player auras to the database
        void _SaveAuras();

        // Save player inventory to the database
        void _SaveInventory();

        // Save player mail to the database
        void _SaveMail();

        // Save quest status to the database
        void _SaveQuestStatus();

        // Save daily quest status to the database
        void _SaveDailyQuestStatus();
        void _SaveWeeklyQuestStatus();
        void _SaveMonthlyQuestStatus();
        void _SaveSkills();
        void _SaveSpells();
        void _SaveEquipmentSets();
        void _SaveBGData();
        void _SaveGlyphs();
        void _SaveTalents();
        void _SaveStats();

        // Set create bits for the update mask
        void _SetCreateBits(UpdateMask* updateMask, Player* target) const override;

        // Set update bits for the update mask
        void _SetUpdateBits(UpdateMask* updateMask, Player* target) const override;

        /*********************************************************/
        /***              ENVIRONMENTAL SYSTEM                 ***/
        /*********************************************************/

        // Handle sobering effect
        void HandleSobering();

        // Send mirror timer to the client
        void SendMirrorTimer(MirrorTimerType Type, uint32 MaxValue, uint32 CurrentValue, int32 Regen);

        // Stop mirror timer
        void StopMirrorTimer(MirrorTimerType Type);

        // Handle drowning effect
        void HandleDrowning(uint32 time_diff);

        // Get the maximum timer value for a mirror timer
        int32 getMaxTimer(MirrorTimerType timer);

        /*********************************************************/
        /***                  HONOR SYSTEM                     ***/
        /*********************************************************/

        time_t m_lastHonorUpdateTime; // Last honor update time

        // Output debug stats values
        void outDebugStatsValues() const;

        ObjectGuid m_lootGuid; // Loot GUID

        Team m_team; // Player's team
        uint32 m_nextSave; // Next save time
        time_t m_speakTime; // Last speak time
        uint32 m_speakCount; // Speak count
        Difficulty m_dungeonDifficulty; // Dungeon difficulty
        Difficulty m_raidDifficulty;

        uint32 m_atLoginFlags; // At-login flags

        Item* m_items[PLAYER_SLOTS_COUNT]; // Array of player items
        uint32 m_currentBuybackSlot; // Current buyback slot

        std::vector<Item*> m_itemUpdateQueue; // Item update queue
        bool m_itemUpdateQueueBlocked; // Item update queue blocked flag

        uint32 m_ExtraFlags; // Extra flags
        ObjectGuid m_curSelectionGuid; // Current selection GUID

        ObjectGuid m_comboTargetGuid; // Combo target GUID
        int8 m_comboPoints; // Combo points

        QuestStatusMap mQuestStatus; // Quest status map

        SkillStatusMap mSkillStatus; // Skill status map

        PlayerMails m_mail;
        PlayerSpellMap m_spells;
        PlayerTalentMap m_talents[MAX_TALENT_SPEC_COUNT];
        SpellCooldowns m_spellCooldowns;
        uint32 m_lastPotionId;                              // last used health/mana potion in combat, that block next potion use
        uint32 m_GuildIdInvited; // Guild ID invited
        uint32 m_ArenaTeamIdInvited; // Arena team ID invited

        uint8 m_activeSpec;
        uint8 m_specsCount;

        ActionButtonList m_actionButtons[MAX_TALENT_SPEC_COUNT];

        Glyph m_glyphs[MAX_TALENT_SPEC_COUNT][MAX_GLYPH_SLOT_INDEX];

        float m_auraBaseMod[BASEMOD_END][MOD_END];
        int16 m_baseRatingValue[MAX_COMBAT_RATING];
        uint16 m_baseSpellPower;
        uint16 m_baseFeralAP;
        uint16 m_baseManaRegen;
        float m_armorPenetrationPct;
        int32 m_spellPenetrationItemMod;

        AuraList m_spellMods[MAX_SPELLMOD];
        GlobalCooldownMgr m_GlobalCooldownMgr; // Global cooldown manager

        int32 m_SpellModRemoveCount; // Spell modifier remove count
        EnchantDurationList m_enchantDuration; // Enchant duration list
        ItemDurationList m_itemDuration; // Item duration list

        ObjectGuid m_resurrectGuid; // Resurrect GUID
        uint32 m_resurrectMap; // Resurrect map ID
        float m_resurrectX, m_resurrectY, m_resurrectZ; // Resurrect coordinates
        uint32 m_resurrectHealth, m_resurrectMana; // Resurrect health and mana

        WorldSession* m_session; // Player session

        typedef std::list<Channel*> JoinedChannelsList;
        JoinedChannelsList m_channels; // List of joined channels

        uint32 m_cinematic; // Cinematic ID

        TradeData* m_trade; // Trade data

        bool   m_DailyQuestChanged;
        bool   m_WeeklyQuestChanged;
        bool   m_MonthlyQuestChanged;

        uint32 m_drunkTimer;
        uint32 m_weaponChangeTimer;


        uint32 m_zoneUpdateId; // Zone update ID
        uint32 m_zoneUpdateTimer; // Zone update timer
        uint32 m_areaUpdateId; // Area update ID
        uint32 m_positionStatusUpdateTimer; // Position status update timer

        uint32 m_deathTimer; // Death timer
        time_t m_deathExpireTime; // Death expire time

        uint32 m_restTime; // Rest time

        uint32 m_WeaponProficiency;
        uint32 m_ArmorProficiency;
        bool m_canParry;
        bool m_canBlock;
        bool m_canDualWield;
        bool m_canTitanGrip;
        uint8 m_swingErrorMsg;
        float m_ammoDPS;

        //////////////////// Rest System/////////////////////
        time_t time_inn_enter; // Time entered inn
        uint32 inn_trigger_id; // Inn trigger ID
        float m_rest_bonus; // Rest bonus
        RestType rest_type; // Rest type
        //////////////////// Rest System/////////////////////

        // Transports
        Transport* m_transport; // Player transport

        uint32 m_resetTalentsCost;
        time_t m_resetTalentsTime;
        uint32 m_usedTalentCount;
        uint32 m_questRewardTalentCount;

        // Social
        PlayerSocial* m_social; // Player social data

        // Groups
        GroupReference m_group; // Group reference
        GroupReference m_originalGroup; // Original group reference
        Group* m_groupInvite; // Group invite
        uint32 m_groupUpdateMask; // Group update mask
        uint64 m_auraUpdateMask; // Aura update mask

        // Player summoning
        time_t m_summon_expire; // Summon expire time
        uint32 m_summon_mapid; // Summon map ID
        float m_summon_x; // Summon X coordinate
        float m_summon_y; // Summon Y coordinate
        float m_summon_z; // Summon Z coordinate

        DeclinedName* m_declinedname;
        Runes* m_runes;
        EquipmentSets m_EquipmentSets;

        /// class dependent melee diminishing constant for dodge/parry/missed chances
        static const float m_diminishing_k[MAX_CLASSES];

    private:
        void _HandleDeadlyPoison(Unit* Target, WeaponAttackType attType, SpellEntry const* spellInfo);
        // internal common parts for CanStore/StoreItem functions
        uint32 m_created_date = 0;

        // Check if an item can be stored in a specific slot
        InventoryResult _CanStoreItem_InSpecificSlot(uint8 bag, uint8 slot, ItemPosCountVec& dest, ItemPrototype const* pProto, uint32& count, bool swap, Item* pSrcItem) const;

        // Check if an item can be stored in a bag
        InventoryResult _CanStoreItem_InBag(uint8 bag, ItemPosCountVec& dest, ItemPrototype const* pProto, uint32& count, bool merge, bool non_specialized, Item* pSrcItem, uint8 skip_bag, uint8 skip_slot) const;

        // Check if an item can be stored in inventory slots
        InventoryResult _CanStoreItem_InInventorySlots(uint8 slot_begin, uint8 slot_end, ItemPosCountVec& dest, ItemPrototype const* pProto, uint32& count, bool merge, Item* pSrcItem, uint8 skip_bag, uint8 skip_slot) const;

        // Store an item in a specific position
        Item* _StoreItem(uint16 pos, Item* pItem, uint32 count, bool clone, bool update);

        // Update known currencies for the player
        void UpdateKnownCurrencies(uint32 itemId, bool apply);

        // Adjust quest required item count
        void AdjustQuestReqItemCount(Quest const* pQuest, QuestStatusData& questStatusData);

        // Set the ability to delay teleport
        void SetCanDelayTeleport(bool setting) { m_bCanDelayTeleport = setting; }

        // Check if the player has a delayed teleport
        bool IsHasDelayedTeleport() const
        {
            // We should not execute delayed teleports for now dead players but has been alive at teleport
            // because we don't want player's ghost teleported from graveyard
            return m_bHasDelayedTeleport && (IsAlive() || !m_bHasBeenAliveAtDelayedTeleport);
        }

        // Set the delayed teleport flag if possible
        bool SetDelayedTeleportFlagIfCan()
        {
            m_bHasDelayedTeleport = m_bCanDelayTeleport;
            m_bHasBeenAliveAtDelayedTeleport = IsAlive();
            return m_bHasDelayedTeleport;
        }

        // Schedule a delayed operation
        void ScheduleDelayedOperation(uint32 operation)
        {
            if (operation < DELAYED_END)
            {
                m_DelayedOperations |= operation;
            }
        }

        void _fillGearScoreData(Item* item, GearScoreVec* gearScore, uint32& twoHandScore);

        Unit* m_mover;

        // The player's camera
        Camera m_camera;

        // Grid reference for the player
        GridReference<Player> m_gridRef;

        // Map reference for the player
        MapReference m_mapRef;

        // Homebind coordinates
        uint32 m_homebindMapId; // Map ID of the homebind location
        uint16 m_homebindAreaId; // Area ID of the homebind location
        float m_homebindX; // X coordinate of the homebind location
        float m_homebindY; // Y coordinate of the homebind location
        float m_homebindZ; // Z coordinate of the homebind location

        // Last fall time and Z coordinate
        uint32 m_lastFallTime;
        float  m_lastFallZ;

        // Last liquid type the player was in
        LiquidTypeEntry const* m_lastLiquid;

        // Mirror timers for various effects
        int32 m_MirrorTimer[MAX_TIMERS];
        uint8 m_MirrorTimerFlags;
        uint8 m_MirrorTimerFlagsLast;
        bool m_isInWater;

        // Current teleport data
        WorldLocation m_teleport_dest; // Destination of the teleport
        uint32 m_teleport_options; // Options for the teleport
        bool mSemaphoreTeleport_Near; // Semaphore for near teleport
        bool mSemaphoreTeleport_Far; // Semaphore for far teleport

        // Delayed operations
        uint32 m_DelayedOperations;
        bool m_bCanDelayTeleport; // Can delay teleport flag
        bool m_bHasDelayedTeleport; // Has delayed teleport flag
        bool m_bHasBeenAliveAtDelayedTeleport; // Has been alive at delayed teleport flag

        // Detect invisibility timer
        uint32 m_DetectInvTimer;

        // Temporary removed pet cache
        uint32 m_temporaryUnsummonedPetNumber;

        AchievementMgr m_achievementMgr;
        ReputationMgr  m_reputationMgr;

        uint32 m_timeSyncCounter;
        uint32 m_timeSyncTimer;
        uint32 m_timeSyncClient;
        uint32 m_timeSyncServer;

        uint32 m_cachedGS;

#ifdef ENABLE_PLAYERBOTS
        PlayerbotAI* m_playerbotAI;
        PlayerbotMgr* m_playerbotMgr;
#endif
};

void AddItemsSetItem(Player* player, Item* item);
void RemoveItemsSetItem(Player* player, ItemPrototype const* proto);

#endif

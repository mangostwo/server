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

#include "Player.h"
#include "Language.h"
#include "Database/DatabaseEnv.h"
#include "Log.h"
#include "Opcodes.h"
#include "SpellMgr.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "UpdateMask.h"
#include "CinematicFlyover.h"
#include "SkillDiscovery.h"
#include "QuestDef.h"
#include "GossipDef.h"
#include "UpdateData.h"
#include "Channel.h"
#include "ChannelMgr.h"
#include "MapManager.h"
#include "MapPersistentStateMgr.h"
#include "InstanceData.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "CreatureAI.h"
#include "Formulas.h"
#include "Group.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Pet.h"
#include "Util.h"
#include "Transports.h"
#include "Weather.h"
#include "BattleGround/BattleGround.h"
#include "BattleGround/BattleGroundMgr.h"
#include "BattleGround/BattleGroundAV.h"
#include "OutdoorPvP/OutdoorPvP.h"
#include "ArenaTeam.h"
#include "Chat.h"
#include "revision_data.h"
#include "Database/DatabaseImpl.h"
#include "Spell.h"
#include "ScriptMgr.h"
#include "SocialMgr.h"
#include "AchievementMgr.h"
#include "Mail.h"
#include "SpellAuras.h"
#include "DBCStores.h"
#include "SQLStorages.h"
#include "Vehicle.h"
#include "Calendar.h"
#include "LFGMgr.h"
#include "DisableMgr.h"
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif /* ENABLE_ELUNA */

#include <cmath>

#define ZONE_UPDATE_INTERVAL (1*IN_MILLISECONDS)

#define PLAYER_SKILL_INDEX(x)       (PLAYER_SKILL_INFO_1_1 + ((x)*3))
#define PLAYER_SKILL_VALUE_INDEX(x) (PLAYER_SKILL_INDEX(x)+1)
#define PLAYER_SKILL_BONUS_INDEX(x) (PLAYER_SKILL_INDEX(x)+2)

#define SKILL_VALUE(x)         PAIR32_LOPART(x)
#define SKILL_MAX(x)           PAIR32_HIPART(x)
#define MAKE_SKILL_VALUE(v, m) MAKE_PAIR32(v,m)

#define SKILL_TEMP_BONUS(x)    int16(PAIR32_LOPART(x))
#define SKILL_PERM_BONUS(x)    int16(PAIR32_HIPART(x))
#define MAKE_SKILL_BONUS(t, p) MAKE_PAIR32(t,p)

enum CharacterFlags
{
    CHARACTER_FLAG_NONE                 = 0x00000000,
    CHARACTER_FLAG_UNK1                 = 0x00000001,
    CHARACTER_FLAG_UNK2                 = 0x00000002,
    CHARACTER_LOCKED_FOR_TRANSFER       = 0x00000004,
    CHARACTER_FLAG_UNK4                 = 0x00000008,
    CHARACTER_FLAG_UNK5                 = 0x00000010,
    CHARACTER_FLAG_UNK6                 = 0x00000020,
    CHARACTER_FLAG_UNK7                 = 0x00000040,
    CHARACTER_FLAG_UNK8                 = 0x00000080,
    CHARACTER_FLAG_UNK9                 = 0x00000100,
    CHARACTER_FLAG_UNK10                = 0x00000200,
    CHARACTER_FLAG_HIDE_HELM            = 0x00000400,
    CHARACTER_FLAG_HIDE_CLOAK           = 0x00000800,
    CHARACTER_FLAG_UNK13                = 0x00001000,
    CHARACTER_FLAG_GHOST                = 0x00002000,
    CHARACTER_FLAG_RENAME               = 0x00004000,
    CHARACTER_FLAG_UNK16                = 0x00008000,
    CHARACTER_FLAG_UNK17                = 0x00010000,
    CHARACTER_FLAG_UNK18                = 0x00020000,
    CHARACTER_FLAG_UNK19                = 0x00040000,
    CHARACTER_FLAG_UNK20                = 0x00080000,
    CHARACTER_FLAG_UNK21                = 0x00100000,
    CHARACTER_FLAG_UNK22                = 0x00200000,
    CHARACTER_FLAG_UNK23                = 0x00400000,
    CHARACTER_FLAG_UNK24                = 0x00800000,
    CHARACTER_FLAG_LOCKED_BY_BILLING    = 0x01000000,
    CHARACTER_FLAG_DECLINED             = 0x02000000,
    CHARACTER_FLAG_UNK27                = 0x04000000,
    CHARACTER_FLAG_UNK28                = 0x08000000,
    CHARACTER_FLAG_UNK29                = 0x10000000,
    CHARACTER_FLAG_UNK30                = 0x20000000,
    CHARACTER_FLAG_UNK31                = 0x40000000,
    CHARACTER_FLAG_UNK32                = 0x80000000
};

enum CharacterCustomizeFlags
{
    CHAR_CUSTOMIZE_FLAG_NONE            = 0x00000000,
    CHAR_CUSTOMIZE_FLAG_CUSTOMIZE       = 0x00000001,       // name, gender, etc...
    CHAR_CUSTOMIZE_FLAG_FACTION         = 0x00010000,       // name, gender, faction, etc...
    CHAR_CUSTOMIZE_FLAG_RACE            = 0x00100000        // name, gender, race, etc...
};



//== PlayerTaxi ================================================

PlayerTaxi::PlayerTaxi()
{
    // Taxi nodes
    memset(m_taximask, 0, sizeof(m_taximask));
}

/**
 * @brief Initializes known taxi nodes for a newly created player.
 *
 * @param race The player race id.
 * @param level Unused player level.
 */
void PlayerTaxi::InitTaxiNodesForLevel(uint32 race, uint32 chrClass, uint32 level)
{
    // class specific initial known nodes
    switch (chrClass)
    {
        case CLASS_DEATH_KNIGHT:
        {
            for (int i = 0; i < TaxiMaskSize; ++i)
            {
                m_taximask[i] |= sOldContinentsNodesMask[i];
            }
            break;
        }
    }

    // race specific initial known nodes: capital and taxi hub masks
    switch (race)
    {
        case RACE_HUMAN:    SetTaximaskNode(2);  break;     // Human
        case RACE_ORC:      SetTaximaskNode(23); break;     // Orc
        case RACE_DWARF:    SetTaximaskNode(6);  break;     // Dwarf
        case RACE_NIGHTELF: SetTaximaskNode(26);
            SetTaximaskNode(27); break;     // Night Elf
        case RACE_UNDEAD:   SetTaximaskNode(11); break;     // Undead
        case RACE_TAUREN:   SetTaximaskNode(22); break;     // Tauren
        case RACE_GNOME:    SetTaximaskNode(6);  break;     // Gnome
        case RACE_TROLL:    SetTaximaskNode(23); break;     // Troll
        case RACE_BLOODELF: SetTaximaskNode(82); break;     // Blood Elf
        case RACE_DRAENEI:  SetTaximaskNode(94); break;     // Draenei
    }

    // new continent starting masks (It will be accessible only at new map)
    switch (Player::TeamForRace(race))
    {
        case ALLIANCE: SetTaximaskNode(100); break;
        case HORDE:    SetTaximaskNode(99);  break;
        default: break;
    }
    // level dependent taxi hubs
    if (level >= 68)
    {
        SetTaximaskNode(213);                               // Shattered Sun Staging Area
    }
}

/**
 * @brief Loads the known taxi-node bitmask from a serialized string.
 *
 * @param data The serialized taxi mask data.
 */
void PlayerTaxi::LoadTaxiMask(const char* data)
{
    Tokens tokens = StrSplit(data, " ");

    int index;
    Tokens::iterator iter;
    for (iter = tokens.begin(), index = 0; (index < TaxiMaskSize) && (iter != tokens.end()); ++iter, ++index)
    {
        // load and set bits only for existing taxi nodes
        m_taximask[index] = sTaxiNodesMask[index] & uint32(atol((*iter).c_str()));
    }
}

/**
 * @brief Appends the player's taxi-node mask to a packet buffer.
 *
 * @param data The destination packet buffer.
 * @param all true to append all existing nodes instead of only known nodes.
 */
void PlayerTaxi::AppendTaximaskTo(ByteBuffer& data, bool all)
{
    if (all)
    {
        for (uint8 i = 0; i < TaxiMaskSize; ++i)
        {
            data << uint32(sTaxiNodesMask[i]);               // all existing nodes
        }
    }
    else
    {
        for (uint8 i = 0; i < TaxiMaskSize; ++i)
        {
            data << uint32(m_taximask[i]);                   // known nodes
        }
    }
}

/**
 * @brief Loads active taxi destinations from a serialized path string.
 *
 * @param values The serialized taxi destination list.
 * @param team The player's faction team.
 * @return true if the taxi route is valid; otherwise, false.
 */
bool PlayerTaxi::LoadTaxiDestinationsFromString(const std::string& values, Team team)
{
    ClearTaxiDestinations();

    Tokens tokens = StrSplit(values, " ");

    for (Tokens::iterator iter = tokens.begin(); iter != tokens.end(); ++iter)
    {
        uint32 node = uint32(atol(iter->c_str()));
        AddTaxiDestination(node);
    }

    if (m_TaxiDestinations.empty())
    {
        return true;
    }

    // Check integrity
    if (m_TaxiDestinations.size() < 2)
    {
        return false;
    }

    for (size_t i = 1; i < m_TaxiDestinations.size(); ++i)
    {
        uint32 cost;
        uint32 path;
        sObjectMgr.GetTaxiPath(m_TaxiDestinations[i - 1], m_TaxiDestinations[i], path, cost);
        if (!path)
        {
            return false;
        }
    }

    // can't load taxi path without mount set (quest taxi path?)
    if (!sObjectMgr.GetTaxiMountDisplayId(GetTaxiSource(), team, true))
    {
        return false;
    }

    return true;
}

/**
 * @brief Serializes the current taxi destination list.
 *
 * @return The serialized taxi destination string.
 */
std::string PlayerTaxi::SaveTaxiDestinationsToString()
{
    if (m_TaxiDestinations.empty())
    {
        return "";
    }

    std::ostringstream ss;

    for (size_t i = 0; i < m_TaxiDestinations.size(); ++i)
    {
        ss << m_TaxiDestinations[i] << " ";
    }

    return ss.str();
}

/**
 * @brief Gets the taxi path id for the current first route segment.
 *
 * @return The current taxi path id, or 0 if no valid route exists.
 */
uint32 PlayerTaxi::GetCurrentTaxiPath() const
{
    if (m_TaxiDestinations.size() < 2)
    {
        return 0;
    }

    uint32 path;
    uint32 cost;

    sObjectMgr.GetTaxiPath(m_TaxiDestinations[0], m_TaxiDestinations[1], path, cost);

    return path;
}

/**
 * @brief Serializes the player's discovered taxi mask into a stream.
 *
 * @param ss The destination output stream.
 * @param taxi The taxi data to serialize.
 * @return The output stream.
 */
std::ostringstream& operator<< (std::ostringstream& ss, PlayerTaxi const& taxi)
{
    for (int i = 0; i < TaxiMaskSize; ++i)
    {
        ss << taxi.m_taximask[i] << " ";
    }
    return ss;
}
//== TradeData =================================================

TradeData* TradeData::GetTraderData() const
{
    return m_trader->GetTradeData();
}

/**
 * @brief Gets the item placed in a trade slot.
 *
 * @param slot The trade slot.
 * @return The item in the slot, or null if empty.
 */
Item* TradeData::GetItem(TradeSlots slot) const
{
    return m_items[slot] ? m_player->GetItemByGuid(m_items[slot]) : NULL;
}

/**
 * @brief Checks whether a specific item is part of the current trade.
 *
 * @param item_guid The item GUID to test.
 * @return true if the item is present in a trade slot; otherwise, false.
 */
bool TradeData::HasItem(ObjectGuid item_guid) const
{
    for (int i = 0; i < TRADE_SLOT_COUNT; ++i)
    {
        if (m_items[i] == item_guid)
        {
            return true;
        }
    }
    return false;
}


/**
 * @brief Gets the item used as the current trade spell reagent.
 *
 * @return The spell-cast item, or null if none is set.
 */
Item* TradeData::GetSpellCastItem() const
{
    return m_spellCastItem ?  m_player->GetItemByGuid(m_spellCastItem) : NULL;
}

/**
 * @brief Sets the item assigned to a trade slot and refreshes trade state.
 *
 * @param slot The trade slot to update.
 * @param item The item to place in the slot, or null to clear it.
 */
void TradeData::SetItem(TradeSlots slot, Item* item)
{
    ObjectGuid itemGuid = item ? item->GetObjectGuid() : ObjectGuid();

    if (m_items[slot] == itemGuid)
    {
        return;
    }

    m_items[slot] = itemGuid;

    SetAccepted(false);
    GetTraderData()->SetAccepted(false);

    Update();

    // need remove possible trader spell applied to changed item
    if (slot == TRADE_SLOT_NONTRADED)
    {
        GetTraderData()->SetSpell(0);
    }

    // need remove possible player spell applied (possible move reagent)
    SetSpell(0);
}

/**
 * @brief Sets the spell cast through the trade window.
 *
 * @param spell_id The spell identifier.
 * @param castItem The optional reagent item used for the spell.
 */
void TradeData::SetSpell(uint32 spell_id, Item* castItem /*= NULL*/)
{
    ObjectGuid itemGuid = castItem ? castItem->GetObjectGuid() : ObjectGuid();

    if (m_spell == spell_id && m_spellCastItem == itemGuid)
    {
        return;
    }

    m_spell = spell_id;
    m_spellCastItem = itemGuid;

    SetAccepted(false);
    GetTraderData()->SetAccepted(false);

    Update(true);                                           // send spell info to item owner
    Update(false);                                          // send spell info to caster self
}

/**
 * @brief Sets the trade money offer and refreshes trade state.
 *
 * @param money The offered money amount.
 */
void TradeData::SetMoney(uint32 money)
{
    if (m_money == money)
    {
        return;
    }

    m_money = money;

    SetAccepted(false);
    GetTraderData()->SetAccepted(false);

    Update();
}

/**
 * @brief Sends the current trade state to one side of the trade.
 *
 * @param for_trader true to update the trader view; false to update the player view.
 */
void TradeData::Update(bool for_trader /*= true*/)
{
    if (for_trader)
    {
        m_trader->GetSession()->SendUpdateTrade(true); // player state for trader
    }
    else
    {
        m_player->GetSession()->SendUpdateTrade(false); // player state for player
    }
}

/**
 * @brief Sets the accepted state for the trade and optionally notifies the other trader.
 *
 * @param state The new accepted state.
 * @param crosssend true to send the status to the trader instead of the owner.
 */
void TradeData::SetAccepted(bool state, bool crosssend /*= false*/)
{
    m_accepted = state;

    if (!state)
    {
        TradeStatusInfo info;
        info.Status = TRADE_STATUS_BACK_TO_TRADE;
        if (crosssend)
        {
            m_trader->GetSession()->SendTradeStatus(info);
        }
        else
        {
            m_player->GetSession()->SendTradeStatus(info);
        }
    }
}

//== Player ====================================================

UpdateMask Player::updateVisualBits;

/**
 * @brief Initializes a player instance and its runtime state.
 *
 * @param session The owning world session.
 */
Player::Player(WorldSession* session): Unit(), m_mover(this), m_camera(this), m_achievementMgr(this), m_reputationMgr(this)
{
    m_transport = 0;

    m_speakTime = 0;
    m_speakCount = 0;

    m_visibilityObserverSweepTimer = World::GetVisibilityObserverSweepInterval();

    m_objectType |= TYPEMASK_PLAYER;
    m_objectTypeId = TYPEID_PLAYER;

    m_valuesCount = PLAYER_END;

    SetActiveObjectState(true);                             // player is always active object

    m_session = session;

    m_ExtraFlags = 0;
    if (GetSession()->GetSecurity() >= SEC_GAMEMASTER)
    {
        SetAcceptTicket(true);
    }

    // players always accept
    if (GetSession()->GetSecurity() == SEC_PLAYER)
    {
        SetAcceptWhispers(true);
    }

    m_comboPoints = 0;

    m_usedTalentCount = 0;
    m_questRewardTalentCount = 0;

    m_regenTimer = 0;
    m_weaponChangeTimer = 0;

    m_zoneUpdateId = 0;
    m_zoneUpdateTimer = 0;
    m_positionStatusUpdateTimer = 0;

    m_areaUpdateId = 0;

    m_nextSave = sWorld.getConfig(CONFIG_UINT32_INTERVAL_SAVE);

    // randomize first save time in range [CONFIG_UINT32_INTERVAL_SAVE] around [CONFIG_UINT32_INTERVAL_SAVE]
    // this must help in case next save after mass player load after server startup
    m_nextSave = urand(m_nextSave / 2, m_nextSave * 3 / 2);

    clearResurrectRequestData();

    memset(m_items, 0, sizeof(Item*)*PLAYER_SLOTS_COUNT);

    m_social = NULL;

    // group is initialized in the reference constructor
    SetGroupInvite(NULL);
    m_groupUpdateMask = 0;
    m_auraUpdateMask = 0;

    duel = NULL;

    m_GuildIdInvited = 0;
    m_ArenaTeamIdInvited = 0;

    m_atLoginFlags = AT_LOGIN_NONE;

    mSemaphoreTeleport_Near = false;
    mSemaphoreTeleport_Far = false;

    m_DelayedOperations = 0;
    m_bCanDelayTeleport = false;
    m_bHasDelayedTeleport = false;
    m_bHasBeenAliveAtDelayedTeleport = true;                // overwrite always at setup teleport data, so not used infact
    m_teleport_options = 0;

    m_trade = NULL;

    m_cinematic = 0;

    PlayerTalkClass = new PlayerMenu(GetSession());
    m_currentBuybackSlot = BUYBACK_SLOT_START;

    m_DailyQuestChanged = false;
    m_WeeklyQuestChanged = false;

    m_lastLiquid = NULL;

    for (int i = 0; i < MAX_TIMERS; ++i)
    {
        m_MirrorTimer[i] = DISABLED_MIRROR_TIMER;
    }

    m_MirrorTimerFlags = UNDERWATER_NONE;
    m_MirrorTimerFlagsLast = UNDERWATER_NONE;

    m_isInWater = false;
    m_drunkTimer = 0;
    m_restTime = 0;
    m_deathTimer = 0;
    // Initialize death expire time to 0
    m_deathExpireTime = 0;

    // Initialize swing error message to 0
    m_swingErrorMsg = 0;

    // Initialize detection invisibility timer to 1 millisecond
    m_DetectInvTimer = 1 * IN_MILLISECONDS;

    // Initialize battleground queue IDs and invited instances
    for (int j = 0; j < PLAYER_MAX_BATTLEGROUND_QUEUES; ++j)
    {
        m_bgBattleGroundQueueID[j].bgQueueTypeId  = BATTLEGROUND_QUEUE_NONE;
        m_bgBattleGroundQueueID[j].invitedToInstance = 0;
    }

    // Set login time to current time
    m_logintime = time(NULL);
    // Set last tick time to login time
    m_Last_tick = m_logintime;
    // Initialize weapon proficiency to 0
    m_WeaponProficiency = 0;
    // Initialize armor proficiency to 0
    m_ArmorProficiency = 0;
    // Initialize parry ability to false
    m_canParry = false;
    // Initialize block ability to false
    m_canBlock = false;
    // Initialize dual wield ability to false
    m_canDualWield = false;
    m_canTitanGrip = false;
    m_ammoDPS = 0.0f;

    // Initialize temporary unsummoned pet number to 0
    m_temporaryUnsummonedPetNumber = 0;

    //////////////////// Rest System/////////////////////
    // Initialize time of entering inn to 0
    time_inn_enter = 0;
    // Initialize inn trigger ID to 0
    inn_trigger_id = 0;
    // Initialize rest bonus to 0
    m_rest_bonus = 0;
    // Initialize rest type to no rest
    rest_type = REST_TYPE_NO;
    //////////////////// Rest System/////////////////////

    // Initialize mails updated flag to false
    m_mailsUpdated = false;
    // Initialize unread mails count to 0
    unReadMails = 0;
    // Initialize next mail delivery time to 0
    m_nextMailDelivereTime = 0;

    // Initialize reset talents cost to 0
    m_resetTalentsCost = 0;
    // Initialize reset talents time to 0
    m_resetTalentsTime = 0;
    // Initialize item update queue blocked flag to false
    m_itemUpdateQueueBlocked = false;

    // Initialize forced speed changes for all move types to 0
    for (int i = 0; i < MAX_MOVE_TYPE; ++i)
    {
        m_forced_speed_changes[i] = 0;
    }

    // Initialize stable slots to 0
    m_stableSlots = 0;

    /////////////////// Instance System /////////////////////
    // Initialize homebind timer to 0
    m_HomebindTimer = 0;
    // Initialize instance validity to true
    m_InstanceValid = true;
    // Initialize dungeon difficulty to normal
    m_dungeonDifficulty = DUNGEON_DIFFICULTY_NORMAL;
    m_raidDifficulty = RAID_DIFFICULTY_10MAN_NORMAL;

    m_lastPotionId = 0;

    m_activeSpec = 0;
    m_specsCount = 1;

    // Initialize aura base modifiers
    for (int i = 0; i < BASEMOD_END; ++i)
    {
        m_auraBaseMod[i][FLAT_MOD] = 0.0f;
        m_auraBaseMod[i][PCT_MOD] = 1.0f;
    }

    // Initialize base rating values to 0
    for (int i = 0; i < MAX_COMBAT_RATING; ++i)
    {
        m_baseRatingValue[i] = 0;
    }

    m_baseSpellPower = 0;
    m_baseFeralAP = 0;
    m_baseManaRegen = 0;
    m_armorPenetrationPct = 0.0f;
    m_spellPenetrationItemMod = 0;

    // Honor System
    // Set last honor update time to current time
    m_lastHonorUpdateTime = time(NULL);

    m_IsBGRandomWinner = false;

    // Player summoning
    // Initialize summon expire time to 0
    m_summon_expire = 0;
    // Initialize summon map ID to 0
    m_summon_mapid = 0;
    // Initialize summon coordinates to (0.0f, 0.0f, 0.0f)
    m_summon_x = 0.0f;
    m_summon_y = 0.0f;
    m_summon_z = 0.0f;

    // Initialize contested PvP timer to 0
    m_contestedPvPTimer = 0;

    // Initialize declined name to NULL
    m_declinedname = NULL;
    m_runes = NULL;

    // Initialize last fall time to 0
    m_lastFallTime = 0;
    // Initialize last fall Z coordinate to 0
    m_lastFallZ = 0;

    m_cachedGS = 0;
}

/**
 * @brief Destroys the player and releases owned resources.
 */
Player::~Player()
{
    // Perform cleanup before deleting the player object
    CleanupsBeforeDelete();

    // Ensure the social object is unloaded (should already be done in PlayerLogout)
    // m_social = NULL;

    // Delete all items in the player's inventory
    for (int i = 0; i < PLAYER_SLOTS_COUNT; ++i)
    {
        delete m_items[i];
    }

    // Clean up communication channels
    CleanupChannels();

    // Delete all mailed items and deallocate mail objects
    for (PlayerMails::const_iterator itr = m_mail.begin(); itr != m_mail.end(); ++itr)
    {
        delete *itr;
    }

    // Delete all items in the player's item map
    for (ItemMap::const_iterator iter = mMitems.begin(); iter != mMitems.end(); ++iter)
    {
        delete iter->second; // Ensure no duplicated items to avoid crashes
    }

    // Delete the player's talk class
    delete PlayerTalkClass;

    // Remove the player from any transport they are on
    if (m_transport)
    {
        m_transport->RemovePassenger(this);
    }

    // Delete all item set effects
    for (size_t x = 0; x < ItemSetEff.size(); ++x)
    {
        delete ItemSetEff[x];
    }

    // clean up player-instance binds, may unload some instance saves
    for (uint8 i = 0; i < MAX_DIFFICULTY; ++i)
    {
        for (BoundInstancesMap::iterator itr = m_boundInstances[i].begin(); itr != m_boundInstances[i].end(); ++itr)
        {
            itr->second.state->RemovePlayer(this);
        }
    }

    delete m_declinedname;
    delete m_runes;
}

/**
 * @brief Performs pre-destruction cleanup for trade, duel, zone, and unit state.
 */
void Player::CleanupsBeforeDelete()
{
    // Stop cinematic flyover if present (must happen before camera dtor).
    // DK may hold an early visibility lease before the flyover is active.
    if (m_cinematicFlyover)
    {
        m_cinematicFlyover->Stop();
    }
    m_cinematicFlyover.reset();

    // Perform cleanup only if the object is fully created
    if (m_uint32Values)
    {
        // Cancel any ongoing trade
        TradeCancel(false);
        // Complete any ongoing duel
        DuelComplete(DUEL_FLED);
    }

    // Notify zone scripts that the player is leaving the zone
    sOutdoorPvPMgr.HandlePlayerLeaveZone(this, m_zoneUpdateId);

    // Perform unit-specific cleanup
    Unit::CleanupsBeforeDelete();
}

/**
 * @brief Creates a new player character with starting data and equipment.
 *
 * @param guidlow The low GUID for the player.
 * @param name The player name.
 * @param race The race id.
 * @param class_ The class id.
 * @param gender The gender id.
 * @param skin The skin customization id.
 * @param face The face customization id.
 * @param hairStyle The hairstyle customization id.
 * @param hairColor The hair color customization id.
 * @param facialHair The facial hair customization id.
 * @param outfitId Unused outfit identifier.
 * @return true if the player was initialized successfully; otherwise, false.
 */
bool Player::Create(uint32 guidlow, const std::string& name, uint8 race, uint8 class_, uint8 gender, uint8 skin, uint8 face, uint8 hairStyle, uint8 hairColor, uint8 facialHair, uint8 /*outfitId */)
{
    // FIXME: outfitId not used in player creation

    // Create the player object with the given GUID
    Object::_Create(guidlow, 0, HIGHGUID_PLAYER);

    // Set the player's name
    m_name = name;

    // Get player info based on race and class
    PlayerInfo const* info = sObjectMgr.GetPlayerInfo(race, class_);
    if (!info)
    {
        sLog.outError("Player has incorrect race/class pair. Can't be loaded.");
        return false;
    }

    // Get class entry from DBC
    ChrClassesEntry const* cEntry = sChrClassesStore.LookupEntry(class_);
    if (!cEntry)
    {
        sLog.outError("Class %u not found in DBC (Wrong DBC files?)", class_);
        return false;
    }

    // Validate gender
    if (gender != uint8(GENDER_MALE) && gender != uint8(GENDER_FEMALE))
    {
        sLog.outError("Invalid gender %u at player creation", uint32(gender));
        return false;
    }

    // Initialize player items to NULL
    for (int i = 0; i < PLAYER_SLOTS_COUNT; ++i)
    {
        m_items[i] = NULL;
    }

    // Set player's initial location
    SetLocationMapId(info->mapId);
    Relocate(info->positionX, info->positionY, info->positionZ, info->orientation);

    // Set the player's map
    SetMap(sMapMgr.CreateMap(info->mapId, this));

    // Set player's power type based on class
    uint8 powertype = cEntry->powerType;

    // Set player's faction based on race
    setFactionForRace(race);

    // Set player's race, class, gender, and power type
    SetByteValue(UNIT_FIELD_BYTES_0, 0, race);
    SetByteValue(UNIT_FIELD_BYTES_0, 1, class_);
    SetByteValue(UNIT_FIELD_BYTES_0, 2, gender);
    SetByteValue(UNIT_FIELD_BYTES_0, 3, powertype);

    // Initialize player's display IDs (model, scale, and model data)
    InitDisplayIds();

    SetByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_PVP);
    SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
    SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_REGENERATE_POWER);
    SetFloatValue(UNIT_MOD_CAST_SPEED, 1.0f);               // fix cast time showed in spell tooltip on client
    SetFloatValue(UNIT_FIELD_HOVERHEIGHT, 1.0f);            // default for players in 3.0.3

    // Set default watched faction index
    SetInt32Value(PLAYER_FIELD_WATCHED_FACTION_INDEX, -1); // -1 is default value

    // Set player's appearance (skin, face, hair style, hair color, facial hair)
    SetByteValue(PLAYER_BYTES, 0, skin);
    SetByteValue(PLAYER_BYTES, 1, face);
    SetByteValue(PLAYER_BYTES, 2, hairStyle);
    SetByteValue(PLAYER_BYTES, 3, hairColor);
    SetByteValue(PLAYER_BYTES_2, 0, facialHair);
    SetByteValue(PLAYER_BYTES_2, 3, REST_STATE_NORMAL);

    SetByteValue(PLAYER_BYTES_3, 0, gender);
    SetByteValue(PLAYER_BYTES_3, 3, 0);                     // BattlefieldArenaFaction (0 or 1)

    // Initialize player's guild information
    SetUInt32Value(PLAYER_GUILDID, 0);
    SetUInt32Value(PLAYER_GUILDRANK, 0);
    SetUInt32Value(PLAYER_GUILD_TIMESTAMP, 0);

    for (int i = 0; i < KNOWN_TITLES_SIZE; ++i)
    {
        SetUInt64Value(PLAYER__FIELD_KNOWN_TITLES + i, 0);  // 0=disabled
    }
    SetUInt32Value(PLAYER_CHOSEN_TITLE, 0);

    // Initialize player's kill counts and contributions
    SetUInt32Value(PLAYER_FIELD_KILLS, 0);
    SetUInt32Value(PLAYER_FIELD_LIFETIME_HONORABLE_KILLS, 0);
    SetUInt32Value(PLAYER_FIELD_TODAY_CONTRIBUTION, 0);
    SetUInt32Value(PLAYER_FIELD_YESTERDAY_CONTRIBUTION, 0);

    // set starting level
    uint32 start_level = getClass() != CLASS_DEATH_KNIGHT
                         ? sWorld.getConfig(CONFIG_UINT32_START_PLAYER_LEVEL)
                         : sWorld.getConfig(CONFIG_UINT32_START_HEROIC_PLAYER_LEVEL);

    if (GetSession()->GetSecurity() >= SEC_MODERATOR)
    {
        uint32 gm_level = sWorld.getConfig(CONFIG_UINT32_START_GM_LEVEL);
        if (gm_level > start_level)
        {
            start_level = gm_level;
        }
    }

    SetUInt32Value(UNIT_FIELD_LEVEL, start_level);

    InitRunes();

    SetUInt32Value(PLAYER_FIELD_COINAGE, sWorld.getConfig(CONFIG_UINT32_START_PLAYER_MONEY));
    SetHonorPoints(sWorld.getConfig(CONFIG_UINT32_START_HONOR_POINTS));
    SetArenaPoints(sWorld.getConfig(CONFIG_UINT32_START_ARENA_POINTS));

    // Initialize played time
    m_Last_tick = time(NULL);
    m_Played_time[PLAYED_TIME_TOTAL] = 0;
    m_Played_time[PLAYED_TIME_LEVEL] = 0;

    // Initialize base stats and related field values
    InitStatsForLevel();
    InitTaxiNodesForLevel();
    InitGlyphsForLevel();
    InitTalentForLevel();
    InitPrimaryProfessions(); // To max set before any spell added

    // Apply original stats mods before spell loading or item equipment
    UpdateMaxHealth(); // Update max Health (for add bonus from stamina)
    SetHealth(GetMaxHealth());

    if (GetPowerType() == POWER_MANA)
    {
        UpdateMaxPower(POWER_MANA); // Update max Mana (for add bonus from intellect)
        SetPower(POWER_MANA, GetMaxPower(POWER_MANA));
    }

    if (GetPowerType() != POWER_MANA)                       // hide additional mana bar if we have no mana
    {
        SetPower(POWER_MANA, 0);
        SetMaxPower(POWER_MANA, 0);
    }

    // original spells
    learnDefaultSpells();

    // Initialize action bar with default actions
    for (PlayerCreateInfoActions::const_iterator action_itr = info->action.begin(); action_itr != info->action.end(); ++action_itr)
    {
        addActionButton(0, action_itr->button, action_itr->action, action_itr->type);
    }

    // Initialize player's starting items
    uint32 raceClassGender = GetUInt32Value(UNIT_FIELD_BYTES_0) & 0x00FFFFFF;

    CharStartOutfitEntry const* oEntry = NULL;
    for (uint32 i = 1; i < sCharStartOutfitStore.GetNumRows(); ++i)
    {
        if (CharStartOutfitEntry const* entry = sCharStartOutfitStore.LookupEntry(i))
        {
            if (entry->RaceClassGender == raceClassGender)
            {
                oEntry = entry;
                break;
            }
        }
    }

    if (oEntry)
    {
        for (int j = 0; j < MAX_OUTFIT_ITEMS; ++j)
        {
            if (oEntry->ItemId[j] <= 0)
            {
                continue;
            }

            uint32 item_id = oEntry->ItemId[j];

            // Just skip, reported in ObjectMgr::LoadItemPrototypes
            ItemPrototype const* iProto = ObjectMgr::GetItemPrototype(item_id);
            if (!iProto)
            {
                continue;
            }

            // BuyCount by default
            int32 count = iProto->BuyCount;

            // Special amount for food/drink
            if (iProto->Class == ITEM_CLASS_CONSUMABLE && iProto->SubClass == ITEM_SUBCLASS_FOOD)
            {
                switch (iProto->Spells[0].SpellCategory)
                {
                    case 11:                                // food
                        count = getClass() == CLASS_DEATH_KNIGHT ? 10 : 4;
                        break;
                    case 59:                                // drink
                        count = 2;
                        break;
                }
                if (iProto->Stackable < count)
                {
                    count = iProto->Stackable;
                }
            }

            StoreNewItemInBestSlots(item_id, count);
        }
    }

    for (PlayerCreateInfoItems::const_iterator item_id_itr = info->item.begin(); item_id_itr != info->item.end(); ++item_id_itr)
    {
        StoreNewItemInBestSlots(item_id_itr->item_id, item_id_itr->item_amount);
    }

    // Equip bags and main-hand weapon
    // Second pass for not equipped items (offhand weapon/shield if it attempted to equip before main-hand weapon)
    // or ammo not equipped in special bag
    for (int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        if (Item* pItem = GetItemByPos(INVENTORY_SLOT_BAG_0, i))
        {
            uint16 eDest;
            // Equip offhand weapon/shield if it attempted to equip before main-hand weapon
            InventoryResult msg = CanEquipItem(NULL_SLOT, eDest, pItem, false);
            if (msg == EQUIP_ERR_OK)
            {
                RemoveItem(INVENTORY_SLOT_BAG_0, i, true);
                EquipItem(eDest, pItem, true);
            }
            // Move other items to more appropriate slots (ammo not equipped in special bag)
            else
            {
                ItemPosCountVec sDest;
                msg = CanStoreItem(NULL_BAG, NULL_SLOT, sDest, pItem, false);
                if (msg == EQUIP_ERR_OK)
                {
                    RemoveItem(INVENTORY_SLOT_BAG_0, i, true);
                    pItem = StoreItem(sDest, pItem, true);
                }

                // If this is ammo then use it
                msg = CanUseAmmo(pItem->GetEntry());
                if (msg == EQUIP_ERR_OK)
                {
                    SetAmmo(pItem->GetEntry());
                }
            }
        }
    }
    // All item positions resolved

    return true;
}

/**
 * @brief Equips or stores a newly created item in the best available slots.
 *
 * @param titem_id The item entry id.
 * @param titem_amount The amount to create.
 * @return true if all remaining items were equipped or stored; otherwise, false.
 */
bool Player::StoreNewItemInBestSlots(uint32 titem_id, uint32 titem_amount)
{
    DEBUG_LOG("STORAGE: Creating initial item, itemId = %u, count = %u", titem_id, titem_amount);

    // Attempt to equip the item one by one
    while (titem_amount > 0)
    {
        uint16 eDest;
        uint8 msg = CanEquipNewItem(NULL_SLOT, eDest, titem_id, false);
        if (msg != EQUIP_ERR_OK)
        {
            break;
        }

        // Equip the new item
        EquipNewItem(eDest, titem_id, true);
        AutoUnequipOffhandIfNeed();
        --titem_amount;
    }

    if (titem_amount == 0)
    {
        return true; // All items equipped
    }

    // Attempt to store the remaining items
    ItemPosCountVec sDest;
    // Store in the main bag to simplify the second pass (special bags may not be equipped yet)
    uint8 msg = CanStoreNewItem(INVENTORY_SLOT_BAG_0, NULL_SLOT, sDest, titem_id, titem_amount);
    if (msg == EQUIP_ERR_OK)
    {
        StoreNewItem(sDest, titem_id, true, Item::GenerateItemRandomPropertyId(titem_id));
        return true; // Items stored
    }

    // Item cannot be added
    sLog.outError("STORAGE: Can't equip or store initial item %u for race %u class %u , error msg = %u", titem_id, getRace(), getClass(), msg);
    return false;
}

// Helper function, mainly for script side, but can be used for simple tasks in MaNGOS as well
Item* Player::StoreNewItemInInventorySlot(uint32 itemEntry, uint32 amount)
{
    ItemPosCountVec vDest;

    uint8 msg = CanStoreNewItem(INVENTORY_SLOT_BAG_0, NULL_SLOT, vDest, itemEntry, amount);

    if (msg == EQUIP_ERR_OK)
    {
        if (Item* pItem = StoreNewItem(vDest, itemEntry, true, Item::GenerateItemRandomPropertyId(itemEntry)))
        {
            return pItem;
        }
    }

    return NULL;
}

/**
 * @brief Updates player state, timers, combat, saving, and delayed actions.
 *
 * @param update_diff The elapsed update time in milliseconds.
 * @param p_time The elapsed update time used by some player timers.
 */
void Player::Update(uint32 update_diff, uint32 p_time)
{
    // If the player is not in the world, return early
    if (!IsInWorld())
    {
        return;
    }

    // Remove failed timed Achievements
    GetAchievementMgr().DoFailedTimedAchievementCriterias();

    // Undelivered mail
    if (m_nextMailDelivereTime && m_nextMailDelivereTime <= time(NULL))
    {
        SendNewMail();
        ++unReadMails;

        // It will be recalculate at mailbox open (for unReadMails important non-0 until mailbox open, it also will be recalculated)
        m_nextMailDelivereTime = 0;
    }

    // Used to implement delayed far teleports
    SetCanDelayTeleport(true);
    Unit::Update(update_diff, p_time);
    SetCanDelayTeleport(false);

    // Periodic observer-side visibility maintenance.
    // The owner's visible set is otherwise refreshed only when the player moves
    // (Camera::UpdateVisibilityForOwner via OnRelocated), while an object moving
    // out of range only re-notifies observers still near that object. A
    // near-stationary player therefore never gets an out-of-range update for an
    // active object that walks away, leaving it frozen on the client. Sweep the
    // owner's visible set on an interval so out-of-range objects are dropped even
    // when the player stands still. Skipped mid-teleport (visibility is rebuilt
    // by the teleport path) and gated by config.
    if (World::GetVisibilityObserverSweepEnabled() && !IsBeingTeleported())
    {
        if (m_visibilityObserverSweepTimer <= update_diff)
        {
            m_visibilityObserverSweepTimer = World::GetVisibilityObserverSweepInterval();
            GetCamera().UpdateVisibilityForOwner();
        }
        else
        {
            m_visibilityObserverSweepTimer -= update_diff;
        }
    }

    // Update cinematic flyover while active, or while it owns pre-begin state
    // such as the DK early visibility lease.
    if (m_cinematicFlyover && m_cinematicFlyover->NeedsUpdate())
    {
        m_cinematicFlyover->Update(update_diff);
    }

    // Update player-only attacks
    if (uint32 ranged_att = getAttackTimer(RANGED_ATTACK))
    {
        setAttackTimer(RANGED_ATTACK, (update_diff >= ranged_att ? 0 : ranged_att - update_diff));
    }

    time_t now = time(NULL);

    // Update PvP flag
    UpdatePvPFlag(now);

    // Update contested PvP state
    UpdateContestedPvP(update_diff);

    // Update duel flag
    UpdateDuelFlag(now);

    // Check duel distance
    CheckDuelDistance(now);

    // Update AFK report
    UpdateAfkReport(now);

    // Update items with limited lifetime
    if (now > m_Last_tick)
    {
        UpdateItemDuration(uint32(now - m_Last_tick));
    }

    // Update timed quests
    if (!m_timedquests.empty())
    {
        QuestSet::iterator iter = m_timedquests.begin();
        while (iter != m_timedquests.end())
        {
            QuestStatusData& q_status = mQuestStatus[*iter];
            if (q_status.m_timer <= update_diff)
            {
                uint32 quest_id  = *iter;
                ++iter; // Current iter will be removed in FailQuest
                FailQuest(quest_id);
            }
            else
            {
                q_status.m_timer -= update_diff;
                if (q_status.uState != QUEST_NEW)
                {
                    q_status.uState = QUEST_CHANGED;
                }
                ++iter;
            }
        }
    }

    // Update melee attacking state
    if (hasUnitState(UNIT_STAT_MELEE_ATTACKING))
    {
        UpdateMeleeAttackingState();

        Unit* pVictim = getVictim();
        if (pVictim && !IsNonMeleeSpellCasted(false))
        {
            Player* vOwner = pVictim->GetCharmerOrOwnerPlayerOrPlayerItself();
            if (vOwner && vOwner->IsPvP() && !IsInDuelWith(vOwner))
            {
                UpdatePvP(true);
                RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
            }
        }
    }

    // Speed collect rest bonus (section/in hour)
    if (HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING))
    {
        if (GetTimeInnEnter() > 0) // Freeze update
        {
            time_t time_inn = now - GetTimeInnEnter();
            if (time_inn >= 10) // Freeze update
            {
                SetRestBonus(GetRestBonus() + ComputeRest(time_inn));
                UpdateInnerTime(now);
            }
        }
    }

    // Update regeneration timer
    if (m_regenTimer)
    {
        if (update_diff >= m_regenTimer)
        {
            m_regenTimer = 0;
        }
        else
        {
            m_regenTimer -= update_diff;
        }
    }

    // Update position status timer
    if (m_positionStatusUpdateTimer)
    {
        if (update_diff >= m_positionStatusUpdateTimer)
        {
            m_positionStatusUpdateTimer = 0;
        }
        else
        {
            m_positionStatusUpdateTimer -= update_diff;
        }
    }

    // Update weapon change timer
    if (m_weaponChangeTimer > 0)
    {
        if (update_diff >= m_weaponChangeTimer)
        {
            m_weaponChangeTimer = 0;
        }
        else
        {
            m_weaponChangeTimer -= update_diff;
        }
    }

    // Update zone timer
    if (m_zoneUpdateTimer > 0)
    {
        if (update_diff >= m_zoneUpdateTimer)
        {
            uint32 newzone, newarea;
            GetZoneAndAreaId(newzone, newarea);

            if (m_zoneUpdateId != newzone)
            {
                UpdateZone(newzone, newarea); // Also update area
            }
            else
            {
                // Use area updates as well
                // Needed for free-for-all arenas, for example
                if (m_areaUpdateId != newarea)
                {
                    UpdateArea(newarea);
                }

                m_zoneUpdateTimer = ZONE_UPDATE_INTERVAL;
            }
        }
        else
        {
            m_zoneUpdateTimer -= update_diff;
        }
    }

    // Update time sync timer
    if (m_timeSyncTimer > 0)
    {
        if (update_diff >= m_timeSyncTimer)
        {
            SendTimeSync();
        }
        else
        {
            m_timeSyncTimer -= update_diff;
        }
    }

    // Regenerate all if the player is alive
    if (IsAlive())
    {
        // if no longer casting, set regen power as soon as it is up.
        if (!IsUnderLastManaUseEffect() && !HasAuraType(SPELL_AURA_STOP_NATURAL_MANA_REGEN))
        {
            SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_REGENERATE_POWER);
        }

        if (!m_regenTimer)
        {
            RegenerateAll();
        }
    }

    if (!IsAlive() && !HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST) && GetDeathState() != GHOULED)
    {
        SetHealth(0);
    }

    // Handle player death
    if (m_deathState == JUST_DIED)
    {
        KillPlayer();
    }

    // Handle periodic saving
    if (m_nextSave > 0)
    {
        if (update_diff >= m_nextSave)
        {
            // m_nextSave reset in SaveToDB call
            // Used by Eluna
#ifdef ENABLE_ELUNA
            if (Eluna* e = GetEluna())
            {
                e->OnSave(this);
            }
#endif /* ENABLE_ELUNA */
            SaveToDB();
            DETAIL_LOG("Player '%s' (GUID: %u) saved", GetName(), GetGUIDLow());
        }
        else
        {
            m_nextSave -= update_diff;
        }
    }

    // Handle water/drowning
    HandleDrowning(update_diff);

    // Handle detect stealth players
    if (m_DetectInvTimer > 0)
    {
        if (update_diff >= m_DetectInvTimer)
        {
            HandleStealthedUnitsDetection();
            m_DetectInvTimer = 3000;
        }
        else
        {
            m_DetectInvTimer -= update_diff;
        }
    }

    // Update played time
    if (now > m_Last_tick)
    {
        uint32 elapsed = uint32(now - m_Last_tick);
        m_Played_time[PLAYED_TIME_TOTAL] += elapsed; // Total played time
        m_Played_time[PLAYED_TIME_LEVEL] += elapsed; // Level played time
        m_Last_tick = now;
    }

    if (GetDrunkValue())
    {
        m_drunkTimer += update_diff;

        if (m_drunkTimer > 9 * IN_MILLISECONDS)
        {
            HandleSobering();
        }
    }

    // Not auto-free ghost from body in instances; also check for resurrection prevention
    if (m_deathTimer > 0  && !GetMap()->Instanceable() && GetDeathState() != GHOULED && !HasAuraType(SPELL_AURA_PREVENT_RESURRECTION))
    {
        if (p_time >= m_deathTimer)
        {
            m_deathTimer = 0;
            BuildPlayerRepop();
            RepopAtGraveyard();
        }
        else
        {
            m_deathTimer -= p_time;
        }
    }

    // Update enchant time
    UpdateEnchantTime(update_diff);

    // Update homebind time
    UpdateHomebindTime(update_diff);

    // Group update
    SendUpdateToOutOfRangeGroupMembers();

    // Handle pet unsummoning if out of range
    Pet* pet = GetPet();
    if (pet && !pet->IsWithinDistInMap(this, GetMap()->GetVisibilityDistance()) && (GetCharmGuid() && (pet->GetObjectGuid() != GetCharmGuid())))
    {
        pet->Unsummon(PET_SAVE_REAGENTS, this);
    }

    // Handle delayed teleport
    if (IsHasDelayedTeleport())
    {
        TeleportTo(m_teleport_dest, m_teleport_options);
    }
}

/**
 * @brief Changes the player's death state and handles player-specific death logic.
 *
 * @param s The new death state.
 */
void Player::SetDeathState(DeathState s)
{
    uint32 ressSpellId = 0;

    bool cur = IsAlive();

    if (s == JUST_DIED && cur)
    {
        // drunken state is cleared on death
        SetDrunkValue(0);
        // lost combo points at any target (targeted combo points clear in Unit::SetDeathState)
        ClearComboPoints();

        clearResurrectRequestData();

        // remove form before other mods to prevent incorrect stats calculation
        RemoveSpellsCausingAura(SPELL_AURA_MOD_SHAPESHIFT);

        // FIXME: is pet dismissed at dying or releasing spirit? if second, add SetDeathState(DEAD) to HandleRepopRequestOpcode and define pet unsummon here with (s == DEAD)
        RemovePet(PET_SAVE_REAGENTS);

        // save value before aura remove in Unit::SetDeathState
        ressSpellId = GetUInt32Value(PLAYER_SELF_RES_SPELL);

        // passive spell
        if (!ressSpellId)
        {
            ressSpellId = GetResurrectionSpellId();
        }

        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_DEATH_AT_MAP, 1);
        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_DEATH, 1);
        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_DEATH_IN_DUNGEON, 1);

        if (InstanceData* mapInstance = GetInstanceData())
        {
            mapInstance->OnPlayerDeath(this);
        }
    }

    Unit::SetDeathState(s);

    // restore resurrection spell id for player after aura remove
    if (s == JUST_DIED && cur && ressSpellId)
    {
        SetUInt32Value(PLAYER_SELF_RES_SPELL, ressSpellId);
    }

    if (IsAlive() && !cur)
    {
        // clear aura case after resurrection by another way (spells will be applied before next death)
        SetUInt32Value(PLAYER_SELF_RES_SPELL, 0);

        // restore default warrior stance
        if (getClass() == CLASS_WARRIOR)
        {
            CastSpell(this, SPELL_ID_PASSIVE_BATTLE_STANCE, true);
        }
    }
}

/**
 * @brief Builds character-enumeration data for the character selection screen.
 *
 * @param result The database row for the character.
 * @param p_data The packet being populated.
 * @return true if the enum data was built successfully; otherwise, false.
 */
bool Player::BuildEnumData(QueryResult* result, WorldPacket* p_data)
{
    //             0               1                2                3                 4                  5                       6                        7
    //    "SELECT characters.guid, characters.name, characters.race, characters.class, characters.gender, characters.playerBytes, characters.playerBytes2, characters.level, "
    //     8                9               10                     11                     12                     13                    14
    //    "characters.zone, characters.map, characters.position_x, characters.position_y, characters.position_z, guild_member.guildid, characters.playerFlags, "
    //    15                    16                   17                     18                   19                         20
    //    "characters.at_login, character_pet.entry, character_pet.modelid, character_pet.level, characters.equipmentCache, character_declinedname.genitive "

    Field* fields = result->Fetch();

    uint32 guid = fields[0].GetUInt32();
    uint8 pRace = fields[2].GetUInt8();
    uint8 pClass = fields[3].GetUInt8();

    PlayerInfo const* info = sObjectMgr.GetPlayerInfo(pRace, pClass);
    if (!info)
    {
        sLog.outError("Player %u has incorrect race/class pair. Don't build enum.", guid);
        return false;
    }

    *p_data << ObjectGuid(HIGHGUID_PLAYER, guid);
    *p_data << fields[1].GetString();                       // name
    *p_data << uint8(pRace);                                // race
    *p_data << uint8(pClass);                               // class
    *p_data << uint8(fields[4].GetUInt8());                 // gender

    uint32 playerBytes = fields[5].GetUInt32();
    *p_data << uint8(playerBytes);                          // skin
    *p_data << uint8(playerBytes >> 8);                     // face
    *p_data << uint8(playerBytes >> 16);                    // hair style
    *p_data << uint8(playerBytes >> 24);                    // hair color

    uint32 playerBytes2 = fields[6].GetUInt32();
    *p_data << uint8(playerBytes2 & 0xFF);                  // facial hair

    *p_data << uint8(fields[7].GetUInt8());                 // level
    *p_data << uint32(fields[8].GetUInt32());               // zone
    *p_data << uint32(fields[9].GetUInt32());               // map

    *p_data << fields[10].GetFloat();                       // x
    *p_data << fields[11].GetFloat();                       // y
    *p_data << fields[12].GetFloat();                       // z

    *p_data << uint32(fields[13].GetUInt32());              // guild id

    uint32 char_flags = 0;
    uint32 playerFlags = fields[14].GetUInt32();
    uint32 atLoginFlags = fields[15].GetUInt32();
    if (playerFlags & PLAYER_FLAGS_HIDE_HELM)
    {
        char_flags |= CHARACTER_FLAG_HIDE_HELM;
    }
    if (playerFlags & PLAYER_FLAGS_HIDE_CLOAK)
    {
        char_flags |= CHARACTER_FLAG_HIDE_CLOAK;
    }
    if (playerFlags & PLAYER_FLAGS_GHOST)
    {
        char_flags |= CHARACTER_FLAG_GHOST;
    }
    if (atLoginFlags & AT_LOGIN_RENAME)
    {
        char_flags |= CHARACTER_FLAG_RENAME;
    }
    if (sWorld.getConfig(CONFIG_BOOL_DECLINED_NAMES_USED))
    {
        if (!fields[20].GetCppString().empty())
        {
            char_flags |= CHARACTER_FLAG_DECLINED;
        }
    }
    else
    {
        char_flags |= CHARACTER_FLAG_DECLINED;
    }

    *p_data << uint32(char_flags);                          // character flags
    // character customize flags
    *p_data << uint32(atLoginFlags & AT_LOGIN_CUSTOMIZE ? CHAR_CUSTOMIZE_FLAG_CUSTOMIZE : CHAR_CUSTOMIZE_FLAG_NONE);
    // First login
    *p_data << uint8(atLoginFlags & AT_LOGIN_FIRST ? 1 : 0);

    // Pets info
    {
        uint32 petDisplayId = 0;
        uint32 petLevel   = 0;
        uint32 petFamily  = 0;

        // show pet at selection character in character list only for non-ghost character
        if (result && !(playerFlags & PLAYER_FLAGS_GHOST) && (pClass == CLASS_WARLOCK || pClass == CLASS_HUNTER || pClass == CLASS_DEATH_KNIGHT))
        {
            uint32 entry = fields[16].GetUInt32();
            CreatureInfo const* cInfo = sCreatureStorage.LookupEntry<CreatureInfo>(entry);
            if (cInfo)
            {
                petDisplayId = fields[17].GetUInt32();
                petLevel     = fields[18].GetUInt32();
                petFamily    = cInfo->Family;
            }
        }

        *p_data << uint32(petDisplayId);
        *p_data << uint32(petLevel);
        *p_data << uint32(petFamily);
    }


    Tokens data = StrSplit(fields[19].GetCppString(), " ");
    for (uint8 slot = 0; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        uint32 visualbase = slot * 2;                       // entry, perm ench., temp ench.
        uint32 item_id = GetUInt32ValueFromArray(data, visualbase);
        const ItemPrototype* proto = ObjectMgr::GetItemPrototype(item_id);
        if (!proto)
        {
            *p_data << uint32(0);
            *p_data << uint8(0);
            *p_data << uint32(0);
            continue;
        }

        SpellItemEnchantmentEntry const* enchant = NULL;

        uint32 enchants = GetUInt32ValueFromArray(data, visualbase + 1);
        for (uint8 enchantSlot = PERM_ENCHANTMENT_SLOT; enchantSlot <= TEMP_ENCHANTMENT_SLOT; ++enchantSlot)
        {
            // values stored in 2 uint16
            uint32 enchantId = 0x0000FFFF & (enchants >> enchantSlot * 16);
            if (!enchantId)
            {
                continue;
            }

            if ((enchant = sSpellItemEnchantmentStore.LookupEntry(enchantId)))
            {
                break;
            }
        }

        *p_data << uint32(proto->DisplayInfoID);
        *p_data << uint8(proto->InventoryType);
        *p_data << uint32(enchant ? enchant->aura_id : 0);
    }

    *p_data << uint32(0);                                   // first bag display id
    *p_data << uint8(0);                                    // first bag inventory type
    *p_data << uint32(0);                                   // enchant?
    *p_data << uint32(0);                                   // bag 2 display id
    *p_data << uint8(0);                                    // bag 2 inventory type
    *p_data << uint32(0);                                   // enchant?
    *p_data << uint32(0);                                   // bag 3 display id
    *p_data << uint8(0);                                    // bag 3 inventory type
    *p_data << uint32(0);                                   // enchant?
    *p_data << uint32(0);                                   // bag 4 display id
    *p_data << uint8(0);                                    // bag 4 inventory type
    *p_data << uint32(0);                                   // enchant?

    return true;
}

/**
 * @brief Toggles AFK status and leaves battlegrounds when entering AFK.
 */
void Player::ToggleAFK()
{
    ToggleFlag(PLAYER_FLAGS, PLAYER_FLAGS_AFK);

    // afk player not allowed in battleground
    if (isAFK() && InBattleGround() && !InArena())
    {
        LeaveBattleground();
    }
}

/**
 * @brief Toggles DND status.
 */
void Player::ToggleDND()
{
    ToggleFlag(PLAYER_FLAGS, PLAYER_FLAGS_DND);
}

/**
 * @brief Gets the active chat tag displayed for the player.
 *
 * @return The chat tag flags for GM, AFK, DND, or none.
 */
ChatTagFlags Player::GetChatTag() const
{
    ChatTagFlags tag = CHAT_TAG_NONE;

    if (isAFK())
    {
        tag |= CHAT_TAG_AFK;
    }
    if (isDND())
    {
        tag |= CHAT_TAG_DND;
    }
    if (isGMChat())
    {
        tag |= CHAT_TAG_GM;
    }
    if (HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_COMMENTATOR))
    {
        tag |= CHAT_TAG_COM;
    }
    if (HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_DEVELOPER))
    {
        tag |= CHAT_TAG_DEV;
    }

    return tag;
}

/**
 * @brief Teleports the player to a target location, handling near and far cases.
 *
 * @param mapid The destination map id.
 * @param x The destination x coordinate.
 * @param y The destination y coordinate.
 * @param z The destination z coordinate.
 * @param orientation The destination orientation.
 * @param options Teleport option flags.
 * @param allowNoDelay true to bypass delayed-teleport deferral when possible.
 * @return true if teleport setup succeeded; otherwise, false.
 */
bool Player::TeleportTo(uint32 mapid, float x, float y, float z, float orientation, uint32 options /*=0*/, AreaTrigger const* at /*=NULL*/)
{
    // Stop cinematic flyover on teleport (body and any early visibility lease
    // are map-bound).
    if (m_cinematicFlyover)
    {
        m_cinematicFlyover->Stop();
    }

    if (!MapManager::IsValidMapCoord(mapid, x, y, z, orientation))
    {
        sLog.outError("TeleportTo: invalid map %d or absent instance template.", mapid);
        return false;
    }

    MapEntry const* mEntry = sMapStore.LookupEntry(mapid);  // Validity checked in IsValidMapCoord

    if (!isGameMaster() && DisableMgr::IsDisabledFor(DISABLE_TYPE_MAP, mapid, this))
    {
        sLog.outDebug("Player (GUID: %u, name: %s) tried to enter a forbidden map %u", GetGUIDLow(), GetName(), mapid);
        SendTransferAbortedByLockStatus(mEntry, AREA_LOCKSTATUS_NOT_ALLOWED);
        return false;
    }

    // preparing unsummon pet if lost (we must get pet before teleportation or will not find it later)
    Pet* pet = GetPet();

    // don't let enter battlegrounds without assigned battleground id (for example through areatrigger)...
    // don't let gm level > 1 either
    if (!InBattleGround() && mEntry->IsBattleGroundOrArena())
    {
        return false;
    }

    // Get MapEntrance trigger if teleport to other -nonBG- map
    bool assignedAreaTrigger = false;
    if (GetMapId() != mapid && !mEntry->IsBattleGroundOrArena() && !at)
    {
        at = sObjectMgr.GetMapEntranceTrigger(mapid);
        assignedAreaTrigger = true;
    }

    // Check requirements for teleport
    if (at)
    {
        uint32 miscRequirement = 0;
        AreaLockStatus lockStatus = GetAreaTriggerLockStatus(at, GetDifficulty(mEntry->IsRaid()), miscRequirement);
        if (lockStatus != AREA_LOCKSTATUS_OK)
        {
            // Teleport not requested by area-trigger
            // TODO - Assume a player with expansion 0 travels from BootyBay to Ratched, and he is attempted to be teleported to outlands
            //        then he will repop near BootyBay instead of normally continuing his journey
            // This code is probably added to catch passengers on ships to northrend who shouldn't go there
            if (lockStatus == AREA_LOCKSTATUS_INSUFFICIENT_EXPANSION && !assignedAreaTrigger && GetTransport())
            {
                RepopAtGraveyard();                         // Teleport to near graveyard if on transport, looks blizz like :)
            }

            SendTransferAbortedByLockStatus(mEntry, lockStatus, miscRequirement);
            return false;
        }
    }

    if (Group* grp = GetGroup())                            // TODO: Verify that this is correct place
    {
        grp->SetPlayerMap(GetObjectGuid(), mapid);
    }

    // if we were on a transport, leave
    if (!(options & TELE_TO_NOT_LEAVE_TRANSPORT) && m_transport)
    {
        m_transport->RemovePassenger(this);
        m_transport = NULL;
        m_movementInfo.ClearTransportData();
    }

    // The player was ported to another map and looses the duel immediately.
    // We have to perform this check before the teleport, otherwise the
    // ObjectAccessor won't find the flag.
    if (duel && GetMapId() != mapid)
        if (GetMap()->GetGameObject(GetGuidValue(PLAYER_DUEL_ARBITER)))
        {
            DuelComplete(DUEL_FLED);
        }

    // reset movement flags at teleport, because player will continue move with these flags after teleport
    m_movementInfo.SetMovementFlags(MOVEFLAG_NONE);
    DisableSpline();

    if ((GetMapId() == mapid) && (!m_transport))            // TODO the !m_transport might have unexpected effects when teleporting from transport to other place on same map
    {
        // lets reset far teleport flag if it wasn't reset during chained teleports
        SetSemaphoreTeleportFar(false);
        // setup delayed teleport flag
        // if teleport spell is casted in Unit::Update() func
        // then we need to delay it until update process will be finished
        if (SetDelayedTeleportFlagIfCan())
        {
            SetSemaphoreTeleportNear(true);
            // lets save teleport destination for player
            m_teleport_dest = WorldLocation(mapid, x, y, z, orientation);
            m_teleport_options = options;
            return true;
        }

        if (!(options & TELE_TO_NOT_UNSUMMON_PET))
        {
            // same map, only remove pet if out of range for new position
            if (pet && !pet->IsWithinDist3d(x, y, z, GetMap()->GetVisibilityDistance()))
            {
                UnsummonPetTemporaryIfAny();
            }
        }

        if (!(options & TELE_TO_NOT_LEAVE_COMBAT))
        {
            CombatStop();
        }

        // this will be used instead of the current location in SaveToDB
        m_teleport_dest = WorldLocation(mapid, x, y, z, orientation);
        SetFallInformation(0, z);

        // code for finish transfer called in WorldSession::HandleMovementOpcodes()
        // at client packet MSG_MOVE_TELEPORT_ACK
        SetSemaphoreTeleportNear(true);
        // near teleport, triggering send MSG_MOVE_TELEPORT_ACK from client at landing
        if (!GetSession()->PlayerLogout())
        {
            WorldPacket data;
            BuildTeleportAckMsg(data, x, y, z, orientation);
            GetSession()->SendPacket(&data);
        }
    }
    else
    {
        // far teleport to another map
        Map* oldmap = IsInWorld() ? GetMap() : NULL;
        // check if we can enter before stopping combat / removing pet / totems / interrupting spells

        // If the map is not created, assume it is possible to enter it.
        // It will be created in the WorldPortAck.
        DungeonPersistentState* state = GetBoundInstanceSaveForSelfOrGroup(mapid);
        Map* map = sMapMgr.FindMap(mapid, state ? state->GetInstanceId() : 0);
        if (!map || map->CanEnter(this))
        {
            // lets reset near teleport flag if it wasn't reset during chained teleports
            SetSemaphoreTeleportNear(false);
            // setup delayed teleport flag
            // if teleport spell is casted in Unit::Update() func
            // then we need to delay it until update process will be finished
            if (SetDelayedTeleportFlagIfCan())
            {
                SetSemaphoreTeleportFar(true);
                // lets save teleport destination for player
                m_teleport_dest = WorldLocation(mapid, x, y, z, orientation);
                m_teleport_options = options;
                return true;
            }

            SetSelectionGuid(ObjectGuid());

            CombatStop();

            ResetContestedPvP();

            // remove player from battleground on far teleport (when changing maps)
            if (BattleGround const* bg = GetBattleGround())
            {
                // Note: at battleground join battleground id set before teleport
                // and we already will found "current" battleground
                // just need check that this is targeted map or leave
                if (bg->GetMapId() != mapid)
                {
                    LeaveBattleground(false); // don't teleport to entry point
                }
            }

            // remove pet on map change
            if (pet)
            {
                UnsummonPetTemporaryIfAny();
            }

            // remove vehicle accessories on map change
            if (IsVehicle())
            {
                GetVehicleInfo()->RemoveAccessoriesFromMap();
            }

            // remove all dyn objects
            RemoveAllDynObjects();

            // stop spellcasting
            // not attempt interrupt teleportation spell at caster teleport
            if (!(options & TELE_TO_SPELL))
                if (IsNonMeleeSpellCasted(true))
                {
                    InterruptNonMeleeSpells(true);
                }

            // remove auras before removing from map...
            RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_CHANGE_MAP | AURA_INTERRUPT_FLAG_MOVE | AURA_INTERRUPT_FLAG_TURNING);

            if (!GetSession()->PlayerLogout())
            {
                // send transfer packet to display load screen
                WorldPacket data(SMSG_TRANSFER_PENDING, (4 + 4 + 4));
                data << uint32(mapid);
                if (m_transport)
                {
                    data << uint32(m_transport->GetEntry());
                    data << uint32(GetMapId());
                }
                GetSession()->SendPacket(&data);
            }

            // remove from old map now
            if (oldmap)
            {
                oldmap->Remove(this, false);
            }

            // new final coordinates
            float final_x = x;
            float final_y = y;
            float final_z = z;
            float final_o = orientation;

            Position const* transportPosition = m_movementInfo.GetTransportPos();

            if (m_transport)
            {
                final_x += transportPosition->x;
                final_y += transportPosition->y;
                final_z += transportPosition->z;
                final_o += transportPosition->o;
            }

            m_teleport_dest = WorldLocation(mapid, final_x, final_y, final_z, final_o);
            SetFallInformation(0, final_z);
            // if the player is saved before worldport ack (at logout for example)
            // this will be used instead of the current location in SaveToDB

            // move packet sent by client always after far teleport
            // code for finish transfer to new map called in WorldSession::HandleMoveWorldportAckOpcode at client packet
            SetSemaphoreTeleportFar(true);

            if (!GetSession()->PlayerLogout())
            {
                // transfer finished, inform client to start load
                WorldPacket data(SMSG_NEW_WORLD, (20));
                data << uint32(mapid);
                if (m_transport)
                {
                    data << float(transportPosition->x);
                    data << float(transportPosition->y);
                    data << float(transportPosition->z);
                    data << float(transportPosition->o);
                }
                else
                {
                    data << float(final_x);
                    data << float(final_y);
                    data << float(final_z);
                    data << float(final_o);
                }

                GetSession()->SendPacket(&data);
                SendSavedInstances();
            }
        }
        else                                                // !map->CanEnter(this)
        {
            return false;
        }
    }
    return true;
}


/**
 * @brief Executes queued delayed player operations.
 */
void Player::ProcessDelayedOperations()
{
    if (m_DelayedOperations == 0)
    {
        return;
    }

    if (m_DelayedOperations & DELAYED_RESURRECT_PLAYER)
    {
        ResurrectPlayer(0.0f, false);

        if (GetMaxHealth() > m_resurrectHealth)
        {
            SetHealth(m_resurrectHealth);
        }
        else
        {
            SetHealth(GetMaxHealth());
        }

        if (GetMaxPower(POWER_MANA) > m_resurrectMana)
        {
            SetPower(POWER_MANA, m_resurrectMana);
        }
        else
        {
            SetPower(POWER_MANA, GetMaxPower(POWER_MANA));
        }

        SetPower(POWER_RAGE, 0);
        SetPower(POWER_ENERGY, GetMaxPower(POWER_ENERGY));

        SpawnCorpseBones();
    }

    if (m_DelayedOperations & DELAYED_SAVE_PLAYER)
    {
        SaveToDB();
    }

    if (m_DelayedOperations & DELAYED_SPELL_CAST_DESERTER)
    {
        CastSpell(this, 26013, true);               // Deserter
    }

    if (m_DelayedOperations & DELAYED_BG_MOUNT_RESTORE)
    {
        if (m_bgData.mountSpell)
        {
            CastSpell(this, m_bgData.mountSpell, true);
            m_bgData.mountSpell = 0;
        }
    }

    if (m_DelayedOperations & DELAYED_BG_TAXI_RESTORE)
    {
        if (m_bgData.HasTaxiPath())
        {
            m_taxi.AddTaxiDestination(m_bgData.taxiPath[0]);
            m_taxi.AddTaxiDestination(m_bgData.taxiPath[1]);
            m_bgData.ClearTaxiPath();

            ContinueTaxiFlight();
        }
    }

    // we have executed ALL delayed ops, so clear the flag
    m_DelayedOperations = 0;
}

/**
 * @brief Adds the player and equipped items to the world.
 */
void Player::AddToWorld()
{
    ///- Do not add/remove the player from the object storage
    ///- It will crash when updating the ObjectAccessor
    ///- The player should only be added when logging in
    Unit::AddToWorld();

    for (int i = PLAYER_SLOT_START; i < PLAYER_SLOT_END; ++i)
    {
        if (m_items[i])
        {
            m_items[i]->AddToWorld();
        }
    }
}

/**
 * @brief Removes the player and equipped items from the world.
 */
void Player::RemoveFromWorld()
{
    for (int i = PLAYER_SLOT_START; i < PLAYER_SLOT_END; ++i)
    {
        if (m_items[i])
        {
            m_items[i]->RemoveFromWorld();
        }
    }

    ///- Do not add/remove the player from the object storage
    ///- It will crash when updating the ObjectAccessor
    ///- The player should only be removed when logging out
    if (IsInWorld())
    {
        GetCamera().ResetView();
    }

    Unit::RemoveFromWorld();
}

/**
 * @brief Awards rage generated from dealing or receiving damage.
 *
 * @param damage The raw damage amount used for rage generation.
 * @param attacker True when the player dealt the damage; false when the player received it.
 */
void Player::RewardRage(uint32 damage, uint32 weaponSpeedHitFactor, bool attacker)
{
    float addRage;

    float rageconversion = float((0.0091107836 * getLevel() * getLevel()) + 3.225598133 * getLevel()) + 4.2652911f;

    if (attacker)
    {
        addRage = ((damage / rageconversion * 7.5f + weaponSpeedHitFactor) / 2.0f);

        // talent who gave more rage on attack
        addRage *= 1.0f + GetTotalAuraModifier(SPELL_AURA_MOD_RAGE_FROM_DAMAGE_DEALT) / 100.0f;
    }
    else
    {
        addRage = damage / rageconversion * 2.5f;

        // Berserker Rage effect
        if (HasAura(18499, EFFECT_INDEX_0))
        {
            addRage *= 1.3f;
        }
    }

    addRage *= sWorld.getConfig(CONFIG_FLOAT_RATE_POWER_RAGE_INCOME);

    ModifyPower(POWER_RAGE, uint32(addRage * 10));
}

/**
 * @brief Regenerates the player's health and power resources for the current tick.
 */
void Player::RegenerateAll(uint32 diff)
{
    // Not in combat or they have regeneration
    if (!IsInCombat() || HasAuraType(SPELL_AURA_MOD_REGEN_DURING_COMBAT) ||
        HasAuraType(SPELL_AURA_MOD_HEALTH_REGEN_IN_COMBAT) || IsPolymorphed())
    {
        RegenerateHealth(diff);
        if (!IsInCombat() && !HasAuraType(SPELL_AURA_INTERRUPT_REGEN))
        {
            Regenerate(POWER_RAGE, diff);
            if (getClass() == CLASS_DEATH_KNIGHT)
            {
                Regenerate(POWER_RUNIC_POWER, diff);
            }
        }
    }

    Regenerate(POWER_ENERGY, diff);

    Regenerate(POWER_MANA, diff);

    if (getClass() == CLASS_DEATH_KNIGHT)
    {
        Regenerate(POWER_RUNE, diff);
    }

    m_regenTimer = REGEN_TIME_FULL;
}

/**
 * @brief Regenerates or decays a specific player power type.
 *
 * @param power The power type to update.
 */
// diff contains the time in milliseconds since last regen.
void Player::Regenerate(Powers power, uint32 diff)
{
    uint32 curValue = GetPower(power);
    uint32 maxValue = GetMaxPower(power);

    float addvalue = 0.0f;

    switch (power)
    {
        case POWER_MANA:
        {
            if (HasAuraType(SPELL_AURA_STOP_NATURAL_MANA_REGEN))
            {
                break;
            }
            bool recentCast = IsUnderLastManaUseEffect();
            float ManaIncreaseRate = sWorld.getConfig(CONFIG_FLOAT_RATE_POWER_MANA);
            if (recentCast)
            {
                // Mangos Updates Mana in intervals of 2s, which is correct
                addvalue = GetFloatValue(UNIT_FIELD_POWER_REGEN_INTERRUPTED_FLAT_MODIFIER) *  ManaIncreaseRate * 2.00f;
            }
            else
            {
                addvalue = GetFloatValue(UNIT_FIELD_POWER_REGEN_FLAT_MODIFIER) * ManaIncreaseRate * 2.00f;
            }
        }   break;
        case POWER_RAGE:                                    // Regenerate rage
        {
            float RageDecreaseRate = sWorld.getConfig(CONFIG_FLOAT_RATE_POWER_RAGE_LOSS);
            addvalue = 20 * RageDecreaseRate;               // 2 rage by tick (= 2 seconds => 1 rage/sec)
        }   break;
        case POWER_ENERGY:                                  // Regenerate energy (rogue)
        {
            float EnergyRate = sWorld.getConfig(CONFIG_FLOAT_RATE_POWER_ENERGY);
            addvalue = 20 * EnergyRate;
            break;
        }
        case POWER_RUNIC_POWER:
        {
            float RunicPowerDecreaseRate = sWorld.getConfig(CONFIG_FLOAT_RATE_POWER_RUNICPOWER_LOSS);
            addvalue = 30 * RunicPowerDecreaseRate;         // 3 RunicPower by tick
            break;
        }
        case POWER_RUNE:
        {
            if (getClass() != CLASS_DEATH_KNIGHT)
            {
                break;
            }

            for (uint32 rune = 0; rune < MAX_RUNES; ++rune)
            {
                if (uint16 cd = GetRuneCooldown(rune))      // if we have cooldown, reduce it...
                {
                    uint32 cd_diff = diff;
                    AuraList const& ModPowerRegenPCTAuras = GetAurasByType(SPELL_AURA_MOD_POWER_REGEN_PERCENT);
                    for (AuraList::const_iterator i = ModPowerRegenPCTAuras.begin(); i != ModPowerRegenPCTAuras.end(); ++i)
                    {
                        if ((*i)->GetModifier()->m_miscvalue == int32(power) && (*i)->GetMiscBValue() == GetCurrentRune(rune))
                        {
                            cd_diff = cd_diff * ((*i)->GetModifier()->m_amount + 100) / 100;
                        }
                    }

                    SetRuneCooldown(rune, (cd < cd_diff) ? 0 : cd - cd_diff);
                }
            }
            break;
        }
        case POWER_FOCUS:
        case POWER_HAPPINESS:
        case POWER_HEALTH:
            break;
        default:
            break;
    }

    // Mana regen calculated in Player::UpdateManaRegen()
    // Exist only for POWER_MANA, POWER_ENERGY, POWER_FOCUS auras
    if (power != POWER_MANA)
    {
        AuraList const& ModPowerRegenPCTAuras = GetAurasByType(SPELL_AURA_MOD_POWER_REGEN_PERCENT);
        for (AuraList::const_iterator i = ModPowerRegenPCTAuras.begin(); i != ModPowerRegenPCTAuras.end(); ++i)
        {
            if ((*i)->GetModifier()->m_miscvalue == int32(power))
            {
                addvalue *= ((*i)->GetModifier()->m_amount + 100) / 100.0f;
            }
        }
    }

    // addvalue computed on a 2sec basis. => update to diff time
    addvalue *= float(diff) / REGEN_TIME_FULL;

    if (power != POWER_RAGE && power != POWER_RUNIC_POWER)
    {
        curValue += uint32(addvalue);
        if (curValue > maxValue)
        {
            curValue = maxValue;
        }
    }
    else
    {
        if (curValue <= uint32(addvalue))
        {
            curValue = 0;
        }
        else
        {
            curValue -= uint32(addvalue);
        }
    }

    SetPower(power, curValue);
}

/**
 * @brief Regenerates the player's health based on state, auras, and rates.
 */
void Player::RegenerateHealth(uint32 diff)
{
    uint32 curValue = GetHealth();
    uint32 maxValue = GetMaxHealth();

    if (curValue >= maxValue)
    {
        return;
    }

    float HealthIncreaseRate = sWorld.getConfig(CONFIG_FLOAT_RATE_HEALTH);

    float addvalue = 0.0f;

    // polymorphed case
    if (IsPolymorphed())
    {
        addvalue = (float)GetMaxHealth() / 3;
    }
    // normal regen case (maybe partly in combat case)
    else if (!IsInCombat() || HasAuraType(SPELL_AURA_MOD_REGEN_DURING_COMBAT))
    {
        addvalue = OCTRegenHPPerSpirit() * HealthIncreaseRate;
        if (!IsInCombat())
        {
            AuraList const& mModHealthRegenPct = GetAurasByType(SPELL_AURA_MOD_HEALTH_REGEN_PERCENT);
            for (AuraList::const_iterator i = mModHealthRegenPct.begin(); i != mModHealthRegenPct.end(); ++i)
            {
                addvalue *= (100.0f + (*i)->GetModifier()->m_amount) / 100.0f;
            }
        }
        else if (HasAuraType(SPELL_AURA_MOD_REGEN_DURING_COMBAT))
        {
            addvalue *= GetTotalAuraModifier(SPELL_AURA_MOD_REGEN_DURING_COMBAT) / 100.0f;
        }

        if (!IsStandState())
        {
            addvalue *= 1.5;
        }
    }

    // always regeneration bonus (including combat)
    addvalue += GetTotalAuraModifier(SPELL_AURA_MOD_HEALTH_REGEN_IN_COMBAT);

    if (addvalue < 0)
    {
        addvalue = 0;
    }

    addvalue *= (float)diff / REGEN_TIME_FULL;

    ModifyHealth(int32(addvalue));
}

/**
 * @brief Gets an NPC the player can currently interact with.
 *
 * @param guid The target creature GUID.
 * @param npcflagmask Optional NPC flag mask that must be present on the creature.
 * @return The interactable creature, or null if interaction is not allowed.
 */
Creature* Player::GetNPCIfCanInteractWith(ObjectGuid guid, uint32 npcflagmask)
{
    // some basic checks
    if (!guid || !IsInWorld() || IsTaxiFlying())
    {
        return NULL;
    }

    // not in interactive state
    if (hasUnitState(UNIT_STAT_CAN_NOT_REACT_OR_LOST_CONTROL))
    {
        return NULL;
    }

    // exist (we need look pets also for some interaction (quest/etc)
    Creature* unit = GetMap()->GetAnyTypeCreature(guid);
    if (!unit)
    {
        return NULL;
    }

    // appropriate npc type
    if (npcflagmask && !unit->HasFlag(UNIT_NPC_FLAGS, npcflagmask))
    {
        return NULL;
    }

    if (npcflagmask & UNIT_NPC_FLAG_STABLEMASTER)
    {
        if (getClass() != CLASS_HUNTER)
        {
            return NULL;
        }
    }

    // if a dead unit should be able to talk - the creature must be alive and have special flags
    if (!unit->IsAlive())
    {
        return NULL;
    }

    if (IsAlive() && unit->IsInvisibleForAlive())
    {
        return NULL;
    }

    // not allow interaction under control, but allow with own pets
    if (unit->GetCharmerGuid())
    {
        return NULL;
    }

    // not enemy
    if (unit->IsHostileTo(this))
    {
        return NULL;
    }

    // not too far
    if (!unit->IsWithinDistInMap(this, INTERACTION_DISTANCE))
    {
        return NULL;
    }

    return unit;
}

/**
 * @brief Gets a game object the player can currently interact with.
 *
 * @param guid The target game object GUID.
 * @param gameobject_type The required game object type, or MAX_GAMEOBJECT_TYPE for any type.
 * @return The interactable game object, or null if interaction is not allowed.
 */
GameObject* Player::GetGameObjectIfCanInteractWith(ObjectGuid guid, uint32 gameobject_type) const
{
    // some basic checks
    if (!guid || !IsInWorld() || IsTaxiFlying())
    {
        return NULL;
    }

    // not in interactive state
    if (hasUnitState(UNIT_STAT_CAN_NOT_REACT_OR_LOST_CONTROL))
    {
        return NULL;
    }

    if (GameObject* go = GetMap()->GetGameObject(guid))
    {
        if (uint32(go->GetGoType()) == gameobject_type || gameobject_type == MAX_GAMEOBJECT_TYPE)
        {
            float maxdist = go->GetInteractionDistance();
            if (go->IsWithinDistInMap(this, maxdist) && go->isSpawned())
            {
                return go;
            }

            sLog.outError("GetGameObjectIfCanInteractWith: GameObject '%s' [GUID: %u] is too far away from player %s [GUID: %u] to be used by him (distance=%f, maximal %f is allowed)",
                          go->GetGOInfo()->name,  go->GetGUIDLow(), GetName(), GetGUIDLow(), go->GetDistance(this), maxdist);
        }
    }
    return NULL;
}

/**
 * @brief Checks whether the player is currently underwater.
 *
 * @return True if the player is underwater; otherwise, false.
 */
bool Player::IsUnderWater() const
{
    return GetTerrain()->IsUnderWater(GetPositionX(), GetPositionY(), GetPositionZ() + 2);
}

/**
 * @brief Updates the player's in-water state.
 *
 * @param apply True if the player is entering water; false if leaving it.
 */
void Player::SetInWater(bool apply)
{
    if (m_isInWater == apply)
    {
        return;
    }

    // define player in water by opcodes
    // move player's guid into HateOfflineList of those mobs
    // which can't swim and move guid back into ThreatList when
    // on surface.
    // TODO: exist also swimming mobs, and function must be symmetric to enter/leave water
    m_isInWater = apply;

    // remove auras that need water/land
    RemoveAurasWithInterruptFlags(apply ? AURA_INTERRUPT_FLAG_NOT_ABOVEWATER : AURA_INTERRUPT_FLAG_NOT_UNDERWATER);

    GetHostileRefManager().updateThreatTables();
}

struct SetGameMasterOnHelper
{
    explicit SetGameMasterOnHelper() {}
    void operator()(Unit* unit) const
    {
        unit->setFaction(35);
        unit->GetHostileRefManager().setOnlineOfflineState(false);
    }
};

struct SetGameMasterOffHelper
{
    explicit SetGameMasterOffHelper(uint32 _faction) : faction(_faction) {}
    void operator()(Unit* unit) const
    {
        unit->setFaction(faction);
        unit->GetHostileRefManager().setOnlineOfflineState(true);
    }
    uint32 faction;
};

/**
 * @brief Enables or disables game master mode for the player.
 *
 * @param on True to enable GM mode; false to disable it.
 */
void Player::SetGameMaster(bool on)
{
    if (on)
    {
        m_ExtraFlags |= PLAYER_EXTRA_GM_ON;
        //setFaction(35);
        SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_0);
        SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_GM);
        CallForAllControlledUnits(SetGameMasterOnHelper(), CONTROLLED_PET | CONTROLLED_TOTEMS | CONTROLLED_GUARDIANS | CONTROLLED_CHARM);

        SetFFAPvP(false);
        ResetContestedPvP();

        GetHostileRefManager().setOnlineOfflineState(false);
        CombatStopWithPets();

        if (Pet* pet = GetPet())
        {
            if (m_ExtraFlags |= PLAYER_EXTRA_GM_ON)
                pet->setFaction(35);
            pet->GetHostileRefManager().setOnlineOfflineState(false);
        }

        SetPhaseMask(PHASEMASK_ANYWHERE, false);            // see and visible in all phases
    }
    else
    {
        m_ExtraFlags &= ~ PLAYER_EXTRA_GM_ON;
        //setFactionForRace(getRace());
        RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_0);
        RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_GM);

        // restore phase
        AuraList const& phases = GetAurasByType(SPELL_AURA_PHASE);
        SetPhaseMask(!phases.empty() ? phases.front()->GetMiscValue() : uint32(PHASEMASK_NORMAL), false);

        if (Pet* pet = GetPet())
        {
            pet->setFaction(getFaction());
            pet->GetHostileRefManager().setOnlineOfflineState(true);
        }

        CallForAllControlledUnits(SetGameMasterOffHelper(getFaction()), CONTROLLED_PET | CONTROLLED_TOTEMS | CONTROLLED_GUARDIANS | CONTROLLED_CHARM);

        // restore FFA PvP Server state
        if (sWorld.IsFFAPvPRealm())
        {
            SetFFAPvP(true);
        }

        // restore FFA PvP area state, remove not allowed for GM mounts
        UpdateArea(m_areaUpdateId);

        GetHostileRefManager().setOnlineOfflineState(true);
    }

    m_camera.UpdateVisibilityForOwner();
    UpdateObjectVisibility();
    UpdateForQuestWorldObjects();
}

/**
 * @brief Sets whether a game master is visible to other players.
 *
 * @param on True to make the GM visible; false to hide them.
 */
void Player::SetGMVisible(bool on)
{
    if (on)
    {
        m_ExtraFlags &= ~PLAYER_EXTRA_GM_INVISIBLE;         // remove flag

        // Reapply stealth/invisibility if active or show if not any
        if (HasAuraType(SPELL_AURA_MOD_STEALTH))
        {
            SetVisibility(VISIBILITY_GROUP_STEALTH);
        }
        else if (HasAuraType(SPELL_AURA_MOD_INVISIBILITY))
        {
            SetVisibility(VISIBILITY_GROUP_INVISIBILITY);
        }
        else
        {
            SetVisibility(VISIBILITY_ON);
        }
    }
    else
    {
        m_ExtraFlags |= PLAYER_EXTRA_GM_INVISIBLE;          // add flag

        SetAcceptWhispers(false);
        SetGameMaster(true);

        SetVisibility(VISIBILITY_OFF);
    }
}


/**
 * @brief Sends the experience gain log packet to the client.
 *
 * @param GivenXP The base amount of experience awarded.
 * @param victim The kill source, or null for non-kill experience.
 * @param RestXP The rested bonus experience amount.
 */
void Player::SendLogXPGain(uint32 GivenXP, Unit* victim, uint32 RestXP)
{
    WorldPacket data(SMSG_LOG_XPGAIN, 21);
    data << (victim ? victim->GetObjectGuid() : ObjectGuid());// guid
    data << uint32(GivenXP + RestXP);                       // given experience
    data << uint8(victim ? 0 : 1);                          // 00-kill_xp type, 01-non_kill_xp type
    if (victim)
    {
        data << uint32(GivenXP);                            // experience without rested bonus
        data << float(1);                                   // 1 - none 0 - 100% group bonus output
    }
    data << uint8(0);                                       // new 2.4.0
    GetSession()->SendPacket(&data);
}

/**
 * @brief Awards experience to the player and handles level-ups.
 *
 * @param xp The experience amount to award.
 * @param victim The unit responsible for kill-based experience, if any.
 */
void Player::GiveXP(uint32 xp, Unit* victim)
{
    if (xp < 1)
    {
        return;
    }

    if (!IsAlive())
    {
        return;
    }

    if (HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_XP_USER_DISABLED))
    {
        return;
    }

    uint32 level = getLevel();

    // Used by Eluna
#ifdef ENABLE_ELUNA
    if (Eluna* e = GetEluna())
    {
        e->OnGiveXP(this, xp, victim);
    }
#endif /* ENABLE_ELUNA */

    // XP to money conversion processed in Player::RewardQuest
    if (level >= sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL))
    {
        return;
    }

    if (victim)
    {
        // handle SPELL_AURA_MOD_KILL_XP_PCT auras
        Unit::AuraList const& ModXPPctAuras = GetAurasByType(SPELL_AURA_MOD_KILL_XP_PCT);
        for (Unit::AuraList::const_iterator i = ModXPPctAuras.begin(); i != ModXPPctAuras.end(); ++i)
        {
            xp = uint32(xp * (1.0f + (*i)->GetModifier()->m_amount / 100.0f));
        }
    }
    else
    {
        // handle SPELL_AURA_MOD_QUEST_XP_PCT auras
        Unit::AuraList const& ModXPPctAuras = GetAurasByType(SPELL_AURA_MOD_QUEST_XP_PCT);
        for (Unit::AuraList::const_iterator i = ModXPPctAuras.begin(); i != ModXPPctAuras.end(); ++i)
        {
            xp = uint32(xp * (1.0f + (*i)->GetModifier()->m_amount / 100.0f));
        }
    }

    // XP resting bonus for kill
    uint32 rested_bonus_xp = victim ? GetXPRestBonus(xp) : 0;

    SendLogXPGain(xp, victim, rested_bonus_xp);

    uint32 curXP = GetUInt32Value(PLAYER_XP);
    uint32 nextLvlXP = GetUInt32Value(PLAYER_NEXT_LEVEL_XP);
    uint32 newXP = curXP + xp + rested_bonus_xp;

    while (newXP >= nextLvlXP && level < sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL))
    {
        newXP -= nextLvlXP;

        if (level < sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL))
        {
            SetLevel(level + 1);
        }

        level = getLevel();
        nextLvlXP = GetUInt32Value(PLAYER_NEXT_LEVEL_XP);
    }

    SetUInt32Value(PLAYER_XP, newXP);
}

/**
 * @brief Advances the player to a new level and reapplies level-based stats.
 *
 * @param level The new level to assign.
 */
void Player::SetLevel(uint32 level)
{
    uint8 oldLevel = getLevel();
    if (level == oldLevel || level > DEFAULT_MAX_LEVEL)
    {
        return;
    }

    SetUInt32Value(UNIT_FIELD_LEVEL, level);
    SetUInt32Value(PLAYER_XP, 0);
    if (GetGroup())
    {
        SetGroupUpdateFlag(GROUP_UPDATE_FLAG_LEVEL);
    }

    PlayerLevelInfo info;
    sObjectMgr.GetPlayerLevelInfo(getRace(), getClass(), level, &info);

    PlayerClassLevelInfo classInfo;
    sObjectMgr.GetPlayerClassLevelInfo(getClass(), level, &classInfo);

    // send levelup info to client
    WorldPacket data(SMSG_LEVELUP_INFO, (4 + 4 + MAX_POWERS * 4 + MAX_STATS * 4));
    data << uint32(level);
    data << uint32(int32(classInfo.basehealth) - int32(GetCreateHealth()));
    // for(int i = 0; i < MAX_POWERS; ++i)                  // Powers loop (0-6)
    data << uint32(int32(classInfo.basemana)   - int32(GetCreateMana()));
    data << uint32(0);
    data << uint32(0);
    data << uint32(0);
    data << uint32(0);
    data << uint32(0);
    data << uint32(0);
    // end for
    for (int i = STAT_STRENGTH; i < MAX_STATS; ++i)         // Stats loop (0-4)
    {
        data << uint32(int32(info.stats[i]) - GetCreateStat(Stats(i)));
    }

    GetSession()->SendPacket(&data);

    SetUInt32Value(PLAYER_NEXT_LEVEL_XP, sObjectMgr.GetXPForLevel(level));

    // update level, max level of skills
    m_Played_time[PLAYED_TIME_LEVEL] = 0;                   // Level Played Time reset

    _ApplyAllLevelScaleItemMods(false);
    UpdateSkillsForLevel();

    // save base values (bonuses already included in stored stats
    for (int i = STAT_STRENGTH; i < MAX_STATS; ++i)
    {
        SetCreateStat(Stats(i), info.stats[i]);
    }

    SetCreateHealth(classInfo.basehealth);
    SetCreateMana(classInfo.basemana);

    InitTalentForLevel();
    InitTaxiNodesForLevel();
    InitGlyphsForLevel();

    UpdateAllStats();

    // set current level health and mana/energy to maximum after applying all mods.
    if (IsAlive())
    {
        SetHealth(GetMaxHealth());
    }
    SetPower(POWER_MANA, GetMaxPower(POWER_MANA));
    SetPower(POWER_ENERGY, GetMaxPower(POWER_ENERGY));
    if (GetPower(POWER_RAGE) > GetMaxPower(POWER_RAGE))
    {
        SetPower(POWER_RAGE, GetMaxPower(POWER_RAGE));
    }
    SetPower(POWER_FOCUS, 0);
    SetPower(POWER_HAPPINESS, 0);

    _ApplyAllLevelScaleItemMods(true);

    // update level to hunter/summon pet
    if (Pet* pet = GetPet())
    {
        pet->SynchronizeLevelWithOwner();
    }

    // Used by Eluna
#ifdef ENABLE_ELUNA
    if (Eluna* e = GetEluna())
    {
        e->OnLevelChanged(this, oldLevel);
    }
#endif /* ENABLE_ELUNA */

    if (MailLevelReward const* mailReward = sObjectMgr.GetMailLevelReward(level, getRaceMask()))
    {
        MailDraft(mailReward->mailTemplateId).SendMailTo(this, MailSender(MAIL_CREATURE, mailReward->senderEntry));
    }

    GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL);
}

/**
 * @brief Sets the number of free talent points available to the player.
 *
 * @param points The free talent point count.
 */
void Player::SetFreeTalentPoints(uint32 points)
{
    // Used by Eluna
#ifdef ENABLE_ELUNA
    if (Eluna* e = GetEluna())
    {
        e->OnFreeTalentPointsChanged(this, points);
    }
#endif /* ENABLE_ELUNA */

    SetUInt32Value(PLAYER_CHARACTER_POINTS1, points);
}

/****************************************/
/* DO NOT REMOVE: */
/* Used for Eluna compatibility */
/****************************************/
void Player::GiveLevel(uint32 level)
{
    return SetLevel(level);
}

/**
 * @brief Recalculates the player's free talent points for the current level.
 *
 * @param resetIfNeed True to reset talents when the allocation is invalid.
 */
void Player::UpdateFreeTalentPoints(bool resetIfNeed)
{
    uint32 level = getLevel();
    // talents base at level diff ( talents = level - 9 but some can be used already)
    if (level < 10)
    {
        // Remove all talent points
        if (m_usedTalentCount > 0)                          // Free any used talents
        {
            if (resetIfNeed)
            {
                resetTalents(true);
            }
            SetFreeTalentPoints(0);
        }
    }
    else
    {
        uint32 talentPointsForLevel = CalculateTalentsPoints();

        // if used more that have then reset
        if (m_usedTalentCount > talentPointsForLevel)
        {
            if (resetIfNeed && GetSession()->GetSecurity() < SEC_ADMINISTRATOR)
            {
                resetTalents(true);
            }
            else
            {
                SetFreeTalentPoints(0);
            }
        }
        // else update amount of free points
        else
        {
            SetFreeTalentPoints(talentPointsForLevel - m_usedTalentCount);
        }
    }
}

/**
 * @brief Initializes level-based talent availability for the player.
 */
void Player::InitTalentForLevel()
{
    UpdateFreeTalentPoints();

    if (!GetSession()->PlayerLoading())
    {
        SendTalentsInfoData(false);                         // update at client
    }
}

/**
 * @brief Initializes the player's base stats and resources for the current level.
 *
 * @param reapplyMods True to remove and reapply stat modifiers during initialization.
 */
void Player::InitStatsForLevel(bool reapplyMods)
{
    if (reapplyMods)                                        // reapply stats values only on .reset stats (level) command
    {
        _RemoveAllStatBonuses();
    }

    PlayerClassLevelInfo classInfo;
    sObjectMgr.GetPlayerClassLevelInfo(getClass(), getLevel(), &classInfo);

    PlayerLevelInfo info;
    sObjectMgr.GetPlayerLevelInfo(getRace(), getClass(), getLevel(), &info);

    SetUInt32Value(PLAYER_FIELD_MAX_LEVEL, sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL));
    SetUInt32Value(PLAYER_NEXT_LEVEL_XP, sObjectMgr.GetXPForLevel(getLevel()));

    // reset before any aura state sources (health set/aura apply)
    SetUInt32Value(UNIT_FIELD_AURASTATE, 0);

    UpdateSkillsForLevel();

    // set default cast time multiplier
    SetFloatValue(UNIT_MOD_CAST_SPEED, 1.0f);

    // save base values (bonuses already included in stored stats
    for (int i = STAT_STRENGTH; i < MAX_STATS; ++i)
    {
        SetCreateStat(Stats(i), info.stats[i]);
    }

    for (int i = STAT_STRENGTH; i < MAX_STATS; ++i)
    {
        SetStat(Stats(i), info.stats[i]);
    }

    SetCreateHealth(classInfo.basehealth);

    // set create powers
    SetCreateMana(classInfo.basemana);

    SetArmor(int32(m_createStats[STAT_AGILITY] * 2));

    InitStatBuffMods();

    // reset rating fields values
    for (uint16 index = PLAYER_FIELD_COMBAT_RATING_1; index < PLAYER_FIELD_COMBAT_RATING_1 + MAX_COMBAT_RATING; ++index)
    {
        SetUInt32Value(index, 0);
    }

    SetUInt32Value(PLAYER_FIELD_MOD_HEALING_DONE_POS, 0);
    for (int i = 0; i < MAX_SPELL_SCHOOL; ++i)
    {
        SetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG + i, 0);
        SetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + i, 0);
        SetFloatValue(PLAYER_FIELD_MOD_DAMAGE_DONE_PCT + i, 1.00f);
    }

    // reset attack power, damage and attack speed fields
    SetFloatValue(UNIT_FIELD_BASEATTACKTIME, 2000.0f);
    SetFloatValue(UNIT_FIELD_BASEATTACKTIME + 1, 2000.0f);  // offhand attack time
    SetFloatValue(UNIT_FIELD_RANGEDATTACKTIME, 2000.0f);

    SetFloatValue(UNIT_FIELD_MINDAMAGE, 0.0f);
    SetFloatValue(UNIT_FIELD_MAXDAMAGE, 0.0f);
    SetFloatValue(UNIT_FIELD_MINOFFHANDDAMAGE, 0.0f);
    SetFloatValue(UNIT_FIELD_MAXOFFHANDDAMAGE, 0.0f);
    SetFloatValue(UNIT_FIELD_MINRANGEDDAMAGE, 0.0f);
    SetFloatValue(UNIT_FIELD_MAXRANGEDDAMAGE, 0.0f);

    SetInt32Value(UNIT_FIELD_ATTACK_POWER,            0);
    SetInt32Value(UNIT_FIELD_ATTACK_POWER_MODS,       0);
    SetFloatValue(UNIT_FIELD_ATTACK_POWER_MULTIPLIER, 0.0f);
    SetInt32Value(UNIT_FIELD_RANGED_ATTACK_POWER,     0);
    SetInt32Value(UNIT_FIELD_RANGED_ATTACK_POWER_MODS, 0);
    SetFloatValue(UNIT_FIELD_RANGED_ATTACK_POWER_MULTIPLIER, 0.0f);

    // Base crit values (will be recalculated in UpdateAllStats() at loading and in _ApplyAllStatBonuses() at reset
    SetFloatValue(PLAYER_CRIT_PERCENTAGE, 0.0f);
    SetFloatValue(PLAYER_OFFHAND_CRIT_PERCENTAGE, 0.0f);
    SetFloatValue(PLAYER_RANGED_CRIT_PERCENTAGE, 0.0f);

    // Init spell schools (will be recalculated in UpdateAllStats() at loading and in _ApplyAllStatBonuses() at reset
    for (uint8 i = 0; i < MAX_SPELL_SCHOOL; ++i)
    {
        SetFloatValue(PLAYER_SPELL_CRIT_PERCENTAGE1 + i, 0.0f);
    }

    SetFloatValue(PLAYER_PARRY_PERCENTAGE, 0.0f);
    SetFloatValue(PLAYER_BLOCK_PERCENTAGE, 0.0f);
    SetUInt32Value(PLAYER_SHIELD_BLOCK, 0);

    // Dodge percentage
    SetFloatValue(PLAYER_DODGE_PERCENTAGE, 0.0f);

    // set armor (resistance 0) to original value (create_agility*2)
    SetArmor(int32(m_createStats[STAT_AGILITY] * 2));
    SetResistanceBuffMods(SpellSchools(0), true, 0.0f);
    SetResistanceBuffMods(SpellSchools(0), false, 0.0f);
    // set other resistance to original value (0)
    for (int i = 1; i < MAX_SPELL_SCHOOL; ++i)
    {
        SetResistance(SpellSchools(i), 0);
        SetResistanceBuffMods(SpellSchools(i), true, 0.0f);
        SetResistanceBuffMods(SpellSchools(i), false, 0.0f);
    }

    SetUInt32Value(PLAYER_FIELD_MOD_TARGET_RESISTANCE, 0);
    SetUInt32Value(PLAYER_FIELD_MOD_TARGET_PHYSICAL_RESISTANCE, 0);
    for (int i = 0; i < MAX_SPELL_SCHOOL; ++i)
    {
        SetUInt32Value(UNIT_FIELD_POWER_COST_MODIFIER + i, 0);
        SetFloatValue(UNIT_FIELD_POWER_COST_MULTIPLIER + i, 0.0f);
    }
    // Reset no reagent cost field
    for (int i = 0; i < 3; ++i)
    {
        SetUInt32Value(PLAYER_NO_REAGENT_COST_1 + i, 0);
    }
    // Init data for form but skip reapply item mods for form
    InitDataForForm(reapplyMods);

    // save new stats
    for (int i = POWER_MANA; i < MAX_POWERS; ++i)
    {
        SetMaxPower(Powers(i),  GetCreatePowers(Powers(i)));
    }

    SetMaxHealth(classInfo.basehealth);                     // stamina bonus will applied later

    // cleanup mounted state (it will set correctly at aura loading if player saved at mount.
    SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, 0);

    // cleanup unit flags (will be re-applied if need at aura load).
    RemoveFlag(UNIT_FIELD_FLAGS,
               UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_NOT_ATTACKABLE_1 |
               UNIT_FLAG_OOC_NOT_ATTACKABLE | UNIT_FLAG_PASSIVE  | UNIT_FLAG_LOOTING          |
               UNIT_FLAG_PET_IN_COMBAT  | UNIT_FLAG_SILENCED     | UNIT_FLAG_PACIFIED         |
               UNIT_FLAG_STUNNED        | UNIT_FLAG_IN_COMBAT    | UNIT_FLAG_DISARMED         |
               UNIT_FLAG_CONFUSED       | UNIT_FLAG_FLEEING      | UNIT_FLAG_NOT_SELECTABLE   |
               UNIT_FLAG_SKINNABLE      | UNIT_FLAG_MOUNT        | UNIT_FLAG_TAXI_FLIGHT);
    SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);    // must be set

    SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_REGENERATE_POWER); // must be set

    // cleanup player flags (will be re-applied if need at aura load), to avoid have ghost flag without ghost aura, for example.
    RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_AFK | PLAYER_FLAGS_DND | PLAYER_FLAGS_GM | PLAYER_FLAGS_GHOST);

    RemoveStandFlags(UNIT_STAND_FLAGS_ALL);                 // one form stealth modified bytes
    RemoveByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_FFA_PVP | UNIT_BYTE2_FLAG_SANCTUARY);

    // restore if need some important flags
    SetUInt32Value(PLAYER_FIELD_BYTES2, 0);                 // flags empty by default

    if (reapplyMods)                                        // reapply stats values only on .reset stats (level) command
    {
        _ApplyAllStatBonuses();
    }

    // set current level health and mana/energy to maximum after applying all mods.
    SetHealth(GetMaxHealth());
    SetPower(POWER_MANA, GetMaxPower(POWER_MANA));
    SetPower(POWER_ENERGY, GetMaxPower(POWER_ENERGY));
    if (GetPower(POWER_RAGE) > GetMaxPower(POWER_RAGE))
    {
        SetPower(POWER_RAGE, GetMaxPower(POWER_RAGE));
    }
    SetPower(POWER_FOCUS, 0);
    SetPower(POWER_HAPPINESS, 0);
    SetPower(POWER_RUNIC_POWER, 0);

    // update level to hunter/summon pet
    if (Pet* pet = GetPet())
    {
        pet->SynchronizeLevelWithOwner();
    }
}

/* Used during Player::SendInitialPacketsBeforeAddToMap */
/**
 * @brief Sends the initial spellbook and cooldown state to the client.
 */
void Player::SendInitialSpells()
{
    time_t curTime = time(NULL);
    time_t infTime = curTime + infinityCooldownDelayCheck;

    /* * * * * * * * * * * * * * * * *
     * * START OF PACKET STRUCTURE * *
     * * * * * * * * * * * * * * * * */
    uint16 spellCount = 0;

    WorldPacket data(SMSG_INITIAL_SPELLS, (1 + 2 + 4 * m_spells.size() + 2 + m_spellCooldowns.size() * (2 + 2 + 2 + 4 + 4)));
    data << uint8(0);

    /* * * * * * * * * * * * * * * * *
     * *  END OF PACKET STRUCTURE  * *
     * * * * * * * * * * * * * * * * */
    size_t countPos = data.wpos();
    data << uint16(spellCount);                             // spell count placeholder

    /* For each spell the player knows */
    for (PlayerSpellMap::const_iterator itr = m_spells.begin(); itr != m_spells.end(); ++itr)
    {
        /* If the spell is marked as removed, don't send it */
        PlayerSpell const& playerSpell = itr->second;

        if (playerSpell.state == PLAYERSPELL_REMOVED)
        {
            continue;
        }

        if (!playerSpell.active || playerSpell.disabled)
        {
            continue;
        }

        /* Insert spell into vector for insertion into packet */
        data << uint32(itr->first);
        data << uint16(0);                                  // it's not slot id

        /* Increase spell counter by 1 (sent in packet) */
        spellCount += 1;
    }

    data.put<uint16>(countPos, spellCount);                 // write real count value

    /* For each spell the player has on cooldown */
    uint16 spellCooldowns = m_spellCooldowns.size();
    data << uint16(spellCooldowns);
    for (SpellCooldowns::const_iterator itr = m_spellCooldowns.begin(); itr != m_spellCooldowns.end(); ++itr)
    {
        /* If the spell doesn't exist in the spellbook, just ignore it */
        SpellEntry const* sEntry = sSpellStore.LookupEntry(itr->first);
        if (!sEntry)
        {
            continue;
        }

        SpellCooldown const& spellCooldown = itr->second;

        data << uint32(itr->first);

        data << uint16(spellCooldown.itemid);               // cast item id
        data << uint16(sEntry->Category);                   // spell category

        /* send infinity cooldown in special format */
        if (spellCooldown.end >= infTime)
        {
            data << uint32(1);                              // cooldown
            data << uint32(0x80000000);                     // category cooldown
            continue;
        }

        time_t cooldown = spellCooldown.end > curTime ? (spellCooldown.end - curTime) * IN_MILLISECONDS : 0;

        if (sEntry->Category)                               // may be wrong, but anyway better than nothing...
        {
            data << uint32(0);                              // cooldown
            data << uint32(cooldown);                       // category cooldown
        }
        else
        {
            data << uint32(cooldown);                       // cooldown
            data << uint32(0);                              // category cooldown
        }
    }

    GetSession()->SendPacket(&data);

    DETAIL_LOG("CHARACTER: Sent Initial Spells");
}

/**
 * @brief Removes a mail entry from the player's in-memory mailbox.
 *
 * @param id The message identifier to remove.
 */
void Player::RemoveMail(uint32 id)
{
    for (PlayerMails::iterator itr = m_mail.begin(); itr != m_mail.end(); ++itr)
    {
        if ((*itr)->messageID == id)
        {
            // do not delete item, because Player::removeMail() is called when returning mail to sender.
            m_mail.erase(itr);
            return;
        }
    }
}

/**
 * @brief Sends the result of a mail operation to the client.
 *
 * @param mailId The mail message identifier.
 * @param mailAction The mail action that was processed.
 * @param mailError The result code for the action.
 * @param equipError The equipment error code used for equip-related failures.
 * @param item_guid The related item GUID low part.
 * @param item_count The related item count.
 */
void Player::SendMailResult(uint32 mailId, MailResponseType mailAction, MailResponseResult mailError, uint32 equipError, uint32 item_guid, uint32 item_count)
{
    WorldPacket data(SMSG_SEND_MAIL_RESULT, (4 + 4 + 4 + (mailError == MAIL_ERR_EQUIP_ERROR ? 4 : (mailAction == MAIL_ITEM_TAKEN ? 4 + 4 : 0))));
    data << (uint32) mailId;
    data << (uint32) mailAction;
    data << (uint32) mailError;
    if (mailError == MAIL_ERR_EQUIP_ERROR)
    {
        data << (uint32) equipError;
    }
    else if (mailAction == MAIL_ITEM_TAKEN)
    {
        data << (uint32) item_guid;                         // item guid low?
        data << (uint32) item_count;                        // item count?
    }
    GetSession()->SendPacket(&data);
}

/**
 * @brief Notifies the client that new mail is available.
 */
void Player::SendNewMail()
{
    // deliver undelivered mail
    WorldPacket data(SMSG_RECEIVED_MAIL, 4);
    data << (uint32) 0;
    GetSession()->SendPacket(&data);
}

/**
 * @brief Recalculates unread mail count and the next pending delivery time.
 */
void Player::UpdateNextMailTimeAndUnreads()
{
    // calculate next delivery time (min. from non-delivered mails
    // and recalculate unReadMail
    time_t cTime = time(NULL);
    m_nextMailDelivereTime = 0;
    unReadMails = 0;
    for (PlayerMails::iterator itr = m_mail.begin(); itr != m_mail.end(); ++itr)
    {
        if ((*itr)->deliver_time > cTime)
        {
            if (!m_nextMailDelivereTime || m_nextMailDelivereTime > (*itr)->deliver_time)
            {
                m_nextMailDelivereTime = (*itr)->deliver_time;
            }
        }
        else if (((*itr)->checked & MAIL_CHECK_MASK_READ) == 0)
        {
            ++unReadMails;
        }
    }
}

/**
 * @brief Tracks a newly scheduled mail delivery time.
 *
 * @param deliver_time The time when the mail should be delivered.
 */
void Player::AddNewMailDeliverTime(time_t deliver_time)
{
    if (deliver_time <= time(NULL))                         // ready now
    {
        ++unReadMails;
        SendNewMail();
    }
    else                                                    // not ready and no have ready mails
    {
        if (!m_nextMailDelivereTime || m_nextMailDelivereTime > deliver_time)
        {
            m_nextMailDelivereTime =  deliver_time;
        }
    }
}


/**
 * Deletes a character from the database
 *
 * The way, how the characters will be deleted is decided based on the config option.
 *
 * @see Player::DeleteOldCharacters
 *
 * @param playerguid       the low-GUID from the player which should be deleted
 * @param accountId        the account id from the player
 * @param updateRealmChars when this flag is set, the amount of characters on that realm will be updated in the realmlist
 * @param deleteFinally    if this flag is set, the config option will be ignored and the character will be permanently removed from the database
 */
void Player::DeleteFromDB(ObjectGuid playerguid, uint32 accountId, bool updateRealmChars, bool deleteFinally)
{
    //Make sure to delete unresolved tickets so they don't take up place in the open tickets list
    CharacterDatabase.PExecute("DELETE FROM `character_ticket` "
                               "WHERE `resolved` = 0 AND `guid` = %u",
                               playerguid.GetCounter());

    // for nonexistent account avoid update realm
    if (accountId == 0)
    {
        updateRealmChars = false;
    }

    uint32 charDelete_method = sWorld.getConfig(CONFIG_UINT32_CHARDELETE_METHOD);
    uint32 charDelete_minLvl = sWorld.getConfig(CONFIG_UINT32_CHARDELETE_MIN_LEVEL);

    // if we want to finally delete the character or the character does not meet the level requirement, we set it to mode 0
    if (deleteFinally || Player::GetLevelFromDB(playerguid) < charDelete_minLvl)
    {
        charDelete_method = 0;
    }

    uint32 lowguid = playerguid.GetCounter();

    // convert corpse to bones if exist (to prevent exiting Corpse in World without DB entry)
    // bones will be deleted by corpse/bones deleting thread shortly
    sObjectAccessor.ConvertCorpseForPlayer(playerguid);

    // remove from guild
    if (uint32 guildId = GetGuildIdFromDB(playerguid))
    {
        if (Guild* guild = sGuildMgr.GetGuildById(guildId))
        {
            if (guild->DelMember(playerguid))
            {
                guild->Disband();
                delete guild;
            }
        }
    }

    // remove from arena teams
    LeaveAllArenaTeams(playerguid);

    // the player was uninvited already on logout so just remove from group
    QueryResult* resultGroup = CharacterDatabase.PQuery("SELECT `groupId` FROM `group_member` WHERE `memberGuid`='%u'", lowguid);
    if (resultGroup)
    {
        uint32 groupId = (*resultGroup)[0].GetUInt32();
        delete resultGroup;
        if (Group* group = sObjectMgr.GetGroupById(groupId))
        {
            RemoveFromGroup(group, playerguid, playerguid, "");
        }
    }

    // remove signs from petitions (also remove petitions if owner);
    RemovePetitionsAndSigns(playerguid, 10);

    switch (charDelete_method)
    {
            // completely remove from the database
        case 0:
        {
            // return back all mails with COD and Item                  0    1             2                3        4         5      6       7
            QueryResult* resultMail = CharacterDatabase.PQuery("SELECT `id`,`messageType`,`mailTemplateId`,`sender`,`subject`,`body`,`money`,`has_items` FROM `mail` WHERE `receiver`='%u' AND `has_items`<>0 AND `cod`<>0", lowguid);
            if (resultMail)
            {
                do
                {
                    Field* fields = resultMail->Fetch();

                    uint32 mail_id       = fields[0].GetUInt32();
                    uint16 mailType      = fields[1].GetUInt16();
                    uint16 mailTemplateId = fields[2].GetUInt16();
                    uint32 sender        = fields[3].GetUInt32();
                    std::string subject  = fields[4].GetCppString();
                    std::string body     = fields[5].GetCppString();
                    uint32 money         = fields[6].GetUInt32();
                    bool has_items       = fields[7].GetBool();

                    // we can return mail now
                    // so firstly delete the old one
                    CharacterDatabase.PExecute("DELETE FROM `mail` WHERE `id` = '%u'", mail_id);

                    // mail not from player
                    if (mailType != MAIL_NORMAL)
                    {
                        if (has_items)
                        {
                            CharacterDatabase.PExecute("DELETE FROM `mail_items` WHERE `mail_id` = '%u'", mail_id);
                        }
                        continue;
                    }

                    MailDraft draft;
                    if (mailTemplateId)
                    {
                        draft.SetMailTemplate(mailTemplateId, false); // items already included
                    }
                    else
                    {
                        draft.SetSubjectAndBody(subject, body);
                    }

                    if (has_items)
                    {
                        // data needs to be at first place for Item::LoadFromDB
                        //                                                           0      1      2           3
                        QueryResult* resultItems = CharacterDatabase.PQuery("SELECT `data`,`text`,`item_guid`,`item_template` FROM `mail_items` JOIN `item_instance` ON `item_guid` = `guid` WHERE `mail_id`='%u'", mail_id);
                        if (resultItems)
                        {
                            do
                            {
                                Field* fields2 = resultItems->Fetch();

                                uint32 item_guidlow = fields2[2].GetUInt32();
                                uint32 item_template = fields2[3].GetUInt32();

                                ItemPrototype const* itemProto = ObjectMgr::GetItemPrototype(item_template);
                                if (!itemProto)
                                {
                                    CharacterDatabase.PExecute("DELETE FROM `item_instance` WHERE `guid` = '%u'", item_guidlow);
                                    continue;
                                }

                                Item* pItem = NewItemOrBag(itemProto);
                                if (!pItem->LoadFromDB(item_guidlow, fields2, playerguid))
                                {
                                    pItem->FSetState(ITEM_REMOVED);
                                    pItem->SaveToDB();      // it also deletes item object !
                                    continue;
                                }

                                draft.AddItem(pItem);
                            }
                            while (resultItems->NextRow());

                            delete resultItems;
                        }
                    }

                    CharacterDatabase.PExecute("DELETE FROM `mail_items` WHERE `mail_id` = '%u'", mail_id);

                    uint32 pl_account = sObjectMgr.GetPlayerAccountIdByGUID(playerguid);

                    draft.SetMoney(money).SendReturnToSender(pl_account, playerguid, ObjectGuid(HIGHGUID_PLAYER, sender));
                }
                while (resultMail->NextRow());

                delete resultMail;
            }

            // unsummon and delete for pets in world is not required: player deleted from CLI or character list with not loaded pet.
            // Get guids of character's pets, will deleted in transaction
            QueryResult* resultPets = CharacterDatabase.PQuery("SELECT `id` FROM `character_pet` WHERE `owner` = '%u'", lowguid);

            // delete char from friends list when selected chars is online (non existing - error)
            QueryResult* resultFriend = CharacterDatabase.PQuery("SELECT DISTINCT `guid` FROM `character_social` WHERE `friend` = '%u'", lowguid);

            // NOW we can finally clear other DB data related to character
            CharacterDatabase.BeginTransaction();
            if (resultPets)
            {
                do
                {
                    Field* fields3 = resultPets->Fetch();
                    uint32 petguidlow = fields3[0].GetUInt32();
                    // do not create separate transaction for pet delete otherwise we will get fatal error!
                    Pet::DeleteFromDB(petguidlow, false);
                }
                while (resultPets->NextRow());
                delete resultPets;
            }

            // cleanup friends for online players, offline case will cleanup later in code
            if (resultFriend)
            {
                do
                {
                    Field* fieldsFriend = resultFriend->Fetch();
                    if (Player* sFriend = sObjectAccessor.FindPlayer(ObjectGuid(HIGHGUID_PLAYER, fieldsFriend[0].GetUInt32())))
                    {
                        if (sFriend->IsInWorld())
                        {
                            sFriend->GetSocial()->RemoveFromSocialList(playerguid, false);
                            sSocialMgr.SendFriendStatus(sFriend, FRIEND_REMOVED, playerguid, false);
                        }
                    }
                }
                while (resultFriend->NextRow());
                delete resultFriend;
            }

            CharacterDatabase.PExecute("DELETE FROM `characters` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_account_data` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_declinedname` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_action` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_aura` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_battleground_data` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_gifts` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_glyphs` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_homebind` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_instance` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `group_instance` WHERE `leaderGuid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_inventory` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_queststatus` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_queststatus_daily` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_queststatus_weekly` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_reputation` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_skills` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_spell` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_spell_cooldown` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_talent` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_ticket` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `item_instance` WHERE `owner_guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_social` WHERE `guid` = '%u' OR `friend`='%u'", lowguid, lowguid);
            CharacterDatabase.PExecute("DELETE FROM `mail` WHERE `receiver` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `mail_items` WHERE `receiver` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_pet` WHERE `owner` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_pet_declinedname` WHERE `owner` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_achievement` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_achievement_progress` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_equipmentsets` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `guild_eventlog` WHERE `PlayerGuid1` = '%u' OR `PlayerGuid2` = '%u'", lowguid, lowguid);
            CharacterDatabase.PExecute("DELETE FROM `guild_bank_eventlog` WHERE `PlayerGuid` = '%u'", lowguid);
            CharacterDatabase.CommitTransaction();
            break;
        }
        // The character gets unlinked from the account, the name gets freed up and appears as deleted ingame
        case 1:
            CharacterDatabase.PExecute("UPDATE `characters` SET `deleteInfos_Name`=`name`, `deleteInfos_Account`=`account`, `deleteDate`='" UI64FMTD "', `name`='', `account`=0 WHERE `guid`=%u", uint64(time(NULL)), lowguid);
            break;
        default:
            sLog.outError("Player::DeleteFromDB: Unsupported delete method: %u.", charDelete_method);
    }

    if (updateRealmChars)
    {
        sWorld.UpdateRealmCharCount(accountId);
    }
}

/**
 * Characters which were kept back in the database after being deleted and are now too old (see config option "CharDelete.KeepDays"), will be completely deleted.
 *
 * @see Player::DeleteFromDB
 */
void Player::DeleteOldCharacters()
{
    uint32 keepDays = sWorld.getConfig(CONFIG_UINT32_CHARDELETE_KEEP_DAYS);
    if (!keepDays)
    {
        return;
    }

    Player::DeleteOldCharacters(keepDays);
}

/**
 * Characters which were kept back in the database after being deleted and are older than the specified amount of days, will be completely deleted.
 *
 * @see Player::DeleteFromDB
 *
 * @param keepDays overrite the config option by another amount of days
 */
void Player::DeleteOldCharacters(uint32 keepDays)
{
    sLog.outString("Player::DeleteOldChars: Deleting all characters which have been deleted %u days before...", keepDays);

    QueryResult* resultChars = CharacterDatabase.PQuery("SELECT `guid`, `deleteInfos_Account` FROM `characters` WHERE `deleteDate` IS NOT NULL AND `deleteDate` < '" UI64FMTD "'", uint64(time(NULL) - time_t(keepDays * DAY)));
    if (resultChars)
    {
        sLog.outString("Player::DeleteOldChars: Found %u character(s) to delete", uint32(resultChars->GetRowCount()));
        do
        {
            Field* charFields = resultChars->Fetch();
            ObjectGuid guid = ObjectGuid(HIGHGUID_PLAYER, charFields[0].GetUInt32());
            Player::DeleteFromDB(guid, charFields[1].GetUInt32(), true, true);
        }
        while (resultChars->NextRow());
        delete resultChars;
    }
    sLog.outString();
}

/**
 * @brief Forces or clears rooted movement for the player.
 *
 * @param enable True to root the player; false to unroot them.
 */
void Player::SetRoot(bool enable)
{
    WorldPacket data(enable ? SMSG_FORCE_MOVE_ROOT : SMSG_FORCE_MOVE_UNROOT, GetPackGUID().size() + 4);
    data << GetPackGUID();
    data << uint32(0);
    GetSession()->SendPacket(&data);
}

/**
 * @brief Enables or disables water walking for the player.
 *
 * @param enable True to enable water walking; false to restore normal movement.
 */
void Player::SetWaterWalk(bool enable)
{
    WorldPacket data(enable ? SMSG_MOVE_WATER_WALK : SMSG_MOVE_LAND_WALK, GetPackGUID().size() + 4);
    data << GetPackGUID();
    data << uint32(0);
    GetSession()->SendPacket(&data);
}

/**
 * @brief Placeholder for levitation support on this client version.
 *
 * @param enable Unused levitation state flag.
 */
void Player::SetLevitate(bool enable)
{
    WorldPacket data;
    if (enable)
    {
        data.Initialize(SMSG_MOVE_GRAVITY_DISABLE, 12);
    }
    else
    {
        data.Initialize(SMSG_MOVE_GRAVITY_ENABLE, 12);
    }

    data << GetPackGUID();
    data << uint32(0);                                      // unk
    SendMessageToSet(&data, true);

    data.Initialize(MSG_MOVE_GRAVITY_CHNG, 64);
    data << GetPackGUID();
    m_movementInfo.Write(data);
    SendMessageToSet(&data, false);
}

/**
 * @brief Enables or disables flying movement flags for the player.
 *
 * @param enable True to enable flight-related movement flags; false to clear them.
 */
void Player::SetCanFly(bool enable)
{
    WorldPacket data;
    if (enable)
    {
        data.Initialize(SMSG_MOVE_SET_CAN_FLY, 12);
    }
    else
    {
        data.Initialize(SMSG_MOVE_UNSET_CAN_FLY, 12);
    }

    data << GetPackGUID();
    data << uint32(0);                                      // unk
    SendMessageToSet(&data, true);

    data.Initialize(MSG_MOVE_UPDATE_CAN_FLY, 64);
    data << GetPackGUID();
    m_movementInfo.Write(data);
    SendMessageToSet(&data, false);
}

/**
 * @brief Enables or disables feather fall movement for the player.
 *
 * @param enable True to enable feather fall; false to restore normal falling.
 */
void Player::SetFeatherFall(bool enable)
{
    WorldPacket data;
    if (enable)
    {
        data.Initialize(SMSG_MOVE_FEATHER_FALL, 8 + 4);
    }
    else
    {
        data.Initialize(SMSG_MOVE_NORMAL_FALL, 8 + 4);
    }

    data << GetPackGUID();
    data << uint32(0);
    SendMessageToSet(&data, true);

    // start fall from current height
    if (!enable)
    {
        SetFallInformation(0, GetPositionZ());
    }
}

/**
 * @brief Enables or disables hover movement for the player.
 *
 * @param enable True to enable hovering; false to disable it.
 */
void Player::SetHover(bool enable)
{
    WorldPacket data;
    if (enable)
    {
        data.Initialize(SMSG_MOVE_SET_HOVER, 8 + 4);
    }
    else
    {
        data.Initialize(SMSG_MOVE_UNSET_HOVER, 8 + 4);
    }

    data << GetPackGUID();
    data << uint32(0);
    SendMessageToSet(&data, true);
}


/**
 * @brief Attempts to improve defense skill and refresh defense-derived bonuses.
 */
void Player::UpdateDefense()
{
    uint32 defense_skill_gain = sWorld.getConfig(CONFIG_UINT32_SKILL_GAIN_DEFENSE);

    if (UpdateSkill(SKILL_DEFENSE, defense_skill_gain))
    {
        // update dependent from defense skill part
        UpdateDefenseBonusesMod();
    }
}


/**
 * @brief Moves the player to a new position and updates related state.
 *
 * @param x The destination X coordinate.
 * @param y The destination Y coordinate.
 * @param z The destination Z coordinate.
 * @param orientation The destination facing angle.
 * @param teleport True if the move should be treated as a teleport.
 * @return True if the position update succeeded; otherwise, false.
 */
bool Player::SetPosition(float x, float y, float z, float orientation, bool teleport)
{
    // prevent crash when a bad coord is sent by the client
    if (!MaNGOS::IsValidMapCoord(x, y, z, orientation))
    {
        DEBUG_LOG("Player::SetPosition(%f, %f, %f, %f, %d) .. bad coordinates for player %d!", x, y, z, orientation, teleport, GetGUIDLow());
        return false;
    }

    Map* m = GetMap();

    const float old_x = GetPositionX();
    const float old_y = GetPositionY();
    const float old_z = GetPositionZ();
    const float old_r = GetOrientation();

    if (teleport || old_x != x || old_y != y || old_z != z || old_r != orientation)
    {
        if (teleport || old_x != x || old_y != y || old_z != z)
        {
            RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_MOVE | AURA_INTERRUPT_FLAG_TURNING);
        }
        else
        {
            RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TURNING);
        }

        RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

        // move and update visible state if need
        m->PlayerRelocation(this, x, y, z, orientation);

        // reread after Map::Relocation
        m = GetMap();
        x = GetPositionX();
        y = GetPositionY();
        z = GetPositionZ();

        // group update
        if (GetGroup() && (old_x != x || old_y != y))
        {
            SetGroupUpdateFlag(GROUP_UPDATE_FLAG_POSITION);
        }
        if (GetTrader() && !IsWithinDistInMap(GetTrader(), INTERACTION_DISTANCE))
        {
            GetSession()->SendCancelTrade(); // will close both side trade windows
        }
    }

    if (m_positionStatusUpdateTimer)                        // Update position's state only on interval
    {
        return true;
    }
    m_positionStatusUpdateTimer = 100;

    // code block for underwater state update
    UpdateUnderwaterState(m, x, y, z);

    // code block for outdoor state and area-explore check
    CheckAreaExploreAndOutdoor();

    return true;
}

/**
 * @brief Saves the player's current position as the recall location.
 */
void Player::SaveRecallPosition()
{
    m_recallMap = GetMapId();
    m_recallX = GetPositionX();
    m_recallY = GetPositionY();
    m_recallZ = GetPositionZ();
    m_recallO = GetOrientation();
}

/**
 * @brief Broadcasts a packet to nearby clients and optionally to the player.
 *
 * @param data The packet to send.
 * @param self True to also send the packet to the player session.
 */
void Player::SendMessageToSet(WorldPacket* data, bool self) const
{
    if (IsInWorld())
    {
        GetMap()->MessageBroadcast(this, data, false);
    }

    // if player is not in world and map in not created/already destroyed
    // no need to create one, just send packet for itself!
    if (self)
    {
        GetSession()->SendPacket(data);
    }
}

/**
 * @brief Broadcasts a packet to nearby clients within a distance and optionally to the player.
 *
 * @param data The packet to send.
 * @param dist The maximum broadcast distance.
 * @param self True to also send the packet to the player session.
 */
void Player::SendMessageToSetInRange(WorldPacket* data, float dist, bool self) const
{
    if (IsInWorld())
    {
        GetMap()->MessageDistBroadcast(this, data, dist, false);
    }

    if (self)
    {
        GetSession()->SendPacket(data);
    }
}

/**
 * @brief Broadcasts a packet within range with optional team filtering.
 *
 * @param data The packet to send.
 * @param dist The maximum broadcast distance.
 * @param self True to also send the packet to the player session.
 * @param own_team_only True to restrict delivery to the player's team.
 */
void Player::SendMessageToSetInRange(WorldPacket* data, float dist, bool self, bool own_team_only) const
{
    if (IsInWorld())
    {
        GetMap()->MessageDistBroadcast(this, data, dist, false, own_team_only);
    }

    if (self)
    {
        GetSession()->SendPacket(data);
    }
}

/**
 * @brief Sends a packet directly to the player's session.
 *
 * @param data The packet to send.
 */
void Player::SendDirectMessage(WorldPacket* data) const
{
    GetSession()->SendPacket(data);
}

/**
 * @brief Starts a cinematic sequence for the player client.
 *
 * @param CinematicSequenceId The cinematic sequence identifier.
 */
void Player::SendCinematicStart(uint32 CinematicSequenceId)
{
    WorldPacket data(SMSG_TRIGGER_CINEMATIC, 4);
    data << uint32(CinematicSequenceId);
    SendDirectMessage(&data);
}

#if defined (WOTLK) || defined (CATA) || defined (MISTS)
/**
 * @brief Starts a movie sequence for the player client.
 *
 * @param MovieId The movie identifier.
 */
void Player::SendMovieStart(uint32 MovieId)
{
    WorldPacket data(SMSG_TRIGGER_MOVIE, 4);
    data << uint32(MovieId);
    SendDirectMessage(&data);
}
#endif

/**
 * @brief True while a DK intro cinematic/flyover is in progress for this player.
 */
bool Player::IsCinematicIntroActive() const
{
    return m_cinematicFlyover && m_cinematicFlyover->IsIntroInProgress();
}

/**
 * @brief Applies the DK intro PvP flag that was deferred during the cinematic.
 */
void Player::ApplyDeferredIntroPvP()
{
    // The intro cinematic deferred the hostile-area PvP flag (see UpdateZone) so
    // its flag sound would not play mid-cinematic; apply it now, at the end.
    if (pvpInfo.inHostileArea && !IsPvP())
    {
        UpdatePvP(true, true);
    }
}

/**
 * @brief Updates outdoor-only effects and exploration discovery for the current position.
 */
void Player::CheckAreaExploreAndOutdoor()
{
    if (!IsAlive())
    {
        return;
    }

    if (IsTaxiFlying())
    {
        return;
    }

    // Defer area-exploration discovery/XP while a DK intro flyover is in progress;
    // it is granted on cinematic complete (see HandleCompleteCinematic).
    if (IsCinematicIntroActive())
    {
        return;
    }

    bool isOutdoor;
    uint16 areaFlag = GetTerrain()->GetAreaFlag(GetPositionX(), GetPositionY(), GetPositionZ(), &isOutdoor);

    if (isOutdoor)
    {
        if (HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING) && GetRestType() == REST_TYPE_IN_TAVERN)
        {
            AreaTriggerEntry const* at = sAreaTriggerStore.LookupEntry(inn_trigger_id);
            if (!at || !IsPointInAreaTriggerZone(at, GetMapId(), GetPositionX(), GetPositionY(), GetPositionZ()))
            {
                // Player left inn (REST_TYPE_IN_CITY overrides REST_TYPE_IN_TAVERN, so just clear rest)
                SetRestType(REST_TYPE_NO);
            }
        }
        // Check if we need to reaply outdoor only passive spells
        const PlayerSpellMap& sp_list = GetSpellMap();
        for (PlayerSpellMap::const_iterator itr = sp_list.begin(); itr != sp_list.end(); ++itr)
        {
            if (itr->second.state == PLAYERSPELL_REMOVED)
            {
                continue;
            }
            SpellEntry const* spellInfo = sSpellStore.LookupEntry(itr->first);
            if (!spellInfo || !IsNeedCastSpellAtOutdoor(spellInfo) || HasAura(itr->first))
            {
                continue;
            }
            if ((spellInfo->Stances || spellInfo->StancesNot) && !IsNeedCastSpellAtFormApply(spellInfo, GetShapeshiftForm()))
            {
                continue;
            }
            CastSpell(this, itr->first, true, NULL);
        }
    }
    else if (sWorld.getConfig(CONFIG_BOOL_VMAP_INDOOR_CHECK) && !isGameMaster())
    {
        RemoveAurasWithAttribute(SPELL_ATTR_OUTDOORS_ONLY);
    }

    if (areaFlag == 0xffff)
    {
        return;
    }
    int offset = areaFlag / 32;

    if (offset >= PLAYER_EXPLORED_ZONES_SIZE)
    {
        sLog.outError("Wrong area flag %u in map data for (X: %f Y: %f) point to field PLAYER_EXPLORED_ZONES_1 + %u ( %u must be < %u ).", areaFlag, GetPositionX(), GetPositionY(), offset, offset, PLAYER_EXPLORED_ZONES_SIZE);
        return;
    }

    uint32 val = (uint32)(1 << (areaFlag % 32));
    uint32 currFields = GetUInt32Value(PLAYER_EXPLORED_ZONES_1 + offset);

    if (!(currFields & val))
    {
        SetUInt32Value(PLAYER_EXPLORED_ZONES_1 + offset, (uint32)(currFields | val));

        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_EXPLORE_AREA);

        AreaTableEntry const* p = GetAreaEntryByAreaFlagAndMap(areaFlag, GetMapId());
        if (!p)
        {
            sLog.outError("PLAYER: Player %u discovered unknown area (x: %f y: %f map: %u", GetGUIDLow(), GetPositionX(), GetPositionY(), GetMapId());
        }
        else if (p->area_level > 0)
        {
            uint32 area = p->ID;
            if (getLevel() >= sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL))
            {
                SendExplorationExperience(area, 0);
            }
            else
            {
                int32 diff = int32(getLevel()) - p->area_level;
                uint32 XP = 0;
                if (diff < -5)
                {
                    XP = uint32(sObjectMgr.GetBaseXP(getLevel() + 5) * sWorld.getConfig(CONFIG_FLOAT_RATE_XP_EXPLORE));
                }
                else if (diff > 5)
                {
                    int32 exploration_percent = (100 - ((diff - 5) * 5));
                    if (exploration_percent > 100)
                    {
                        exploration_percent = 100;
                    }
                    else if (exploration_percent < 0)
                    {
                        exploration_percent = 0;
                    }

                    XP = uint32(sObjectMgr.GetBaseXP(p->area_level) * exploration_percent / 100 * sWorld.getConfig(CONFIG_FLOAT_RATE_XP_EXPLORE));
                }
                else
                {
                    XP = uint32(sObjectMgr.GetBaseXP(p->area_level) * sWorld.getConfig(CONFIG_FLOAT_RATE_XP_EXPLORE));
                }

                GiveXP(XP, NULL);
                SendExplorationExperience(area, XP);
            }
            DETAIL_LOG("PLAYER: Player %u discovered a new area: %u", GetGUIDLow(), area);
        }
    }
}

/**
 * @brief Gets the faction team associated with a race.
 *
 * @param race The race identifier to evaluate.
 * @return The team assigned to the race.
 */
Team Player::TeamForRace(uint8 race)
{
    ChrRacesEntry const* rEntry = sChrRacesStore.LookupEntry(race);
    if (!rEntry)
    {
        sLog.outError("Race %u not found in DBC: wrong DBC files?", uint32(race));
        return ALLIANCE;
    }

    switch (rEntry->TeamID)
    {
        case 7: return ALLIANCE;
        case 1: return HORDE;
    }

    sLog.outError("Race %u have wrong teamid %u in DBC: wrong DBC files?", uint32(race), rEntry->TeamID);
    return TEAM_NONE;
}

/**
 * @brief Gets the faction template associated with a race.
 *
 * @param race The race identifier to evaluate.
 * @return The faction template identifier for the race.
 */
uint32 Player::getFactionForRace(uint8 race)
{
    ChrRacesEntry const* rEntry = sChrRacesStore.LookupEntry(race);
    if (!rEntry)
    {
        sLog.outError("Race %u not found in DBC: wrong DBC files?", uint32(race));
        return 0;
    }

    return rEntry->FactionID;
}

/**
 * @brief Sets the player's team and faction from the specified race.
 *
 * @param race The race identifier to apply.
 */
void Player::setFactionForRace(uint8 race)
{
    m_team = TeamForRace(race);
    setFaction(getFactionForRace(race));
}

/**
 * @brief Gets the player's current reputation rank with a faction.
 *
 * @param faction The faction identifier to query.
 * @return The current reputation rank.
 */
ReputationRank Player::GetReputationRank(uint32 faction) const
{
    FactionEntry const* factionEntry = sFactionStore.LookupEntry(faction);
    return GetReputationMgr().GetRank(factionEntry);
}

// Calculate total reputation percent player gain with quest/creature level
int32 Player::CalculateReputationGain(ReputationSource source, int32 rep, int32 faction, uint32 creatureOrQuestLevel, bool noAuraBonus)
{
    float percent = 100.0f;

    float repMod = noAuraBonus ? 0.0f : (float)GetTotalAuraModifier(SPELL_AURA_MOD_REPUTATION_GAIN);

    // faction specific auras only seem to apply to kills
    if (source == REPUTATION_SOURCE_KILL)
    {
        repMod += GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_FACTION_REPUTATION_GAIN, faction);
    }

    percent += rep > 0 ? repMod : -repMod;

    float rate;
    switch (source)
    {
        case REPUTATION_SOURCE_KILL:
            rate = sWorld.getConfig(CONFIG_FLOAT_RATE_REPUTATION_LOWLEVEL_KILL);
            break;
        case REPUTATION_SOURCE_QUEST:
            rate = sWorld.getConfig(CONFIG_FLOAT_RATE_REPUTATION_LOWLEVEL_QUEST);
            break;
        case REPUTATION_SOURCE_SPELL:
        default:
            rate = 1.0f;
            break;
    }

    if (rate != 1.0f && creatureOrQuestLevel <= MaNGOS::XP::GetGrayLevel(getLevel()))
    {
        percent *= rate;
    }

    if (percent <= 0.0f)
    {
        return 0;
    }

    // Multiply result with the faction specific rate
    if (const RepRewardRate* repData = sObjectMgr.GetRepRewardRate(faction))
    {
        float repRate = 0.0f;
        switch (source)
        {
            case REPUTATION_SOURCE_KILL:
                repRate = repData->creature_rate;
                break;
            case REPUTATION_SOURCE_QUEST:
                repRate = repData->quest_rate;
                break;
            case REPUTATION_SOURCE_SPELL:
                repRate = repData->spell_rate;
                break;
        }

        // for custom, a rate of 0.0 will totally disable reputation gain for this faction/type
        if (repRate <= 0.0f)
        {
            return 0;
        }

        percent *= repRate;
    }

    return int32(sWorld.getConfig(CONFIG_FLOAT_RATE_REPUTATION_GAIN) * rep * percent / 100.0f);
}

// Calculates how many reputation points player gains in victim's enemy factions
void Player::RewardReputation(Unit* pVictim, float rate)
{
    if (!pVictim || pVictim->GetTypeId() == TYPEID_PLAYER)
    {
        return;
    }

    Creature* pVictimAsCreature = reinterpret_cast<Creature*>(pVictim);

    if (pVictimAsCreature->IsReputationGainDisabled())
    {
        return;
    }

    // used current difficulty creature entry instead normal version (GetEntry())
    ReputationOnKillEntry const* Rep = sObjectMgr.GetReputationOnKillEntry(pVictimAsCreature->GetCreatureInfo()->Entry);

    if (!Rep)
    {
        return;
    }

    uint32 repFaction1 = Rep->repfaction1;
    uint32 repFaction2 = Rep->repfaction2;

    // Championning tabard reputation system
    // Aura 57818 is a hidden aura common to tabards allowing championning.
    if (pVictim->GetMap()->IsNonRaidDungeon() && HasAura(57818))
    {
        MapEntry const* storedMap = sMapStore.LookupEntry(GetMapId());
        InstanceTemplate const* instance = ObjectMgr::GetInstanceTemplate(GetMapId());
        Item const* pItem = GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_TABARD);
        if (storedMap && instance && pItem)
        {
            ItemPrototype const* pProto = pItem->GetProto();// Checked on load
            // The required MinLevel for the tabard to work is related to the item level of the tabard
            if ((instance->levelMin + 1 >= pProto->ItemLevel || !GetMap()->IsRegularDifficulty())
                    // For ItemLevel == 75 (or 85) need to check expansion
                    && (pProto->ItemLevel == 75 && storedMap->Expansion() == EXPANSION_WOTLK))
            {
                if (uint32 tabardFactionID = pItem->GetProto()->RequiredReputationFaction)
                {
                    repFaction1 = tabardFactionID;
                    repFaction2 = tabardFactionID;
                }
            }
        }
    }

    if (repFaction1 && (!Rep->team_dependent || GetTeam() == ALLIANCE))
    {
        int32 donerep1 = CalculateReputationGain(REPUTATION_SOURCE_KILL, Rep->repvalue1, repFaction1, pVictim->getLevel());
        donerep1 = int32(donerep1 * rate);
        FactionEntry const* factionEntry1 = sFactionStore.LookupEntry(repFaction1);
        uint32 current_reputation_rank1 = GetReputationMgr().GetRank(factionEntry1);
        if (factionEntry1 && current_reputation_rank1 <= Rep->reputation_max_cap1)
        {
            GetReputationMgr().ModifyReputation(factionEntry1, donerep1);
        }

        // Wiki: Team factions value divided by 2
        if (factionEntry1 && Rep->is_teamaward1)
        {
            FactionEntry const* team1_factionEntry = sFactionStore.LookupEntry(factionEntry1->team);
            if (team1_factionEntry)
            {
                GetReputationMgr().ModifyReputation(team1_factionEntry, donerep1 / 2);
            }
        }
    }

    if (repFaction2 && (!Rep->team_dependent || GetTeam() == HORDE))
    {
        int32 donerep2 = CalculateReputationGain(REPUTATION_SOURCE_KILL, Rep->repvalue2, repFaction2, pVictim->getLevel());
        donerep2 = int32(donerep2 * rate);
        FactionEntry const* factionEntry2 = sFactionStore.LookupEntry(repFaction2);
        uint32 current_reputation_rank2 = GetReputationMgr().GetRank(factionEntry2);
        if (factionEntry2 && current_reputation_rank2 <= Rep->reputation_max_cap2)
        {
            GetReputationMgr().ModifyReputation(factionEntry2, donerep2);
        }

        // Wiki: Team factions value divided by 2
        if (factionEntry2 && Rep->is_teamaward2)
        {
            FactionEntry const* team2_factionEntry = sFactionStore.LookupEntry(factionEntry2->team);
            if (team2_factionEntry)
            {
                GetReputationMgr().ModifyReputation(team2_factionEntry, donerep2 / 2);
            }
        }
    }
}

// Calculate how many reputation points player gain with the quest
void Player::RewardReputation(Quest const* pQuest)
{
    // quest reputation reward/loss
    for (int i = 0; i < QUEST_REPUTATIONS_COUNT; ++i)
    {
        if (!pQuest->RewRepFaction[i])
        {
            continue;
        }

        // No diplomacy mod are applied to the final value (flat). Note the formula (finalValue = DBvalue/100)
        if (pQuest->RewRepValue[i])
        {
            int32 rep = CalculateReputationGain(REPUTATION_SOURCE_QUEST, pQuest->RewRepValue[i] / 100, pQuest->RewRepFaction[i], GetQuestLevelForPlayer(pQuest), true);

            if (FactionEntry const* factionEntry = sFactionStore.LookupEntry(pQuest->RewRepFaction[i]))
            {
                GetReputationMgr().ModifyReputation(factionEntry, rep);
            }
        }
        else
        {
            uint32 row = ((pQuest->RewRepValueId[i] < 0) ? 1 : 0) + 1;
            uint32 field = abs(pQuest->RewRepValueId[i]);

            if (const QuestFactionRewardEntry* pRow = sQuestFactionRewardStore.LookupEntry(row))
            {
                int32 repPoints = pRow->rewardValue[field];

                if (!repPoints)
                {
                    continue;
                }

                repPoints = CalculateReputationGain(REPUTATION_SOURCE_QUEST, repPoints, pQuest->RewRepFaction[i], GetQuestLevelForPlayer(pQuest));

                if (const FactionEntry* factionEntry = sFactionStore.LookupEntry(pQuest->RewRepFaction[i]))
                {
                    GetReputationMgr().ModifyReputation(factionEntry, repPoints);
                }
            }
        }
    }

    // TODO: implement reputation spillover
}

void Player::UpdateArenaFields(void)
{
    /* arena calcs go here */
}

void Player::UpdateHonorFields()
{
    /// called when rewarding honor and at each save
    time_t now = time(NULL);
    time_t today = (time(NULL) / DAY) * DAY;

    if (m_lastHonorUpdateTime < today)
    {
        time_t yesterday = today - DAY;

        uint16 kills_today = PAIR32_LOPART(GetUInt32Value(PLAYER_FIELD_KILLS));

        // update yesterday's contribution
        if (m_lastHonorUpdateTime >= yesterday)
        {
            SetUInt32Value(PLAYER_FIELD_YESTERDAY_CONTRIBUTION, GetUInt32Value(PLAYER_FIELD_TODAY_CONTRIBUTION));

            // this is the first update today, reset today's contribution
            SetUInt32Value(PLAYER_FIELD_TODAY_CONTRIBUTION, 0);
            SetUInt32Value(PLAYER_FIELD_KILLS, MAKE_PAIR32(0, kills_today));
        }
        else
        {
            // no honor/kills yesterday or today, reset
            SetUInt32Value(PLAYER_FIELD_YESTERDAY_CONTRIBUTION, 0);
            SetUInt32Value(PLAYER_FIELD_KILLS, 0);
        }
    }

    m_lastHonorUpdateTime = now;
}

/// Calculate the amount of honor gained based on the victim
/// and the size of the group for which the honor is divided
/// An exact honor value can also be given (overriding the calcs)
bool Player::RewardHonor(Unit* uVictim, uint32 groupsize, float honor)
{
    // do not reward honor in arenas, but enable onkill spellproc
    if (InArena())
    {
        if (!uVictim || uVictim == this || uVictim->GetTypeId() != TYPEID_PLAYER)
        {
            return false;
        }

        if (GetBGTeam() == (reinterpret_cast<Player*>(uVictim))->GetBGTeam())
        {
            return false;
        }

        return true;
    }

    // 'Inactive' this aura prevents the player from gaining honor points and battleground tokens
    if (GetDummyAura(SPELL_AURA_PLAYER_INACTIVE))
    {
        return false;
    }

    ObjectGuid victim_guid;
    uint32 victim_rank = 0;

    // need call before fields update to have chance move yesterday data to appropriate fields before today data change.
    UpdateHonorFields();

    if (honor <= 0)
    {
        if (!uVictim || uVictim == this || uVictim->HasAuraType(SPELL_AURA_NO_PVP_CREDIT))
        {
            return false;
        }

        victim_guid = uVictim->GetObjectGuid();

        if (uVictim->GetTypeId() == TYPEID_PLAYER)
        {
            Player* pVictim = reinterpret_cast<Player*>(uVictim);

            if (GetTeam() == pVictim->GetTeam() && !sWorld.IsFFAPvPRealm())
            {
                return false;
            }

            float f = 1;                                    // need for total kills (?? need more info)
            uint32 k_grey = 0;
            uint32 k_level = getLevel();
            uint32 v_level = pVictim->getLevel();

            {
                // PLAYER_CHOSEN_TITLE VALUES DESCRIPTION
                //  [0]      Just name
                //  [1..14]  Alliance honor titles and player name
                //  [15..28] Horde honor titles and player name
                //  [29..38] Other title and player name
                //  [39+]    Nothing
                uint32 victim_title = pVictim->GetUInt32Value(PLAYER_CHOSEN_TITLE);
                // Get Killer titles, CharTitlesEntry::bit_index
                // Ranks:
                //  title[1..14]  -> rank[5..18]
                //  title[15..28] -> rank[5..18]
                //  title[other]  -> 0
                if (victim_title == 0)
                {
                    victim_guid.Clear();                    // Don't show HK: <rank> message, only log.
                }
                else if (victim_title < 15)
                {
                    victim_rank = victim_title + 4;
                }
                else if (victim_title < 29)
                {
                    victim_rank = victim_title - 14 + 4;
                }
                else
                {
                    victim_guid.Clear();                    // Don't show HK: <rank> message, only log.
                }
            }

            k_grey = MaNGOS::XP::GetGrayLevel(k_level);

            if (v_level <= k_grey)
            {
                return false;
            }

            float diff_level = (k_level == k_grey) ? 1 : ((float(v_level) - float(k_grey)) / (float(k_level) - float(k_grey)));

            int32 v_rank = 1;                               // need more info

            honor = ((f * diff_level * (190 + v_rank * 10)) / 6);
            honor *= float(k_level) / 70.0f;                // factor of dependence on levels of the killer

            // count the number of playerkills in one day
            ApplyModUInt32Value(PLAYER_FIELD_KILLS, 1, true);
            // and those in a lifetime
            ApplyModUInt32Value(PLAYER_FIELD_LIFETIME_HONORABLE_KILLS, 1, true);
            UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_EARN_HONORABLE_KILL);
            UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HK_CLASS, pVictim->getClass());
            UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HK_RACE, pVictim->getRace());
        }
        else
        {
            Creature* cVictim = reinterpret_cast<Creature*>(uVictim);

            if (!cVictim->IsRacialLeader())
            {
                return false;
            }

            honor = 100;                                    // ??? need more info
            victim_rank = 19;                               // HK: Leader
        }
    }

    if (uVictim != NULL)
    {
        honor *= sWorld.getConfig(CONFIG_FLOAT_RATE_HONOR);
        honor *= (GetMaxPositiveAuraModifier(SPELL_AURA_MOD_HONOR_GAIN) + 100.0f) / 100.0f;

        if (groupsize > 1)
        {
            honor /= groupsize;
        }

        honor *= (((float)urand(8, 12)) / 10);              // approx honor: 80% - 120% of real honor
    }

    // honor - for show honor points in log
    // victim_guid - for show victim name in log
    // victim_rank [1..4]  HK: <dishonored rank>
    // victim_rank [5..19] HK: <alliance\horde rank>
    // victim_rank [0,20+] HK: <>
    WorldPacket data(SMSG_PVP_CREDIT, 4 + 8 + 4);
    data << uint32(honor);
    data << ObjectGuid(victim_guid);
    data << uint32(victim_rank);
    GetSession()->SendPacket(&data);

    // add honor points
    ModifyHonorPoints(int32(honor));

    ApplyModUInt32Value(PLAYER_FIELD_TODAY_CONTRIBUTION, uint32(honor), true);
    return true;
}

void Player::SetHonorPoints(uint32 value)
{
    if (value > sWorld.getConfig(CONFIG_UINT32_MAX_HONOR_POINTS))
    {
        value = sWorld.getConfig(CONFIG_UINT32_MAX_HONOR_POINTS);
    }

    SetUInt32Value(PLAYER_FIELD_HONOR_CURRENCY, value);
}

void Player::SetArenaPoints(uint32 value)
{
    if (value > sWorld.getConfig(CONFIG_UINT32_MAX_ARENA_POINTS))
    {
        value = sWorld.getConfig(CONFIG_UINT32_MAX_ARENA_POINTS);
    }

    SetUInt32Value(PLAYER_FIELD_ARENA_CURRENCY, value);
}

void Player::ModifyHonorPoints(int32 value)
{
    int32 newValue = (int32)GetHonorPoints() + value;

    if (newValue < 0)
    {
        newValue = 0;
    }

    SetHonorPoints(newValue);
}

void Player::ModifyArenaPoints(int32 value)
{
    int32 newValue = (int32)GetArenaPoints() + value;

    if (newValue < 0)
    {
        newValue = 0;
    }

    SetArenaPoints(newValue);
}


/**
 * @brief Updates area-specific player state and auras.
 *
 * @param newArea The new area identifier.
 */
void Player::UpdateArea(uint32 newArea)
{
    m_areaUpdateId    = newArea;

    AreaTableEntry const* area = GetAreaEntryByAreaID(newArea);

    // FFA_PVP flags are area and not zone id dependent
    // so apply them accordingly
    if (area && (area->flags & AREA_FLAG_ARENA))
    {
        if (!isGameMaster())
        {
            SetFFAPvP(true);
        }
    }
    else
    {
        // remove ffa flag only if not ffapvp realm
        // removal in sanctuaries and capitals is handled in zone update
        if (IsFFAPvP() && !sWorld.IsFFAPvPRealm())
        {
            SetFFAPvP(false);
        }
    }

    if (area)
    {
        // Dalaran restricted flight zone
        if ((area->flags & AREA_FLAG_CANNOT_FLY) && IsFreeFlying() && !isGameMaster() && !HasAura(58600))
        {
            CastSpell(this, 58600, true);                   // Restricted Flight Area
        }

        // TODO: implement wintergrasp parachute when battle in progress
        /* if ((area->flags & AREA_FLAG_OUTDOOR_PVP) && IsFreeFlying() && <WINTERGRASP_BATTLE_IN_PROGRESS> && !isGameMaster())
            CastSpell(this, 58730, true); */
    }

    UpdateAreaDependentAuras();
}

/**
 * @brief Checks whether the player is eligible to interact with a capture point.
 *
 * @return True if the player can use the capture point; otherwise, false.
 */
bool Player::CanUseCapturePoint()
{
    return IsAlive() &&                                     // living
           !HasStealthAura() &&                             // not stealthed
           !HasInvisibilityAura() &&                        // visible
           (IsPvP() || sWorld.IsPvPRealm()) &&
           !HasMovementFlag(MOVEFLAG_FLYING) &&
           !IsTaxiFlying() &&
           !isGameMaster();
}

/**
 * @brief Updates zone and area state after the player changes location.
 *
 * @param newZone The new zone identifier.
 * @param newArea The new area identifier.
 */
void Player::UpdateZone(uint32 newZone, uint32 newArea)
{
    /* If we're trying to update into a zone that doesn't exist, just return */
    AreaTableEntry const* zone = GetAreaEntryByAreaID(newZone);
    if (!zone)
    {
        return;
    }

    /* If we're moving into a different zone */
    if (m_zoneUpdateId != newZone)
    {
        // handle outdoor pvp zones
        sOutdoorPvPMgr.HandlePlayerLeaveZone(this, m_zoneUpdateId);
        sOutdoorPvPMgr.HandlePlayerEnterZone(this, newZone);

        SendInitWorldStates(newZone, newArea);              // only if really enters to new zone, not just area change, works strange...

        if (sWorld.getConfig(CONFIG_BOOL_WEATHER))
        {
            Weather* wth = GetMap()->GetWeatherSystem()->FindOrCreateWeather(newZone);
            wth->SendWeatherUpdateToPlayer(this);
        }
    }

    // Used by Eluna
#ifdef ENABLE_ELUNA
    if (Eluna* e = GetEluna())
    {
        e->OnUpdateZone(this, newZone, newArea);
    }
#endif /* ENABLE_ELUNA */

    m_zoneUpdateId    = newZone;
    m_zoneUpdateTimer = ZONE_UPDATE_INTERVAL;

    // zone changed, so area changed as well, update it
    UpdateArea(newArea);

    // in PvP, any not controlled zone (except zone->team == 6, default case)
    // in PvE, only opposition team capital
    switch (zone->team)
    {
        case AREATEAM_ALLY:
            pvpInfo.inHostileArea = GetTeam() != ALLIANCE && (sWorld.IsPvPRealm() || zone->flags & AREA_FLAG_CAPITAL);
            break;
        case AREATEAM_HORDE:
            pvpInfo.inHostileArea = GetTeam() != HORDE && (sWorld.IsPvPRealm() || zone->flags & AREA_FLAG_CAPITAL);
            break;
        case AREATEAM_NONE:
            // overwrite for battlegrounds, maybe batter some zone flags but current known not 100% fit to this
            pvpInfo.inHostileArea = sWorld.IsPvPRealm() || InBattleGround();
            break;
        default:                                            // 6 in fact
            pvpInfo.inHostileArea = false;
            break;
    }

    if (pvpInfo.inHostileArea)                              // in hostile area
    {
        // Defer the PvP flag while a DK intro flyover is in progress so its flag
        // sound does not play mid-cinematic; ApplyDeferredIntroPvP() (called from
        // HandleCompleteCinematic) sets it at the cinematic's end instead.
        if ((!IsPvP() || pvpInfo.endTimer != 0) && !IsCinematicIntroActive())
        {
            UpdatePvP(true, true);
        }
    }
    else                                                    // in friendly area
    {
        if (IsPvP() && !HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_IN_PVP) && pvpInfo.endTimer == 0)
        {
            pvpInfo.endTimer = time(0); // start toggle-off
        }
    }

    if (zone->flags & AREA_FLAG_SANCTUARY)                  // in sanctuary
    {
        SetByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_SANCTUARY);
        if (sWorld.IsFFAPvPRealm())
        {
            SetFFAPvP(false);
        }
    }
    else
    {
        RemoveByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_SANCTUARY);
    }

    if (zone->flags & AREA_FLAG_CAPITAL)                    // in capital city
    {
        SetRestType(REST_TYPE_IN_CITY);
    }
    else if (HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING) && GetRestType() != REST_TYPE_IN_TAVERN)
    {
        // resting and not in tavern (leave city then); tavern leave handled in CheckAreaExploreAndOutdoor
        SetRestType(REST_TYPE_NO);
    }

    // remove items with area/map limitations (delete only for alive player to allow back in ghost mode)
    // if player resurrected at teleport this will be applied in resurrect code
    if (IsAlive())
    {
        DestroyZoneLimitedItem(true, newZone);
    }

    // check some item equip limitations (in result lost CanTitanGrip at talent reset, for example)
    AutoUnequipOffhandIfNeed();

    // recent client version not send leave/join channel packets for built-in local channels
    UpdateLocalChannels(newZone);

    // group update
    if (GetGroup())
    {
        SetGroupUpdateFlag(GROUP_UPDATE_FLAG_ZONE);
    }

    UpdateZoneDependentAuras();
    UpdateZoneDependentPets();
}


//---------------------------------------------------------//









/**
 * (un-)Apply item spells triggered at adding item to inventory ITEM_SPELLTRIGGER_ON_STORE
 *
 * @param item  added/removed item to/from inventory
 * @param apply (un-)apply spell affects.
 *
 * Note: item moved from slot to slot in 2 steps RemoveItem and StoreItem/EquipItem
 * In result function not called in RemoveItem for prevent unexpected re-apply auras from related spells
 * with duration reset and etc. Instead unapply done in StoreItem/EquipItem and in specialized
 * functions for item final remove/destroy from inventory. If new RemoveItem calls added need be sure that
 * function will call after it in some way if need.
 */











/**
 * @brief Converts the player's battleground corpse into lootable bones after insignia removal.
 *
 * @param looterPlr The player removing the insignia.
 */
void Player::RemovedInsignia(Player* looterPlr)
{
    if (!GetBattleGroundId())
    {
        return;
    }

    // If not released spirit, do it !
    if (m_deathTimer > 0)
    {
        m_deathTimer = 0;
        BuildPlayerRepop();
        RepopAtGraveyard();
    }

    Corpse* corpse = GetCorpse();
    if (!corpse)
    {
        return;
    }

    // We have to convert player corpse to bones, not to be able to resurrect there
    // SpawnCorpseBones isn't handy, 'cos it saves player while he in BG
    Corpse* bones = sObjectAccessor.ConvertCorpseForPlayer(GetObjectGuid(), true);
    if (!bones)
    {
        return;
    }

    // Now we must make bones lootable, and send player loot
    bones->SetFlag(CORPSE_FIELD_DYNAMIC_FLAGS, CORPSE_DYNFLAG_LOOTABLE);

    // We store the level of our player in the gold field
    // We retrieve this information at Player::SendLoot()
    bones->loot.gold = getLevel();
    bones->lootRecipient = looterPlr;
    looterPlr->SendLoot(bones->GetObjectGuid(), LOOT_INSIGNIA);
}

/**
 * @brief Sends a loot release response for a loot object.
 *
 * @param guid The GUID of the released loot source.
 */
void Player::SendLootRelease(ObjectGuid guid)
{
    WorldPacket data(SMSG_LOOT_RELEASE_RESPONSE, (8 + 1));
    data << guid;
    data << uint8(1);
    SendDirectMessage(&data);
}

/**
 * @brief Opens and sends loot contents for a supported loot source.
 *
 * @param guid The GUID of the loot source.
 * @param loot_type The requested loot interaction type.
 */
void Player::SendLoot(ObjectGuid guid, LootType loot_type)
{
    if (ObjectGuid lootGuid = GetLootGuid())
    {
        m_session->DoLootRelease(lootGuid);
    }

    Loot* loot = NULL;
    PermissionTypes permission = ALL_PERMISSION;

    DEBUG_LOG("Player::SendLoot");
    switch (guid.GetHigh())
    {
        case HIGHGUID_GAMEOBJECT:
        {
            DEBUG_LOG("       IS_GAMEOBJECT_GUID(guid)");
            GameObject* go = GetMap()->GetGameObject(guid);

            // not check distance for GO in case owned GO (fishing bobber case, for example)
            // And permit out of range GO with no owner in case fishing hole
            if (!go || (loot_type != LOOT_FISHINGHOLE && (loot_type != LOOT_FISHING && loot_type != LOOT_FISHING_FAIL || go->GetOwnerGuid() != GetObjectGuid()) && !go->IsWithinDistInMap(this, INTERACTION_DISTANCE)))
            {
                SendLootRelease(guid);
                return;
            }

            GameObjectInfo const* goInfo = go->GetGOInfo();

            loot = &go->loot;

            Player* recipient = go->GetLootRecipient();
            if (!recipient)
            {
                go->SetLootRecipient(this);
                recipient = this;
            }

            // generate loot only if ready for open and spawned in world and not already looted once.
            if (go->getLootState() == GO_READY && go->isSpawned())
            {
                uint32 lootid = goInfo->GetLootId();
                if ((go->GetEntry() == BG_AV_OBJECTID_MINE_N || go->GetEntry() == BG_AV_OBJECTID_MINE_S))
                {
                    if (BattleGround* bg = GetBattleGround())
                        if (bg->GetTypeID(true) == BATTLEGROUND_AV)
                            if (!(((BattleGroundAV*)bg)->PlayerCanDoMineQuest(go->GetEntry(), GetTeam())))
                            {
                                SendLootRelease(guid);
                                return;
                            }
                }

                loot->clear();
                switch (loot_type)
                {
                        // Entry 0 in fishing loot template used for store junk fish loot at fishing fail it junk allowed by config option
                        // this is overwrite fishinghole loot for example
                    case LOOT_FISHING_FAIL:
                        loot->FillLoot(0, LootTemplates_Fishing, this, true);
                        break;
                    case LOOT_FISHING:
                        uint32 zone, subzone;
                        go->GetZoneAndAreaId(zone, subzone);
                        // if subzone loot exist use it
                        if (!loot->FillLoot(subzone, LootTemplates_Fishing, this, true, (subzone != zone)) && subzone != zone)
                            // else use zone loot (if zone diff. from subzone, must exist in like case)
                        {
                            loot->FillLoot(zone, LootTemplates_Fishing, this, true);
                        }
                        break;
                    default:
                        if (!lootid)
                        {
                            break;
                        }
                        DEBUG_LOG("       send normal GO loot");

                        loot->FillLoot(lootid, LootTemplates_Gameobject, this, false);
                        loot->generateMoneyLoot(goInfo->MinMoneyLoot, goInfo->MaxMoneyLoot);

                        if (go->GetGoType() == GAMEOBJECT_TYPE_CHEST && goInfo->chest.groupLootRules)
                        {
                            if (Group* group = go->GetGroupLootRecipient())
                            {
                                group->UpdateLooterGuid(go, true);

                                switch (group->GetLootMethod())
                                {
                                    case GROUP_LOOT:
                                        // GroupLoot delete items over threshold (threshold even not implemented), and roll them. Items with quality<threshold, round robin
                                        group->GroupLoot(go, loot);
                                        permission = GROUP_PERMISSION;
                                        break;
                                    case NEED_BEFORE_GREED:
                                        group->NeedBeforeGreed(go, loot);
                                        permission = GROUP_PERMISSION;
                                        break;
                                    case MASTER_LOOT:
                                        group->MasterLoot(go, loot);
                                        permission = MASTER_PERMISSION;
                                        break;
                                    default:
                                        break;
                                }
                            }
                        }
                        break;
                }
                go->SetLootState(GO_ACTIVATED);
            }

            if (go->getLootState() == GO_ACTIVATED && go->GetGoType() == GAMEOBJECT_TYPE_CHEST && go->GetGOInfo()->chest.groupLootRules)
            {
                if (Group* group = go->GetGroupLootRecipient())
                {
                    if (group == GetGroup())
                    {
                        if (group->GetLootMethod() == FREE_FOR_ALL)
                        {
                            permission = ALL_PERMISSION;
                        }
                        else if (group->GetLooterGuid() == GetObjectGuid())
                        {
                            if (group->GetLootMethod() == MASTER_LOOT)
                            {
                                permission = MASTER_PERMISSION;
                            }
                            else
                            {
                                permission = ALL_PERMISSION;
                            }
                        }
                        else
                        {
                            permission = GROUP_PERMISSION;
                        }
                    }
                    else
                    {
                        permission = NONE_PERMISSION;
                    }
                }
                else if (recipient == this)
                {
                    permission = ALL_PERMISSION;
                }
                else
                {
                    permission = NONE_PERMISSION;
                }
            }
            break;
        }
        case HIGHGUID_ITEM:
        {
            Item* item = GetItemByGuid(guid);

            if (!item)
            {
                SendLootRelease(guid);
                return;
            }

            permission = OWNER_PERMISSION;

            loot = &item->loot;

            if (!item->HasGeneratedLoot())
            {
                item->loot.clear();
                ItemPrototype const* itemProto = item->GetProto();

                switch (loot_type)
                {
                    case LOOT_DISENCHANTING:
                        loot->FillLoot(itemProto->DisenchantID, LootTemplates_Disenchant, this, true);
                        item->SetLootState(ITEM_LOOT_TEMPORARY);
                        break;
                    case LOOT_PROSPECTING:
                        loot->FillLoot(item->GetEntry(), LootTemplates_Prospecting, this, true);
                        item->SetLootState(ITEM_LOOT_TEMPORARY);
                        break;
                    case LOOT_MILLING:
                        loot->FillLoot(item->GetEntry(), LootTemplates_Milling, this, true);
                        item->SetLootState(ITEM_LOOT_TEMPORARY);
                        break;
                    default:
                        loot->FillLoot(item->GetEntry(), LootTemplates_Item, this, true, itemProto->MaxMoneyLoot == 0);
                        loot->generateMoneyLoot(itemProto->MinMoneyLoot, itemProto->MaxMoneyLoot);
                        item->SetLootState(ITEM_LOOT_CHANGED);
                        break;
                }
            }
            break;
        }
        case HIGHGUID_CORPSE:                               // remove insignia
        {
            Corpse* bones = GetMap()->GetCorpse(guid);

            if (!bones || !((loot_type == LOOT_CORPSE) || (loot_type == LOOT_INSIGNIA)) || (bones->GetType() != CORPSE_BONES))
            {
                SendLootRelease(guid);
                return;
            }

            loot = &bones->loot;

            if (!bones->lootForBody)
            {
                bones->lootForBody = true;
                uint32 pLevel = bones->loot.gold;
                bones->loot.clear();
                if (GetBattleGround() && GetBattleGround()->GetTypeID(true) == BATTLEGROUND_AV)
                {
                    loot->FillLoot(0, LootTemplates_Creature, this, false);
                }
                // It may need a better formula
                // Now it works like this: lvl10: ~6copper, lvl70: ~9silver
                bones->loot.gold = (uint32)(urand(50, 150) * 0.016f * pow(((float)pLevel) / 5.76f, 2.5f) * sWorld.getConfig(CONFIG_FLOAT_RATE_DROP_MONEY));
            }

            if (bones->lootRecipient != this)
            {
                permission = NONE_PERMISSION;
            }
            else
            {
                permission = OWNER_PERMISSION;
            }
            break;
        }
        case HIGHGUID_UNIT:
        case HIGHGUID_VEHICLE:
        {
            Creature* creature = GetMap()->GetCreature(guid);

            // must be in range and creature must be alive for pickpocket and must be dead for another loot
            if (!creature || creature->IsAlive() != (loot_type == LOOT_PICKPOCKETING) || !creature->IsWithinDistInMap(this, INTERACTION_DISTANCE))
            {
                SendLootRelease(guid);
                return;
            }

            if (loot_type == LOOT_PICKPOCKETING && IsFriendlyTo(creature))
            {
                SendLootRelease(guid);
                return;
            }

            loot = &creature->loot;
            CreatureInfo const* creatureInfo = creature->GetCreatureInfo();

            if (loot_type == LOOT_PICKPOCKETING)
            {
                if (!creature->lootForPickPocketed)
                {
                    creature->lootForPickPocketed = true;
                    loot->clear();

                    if (uint32 lootid = creatureInfo->PickpocketLootId)
                    {
                        loot->FillLoot(lootid, LootTemplates_Pickpocketing, this, false);
                    }

                    // Generate extra money for pick pocket loot
                    const uint32 a = urand(0, creature->getLevel() / 2);
                    const uint32 b = urand(0, getLevel() / 2);
                    loot->gold = uint32(10 * (a + b) * sWorld.getConfig(CONFIG_FLOAT_RATE_DROP_MONEY));
                    permission = OWNER_PERMISSION;
                }
            }
            else
            {
                // the player whose group may loot the corpse
                Player* recipient = creature->GetLootRecipient();
                if (!recipient)
                {
                    creature->SetLootRecipient(this);
                    recipient = this;
                }

                if (creature->lootForPickPocketed)
                {
                    creature->lootForPickPocketed = false;
                    loot->clear();
                }

                if (!creature->lootForBody)
                {
                    creature->lootForBody = true;
                    loot->clear();

                    if (uint32 lootid = creatureInfo->LootId)
                    {
                        loot->FillLoot(lootid, LootTemplates_Creature, recipient, false);
                    }

                    loot->generateMoneyLoot(creatureInfo->MinLootGold, creatureInfo->MaxLootGold);

                    if (Group* group = creature->GetGroupLootRecipient())
                    {
                        group->UpdateLooterGuid(creature, true);

                        switch (group->GetLootMethod())
                        {
                            case GROUP_LOOT:
                                // GroupLoot delete items over threshold (threshold even not implemented), and roll them. Items with quality<threshold, round robin
                                group->GroupLoot(creature, loot);
                                break;
                            case NEED_BEFORE_GREED:
                                group->NeedBeforeGreed(creature, loot);
                                break;
                            case MASTER_LOOT:
                                group->MasterLoot(creature, loot);
                                break;
                            default:
                                break;
                        }
                    }
                }

                // possible only if creature->lootForBody && loot->empty() at spell cast check
                if (loot_type == LOOT_SKINNING)
                {
                    if (!creature->lootForSkin)
                    {
                        creature->lootForSkin = true;
                        loot->clear();
                        loot->FillLoot(creatureInfo->SkinningLootId, LootTemplates_Skinning, this, false);

                        // let reopen skinning loot if will closed.
                        if (!loot->empty())
                        {
                            creature->SetUInt32Value(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
                            creature->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_FLAG_SKINNABLE);
                        }

                        permission = OWNER_PERMISSION;

                        // Inform Instance Data, may be scripts related to OnSkinning like The Beast in UBRS
                        if (InstanceData* mapInstance = creature->GetInstanceData())
                        {
                            mapInstance->OnCreatureLooted(creature, LOOT_SKINNING);
                        }
                    }
                }
                // set group rights only for loot_type != LOOT_SKINNING
                else
                {
                    if (Group* group = creature->GetGroupLootRecipient())
                    {
                        if (group == GetGroup())
                        {
                            if (group->GetLootMethod() == FREE_FOR_ALL)
                            {
                                permission = ALL_PERMISSION;
                            }
                            else if (group->GetLooterGuid() == GetObjectGuid())
                            {
                                if (group->GetLootMethod() == MASTER_LOOT)
                                {
                                    permission = MASTER_PERMISSION;
                                }
                                else
                                {
                                    permission = ALL_PERMISSION;
                                }
                            }
                            else
                            {
                                permission = GROUP_PERMISSION;
                            }
                        }
                        else
                        {
                            permission = NONE_PERMISSION;
                        }
                    }
                    else if (recipient == this)
                    {
                        permission = OWNER_PERMISSION;
                    }
                    else
                    {
                        permission = NONE_PERMISSION;
                    }
                }
            }
            break;
        }
        default:
        {
            sLog.outError("%s is unsupported for looting.", guid.GetString().c_str());
            return;
        }
    }

    SetLootGuid(guid);

    // LOOT_INSIGNIA and LOOT_FISHINGHOLE unsupported by client
    switch (loot_type)
    {
        case LOOT_INSIGNIA:     loot_type = LOOT_SKINNING; break;
        case LOOT_FISHING_FAIL: loot_type = LOOT_FISHING; break;
        case LOOT_FISHINGHOLE:  loot_type = LOOT_FISHING; break;
        default: break;
    }

    // need know merged fishing/corpse loot type for achievements
    loot->loot_type = loot_type;

    WorldPacket data(SMSG_LOOT_RESPONSE, (9 + 50));         // we guess size
    data << ObjectGuid(guid);
    data << uint8(loot_type);
    data << LootView(*loot, this, permission);
    SendDirectMessage(&data);

    // add 'this' player as one of the players that are looting 'loot'
    if (permission != NONE_PERMISSION)
    {
        loot->AddLooter(GetObjectGuid());
    }

    if (loot_type == LOOT_CORPSE && !guid.IsItem())
    {
        SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_LOOTING);
    }
}

/**
 * @brief Notifies the client that money was removed from the current loot.
 */
void Player::SendNotifyLootMoneyRemoved()
{
    WorldPacket data(SMSG_LOOT_CLEAR_MONEY, 0);
    GetSession()->SendPacket(&data);
}

/**
 * @brief Notifies the client that a loot slot was removed.
 *
 * @param lootSlot The loot slot index that was removed.
 */
void Player::SendNotifyLootItemRemoved(uint8 lootSlot)
{
    WorldPacket data(SMSG_LOOT_REMOVED, 1);
    data << uint8(lootSlot);
    GetSession()->SendPacket(&data);
}

/**
 * @brief Sends a single world state update to the client.
 *
 * @param Field The world state field identifier.
 * @param Value The new field value.
 */
void Player::SendUpdateWorldState(uint32 Field, uint32 Value)
{
    WorldPacket data(SMSG_UPDATE_WORLD_STATE, 8);
    data << Field;
    data << Value;
    GetSession()->SendPacket(&data);
}

/**
 * @brief Sends the initial world state set for the player's current zone.
 *
 * @param zoneid The zone identifier used to select world states.
 */
void Player::SendInitWorldStates(uint32 zoneid, uint32 areaid)
{
    // data depends on zoneid/mapid...
    BattleGround* bg = GetBattleGround();
    uint32 mapid = GetMapId();

    DEBUG_LOG("Sending SMSG_INIT_WORLD_STATES to Map:%u, Zone: %u", mapid, zoneid);

    uint32 count = 0;                                       // count of world states in packet

    WorldPacket data(SMSG_INIT_WORLD_STATES, (4 + 4 + 4 + 2 + 8 * 8)); // guess
    data << uint32(mapid);                                  // mapid
    data << uint32(zoneid);                                 // zone id
    data << uint32(areaid);                                 // area id, new 2.1.0
    size_t count_pos = data.wpos();
    data << uint16(0);                                      // count of uint64 blocks, placeholder

    // Current arena season
    FillInitialWorldState(data, count, 0xC77, sWorld.getConfig(CONFIG_UINT32_ARENA_SEASON_ID));
    // Previous arena season
    FillInitialWorldState(data, count, 0xF3D, sWorld.getConfig(CONFIG_UINT32_ARENA_SEASON_PREVIOUS_ID));
    // 0 - Battle for Wintergrasp in progress, 1 - otherwise
    FillInitialWorldState(data, count, 0xED9, 1);
    // Time when next Battle for Wintergrasp starts
    FillInitialWorldState(data, count, 0x1102, uint32(time(NULL) + 9000));

    switch (zoneid)
    {
        case 139:                                           // Eastern Plaguelands
            if (OutdoorPvP* outdoorPvP = sOutdoorPvPMgr.GetScript(zoneid))
            {
                outdoorPvP->FillInitialWorldStates(data, count);
            }
            break;
        case 1377:                                          // Silithus
        case 3483:                                          // Hellfire Peninsula
        case 3518:                                          // Nagrand
        case 3519:                                          // Terokkar Forest
        case 3521:                                          // Zangarmarsh
            if (OutdoorPvP* outdoorPvP = sOutdoorPvPMgr.GetScript(zoneid))
            {
                outdoorPvP->FillInitialWorldStates(data, count);
            }
            break;
        case 2597:                                          // AV
            if (bg && bg->GetTypeID(true) == BATTLEGROUND_AV)
            {
                bg->FillInitialWorldStates(data, count);
            }
            break;
        case 3277:                                          // WS
            if (bg && bg->GetTypeID(true) == BATTLEGROUND_WS)
            {
                bg->FillInitialWorldStates(data, count);
            }
            break;
        case 3358:                                          // AB
            if (bg && bg->GetTypeID(true) == BATTLEGROUND_AB)
            {
                bg->FillInitialWorldStates(data, count);
            }
            break;
        case 3820:                                          // EY
            if (bg && bg->GetTypeID(true) == BATTLEGROUND_EY)
            {
                bg->FillInitialWorldStates(data, count);
            }
            break;
        case 3698:                                          // Nagrand Arena
            if (bg && bg->GetTypeID(true) == BATTLEGROUND_NA)
            {
                bg->FillInitialWorldStates(data, count);
            }
            break;
        case 3702:                                          // Blade's Edge Arena
            if (bg && bg->GetTypeID(true) == BATTLEGROUND_BE)
            {
                bg->FillInitialWorldStates(data, count);
            }
            break;
        case 3968:                                          // Ruins of Lordaeron
            if (bg && bg->GetTypeID(true) == BATTLEGROUND_RL)
            {
                bg->FillInitialWorldStates(data, count);
            }
            break;
    }

    FillBGWeekendWorldStates(data, count);

    data.put<uint16>(count_pos, count);                     // set actual world state amount

    GetSession()->SendPacket(&data);
}



/**
 * @brief Consumes and returns the rested experience bonus for an XP award.
 *
 * @param xp The base experience amount being awarded.
 * @return The rested bonus experience amount.
 */
uint32 Player::GetXPRestBonus(uint32 xp)
{
    uint32 rested_bonus = (uint32)GetRestBonus();           // xp for each rested bonus

    if (rested_bonus > xp)                                  // max rested_bonus == xp or (r+x) = 200% xp
    {
        rested_bonus = xp;
    }

    SetRestBonus(GetRestBonus() - rested_bonus);

    DETAIL_LOG("Player gain %u xp (+ %u Rested Bonus). Rested points=%f", xp + rested_bonus, rested_bonus, GetRestBonus());
    return rested_bonus;
}

/**
 * @brief Sends a bind point confirmation prompt to the client.
 *
 * @param guid The binder NPC GUID.
 */
void Player::SetBindPoint(ObjectGuid guid)
{
    WorldPacket data(SMSG_BINDER_CONFIRM, 8);
    data << ObjectGuid(guid);
    GetSession()->SendPacket(&data);
}

/**
 * @brief Sends a talent reset confirmation prompt to the client.
 *
 * @param guid The trainer or source GUID for the confirmation.
 */
void Player::SendTalentWipeConfirm(ObjectGuid guid)
{
    WorldPacket data(MSG_TALENT_WIPE_CONFIRM, (8 + 4));
    data << ObjectGuid(guid);
    data << uint32(resetTalentsCost());
    GetSession()->SendPacket(&data);
}

/**
 * @brief Sends a pet talent reset confirmation prompt to the client.
 */
void Player::SendPetSkillWipeConfirm()
{
    Pet* pet = GetPet();
    if (!pet)
    {
        return;
    }
    WorldPacket data(SMSG_PET_UNLEARN_CONFIRM, (8 + 4));
    data << ObjectGuid(pet->GetObjectGuid());
    data << uint32(pet->resetTalentsCost());
    GetSession()->SendPacket(&data);
}

/*********************************************************/
/***                    STORAGE SYSTEM                 ***/
/*********************************************************/

/**
 * @brief Updates a visible virtual weapon slot and consumes temporary enchant charges when needed.
 *
 * @param i The virtual slot index.
 * @param item The item to reflect in the virtual slot.
 */













/*********************************************************/
/***                    GOSSIP SYSTEM                  ***/
/*********************************************************/


/**
 * @brief Applies cooldown lockouts to spells in the specified school mask.
 *
 * @param idSchoolMask The spell school mask to prohibit.
 * @param unTimeMs The prohibition duration in milliseconds.
 */
void Player::ProhibitSpellSchool(SpellSchoolMask idSchoolMask, uint32 unTimeMs)
{
    // last check 2.0.10
    WorldPacket data(SMSG_SPELL_COOLDOWN, 8 + 1 + m_spells.size() * 8);
    data << GetObjectGuid();
    data << uint8(0x0);                                     // flags (0x1, 0x2)
    time_t curTime = time(NULL);
    for (PlayerSpellMap::const_iterator itr = m_spells.begin(); itr != m_spells.end(); ++itr)
    {
        if (itr->second.state == PLAYERSPELL_REMOVED)
        {
            continue;
        }
        uint32 unSpellId = itr->first;
        SpellEntry const* spellInfo = sSpellStore.LookupEntry(unSpellId);
        MANGOS_ASSERT(spellInfo);

        // Not send cooldown for this spells
        if (spellInfo->HasAttribute(SPELL_ATTR_DISABLED_WHILE_ACTIVE))
        {
            continue;
        }

        if ((idSchoolMask & GetSpellSchoolMask(spellInfo)) && GetSpellCooldownDelay(unSpellId) < unTimeMs)
        {
            data << uint32(unSpellId);
            data << uint32(unTimeMs);                       // in m.secs
            AddSpellCooldown(unSpellId, 0, curTime + unTimeMs / IN_MILLISECONDS);
        }
    }
    GetSession()->SendPacket(&data);
}

/**
 * @brief Reinitializes player combat data for the current shapeshift form.
 *
 * @param reapplyMods True when reapplying modifiers without a real form change.
 */
void Player::InitDataForForm(bool reapplyMods)
{
    ShapeshiftForm form = GetShapeshiftForm();

    SpellShapeshiftFormEntry const* ssEntry = sSpellShapeshiftFormStore.LookupEntry(form);
    if (ssEntry && ssEntry->attackSpeed)
    {
        SetAttackTime(BASE_ATTACK, ssEntry->attackSpeed);
        SetAttackTime(OFF_ATTACK, ssEntry->attackSpeed);
        SetAttackTime(RANGED_ATTACK, BASE_ATTACK_TIME);
    }
    else
    {
        SetRegularAttackTime();
    }

    switch (form)
    {
        case FORM_CAT:
        {
            if (GetPowerType() != POWER_ENERGY)
            {
                SetPowerType(POWER_ENERGY);
            }
            break;
        }
        case FORM_BEAR:
        case FORM_DIREBEAR:
        {
            if (GetPowerType() != POWER_RAGE)
            {
                SetPowerType(POWER_RAGE);
            }
            break;
        }
        default:                                            // 0, for example
        {
            ChrClassesEntry const* cEntry = sChrClassesStore.LookupEntry(getClass());
            if (cEntry && cEntry->powerType < MAX_POWERS && uint32(GetPowerType()) != cEntry->powerType)
            {
                SetPowerType(Powers(cEntry->powerType));
            }
            break;
        }
    }

    // update auras at form change, ignore this at mods reapply (.reset stats/etc) when form not change.
    if (!reapplyMods)
    {
        UpdateEquipSpellsAtFormChange();
    }

    UpdateAttackPowerAndDamage();
    UpdateAttackPowerAndDamage(true);
}

/**
 * @brief Initializes the player's native and current display identifiers.
 */
void Player::InitDisplayIds()
{
    PlayerInfo const* info = sObjectMgr.GetPlayerInfo(getRace(), getClass());
    if (!info)
    {
        sLog.outError("Player %u has incorrect race/class pair. Can't init display ids.", GetGUIDLow());
        return;
    }

    // reset scale before reapply auras
    SetObjectScale(DEFAULT_OBJECT_SCALE);

    uint8 gender = getGender();
    switch (gender)
    {
        case GENDER_FEMALE:
            SetDisplayId(info->displayId_f);
            SetNativeDisplayId(info->displayId_f);
            break;
        case GENDER_MALE:
            SetDisplayId(info->displayId_m);
            SetNativeDisplayId(info->displayId_m);
            break;
        default:
            sLog.outError("Invalid gender %u for player", gender);
            return;
    }
}

void Player::TakeExtendedCost(uint32 extendedCostId, uint32 count)
{
    ItemExtendedCostEntry const* extendedCost = sItemExtendedCostStore.LookupEntry(extendedCostId);

    if (extendedCost->reqhonorpoints)
    {
        ModifyHonorPoints(-int32(extendedCost->reqhonorpoints * count));
    }
    if (extendedCost->reqarenapoints)
    {
        ModifyArenaPoints(-int32(extendedCost->reqarenapoints * count));
    }

    for (uint8 i = 0; i < MAX_EXTENDED_COST_ITEMS; ++i)
    {
        if (extendedCost->reqitem[i])
        {
            DestroyItemCount(extendedCost->reqitem[i], extendedCost->reqitemcount[i] * count, true);
        }
    }
}

// Return true is the bought item has a max count to force refresh of window by caller
bool Player::BuyItemFromVendorSlot(ObjectGuid vendorGuid, uint32 vendorslot, uint32 item, uint8 count, uint8 bag, uint8 slot)
{
    // cheating attempt
    if (count < 1)
    {
        count = 1;
    }

    // cheating attempt
    if (bag != NULL_BAG && bag != INVENTORY_SLOT_BAG_0 && slot > MAX_BAG_SIZE && slot != NULL_SLOT)
    {
        return false;
    }

    if (!IsAlive())
    {
        return false;
    }

    ItemPrototype const* pProto = ObjectMgr::GetItemPrototype(item);
    if (!pProto)
    {
        SendBuyError(BUY_ERR_CANT_FIND_ITEM, NULL, item, 0);
        return false;
    }

    Creature* pCreature = GetNPCIfCanInteractWith(vendorGuid, UNIT_NPC_FLAG_VENDOR);
    if (!pCreature)
    {
        DEBUG_LOG("WORLD: BuyItemFromVendor - %s not found or you can't interact with him.", vendorGuid.GetString().c_str());
        SendBuyError(BUY_ERR_DISTANCE_TOO_FAR, NULL, item, 0);
        return false;
    }

    VendorItemData const* vItems = pCreature->GetVendorItems();
    VendorItemData const* tItems = pCreature->GetVendorTemplateItems();
    if ((!vItems || vItems->Empty()) && (!tItems || tItems->Empty()))
    {
        SendBuyError(BUY_ERR_CANT_FIND_ITEM, pCreature, item, 0);
        return false;
    }

    uint32 vCount = vItems ? vItems->GetItemCount() : 0;
    uint32 tCount = tItems ? tItems->GetItemCount() : 0;

    if (vendorslot >= vCount + tCount)
    {
        SendBuyError(BUY_ERR_CANT_FIND_ITEM, pCreature, item, 0);
        return false;
    }

    VendorItem const* crItem = vendorslot < vCount ? vItems->GetItem(vendorslot) : tItems->GetItem(vendorslot - vCount);
    if (!crItem)                                            // store diff item (cheating)
    {
        SendBuyError(BUY_ERR_CANT_FIND_ITEM, pCreature, item, 0);
        return false;
    }

    if (crItem->item != item)                               // store diff item (cheating or special convert)
    {
        bool converted = false;

        // possible item converted for BoA case
        ItemPrototype const* crProto = ObjectMgr::GetItemPrototype(crItem->item);
        if (crProto->Flags & ITEM_FLAG_BOA && crProto->RequiredReputationFaction &&
                uint32(GetReputationRank(crProto->RequiredReputationFaction)) >= crProto->RequiredReputationRank)
            converted = (sObjectMgr.GetItemConvert(crItem->item, getRaceMask()) != 0);

        if (!converted)
        {
            SendBuyError(BUY_ERR_CANT_FIND_ITEM, pCreature, item, 0);
            return false;
        }
    }

    uint32 totalCount = pProto->BuyCount * count;

    // check current item amount if it limited
    if (crItem->maxcount != 0)
    {
        if (pCreature->GetVendorItemCurrentCount(crItem) < totalCount)
        {
            SendBuyError(BUY_ERR_ITEM_ALREADY_SOLD, pCreature, item, 0);
            return false;
        }
    }

    if (uint32(GetReputationRank(pProto->RequiredReputationFaction)) < pProto->RequiredReputationRank)
    {
        SendBuyError(BUY_ERR_REPUTATION_REQUIRE, pCreature, item, 0);
        return false;
    }

    if (uint32 extendedCostId = crItem->ExtendedCost)
    {
        ItemExtendedCostEntry const* iece = sItemExtendedCostStore.LookupEntry(extendedCostId);
        if (!iece)
        {
            sLog.outError("Item %u have wrong ExtendedCost field value %u", pProto->ItemId, extendedCostId);
            return false;
        }

        // honor points price
        if (GetHonorPoints() < (iece->reqhonorpoints * count))
        {
            SendEquipError(EQUIP_ERR_NOT_ENOUGH_HONOR_POINTS, NULL, NULL);
            return false;
        }

        // arena points price
        if (GetArenaPoints() < (iece->reqarenapoints * count))
        {
            SendEquipError(EQUIP_ERR_NOT_ENOUGH_ARENA_POINTS, NULL, NULL);
            return false;
        }

        // item base price
        for (uint8 i = 0; i < MAX_EXTENDED_COST_ITEMS; ++i)
        {
            if (iece->reqitem[i] && !HasItemCount(iece->reqitem[i], iece->reqitemcount[i] * count))
            {
                SendEquipError(EQUIP_ERR_VENDOR_MISSING_TURNINS, NULL, NULL);
                return false;
            }
        }

        // check for personal arena rating requirement
        if (GetMaxPersonalArenaRatingRequirement(iece->reqarenaslot) < iece->reqpersonalarenarating)
        {
            // probably not the proper equip err
            SendEquipError(EQUIP_ERR_CANT_EQUIP_RANK, NULL, NULL);
            return false;
        }
    }

    if (crItem->conditionId && !isGameMaster() && !sObjectMgr.IsPlayerMeetToCondition(crItem->conditionId, this, pCreature->GetMap(), pCreature, CONDITION_FROM_VENDOR))
    {
        SendBuyError(BUY_ERR_CANT_FIND_ITEM, pCreature, item, 0);
        return false;
    }

    uint32 price = (crItem->ExtendedCost == 0 || pProto->Flags2 & ITEM_FLAG2_EXT_COST_REQUIRES_GOLD) ? pProto->BuyPrice * count : 0;

    // reputation discount
    if (price)
    {
        price = uint32(floor(price * GetReputationPriceDiscount(pCreature)));
    }

    if (GetMoney() < price)
    {
        SendBuyError(BUY_ERR_NOT_ENOUGHT_MONEY, pCreature, item, 0);
        return false;
    }

    Item* pItem = NULL;

    if ((bag == NULL_BAG && slot == NULL_SLOT) || IsInventoryPos(bag, slot))
    {
        ItemPosCountVec dest;
        InventoryResult msg = CanStoreNewItem(bag, slot, dest, item, totalCount);
        if (msg != EQUIP_ERR_OK)
        {
            SendEquipError(msg, NULL, NULL, item);
            return false;
        }

        ModifyMoney(-int32(price));

        if (crItem->ExtendedCost)
        {
            TakeExtendedCost(crItem->ExtendedCost, count);
        }

        pItem = StoreNewItem(dest, item, true);
    }
    else if (IsEquipmentPos(bag, slot))
    {
        if (totalCount != 1)
        {
            SendEquipError(EQUIP_ERR_ITEM_CANT_BE_EQUIPPED, NULL, NULL);
            return false;
        }

        uint16 dest;
        InventoryResult msg = CanEquipNewItem(slot, dest, item, false);
        if (msg != EQUIP_ERR_OK)
        {
            SendEquipError(msg, NULL, NULL, item);
            return false;
        }

        ModifyMoney(-int32(price));

        if (crItem->ExtendedCost)
        {
            TakeExtendedCost(crItem->ExtendedCost, count);
        }

        pItem = EquipNewItem(dest, item, true);

        if (pItem)
        {
            AutoUnequipOffhandIfNeed();
        }
    }
    else
    {
        SendEquipError(EQUIP_ERR_ITEM_DOESNT_GO_TO_SLOT, NULL, NULL);
        return false;
    }

    if (!pItem)
    {
        return false;
    }

    uint32 new_count = pCreature->UpdateVendorItemCurrentCount(crItem, totalCount);

    WorldPacket data(SMSG_BUY_ITEM, 8 + 4 + 4 + 4);
    data << pCreature->GetObjectGuid();
    data << uint32(vendorslot + 1);                 // numbered from 1 at client
    data << uint32(crItem->maxcount > 0 ? new_count : 0xFFFFFFFF);
    data << uint32(count);
    GetSession()->SendPacket(&data);

    SendNewItem(pItem, totalCount, true, false, false);

    return crItem->maxcount != 0;
}

uint32 Player::GetMaxPersonalArenaRatingRequirement(uint32 minarenaslot)
{
    // returns the maximal personal arena rating that can be used to purchase items requiring this condition
    // the personal rating of the arena team must match the required limit as well
    // so return max[in arenateams](min(personalrating[teamtype], teamrating[teamtype]))
    uint32 max_personal_rating = 0;
    for (int i = minarenaslot; i < MAX_ARENA_SLOT; ++i)
    {
        if (ArenaTeam* at = sObjectMgr.GetArenaTeamById(GetArenaTeamId(i)))
        {
            uint32 p_rating = GetArenaPersonalRating(i);
            uint32 t_rating = at->GetRating();
            p_rating = p_rating < t_rating ? p_rating : t_rating;
            if (max_personal_rating < p_rating)
            {
                max_personal_rating = p_rating;
            }
        }
    }
    return max_personal_rating;
}

/**
 * @brief Updates the invalid-instance homebind timer and teleports when it expires.
 *
 * @param time The elapsed update time in milliseconds.
 */
void Player::UpdateHomebindTime(uint32 time)
{
    // GMs never get homebind timer online
    if (m_InstanceValid || isGameMaster())
    {
        if (m_HomebindTimer)                                // instance valid, but timer not reset
        {
            // hide reminder
            WorldPacket data(SMSG_RAID_GROUP_ONLY, 4 + 4);
            data << uint32(0);
            data << uint32(ERR_RAID_GROUP_NONE);            // error used only when timer = 0
            GetSession()->SendPacket(&data);
        }
        // instance is valid, reset homebind timer
        m_HomebindTimer = 0;
    }
    else if (m_HomebindTimer > 0)
    {
        if (time >= m_HomebindTimer)
        {
            // teleport to nearest graveyard
            RepopAtGraveyard();
        }
        else
        {
            m_HomebindTimer -= time;
        }
    }
    else
    {
        // instance is invalid, start homebind timer
        m_HomebindTimer = 60000;
        // send message to player
        WorldPacket data(SMSG_RAID_GROUP_ONLY, 4 + 4);
        data << uint32(m_HomebindTimer);
        data << uint32(ERR_RAID_GROUP_NONE);                // error used only when timer = 0
        GetSession()->SendPacket(&data);
        DEBUG_LOG("PLAYER: Player '%s' (GUID: %u) will be teleported to homebind in 60 seconds", GetName(), GetGUIDLow());
    }
}

/**
 * @brief Sets or clears the player's PvP state with timeout-aware handling.
 *
 * @param state True to enable PvP; false to disable it.
 * @param ovrride True to bypass the delayed PvP timeout behavior.
 */
void Player::UpdatePvP(bool state, bool ovrride)
{
    if (!state || ovrride)
    {
        SetPvP(state);
        pvpInfo.endTimer = 0;
    }
    else
    {
        if (pvpInfo.endTimer != 0)
        {
            pvpInfo.endTimer = time(NULL);
        }
        else
        {
            SetPvP(state);
        }
    }
}

/**
 * @brief Applies personal and category cooldowns for a spell cast.
 *
 * @param spellInfo The spell entry that triggered the cooldown.
 * @param itemId The casting item entry, if any.
 * @param spell The active spell instance.
 * @param infinityCooldown True to apply a long-lived cooldown marker.
 */
void Player::AddSpellAndCategoryCooldowns(SpellEntry const* spellInfo, uint32 itemId, Spell* spell, bool infinityCooldown)
{
    // init cooldown values
    uint32 cat   = 0;
    int32 rec    = -1;
    int32 catrec = -1;

    // some special item spells without correct cooldown in SpellInfo
    // cooldown information stored in item prototype
    // This used in same way in WorldSession::HandleItemQuerySingleOpcode data sending to client.

    if (itemId)
    {
        if (ItemPrototype const* proto = ObjectMgr::GetItemPrototype(itemId))
        {
            for (int idx = 0; idx < MAX_ITEM_PROTO_SPELLS; ++idx)
            {
                if (proto->Spells[idx].SpellId == spellInfo->Id)
                {
                    cat    = proto->Spells[idx].SpellCategory;
                    rec    = proto->Spells[idx].SpellCooldown;
                    catrec = proto->Spells[idx].SpellCategoryCooldown;
                    break;
                }
            }
        }
    }

    // if no cooldown found above then base at DBC data
    if (rec < 0 && catrec < 0)
    {
        cat = spellInfo->Category;
        rec = spellInfo->RecoveryTime;
        catrec = spellInfo->CategoryRecoveryTime;
    }

    time_t curTime = time(NULL);

    time_t catrecTime;
    time_t recTime;

    // overwrite time for selected category
    if (infinityCooldown)
    {
        // use +MONTH as infinity mark for spell cooldown (will checked as MONTH/2 at save ans skipped)
        // but not allow ignore until reset or re-login
        catrecTime = catrec > 0 ? curTime + infinityCooldownDelay : 0;
        recTime    = rec    > 0 ? curTime + infinityCooldownDelay : catrecTime;
    }
    else
    {
        // shoot spells used equipped item cooldown values already assigned in GetAttackTime(RANGED_ATTACK)
        // prevent 0 cooldowns set by another way
        if (rec <= 0 && catrec <= 0 && (cat == 76 || (IsAutoRepeatRangedSpell(spellInfo) && spellInfo->Id != SPELL_ID_AUTOSHOT)))
        {
            rec = GetAttackTime(RANGED_ATTACK);
        }

        // Now we have cooldown data (if found any), time to apply mods
        if (rec > 0)
        {
            ApplySpellMod(spellInfo->Id, SPELLMOD_COOLDOWN, rec);
        }

        if (catrec > 0)
        {
            ApplySpellMod(spellInfo->Id, SPELLMOD_COOLDOWN, catrec);
        }

        // replace negative cooldowns by 0
        if (rec < 0) rec = 0;
        {
            if (catrec < 0) catrec = 0;
        }

        // no cooldown after applying spell mods
        if (rec == 0 && catrec == 0)
        {
            return;
        }

        catrecTime = catrec ? curTime + catrec / IN_MILLISECONDS : 0;
        recTime    = rec ? curTime + rec / IN_MILLISECONDS : catrecTime;
    }

    // self spell cooldown
    if (recTime > 0)
    {
        AddSpellCooldown(spellInfo->Id, itemId, recTime);
    }

    // category spells
    if (cat && catrec > 0)
    {
        SpellCategoryStore::const_iterator i_scstore = sSpellCategoryStore.find(cat);
        if (i_scstore != sSpellCategoryStore.end())
        {
            for (SpellCategorySet::const_iterator i_scset = i_scstore->second.begin(); i_scset != i_scstore->second.end(); ++i_scset)
            {
                if (*i_scset == spellInfo->Id)              // skip main spell, already handled above
                {
                    continue;
                }

                AddSpellCooldown(*i_scset, itemId, catrecTime);
            }
        }
    }
}

/**
 * @brief Stores a cooldown entry for a spell.
 *
 * @param spellid The spell identifier.
 * @param itemid The associated item identifier, if any.
 * @param end_time The server time when the cooldown ends.
 */
void Player::AddSpellCooldown(uint32 spellid, uint32 itemid, time_t end_time)
{
    SpellCooldown sc;
    sc.end = end_time;
    sc.itemid = itemid;
    m_spellCooldowns[spellid] = sc;
}

/**
 * @brief Applies cooldowns and notifies the client about a spell cooldown event.
 *
 * @param spellInfo The spell entry that triggered the cooldown.
 * @param itemId The associated item entry, if any.
 * @param spell The active spell instance.
 */
void Player::SendCooldownEvent(SpellEntry const* spellInfo, uint32 itemId, Spell* spell)
{
    // start cooldowns at server side, if any
    AddSpellAndCategoryCooldowns(spellInfo, itemId, spell);

    // Send activate cooldown timer (possible 0) at client side
    WorldPacket data(SMSG_COOLDOWN_EVENT, (4 + 8));
    data << uint32(spellInfo->Id);
    data << GetObjectGuid();
    SendDirectMessage(&data);
}

void Player::UpdatePotionCooldown(Spell* spell)
{
    // no potion used in combat or still in combat
    if (!m_lastPotionId || IsInCombat())
    {
        return;
    }

    // Call not from spell cast, send cooldown event for item spells if no in combat
    if (!spell)
    {
        // spell/item pair let set proper cooldown (except nonexistent charged spell cooldown spellmods for potions)
        if (ItemPrototype const* proto = ObjectMgr::GetItemPrototype(m_lastPotionId))
            for (int idx = 0; idx < 5; ++idx)
            {
                if (proto->Spells[idx].SpellId && proto->Spells[idx].SpellTrigger == ITEM_SPELLTRIGGER_ON_USE)
                    if (SpellEntry const* spellInfo = sSpellStore.LookupEntry(proto->Spells[idx].SpellId))
                    {
                        SendCooldownEvent(spellInfo, m_lastPotionId);
                    }
            }
    }
    // from spell cases (m_lastPotionId set in Spell::SendSpellCooldown)
    else
    {
        SendCooldownEvent(spell->m_spellInfo, m_lastPotionId, spell);
    }

    m_lastPotionId = 0;
}





/**
 * @brief Checks whether this player should be visible to another player in grid range.
 *
 * @param pl The observing player.
 * @return True if this player should be visible; otherwise, false.
 */
bool Player::IsVisibleInGridForPlayer(Player* pl) const
{
    // gamemaster in GM mode see all, including ghosts
    if (pl->isGameMaster() && GetSession()->GetSecurity() <= pl->GetSession()->GetSecurity())
    {
        return true;
    }

    // player see dead player/ghost from own group/raid
    if (IsInSameRaidWith(pl))
    {
        return true;
    }

    // Live player see live player or dead player with not realized corpse
    if (pl->IsAlive() || pl->m_deathTimer > 0)
    {
        return IsAlive() || m_deathTimer > 0;
    }

    // Ghost see other friendly ghosts, that's for sure
    if (!(IsAlive() || m_deathTimer > 0) && IsFriendlyTo(pl))
    {
        return true;
    }

    // Dead player see live players near own corpse
    if (IsAlive())
    {
        if (Corpse* corpse = pl->GetCorpse())
        {
            // 20 - aggro distance for same level, 25 - max additional distance if player level less that creature level
            if (corpse->IsWithinDistInMap(this, (20 + 25) * sWorld.getConfig(CONFIG_FLOAT_RATE_CREATURE_AGGRO)))
            {
                return true;
            }
        }
    }

    // and not see any other
    return false;
}

/**
 * @brief Checks whether this player should appear in global player visibility contexts.
 *
 * @param u The player attempting to see this player.
 * @return True if this player is globally visible; otherwise, false.
 */
bool Player::IsVisibleGloballyFor(Player* u) const
{
    if (!u)
    {
        return false;
    }

    // Always can see self
    if (u == this)
    {
        return true;
    }

    // Visible units, always are visible for all players
    if (GetVisibility() == VISIBILITY_ON)
    {
        return true;
    }

    // GMs are visible for higher gms (or players are visible for gms)
    if (u->GetSession()->GetSecurity() > SEC_PLAYER)
    {
        return GetSession()->GetSecurity() <= u->GetSession()->GetSecurity();
    }

    // non faction visibility non-breakable for non-GMs
    if (GetVisibility() == VISIBILITY_OFF)
    {
        return false;
    }

    // non-gm stealth/invisibility not hide from global player lists
    return true;
}

template<class T>
inline void BeforeVisibilityDestroy(T* /*t*/, Player* /*p*/)
{
}

template<>
inline void BeforeVisibilityDestroy<Creature>(Creature* t, Player* p)
{
    if (p->GetPetGuid() == t->GetObjectGuid() && (t->IsPet()))
    {
        (reinterpret_cast<Pet*>(t))->Unsummon(PET_SAVE_REAGENTS);
    }
}

/**
 * @brief Updates visibility of a single world object for the player.
 *
 * @param viewPoint The viewpoint used for visibility checks.
 * @param target The target object whose visibility is being updated.
 */
void Player::UpdateVisibilityOf(WorldObject const* viewPoint, WorldObject* target)
{
    if (HaveAtClient(target))
    {
        if (!target->IsVisibleForInState(this, viewPoint, true))
        {
            ObjectGuid t_guid = target->GetObjectGuid();

            if (target->GetTypeId() == TYPEID_UNIT)
            {
                BeforeVisibilityDestroy<Creature>(reinterpret_cast<Creature*>(target), this);

                // at remove from map (destroy) show kill animation (in different out of range/stealth case)
                target->DestroyForPlayer(this, !target->IsInWorld() && (reinterpret_cast<Creature*>(target))->IsDead());
            }
            else
            {
                target->DestroyForPlayer(this);
            }

            m_clientGUIDs.erase(t_guid);

            DEBUG_FILTER_LOG(LOG_FILTER_VISIBILITY_CHANGES, "UpdateVisibilityOf: %s out of range for player %u. Distance = %f", t_guid.GetString().c_str(), GetGUIDLow(), GetDistance(target));
        }
    }
    else
    {
        if (target->IsVisibleForInState(this, viewPoint, false))
        {
            target->SendCreateUpdateToPlayer(this);
            if (target->GetTypeId() != TYPEID_GAMEOBJECT || !(reinterpret_cast<GameObject*>(target))->IsTransport())
            {
                m_clientGUIDs.insert(target->GetObjectGuid());
            }

            DEBUG_FILTER_LOG(LOG_FILTER_VISIBILITY_CHANGES, "UpdateVisibilityOf: %s is visible now for player %u. Distance = %f", target->GetGuidStr().c_str(), GetGUIDLow(), GetDistance(target));

            // target aura duration for caster show only if target exist at caster client
            // send data at target visibility change (adding to client)
            if (target != this && target->isType(TYPEMASK_UNIT))
            {
                SendAurasForTarget(reinterpret_cast<Unit*>(target));
            }
        }
    }
}

template<class T>
inline void UpdateVisibilityOf_helper(GuidSet& s64, T* target)
{
    s64.insert(target->GetObjectGuid());
}

template<>
inline void UpdateVisibilityOf_helper(GuidSet& s64, GameObject* target)
{
    if (!target->IsTransport())
    {
        s64.insert(target->GetObjectGuid());
    }
}

template<class T>
void Player::UpdateVisibilityOf(WorldObject const* viewPoint, T* target, UpdateData& data, std::set<WorldObject*>& visibleNow)
{
    if (HaveAtClient(target))
    {
        if (!target->IsVisibleForInState(this, viewPoint, true))
        {
            BeforeVisibilityDestroy<T>(target, this);

            ObjectGuid t_guid = target->GetObjectGuid();

            target->BuildOutOfRangeUpdateBlock(&data);
            m_clientGUIDs.erase(t_guid);

            DEBUG_FILTER_LOG(LOG_FILTER_VISIBILITY_CHANGES, "UpdateVisibilityOf(TemplateV): %s is out of range for %s. Distance = %f", t_guid.GetString().c_str(), GetGuidStr().c_str(), GetDistance(target));
        }
    }
    else
    {
        if (target->IsVisibleForInState(this, viewPoint, false))
        {
            visibleNow.insert(target);
            target->BuildCreateUpdateBlockForPlayer(&data, this);
            UpdateVisibilityOf_helper(m_clientGUIDs, target);

            DEBUG_FILTER_LOG(LOG_FILTER_VISIBILITY_CHANGES, "UpdateVisibilityOf(TemplateV): %s is visible now for %s. Distance = %f", target->GetGuidStr().c_str(), GetGuidStr().c_str(), GetDistance(target));
        }
    }
}

template void Player::UpdateVisibilityOf(WorldObject const* viewPoint, Player*        target, UpdateData& data, std::set<WorldObject*>& visibleNow);
template void Player::UpdateVisibilityOf(WorldObject const* viewPoint, Creature*      target, UpdateData& data, std::set<WorldObject*>& visibleNow);
template void Player::UpdateVisibilityOf(WorldObject const* viewPoint, Corpse*        target, UpdateData& data, std::set<WorldObject*>& visibleNow);
template void Player::UpdateVisibilityOf(WorldObject const* viewPoint, GameObject*    target, UpdateData& data, std::set<WorldObject*>& visibleNow);
template void Player::UpdateVisibilityOf(WorldObject const* viewPoint, DynamicObject* target, UpdateData& data, std::set<WorldObject*>& visibleNow);

void Player::SetPhaseMask(uint32 newPhaseMask, bool update)
{
    // GM-mode have mask PHASEMASK_ANYWHERE always
    if (isGameMaster())
    {
        newPhaseMask = PHASEMASK_ANYWHERE;
    }

    // phase auras normally not expected at BG but anyway better check
    if (BattleGround* bg = GetBattleGround())
    {
        bg->EventPlayerDroppedFlag(this);
    }

    Unit::SetPhaseMask(newPhaseMask, update);
    GetSession()->SendSetPhaseShift(GetPhaseMask());
}

/**
 * @brief Initializes the number of primary professions the player may learn.
 */
void Player::InitPrimaryProfessions()
{
    SetFreePrimaryProfessions(sWorld.getConfig(CONFIG_UINT32_MAX_PRIMARY_TRADE_SKILL));
}



void Player::SendInitialPacketsBeforeAddToMap()
{
    GetSocial()->SendSocialList();

    // Homebind
    WorldPacket data(SMSG_BINDPOINTUPDATE, 5 * 4);
    data << m_homebindX << m_homebindY << m_homebindZ;
    data << (uint32) m_homebindMapId;
    data << (uint32) m_homebindAreaId;
    GetSession()->SendPacket(&data);

    // SMSG_SET_PROFICIENCY
    // SMSG_SET_PCT_SPELL_MODIFIER
    // SMSG_SET_FLAT_SPELL_MODIFIER

    SendTalentsInfoData(false);

    data.Initialize(SMSG_INSTANCE_DIFFICULTY, 4 + 4);
    data << uint32(GetMap()->GetDifficulty());
    data << uint32(0);
    GetSession()->SendPacket(&data);

    SendInitialSpells();

    data.Initialize(SMSG_SEND_UNLEARN_SPELLS, 4);
    data << uint32(0);                                      // count, for(count) uint32;
    GetSession()->SendPacket(&data);

    SendInitialActionButtons();
    m_reputationMgr.SendInitialReputations();

    if (!IsAlive())
    {
        SendCorpseReclaimDelay(true);
    }

    SendInitWorldStates(GetZoneId(), GetAreaId());

    SendEquipmentSetList();

    m_achievementMgr.SendAllAchievementData();

    data.Initialize(SMSG_LOGIN_SETTIMESPEED, 4 + 4 + 4);
    data << uint32(secsToTimeBitFields(sWorld.GetGameTime()));
    data << (float)0.01666667f;                             // game speed
    data << uint32(0);                                      // added in 3.1.2
    GetSession()->SendPacket(&data);

    // SMSG_TALENTS_INFO x 2 for pet (unspent points and talents in separate packets...)
    // SMSG_PET_GUIDS
    // SMSG_POWER_UPDATE

    // set fly flag if in fly form or taxi flight to prevent visually drop at ground in showup moment
    if (IsFreeFlying() || IsTaxiFlying())
    {
        m_movementInfo.AddMovementFlag(MOVEFLAG_FLYING);
    }

    SetMover(this);
}

/**
 * @brief Sends map-dependent initialization packets after the player is added to the world.
 */
void Player::SendInitialPacketsAfterAddToMap()
{
    // update zone
    uint32 newzone, newarea;
    GetZoneAndAreaId(newzone, newarea);
    UpdateZone(newzone, newarea);                           // also call SendInitWorldStates();

    ResetTimeSync();
    SendTimeSync();

    CastSpell(this, 836, true);                             // LOGINEFFECT

    // set some aura effects that send packet to player client after add player to map
    // SendMessageToSet not send it to player not it map, only for aura that not changed anything at re-apply
    // same auras state lost at far teleport, send it one more time in this case also
    static const AuraType auratypes[] =
    {
        SPELL_AURA_MOD_FEAR,     SPELL_AURA_TRANSFORM,                 SPELL_AURA_WATER_WALK,
        SPELL_AURA_FEATHER_FALL, SPELL_AURA_HOVER,                     SPELL_AURA_SAFE_FALL,
        SPELL_AURA_FLY,          SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED,  SPELL_AURA_NONE
    };
    for (AuraType const* itr = &auratypes[0]; itr && itr[0] != SPELL_AURA_NONE; ++itr)
    {
        Unit::AuraList const& auraList = GetAurasByType(*itr);
        if (!auraList.empty())
        {
            auraList.front()->ApplyModifier(true, true);
        }
    }

    if (HasAuraType(SPELL_AURA_MOD_STUN) || HasAuraType(SPELL_AURA_MOD_ROOT))
    {
        SetRoot(true);
    }

    SendAurasForTarget(this);
    SendEnchantmentDurations();                             // must be after add to map
    SendItemDurations();                                    // must be after add to map
}



/**
 * @brief Applies the default equip cooldown for item use spells.
 *
 * @param pItem The item whose on-use spells should receive equip cooldowns.
 */
void Player::ApplyEquipCooldown(Item* pItem)
{
    if (pItem->GetProto()->Flags & ITEM_FLAG_NO_EQUIP_COOLDOWN)
    {
        return;
    }

    for (int i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
    {
        _Spell const& spellData = pItem->GetProto()->Spells[i];

        // no spell
        if (!spellData.SpellId)
        {
            continue;
        }

        // wrong triggering type (note: ITEM_SPELLTRIGGER_ON_NO_DELAY_USE not have cooldown)
        if (spellData.SpellTrigger != ITEM_SPELLTRIGGER_ON_USE)
        {
            continue;
        }

        AddSpellCooldown(spellData.SpellId, pItem->GetEntry(), time(NULL) + 30);

        WorldPacket data(SMSG_ITEM_COOLDOWN, 12);
        data << ObjectGuid(pItem->GetObjectGuid());
        data << uint32(spellData.SpellId);
        GetSession()->SendPacket(&data);
    }
}






/**
 * @brief Sends visible aura duration updates for a target to the player.
 *
 * @param target The unit whose aura durations should be sent.
 */
void Player::SendAurasForTarget(Unit* target)
{
    Unit::VisibleAuraMap const& visibleAuras = target->GetVisibleAuras();
    if (visibleAuras.empty())
    {
        return;
    }

    WorldPacket data(SMSG_AURA_UPDATE_ALL);
    data << target->GetPackGUID();

    for (Unit::VisibleAuraMap::const_iterator itr = visibleAuras.begin(); itr != visibleAuras.end(); ++itr)
    {
        SpellAuraHolderConstBounds bounds = target->GetSpellAuraHolderBounds(itr->second);
        for (SpellAuraHolderMap::const_iterator iter = bounds.first; iter != bounds.second; ++iter)
        {
            iter->second->BuildUpdatePacket(data);
        }
    }

    GetSession()->SendPacket(&data);
}




/**
 * @brief Calculates the vendor price discount earned from reputation and rank.
 *
 * @param pCreature The vendor creature.
 * @return The price multiplier applied to vendor costs.
 */
float Player::GetReputationPriceDiscount(Creature const* pCreature) const
{
    FactionTemplateEntry const* vendor_faction = pCreature->getFactionTemplateEntry();
    if (!vendor_faction || !vendor_faction->faction)
    {
        return 1.0f;
    }

    ReputationRank rank = GetReputationRank(vendor_faction->faction);
    if (rank <= REP_NEUTRAL)
    {
        return 1.0f;
    }

    return 1.0f - 0.05f * (rank - REP_NEUTRAL);
}

/*
 * Check spell availability for training base at SkillLineAbility/SkillRaceClassInfo data.
 * Checked allowed race/class and dependent from race/class allowed min level
 *
 * @param spell_id  checked spell id
 * @param pReqlevel if arg provided then function work in view mode (level check not applied but detected minlevel returned to var by arg pointer.
                    if arg not provided then considered train action mode and level checked
 * @return          true if spell available for show in trainer list (with skip level check) or training.
 */
bool Player::IsSpellFitByClassAndRace(uint32 spell_id, uint32* pReqlevel /*= NULL*/) const
{
    uint32 racemask  = getRaceMask();
    uint32 classmask = getClassMask();

    SkillLineAbilityMapBounds bounds = sSpellMgr.GetSkillLineAbilityMapBounds(spell_id);
    if (bounds.first == bounds.second)
    {
        return true;
    }

    for (SkillLineAbilityMap::const_iterator _spell_idx = bounds.first; _spell_idx != bounds.second; ++_spell_idx)
    {
        SkillLineAbilityEntry const* abilityEntry = _spell_idx->second;
        // skip wrong race skills
        if (abilityEntry->racemask && (abilityEntry->racemask & racemask) == 0)
        {
            continue;
        }

        // skip wrong class skills
        if (abilityEntry->classmask && (abilityEntry->classmask & classmask) == 0)
        {
            continue;
        }

        SkillRaceClassInfoMapBounds raceBounds = sSpellMgr.GetSkillRaceClassInfoMapBounds(abilityEntry->skillId);
        for (SkillRaceClassInfoMap::const_iterator itr = raceBounds.first; itr != raceBounds.second; ++itr)
        {
            SkillRaceClassInfoEntry const* skillRCEntry = itr->second;
            if ((skillRCEntry->raceMask & racemask) && (skillRCEntry->classMask & classmask))
            {
                if (skillRCEntry->flags & ABILITY_SKILL_NONTRAINABLE)
                {
                    return false;
                }

                if (pReqlevel)                              // show trainers list case
                {
                    if (skillRCEntry->reqLevel)
                    {
                        *pReqlevel = skillRCEntry->reqLevel;
                        return true;
                    }
                }
                else                                        // check availble case at train
                {
                    if (skillRCEntry->reqLevel && getLevel() < skillRCEntry->reqLevel)
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    return false;
}


/**
 * @brief Accepts or declines a pending summon and teleports when valid.
 *
 * @param agree True to accept the summon; false to decline it.
 */
void Player::SummonIfPossible(bool agree)
{
    if (!agree)
    {
        m_summon_expire = 0;
        return;
    }

    // expire and auto declined
    if (m_summon_expire < time(NULL))
    {
        return;
    }

    // stop taxi flight at summon
    if (IsTaxiFlying())
    {
        GetMotionMaster()->MovementExpired();
        m_taxi.ClearTaxiDestinations();
    }

    // drop flag at summon
    // this code can be reached only when GM is summoning player who carries flag, because player should be immune to summoning spells when he carries flag
    if (BattleGround* bg = GetBattleGround())
    {
        bg->EventPlayerDroppedFlag(this);
    }

    m_summon_expire = 0;

    GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_ACCEPTED_SUMMONINGS, 1);

    TeleportTo(m_summon_mapid, m_summon_x, m_summon_y, m_summon_z, GetOrientation());
}

/**
 * @brief Removes an item from the list of items with active duration tracking.
 *
 * @param item The item to stop tracking.
 */
void Player::RemoveItemDurations(Item* item)
{
    for (ItemDurationList::iterator itr = m_itemDuration.begin(); itr != m_itemDuration.end(); ++itr)
    {
        if (*itr == item)
        {
            m_itemDuration.erase(itr);
            break;
        }
    }
}

/**
 * @brief Adds an item to the list of items with active duration tracking.
 *
 * @param item The item to track.
 */
void Player::AddItemDurations(Item* item)
{
    if (item->GetUInt32Value(ITEM_FIELD_DURATION))
    {
        m_itemDuration.push_back(item);
        item->SendTimeUpdate(this);
    }
}

/**
 * @brief Automatically unequips the offhand item when a two-handed weapon requires it.
 */
void Player::AutoUnequipOffhandIfNeed()
{
    Item* offItem = GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
    if (!offItem)
    {
        return;
    }

    // need unequip offhand for 2h-weapon without TitanGrip (in any from hands)
    if ((CanDualWield() || offItem->GetProto()->InventoryType == INVTYPE_SHIELD || offItem->GetProto()->InventoryType == INVTYPE_HOLDABLE) &&
            (CanTitanGrip() || (offItem->GetProto()->InventoryType != INVTYPE_2HWEAPON && !IsTwoHandUsed())))
    {
        return;
    }

    ItemPosCountVec off_dest;
    uint8 off_msg = CanStoreItem(NULL_BAG, NULL_SLOT, off_dest, offItem, false);
    if (off_msg == EQUIP_ERR_OK)
    {
        RemoveItem(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND, true);
        StoreItem(off_dest, offItem, true);
    }
    else
    {
        MoveItemFromInventory(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND, true);
        CharacterDatabase.BeginTransaction();
        offItem->DeleteFromInventoryDB();                   // deletes item from character's inventory
        offItem->SaveToDB();                                // recursive and not have transaction guard into self, item not in inventory and can be save standalone
        CharacterDatabase.CommitTransaction();

        std::string subject = GetSession()->GetMangosString(LANG_NOT_EQUIPPED_ITEM);
        MailDraft(subject, "There's were problems with equipping this item.").AddItem(offItem).SendMailTo(this, MailSender(this, MAIL_STATIONERY_GM), MAIL_CHECK_MASK_COPIED);
    }
}

/**
 * @brief Checks whether the player has an equipped item that satisfies a spell requirement.
 *
 * @param spellInfo The spell entry defining the equipment requirement.
 * @param ignoreItem An equipped item to ignore during the search.
 * @return True if a valid item is equipped; otherwise, false.
 */
bool Player::HasItemFitToSpellReqirements(SpellEntry const* spellInfo, Item const* ignoreItem)
{
    if (spellInfo->EquippedItemClass < 0)
    {
        return true;
    }

    // scan other equipped items for same requirements (mostly 2 daggers/etc)
    // for optimize check 2 used cases only
    switch (spellInfo->EquippedItemClass)
    {
        case ITEM_CLASS_WEAPON:
        {
            for (int i = EQUIPMENT_SLOT_MAINHAND; i < EQUIPMENT_SLOT_TABARD; ++i)
            {
                if (Item* item = GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                    if (item != ignoreItem && item->IsFitToSpellRequirements(spellInfo))
                    {
                        return true;
                    }
            }
            break;
        }
        case ITEM_CLASS_ARMOR:
        {
            // tabard not have dependent spells
            for (int i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_MAINHAND; ++i)
            {
                if (Item* item = GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                    if (item != ignoreItem && item->IsFitToSpellRequirements(spellInfo))
                    {
                        return true;
                    }
            }

            // shields can be equipped to offhand slot
            if (Item* item = GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND))
                if (item != ignoreItem && item->IsFitToSpellRequirements(spellInfo))
                {
                    return true;
                }

            // ranged slot can have some armor subclasses
            if (Item* item = GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_RANGED))
                if (item != ignoreItem && item->IsFitToSpellRequirements(spellInfo))
                {
                    return true;
                }

            break;
        }
        default:
            sLog.outError("HasItemFitToSpellReqirements: Not handled spell requirement for item class %u", spellInfo->EquippedItemClass);
            break;
    }

    return false;
}

/**
 * @brief Checks whether the player may cast a spell without consuming reagents.
 *
 * @param spellInfo The spell being cast.
 * @return True if reagents can be ignored; otherwise, false.
 */
bool Player::CanNoReagentCast(SpellEntry const* spellInfo) const
{
    // don't take reagents for spells with SPELL_ATTR_EX5_NO_REAGENT_WHILE_PREP
    if (spellInfo->HasAttribute(SPELL_ATTR_EX5_NO_REAGENT_WHILE_PREP) && HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PREPARATION))
    {
        return true;
    }

    // Check no reagent use mask
    uint64 noReagentMask_0_1 = GetUInt64Value(PLAYER_NO_REAGENT_COST_1);
    uint32 noReagentMask_2   = GetUInt32Value(PLAYER_NO_REAGENT_COST_1 + 2);
    if (spellInfo->IsFitToFamilyMask(noReagentMask_0_1, noReagentMask_2))
    {
        return true;
    }

    return false;
}

/**
 * @brief Removes auras and interrupts casts that depend on a removed item.
 *
 * @param pItem The item being removed or invalidated.
 */
void Player::RemoveItemDependentAurasAndCasts(Item* pItem)
{
    SpellAuraHolderMap& auras = GetSpellAuraHolderMap();
    for (SpellAuraHolderMap::const_iterator itr = auras.begin(); itr != auras.end();)
    {
        SpellAuraHolder* holder = itr->second;

        // skip passive (passive item dependent spells work in another way) and not self applied auras
        SpellEntry const* spellInfo = holder->GetSpellProto();
        if (holder->IsPassive() ||  holder->GetCasterGuid() != GetObjectGuid())
        {
            ++itr;
            continue;
        }

        // skip if not item dependent or have alternative item
        if (HasItemFitToSpellReqirements(spellInfo, pItem))
        {
            ++itr;
            continue;
        }

        // no alt item, remove aura, restart check
        RemoveAurasDueToSpell(holder->GetId());
        itr = auras.begin();
    }

    // currently casted spells can be dependent from item
    for (uint32 i = 0; i < CURRENT_MAX_SPELL; ++i)
    {
        if (Spell* spell = GetCurrentSpell(CurrentSpellTypes(i)))
            if (spell->getState() != SPELL_STATE_DELAYED && !HasItemFitToSpellReqirements(spell->m_spellInfo, pItem))
            {
                InterruptSpell(CurrentSpellTypes(i));
            }
    }
}

/**
 * @brief Chooses the resurrection spell currently available to the player.
 *
 * @return The resurrection spell identifier, or 0 if none is available.
 */
uint32 Player::GetResurrectionSpellId()
{
    // search priceless resurrection possibilities
    uint32 prio = 0;
    uint32 spell_id = 0;
    AuraList const& dummyAuras = GetAurasByType(SPELL_AURA_DUMMY);
    for (AuraList::const_iterator itr = dummyAuras.begin(); itr != dummyAuras.end(); ++itr)
    {
        // Soulstone Resurrection                           // prio: 3 (max, non death persistent)
        if (prio < 2 && (*itr)->GetSpellProto()->SpellVisual[0] == 99 && (*itr)->GetSpellProto()->SpellIconID == 92)
        {
            switch ((*itr)->GetId())
            {
                case 20707: spell_id =  3026; break;        // rank 1
                case 20762: spell_id = 20758; break;        // rank 2
                case 20763: spell_id = 20759; break;        // rank 3
                case 20764: spell_id = 20760; break;        // rank 4
                case 20765: spell_id = 20761; break;        // rank 5
                case 27239: spell_id = 27240; break;        // rank 6
                case 47883: spell_id = 47882; break;        // rank 7
                default:
                    sLog.outError("Unhandled spell %u: S.Resurrection", (*itr)->GetId());
                    continue;
            }

            prio = 3;
        }
        // Twisting Nether                                  // prio: 2 (max)
        else if ((*itr)->GetId() == 23701 && roll_chance_i(10))
        {
            prio = 2;
            spell_id = 23700;
        }
    }

    // Reincarnation (passive spell)                        // prio: 1
    // Glyph of Renewed Life remove reagent requiremnnt
    if (prio < 1 && HasSpell(20608) && !HasSpellCooldown(21169) && (HasItemCount(17030, 1) || HasAura(58059, EFFECT_INDEX_0)))
    {
        spell_id = 21169;
    }

    return spell_id;
}

// Used in triggers for check "Only to targets that grant experience or honor" req
bool Player::isHonorOrXPTarget(Unit* pVictim) const
{
    uint32 v_level = pVictim->getLevel();
    uint32 k_grey  = MaNGOS::XP::GetGrayLevel(getLevel());

    // Victim level less gray level
    if (v_level <= k_grey)
    {
        return false;
    }

    if (pVictim->GetTypeId() == TYPEID_UNIT)
    {
        Creature* pVictimAsCreature = reinterpret_cast<Creature*>(pVictim);
        if (pVictimAsCreature->IsTotem() ||
            pVictimAsCreature->IsPet() ||
            pVictimAsCreature->GetCreatureInfo()->ExtraFlags & CREATURE_FLAG_EXTRA_NO_XP_AT_KILL)
        {
            return false;
        }
    }
    return true;
}

/**
 * @brief Rewards the player for killing a unit outside group reward distribution.
 *
 * @param pVictim The killed unit.
 */
void Player::RewardSinglePlayerAtKill(Unit* pVictim)
{
    bool PvP = pVictim->IsCharmedOwnedByPlayerOrPlayer();
    uint32 xp = PvP ? 0 : MaNGOS::XP::Gain(this, pVictim);

    // honor can be in PvP and !PvP (racial leader) cases
    RewardHonor(pVictim, 1);

    // xp and reputation only in !PvP case
    if (!PvP)
    {
        RewardReputation(pVictim, 1);
        GiveXP(xp, pVictim);

        if (Pet* pet = GetPet())
        {
            pet->GivePetXP(xp);
        }

        // normal creature (not pet/etc) can be only in !PvP case
        if (pVictim->GetTypeId() == TYPEID_UNIT)
            if (CreatureInfo const* normalInfo = ObjectMgr::GetCreatureTemplate(pVictim->GetEntry()))
            {
                KilledMonster(normalInfo, pVictim->GetObjectGuid());
            }
    }
}

/**
 * @brief Grants event kill credit to the player or nearby group members.
 *
 * @param creature_id The credited creature entry identifier.
 * @param pRewardSource The world object used for distance checks.
 */
void Player::RewardPlayerAndGroupAtEvent(uint32 creature_id, WorldObject* pRewardSource)
{
    MANGOS_ASSERT((!GetGroup() || pRewardSource) && "Player::RewardPlayerAndGroupAtEvent called for Group-Case but no source for range searching provided");

    ObjectGuid creature_guid = pRewardSource && pRewardSource->GetTypeId() == TYPEID_UNIT ? pRewardSource->GetObjectGuid() : ObjectGuid();

    // prepare data for near group iteration
    if (Group* pGroup = GetGroup())
    {
        for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* pGroupGuy = itr->getSource();
            if (!pGroupGuy)
            {
                continue;
            }

            if (!pGroupGuy->IsAtGroupRewardDistance(pRewardSource))
            {
                continue;                                    // member (alive or dead) or his corpse at req. distance
            }

            // quest objectives updated only for alive group member or dead but with not released body
            if (pGroupGuy->IsAlive() || !pGroupGuy->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST))
            {
                pGroupGuy->KilledMonsterCredit(creature_id, creature_guid);
            }
        }
    }
    else                                                    // if (!pGroup)
    {
        KilledMonsterCredit(creature_id, creature_guid);
    }
}

/**
 * @brief Grants quest cast credit to the player or nearby group members.
 *
 * @param pRewardSource The credited creature or gameobject.
 * @param spellid The spell that granted the credit.
 */
void Player::RewardPlayerAndGroupAtCast(WorldObject* pRewardSource, uint32 spellid)
{
    // prepare data for near group iteration
    if (Group* pGroup = GetGroup())
    {
        for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* pGroupGuy = itr->getSource();
            if (!pGroupGuy)
            {
                continue;
            }

            if (!pGroupGuy->IsAtGroupRewardDistance(pRewardSource))
            {
                continue;                                // member (alive or dead) or his corpse at req. distance
            }

            // quest objectives updated only for alive group member or dead but with not released body
            if (pGroupGuy->IsAlive() || !pGroupGuy->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST))
            {
                pGroupGuy->CastedCreatureOrGO(pRewardSource->GetEntry(), pRewardSource->GetObjectGuid(), spellid, pGroupGuy == this);
            }
        }
    }
    else                                                    // if (!pGroup)
    {
        CastedCreatureOrGO(pRewardSource->GetEntry(), pRewardSource->GetObjectGuid(), spellid);
    }
}

/**
 * @brief Checks whether the player or corpse is close enough for shared rewards.
 *
 * @param pRewardSource The source object used for the distance check.
 * @return True if the player qualifies for group reward range; otherwise, false.
 */
bool Player::IsAtGroupRewardDistance(WorldObject const* pRewardSource) const
{
    if (pRewardSource->IsWithinDistInMap(this, sWorld.getConfig(CONFIG_FLOAT_GROUP_XP_DISTANCE)))
    {
        return true;
    }

    if (IsAlive())
    {
        return false;
    }

    Corpse* corpse = GetCorpse();
    if (!corpse)
    {
        return false;
    }

    return pRewardSource->IsWithinDistInMap(corpse, sWorld.getConfig(CONFIG_FLOAT_GROUP_XP_DISTANCE));
}

/**
 * @brief Gets the player's base weapon skill for an attack type.
 *
 * @param attType The attack type to evaluate.
 * @return The corresponding base weapon skill value.
 */
uint32 Player::GetBaseWeaponSkillValue(WeaponAttackType attType) const
{
    Item* item = GetWeaponForAttack(attType, true, true);

    // unarmed only with base attack
    if (attType != BASE_ATTACK && !item)
    {
        return 0;
    }

    // weapon skill or (unarmed for base attack)
    uint32  skill = item ? item->GetSkill() : uint32(SKILL_UNARMED);
    return GetBaseSkillValue(skill);
}

/**
 * @brief Resurrects the player using the pending resurrection request data.
 */
void Player::ResurectUsingRequestData()
{
    /// Teleport before resurrecting by player, otherwise the player might get attacked from creatures near his corpse
    if (m_resurrectGuid.IsPlayer())
    {
        TeleportTo(m_resurrectMap, m_resurrectX, m_resurrectY, m_resurrectZ, GetOrientation());
    }

    // we cannot resurrect player when we triggered far teleport
    // player will be resurrected upon teleportation
    if (IsBeingTeleportedFar())
    {
        ScheduleDelayedOperation(DELAYED_RESURRECT_PLAYER);
        return;
    }

    ResurrectPlayer(0.0f, false);

    if (GetMaxHealth() > m_resurrectHealth)
    {
        SetHealth(m_resurrectHealth);
    }
    else
    {
        SetHealth(GetMaxHealth());
    }

    if (GetMaxPower(POWER_MANA) > m_resurrectMana)
    {
        SetPower(POWER_MANA, m_resurrectMana);
    }
    else
    {
        SetPower(POWER_MANA, GetMaxPower(POWER_MANA));
    }

    SetPower(POWER_RAGE, 0);

    SetPower(POWER_ENERGY, GetMaxPower(POWER_ENERGY));

    SpawnCorpseBones();
}

/**
 * @brief Sends a client-control state update for a unit.
 *
 * @param target The unit whose control state is being updated.
 * @param allowMove Nonzero to allow movement; zero to disable it.
 */
void Player::SetClientControl(Unit* target, uint8 allowMove)
{
    WorldPacket data(SMSG_CLIENT_CONTROL_UPDATE, target->GetPackGUID().size() + 1);
    data << target->GetPackGUID();
    data << uint8(allowMove);
    GetSession()->SendPacket(&data);
}

/**
 * @brief Applies or removes auras that depend on the player's current zone.
 */
void Player::UpdateZoneDependentAuras()
{
    // Some spells applied at enter into zone (with subzones), aura removed in UpdateAreaDependentAuras that called always at zone->area update
    SpellAreaForAreaMapBounds saBounds = sSpellMgr.GetSpellAreaForAreaMapBounds(m_zoneUpdateId);
    for (SpellAreaForAreaMap::const_iterator itr = saBounds.first; itr != saBounds.second; ++itr)
    {
        itr->second->ApplyOrRemoveSpellIfCan(this, m_zoneUpdateId, 0, true);
    }
}

/**
 * @brief Applies or removes auras that depend on the player's current subzone.
 */
void Player::UpdateAreaDependentAuras()
{
    // remove auras from spells with area limitations
    for (SpellAuraHolderMap::iterator iter = m_spellAuraHolders.begin(); iter != m_spellAuraHolders.end();)
    {
        // use m_zoneUpdateId for speed: UpdateArea called from UpdateZone or instead UpdateZone in both cases m_zoneUpdateId up-to-date
        if (sSpellMgr.GetSpellAllowedInLocationError(iter->second->GetSpellProto(), GetMapId(), m_zoneUpdateId, m_areaUpdateId, this) != SPELL_CAST_OK)
        {
            RemoveSpellAuraHolder(iter->second);
            iter = m_spellAuraHolders.begin();
        }
        else
        {
            ++iter;
        }
    }

    // some auras applied at subzone enter
    SpellAreaForAreaMapBounds saBounds = sSpellMgr.GetSpellAreaForAreaMapBounds(m_areaUpdateId);
    for (SpellAreaForAreaMap::const_iterator itr = saBounds.first; itr != saBounds.second; ++itr)
    {
        itr->second->ApplyOrRemoveSpellIfCan(this, m_zoneUpdateId, m_areaUpdateId, true);
    }
}

struct UpdateZoneDependentPetsHelper
{
    explicit UpdateZoneDependentPetsHelper(Player* _owner, uint32 zone, uint32 area) : owner(_owner), zone_id(zone), area_id(area) {}
    void operator()(Unit* unit) const
    {
        if (unit->GetTypeId() == TYPEID_UNIT && (reinterpret_cast<Creature*>(unit))->IsPet() && !(reinterpret_cast<Pet*>(unit))->IsPermanentPetFor(owner))
            if (uint32 spell_id = unit->GetUInt32Value(UNIT_CREATED_BY_SPELL))
                if (SpellEntry const* spellEntry = sSpellStore.LookupEntry(spell_id))
                    if (sSpellMgr.GetSpellAllowedInLocationError(spellEntry, owner->GetMapId(), zone_id, area_id, owner) != SPELL_CAST_OK)
                    {
                        (reinterpret_cast<Pet*>(unit))->Unsummon(PET_SAVE_AS_DELETED, owner);
                    }
    }
    Player* owner;
    uint32 zone_id;
    uint32 area_id;
};

void Player::UpdateZoneDependentPets()
{
    // check pet (permanent pets ignored), minipet, guardians (including protector)
    CallForAllControlledUnits(UpdateZoneDependentPetsHelper(this, m_zoneUpdateId, m_areaUpdateId), CONTROLLED_PET | CONTROLLED_GUARDIANS | CONTROLLED_MINIPET);
}


/**
 * @brief Updates liquid auras and mirror timers based on the player's position.
 *
 * @param m The current map.
 * @param x The X coordinate.
 * @param y The Y coordinate.
 * @param z The Z coordinate.
 */
void Player::UpdateUnderwaterState(Map* m, float x, float y, float z)
{
    GridMapLiquidData liquid_status;
    GridMapLiquidStatus res = m->GetTerrain()->getLiquidStatus(x, y, z, MAP_ALL_LIQUIDS, &liquid_status);
    if (!res)
    {
        m_MirrorTimerFlags &= ~(UNDERWATER_INWATER | UNDERWATER_INLAVA | UNDERWATER_INSLIME | UNDERWATER_INDARKWATER);
        if (m_lastLiquid && m_lastLiquid->SpellId)
        {
            RemoveAurasDueToSpell(m_lastLiquid->SpellId == 37025 ? 37284 : m_lastLiquid->SpellId);
        }
        m_lastLiquid = NULL;
        return;
    }

    if (uint32 liqEntry = liquid_status.entry)
    {
        LiquidTypeEntry const* liquid = sLiquidTypeStore.LookupEntry(liqEntry);
        if (m_lastLiquid && m_lastLiquid->SpellId && m_lastLiquid->Id != liqEntry)
        {
            RemoveAurasDueToSpell(m_lastLiquid->SpellId);
        }

        if (liquid && liquid->SpellId)
        {
            // Exception for SSC water
            uint32 liquidSpellId = liquid->SpellId == 37025 ? 37284 : liquid->SpellId;

            if (res & (LIQUID_MAP_UNDER_WATER | LIQUID_MAP_IN_WATER))
            {
                if (!HasAura(liquidSpellId))
                {
                    // Handle exception for SSC water
                    if (liquid->SpellId == 37025)
                    {
                        if (InstanceData* pInst = GetInstanceData())
                        {
                            if (pInst->CheckConditionCriteriaMeet(this, INSTANCE_CONDITION_ID_LURKER, NULL, CONDITION_FROM_HARDCODED))
                            {
                                if (pInst->CheckConditionCriteriaMeet(this, INSTANCE_CONDITION_ID_SCALDING_WATER, NULL, CONDITION_FROM_HARDCODED))
                                {
                                    CastSpell(this, liquidSpellId, true);
                                }
                                else
                                {
                                    SummonCreature(21508, 0, 0, 0, 0, TEMPSPAWN_TIMED_OOC_DESPAWN, 2000);
                                    // Special update timer for the SSC water
                                    m_positionStatusUpdateTimer = 2000;
                                }
                            }
                        }
                    }
                    else
                    {
                        CastSpell(this, liquidSpellId, true);
                    }
                }
            }
            else
            {
                RemoveAurasDueToSpell(liquidSpellId);
            }
        }

        m_lastLiquid = liquid;
    }
    else if (m_lastLiquid && m_lastLiquid->SpellId)
    {
        RemoveAurasDueToSpell(m_lastLiquid->SpellId == 37025 ? 37284 : m_lastLiquid->SpellId);
        m_lastLiquid = NULL;
    }

    // All liquids type - check under water position
    if (liquid_status.type_flags & (MAP_LIQUID_TYPE_WATER | MAP_LIQUID_TYPE_OCEAN | MAP_LIQUID_TYPE_MAGMA | MAP_LIQUID_TYPE_SLIME))
    {
        if (res & LIQUID_MAP_UNDER_WATER)
        {
            m_MirrorTimerFlags |= UNDERWATER_INWATER;
        }
        else
        {
            m_MirrorTimerFlags &= ~UNDERWATER_INWATER;
        }
    }

    // Allow travel in dark water on taxi or transport
    if ((liquid_status.type_flags & MAP_LIQUID_TYPE_DARK_WATER) && !IsTaxiFlying() && !GetTransport())
    {
        m_MirrorTimerFlags |= UNDERWATER_INDARKWATER;
    }
    else
    {
        m_MirrorTimerFlags &= ~UNDERWATER_INDARKWATER;
    }

    // in lava check, anywhere in lava level
    if (liquid_status.type_flags & MAP_LIQUID_TYPE_MAGMA)
    {
        if (res & (LIQUID_MAP_UNDER_WATER | LIQUID_MAP_IN_WATER | LIQUID_MAP_WATER_WALK))
        {
            m_MirrorTimerFlags |= UNDERWATER_INLAVA;
        }
        else
        {
            m_MirrorTimerFlags &= ~UNDERWATER_INLAVA;
        }
    }
    // in slime check, anywhere in slime level
    if (liquid_status.type_flags & MAP_LIQUID_TYPE_SLIME)
    {
        if (res & (LIQUID_MAP_UNDER_WATER | LIQUID_MAP_IN_WATER | LIQUID_MAP_WATER_WALK))
        {
            m_MirrorTimerFlags |= UNDERWATER_INSLIME;
        }
        else
        {
            m_MirrorTimerFlags &= ~UNDERWATER_INSLIME;
        }
    }
}

/**
 * @brief Enables or disables the player's ability to parry.
 *
 * @param value True to allow parry; false to disable it.
 */
void Player::SetCanParry(bool value)
{
    if (m_canParry == value)
    {
        return;
    }

    m_canParry = value;
    UpdateParryPercentage();
}

/**
 * @brief Enables or disables the player's ability to block.
 *
 * @param value True to allow block; false to disable it.
 */
void Player::SetCanBlock(bool value)
{
    if (m_canBlock == value)
    {
        return;
    }

    m_canBlock = value;
    UpdateBlockPercentage();
}

/**
 * @brief Checks whether this item position entry exists in a vector of positions.
 *
 * @param vec The vector of item positions to search.
 * @return True if an entry with the same position exists; otherwise, false.
 */
bool ItemPosCount::isContainedIn(ItemPosCountVec const& vec) const
{
    for (ItemPosCountVec::const_iterator itr = vec.begin(); itr != vec.end(); ++itr)
    {
        if (itr->pos == pos)
        {
            return true;
        }
    }

    return false;
}


uint32 Player::GetBarberShopCost(uint8 newhairstyle, uint8 newhaircolor, uint8 newfacialhair, uint32 newskintone)
{
    uint32 level = getLevel();

    if (level > GT_MAX_LEVEL)
    {
        level = GT_MAX_LEVEL;                               // max level in this dbc
    }

    uint8 hairstyle = GetByteValue(PLAYER_BYTES, 2);
    uint8 haircolor = GetByteValue(PLAYER_BYTES, 3);
    uint8 facialhair = GetByteValue(PLAYER_BYTES_2, 0);
    uint8 skintone = GetByteValue(PLAYER_BYTES, 0);

    if (hairstyle == newhairstyle && haircolor == newhaircolor && facialhair == newfacialhair &&
            (skintone == newskintone || newskintone == -1))
        return 0;

    GtBarberShopCostBaseEntry const* bsc = sGtBarberShopCostBaseStore.LookupEntry(level - 1);

    if (!bsc)                                               // shouldn't happen
    {
        return 0xFFFFFFFF;
    }

    float cost = 0;

    if (hairstyle != newhairstyle)
    {
        cost += bsc->cost;                                  // full price
    }

    if (haircolor != newhaircolor && hairstyle == newhairstyle)
    {
        cost += bsc->cost * 0.5f;                           // +1/2 of price
    }

    if (facialhair != newfacialhair)
    {
        cost += bsc->cost * 0.75f;                          // +3/4 of price
    }

    if (skintone != newskintone && newskintone != -1)
    {
        cost += bsc->cost * 0.5f;                           // +1/2 of price
    }

    return uint32(cost);
}

void Player::InitGlyphsForLevel()
{
    for (uint32 i = 0; i < sGlyphSlotStore.GetNumRows(); ++i)
    {
        if (GlyphSlotEntry const* gs = sGlyphSlotStore.LookupEntry(i))
            if (gs->Order)
            {
                SetGlyphSlot(gs->Order - 1, gs->Id);
            }
    }

    uint32 level = getLevel();
    uint32 value = 0;

    // 0x3F = 0x01 | 0x02 | 0x04 | 0x08 | 0x10 | 0x20 for 80 level
    if (level >= 15)
    {
        value |= (0x01 | 0x02);
    }
    if (level >= 30)
    {
        value |= 0x08;
    }
    if (level >= 50)
    {
        value |= 0x04;
    }
    if (level >= 70)
    {
        value |= 0x10;
    }
    if (level >= 80)
    {
        value |= 0x20;
    }

    SetUInt32Value(PLAYER_GLYPHS_ENABLED, value);
}

void Player::ApplyGlyph(uint8 slot, bool apply)
{
    if (uint32 glyph = GetGlyph(slot))
    {
        if (GlyphPropertiesEntry const* gp = sGlyphPropertiesStore.LookupEntry(glyph))
        {
            if (apply)
            {
                CastSpell(this, gp->SpellId, true);
                SetUInt32Value(PLAYER_FIELD_GLYPHS_1 + slot, glyph);
            }
            else
            {
                RemoveAurasDueToSpell(gp->SpellId);
                SetUInt32Value(PLAYER_FIELD_GLYPHS_1 + slot, 0);
            }
        }
    }
}

void Player::ApplyGlyphs(bool apply)
{
    for (uint8 i = 0; i < MAX_GLYPH_SLOT_INDEX; ++i)
    {
        ApplyGlyph(i, apply);
    }
}

/**
 * @brief Checks whether the player is immune to all spell schools.
 *
 * @return True if total immunity is active; otherwise, false.
 */
bool Player::isTotalImmune()
{
    AuraList const& immune = GetAurasByType(SPELL_AURA_SCHOOL_IMMUNITY);

    uint32 immuneMask = 0;
    for (AuraList::const_iterator itr = immune.begin(); itr != immune.end(); ++itr)
    {
        immuneMask |= (*itr)->GetModifier()->m_miscvalue;
        if (immuneMask & SPELL_SCHOOL_MASK_ALL)             // total immunity
        {
            return true;
        }
    }
    return false;
}

bool Player::HasTitle(uint32 bitIndex) const
{
    if (bitIndex > MAX_TITLE_INDEX)
    {
        return false;
    }

    uint32 fieldIndexOffset = bitIndex / 32;
    uint32 flag = 1 << (bitIndex % 32);
    return HasFlag(PLAYER__FIELD_KNOWN_TITLES + fieldIndexOffset, flag);
}

void Player::SetTitle(CharTitlesEntry const* title, bool lost)
{
    uint32 fieldIndexOffset = title->bit_index / 32;
    uint32 flag = 1 << (title->bit_index % 32);

    if (lost)
    {
        if (!HasFlag(PLAYER__FIELD_KNOWN_TITLES + fieldIndexOffset, flag))
        {
            return;
        }

        RemoveFlag(PLAYER__FIELD_KNOWN_TITLES + fieldIndexOffset, flag);
    }
    else
    {
        if (HasFlag(PLAYER__FIELD_KNOWN_TITLES + fieldIndexOffset, flag))
        {
            return;
        }

        SetFlag(PLAYER__FIELD_KNOWN_TITLES + fieldIndexOffset, flag);
    }

    WorldPacket data(SMSG_TITLE_EARNED, 4 + 4);
    data << uint32(title->bit_index);
    data << uint32(lost ? 0 : 1);                           // 1 - earned, 0 - lost
    GetSession()->SendPacket(&data);
}

void Player::ConvertRune(uint8 index, RuneType newType)
{
    SetCurrentRune(index, newType);

    WorldPacket data(SMSG_CONVERT_RUNE, 2);
    data << uint8(index);
    data << uint8(newType);
    GetSession()->SendPacket(&data);
}

bool Player::ActivateRunes(RuneType type, uint32 count)
{
    bool modify = false;
    for (uint32 j = 0; count > 0 && j < MAX_RUNES; ++j)
    {
        if (GetRuneCooldown(j) && GetCurrentRune(j) == type)
        {
            SetRuneCooldown(j, 0);
            --count;
            modify = true;
        }
    }

    return modify;
}

void Player::ResyncRunes()
{
    WorldPacket data(SMSG_RESYNC_RUNES, 4 + MAX_RUNES * 2);
    data << uint32(MAX_RUNES);
    for (uint32 i = 0; i < MAX_RUNES; ++i)
    {
        data << uint8(GetCurrentRune(i));                   // rune type
        data << uint8(255 - ((GetRuneCooldown(i) / REGEN_TIME_FULL) * 51));     // passed cooldown time (0-255)
    }
    GetSession()->SendPacket(&data);
}

void Player::AddRunePower(uint8 index)
{
    WorldPacket data(SMSG_ADD_RUNE_POWER, 4);
    data << uint32(1 << index);                             // mask (0x00-0x3F probably)
    GetSession()->SendPacket(&data);
}

static RuneType runeSlotTypes[MAX_RUNES] =
{
    /*0*/ RUNE_BLOOD,
    /*1*/ RUNE_BLOOD,
    /*2*/ RUNE_UNHOLY,
    /*3*/ RUNE_UNHOLY,
    /*4*/ RUNE_FROST,
    /*5*/ RUNE_FROST
};

void Player::InitRunes()
{
    if (getClass() != CLASS_DEATH_KNIGHT)
    {
        return;
    }

    m_runes = new Runes;

    m_runes->runeState = 0;

    for (uint32 i = 0; i < MAX_RUNES; ++i)
    {
        SetBaseRune(i, runeSlotTypes[i]);                   // init base types
        SetCurrentRune(i, runeSlotTypes[i]);                // init current types
        SetRuneCooldown(i, 0);                              // reset cooldowns
        m_runes->SetRuneState(i);
    }

    for (uint32 i = 0; i < NUM_RUNE_TYPES; ++i)
    {
        SetFloatValue(PLAYER_RUNE_REGEN_1 + i, 0.1f);
    }
}

bool Player::IsBaseRuneSlotsOnCooldown(RuneType runeType) const
{
    for (uint32 i = 0; i < MAX_RUNES; ++i)
    {
        if (GetBaseRune(i) == runeType && GetRuneCooldown(i) == 0)
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief Builds temporary loot data and stores all eligible loot automatically.
 *
 * @param lootTarget The object owning the loot.
 * @param loot_id The loot template identifier.
 * @param store The loot store to use.
 * @param broadcast True to broadcast item gains.
 * @param bag The preferred destination bag.
 * @param slot The preferred destination slot.
 */
void Player::AutoStoreLoot(WorldObject const* lootTarget, uint32 loot_id, LootStore const& store, bool broadcast, uint8 bag, uint8 slot)
{
    Loot loot(lootTarget);
    loot.FillLoot(loot_id, store, this, true);

    AutoStoreLoot(loot, broadcast, bag, slot);
}

/**
 * @brief Stores all eligible loot entries directly into the player's inventory.
 *
 * @param loot The loot container to process.
 * @param broadcast True to broadcast item gains.
 * @param bag The preferred destination bag.
 * @param slot The preferred destination slot.
 */
void Player::AutoStoreLoot(Loot& loot, bool broadcast, uint8 bag, uint8 slot)
{
    uint32 max_slot = loot.GetMaxSlotInLootFor(this);
    for (uint32 i = 0; i < max_slot; ++i)
    {
        LootItem* lootItem = loot.LootItemInSlot(i, this);

        ItemPosCountVec dest;
        InventoryResult msg = CanStoreNewItem(bag, slot, dest, lootItem->itemid, lootItem->count);
        if (msg != EQUIP_ERR_OK && slot != NULL_SLOT)
        {
            msg = CanStoreNewItem(bag, NULL_SLOT, dest, lootItem->itemid, lootItem->count);
        }
        if (msg != EQUIP_ERR_OK && bag != NULL_BAG)
        {
            msg = CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, lootItem->itemid, lootItem->count);
        }
        if (msg != EQUIP_ERR_OK)
        {
            SendEquipError(msg, NULL, NULL, lootItem->itemid);
            continue;
        }

        Item* pItem = StoreNewItem(dest, lootItem->itemid, true, lootItem->randomPropertyId);
        SendNewItem(pItem, lootItem->count, false, false, broadcast);
    }
}

/**
 * @brief Replaces an item with another item while preserving transferable state.
 *
 * @param item The original item.
 * @param newItemId The new item entry identifier.
 * @return The converted item, or NULL if conversion failed.
 */
Item* Player::ConvertItem(Item* item, uint32 newItemId)
{
    uint16 pos = item->GetPos();

    Item* pNewItem = Item::CreateItem(newItemId, 1, this);
    if (!pNewItem)
    {
        return NULL;
    }

    // copy enchantments
    for (uint8 j = PERM_ENCHANTMENT_SLOT; j <= TEMP_ENCHANTMENT_SLOT; ++j)
    {
        if (item->GetEnchantmentId(EnchantmentSlot(j)))
            pNewItem->SetEnchantment(EnchantmentSlot(j), item->GetEnchantmentId(EnchantmentSlot(j)),
                                     item->GetEnchantmentDuration(EnchantmentSlot(j)), item->GetEnchantmentCharges(EnchantmentSlot(j)));
    }

    // copy durability
    if (item->GetUInt32Value(ITEM_FIELD_DURABILITY) < item->GetUInt32Value(ITEM_FIELD_MAXDURABILITY))
    {
        double loosePercent = 1 - item->GetUInt32Value(ITEM_FIELD_DURABILITY) / double(item->GetUInt32Value(ITEM_FIELD_MAXDURABILITY));
        DurabilityLoss(pNewItem, loosePercent);
    }

    if (IsInventoryPos(pos))
    {
        ItemPosCountVec dest;
        InventoryResult msg = CanStoreItem(item->GetBagSlot(), item->GetSlot(), dest, pNewItem, true);
        // ignore cast/combat time restriction
        if (msg == EQUIP_ERR_OK)
        {
            DestroyItem(item->GetBagSlot(), item->GetSlot(), true);
            return StoreItem(dest, pNewItem, true);
        }
    }
    else if (IsBankPos(pos))
    {
        ItemPosCountVec dest;
        InventoryResult msg = CanBankItem(item->GetBagSlot(), item->GetSlot(), dest, pNewItem, true);
        // ignore cast/combat time restriction
        if (msg == EQUIP_ERR_OK)
        {
            DestroyItem(item->GetBagSlot(), item->GetSlot(), true);
            return BankItem(dest, pNewItem, true);
        }
    }
    else if (IsEquipmentPos(pos))
    {
        uint16 dest;
        InventoryResult msg = CanEquipItem(item->GetSlot(), dest, pNewItem, true, false);
        // ignore cast/combat time restriction
        if (msg == EQUIP_ERR_OK)
        {
            DestroyItem(item->GetBagSlot(), item->GetSlot(), true);
            pNewItem = EquipItem(dest, pNewItem, true);
            AutoUnequipOffhandIfNeed();
            return pNewItem;
        }
    }

    // fail
    delete pNewItem;
    return NULL;
}

/**
 * @brief Calculates the total talent points available for the player's level.
 *
 * @return The number of talent points granted by level and rate settings.
 */
uint32 Player::CalculateTalentsPoints() const
{
    uint32 base_level = getClass() == CLASS_DEATH_KNIGHT ? 55 : 9;
    uint32 base_talent = getLevel() <= base_level ? 0 : getLevel() - base_level;

    uint32 talentPointsForLevel = base_talent + m_questRewardTalentCount;

    return uint32(talentPointsForLevel * sWorld.getConfig(CONFIG_FLOAT_RATE_TALENT));
}

bool Player::CanStartFlyInArea(uint32 mapid, uint32 zone, uint32 area) const
{
    if (isGameMaster())
    {
        return true;
    }
    // continent checked in SpellMgr::GetSpellAllowedInLocationError at cast and area update
    uint32 v_map = GetVirtualMapForMapAndZone(mapid, zone);

    if (v_map == 571 && !HasSpell(54197))   // Cold Weather Flying
    {
        return false;
    }

    // don't allow flying in Dalaran restricted areas
    // (no other zones currently has areas with AREA_FLAG_CANNOT_FLY)
    if (AreaTableEntry const* atEntry = GetAreaEntryByAreaID(area))
    {
        return (!(atEntry->flags & AREA_FLAG_CANNOT_FLY));
    }

    // TODO: disallow mounting in wintergrasp too when battle is in progress
    // forced dismount part in Player::UpdateArea()
    return true;
}

struct DoPlayerLearnSpell
{
    DoPlayerLearnSpell(Player& _player) : player(_player) {}
    void operator()(uint32 spell_id) { player.learnSpell(spell_id, false); }
    Player& player;
};

/**
 * @brief Learns a spell and all higher ranks linked in its rank chain.
 *
 * @param spellid The base spell identifier.
 */
void Player::learnSpellHighRank(uint32 spellid)
{
    learnSpell(spellid, false);

    DoPlayerLearnSpell worker(*this);
    sSpellMgr.doForHighRanks(spellid, worker);
}

/**
 * @brief Loads skill values from the database and initializes related rewards.
 *
 * @param result The query result containing saved skill rows.
 */
void Player::_LoadSkills(QueryResult* result)
{
    //                                                           0      1      2
    // SetPQuery(PLAYER_LOGIN_QUERY_LOADSKILLS,          "SELECT `skill`, `value`, `max` FROM `character_skills` WHERE `guid` = '%u'", GUID_LOPART(m_guid));

    uint32 count = 0;
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();

            uint16 skill    = fields[0].GetUInt16();
            uint16 value    = fields[1].GetUInt16();
            uint16 max      = fields[2].GetUInt16();

            SkillLineEntry const* pSkill = sSkillLineStore.LookupEntry(skill);
            if (!pSkill)
            {
                sLog.outError("Character %u has skill %u that does not exist.", GetGUIDLow(), skill);
                continue;
            }

            // set fixed skill ranges
            switch (GetSkillRangeType(pSkill, false))
            {
                case SKILL_RANGE_LANGUAGE:                  // 300..300
                    value = max = 300;
                    break;
                case SKILL_RANGE_MONO:                      // 1..1, grey monolite bar
                    value = max = 1;
                    break;
                case SKILL_RANGE_LEVEL:
                    max = GetMaxSkillValueForLevel();       // max value can be wrong for the actual level
                    break;
                default:
                    break;
            }

            if (value == 0)
            {
                sLog.outError("Character %u has skill %u with value 0. Will be deleted.", GetGUIDLow(), skill);
                CharacterDatabase.PExecute("DELETE FROM `character_skills` WHERE `guid` = '%u' AND `skill` = '%u' ", GetGUIDLow(), skill);
                continue;
            }

            SetUInt32Value(PLAYER_SKILL_INDEX(count), MAKE_PAIR32(skill, 0));
            SetUInt32Value(PLAYER_SKILL_VALUE_INDEX(count), MAKE_SKILL_VALUE(value, max));
            SetUInt32Value(PLAYER_SKILL_BONUS_INDEX(count), 0);

            mSkillStatus.insert(SkillStatusMap::value_type(skill, SkillStatusData(count, SKILL_UNCHANGED)));

            learnSkillRewardedSpells(skill, value);

            ++count;

            if (count >= PLAYER_MAX_SKILLS)                 // client limit
            {
                sLog.outError("Character %u has more than %u skills.", GetGUIDLow(), PLAYER_MAX_SKILLS);
                break;
            }
        }
        while (result->NextRow());
        delete result;
    }

    for (; count < PLAYER_MAX_SKILLS; ++count)
    {
        SetUInt32Value(PLAYER_SKILL_INDEX(count), 0);
        SetUInt32Value(PLAYER_SKILL_VALUE_INDEX(count), 0);
        SetUInt32Value(PLAYER_SKILL_BONUS_INDEX(count), 0);
    }

    // special settings
    if (getClass() == CLASS_DEATH_KNIGHT)
    {
        uint32 base_level = std::min(getLevel(), sWorld.getConfig(CONFIG_UINT32_START_HEROIC_PLAYER_LEVEL));
        if (base_level < 1)
        {
            base_level = 1;
        }
        uint32 base_skill = (base_level - 1) * 5;           // 270 at starting level 55
        if (base_skill < 1)
        {
            base_skill = 1;                                 // skill mast be known and then > 0 in any case
        }

        if (GetPureSkillValue(SKILL_FIRST_AID) < base_skill)
        {
            SetSkill(SKILL_FIRST_AID, base_skill, base_skill);
        }
        if (GetPureSkillValue(SKILL_AXES) < base_skill)
        {
            SetSkill(SKILL_AXES, base_skill, base_skill);
        }
        if (GetPureSkillValue(SKILL_DEFENSE) < base_skill)
        {
            SetSkill(SKILL_DEFENSE, base_skill, base_skill);
        }
        if (GetPureSkillValue(SKILL_POLEARMS) < base_skill)
        {
            SetSkill(SKILL_POLEARMS, base_skill, base_skill);
        }
        if (GetPureSkillValue(SKILL_SWORDS) < base_skill)
        {
            SetSkill(SKILL_SWORDS, base_skill, base_skill);
        }
        if (GetPureSkillValue(SKILL_2H_AXES) < base_skill)
        {
            SetSkill(SKILL_2H_AXES, base_skill, base_skill);
        }
        if (GetPureSkillValue(SKILL_2H_SWORDS) < base_skill)
        {
            SetSkill(SKILL_2H_SWORDS, base_skill, base_skill);
        }
        if (GetPureSkillValue(SKILL_UNARMED) < base_skill)
        {
            SetSkill(SKILL_UNARMED, base_skill, base_skill);
        }
    }
}

uint32 Player::GetPhaseMaskForSpawn() const
{
    uint32 phase = PHASEMASK_NORMAL;
    if (!isGameMaster())
    {
        phase = GetPhaseMask();
    }
    else
    {
        AuraList const& phases = GetAurasByType(SPELL_AURA_PHASE);
        if (!phases.empty())
        {
            phase = phases.front()->GetMiscValue();
        }
    }

    // some aura phases include 1 normal map in addition to phase itself
    if (uint32 n_phase = phase & ~PHASEMASK_NORMAL)
    {
        return n_phase;
    }

    return PHASEMASK_NORMAL;
}

/**
 * @brief Checks whether a concrete item can be equipped under unique-equip rules.
 *
 * @param pItem The item to test.
 * @param eslot The equipment slot being considered.
 * @return The inventory result describing whether equipping is allowed.
 */
InventoryResult Player::CanEquipUniqueItem(Item* pItem, uint8 eslot, uint32 limit_count) const
{
    ItemPrototype const* pProto = pItem->GetProto();

    // proto based limitations
    if (InventoryResult res = CanEquipUniqueItem(pProto, eslot, limit_count))
    {
        return res;
    }

    // check unique-equipped on gems
    for (uint32 enchant_slot = SOCK_ENCHANTMENT_SLOT; enchant_slot < SOCK_ENCHANTMENT_SLOT + 3; ++enchant_slot)
    {
        uint32 enchant_id = pItem->GetEnchantmentId(EnchantmentSlot(enchant_slot));
        if (!enchant_id)
        {
            continue;
        }
        SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
        if (!enchantEntry)
        {
            continue;
        }

        ItemPrototype const* pGem = ObjectMgr::GetItemPrototype(enchantEntry->GemID);
        if (!pGem)
        {
            continue;
        }

        // include for check equip another gems with same limit category for not equipped item (and then not counted)
        uint32 gem_limit_count = !pItem->IsEquipped() && pGem->ItemLimitCategory
                                 ? pItem->GetGemCountWithLimitCategory(pGem->ItemLimitCategory) : 1;

        if (InventoryResult res = CanEquipUniqueItem(pGem, eslot, gem_limit_count))
        {
            return res;
        }
    }

    return EQUIP_ERR_OK;
}

/**
 * @brief Checks whether an item prototype can be equipped under unique-equip rules.
 *
 * @param itemProto The item prototype to test.
 * @param except_slot An equipment slot to ignore during the check.
 * @return The inventory result describing whether equipping is allowed.
 */
InventoryResult Player::CanEquipUniqueItem(ItemPrototype const* itemProto, uint8 except_slot, uint32 limit_count) const
{
    // check unique-equipped on item
    if (itemProto->Flags & ITEM_FLAG_UNIQUE_EQUIPPED)
    {
        // there is an equip limit on this item
        if (HasItemOrGemWithIdEquipped(itemProto->ItemId, 1, except_slot))
        {
            return EQUIP_ERR_ITEM_UNIQUE_EQUIPABLE;
        }
    }

    // check unique-equipped limit
    if (itemProto->ItemLimitCategory)
    {
        ItemLimitCategoryEntry const* limitEntry = sItemLimitCategoryStore.LookupEntry(itemProto->ItemLimitCategory);
        if (!limitEntry)
        {
            return EQUIP_ERR_ITEM_CANT_BE_EQUIPPED;
        }

        // NOTE: limitEntry->mode not checked because if item have have-limit then it applied and to equip case

        if (limit_count > limitEntry->maxCount)
        {
            return EQUIP_ERR_ITEM_MAX_LIMIT_CATEGORY_EQUIPPED_EXCEEDED_IS;
        }

        // there is an equip limit on this item
        if (HasItemOrGemWithLimitCategoryEquipped(itemProto->ItemLimitCategory, limitEntry->maxCount - limit_count + 1, except_slot))
        {
            return EQUIP_ERR_ITEM_MAX_LIMIT_CATEGORY_EQUIPPED_EXCEEDED_IS;
        }
    }

    return EQUIP_ERR_OK;
}

/**
 * @brief Calculates and applies fall damage from movement updates.
 *
 * @param movementInfo The movement packet information containing fall data.
 */
void Player::HandleFall(MovementInfo const& movementInfo)
{
    // calculate total z distance of the fall
    float z_diff = m_lastFallZ - movementInfo.GetPos()->z;
    DEBUG_LOG("zDiff = %f", z_diff);

    // Players with low fall distance, Feather Fall or physical immunity (charges used) are ignored
    // 14.57 can be calculated by resolving damageperc formula below to 0
    if (z_diff >= 14.57f && !IsDead() && !isGameMaster() && !HasMovementFlag(MOVEFLAG_ONTRANSPORT) &&
            !HasAuraType(SPELL_AURA_HOVER) && !HasAuraType(SPELL_AURA_FEATHER_FALL) &&
            !HasAuraType(SPELL_AURA_FLY) && !IsImmuneToDamage(SPELL_SCHOOL_MASK_NORMAL))
    {
        // Safe fall, fall height reduction
        int32 safe_fall = GetTotalAuraModifier(SPELL_AURA_SAFE_FALL);

        float damageperc = 0.018f * (z_diff - safe_fall) - 0.2426f;

        if (damageperc > 0)
        {
            uint32 damage = (uint32)(damageperc * GetMaxHealth() * sWorld.getConfig(CONFIG_FLOAT_RATE_DAMAGE_FALL));

            float height = movementInfo.GetPos()->z;
            UpdateAllowedPositionZ(movementInfo.GetPos()->x, movementInfo.GetPos()->y, height);

            if (damage > 0)
            {
                // Prevent fall damage from being more than the player maximum health
                if (damage > GetMaxHealth())
                {
                    damage = GetMaxHealth();
                }

                // Gust of Wind
                if (GetDummyAura(43621))
                {
                    damage = GetMaxHealth() / 2;
                }

                uint32 original_health = GetHealth();
                uint32 final_damage = EnvironmentalDamage(DAMAGE_FALL, damage);

                // recheck alive, might have died of EnvironmentalDamage, avoid cases when player die in fact like Spirit of Redemption case
                if (IsAlive() && final_damage < original_health)
                {
                    GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING, uint32(z_diff * 100));
                }
            }

            // Z given by moveinfo, LastZ, FallTime, WaterZ, MapZ, Damage, Safefall reduction
            DEBUG_LOG("FALLDAMAGE z=%f sz=%f pZ=%f FallTime=%d mZ=%f damage=%d SF=%d" , movementInfo.GetPos()->z, height, GetPositionZ(), movementInfo.GetFallTime(), height, damage, safe_fall);
        }
    }
}

void Player::UpdateAchievementCriteria(AchievementCriteriaTypes type, uint32 miscvalue1/*=0*/, uint32 miscvalue2/*=0*/, Unit* unit/*=NULL*/, uint32 time/*=0*/)
{
    GetAchievementMgr().UpdateAchievementCriteria(type, miscvalue1, miscvalue2, unit, time);
}

void Player::StartTimedAchievementCriteria(AchievementCriteriaTypes type, uint32 timedRequirementId, time_t startTime /*= 0*/)
{
    GetAchievementMgr().StartTimedAchievementCriteria(type, timedRequirementId, startTime);
}

PlayerTalent const* Player::GetKnownTalentById(int32 talentId) const
{
    PlayerTalentMap::const_iterator itr = m_talents[m_activeSpec].find(talentId);
    if (itr != m_talents[m_activeSpec].end() && itr->second.state != PLAYERSPELL_REMOVED)
    {
        return &itr->second;
    }
    else
    {
        return NULL;
    }
}

SpellEntry const* Player::GetKnownTalentRankById(int32 talentId) const
{
    if (PlayerTalent const* talent = GetKnownTalentById(talentId))
    {
        return sSpellStore.LookupEntry(talent->talentEntry->RankID[talent->currentRank]);
    }
    else
    {
        return NULL;
    }
}

/**
 * @brief Learns a specific talent rank if all requirements are satisfied.
 *
 * @param talentId The talent entry identifier.
 * @param talentRank The requested talent rank index.
 */
void Player::LearnTalent(uint32 talentId, uint32 talentRank)
{
    uint32 CurTalentPoints = GetFreeTalentPoints();

    if (CurTalentPoints == 0)
    {
        return;
    }

    if (talentRank >= MAX_TALENT_RANK)
    {
        return;
    }

    TalentEntry const* talentInfo = sTalentStore.LookupEntry(talentId);

    if (!talentInfo)
    {
        return;
    }

    TalentTabEntry const* talentTabInfo = sTalentTabStore.LookupEntry(talentInfo->TalentTab);

    if (!talentTabInfo)
    {
        return;
    }

    // prevent learn talent for different class (cheating)
    if ((getClassMask() & talentTabInfo->ClassMask) == 0)
    {
        return;
    }

    // find current max talent rank
    uint32 curtalent_maxrank = 0;
    if (PlayerTalent const* talent = GetKnownTalentById(talentId))
    {
        curtalent_maxrank = talent->currentRank + 1;
    }

    // we already have same or higher talent rank learned
    if (curtalent_maxrank >= (talentRank + 1))
    {
        return;
    }

    // check if we have enough talent points
    if (CurTalentPoints < (talentRank - curtalent_maxrank + 1))
    {
        return;
    }

    // Check if it requires another talent
    if (talentInfo->DependsOn > 0)
    {
        if (TalentEntry const* depTalentInfo = sTalentStore.LookupEntry(talentInfo->DependsOn))
        {
            bool hasEnoughRank = false;
            PlayerTalentMap::iterator dependsOnTalent = m_talents[m_activeSpec].find(depTalentInfo->TalentID);
            if (dependsOnTalent != m_talents[m_activeSpec].end() && dependsOnTalent->second.state != PLAYERSPELL_REMOVED)
            {
                PlayerTalent depTalent = (*dependsOnTalent).second;
                if (depTalent.currentRank >= talentInfo->DependsOnRank)
                {
                    hasEnoughRank = true;
                }
            }

            if (!hasEnoughRank)
            {
                return;
            }
        }
    }

    // Find out how many points we have in this field
    uint32 spentPoints = 0;

    uint32 tTab = talentInfo->TalentTab;
    if (talentInfo->Row > 0)
    {
        for (PlayerTalentMap::const_iterator iter = m_talents[m_activeSpec].begin(); iter != m_talents[m_activeSpec].end(); ++iter)
        {
            if (iter->second.state != PLAYERSPELL_REMOVED && iter->second.talentEntry->TalentTab == tTab)
            {
                spentPoints += iter->second.currentRank + 1;
            }
        }
    }

    // not have required min points spent in talent tree
    if (spentPoints < (talentInfo->Row * MAX_TALENT_RANK))
    {
        return;
    }

    // spell not set in talent.dbc
    uint32 spellid = talentInfo->RankID[talentRank];
    if (spellid == 0)
    {
        sLog.outError("Talent.dbc have for talent: %u Rank: %u spell id = 0", talentId, talentRank);
        return;
    }

    // already known
    if (HasSpell(spellid))
    {
        return;
    }

    // learn! (other talent ranks will unlearned at learning)
    learnSpell(spellid, false);
    DETAIL_LOG("TalentID: %u Rank: %u Spell: %u\n", talentId, talentRank, spellid);

#ifdef ENABLE_ELUNA
    if (Eluna* e = GetEluna())
    {
        e->OnLearnTalents(this, talentId, talentRank, spellid);
    }
#endif /*ENABLE_ELUNA*/
}

void Player::LearnPetTalent(ObjectGuid petGuid, uint32 talentId, uint32 talentRank)
{
    Pet* pet = GetPet();
    if (!pet)
    {
        return;
    }

    if (petGuid != pet->GetObjectGuid())
    {
        return;
    }

    uint32 CurTalentPoints = pet->GetFreeTalentPoints();

    if (CurTalentPoints == 0)
    {
        return;
    }

    if (talentRank >= MAX_PET_TALENT_RANK)
    {
        return;
    }

    TalentEntry const* talentInfo = sTalentStore.LookupEntry(talentId);

    if (!talentInfo)
    {
        return;
    }

    TalentTabEntry const* talentTabInfo = sTalentTabStore.LookupEntry(talentInfo->TalentTab);

    if (!talentTabInfo)
    {
        return;
    }

    CreatureInfo const* ci = pet->GetCreatureInfo();

    if (!ci)
    {
        return;
    }

    CreatureFamilyEntry const* pet_family = sCreatureFamilyStore.LookupEntry(ci->Family);

    if (!pet_family)
    {
        return;
    }

    if (pet_family->petTalentType < 0)                      // not hunter pet
    {
        return;
    }

    // prevent learn talent for different family (cheating)
    if (!((1 << pet_family->petTalentType) & talentTabInfo->petTalentMask))
    {
        return;
    }

    // find current max talent rank
    int32 curtalent_maxrank = 0;
    for (int32 k = MAX_TALENT_RANK - 1; k > -1; --k)
    {
        if (talentInfo->RankID[k] && pet->HasSpell(talentInfo->RankID[k]))
        {
            curtalent_maxrank = k + 1;
            break;
        }
    }

    // we already have same or higher talent rank learned
    if (curtalent_maxrank >= int32(talentRank + 1))
    {
        return;
    }

    // check if we have enough talent points
    if (CurTalentPoints < (talentRank - curtalent_maxrank + 1))
    {
        return;
    }

    // Check if it requires another talent
    if (talentInfo->DependsOn > 0)
    {
        if (TalentEntry const* depTalentInfo = sTalentStore.LookupEntry(talentInfo->DependsOn))
        {
            bool hasEnoughRank = false;
            for (int i = talentInfo->DependsOnRank; i < MAX_TALENT_RANK; ++i)
            {
                if (depTalentInfo->RankID[i] != 0)
                    if (pet->HasSpell(depTalentInfo->RankID[i]))
                    {
                        hasEnoughRank = true;
                    }
            }
            if (!hasEnoughRank)
            {
                return;
            }
        }
    }

    // Find out how many points we have in this field
    uint32 spentPoints = 0;

    uint32 tTab = talentInfo->TalentTab;
    if (talentInfo->Row > 0)
    {
        unsigned int numRows = sTalentStore.GetNumRows();
        for (unsigned int i = 0; i < numRows; ++i)          // Loop through all talents.
        {
            // Someday, someone needs to revamp
            const TalentEntry* tmpTalent = sTalentStore.LookupEntry(i);
            if (tmpTalent)                                  // the way talents are tracked
            {
                if (tmpTalent->TalentTab == tTab)
                {
                    for (int j = 0; j < MAX_TALENT_RANK; ++j)
                    {
                        if (tmpTalent->RankID[j] != 0)
                        {
                            if (pet->HasSpell(tmpTalent->RankID[j]))
                            {
                                spentPoints += j + 1;
                            }
                        }
                    }
                }
            }
        }
    }

    // not have required min points spent in talent tree
    if (spentPoints < (talentInfo->Row * MAX_PET_TALENT_RANK))
    {
        return;
    }

    // spell not set in talent.dbc
    uint32 spellid = talentInfo->RankID[talentRank];
    if (spellid == 0)
    {
        sLog.outError("Talent.dbc have for talent: %u Rank: %u spell id = 0", talentId, talentRank);
        return;
    }

    // already known
    if (pet->HasSpell(spellid))
    {
        return;
    }

    // learn! (other talent ranks will unlearned at learning)
    pet->learnSpell(spellid);
    DETAIL_LOG("PetTalentID: %u Rank: %u Spell: %u\n", talentId, talentRank, spellid);
}

void Player::UpdateKnownCurrencies(uint32 itemId, bool apply)
{
    if (CurrencyTypesEntry const* ctEntry = sCurrencyTypesStore.LookupEntry(itemId))
    {
        if (apply)
        {
            SetFlag64(PLAYER_FIELD_KNOWN_CURRENCIES, (UI64LIT(1) << (ctEntry->BitIndex - 1)));
        }
        else
        {
            RemoveFlag64(PLAYER_FIELD_KNOWN_CURRENCIES, (UI64LIT(1) << (ctEntry->BitIndex - 1)));
        }
    }
}

/**
 * @brief Refreshes stored fall tracking data when movement indicates a new fall state.
 *
 * @param minfo The current movement information.
 * @param opcode The movement opcode being processed.
 */
void Player::UpdateFallInformationIfNeed(MovementInfo const& minfo, uint16 opcode)
{
    if (m_lastFallTime >= minfo.GetFallTime() || m_lastFallZ <= minfo.GetPos()->z || opcode == MSG_MOVE_FALL_LAND)
    {
        SetFallInformation(minfo.GetFallTime(), minfo.GetPos()->z);
    }
}

/**
 * @brief Temporarily unsummons the current pet when the player's state requires it.
 */
void Player::UnsummonPetTemporaryIfAny()
{
    Pet* pet = GetPet();
    if (!pet)
    {
        return;
    }

    if (!m_temporaryUnsummonedPetNumber && pet->isControlled() && !pet->isTemporarySummoned())
    {
        m_temporaryUnsummonedPetNumber = pet->GetCharmInfo()->GetPetNumber();
    }

    pet->Unsummon(PET_SAVE_AS_CURRENT, this);
}

/**
 * @brief Resummons a pet that was temporarily unsummoned earlier.
 */
void Player::ResummonPetTemporaryUnSummonedIfAny()
{
    if (!m_temporaryUnsummonedPetNumber)
    {
        return;
    }

    // not resummon in not appropriate state
    if (IsPetNeedBeTemporaryUnsummoned())
    {
        return;
    }

    if (GetPetGuid())
    {
        return;
    }

    Pet* NewPet = new Pet;
    if (!NewPet->LoadPetFromDB(this, 0, m_temporaryUnsummonedPetNumber, true))
    {
        delete NewPet;
    }

    m_temporaryUnsummonedPetNumber = 0;
}

/**
 * @brief Checks whether the player can currently see spell-click interaction on a creature.
 *
 * @param c The creature to evaluate.
 * @return True if spell-click should be visible; otherwise, false.
 */
bool Player::canSeeSpellClickOn(Creature const* c) const
{
    if (!c->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK))
    {
        return false;
    }

    SpellClickInfoMapBounds clickPair = sObjectMgr.GetSpellClickInfoMapBounds(c->GetEntry());
    for (SpellClickInfoMap::const_iterator itr = clickPair.first; itr != clickPair.second; ++itr)
    {
        if (itr->second.IsFitToRequirements(this, c))
        {
            return true;
        }
    }

    return false;
}

void Player::BuildPlayerTalentsInfoData(WorldPacket* data)
{
    *data << uint32(GetFreeTalentPoints());                 // unspentTalentPoints
    *data << uint8(m_specsCount);                           // talent group count (0, 1 or 2)
    *data << uint8(m_activeSpec);                           // talent group index (0 or 1)

    if (m_specsCount)
    {
        // loop through all specs (only 1 for now)
        for (uint32 specIdx = 0; specIdx < m_specsCount; ++specIdx)
        {
            uint8 talentIdCount = 0;
            size_t pos = data->wpos();
            *data << uint8(talentIdCount);                  // [PH], talentIdCount

            // find class talent tabs (all players have 3 talent tabs)
            uint32 const* talentTabIds = GetTalentTabPages(getClass());

            for (uint32 i = 0; i < 3; ++i)
            {
                uint32 talentTabId = talentTabIds[i];
                for (PlayerTalentMap::iterator iter = m_talents[specIdx].begin(); iter != m_talents[specIdx].end(); ++iter)
                {
                    PlayerTalent talent = (*iter).second;

                    if (talent.state == PLAYERSPELL_REMOVED)
                    {
                        continue;
                    }

                    // skip another tab talents
                    if (talent.talentEntry->TalentTab != talentTabId)
                    {
                        continue;
                    }

                    *data << uint32(talent.talentEntry->TalentID);  // Talent.dbc
                    *data << uint8(talent.currentRank);     // talentMaxRank (0-4)

                    ++talentIdCount;
                }
            }

            data->put<uint8>(pos, talentIdCount);           // put real count

            *data << uint8(MAX_GLYPH_SLOT_INDEX);           // glyphs count

            // GlyphProperties.dbc
            for (uint8 i = 0; i < MAX_GLYPH_SLOT_INDEX; ++i)
            {
                *data << uint16(m_glyphs[specIdx][i].GetId());
            }
        }
    }
}

void Player::BuildPetTalentsInfoData(WorldPacket* data)
{
    uint32 unspentTalentPoints = 0;
    size_t pointsPos = data->wpos();
    *data << uint32(unspentTalentPoints);                   // [PH], unspentTalentPoints

    uint8 talentIdCount = 0;
    size_t countPos = data->wpos();
    *data << uint8(talentIdCount);                          // [PH], talentIdCount

    Pet* pet = GetPet();
    if (!pet)
    {
        return;
    }

    unspentTalentPoints = pet->GetFreeTalentPoints();

    data->put<uint32>(pointsPos, unspentTalentPoints);      // put real points

    CreatureInfo const* ci = pet->GetCreatureInfo();
    if (!ci)
    {
        return;
    }

    CreatureFamilyEntry const* pet_family = sCreatureFamilyStore.LookupEntry(ci->Family);
    if (!pet_family || pet_family->petTalentType < 0)
    {
        return;
    }

    for (uint32 talentTabId = 1; talentTabId < sTalentTabStore.GetNumRows(); ++talentTabId)
    {
        TalentTabEntry const* talentTabInfo = sTalentTabStore.LookupEntry(talentTabId);
        if (!talentTabInfo)
        {
            continue;
        }

        if (!((1 << pet_family->petTalentType) & talentTabInfo->petTalentMask))
        {
            continue;
        }

        for (uint32 talentId = 0; talentId < sTalentStore.GetNumRows(); ++talentId)
        {
            TalentEntry const* talentInfo = sTalentStore.LookupEntry(talentId);
            if (!talentInfo)
            {
                continue;
            }

            // skip another tab talents
            if (talentInfo->TalentTab != talentTabId)
            {
                continue;
            }

            // find max talent rank
            int32 curtalent_maxrank = -1;
            for (int32 k = 4; k > -1; --k)
            {
                if (talentInfo->RankID[k] && pet->HasSpell(talentInfo->RankID[k]))
                {
                    curtalent_maxrank = k;
                    break;
                }
            }

            // not learned talent
            if (curtalent_maxrank < 0)
            {
                continue;
            }

            *data << uint32(talentInfo->TalentID);          // Talent.dbc
            *data << uint8(curtalent_maxrank);              // talentMaxRank (0-4)

            ++talentIdCount;
        }

        data->put<uint8>(countPos, talentIdCount);          // put real count

        break;
    }
}

void Player::SendTalentsInfoData(bool pet)
{
    WorldPacket data(SMSG_TALENT_UPDATE, 50);
    data << uint8(pet ? 1 : 0);
    if (pet)
    {
        BuildPetTalentsInfoData(&data);
    }
    else
    {
        BuildPlayerTalentsInfoData(&data);
    }
    GetSession()->SendPacket(&data);
}

void Player::BuildEnchantmentsInfoData(WorldPacket* data)
{
    uint32 slotUsedMask = 0;
    size_t slotUsedMaskPos = data->wpos();
    *data << uint32(slotUsedMask);                          // slotUsedMask < 0x80000

    for (uint32 i = 0; i < EQUIPMENT_SLOT_END; ++i)
    {
        Item* item = GetItemByPos(INVENTORY_SLOT_BAG_0, i);

        if (!item)
        {
            continue;
        }

        slotUsedMask |= (1 << i);

        *data << uint32(item->GetEntry());                  // item entry

        uint16 enchantmentMask = 0;
        size_t enchantmentMaskPos = data->wpos();
        *data << uint16(enchantmentMask);                   // enchantmentMask < 0x1000

        for (uint32 j = 0; j < MAX_ENCHANTMENT_SLOT; ++j)
        {
            uint32 enchId = item->GetEnchantmentId(EnchantmentSlot(j));

            if (!enchId)
            {
                continue;
            }

            enchantmentMask |= (1 << j);

            *data << uint16(enchId);                        // enchantmentId?
        }

        data->put<uint16>(enchantmentMaskPos, enchantmentMask);

        *data << uint16(item->GetItemRandomPropertyId());
        *data << item->GetGuidValue(ITEM_FIELD_CREATOR).WriteAsPacked();
        *data << uint32(item->GetItemSuffixFactor());
    }

    data->put<uint32>(slotUsedMaskPos, slotUsedMask);
}

void Player::SendEquipmentSetList()
{
    uint32 count = 0;
    WorldPacket data(SMSG_LOAD_EQUIPMENT_SET, 4);
    size_t count_pos = data.wpos();
    data << uint32(count);                                  // count placeholder
    for (EquipmentSets::iterator itr = m_EquipmentSets.begin(); itr != m_EquipmentSets.end(); ++itr)
    {
        if (itr->second.state == EQUIPMENT_SET_DELETED)
        {
            continue;
        }
        data.appendPackGUID(itr->second.Guid);
        data << uint32(itr->first);
        data << itr->second.Name;
        data << itr->second.IconName;
        for (uint32 i = 0; i < EQUIPMENT_SLOT_END; ++i)
        {
            // ignored slots stored in IgnoreMask, client wants "1" as raw GUID, so no HIGHGUID_ITEM
            if (itr->second.IgnoreMask & (1 << i))
            {
                data << ObjectGuid(uint64(1)).WriteAsPacked();
            }
            else
            {
                data << ObjectGuid(HIGHGUID_ITEM, itr->second.Items[i]).WriteAsPacked();
            }
        }

        ++count;                                            // client have limit but it checked at loading and set
    }
    data.put<uint32>(count_pos, count);
    GetSession()->SendPacket(&data);
}

void Player::SetEquipmentSet(uint32 index, EquipmentSet eqset)
{
    if (eqset.Guid != 0)
    {
        bool found = false;

        for (EquipmentSets::iterator itr = m_EquipmentSets.begin(); itr != m_EquipmentSets.end(); ++itr)
        {
            if ((itr->second.Guid == eqset.Guid) && (itr->first == index))
            {
                found = true;
                break;
            }
        }

        if (!found)                                         // something wrong...
        {
            sLog.outError("Player %s tried to save equipment set " UI64FMTD " (index %u), but that equipment set not found!", GetName(), eqset.Guid, index);
            return;
        }
    }

    EquipmentSet& eqslot = m_EquipmentSets[index];

    EquipmentSetUpdateState old_state = eqslot.state;

    eqslot = eqset;

    if (eqset.Guid == 0)
    {
        eqslot.Guid = sObjectMgr.GenerateEquipmentSetGuid();

        WorldPacket data(SMSG_EQUIPMENT_SET_ID, 4 + 1);
        data << uint32(index);
        data.appendPackGUID(eqslot.Guid);
        GetSession()->SendPacket(&data);
    }

    eqslot.state = old_state == EQUIPMENT_SET_NEW ? EQUIPMENT_SET_NEW : EQUIPMENT_SET_CHANGED;
}

void Player::_SaveEquipmentSets()
{
    static SqlStatementID updSets ;
    static SqlStatementID insSets ;
    static SqlStatementID delSets ;

    for (EquipmentSets::iterator itr = m_EquipmentSets.begin(); itr != m_EquipmentSets.end();)
    {
        uint32 index = itr->first;
        EquipmentSet& eqset = itr->second;
        switch (eqset.state)
        {
            case EQUIPMENT_SET_UNCHANGED:
                ++itr;
                break;                                      // nothing do
            case EQUIPMENT_SET_CHANGED:
            {
                SqlStatement stmt = CharacterDatabase.CreateStatement(updSets, "UPDATE `character_equipmentsets` SET `name`=?, `iconname`=?, `ignore_mask`=?, `item0`=?, `item1`=?, `item2`=?, `item3`=?, `item4`=?, "
                                    "`item5`=?, `item6`=?, `item7`=?, `item8`=?, `item9`=?, `item10`=?, `item11`=?, `item12`=?, `item13`=?, `item14`=?, "
                                    "`item15`=?, `item16`=?, `item17`=?, `item18`=? WHERE `guid`=? AND `setguid`=? AND `setindex`=?");

                stmt.addString(eqset.Name);
                stmt.addString(eqset.IconName);
                stmt.addUInt32(eqset.IgnoreMask);

                for (int i = 0; i < EQUIPMENT_SLOT_END; ++i)
                {
                    stmt.addUInt32(eqset.Items[i]);
                }

                stmt.addUInt32(GetGUIDLow());
                stmt.addUInt64(eqset.Guid);
                stmt.addUInt32(index);

                stmt.Execute();

                eqset.state = EQUIPMENT_SET_UNCHANGED;
                ++itr;
                break;
            }
            case EQUIPMENT_SET_NEW:
            {
                SqlStatement stmt = CharacterDatabase.CreateStatement(insSets, "INSERT INTO `character_equipmentsets` VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
                stmt.addUInt32(GetGUIDLow());
                stmt.addUInt64(eqset.Guid);
                stmt.addUInt32(index);
                stmt.addString(eqset.Name);
                stmt.addString(eqset.IconName);
                stmt.addUInt32(eqset.IgnoreMask);

                for (int i = 0; i < EQUIPMENT_SLOT_END; ++i)
                {
                    stmt.addUInt32(eqset.Items[i]);
                }

                stmt.Execute();

                eqset.state = EQUIPMENT_SET_UNCHANGED;
                ++itr;
                break;
            }
            case EQUIPMENT_SET_DELETED:
            {
                SqlStatement stmt = CharacterDatabase.CreateStatement(delSets, "DELETE FROM `character_equipmentsets` WHERE `setguid` = ?");
                stmt.PExecute(eqset.Guid);
                m_EquipmentSets.erase(itr++);
                break;
            }
        }
    }
}

/**
 * @brief Saves battleground return position and instance data to the database.
 */
void Player::_SaveBGData()
{
    // nothing save
    if (!m_bgData.m_needSave)
    {
        return;
    }

    static SqlStatementID delBGData ;
    static SqlStatementID insBGData ;

    SqlStatement stmt =  CharacterDatabase.CreateStatement(delBGData, "DELETE FROM `character_battleground_data` WHERE `guid` = ?");

    stmt.PExecute(GetGUIDLow());

    if (m_bgData.bgInstanceID)
    {
        stmt = CharacterDatabase.CreateStatement(insBGData, "INSERT INTO `character_battleground_data` VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
        /* guid, bgInstanceID, bgTeam, x, y, z, o, map, taxi[0], taxi[1], mountSpell */
        stmt.addUInt32(GetGUIDLow());
        stmt.addUInt32(m_bgData.bgInstanceID);
        stmt.addUInt32(uint32(m_bgData.bgTeam));
        stmt.addFloat(m_bgData.joinPos.coord_x);
        stmt.addFloat(m_bgData.joinPos.coord_y);
        stmt.addFloat(m_bgData.joinPos.coord_z);
        stmt.addFloat(m_bgData.joinPos.orientation);
        stmt.addUInt32(m_bgData.joinPos.mapid);
        stmt.addUInt32(m_bgData.taxiPath[0]);
        stmt.addUInt32(m_bgData.taxiPath[1]);
        stmt.addUInt32(m_bgData.mountSpell);

        stmt.Execute();
    }

    m_bgData.m_needSave = false;
}

void Player::DeleteEquipmentSet(uint64 setGuid)
{
    for (EquipmentSets::iterator itr = m_EquipmentSets.begin(); itr != m_EquipmentSets.end(); ++itr)
    {
        if (itr->second.Guid == setGuid)
        {
            if (itr->second.state == EQUIPMENT_SET_NEW)
            {
                m_EquipmentSets.erase(itr);
            }
            else
            {
                itr->second.state = EQUIPMENT_SET_DELETED;
            }
            break;
        }
    }
}

void Player::ActivateSpec(uint8 specNum)
{
    if (GetActiveSpec() == specNum)
    {
        return;
    }

    if (specNum >= GetSpecsCount())
    {
        return;
    }

    UnsummonPetTemporaryIfAny();

    // prevent deletion of action buttons by client at spell unlearn or by player while spec change in progress
    SendLockActionButtons();

    ApplyGlyphs(false);

    // copy of new talent spec (we will use it as model for converting current tlanet state to new)
    PlayerTalentMap tempSpec = m_talents[specNum];

    // copy old spec talents to new one, must be before spec switch to have previous spec num(as m_activeSpec)
    m_talents[specNum] = m_talents[m_activeSpec];

    SetActiveSpec(specNum);

    // remove all talent spells that don't exist in next spec but exist in old
    for (PlayerTalentMap::iterator specIter = m_talents[m_activeSpec].begin(); specIter != m_talents[m_activeSpec].end();)
    {
        PlayerTalent& talent = specIter->second;

        if (talent.state == PLAYERSPELL_REMOVED)
        {
            ++specIter;
            continue;
        }

        PlayerTalentMap::iterator iterTempSpec = tempSpec.find(specIter->first);

        // remove any talent rank if talent not listed in temp spec
        if (iterTempSpec == tempSpec.end() || iterTempSpec->second.state == PLAYERSPELL_REMOVED)
        {
            TalentEntry const* talentInfo = talent.talentEntry;

            for (int r = 0; r < MAX_TALENT_RANK; ++r)
            {
                if (talentInfo->RankID[r])
                {
                    removeSpell(talentInfo->RankID[r], !IsPassiveSpell(talentInfo->RankID[r]), false);
                }
            }

            specIter = m_talents[m_activeSpec].begin();
        }
        else
        {
            ++specIter;
        }
    }

    // now new spec data have only talents (maybe different rank) as in temp spec data, sync ranks then.
    for (PlayerTalentMap::const_iterator tempIter = tempSpec.begin(); tempIter != tempSpec.end(); ++tempIter)
    {
        PlayerTalent const& talent = tempIter->second;

        // removed state talent already unlearned in prev. loop
        // but we need restore it if it deleted for finish removed-marked data in DB
        if (talent.state == PLAYERSPELL_REMOVED)
        {
            m_talents[m_activeSpec][tempIter->first] = talent;
            continue;
        }

        uint32 talentSpellId = talent.talentEntry->RankID[talent.currentRank];

        // learn talent spells if they not in new spec (old spec copy)
        // and if they have different rank
        if (PlayerTalent const* cur_talent = GetKnownTalentById(tempIter->first))
        {
            if (cur_talent->currentRank != talent.currentRank)
            {
                learnSpell(talentSpellId, false);
            }
        }
        else
        {
            learnSpell(talentSpellId, false);
        }

        // sync states - original state is changed in addSpell that learnSpell calls
        PlayerTalentMap::iterator specIter = m_talents[m_activeSpec].find(tempIter->first);
        if (specIter != m_talents[m_activeSpec].end())
        {
            specIter->second.state = talent.state;
        }
        else
        {
            sLog.outError("ActivateSpec: Talent spell %u expected to learned at spec switch but not listed in talents at final check!", talentSpellId);

            // attempt resync DB state (deleted lost spell from DB)
            if (talent.state != PLAYERSPELL_NEW)
            {
                PlayerTalent& talentNew = m_talents[m_activeSpec][tempIter->first];
                talentNew = talent;
                talentNew.state = PLAYERSPELL_REMOVED;
            }
        }
    }

    InitTalentForLevel();

    // recheck action buttons (not checked at loading/spec copy)
    ActionButtonList const& currentActionButtonList = m_actionButtons[m_activeSpec];
    for (ActionButtonList::const_iterator itr = currentActionButtonList.begin(); itr != currentActionButtonList.end();)
    {
        if (itr->second.uState != ACTIONBUTTON_DELETED)
        {
            // remove broken without any output (it can be not correct because talents not copied at spec creating)
            if (!IsActionButtonDataValid(itr->first, itr->second.GetAction(), itr->second.GetType(), this, false))
            {
                removeActionButton(m_activeSpec, itr->first);
                itr = currentActionButtonList.begin();
                continue;
            }
        }
        ++itr;
    }

    ResummonPetTemporaryUnSummonedIfAny();

    ApplyGlyphs(true);

    SendInitialActionButtons();

    Powers powerType = GetPowerType();
    if (powerType != POWER_MANA)
    {
        SetPower(POWER_MANA, 0);
    }

    SetPower(powerType, 0);
}

void Player::UpdateSpecCount(uint8 count)
{
    uint8 curCount = GetSpecsCount();
    if (curCount == count)
    {
        return;
    }

    // maybe current spec data must be copied to 0 spec?
    if (m_activeSpec >= count)
    {
        ActivateSpec(0);
    }

    // copy spec data from new specs
    if (count > curCount)
    {
        // copy action buttons from active spec (more easy in this case iterate first by button)
        ActionButtonList const& currentActionButtonList = m_actionButtons[m_activeSpec];

        for (ActionButtonList::const_iterator itr = currentActionButtonList.begin(); itr != currentActionButtonList.end(); ++itr)
        {
            if (itr->second.uState != ACTIONBUTTON_DELETED)
            {
                for (uint8 spec = curCount; spec < count; ++spec)
                {
                    addActionButton(spec, itr->first, itr->second.GetAction(), itr->second.GetType());
                }
            }
        }
    }
    // delete spec data for removed specs
    else if (count < curCount)
    {
        // delete action buttons for removed spec
        for (uint8 spec = count; spec < curCount; ++spec)
        {
            // delete action buttons for removed spec
            for (uint8 button = 0; button < MAX_ACTION_BUTTONS; ++button)
            {
                removeActionButton(spec, button);
            }
        }
    }

    SetSpecsCount(count);

    SendTalentsInfoData(false);
}

/**
 * @brief Adds or removes money from the player while clamping to valid limits.
 *
 * @param d The signed money delta.
 */
void Player::ModifyMoney(int32 d)
{
#ifdef ENABLE_ELUNA
    if (Eluna* e = GetEluna())
    {
        e->OnMoneyChanged(this, d);
    }
#endif /* ENABLE_ELUNA */

    if (d < 0)
    {
        SetMoney(GetMoney() > uint32(-d) ? GetMoney() + d : 0);
    }
    else
    {
        SetMoney(GetMoney() < uint32(MAX_MONEY_AMOUNT - d) ? GetMoney() + d : MAX_MONEY_AMOUNT);
    }

    // "At Gold Limit"
    if (GetMoney() >= MAX_MONEY_AMOUNT)
    {
        SendEquipError(EQUIP_ERR_TOO_MUCH_GOLD, NULL, NULL);
    }
}

/**
 * @brief Clears an at-login flag from the player and optionally from the database.
 *
 * @param f The flag to remove.
 * @param in_db_also True to persist the removal to the database immediately.
 */
void Player::RemoveAtLoginFlag(AtLoginFlags f, bool in_db_also /*= false*/)
{
    m_atLoginFlags &= ~f;

    if (in_db_also)
    {
        CharacterDatabase.PExecute("UPDATE `characters` set `at_login` = `at_login` & ~ %u WHERE `guid` ='%u'", uint32(f), GetGUIDLow());
    }
}

/**
 * @brief Sends a packet that clears a spell cooldown on the client.
 *
 * @param spell_id The spell whose cooldown is being cleared.
 * @param target The target unit associated with the cooldown clear.
 */
void Player::SendClearCooldown(uint32 spell_id, Unit* target)
{
    WorldPacket data(SMSG_CLEAR_COOLDOWN, 4 + 8);
    data << uint32(spell_id);
    data << target->GetObjectGuid();
    SendDirectMessage(&data);
}

/**
 * @brief Builds a teleport acknowledgement packet using a destination position.
 *
 * @param data The packet to populate.
 * @param x The destination X coordinate.
 * @param y The destination Y coordinate.
 * @param z The destination Z coordinate.
 * @param ang The destination orientation.
 */
void Player::BuildTeleportAckMsg(WorldPacket& data, float x, float y, float z, float ang) const
{
    MovementInfo mi = m_movementInfo;
    mi.ChangePosition(x, y, z, ang);

    data.Initialize(MSG_MOVE_TELEPORT_ACK, 64);
    data << GetPackGUID();
    data << uint32(0);                                      // this value increments every time
    data << mi;
}

/**
 * @brief Checks whether the player's movement info currently contains a flag.
 *
 * @param f The movement flag to test.
 * @return True if the flag is present; otherwise, false.
 */
bool Player::HasMovementFlag(MovementFlags f) const
{
    return m_movementInfo.HasMovementFlag(f);
}

void Player::ResetTimeSync()
{
    m_timeSyncCounter = 0;
    m_timeSyncTimer = 0;
    m_timeSyncClient = 0;
    m_timeSyncServer = GameTime::GetGameTimeMS();
}

void Player::SendTimeSync()
{
    WorldPacket data(SMSG_TIME_SYNC_REQ, 4);
    data << uint32(m_timeSyncCounter++);
    GetSession()->SendPacket(&data);

    // Schedule next sync in 10 sec
    m_timeSyncTimer = 10000;
    m_timeSyncServer = GameTime::GetGameTimeMS();
}


bool Player::IsImmuneToSpellEffect(SpellEntry const* spellInfo, SpellEffectIndex index, bool castOnSelf) const
{
    switch (spellInfo->Effect[index])
    {
        case SPELL_EFFECT_ATTACK_ME:
            return true;
        default:
            break;
    }
    switch (spellInfo->EffectApplyAuraName[index])
    {
        case SPELL_AURA_MOD_TAUNT:
            return true;
        default:
            break;
    }

    return Unit::IsImmuneToSpellEffect(spellInfo, index, castOnSelf);
}

/**
 * @brief Sets the player's home bind location and persists it to the database.
 *
 * @param loc The new home bind world location.
 * @param area_id The associated area identifier.
 */
void Player::SetHomebindToLocation(WorldLocation const& loc, uint32 area_id)
{
    m_homebindMapId = loc.mapid;
    m_homebindAreaId = area_id;
    m_homebindX = loc.coord_x;
    m_homebindY = loc.coord_y;
    m_homebindZ = loc.coord_z;

    // update sql homebind
    CharacterDatabase.PExecute("UPDATE `character_homebind` SET `map` = '%u', `zone` = '%u', `position_x` = '%f', `position_y` = '%f', `position_z` = '%f' WHERE `guid` = '%u'",
                               m_homebindMapId, m_homebindAreaId, m_homebindX, m_homebindY, m_homebindZ, GetGUIDLow());
}

/**
 * @brief Resolves an object GUID to a world object constrained by a type mask.
 *
 * @param guid The object GUID to resolve.
 * @param typemask The allowed object type mask.
 * @return The matching object, or NULL if not found or disallowed.
 */
Object* Player::GetObjectByTypeMask(ObjectGuid guid, TypeMask typemask)
{
    switch (guid.GetHigh())
    {
        case HIGHGUID_ITEM:
            if (typemask & TYPEMASK_ITEM)
            {
                return GetItemByGuid(guid);
            }
            break;
        case HIGHGUID_PLAYER:
            if (GetObjectGuid() == guid)
            {
                return this;
            }
            if ((typemask & TYPEMASK_PLAYER) && IsInWorld())
            {
                return sObjectAccessor.FindPlayer(guid);
            }
            break;
        case HIGHGUID_GAMEOBJECT:
            if ((typemask & TYPEMASK_GAMEOBJECT) && IsInWorld())
            {
                return GetMap()->GetGameObject(guid);
            }
            break;
        case HIGHGUID_UNIT:
        case HIGHGUID_VEHICLE:
            if ((typemask & TYPEMASK_UNIT) && IsInWorld())
            {
                return GetMap()->GetCreature(guid);
            }
            break;
        case HIGHGUID_PET:
            if ((typemask & TYPEMASK_UNIT) && IsInWorld())
            {
                return GetMap()->GetPet(guid);
            }
            break;
        case HIGHGUID_DYNAMICOBJECT:
            if ((typemask & TYPEMASK_DYNAMICOBJECT) && IsInWorld())
            {
                return GetMap()->GetDynamicObject(guid);
            }
            break;
        case HIGHGUID_TRANSPORT:
        case HIGHGUID_CORPSE:
        case HIGHGUID_MO_TRANSPORT:
        case HIGHGUID_INSTANCE:
        case HIGHGUID_GROUP:
        default:
            break;
    }

    return NULL;
}

/**
 * @brief Updates the player's current resting state.
 *
 * @param n_r_type The new rest type.
 * @param areaTriggerId The inn or rest area trigger identifier, if applicable.
 */
void Player::SetRestType(RestType n_r_type, uint32 areaTriggerId /*= 0*/)
{
    rest_type = n_r_type;

    if (rest_type == REST_TYPE_NO)
    {
        RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING);

        // Set player to FFA PVP when not in rested environment.
        if (sWorld.IsFFAPvPRealm())
        {
            SetFFAPvP(true);
        }
    }
    else
    {
        SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING);

        inn_trigger_id = areaTriggerId;
        time_inn_enter = time(NULL);

        if (sWorld.IsFFAPvPRealm())
        {
            SetFFAPvP(false);
        }
    }
}




/**
 * @brief Computes rested experience gained over a period of time.
 *
 * @param timePassed The elapsed time in seconds.
 * @param offline True when the gain is being computed for offline time.
 * @param inRestPlace True when the player is in a rest area while offline.
 * @return The amount of rested experience bonus gained.
 */
float Player::ComputeRest(time_t timePassed, bool offline /*= false*/, bool inRestPlace /*= false*/)
{
    // Every 8h in resting zone we gain a bubble
    // A bubble is 5% of the total xp so there are 20 bubbles
    // So we gain (total XP/20 every 8h) (8h = 288800 sec)
    // (TotalXP/20)/28800; simplified to (TotalXP/576000) per second
    // Client automatically double the value sent so we have to divide it by 2
    // So final formula (TotalXP/1152000)
    float bonus = timePassed * (GetUInt32Value(PLAYER_NEXT_LEVEL_XP) / 1152000.0f); // Get the gained rest xp for given second
    if (!offline)
    {
        bonus *= sWorld.getConfig(CONFIG_FLOAT_RATE_REST_INGAME);                   // Apply the custom setting
    }
    else
    {
        if (inRestPlace)
        {
            bonus *= sWorld.getConfig(CONFIG_FLOAT_RATE_REST_OFFLINE_IN_TAVERN_OR_CITY);
        }
        else
        {
            bonus *= sWorld.getConfig(CONFIG_FLOAT_RATE_REST_OFFLINE_IN_WILDERNESS) / 4.0f; // bonus is reduced by 4 when not in rest place
        }
    }
    return bonus;
}

float Player::GetCollisionHeight(bool mounted) const
{
    if (mounted)
    {
        // mounted case
        CreatureDisplayInfoEntry const* mountDisplayInfo = sCreatureDisplayInfoStore.LookupEntry(GetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID));
        if (!mountDisplayInfo)
        {
            return GetCollisionHeight(false);
        }

        CreatureModelDataEntry const* mountModelData = sCreatureModelDataStore.LookupEntry(mountDisplayInfo->ModelId);
        if (!mountModelData)
        {
            return GetCollisionHeight(false);
        }

        CreatureDisplayInfoEntry const* displayInfo = sCreatureDisplayInfoStore.LookupEntry(GetNativeDisplayId());
        if (!displayInfo)
        {
            sLog.outError("GetCollisionHeight::Unable to find CreatureDisplayInfoEntry for %u", GetNativeDisplayId());
            return 0;
        }

        CreatureModelDataEntry const* modelData = sCreatureModelDataStore.LookupEntry(displayInfo->ModelId);
        if (!modelData)
        {
            sLog.outError("GetCollisionHeight::Unable to find CreatureModelDataEntry for %u", displayInfo->ModelId);
            return 0;
        }

        float scaleMod = GetObjectScale(); // 99% sure about this

        return scaleMod * mountModelData->MountHeight + modelData->CollisionHeight * 0.5f;
    }
    else
    {
        // use native model collision height in dismounted case
        CreatureDisplayInfoEntry const* displayInfo = sCreatureDisplayInfoStore.LookupEntry(GetNativeDisplayId());
        if (!displayInfo)
        {
            sLog.outError("GetCollisionHeight::Unable to find CreatureDisplayInfoEntry for %u", GetNativeDisplayId());
            return 0;
        }

        CreatureModelDataEntry const* modelData = sCreatureModelDataStore.LookupEntry(displayInfo->ModelId);
        if (!modelData)
        {
            sLog.outError("GetCollisionHeight::Unable to find CreatureModelDataEntry for %u", displayInfo->ModelId);
            return 0;
        }

        return modelData->CollisionHeight;
    }
}

void Player::SetRandomWinner(bool isWinner)
{
    m_IsBGRandomWinner = isWinner;
    if (m_IsBGRandomWinner)
    {
        CharacterDatabase.PExecute("INSERT INTO `character_battleground_random` (`guid`) VALUES ('%u')", GetGUIDLow());
    }
}

void Player::_LoadRandomBGStatus(QueryResult *result)
{
    // QueryResult* result = CharacterDatabase.PQuery("SELECT `guid` FROM `character_battleground_random` WHERE `guid` = '%u'", GetGUIDLow());

    if (result)
    {
        m_IsBGRandomWinner = true;
        delete result;
    }
}












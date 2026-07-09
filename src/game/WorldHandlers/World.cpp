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
 * @file World.cpp
 * @brief Core world server implementation
 *
 * This file implements the World class, which is the central hub of the
 * MaNGOS game server. It manages:
 * - Server configuration and settings
 * - Game time and world updates
 * - Player sessions and limits
 * - All game systems initialization (maps, spells, quests, etc.)
 * - Server shutdown and restart procedures
 * - In-game announcements and events
 * - Various utility functions for the game world
 *
 * The World class is a singleton accessed via sWorld, and runs the main
 * server loop that processes all game logic.
 *
 * @ingroup world
 */

#include "World.h"
#include "Database/DatabaseEnv.h"
#include "Config/Config.h"
#include "Platform/Define.h"
#include "SystemConfig.h"
#include "Log.h"
#include "Opcodes.h"
#include "WorldSession.h"
#include "WorldPacket.h"
#include "Player.h"
#include "SkillExtraItems.h"
#include "SkillDiscovery.h"
#include "AccountMgr.h"
#include "AchievementMgr.h"
#include "AuctionHouseMgr.h"
#include "ObjectMgr.h"
#include "CreatureEventAIMgr.h"
#include "GuildMgr.h"
#include "SpellMgr.h"
#include "Chat.h"
#include "DBCStores.h"
#include "MassMailMgr.h"
#include "LootMgr.h"
#include "ItemEnchantmentMgr.h"
#include "MapManager.h"
#include "ScriptMgr.h"
#include "CreatureAIRegistry.h"
#include "ProgressBar.h"
#include "Policies/Singleton.h"
#include "BattleGround/BattleGroundMgr.h"
#include "OutdoorPvP/OutdoorPvP.h"
#include "TemporarySummon.h"
#include "VMapFactory.h"
#include "MoveMap.h"
#include "GameEventMgr.h"
#include "PoolManager.h"
#include "Database/DatabaseImpl.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "MapPersistentStateMgr.h"
#include "WaypointManager.h"
#include "GMTicketMgr.h"
#include "Util.h"
#include "AuctionHouseBot/AuctionHouseBot.h"
#include "CharacterDatabaseCleaner.h"
#include "CreatureLinkingMgr.h"
#include "Calendar.h"
#include "Weather.h"
#include "LFGMgr.h"
#include "DisableMgr.h"
#include "Language.h"
#include "CommandMgr.h"
#include "GitRevision.h"
#include "UpdateTime.h"
#include "GameTime.h"

#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#include "ElunaConfig.h"
#include "ElunaLoader.h"
#endif /* ENABLE_ELUNA */

// WARDEN
#include "WardenCheckMgr.h"

#include <iostream>
#include <sstream>

INSTANTIATE_SINGLETON_1(World);

extern void LoadGameObjectModelList();

volatile bool World::m_stopEvent = false;
uint8 World::m_ExitCode = SHUTDOWN_EXIT_CODE;

ACE_Atomic_Op<ACE_Thread_Mutex, uint32> World::m_worldLoopCounter = 0;

float World::m_MaxVisibleDistanceOnContinents = DEFAULT_VISIBILITY_DISTANCE;
float World::m_MaxVisibleDistanceInInstances  = DEFAULT_VISIBILITY_INSTANCE;
float World::m_MaxVisibleDistanceInBGArenas   = DEFAULT_VISIBILITY_BGARENAS;

float World::m_MaxVisibleDistanceInFlight     = DEFAULT_VISIBILITY_DISTANCE;
float World::m_VisibleUnitGreyDistance        = 0;
float World::m_VisibleObjectGreyDistance      = 0;

float  World::m_relocation_lower_limit_sq     = 10.f * 10.f;
uint32 World::m_relocation_ai_notify_delay    = 1000u;

bool   World::m_visibility_observer_sweep_enabled  = true;
uint32 World::m_visibility_observer_sweep_interval = 2000u;

/**
 * @brief World class constructor
 *
 * Initializes all world state variables to their default values:
 * - Player limit set to 0 (unlimited)
 * - Movement allowed
 * - Shutdown mask and timer cleared
 * - Game time set to current system time
 * - Session counts zeroed
 * - All config arrays cleared
 * - DBC locale set to enUS
 * - Broadcast system disabled
 *
 * This is called once when the server starts, before any configuration
 * is loaded or systems initialized.
 */
World::World()
{
    m_playerLimit = 0;
    m_allowMovement = true;
    m_ShutdownMask = 0;
    m_ShutdownTimer = 0;
    m_gameTime = time(NULL);
    m_startTime = m_gameTime;
    m_maxActiveSessionCount = 0;
    m_maxQueuedSessionCount = 0;
    m_NextDailyQuestReset = 0;
    m_NextWeeklyQuestReset = 0;
    m_broadcastEnable = false;
    m_broadcastList.clear();
    m_broadcastWeight = 0;

    m_defaultDbcLocale = LOCALE_enUS;
    m_availableDbcLocaleMask = 0;

    for (int i = 0; i < CONFIG_UINT32_VALUE_COUNT; ++i)
    {
        m_configUint32Values[i] = 0;
    }

    for (int i = 0; i < CONFIG_INT32_VALUE_COUNT; ++i)
    {
        m_configInt32Values[i] = 0;
    }

    for (int i = 0; i < CONFIG_FLOAT_VALUE_COUNT; ++i)
    {
        m_configFloatValues[i] = 0.0f;
    }

    for (int i = 0; i < CONFIG_BOOL_VALUE_COUNT; ++i)
    {
        m_configBoolValues[i] = false;
    }
}

/// World destructor
World::~World()
{
#ifdef ENABLE_ELUNA
    // Delete world Eluna state
    delete eluna;
    eluna = nullptr;
#endif /* ENABLE_ELUNA */

    ///- Empty the kicked session set
    while (!m_sessions.empty())
    {
        // not remove from queue, prevent loading new sessions
        delete m_sessions.begin()->second;
        m_sessions.erase(m_sessions.begin());
    }

    CliCommandHolder* command = NULL;
    while (cliCmdQueue.next(command))
    {
        delete command;
    }

    WorldSession* session = NULL;
    while (addSessQueue.next(session))
    {
        delete session;
    }

    VMAP::VMapFactory::clear();
    MMAP::MMapFactory::clear();
}

/// Cleanups before world stop
void World::CleanupsBeforeStop()
{
    KickAll();                                       // save and kick all players
    UpdateSessions(1);                               // real players unload required UpdateSessions call
    sBattleGroundMgr.DeleteAllBattleGrounds();       // unload battleground templates before different singletons destroyed
}




void
World::AddSession_(WorldSession* s)
{
    MANGOS_ASSERT(s);

    // NOTE - Still there is race condition in WorldSession* being used in the Sockets

    ///- kick already loaded player with same account (if any) and remove session
    ///- if player is in loading and want to load again, return
    if (!RemoveSession(s->GetAccountId()))
    {
        s->KickPlayer();
        delete s;                                           // session not added yet in session list, so not listed in queue
        return;
    }

    // decrease session counts only at not reconnection case
    bool decrease_session = true;

    // if session already exist, prepare to it deleting at next world update
    // NOTE - KickPlayer() should be called on "old" in RemoveSession()
    {
        SessionMap::const_iterator old = m_sessions.find(s->GetAccountId());

        if (old != m_sessions.end())
        {
            // prevent decrease sessions count if session queued
            if (RemoveQueuedSession(old->second))
            {
                decrease_session = false;
            }
            // not remove replaced session form queue if listed
            delete old->second;
        }
    }

    m_sessions[s->GetAccountId()] = s;

    uint32 Sessions = GetActiveAndQueuedSessionCount();
    uint32 pLimit = GetPlayerAmountLimit();
    uint32 QueueSize = GetQueuedSessionCount();             // number of players in the queue

    // so we don't count the user trying to
    // login as a session and queue the socket that we are using
    if (decrease_session)
    {
        --Sessions;
    }

    if (pLimit > 0 && Sessions >= pLimit && s->GetSecurity() == SEC_PLAYER)
    {
        AddQueuedSession(s);
        UpdateMaxSessionCounters();
        DETAIL_LOG("PlayerQueue: Account id %u is in Queue Position (%u).", s->GetAccountId(), ++QueueSize);
        return;
    }

    WorldPacket packet(SMSG_AUTH_RESPONSE, 1 + 4 + 1 + 4 + 1);
    packet << uint8(AUTH_OK);
    packet << uint32(0);                                    // BillingTimeRemaining
    packet << uint8(0);                                     // BillingPlanFlags
    packet << uint32(0);                                    // BillingTimeRested
    packet << uint8(s->Expansion());                        // 0 - normal, 1 - TBC, 2 - WotLK. Must be set in database manually for each account.
    s->SendPacket(&packet);

    s->SendAddonsInfo();

    WorldPacket pkt(SMSG_CLIENTCACHE_VERSION, 4);
    pkt << uint32(getConfig(CONFIG_UINT32_CLIENTCACHE_VERSION));
    s->SendPacket(&pkt);

    s->SendTutorialsData();

    UpdateMaxSessionCounters();

    // Updates the population
    if (pLimit > 0)
    {
        float popu = float(GetActiveSessionCount());        // updated number of users on the server
        popu /= pLimit;
        popu *= 2;

        static SqlStatementID id;

        SqlStatement stmt = LoginDatabase.CreateStatement(id, "UPDATE `realmlist` SET `population` = ? WHERE `id` = ?");
        stmt.PExecute(popu, realmID);

        DETAIL_LOG("Server Population (%f).", popu);
    }
}





/// Initialize the World
void World::SetInitialWorldSettings()
{
    ///- Initialize the random number generator
    srand((unsigned int)time(NULL));

    ///- Time server startup
    uint32 startupBegin = GameTime::GetGameTimeMS();

    ///- Initialize detour memory management
    dtAllocSetCustom(dtCustomAlloc, dtCustomFree);

    ///- Initialize config settings
    LoadConfigSettings();

    ///- Initialize VMapManager function pointers (to untangle game/collision circular deps)
    if (VMAP::VMapManager2* vmmgr2 = dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager()))
    {
        //vmmgr2->GetLiquidFlagsPtr = &GetLiquidFlags;
        vmmgr2->IsVMAPDisabledForPtr = &DisableMgr::IsVMAPDisabledFor;
    }

    ///- Check the existence of the map files for all races start areas.
    if (!MapManager::ExistMapAndVMap(0, -6240.32f, 331.033f) ||                     // Dwarf/ Gnome
        !MapManager::ExistMapAndVMap(0, -8949.95f, -132.493f) ||                // Human
        !MapManager::ExistMapAndVMap(1, -618.518f, -4251.67f) ||                // Orc
        !MapManager::ExistMapAndVMap(0, 1676.35f, 1677.45f) ||                  // Scourge
        !MapManager::ExistMapAndVMap(1, 10311.3f, 832.463f) ||                  // NightElf
        !MapManager::ExistMapAndVMap(1, -2917.58f, -257.98f) ||                 // Tauren
        (m_configUint32Values[CONFIG_UINT32_EXPANSION] >= EXPANSION_TBC &&
          (!MapManager::ExistMapAndVMap(530, 10349.6f, -6357.29f) ||            // BloodElf
              !MapManager::ExistMapAndVMap(530, -3961.64f, -13931.2f))) ||          // Draenei
            (m_configUint32Values[CONFIG_UINT32_EXPANSION] >= EXPANSION_WOTLK &&
              !MapManager::ExistMapAndVMap(609, 2355.84f, -5664.77f)))              // Death Knight
    {
        sLog.outError("Correct *.map files not found in path '%smaps' or *.vmtree/*.vmtile files in '%svmaps'. Please place *.map and vmap files in appropriate directories or correct the DataDir value in the mangosd.conf file.", m_dataPath.c_str(), m_dataPath.c_str());
        Log::WaitBeforeContinueIfNeed();
        exit(1);
    }

    ///- Loading strings. Getting no records means core load has to be canceled because no error message can be output.
    sLog.outString("Loading MaNGOS strings...");
    if (!sObjectMgr.LoadMangosStrings())
    {
        Log::WaitBeforeContinueIfNeed();
        exit(1);                                            // Error message displayed in function already
    }

    ///- Update the realm entry in the database with the realm type from the config file
    // No SQL injection as values are treated as integers

    // not send custom type REALM_FFA_PVP to realm list
    uint32 server_type = IsFFAPvPRealm() ? uint32(REALM_TYPE_PVP) : getConfig(CONFIG_UINT32_GAME_TYPE);
    uint32 realm_zone = getConfig(CONFIG_UINT32_REALM_ZONE);
    LoginDatabase.PExecute("UPDATE `realmlist` SET `icon` = %u, `timezone` = %u WHERE `id` = '%u'", server_type, realm_zone, realmID);

    ///- Remove the bones (they should not exist in DB though) and old corpses after a restart
    CharacterDatabase.PExecute("DELETE FROM `corpse` WHERE `corpse_type` = '0' OR `time` < (UNIX_TIMESTAMP()-'%u')", 3 * DAY);

    ///- Load the DBC files
    sLog.outString("Initialize DBC data stores...");
    LoadDBCStores(m_dataPath);
    DetectDBCLang();
    sObjectMgr.SetDBCLocaleIndex(GetDefaultDbcLocale());    // Get once for all the locale index of DBC language (console/broadcasts)

    sLog.outString("Loading SpellTemplate...");
    sObjectMgr.LoadSpellTemplate();

    sLog.outString("Loading Script Names...");
    sScriptMgr.LoadScriptNames();

    sLog.outString("Loading InstanceTemplate...");
    sObjectMgr.LoadInstanceTemplate();

    sLog.outString("Loading SkillLineAbilityMultiMap Data...");
    sSpellMgr.LoadSkillLineAbilityMap();

    sLog.outString("Loading SkillRaceClassInfoMultiMap Data...");
    sSpellMgr.LoadSkillRaceClassInfoMap();

    ///- Clean up and pack instances
    sLog.outString("Cleaning up instances...");
    sMapPersistentStateMgr.CleanupInstances();              // must be called before `creature_respawn`/`gameobject_respawn` tables

    sLog.outString("Packing instances...");
    sMapPersistentStateMgr.PackInstances();

    sLog.outString("Packing groups...");
    sObjectMgr.PackGroupIds();                              // must be after CleanupInstances

    ///- Init highest guids before any guid using table loading to prevent using not initialized guids in some code.
    sObjectMgr.SetHighestGuids();                           // must be after packing instances
    sLog.outString();

#ifdef ENABLE_ELUNA
    ///- Initialize Lua Engine

    // lua state begins uninitialized
    eluna = nullptr;

    sLog.outString("Loading Eluna config...");
    sElunaConfig->Initialize();

    if (sElunaConfig->IsElunaEnabled())
    {
        ///- Initialize Lua Engine
        sLog.outString("Loading Lua scripts...");
        sElunaLoader->LoadScripts();
    }
#endif /* ENABLE_ELUNA */

    sLog.outString("Loading Page Texts...");
    sObjectMgr.LoadPageTexts();

    sLog.outString("Loading Game Object Templates...");     // must be after LoadPageTexts
    sObjectMgr.LoadGameobjectInfo();

    sLog.outString("Loading GameObject models...");
    LoadGameObjectModelList();
    sLog.outString();

    sLog.outString("Loading Spell Chain Data...");
    sSpellMgr.LoadSpellChains();

    sLog.outString("Loading Spell Elixir types...");
    sSpellMgr.LoadSpellElixirs();

    sLog.outString("Loading Spell Learn Skills...");
    sSpellMgr.LoadSpellLearnSkills();                       // must be after LoadSpellChains

    sLog.outString("Loading Spell Learn Spells...");
    sSpellMgr.LoadSpellLearnSpells();

    sLog.outString("Loading Spell Proc Event conditions...");
    sSpellMgr.LoadSpellProcEvents();

    sLog.outString("Loading Spell Bonus Data...");
    sSpellMgr.LoadSpellBonuses();                           // must be after LoadSpellChains

    sLog.outString("Loading Spell Proc Item Enchant...");
    sSpellMgr.LoadSpellProcItemEnchant();                   // must be after LoadSpellChains

    sLog.outString("Loading Aggro Spells Definitions...");
    sSpellMgr.LoadSpellThreats();

    sLog.outString("Loading NPC Texts...");
    sObjectMgr.LoadGossipText();

    sLog.outString("Loading Item Random Enchantments Table...");
    LoadRandomEnchantmentsTable();

    sLog.outString("Loading Disables...");                  // must be before loading quests and items
    DisableMgr::LoadDisables();

    sLog.outString("Loading Item Templates...");            // must be after LoadRandomEnchantmentsTable and LoadPageTexts
    sObjectMgr.LoadItemPrototypes();

    sLog.outString("Loading Item converts...");             // must be after LoadItemPrototypes
    sObjectMgr.LoadItemConverts();

    sLog.outString("Loading Item expire converts...");      // must be after LoadItemPrototypes
    sObjectMgr.LoadItemExpireConverts();

    sLog.outString("Loading Creature Model Based Info Data...");
    sObjectMgr.LoadCreatureModelInfo();

    sLog.outString("Loading Equipment templates...");
    sObjectMgr.LoadEquipmentTemplates();

    sLog.outString("Loading Creature Stats...");
    sObjectMgr.LoadCreatureClassLvlStats();

    sLog.outString("Loading Creature templates...");
    sObjectMgr.LoadCreatureTemplates();

    sLog.outString("Loading Creature template spells...");
    sObjectMgr.LoadCreatureTemplateSpells();

    sLog.outString("Loading Creature Model for race...");   // must be after creature templates
    sObjectMgr.LoadCreatureModelRace();

    sLog.outString("Loading SpellsScriptTarget...");
    sSpellMgr.LoadSpellScriptTarget();                      // must be after LoadCreatureTemplates and LoadGameobjectInfo

    sLog.outString("Loading Vehicle Accessory...");         // must be after creature templates
    sObjectMgr.LoadVehicleAccessory();

    sLog.outString("Loading ItemRequiredTarget...");
    sObjectMgr.LoadItemRequiredTarget();

    sLog.outString("Loading Reputation Reward Rates...");
    sObjectMgr.LoadReputationRewardRate();

    sLog.outString("Loading Creature Reputation OnKill Data...");
    sObjectMgr.LoadReputationOnKill();

    sLog.outString("Loading Reputation Spillover Data...");
    sObjectMgr.LoadReputationSpilloverTemplate();

    sLog.outString("Loading Points Of Interest Data...");
    sObjectMgr.LoadPointsOfInterest();

    sLog.outString("Loading Creature Data...");
    sObjectMgr.LoadCreatures();

    sLog.outString("Loading pet levelup spells...");
    sSpellMgr.LoadPetLevelupSpellMap();

    sLog.outString("Loading pet default spell additional to levelup spells...");
    sSpellMgr.LoadPetDefaultSpells();

    sLog.outString("Loading Creature Addon Data...");
    sObjectMgr.LoadCreatureAddons();                        // must be after LoadCreatureTemplates() and LoadCreatures()
    sLog.outString(">>> Creature Addon Data loaded");
    sLog.outString();

    sLog.outString("Loading Gameobject Data...");
    sObjectMgr.LoadGameObjects();

    sLog.outString("Loading CreatureLinking Data...");      // must be after Creatures
    sCreatureLinkingMgr.LoadFromDB();

    sLog.outString("Loading Objects Pooling Data...");
    sPoolMgr.LoadFromDB();

    sLog.outString("Loading Weather Data...");
    sWeatherMgr.LoadWeatherZoneChances();

    sLog.outString("Loading Quests...");
    sObjectMgr.LoadQuests();                                // must be loaded after DBCs, creature_template, item_template, gameobject tables

    sLog.outString("Loading Quest POI");
    sObjectMgr.LoadQuestPOI();

    sLog.outString("Loading Quests Relations...");
    sObjectMgr.LoadQuestRelations();                        // must be after quest load
    sLog.outString(">>> Quests Relations loaded");
    sLog.outString();

    sLog.outString("Checking Quest Disables...");
    DisableMgr::CheckQuestDisables();                       // must be after loading quests

    sLog.outString("Loading Game Event Data...");           // must be after sPoolMgr.LoadFromDB and quests to properly load pool events and quests for events
    sGameEventMgr.LoadFromDB();
    sLog.outString(">>> Game Event Data loaded");
    sLog.outString();

    // Load Conditions
    sLog.outString("Loading Conditions...");
    sObjectMgr.LoadConditions();

    sLog.outString("Creating map persistent states for non-instanceable maps...");     // must be after PackInstances(), LoadCreatures(), sPoolMgr.LoadFromDB(), sGameEventMgr.LoadFromDB();
    sMapPersistentStateMgr.InitWorldMaps();
    sLog.outString();

    sLog.outString("Loading Creature Respawn Data...");     // must be after LoadCreatures(), and sMapPersistentStateMgr.InitWorldMaps()
    sMapPersistentStateMgr.LoadCreatureRespawnTimes();

    sLog.outString("Loading Gameobject Respawn Data...");   // must be after LoadGameObjects(), and sMapPersistentStateMgr.InitWorldMaps()
    sMapPersistentStateMgr.LoadGameobjectRespawnTimes();

    sLog.outString("Loading UNIT_NPC_FLAG_SPELLCLICK Data...");
    sObjectMgr.LoadNPCSpellClickSpells();

    sLog.outString("Loading SpellArea Data...");            // must be after quest load
    sSpellMgr.LoadSpellAreas();

    sLog.outString("Loading AreaTrigger definitions...");
    sObjectMgr.LoadAreaTriggerTeleports();                  // must be after item template load

    sLog.outString("Loading Quest Area Triggers...");
    sObjectMgr.LoadQuestAreaTriggers();                     // must be after LoadQuests

    sLog.outString("Loading Tavern Area Triggers...");
    sObjectMgr.LoadTavernAreaTriggers();

#ifdef ENABLE_SD3
    sLog.outString("Loading all script bindings...");
    sScriptMgr.LoadScriptBinding();
#endif /* ENABLE_SD3 */

    sLog.outString("Loading Graveyard-zone links...");
    sObjectMgr.LoadGraveyardZones();

    sLog.outString("Loading spell target destination coordinates...");
    sSpellMgr.LoadSpellTargetPositions();

    sLog.outString("Loading spell pet auras...");
    sSpellMgr.LoadSpellPetAuras();

    sLog.outString("Loading Player Create Info & Level Stats...");
    sObjectMgr.LoadPlayerInfo();
    sLog.outString(">>> Player Create Info & Level Stats loaded");
    sLog.outString();

    sLog.outString("Loading Exploration BaseXP Data...");
    sObjectMgr.LoadExplorationBaseXP();

    sLog.outString("Loading Pet Name Parts...");
    sObjectMgr.LoadPetNames();

    CharacterDatabaseCleaner::CleanDatabase();
    sLog.outString();

    sLog.outString("Loading the max pet number...");
    sObjectMgr.LoadPetNumber();

    sLog.outString("Loading pet level stats...");
    sObjectMgr.LoadPetLevelInfo();

    sLog.outString("Loading Player Corpses...");
    sObjectMgr.LoadCorpses();

    sLog.outString("Loading Player level dependent mail rewards...");
    sObjectMgr.LoadMailLevelRewards();

    sLog.outString("Loading Loot Tables...");
    LoadLootTables();
    sLog.outString(">>> Loot Tables loaded");
    sLog.outString();

    sLog.outString("Loading Skill Discovery Table...");
    LoadSkillDiscoveryTable();

    sLog.outString("Loading Skill Extra Item Table...");
    LoadSkillExtraItemTable();

    sLog.outString("Loading Skill Fishing base level requirements...");
    sObjectMgr.LoadFishingBaseSkillLevel();

    sLog.outString();
    sLog.outString("Loading Achievements...");
    sAchievementMgr.LoadAchievementReferenceList();
    sAchievementMgr.LoadAchievementCriteriaList();
    sAchievementMgr.LoadAchievementCriteriaRequirements();
    sAchievementMgr.LoadRewards();
    sAchievementMgr.LoadRewardLocales();
    sAchievementMgr.LoadCompletedAchievements();
    sAchievementMgr.CleanupOrphanedCriteriaProgress();
    sLog.outString(">>> Achievements loaded");
    sLog.outString();

    sLog.outString("Loading Instance encounters data...");  // must be after Creature loading
    sObjectMgr.LoadInstanceEncounters();

    sLog.outString("Loading Gossip scripts...");
    sScriptMgr.LoadDbScripts(DBS_ON_GOSSIP);                 // must be before gossip menu options

    sObjectMgr.LoadGossipMenus();

    sLog.outString("Loading Vendors...");
    sObjectMgr.LoadVendorTemplates();                       // must be after load ItemTemplate
    sObjectMgr.LoadVendors();                               // must be after load CreatureTemplate, VendorTemplate, and ItemTemplate

    sLog.outString("Loading Trainers...");
    sObjectMgr.LoadTrainerTemplates();                      // must be after load CreatureTemplate
    sObjectMgr.LoadTrainers();                              // must be after load CreatureTemplate, TrainerTemplate

    sLog.outString("Loading Waypoint scripts...");          // before loading from creature_movement
    sScriptMgr.LoadDbScripts(DBS_ON_CREATURE_MOVEMENT);

    sLog.outString("Loading Waypoints...");
    sWaypointMgr.Load();

    ///- Loading localization data
    sLog.outString("Loading Localization strings...");
    sObjectMgr.LoadCreatureLocales();                       // must be after CreatureInfo loading
    sObjectMgr.LoadGameObjectLocales();                     // must be after GameobjectInfo loading
    sObjectMgr.LoadItemLocales();                           // must be after ItemPrototypes loading
    sObjectMgr.LoadQuestLocales();                          // must be after QuestTemplates loading
    sObjectMgr.LoadGossipTextLocales();                     // must be after LoadGossipText
    sObjectMgr.LoadPageTextLocales();                       // must be after PageText loading
    sObjectMgr.LoadGossipMenuItemsLocales();                // must be after gossip menu items loading
    sObjectMgr.LoadPointOfInterestLocales();                // must be after POI loading
    sCommandMgr.LoadCommandHelpLocale();
    sLog.outString(">>> Localization strings loaded");
    sLog.outString();

    ///- Load dynamic data tables from the database
    sLog.outString("Loading Auctions...");
    sAuctionMgr.LoadAuctionItems();
    sAuctionMgr.LoadAuctions();
    sLog.outString(">>> Auctions loaded");
    sLog.outString();

    sLog.outString("Loading Guilds...");
    sGuildMgr.LoadGuilds();

    sLog.outString("Loading ArenaTeams...");
    sObjectMgr.LoadArenaTeams();

    sLog.outString("Loading Groups...");
    sObjectMgr.LoadGroups();

    sCalendarMgr.LoadCalendarsFromDB();

    sLog.outString("Loading ReservedNames...");
    sObjectMgr.LoadReservedPlayersNames();

    sLog.outString("Loading GameObjects for quests...");
    sObjectMgr.LoadGameObjectForQuests();

    sLog.outString("Loading BattleMasters...");
    sBattleGroundMgr.LoadBattleMastersEntry();

    sLog.outString("Loading BattleGround event indexes...");
    sBattleGroundMgr.LoadBattleEventIndexes();

    sLog.outString("Loading GameTeleports...");
    sObjectMgr.LoadGameTele();

    sLog.outString("Loading GM tickets...");
    sTicketMgr.LoadGMTickets();

    sLog.outString("Loading Dungeon Finder Requirements...");
    sObjectMgr.LoadDungeonFinderRequirements();

    sLog.outString("Loading Dungeon Finder Rewards...");
    sObjectMgr.LoadDungeonFinderRewards();

    sLog.outString("Loading Dungeon Finder Items...");
    sObjectMgr.LoadDungeonFinderItems();

    ///- Handle outdated emails (delete/return)
    sLog.outString("Returning old mails...");
    sObjectMgr.ReturnOrDeleteOldMails(false);

#ifdef ENABLE_ELUNA
    if (sElunaConfig->IsElunaEnabled())
    {
        ///- Run eluna scripts.
        sLog.outString("Starting Eluna world state...");
        // use map id -1 for the global Eluna state
        eluna = new Eluna(nullptr);
        sLog.outString();
    }
#endif /*ENABLE_ELUNA*/

    ///- Load and initialize DBScripts Engine
    sLog.outString("Loading DB-Scripts Engine...");
    sScriptMgr.LoadDbScripts(DBS_ON_QUEST_START);           // must be after load Creature/Gameobject(Template/Data) and QuestTemplate
    sScriptMgr.LoadDbScripts(DBS_ON_QUEST_END);             // must be after load Creature/Gameobject(Template/Data) and QuestTemplate
    sScriptMgr.LoadDbScripts(DBS_ON_SPELL);                 // must be after load Creature/Gameobject(Template/Data)
    sScriptMgr.LoadDbScripts(DBS_ON_GO_USE);                // must be after load Creature/Gameobject(Template/Data)
    sScriptMgr.LoadDbScripts(DBS_ON_GOT_USE);               // must be after load Creature/Gameobject(Template/Data)
    sScriptMgr.LoadDbScripts(DBS_ON_EVENT);                 // must be after load Creature/Gameobject(Template/Data)
    sScriptMgr.LoadDbScripts(DBS_ON_CREATURE_DEATH);        // must be after load Creature/Gameobject(Template/Data)
    sLog.outString(">>> DB Scripts loaded");
    sLog.outString();

    sLog.outString("Loading Scripts text locales...");      // must be after Load*Scripts calls
    sScriptMgr.LoadDbScriptStrings();

    ///- Load and initialize EventAI Scripts
    sLog.outString("Loading CreatureEventAI Texts...");
    sEventAIMgr.LoadCreatureEventAI_Texts(false);           // false, will checked in LoadCreatureEventAI_Scripts

    sLog.outString("Loading CreatureEventAI Summons...");
    sEventAIMgr.LoadCreatureEventAI_Summons(false);         // false, will checked in LoadCreatureEventAI_Scripts

    sLog.outString("Loading CreatureEventAI Scripts...");
    sEventAIMgr.LoadCreatureEventAI_Scripts();

    sLog.outString("Initializing Scripts...");
#ifdef ENABLE_SD3
    switch (sScriptMgr.LoadScriptLibrary("mangosscript"))
    {
        case SCRIPT_LOAD_OK:
            sLog.outString("Scripting library loaded.");
            break;
        case SCRIPT_LOAD_ERR_NOT_FOUND:
            sLog.outError("Scripting library not found or not accessible.");
            break;
        case SCRIPT_LOAD_ERR_WRONG_API:
            sLog.outError("Scripting library has wrong list functions (outdated?).");
            break;
        case SCRIPT_LOAD_ERR_OUTDATED:
            sLog.outError("Scripting library build for old mangosd revision. You need rebuild it.");
            break;
    }
#else /* ENABLE_SD3 */
    sLog.outError("SD3 was not included in compilation, not using it.");
#endif /* ENABLE_SD3 */
    sLog.outString();

    ///- Initialize game time and timers
    sLog.outString("Initialize game time and timers");
    m_gameTime = time(NULL);
    m_startTime = m_gameTime;

    std::tm local;
    local = safe_localtime(time(nullptr));
    char isoDate[128];
    sprintf(isoDate, "%04d-%02d-%02d %02d:%02d:%02d",
            local.tm_year + 1900, local.tm_mon + 1, local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec);

    LoginDatabase.PExecute("INSERT INTO `uptime` (`realmid`, `starttime`, `startstring`, `uptime`) VALUES('%u', " UI64FMTD ", '%s', 0)",
                           realmID, uint64(m_startTime), isoDate);

    m_timers[WUPDATE_AUCTIONS].SetInterval(MINUTE * IN_MILLISECONDS);
    m_timers[WUPDATE_UPTIME].SetInterval(getConfig(CONFIG_UINT32_UPTIME_UPDATE) * MINUTE * IN_MILLISECONDS);
    // Update "uptime" table based on configuration entry in minutes.
    m_timers[WUPDATE_CORPSES].SetInterval(20 * MINUTE * IN_MILLISECONDS);
    m_timers[WUPDATE_DELETECHARS].SetInterval(DAY * IN_MILLISECONDS); // check for chars to delete every day

    // for AhBot
    m_timers[WUPDATE_AHBOT].SetInterval(20 * IN_MILLISECONDS); // every 20 sec

    // for Dungeon Finder
    m_timers[WUPDATE_LFGMGR].SetInterval(30 * IN_MILLISECONDS); // every 30 sec

    // for AutoBroadcast
    sLog.outString("Starting AutoBroadcast System");
    if (m_broadcastEnable)
    {
        LoadBroadcastStrings();
    }
    else
    {
        sLog.outString("AutoBroadcast is disabled");
    }
    sLog.outString();

    if (m_broadcastEnable)
    {
        m_broadcastTimer.SetInterval(getConfig(CONFIG_UINT32_AUTOBROADCAST_INTERVAL) * IN_MILLISECONDS);
    }

    // to set mailtimer to return mails every day between 4 and 5 am
    // mailtimer is increased when updating auctions
    // one second is 1000 -(tested on win system)

    std::tm ltm = safe_localtime(m_gameTime);
    mail_timer = uint32((((ltm.tm_hour + 20) % 24) * HOUR * IN_MILLISECONDS) / m_timers[WUPDATE_AUCTIONS].GetInterval());
    // 1440
    mail_timer_expires = uint32((DAY * IN_MILLISECONDS) / (m_timers[WUPDATE_AUCTIONS].GetInterval()));
    DEBUG_LOG("Mail timer set to: %u, mail return is called every %u minutes", mail_timer, mail_timer_expires);

    ///- Initialize static helper structures
    AIRegistry::Initialize();
    Player::InitVisibleBits();

    ///- Initialize MapManager
    sLog.outString("Starting Map System");
    sMapMgr.Initialize();
    sLog.outString();

    ///- Initialize Battlegrounds
    sLog.outString("Starting BattleGround System");
    sBattleGroundMgr.CreateInitialBattleGrounds();
    sBattleGroundMgr.InitAutomaticArenaPointDistribution();

    ///- Initialize Outdoor PvP
    sLog.outString("Starting Outdoor PvP System");
    sOutdoorPvPMgr.InitOutdoorPvP();

    // Not sure if this can be moved up in the sequence (with static data loading) as it uses MapManager
    sLog.outString("Loading Transports...");
    sMapMgr.LoadTransports();

    // Initialize Warden
    sLog.outString("Loading Warden Checks...");
    sWardenCheckMgr->LoadWardenChecks();
    sLog.outString();

    sLog.outString("Loading Warden Action Overrides...");
    sWardenCheckMgr->LoadWardenOverrides();
    sLog.outString();

    sLog.outString("Deleting expired bans...");
    LoginDatabase.Execute("DELETE FROM `ip_banned` WHERE `unbandate`<=UNIX_TIMESTAMP() AND `unbandate`<>`bandate`");
    sLog.outString();

    sLog.outString("Calculate next daily quest and dungeon reset time...");
    InitDailyQuestResetTime();

    sLog.outString("Calculate next weekly quest reset time...");
    InitWeeklyQuestResetTime();

    sLog.outString("Calculate next monthly quest reset time...");
    SetMonthlyQuestResetTime();

    sLog.outString("Calculate random battleground reset time...");
    InitRandomBGResetTime();

    sLog.outString("Starting Game Event system...");
    uint32 nextGameEvent = sGameEventMgr.Initialize();
    m_timers[WUPDATE_EVENTS].SetInterval(nextGameEvent);    // depend on next event
    sLog.outString();

    sLog.outString("Loading grids for active creatures or transports...");
    uint32 loadContinentsBegin = GameTime::GetGameTimeMS();
    ObjectMgr::LivingWorldStartupStats lwStats = sObjectMgr.LoadActiveEntities(NULL);
    uint32 loadContinentsMs = GetMSTimeDiffToNow(loadContinentsBegin);
    sLog.outString("[LivingWorld] startup summary: maps-forced=%u, total-unique-grids=%u, total-newly-loaded=%u, total-map-transports=%u, LoadContinents=%u ms",
                   lwStats.forcedMaps, lwStats.totalUniqueGrids, lwStats.totalNewlyLoaded, lwStats.totalMapTransports, loadContinentsMs);
    sLog.outString();

    // Delete all characters which have been deleted X days before
    Player::DeleteOldCharacters();

    sLog.outString("Initialize AuctionHouseBot...");
    sAuctionBot.Initialize();
    sLog.outString();

#ifdef ENABLE_ELUNA
    ///- Run eluna scripts.
    // in multithread foreach: run scripts
    if (Eluna* e = GetEluna())
    {
        e->OnConfigLoad(false); // Must be done after Eluna is initialized and scripts have run.
    }
#endif

#ifdef ENABLE_PLAYERBOTS
    sPlayerbotAIConfig.Initialize();
#endif

    showFooter();

    uint32 startupDuration = GetMSTimeDiffToNow(startupBegin);
    sLog.outString("SERVER STARTUP TIME: %i minutes %i seconds", (startupDuration / 60000), ((startupDuration % 60000) / 1000));
    sLog.outString();
}

/**
 * @brief Prints the startup footer and enabled module summary.
 */
void World::showFooter()
{
    std::set<std::string> modules_;

    // ELUNA is either included or disabled
#ifdef ENABLE_ELUNA
    modules_.insert("                 Eluna : Enabled");
#endif

    // SD3 is either included or disabled
#ifdef ENABLE_SD3
    modules_.insert("      ScriptDev3 (SD3) : Enabled");
#endif

    // PLAYERBOTS can be included or excluded but also disabled via mangos.conf
#ifdef ENABLE_PLAYERBOTS
    bool playerBotActive = sConfig.GetBoolDefault("PlayerbotAI.DisableBots", true);
    if (playerBotActive)
    {
        modules_.insert("            PlayerBots : Disabled");
    }
    else
    {
        modules_.insert("            PlayerBots : Enabled");
    }
#endif

    // Remote Access can be activated / deactivated via mangos.conf
    bool raActive = sConfig.GetBoolDefault("Ra.Enable", false);
    if (raActive)
    {
        modules_.insert("    Remote Access (RA) : Enabled");
    }
    else
    {
        modules_.insert("    Remote Access (RA) : Disabled");
    }

    // SOAP can be included or excluded but also disabled via mangos.conf
#ifdef ENABLE_SOAP
    bool soapActive = sConfig.GetBoolDefault("SOAP.Enabled", false);
    if (soapActive)
    {
        modules_.insert("                  SOAP : Enabled");
    }
    else
    {
        modules_.insert("                  SOAP : Disabled");
    }
#endif

    // Warden is always included, set active or disabled via mangos.conf
    bool wardenActive = (sWorld.getConfig(CONFIG_BOOL_WARDEN_WIN_ENABLED) || sWorld.getConfig(CONFIG_BOOL_WARDEN_OSX_ENABLED));
    if (wardenActive)
    {
        modules_.insert("                Warden : Enabled");
    }
    else
    {
        modules_.insert("                Warden : Disabled");
    }

    std::string thisClientVersion (EXPECTED_MANGOSD_CLIENT_VERSION);
    std::string thisClientBuilds = AcceptableClientBuildsListStr();

    std::string sModules;
    for (std::set<std::string>::const_iterator it = modules_.begin(); it != modules_.end(); ++it)
    {
        sModules = sModules + " \n" + *it;
    }

    sLog.outString("\n"
        "_______________________________________________________\n"
        "\n"
        " MaNGOS Server: World Initialization Complete\n"
        "_______________________________________________________\n"
        "\n"
        "        Server Version : %s\n"
        "         Eluna Version : %s\n"
        "           SD3 Version : %s\n"
        "      Database Version : Rel%s.%s.%s\n"
        "\n"
        "    Supporting Clients : %s\n"
        "                Builds : %s\n"
        "\n"
        "         Module Status -\n%s\n"
        "_______________________________________________________\n"
        , GitRevision::GetProductVersionStr(), GitRevision::GetDepElunaFullRevision(), GitRevision::GetDepSD3FullRevision(), GitRevision::GetWorldDBVersion(), GitRevision::GetWorldDBStructure(), GitRevision::GetWorldDBContent(),
            thisClientVersion.c_str(), thisClientBuilds.c_str(), sModules.c_str());
}

/**
 * @brief Detects the active DBC locale and available locale mask.
 */
void World::DetectDBCLang()
{
    uint32 m_lang_confid = sConfig.GetIntDefault("DBC.Locale", 255);

    if (m_lang_confid != 255 && m_lang_confid >= MAX_LOCALE)
    {
        sLog.outError("Incorrect DBC.Locale! Must be >= 0 and < %d (set to 0)", MAX_LOCALE);
        m_lang_confid = LOCALE_enUS;
    }

    ChrRacesEntry const* race = sChrRacesStore.LookupEntry(RACE_HUMAN);
    MANGOS_ASSERT(race);

    std::string availableLocalsStr;

    uint32 default_locale = MAX_LOCALE;
    for (int i = MAX_LOCALE - 1; i >= 0; --i)
    {
        if (strlen(race->Name_lang[i]) > 0)                      // check by race names
        {
            default_locale = i;
            m_availableDbcLocaleMask |= (1 << i);
            availableLocalsStr += localeNames[i];
            availableLocalsStr += " ";
        }
    }

    if (default_locale != m_lang_confid && m_lang_confid < MAX_LOCALE &&
        (m_availableDbcLocaleMask & (1 << m_lang_confid)))
    {
        default_locale = m_lang_confid;
    }

    if (default_locale >= MAX_LOCALE)
    {
        sLog.outError("Unable to determine your DBC Locale! (corrupt DBC?)");
        Log::WaitBeforeContinueIfNeed();
        exit(1);
    }

    m_defaultDbcLocale = LocaleConstant(default_locale);

    sLog.outString("Using %s DBC Locale as default. All available DBC locales: %s", localeNames[m_defaultDbcLocale], availableLocalsStr.empty() ? "<none>" : availableLocalsStr.c_str());
    sLog.outString();
}

/// Update the World !
void World::Update(uint32 diff)
{
    ///- Update the different timers
    for (int i = 0; i < WUPDATE_COUNT; ++i)
    {
        if (m_timers[i].GetCurrent() >= 0)
        {
            m_timers[i].Update(diff);
        }
        else
        {
            m_timers[i].SetCurrent(0);
        }
    }

    if (m_broadcastEnable)
    {
        if (m_broadcastTimer.GetCurrent() >= 0)
        {
            m_broadcastTimer.Update(diff);
        }
        else
        {
            m_broadcastTimer.SetCurrent(0);
        }

        if (m_broadcastTimer.Passed())
        {
            m_broadcastTimer.Reset();
            AutoBroadcast();
        }
    }

    ///- Update the game time and check for shutdown time
    _UpdateGameTime();
    GameTime::UpdateGameTimers();
    sWorldUpdateTime.UpdateWithDiff(diff);

    ///- Flush buffered log files about once per second. The file sinks are
    ///  fully buffered (setvbuf), so this bounds how long a buffered line waits
    ///  to reach disk; error paths still flush immediately at emit time. Safe as
    ///  a function-local static: World::Update runs only on the world thread.
    static uint32 logFlushTimer = 0;
    logFlushTimer += diff;
    if (logFlushTimer >= 1000)
    {
        logFlushTimer = 0;
        sLog.Flush();
    }

    ///-Update mass mailer tasks if any
    sMassMailMgr.Update();

    /// Handle daily quests and dungeon reset time
    if (m_gameTime > m_NextDailyQuestReset)
    {
        ResetDailyQuests();
        sLFGMgr.ResetDailyRecords();
    }

    /// Handle weekly quests reset time
    if (m_gameTime > m_NextWeeklyQuestReset)
    {
        ResetWeeklyQuests();
    }

    /// Handle monthly quests reset time
    if (m_gameTime > m_NextMonthlyQuestReset)
    {
        ResetMonthlyQuests();
    }

    /// Handle random battlegrounds reset time
    if (m_gameTime > m_NextRandomBGReset)
    {
        ResetRandomBG();
    }

    /// <ul><li> Handle auctions when the timer has passed
    if (m_timers[WUPDATE_AUCTIONS].Passed())
    {
        m_timers[WUPDATE_AUCTIONS].Reset();

        ///- Update mails (return old mails with item, or delete them)
        //(tested... works on win)
        if (++mail_timer > mail_timer_expires)
        {
            mail_timer = 0;
            sObjectMgr.ReturnOrDeleteOldMails(true);
        }

        ///- Handle expired auctions
        sAuctionMgr.Update();
    }

    /// <li> Handle AHBot operations
    if (m_timers[WUPDATE_AHBOT].Passed())
    {
        sAuctionBot.Update();
        m_timers[WUPDATE_AHBOT].Reset();
    }

    /// <li> Update Dungeon Finder
    if (m_timers[WUPDATE_LFGMGR].Passed())
    {
        sLFGMgr.Update();
        m_timers[WUPDATE_LFGMGR].Reset();
    }

    /// <li> Handle session updates
    UpdateSessions(diff);

    /// <li> Update uptime table
    if (m_timers[WUPDATE_UPTIME].Passed())
    {
        uint32 tmpDiff = uint32(m_gameTime - m_startTime);
        uint32 maxClientsNum = GetMaxActiveSessionCount();

        m_timers[WUPDATE_UPTIME].Reset();
        LoginDatabase.PExecute("UPDATE `uptime` SET `uptime` = %u, `maxplayers` = %u WHERE `realmid` = %u AND `starttime` = " UI64FMTD, tmpDiff, maxClientsNum, realmID, uint64(m_startTime));
    }

    /// <li> Handle all other objects
    ///- Update objects (maps, transport, creatures,...)
    sMapMgr.Update(diff);
    sBattleGroundMgr.Update(diff);
    sOutdoorPvPMgr.Update(diff);

    ///- Used by Eluna
#ifdef ENABLE_ELUNA
    if (Eluna* e = GetEluna())
    {
        e->UpdateEluna(diff);
        e->OnWorldUpdate(diff);
    }
#endif /* ENABLE_ELUNA */

    ///- Delete all characters which have been deleted X days before
    if (m_timers[WUPDATE_DELETECHARS].Passed())
    {
        m_timers[WUPDATE_DELETECHARS].Reset();
        Player::DeleteOldCharacters();
    }

    // execute callbacks from sql queries that were queued recently
    UpdateResultQueue();

    ///- Erase corpses once every 20 minutes
    if (m_timers[WUPDATE_CORPSES].Passed())
    {
        m_timers[WUPDATE_CORPSES].Reset();

        sObjectAccessor.RemoveOldCorpses();
    }

    ///- Process Game events when necessary
    if (m_timers[WUPDATE_EVENTS].Passed())
    {
        m_timers[WUPDATE_EVENTS].Reset();                   // to give time for Update() to be processed
        uint32 nextGameEvent = sGameEventMgr.Update();
        m_timers[WUPDATE_EVENTS].SetInterval(nextGameEvent);
        m_timers[WUPDATE_EVENTS].Reset();
    }

    /// </ul>
    ///- Move all creatures with "delayed move" and remove and delete all objects with "delayed remove"
    sMapMgr.RemoveAllObjectsInRemoveList();

    // update the instance reset times
    sMapPersistentStateMgr.Update();

    // And last, but not least handle the issued cli commands
    ProcessCliCommands();

    // cleanup unused GridMap objects as well as VMaps
    sTerrainMgr.Update(diff);
}

namespace MaNGOS
{
    class WorldWorldTextBuilder
    {
        public:
            typedef std::vector<WorldPacket*> WorldPacketList;
            explicit WorldWorldTextBuilder(int32 textId, va_list* args = NULL) : i_textId(textId), i_args(args) {}
            void operator()(WorldPacketList& data_list, int32 loc_idx)
            {
                char const* text = sObjectMgr.GetMangosString(i_textId, loc_idx);

                if (i_args)
                {
                    // we need copy va_list before use or original va_list will corrupted
                    va_list ap;
                    va_copy(ap, *i_args);

                    char str [2048];
                    vsnprintf(str, 2048, text, ap);
                    va_end(ap);

                    do_helper(data_list, &str[0]);
                }
                else
                {
                    do_helper(data_list, (char*)text);
                }
            }
        private:
            char* lineFromMessage(char*& pos)
            {
                char* start = strtok(pos, "\n");
                pos = NULL;
                return start;
            }
            void do_helper(WorldPacketList& data_list, char* text)
            {
                char* pos = text;

                while (char* line = lineFromMessage(pos))
                {
                    WorldPacket* data = new WorldPacket();
                    ChatHandler::BuildChatPacket(*data, CHAT_MSG_SYSTEM, line);
                    data_list.push_back(data);
                }
            }

            int32 i_textId;
            va_list* i_args;
    };
}                                                           // namespace MaNGOS

/// Sends a system message to all players
void World::SendWorldText(int32 string_id, ...)
{
    va_list ap;
    va_start(ap, string_id);

    MaNGOS::WorldWorldTextBuilder wt_builder(string_id, &ap);
    MaNGOS::LocalizedPacketListDo<MaNGOS::WorldWorldTextBuilder> wt_do(wt_builder);
    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
    {
        if (WorldSession* session = itr->second)
        {
            Player* player = session->GetPlayer();
            if (player && player->IsInWorld())
            {
                wt_do(player);
            }
        }
    }

    va_end(ap);
}

/// Sends a packet to all players with optional account access level restrictions
void World::SendGlobalMessage(WorldPacket* packet, AccountTypes minSec)
{
    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
    {
        if (WorldSession* session = itr->second)
        {
            if (session->GetSecurity() < minSec)
            {
                continue;
            }
            Player* player = session->GetPlayer();
            if (player && player->IsInWorld())
            {
                session->SendPacket(packet);
            }
        }
    }
}

/// Sends a server message to the specified or all players
void World::SendServerMessage(ServerMessageType type, const char* text /*=""*/, Player* player /*= NULL*/)
{
    WorldPacket data(SMSG_SERVER_MESSAGE, 50);              // guess size
    data << uint32(type);
    data << text;

    if (player)
    {
        player->GetSession()->SendPacket(&data);
    }
    else
    {
        SendGlobalMessage(&data);
    }
}

/// Sends a zone under attack message to all players not in an instance
void World::SendZoneUnderAttackMessage(uint32 zoneId, Team team)
{
    WorldPacket data(SMSG_ZONE_UNDER_ATTACK, 4);
    data << uint32(zoneId);

    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
    {
        if (WorldSession* session = itr->second)
        {
            Player* player = session->GetPlayer();
            if (player && player->IsInWorld() && player->GetTeam() == team && !player->GetMap()->Instanceable())
            {
                itr->second->SendPacket(&data);
            }
        }
    }
}

/// Sends a world defense message to all players not in an instance
void World::SendDefenseMessage(uint32 zoneId, int32 textId)
{
    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
    {
        if (WorldSession* session = itr->second)
        {
            Player* player = session->GetPlayer();
            if (player && player->IsInWorld() && !player->GetMap()->Instanceable())
            {
                char const* message = session->GetMangosString(textId);
                uint32 messageLength = strlen(message) + 1;

                WorldPacket data(SMSG_DEFENSE_MESSAGE, 4 + 4 + messageLength);
                data << uint32(zoneId);
                data << uint32(messageLength);
                data << message;
                session->SendPacket(&data);
            }
        }
    }
}

/// Kick (and save) all players
void World::KickAll()
{
    m_QueuedSessions.clear();                               // prevent send queue update packet and login queued sessions

    // session not removed at kick and will removed in next update tick
    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
    {
        itr->second->KickPlayer();
    }
}

/// Kick (and save) all players with security level less `sec`
void World::KickAllLess(AccountTypes sec)
{
    // session not removed at kick and will removed in next update tick
    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
    {
        if (WorldSession* session = itr->second)
            if (session->GetSecurity() < sec)
            {
                session->KickPlayer();
            }
    }
}

/// Ban an account or ban an IP address, duration_secs if it is positive used, otherwise permban
BanReturn World::BanAccount(BanMode mode, std::string nameOrIP, uint32 duration_secs, std::string reason, std::string author)
{
    LoginDatabase.escape_string(nameOrIP);
    LoginDatabase.escape_string(reason);
    std::string safe_author = author;
    LoginDatabase.escape_string(safe_author);

    QueryResult* resultAccounts = NULL;                     // used for kicking

    ///- Update the database with ban information
    switch (mode)
    {
        case BAN_IP:
            // No SQL injection as strings are escaped
            resultAccounts = LoginDatabase.PQuery("SELECT `id` FROM `account` WHERE `last_ip` = '%s'", nameOrIP.c_str());
            LoginDatabase.PExecute("INSERT INTO `ip_banned` VALUES ('%s',UNIX_TIMESTAMP(),UNIX_TIMESTAMP()+%u,'%s','%s')", nameOrIP.c_str(), duration_secs, safe_author.c_str(), reason.c_str());
            break;
        case BAN_ACCOUNT:
            // No SQL injection as string is escaped
            resultAccounts = LoginDatabase.PQuery("SELECT `id` FROM `account` WHERE `username` = '%s'", nameOrIP.c_str());
            break;
        case BAN_CHARACTER:
            // No SQL injection as string is escaped
            resultAccounts = CharacterDatabase.PQuery("SELECT `account` FROM `characters` WHERE `name` = '%s'", nameOrIP.c_str());
            break;
        default:
            return BAN_SYNTAX_ERROR;
    }

    if (!resultAccounts)
    {
        if (mode == BAN_IP)
        {
            return BAN_SUCCESS;
        }                             // ip correctly banned but nobody affected (yet)
        else
        {
            return BAN_NOTFOUND;
        }                            // Nobody to ban
    }

    ///- Disconnect all affected players (for IP it can be several)
    do
    {
        Field* fieldsAccount = resultAccounts->Fetch();
        uint32 account = fieldsAccount->GetUInt32();

        if (mode != BAN_IP)
        {
            // No SQL injection as strings are escaped
            LoginDatabase.PExecute("INSERT INTO `account_banned` VALUES ('%u', UNIX_TIMESTAMP(), UNIX_TIMESTAMP()+%u, '%s', '%s', '1')",
                                   account, duration_secs, safe_author.c_str(), reason.c_str());
        }

        if (WorldSession* sess = FindSession(account))
        {
            if (std::string(sess->GetPlayerName()) != author)
            {
                sess->KickPlayer();
            }
        }
    }
    while (resultAccounts->NextRow());

    delete resultAccounts;
    return BAN_SUCCESS;
}

/// Remove a ban from an account or IP address
bool World::RemoveBanAccount(BanMode mode, std::string nameOrIP)
{
    if (mode == BAN_IP)
    {
        LoginDatabase.escape_string(nameOrIP);
        LoginDatabase.PExecute("DELETE FROM `ip_banned` WHERE `ip` = '%s'", nameOrIP.c_str());
    }
    else
    {
        uint32 account = 0;
        if (mode == BAN_ACCOUNT)
        {
            account = sAccountMgr.GetId(nameOrIP);
        }
        else if (mode == BAN_CHARACTER)
        {
            account = sObjectMgr.GetPlayerAccountIdByPlayerName(nameOrIP);
        }

        if (!account)
        {
            return false;
        }

        // NO SQL injection as account is uint32
        LoginDatabase.PExecute("UPDATE `account_banned` SET `active` = '0' WHERE `id` = '%u'", account);
    }
    return true;
}

/// Update the game time
void World::_UpdateGameTime()
{
    ///- update the time
    time_t thisTime = time(NULL);
    uint32 elapsed = uint32(thisTime - m_gameTime);
    m_gameTime = thisTime;

    ///- if there is a shutdown timer
    if (!m_stopEvent && m_ShutdownTimer > 0 && elapsed > 0)
    {
        ///- ... and it is overdue, stop the world (set m_stopEvent)
        if (m_ShutdownTimer <= elapsed)
        {
            if (!(m_ShutdownMask & SHUTDOWN_MASK_IDLE) || GetActiveAndQueuedSessionCount() == 0)
            {
                m_stopEvent = true;
            }                         // exist code already set
            else
            {
                m_ShutdownTimer = 1;
            }                        // minimum timer value to wait idle state
        }
        ///- ... else decrease it and if necessary display a shutdown countdown to the users
        else
        {
            m_ShutdownTimer -= elapsed;

            ShutdownMsg();
        }
    }
}

/// Shutdown the server
void World::ShutdownServ(uint32 time, uint32 options, uint8 exitcode)
{
    // ignore if server shutdown at next tick
    if (m_stopEvent)
    {
        return;
    }

    m_ShutdownMask = options;
    m_ExitCode = exitcode;

    ///- If the shutdown time is 0, set m_stopEvent (except if shutdown is 'idle' with remaining sessions)
    if (time == 0)
    {
        if (!(options & SHUTDOWN_MASK_IDLE) || GetActiveAndQueuedSessionCount() == 0)
        {
                sObjectAccessor.SaveAllPlayers();        // save all players.
                m_stopEvent = true;                                // exist code already set
        }
        else
        {
            m_ShutdownTimer = 1;
        }                            // So that the session count is re-evaluated at next world tick
    }
    ///- Else set the shutdown timer and warn users
    else
    {
        m_ShutdownTimer = time;
        ShutdownMsg(true);
    }

    ///- Used by Eluna
#ifdef ENABLE_ELUNA
    if (Eluna* e = GetEluna())
    {
        e->OnShutdownInitiate(ShutdownExitCode(exitcode), ShutdownMask(options));
    }
#endif /* ENABLE_ELUNA */
}

/// Display a shutdown message to the user(s)
void World::ShutdownMsg(bool show /*= false*/, Player* player /*= NULL*/)
{
    // not show messages for idle shutdown mode
    if (m_ShutdownMask & SHUTDOWN_MASK_IDLE)
    {
        return;
    }

    ///- Display a message every 12 hours, 1 hour, 5 minutes, 1 minute and 15 seconds
    if (show ||
        (m_ShutdownTimer < 5 * MINUTE && (m_ShutdownTimer % 15) == 0) ||            // < 5 min; every 15 sec
        (m_ShutdownTimer < 15 * MINUTE && (m_ShutdownTimer % MINUTE) == 0) ||       // < 15 min; every 1 min
        (m_ShutdownTimer < 30 * MINUTE && (m_ShutdownTimer % (5 * MINUTE)) == 0) || // < 30 min; every 5 min
        (m_ShutdownTimer < 12 * HOUR && (m_ShutdownTimer % HOUR) == 0) ||           // < 12 h; every 1 h
        (m_ShutdownTimer >= 12 * HOUR && (m_ShutdownTimer % (12 * HOUR)) == 0))     // >= 12 h; every 12 h
    {
        std::string str = secsToTimeString(m_ShutdownTimer, TimeFormat::Numeric);

        ServerMessageType msgid = (m_ShutdownMask & SHUTDOWN_MASK_RESTART) ? SERVER_MSG_RESTART_TIME : SERVER_MSG_SHUTDOWN_TIME;

        SendServerMessage(msgid, str.c_str(), player);
        DEBUG_LOG("Server is %s in %s", (m_ShutdownMask & SHUTDOWN_MASK_RESTART) ? "restart" : "shutting down", str.c_str());
    }
}

/// Cancel a planned server shutdown
void World::ShutdownCancel()
{
    // nothing cancel or too later
    if (!m_ShutdownTimer || m_stopEvent)
    {
        return;
    }

    ServerMessageType msgid = (m_ShutdownMask & SHUTDOWN_MASK_RESTART) ? SERVER_MSG_RESTART_CANCELLED : SERVER_MSG_SHUTDOWN_CANCELLED;

    m_ShutdownMask = 0;
    m_ShutdownTimer = 0;
    m_ExitCode = SHUTDOWN_EXIT_CODE;                       // to default value
    SendServerMessage(msgid);

    DEBUG_LOG("Server %s cancelled.", (m_ShutdownMask & SHUTDOWN_MASK_RESTART) ? "restart" : "shutdown");

    ///- Used by Eluna
#ifdef ENABLE_ELUNA
    if (Eluna* e = GetEluna())
    {
        e->OnShutdownCancel();
    }
#endif /* ENABLE_ELUNA */
}

/**
 * @brief Updates all active sessions and integrates newly queued ones.
 *
 * @param diff The elapsed world update time in milliseconds.
 */
void World::UpdateSessions(uint32 diff)
{
    ///- Add new sessions
    WorldSession* sess;
    while (addSessQueue.next(sess))
    {
        AddSession_(sess);
    }

    ///- Then send an update signal to remaining ones
    for (SessionMap::iterator itr = m_sessions.begin(), next; itr != m_sessions.end(); itr = next)
    {
        next = itr;
        ++next;
        ///- and remove not active sessions from the list
        WorldSession* pSession = itr->second;
        WorldSessionFilter updater(pSession);

        if (!pSession->Update(updater))
        {
            RemoveQueuedSession(pSession);
            m_sessions.erase(itr);
            delete pSession;
        }
    }
}

// This handles the issued and queued CLI/RA commands
void World::ProcessCliCommands()
{
    CliCommandHolder* command;
    while (cliCmdQueue.next(command))
    {
        DEBUG_LOG("CLI command under processing...");
        CliCommandHolder::Print* zprint = command->m_print;
        void* callbackArg = command->m_callbackArg;
        CliHandler handler(command->m_cliAccountId, command->m_cliAccessLevel, callbackArg, zprint);
        handler.ParseCommands(command->m_command);

        if (command->m_commandFinished)
        {
            command->m_commandFinished(callbackArg, !handler.HasSentErrorMessage());
        }

        delete command;
    }
}

/**
 * @brief Initializes the asynchronous result queue state.
 */
void World::InitResultQueue()
{
}

/**
 * @brief Processes queued asynchronous database results.
 */
void World::UpdateResultQueue()
{
    // process async result queues
    CharacterDatabase.ProcessResultQueue();
    WorldDatabase.ProcessResultQueue();
    LoginDatabase.ProcessResultQueue();
}

/**
 * @brief Requests an asynchronous character count refresh for an account.
 *
 * @param accountId The account id to refresh.
 */
void World::UpdateRealmCharCount(uint32 accountId)
{
    CharacterDatabase.AsyncPQuery(this, &World::_UpdateRealmCharCount, accountId,
                                  "SELECT COUNT(`guid`) FROM `characters` WHERE `account` = '%u'", accountId);
}

/**
 * @brief Stores the updated realm character count for an account.
 *
 * @param resultCharCount The asynchronous query result.
 * @param accountId The account id being updated.
 */
void World::_UpdateRealmCharCount(QueryResult* resultCharCount, uint32 accountId)
{
    if (resultCharCount)
    {
        Field* fields = resultCharCount->Fetch();
        uint32 charCount = fields[0].GetUInt32();
        delete resultCharCount;

        LoginDatabase.BeginTransaction();
        LoginDatabase.PExecute("DELETE FROM `realmcharacters` WHERE `acctid`= '%u' AND `realmid` = '%u'", accountId, realmID);
        LoginDatabase.PExecute("INSERT INTO `realmcharacters` (`numchars`, `acctid`, `realmid`) VALUES (%u, %u, %u)", charCount, accountId, realmID);
        LoginDatabase.CommitTransaction();
    }
}

void World::InitWeeklyQuestResetTime()
{
    QueryResult* result = CharacterDatabase.Query("SELECT `NextWeeklyQuestResetTime` FROM `saved_variables`");
    if (!result)
    {
        m_NextWeeklyQuestReset = time_t(time(NULL));        // game time not yet init
    }
    else
    {
        m_NextWeeklyQuestReset = time_t((*result)[0].GetUInt64());
    }

    // generate time by config
    time_t curTime = time(NULL);
    tm localTm = safe_localtime(curTime);

    int week_day_offset = localTm.tm_wday - int(getConfig(CONFIG_UINT32_QUEST_WEEKLY_RESET_WEEK_DAY));

    // current week reset time
    localTm.tm_hour = getConfig(CONFIG_UINT32_QUEST_WEEKLY_RESET_HOUR);
    localTm.tm_min  = 0;
    localTm.tm_sec  = 0;
    time_t nextWeekResetTime = mktime(&localTm);
    nextWeekResetTime -= week_day_offset * DAY;             // move time to proper day

    // next reset time before current moment
    if (curTime >= nextWeekResetTime)
    {
        nextWeekResetTime += WEEK;
    }

    // normalize reset time
    m_NextWeeklyQuestReset = m_NextWeeklyQuestReset < curTime ? nextWeekResetTime - WEEK : nextWeekResetTime;

    if (!result)
    {
        CharacterDatabase.PExecute("INSERT INTO `saved_variables` (`NextWeeklyQuestResetTime`) VALUES ('" UI64FMTD "')", uint64(m_NextWeeklyQuestReset));
    }
    else
    {
        delete result;
    }
}

void World::InitDailyQuestResetTime()
{
    QueryResult* result = CharacterDatabase.Query("SELECT `NextDailyQuestResetTime` FROM `saved_variables`");
    if (!result)
    {
        m_NextDailyQuestReset = time_t(time(NULL));         // game time not yet init
    }
    else
    {
        m_NextDailyQuestReset = time_t((*result)[0].GetUInt64());
    }

    // generate time by config
    time_t curTime = time(NULL);
    tm localTm = safe_localtime(curTime);

    localTm.tm_hour = getConfig(CONFIG_UINT32_QUEST_DAILY_RESET_HOUR);
    localTm.tm_min  = 0;
    localTm.tm_sec  = 0;

    // current day reset time
    time_t nextDayResetTime = mktime(&localTm);

    // next reset time before current moment
    if (curTime >= nextDayResetTime)
    {
        nextDayResetTime += DAY;
    }

    // normalize reset time
    m_NextDailyQuestReset = m_NextDailyQuestReset < curTime ? nextDayResetTime - DAY : nextDayResetTime;

    if (!result)
    {
        CharacterDatabase.PExecute("INSERT INTO `saved_variables` (`NextDailyQuestResetTime`) VALUES ('" UI64FMTD "')", uint64(m_NextDailyQuestReset));
    }
    else
    {
        delete result;
    }
}

void World::SetMonthlyQuestResetTime(bool initialize)
{
    if (initialize)
    {
        QueryResult* result = CharacterDatabase.Query("SELECT `NextMonthlyQuestResetTime` FROM `saved_variables`");

        if (!result)
        {
            m_NextMonthlyQuestReset = time_t(time(NULL));
        }
        else
        {
            m_NextMonthlyQuestReset = time_t((*result)[0].GetUInt64());
        }

        delete result;
    }

    // generate time
    time_t currentTime = time(NULL);
    tm localTm = safe_localtime(currentTime);

    int month = localTm.tm_mon;
    int year = localTm.tm_year;

    ++month;

    // month 11 is december, next is january (0)
    if (month > 11)
    {
        month = 0;
        year += 1;
    }

    // reset time for next month
    localTm.tm_year = year;
    localTm.tm_mon = month;
    localTm.tm_mday = 1;                                    // don't know if we really need config option for day/hour
    localTm.tm_hour = 0;
    localTm.tm_min  = 0;
    localTm.tm_sec  = 0;

    time_t nextMonthResetTime = mktime(&localTm);

    m_NextMonthlyQuestReset = (initialize && m_NextMonthlyQuestReset < nextMonthResetTime) ? m_NextMonthlyQuestReset : nextMonthResetTime;

    // Row must exist for this to work. Currently row is added by InitDailyQuestResetTime(), called before this function
    CharacterDatabase.PExecute("UPDATE `saved_variables` SET `NextMonthlyQuestResetTime` = '" UI64FMTD "'", uint64(m_NextMonthlyQuestReset));
}

void World::InitRandomBGResetTime()
{
    QueryResult * result = CharacterDatabase.Query("SELECT `NextRandomBGResetTime` FROM `saved_variables`");
    if (!result)
    {
        m_NextRandomBGReset = time_t(time(NULL));         // game time not yet init
    }
    else
    {
        m_NextRandomBGReset = time_t((*result)[0].GetUInt64());
    }

    // generate time by config
    time_t curTime = time(NULL);
    tm localTm = safe_localtime(curTime);

    localTm.tm_hour = getConfig(CONFIG_UINT32_RANDOM_BG_RESET_HOUR);
    localTm.tm_min  = 0;
    localTm.tm_sec  = 0;

    // current day reset time
    time_t nextDayResetTime = mktime(&localTm);

    // next reset time before current moment
    if (curTime >= nextDayResetTime)
    {
        nextDayResetTime += DAY;
    }

    // normalize reset time
    m_NextRandomBGReset = m_NextRandomBGReset < curTime ? nextDayResetTime - DAY : nextDayResetTime;
    if (!result)
    {
        CharacterDatabase.PExecute("INSERT INTO `saved_variables` (`NextRandomBGResetTime`) VALUES ('" UI64FMTD "')", uint64(m_NextRandomBGReset));
    }
    else
    {
        delete result;
    }
}

void World::ResetDailyQuests()
{
    DETAIL_LOG("Daily quests reset for all characters.");
    CharacterDatabase.Execute("DELETE FROM `character_queststatus_daily`");
    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
    {
        if (itr->second->GetPlayer())
        {
            itr->second->GetPlayer()->ResetDailyQuestStatus();
        }
    }

    m_NextDailyQuestReset = time_t(m_NextDailyQuestReset + DAY);
    CharacterDatabase.PExecute("UPDATE `saved_variables` SET `NextDailyQuestResetTime` = '" UI64FMTD "'", uint64(m_NextDailyQuestReset));
}

void World::ResetRandomBG()
{
    sLog.outDetail("Random BG status reset for all characters.");
    CharacterDatabase.Execute("DELETE FROM `character_battleground_random`");
    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
    {
        if (itr->second->GetPlayer())
        {
            itr->second->GetPlayer()->SetRandomWinner(false);
        }
    }

    m_NextRandomBGReset = time_t(m_NextRandomBGReset + DAY);
    CharacterDatabase.PExecute("UPDATE `saved_variables` SET `NextRandomBGResetTime` = '" UI64FMTD "'", uint64(m_NextRandomBGReset));
}

void World::ResetWeeklyQuests()
{
    DETAIL_LOG("Weekly quests reset for all characters.");
    CharacterDatabase.Execute("DELETE FROM `character_queststatus_weekly`");
    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
    {
        if (itr->second->GetPlayer())
        {
            itr->second->GetPlayer()->ResetWeeklyQuestStatus();
        }
    }

    m_NextWeeklyQuestReset = time_t(m_NextWeeklyQuestReset + WEEK);
    CharacterDatabase.PExecute("UPDATE `saved_variables` SET `NextWeeklyQuestResetTime` = '" UI64FMTD "'", uint64(m_NextWeeklyQuestReset));
}

void World::ResetMonthlyQuests()
{
    DETAIL_LOG("Monthly quests reset for all characters.");
    CharacterDatabase.Execute("TRUNCATE character_queststatus_monthly");

    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
    {
        if (itr->second->GetPlayer())
        {
            itr->second->GetPlayer()->ResetMonthlyQuestStatus();
        }
    }

    SetMonthlyQuestResetTime(false);
}

/**
 * @brief Sets the player access limit and updates the realm list if needed.
 *
 * @param limit The new player limit or security gate value.
 * @param needUpdate Whether the database row should be considered for update.
 */
void World::SetPlayerLimit(int32 limit, bool needUpdate)
{
    if (limit < -SEC_ADMINISTRATOR)
    {
        limit = -SEC_ADMINISTRATOR;
    }

    // lock update need
    bool db_update_need = needUpdate || (limit < 0) != (m_playerLimit < 0) || (limit < 0 && m_playerLimit < 0 && limit != m_playerLimit);

    m_playerLimit = limit;

    if (db_update_need)
        LoginDatabase.PExecute("UPDATE `realmlist` SET `allowedSecurityLevel` = '%u' WHERE `id` = '%u'",
                               uint32(GetPlayerSecurityLimit()), realmID);
}

/**
 * @brief Updates the recorded peak active and queued session counters.
 */
void World::UpdateMaxSessionCounters()
{
    m_maxActiveSessionCount = std::max(m_maxActiveSessionCount, uint32(m_sessions.size() - m_QueuedSessions.size()));
    m_maxQueuedSessionCount = std::max(m_maxQueuedSessionCount, uint32(m_QueuedSessions.size()));
}

















/**
 * @brief Invalidates cached player data for all connected clients.
 *
 * @param guid The player guid to invalidate.
 */
void World::InvalidatePlayerDataToAllClient(ObjectGuid guid)
{
    WorldPacket data(SMSG_INVALIDATE_PLAYER, 8);
    data << guid;
    SendGlobalMessage(&data);
}

/**
 * @brief Loads automatic broadcast messages and their weights from the database.
 */
void World::LoadBroadcastStrings()
{
    if (!m_broadcastEnable)
    {
        return;
    }

    std::string queryStr = "SELECT `autobroadcast`.`id`, `autobroadcast`.`content`,`autobroadcast`.`ratio` FROM `autobroadcast`";

    QueryResult* result = WorldDatabase.Query(queryStr.c_str());

    if (!result)
    {
        m_broadcastEnable = false;
        sLog.outErrorDb("DB table `autobroadcast` is empty.");
        sLog.outString();
        return;
    }

    m_broadcastList.clear();

    BarGoLink bar(result->GetRowCount());
    m_broadcastWeight = 0;

    do
    {
        Field* fields = result->Fetch();
        bar.step();

        uint32 ratio = fields[2].GetUInt32();
        if (ratio == 0)
        {
            continue;
        }

        m_broadcastWeight += ratio;

        BroadcastString bs;
        bs.text = fields[1].GetString();
        bs.freq = m_broadcastWeight;
        m_broadcastList.push_back(bs);
    } while (result->NextRow());

    delete result;
    if (m_broadcastWeight == 0)
    {
        sLog.outString(">> Loaded 0 broadcast strings.");
        m_broadcastEnable = false;
    }
    else
    {
        sLog.outString(">> Loaded %zu broadcast strings.", m_broadcastList.size());
    }
}

/**
 * @brief Sends a weighted random automatic broadcast message.
 */
void World::AutoBroadcast()
{
    if (m_broadcastList.size() == 1)
    {
        SendWorldText(LANG_AUTOBROADCAST, m_broadcastList[0].text.c_str());
    }
    else
    {
        uint32 rn = urand(1, m_broadcastWeight);
        std::vector<BroadcastString>::const_iterator it;
        for (it = m_broadcastList.begin(); it != m_broadcastList.end(); ++it)
        {
            if (rn <= it->freq)
            {
                break;
            }
        }
        SendWorldText(LANG_AUTOBROADCAST, it->text.c_str());
    }
}

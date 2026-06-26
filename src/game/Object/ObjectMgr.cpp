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

#include "ObjectMgr.h"
#include "LivingWorldAnchorPolicy.h"
#include "MotionGenerators/MotionMaster.h"  // WAYPOINT_MOTION_TYPE
#include "Database/DatabaseEnv.h"
#include "Policies/Singleton.h"

#include "SQLStorages.h"
#include "Log.h"
#include "MapManager.h"
#include "ObjectGuid.h"
#include "ScriptMgr.h"
#include "SpellMgr.h"
#include "UpdateMask.h"
#include "World.h"
#include "Group.h"
#include "ArenaTeam.h"
#include "Transports.h"
#include "ProgressBar.h"
#include "Language.h"
#include "PoolManager.h"
#include "GameEventMgr.h"
#include "Spell.h"
#include "Chat.h"
#include "AccountMgr.h"
#include "MapPersistentStateMgr.h"
#include "SpellAuras.h"
#include "Util.h"
#include "WaypointManager.h"
#include "GossipDef.h"
#include "Mail.h"
#include "InstanceData.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "DisableMgr.h"

#include <limits>
#include <set>

INSTANTIATE_SINGLETON_1(ObjectMgr);

// Temporary startup accumulator for LivingWorld observability (written during LoadActiveEntities(NULL) only)
static ObjectMgr::LivingWorldStartupStats s_livingWorldStats;
static bool s_livingWorldStartupPass = false;

/**
 * @brief Normalizes a player name to the server's canonical casing.
 *
 * @param name The player name to normalize.
 * @return true if the name was normalized successfully; otherwise false.
 */
bool normalizePlayerName(std::string& name)
{
    if (name.empty())
    {
        return false;
    }

    wchar_t wstr_buf[MAX_INTERNAL_PLAYER_NAME + 1];
    size_t wstr_len = MAX_INTERNAL_PLAYER_NAME;

    if (!Utf8toWStr(name, &wstr_buf[0], wstr_len))
    {
        return false;
    }

    wstr_buf[0] = wcharToUpper(wstr_buf[0]);
    for (size_t i = 1; i < wstr_len; ++i)
    {
        wstr_buf[i] = wcharToLower(wstr_buf[i]);
    }

    if (!WStrToUtf8(wstr_buf, wstr_len, name))
    {
        return false;
    }

    return true;
}

LanguageDesc lang_description[LANGUAGES_COUNT] =
{
    { LANG_ADDON,           0, 0                       },
    { LANG_UNIVERSAL,       0, 0                       },
    { LANG_ORCISH,        669, SKILL_LANG_ORCISH       },
    { LANG_DARNASSIAN,    671, SKILL_LANG_DARNASSIAN   },
    { LANG_TAURAHE,       670, SKILL_LANG_TAURAHE      },
    { LANG_DWARVISH,      672, SKILL_LANG_DWARVEN      },
    { LANG_COMMON,        668, SKILL_LANG_COMMON       },
    { LANG_DEMONIC,       815, SKILL_LANG_DEMON_TONGUE },
    { LANG_TITAN,         816, SKILL_LANG_TITAN        },
    { LANG_THALASSIAN,    813, SKILL_LANG_THALASSIAN   },
    { LANG_DRACONIC,      814, SKILL_LANG_DRACONIC     },
    { LANG_KALIMAG,       817, SKILL_LANG_OLD_TONGUE   },
    { LANG_GNOMISH,      7340, SKILL_LANG_GNOMISH      },
    { LANG_TROLL,        7341, SKILL_LANG_TROLL        },
    { LANG_GUTTERSPEAK, 17737, SKILL_LANG_GUTTERSPEAK  },
    { LANG_DRAENEI,     29932, SKILL_LANG_DRAENEI      },
    { LANG_ZOMBIE,          0, 0                       },
    { LANG_GNOMISH_BINARY,  0, 0                       },
    { LANG_GOBLIN_BINARY,   0, 0                       }
};

/**
 * @brief Looks up language metadata by language id.
 *
 * @param lang The language id.
 * @return LanguageDesc const* The matching language descriptor, or null if not found.
 */
LanguageDesc const* GetLanguageDescByID(uint32 lang)
{
    for (int i = 0; i < LANGUAGES_COUNT; ++i)
    {
        if (uint32(lang_description[i].lang_id) == lang)
        {
            return &lang_description[i];
        }
    }

    return NULL;
}

/**
 * @brief Checks whether a player satisfies spell-click interaction requirements.
 *
 * @param player The player attempting the interaction.
 * @param clickedCreature The clicked creature.
 * @return true if the interaction requirements are met; otherwise, false.
 */
bool SpellClickInfo::IsFitToRequirements(Player const* player, Creature const* clickedCreature) const
{
    if (conditionId)
    {
        return sObjectMgr.IsPlayerMeetToCondition(conditionId, player, player->GetMap(), clickedCreature, CONDITION_FROM_SPELLCLICK);
    }

    if (questStart)
    {
        // not in expected required quest state
        if (!player || ((!questStartCanActive || !player->IsActiveQuest(questStart)) && !player->GetQuestRewardStatus(questStart)))
        {
            return false;
        }
    }

    if (questEnd)
    {
        // not in expected forbidden quest state
        if (!player || player->GetQuestRewardStatus(questEnd))
        {
            return false;
        }
    }

    return true;
}

template<typename T>
/**
 * @brief Generates the next identifier from the typed guid generator.
 *
 * @return T The next generated identifier value.
 */
T IdGenerator<T>::Generate()
{
    if (m_nextGuid >= std::numeric_limits<T>::max() - 1)
    {
        sLog.outError("%s guid overflow!! Can't continue, shutting down server. ", m_name);
        World::StopNow(ERROR_EXIT_CODE);
    }
    return m_nextGuid++;
}

template uint32 IdGenerator<uint32>::Generate();
template uint64 IdGenerator<uint64>::Generate();

/**
 * @brief Initializes the global object manager.
 */
ObjectMgr::ObjectMgr() :
    m_ArenaTeamIds("Arena team ids"),
    m_AuctionIds("Auction ids"),
    m_EquipmentSetIds("Equipment set ids"),
    m_GuildIds("Guild ids"),
    m_MailIds("Mail ids"),
    m_PetNumbers("Pet numbers"),
    m_FirstTemporaryCreatureGuid(1),
    m_FirstTemporaryGameObjectGuid(1),
    DBCLocaleIndex(LOCALE_enUS)
{
}

/**
 * @brief Releases dynamically allocated object manager resources.
 */
ObjectMgr::~ObjectMgr()
{
    for (QuestMap::iterator i = mQuestTemplates.begin(); i != mQuestTemplates.end(); ++i)
    {
        delete i->second;
    }

    for (PetLevelInfoMap::iterator i = petInfo.begin(); i != petInfo.end(); ++i)
    {
        delete[] i->second;
    }

    // free only if loaded
    for (int class_ = 0; class_ < MAX_CLASSES; ++class_)
    {
        delete[] playerClassInfo[class_].levelInfo;
    }

    for (int race = 0; race < MAX_RACES; ++race)
    {
        for (int class_ = 0; class_ < MAX_CLASSES; ++class_)
        {
            delete[] playerInfo[race][class_].levelInfo;
        }
    }

    // free objects
    for (GroupMap::iterator itr = mGroupMap.begin(); itr != mGroupMap.end(); ++itr)
    {
        delete itr->second;
    }

    for (ArenaTeamMap::iterator itr = mArenaTeamMap.begin(); itr != mArenaTeamMap.end(); ++itr)
    {
        delete itr->second;
    }

    for (CacheVendorItemMap::iterator itr = m_mCacheVendorTemplateItemMap.begin(); itr != m_mCacheVendorTemplateItemMap.end(); ++itr)
    {
        itr->second.Clear();
    }

    for (CacheVendorItemMap::iterator itr = m_mCacheVendorItemMap.begin(); itr != m_mCacheVendorItemMap.end(); ++itr)
    {
        itr->second.Clear();
    }

    for (CacheTrainerSpellMap::iterator itr = m_mCacheTrainerSpellMap.begin(); itr != m_mCacheTrainerSpellMap.end(); ++itr)
    {
        itr->second.Clear();
    }

    mDungeonFinderRewardsMap.clear();
    mDungeonFinderItemsMap.clear();
}

/**
 * @brief Finds a loaded group by its identifier.
 *
 * @param id The group id.
 * @return The matching group, or null if not found.
 */
Group* ObjectMgr::GetGroupById(uint32 id) const
{
    GroupMap::const_iterator itr = mGroupMap.find(id);
    if (itr != mGroupMap.end())
    {
        return itr->second;
    }

    return NULL;
}

ArenaTeam* ObjectMgr::GetArenaTeamById(uint32 arenateamid) const
{
    ArenaTeamMap::const_iterator itr = mArenaTeamMap.find(arenateamid);
    if (itr != mArenaTeamMap.end())
    {
        return itr->second;
    }

    return NULL;
}

ArenaTeam* ObjectMgr::GetArenaTeamByName(const std::string& arenateamname) const
{
    for (ArenaTeamMap::const_iterator itr = mArenaTeamMap.begin(); itr != mArenaTeamMap.end(); ++itr)
    {
        if (itr->second->GetName() == arenateamname)
        {
            return itr->second;
        }
    }

    return NULL;
}

ArenaTeam* ObjectMgr::GetArenaTeamByCaptain(ObjectGuid guid) const
{
    for (ArenaTeamMap::const_iterator itr = mArenaTeamMap.begin(); itr != mArenaTeamMap.end(); ++itr)
    {
        if (itr->second->GetCaptainGuid() == guid)
        {
            return itr->second;
        }
    }

    return NULL;
}

/**
 * @brief Stores a localized string in a locale-indexed vector.
 *
 * @param s The localized string value.
 * @param locale The locale index.
 * @param data The destination locale vector.
 */
void ObjectMgr::AddLocaleString(std::string const& s, LocaleConstant locale, StringVector& data)
{
    if (!s.empty())
    {
        if (data.size() <= size_t(locale))
        {
            data.resize(locale + 1);
        }
        data[locale] = s;
    }
}


// name must be checked to correctness (if received) before call this function
ObjectGuid ObjectMgr::GetPlayerGuidByName(std::string name) const
{
    ObjectGuid guid;

    CharacterDatabase.escape_string(name);

    // Player name safe to sending to DB (checked at login) and this function using
    QueryResult* result = CharacterDatabase.PQuery("SELECT `guid` FROM `characters` WHERE `name` = '%s'", name.c_str());
    if (result)
    {
        guid = ObjectGuid(HIGHGUID_PLAYER, (*result)[0].GetUInt32());

        delete result;
    }

    return guid;
}

/**
 * @brief Resolves a player name from a player GUID.
 *
 * @param guid The player GUID.
 * @param name Receives the resolved player name.
 * @return true if the player name was found; otherwise, false.
 */
bool ObjectMgr::GetPlayerNameByGUID(ObjectGuid guid, std::string& name) const
{
    // prevent DB access for online player
    if (Player* player = GetPlayer(guid))
    {
        name = player->GetName();
        return true;
    }

    uint32 lowguid = guid.GetCounter();

    QueryResult* result = CharacterDatabase.PQuery("SELECT `name` FROM `characters` WHERE `guid` = '%u'", lowguid);

    if (result)
    {
        name = (*result)[0].GetCppString();
        delete result;
        return true;
    }

    return false;
}

/**
 * @brief Resolves a player's team from a player GUID.
 *
 * @param guid The player GUID.
 * @return The player's team, or TEAM_NONE if unavailable.
 */
Team ObjectMgr::GetPlayerTeamByGUID(ObjectGuid guid) const
{
    // prevent DB access for online player
    if (Player* player = GetPlayer(guid))
    {
        return Player::TeamForRace(player->getRace());
    }

    uint32 lowguid = guid.GetCounter();

    QueryResult* result = CharacterDatabase.PQuery("SELECT `race` FROM `characters` WHERE `guid` = '%u'", lowguid);

    if (result)
    {
        uint8 race = (*result)[0].GetUInt8();
        delete result;
        return Player::TeamForRace(race);
    }

    return TEAM_NONE;
}

/**
 * @brief Resolves an account id from a player GUID.
 *
 * @param guid The player GUID.
 * @return The account id, or 0 if unavailable.
 */
uint32 ObjectMgr::GetPlayerAccountIdByGUID(ObjectGuid guid) const
{
    if (!guid.IsPlayer())
    {
        return 0;
    }

    // prevent DB access for online player
    if (Player* player = GetPlayer(guid))
    {
        return player->GetSession()->GetAccountId();
    }

    uint32 lowguid = guid.GetCounter();

    QueryResult* result = CharacterDatabase.PQuery("SELECT `account` FROM `characters` WHERE `guid` = '%u'", lowguid);
    if (result)
    {
        uint32 acc = (*result)[0].GetUInt32();
        delete result;
        return acc;
    }

    return 0;
}

/**
 * @brief Resolves an account id from a player name.
 *
 * @param name The player name.
 * @return The account id, or 0 if unavailable.
 */
uint32 ObjectMgr::GetPlayerAccountIdByPlayerName(const std::string& name) const
{
    QueryResult* result = CharacterDatabase.PQuery("SELECT `account` FROM `characters` WHERE `name` = '%s'", name.c_str());
    if (result)
    {
        uint32 acc = (*result)[0].GetUInt32();
        delete result;
        return acc;
    }

    return 0;
}

/**
 * @brief Gets class-based level info for a player class and level.
 *
 * @param class_ The player class id.
 * @param level The player level.
 * @param info Receives the class level info.
 */
void ObjectMgr::GetPlayerClassLevelInfo(uint32 class_, uint32 level, PlayerClassLevelInfo* info) const
{
    if (level < 1 || class_ >= MAX_CLASSES)
    {
        return;
    }

    PlayerClassInfo const* pInfo = &playerClassInfo[class_];

    if (level > sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL))
    {
        level = sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL);
    }

    *info = pInfo->levelInfo[level - 1];
}

/**
 * @brief Gets full player level info for a race, class, and level.
 *
 * @param race The player race id.
 * @param class_ The player class id.
 * @param level The requested player level.
 * @param info Receives the computed level info.
 */
void ObjectMgr::GetPlayerLevelInfo(uint32 race, uint32 class_, uint32 level, PlayerLevelInfo* info) const
{
    if (level < 1 || race   >= MAX_RACES || class_ >= MAX_CLASSES)
    {
        return;
    }

    PlayerInfo const* pInfo = &playerInfo[race][class_];
    if (pInfo->displayId_m == 0 || pInfo->displayId_f == 0)
    {
        return;
    }

    if (level <= sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL))
    {
        *info = pInfo->levelInfo[level - 1];
    }
    else
    {
        BuildPlayerLevelInfo(race, class_, level, info);
    }
}

/**
 * @brief Builds extrapolated player level stats beyond the stored base tables.
 *
 * @param race The player race id.
 * @param _class The player class id.
 * @param level The target player level.
 * @param info Receives the computed level info.
 */
void ObjectMgr::BuildPlayerLevelInfo(uint8 race, uint8 _class, uint8 level, PlayerLevelInfo* info) const
{
    // base data (last known level)
    *info = playerInfo[race][_class].levelInfo[sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL) - 1];

    for (int lvl = sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL) - 1; lvl < level; ++lvl)
    {
        switch (_class)
        {
            case CLASS_WARRIOR:
                info->stats[STAT_STRENGTH]  += (lvl > 23 ? 2 : (lvl > 1  ? 1 : 0));
                info->stats[STAT_STAMINA]   += (lvl > 23 ? 2 : (lvl > 1  ? 1 : 0));
                info->stats[STAT_AGILITY]   += (lvl > 36 ? 1 : (lvl > 6 && (lvl % 2) ? 1 : 0));
                info->stats[STAT_INTELLECT] += (lvl > 9 && !(lvl % 2) ? 1 : 0);
                info->stats[STAT_SPIRIT]    += (lvl > 9 && !(lvl % 2) ? 1 : 0);
                break;
            case CLASS_PALADIN:
                info->stats[STAT_STRENGTH]  += (lvl > 3  ? 1 : 0);
                info->stats[STAT_STAMINA]   += (lvl > 33 ? 2 : (lvl > 1 ? 1 : 0));
                info->stats[STAT_AGILITY]   += (lvl > 38 ? 1 : (lvl > 7 && !(lvl % 2) ? 1 : 0));
                info->stats[STAT_INTELLECT] += (lvl > 6 && (lvl % 2) ? 1 : 0);
                info->stats[STAT_SPIRIT]    += (lvl > 7 ? 1 : 0);
                break;
            case CLASS_HUNTER:
                info->stats[STAT_STRENGTH]  += (lvl > 4  ? 1 : 0);
                info->stats[STAT_STAMINA]   += (lvl > 4  ? 1 : 0);
                info->stats[STAT_AGILITY]   += (lvl > 33 ? 2 : (lvl > 1 ? 1 : 0));
                info->stats[STAT_INTELLECT] += (lvl > 8 && (lvl % 2) ? 1 : 0);
                info->stats[STAT_SPIRIT]    += (lvl > 38 ? 1 : (lvl > 9 && !(lvl % 2) ? 1 : 0));
                break;
            case CLASS_ROGUE:
                info->stats[STAT_STRENGTH]  += (lvl > 5  ? 1 : 0);
                info->stats[STAT_STAMINA]   += (lvl > 4  ? 1 : 0);
                info->stats[STAT_AGILITY]   += (lvl > 16 ? 2 : (lvl > 1 ? 1 : 0));
                info->stats[STAT_INTELLECT] += (lvl > 8 && !(lvl % 2) ? 1 : 0);
                info->stats[STAT_SPIRIT]    += (lvl > 38 ? 1 : (lvl > 9 && !(lvl % 2) ? 1 : 0));
                break;
            case CLASS_PRIEST:
                info->stats[STAT_STRENGTH]  += (lvl > 9 && !(lvl % 2) ? 1 : 0);
                info->stats[STAT_STAMINA]   += (lvl > 5  ? 1 : 0);
                info->stats[STAT_AGILITY]   += (lvl > 38 ? 1 : (lvl > 8 && (lvl % 2) ? 1 : 0));
                info->stats[STAT_INTELLECT] += (lvl > 22 ? 2 : (lvl > 1 ? 1 : 0));
                info->stats[STAT_SPIRIT]    += (lvl > 3  ? 1 : 0);
                break;
            case CLASS_SHAMAN:
                info->stats[STAT_STRENGTH]  += (lvl > 34 ? 1 : (lvl > 6 && (lvl % 2) ? 1 : 0));
                info->stats[STAT_STAMINA]   += (lvl > 4 ? 1 : 0);
                info->stats[STAT_AGILITY]   += (lvl > 7 && !(lvl % 2) ? 1 : 0);
                info->stats[STAT_INTELLECT] += (lvl > 5 ? 1 : 0);
                info->stats[STAT_SPIRIT]    += (lvl > 4 ? 1 : 0);
                break;
            case CLASS_MAGE:
                info->stats[STAT_STRENGTH]  += (lvl > 9 && !(lvl % 2) ? 1 : 0);
                info->stats[STAT_STAMINA]   += (lvl > 5  ? 1 : 0);
                info->stats[STAT_AGILITY]   += (lvl > 9 && !(lvl % 2) ? 1 : 0);
                info->stats[STAT_INTELLECT] += (lvl > 24 ? 2 : (lvl > 1 ? 1 : 0));
                info->stats[STAT_SPIRIT]    += (lvl > 33 ? 2 : (lvl > 2 ? 1 : 0));
                break;
            case CLASS_WARLOCK:
                info->stats[STAT_STRENGTH]  += (lvl > 9 && !(lvl % 2) ? 1 : 0);
                info->stats[STAT_STAMINA]   += (lvl > 38 ? 2 : (lvl > 3 ? 1 : 0));
                info->stats[STAT_AGILITY]   += (lvl > 9 && !(lvl % 2) ? 1 : 0);
                info->stats[STAT_INTELLECT] += (lvl > 33 ? 2 : (lvl > 2 ? 1 : 0));
                info->stats[STAT_SPIRIT]    += (lvl > 38 ? 2 : (lvl > 3 ? 1 : 0));
                break;
            case CLASS_DRUID:
                info->stats[STAT_STRENGTH]  += (lvl > 38 ? 2 : (lvl > 6 && (lvl % 2) ? 1 : 0));
                info->stats[STAT_STAMINA]   += (lvl > 32 ? 2 : (lvl > 4 ? 1 : 0));
                info->stats[STAT_AGILITY]   += (lvl > 38 ? 2 : (lvl > 8 && (lvl % 2) ? 1 : 0));
                info->stats[STAT_INTELLECT] += (lvl > 38 ? 3 : (lvl > 4 ? 1 : 0));
                info->stats[STAT_SPIRIT]    += (lvl > 38 ? 3 : (lvl > 5 ? 1 : 0));
        }
    }
}

/* ********************************************************************************************* */
/* *                                Static Wrappers                                              */
/* ********************************************************************************************* */

/**
 * @brief Gets static gameobject template data by entry id.
 *
 * @param id The gameobject entry id.
 * @return The gameobject template, or null if missing.
 */
GameObjectInfo const* ObjectMgr::GetGameObjectInfo(uint32 id) { return sGOStorage.LookupEntry<GameObjectInfo>(id); }

/**
 * @brief Finds an online player by name.
 *
 * @param name The player name.
 * @return The matching player, or null if not found.
 */
Player* ObjectMgr::GetPlayer(const char* name) { return sObjectAccessor.FindPlayerByName(name); }

/**
 * @brief Finds a player by GUID.
 *
 * @param guid The player GUID.
 * @param inWorld true to restrict the search to players currently in world.
 * @return The matching player, or null if not found.
 */
Player* ObjectMgr::GetPlayer(ObjectGuid guid, bool inWorld /*=true*/) { return sObjectAccessor.FindPlayer(guid, inWorld); }

/**
 * @brief Gets static creature template data by entry id.
 *
 * @param id The creature entry id.
 * @return The creature template, or null if missing.
 */
CreatureInfo const* ObjectMgr::GetCreatureTemplate(uint32 id) { return sCreatureStorage.LookupEntry<CreatureInfo>(id); }

/**
 * @brief Gets creature model metadata by display id.
 *
 * @param modelid The creature model id.
 * @return The creature model info, or null if missing.
 */
CreatureModelInfo const* ObjectMgr::GetCreatureModelInfo(uint32 modelid) { return sCreatureModelStorage.LookupEntry<CreatureModelInfo>(modelid); }

/**
 * @brief Gets equipment template data by entry id.
 *
 * @param entry The equipment template entry id.
 * @return The equipment template, or null if missing.
 */
EquipmentInfo const* ObjectMgr::GetEquipmentInfo(uint32 entry) { return sEquipmentStorage.LookupEntry<EquipmentInfo>(entry); }

/**
 * @brief Gets creature spawn addon data by low GUID.
 *
 * @param lowguid The creature spawn low GUID.
 * @return The addon data, or null if missing.
 */
CreatureDataAddon const* ObjectMgr::GetCreatureAddon(uint32 lowguid) { return sCreatureDataAddonStorage.LookupEntry<CreatureDataAddon>(lowguid); }

/**
 * @brief Gets creature template addon data by entry id.
 *
 * @param entry The creature entry id.
 * @return The template addon data, or null if missing.
 */
CreatureDataAddon const* ObjectMgr::GetCreatureTemplateAddon(uint32 entry) { return sCreatureInfoAddonStorage.LookupEntry<CreatureDataAddon>(entry); }

/**
 * @brief Gets item prototype data by entry id.
 *
 * @param id The item entry id.
 * @return The item prototype, or null if missing.
 */
ItemPrototype const* ObjectMgr::GetItemPrototype(uint32 id) { return sItemStorage.LookupEntry<ItemPrototype>(id); }

/**
 * @brief Gets instance template data by map id.
 *
 * @param map The instance map id.
 * @return The instance template, or null if missing.
 */
InstanceTemplate const* ObjectMgr::GetInstanceTemplate(uint32 map) { return sInstanceTemplate.LookupEntry<InstanceTemplate>(map); }

/* ********************************************************************************************* */
/* *                                Loading Functions                                            */
/* ********************************************************************************************* */





struct SQLInstanceLoader : public SQLStorageLoaderBase<SQLInstanceLoader, SQLStorage>
{
    template<class D>
    void convert_from_str(uint32 /*field_pos*/, char const* src, D& dst)
    {
        dst = D(sScriptMgr.GetScriptId(src));
    }
};

/**
 * @brief Loads instance templates and validates map and ghost entrance data.
 */
void ObjectMgr::LoadInstanceTemplate()
{
    SQLInstanceLoader loader;
    loader.Load(sInstanceTemplate);

    for (uint32 i = 0; i < sInstanceTemplate.GetMaxEntry(); ++i)
    {
        InstanceTemplate const* temp = GetInstanceTemplate(i);
        if (!temp)
        {
            continue;
        }

        MapEntry const* mapEntry = sMapStore.LookupEntry(temp->map);
        if (!mapEntry)
        {
            sLog.outErrorDb("ObjectMgr::LoadInstanceTemplate: bad mapid %d for template!", temp->map);
            sInstanceTemplate.EraseEntry(i);
            continue;
        }

        if (!mapEntry->Instanceable())
        {
            sLog.outErrorDb("ObjectMgr::LoadInstanceTemplate: non-instanceable mapid %d for template!", temp->map);
            sInstanceTemplate.EraseEntry(i);
            continue;
        }

        if (temp->parent > 0)
        {
            // check existence
            MapEntry const* parentEntry = sMapStore.LookupEntry(temp->parent);
            if (!parentEntry)
            {
                sLog.outErrorDb("ObjectMgr::LoadInstanceTemplate: bad parent map id %u for instance template %d template!",
                                temp->parent, temp->map);
                const_cast<InstanceTemplate*>(temp)->parent = 0;
                continue;
            }

            if (parentEntry->IsContinent())
            {
                sLog.outErrorDb("ObjectMgr::LoadInstanceTemplate: parent point to continent map id %u for instance template %d template, ignored, need be set only for non-continent parents!",
                                parentEntry->MapID, temp->map);
                const_cast<InstanceTemplate*>(temp)->parent = 0;
                continue;
            }
        }
    }

    sLog.outString(">> Loaded %u Instance Template definitions", sInstanceTemplate.GetRecordCount());
    sLog.outString();
}

struct SQLWorldLoader : public SQLStorageLoaderBase<SQLWorldLoader, SQLStorage>
{
    template<class D>
    void convert_from_str(uint32 /*field_pos*/, char const* src, D& dst)
    {
        dst = D(sScriptMgr.GetScriptId(src));
    }
};

/**
 * @brief Loads player condition definitions and removes invalid entries.
 */
void ObjectMgr::LoadConditions()
{
    SQLWorldLoader loader;
    loader.Load(sConditionStorage);

    for (uint32 i = 0; i < sConditionStorage.GetMaxEntry(); ++i)
    {
        const PlayerCondition* condition = sConditionStorage.LookupEntry<PlayerCondition>(i);
        if (!condition)
        {
            continue;
        }

        if (!condition->IsValid())
        {
            sLog.outErrorDb("ObjectMgr::LoadConditions: invalid condition_entry %u, skip", i);
            sConditionStorage.EraseEntry(i);
            continue;
        }
    }

    sLog.outString(">> Loaded %u Condition definitions", sConditionStorage.GetRecordCount());
    sLog.outString();
}

/**
 * @brief Gets a loaded gossip text record by id.
 *
 * @param Text_ID The gossip text identifier.
 * @return The gossip text record, or null if missing.
 */
GossipText const* ObjectMgr::GetGossipText(uint32 Text_ID) const
{
    GossipTextMap::const_iterator itr = mGossipText.find(Text_ID);
    if (itr != mGossipText.end())
    {
        return &itr->second;
    }
    return NULL;
}



// not very fast function but it is called only once a day, or on starting-up
/// @param serverUp true if the server is already running, false when the server is started
void ObjectMgr::ReturnOrDeleteOldMails(bool serverUp)
{
    time_t curTime = time(NULL);
    std::tm lt = safe_localtime(curTime);
    uint64 basetime(curTime);
    sLog.outString("Returning mails current time: hour: %d, minute: %d, second: %d ", lt.tm_hour, lt.tm_min, lt.tm_sec);

    // delete all old mails without item and without body immediately, if starting server
    if (!serverUp)
    {
        CharacterDatabase.PExecute("DELETE FROM `mail` WHERE `expire_time` < '" UI64FMTD "' AND `has_items` = '0' AND `body` = ''", (uint64)basetime);
    }
    //                                                      0    1             2        3          4           5             6     7         8
    QueryResult* result = CharacterDatabase.PQuery("SELECT `id`,`messageType`,`sender`,`receiver`,`has_items`,`expire_time`,`cod`,`checked`,`mailTemplateId` FROM `mail` WHERE `expire_time` < '" UI64FMTD "'", (uint64)basetime);
    if (!result)
    {
        BarGoLink bar(1);
        bar.step();
        sLog.outString(">> Only expired mails (need to be return or delete) or DB table `mail` is empty.");
        sLog.outString();
        return;                                             // any mails need to be returned or deleted
    }

    // std::ostringstream delitems, delmails; // will be here for optimization
    // bool deletemail = false, deleteitem = false;
    // delitems << "DELETE FROM `item_instance` WHERE `guid` IN ( ";
    // delmails << "DELETE FROM `mail` WHERE `id` IN ( "

    BarGoLink bar(result->GetRowCount());
    uint32 count = 0;
    Field* fields;

    do
    {
        bar.step();

        fields = result->Fetch();
        Mail* m = new Mail;
        m->messageID = fields[0].GetUInt32();
        m->messageType = fields[1].GetUInt8();
        m->sender = fields[2].GetUInt32();
        m->receiverGuid = ObjectGuid(HIGHGUID_PLAYER, fields[3].GetUInt32());
        bool has_items = fields[4].GetBool();
        m->expire_time = (time_t)fields[5].GetUInt64();
        m->deliver_time = 0;
        m->COD = fields[6].GetUInt32();
        m->checked = fields[7].GetUInt32();
        m->mailTemplateId = fields[8].GetInt16();

        Player* pl = 0;
        if (serverUp)
        {
            pl = GetPlayer(m->receiverGuid);
        }
        if (pl)
        {
            // this code will run very improbably (the time is between 4 and 5 am, in game is online a player, who has old mail
            // his in mailbox and he has already listed his mails )
            delete m;
            continue;
        }
        // delete or return mail:
        if (has_items)
        {
            QueryResult* resultItems = CharacterDatabase.PQuery("SELECT `item_guid`,`item_template` FROM `mail_items` WHERE `mail_id`='%u'", m->messageID);
            if (resultItems)
            {
                do
                {
                    Field* fields2 = resultItems->Fetch();

                    uint32 item_guid_low = fields2[0].GetUInt32();
                    uint32 item_template = fields2[1].GetUInt32();

                    m->AddItem(item_guid_low, item_template);
                }
                while (resultItems->NextRow());

                delete resultItems;
            }
            // if it is mail from non-player, or if it's already return mail, it shouldn't be returned, but deleted
            if (m->messageType != MAIL_NORMAL || (m->checked & (MAIL_CHECK_MASK_COD_PAYMENT | MAIL_CHECK_MASK_RETURNED)))
            {
                // mail open and then not returned
                for (MailItemInfoVec::iterator itr2 = m->items.begin(); itr2 != m->items.end(); ++itr2)
                {
                    CharacterDatabase.PExecute("DELETE FROM `item_instance` WHERE `guid` = '%u'", itr2->item_guid);
                }
            }
            else
            {
                // mail will be returned:
                CharacterDatabase.PExecute("UPDATE `mail` SET `sender` = '%u', `receiver` = '%u', `expire_time` = '" UI64FMTD "', `deliver_time` = '" UI64FMTD "', `cod` = '0', `checked` = '%u' WHERE `id` = '%u'",
                                           m->receiverGuid.GetCounter(), m->sender, (uint64)(basetime + 30 * DAY), (uint64)basetime, MAIL_CHECK_MASK_RETURNED, m->messageID);
                for (MailItemInfoVec::iterator itr2 = m->items.begin(); itr2 != m->items.end(); ++itr2)
                {
                    // update receiver in mail items for its proper delivery, and in instance_item for avoid lost item at sender delete
                    CharacterDatabase.PExecute("UPDATE `mail_items` SET `receiver` = %u WHERE `item_guid` = '%u'", m->sender, itr2->item_guid);
                    CharacterDatabase.PExecute("UPDATE `item_instance` SET `owner_guid` = %u WHERE `guid` = '%u'", m->sender, itr2->item_guid);
                }
                delete m;
                continue;
            }
        }

        // deletemail = true;
        // delmails << m->messageID << ", ";
        CharacterDatabase.PExecute("DELETE FROM `mail` WHERE `id` = '%u'", m->messageID);
        delete m;
        ++count;
    }
    while (result->NextRow());
    delete result;

    sLog.outString(">> Loaded %u mails", count);
    sLog.outString();
}

/**
 * @brief Loads area trigger to quest objective relationships.
 */
void ObjectMgr::LoadQuestAreaTriggers()
{
    mQuestAreaTriggerMap.clear();                           // need for reload case

    QueryResult* result = WorldDatabase.PQuery("SELECT `entry`, `quest` FROM `quest_relations` WHERE `actor` = %d", QA_AREATRIGGER);

    uint32 count = 0;

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();
        sLog.outString(">> Loaded %u quest trigger points", count);
        sLog.outString();
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        ++count;
        bar.step();

        Field* fields = result->Fetch();

        uint32 trigger_ID = fields[0].GetUInt32();
        uint32 quest_ID   = fields[1].GetUInt32();

        AreaTriggerEntry const* atEntry = sAreaTriggerStore.LookupEntry(trigger_ID);
        if (!atEntry)
        {
            sLog.outErrorDb("Table `quest_relations` has area trigger (ID: %u) not listed in `AreaTrigger.dbc`.", trigger_ID);
            continue;
        }

        Quest const* quest = GetQuestTemplate(quest_ID);
        if (!quest)
        {
            sLog.outErrorDb("Table `quest_relations` has record (id: %u) for not existing quest %u", trigger_ID, quest_ID);
            continue;
        }

        if (!quest->HasSpecialFlag(QUEST_SPECIAL_FLAG_EXPLORATION_OR_EVENT))
        {
            sLog.outErrorDb("Table `quest_relations` has record (id: %u) for not quest %u, but quest not have flag QUEST_SPECIAL_FLAG_EXPLORATION_OR_EVENT. Trigger or quest flags must be fixed, quest modified to require objective.", trigger_ID, quest_ID);

            // this will prevent quest completing without objective
            const_cast<Quest*>(quest)->SetSpecialFlag(QUEST_SPECIAL_FLAG_EXPLORATION_OR_EVENT);

            // continue; - quest modified to required objective and trigger can be allowed.
        }

        mQuestAreaTriggerMap[trigger_ID] = quest_ID;
    }
    while (result->NextRow());

    delete result;

    sLog.outString(">> Loaded %u quest trigger points", count);
    sLog.outString();
}

/**
 * @brief Loads area triggers that mark tavern rest zones.
 */
void ObjectMgr::LoadTavernAreaTriggers()
{
    mTavernAreaTriggerSet.clear();                          // need for reload case

    QueryResult* result = WorldDatabase.Query("SELECT `id` FROM `areatrigger_tavern`");

    uint32 count = 0;

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();
        sLog.outString(">> Loaded %u tavern triggers", count);
        sLog.outString();
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        ++count;
        bar.step();

        Field* fields = result->Fetch();

        uint32 Trigger_ID      = fields[0].GetUInt32();

        AreaTriggerEntry const* atEntry = sAreaTriggerStore.LookupEntry(Trigger_ID);
        if (!atEntry)
        {
            sLog.outErrorDb("Table `areatrigger_tavern` has area trigger (ID:%u) not listed in `AreaTrigger.dbc`.", Trigger_ID);
            continue;
        }

        mTavernAreaTriggerSet.insert(Trigger_ID);
    }
    while (result->NextRow());

    delete result;

    sLog.outString(">> Loaded %u tavern triggers", count);
    sLog.outString();
}

/**
 * @brief Renumbers group ids into a compact sequential range.
 */
void ObjectMgr::PackGroupIds()
{
    // this routine renumbers groups in such a way so they start from 1 and go up

    // obtain set of all groups
    std::set<uint32> groupIds;

    // all valid ids are in the instance table
    // any associations to ids not in this table are assumed to be
    // cleaned already in CleanupInstances
    QueryResult* result = CharacterDatabase.Query("SELECT `groupId` FROM `groups`");
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 id = fields[0].GetUInt32();

            if (id == 0)
            {
                CharacterDatabase.BeginTransaction();
                CharacterDatabase.PExecute("DELETE FROM `groups` WHERE `groupId` = '%u'", id);
                CharacterDatabase.PExecute("DELETE FROM `group_member` WHERE `groupId` = '%u'", id);
                CharacterDatabase.CommitTransaction();
                continue;
            }

            groupIds.insert(id);
        }
        while (result->NextRow());
        delete result;
    }

    BarGoLink bar(groupIds.size() + 1);
    bar.step();

    uint32 groupId = 1;
    // we do assume std::set is sorted properly on integer value
    for (std::set<uint32>::iterator i = groupIds.begin(); i != groupIds.end(); ++i)
    {
        if (*i != groupId)
        {
            // remap group id
            CharacterDatabase.BeginTransaction();
            CharacterDatabase.PExecute("UPDATE `groups` SET `groupId` = '%u' WHERE `groupId` = '%u'", groupId, *i);
            CharacterDatabase.PExecute("UPDATE `group_member` SET `groupId` = '%u' WHERE `groupId` = '%u'", groupId, *i);
            CharacterDatabase.CommitTransaction();
        }

        ++groupId;
        bar.step();
    }

    sLog.outString(">> Group Ids remapped, next group id is %u", groupId);
    sLog.outString();
}

/**
 * @brief Initializes high-water marks for generated GUID and id sequences.
 */
void ObjectMgr::SetHighestGuids()
{
    QueryResult* result = CharacterDatabase.Query("SELECT MAX(`guid`) FROM `characters`");
    if (result)
    {
        m_CharGuids.Set((*result)[0].GetUInt32() + 1);
        delete result;
    }

    result = WorldDatabase.Query("SELECT MAX(`guid`) FROM `creature`");
    if (result)
    {
        m_FirstTemporaryCreatureGuid = (*result)[0].GetUInt32() + 1;
        delete result;
    }

    result = CharacterDatabase.Query("SELECT MAX(`guid`) FROM `item_instance`");
    if (result)
    {
        m_ItemGuids.Set((*result)[0].GetUInt32() + 1);
        delete result;
    }

    result = CharacterDatabase.Query("SELECT MAX(`id`) FROM `instance`");
    if (result)
    {
        m_InstanceGuids.Set((*result)[0].GetUInt32() + 1);
        delete result;
    }

    // Cleanup other tables from nonexistent guids (>=m_hiItemGuid)
    CharacterDatabase.BeginTransaction();
    CharacterDatabase.PExecute("DELETE FROM `character_inventory` WHERE `item` >= '%u'", m_ItemGuids.GetNextAfterMaxUsed());
    CharacterDatabase.PExecute("DELETE FROM `mail_items` WHERE `item_guid` >= '%u'", m_ItemGuids.GetNextAfterMaxUsed());
    CharacterDatabase.PExecute("DELETE FROM `auction` WHERE `itemguid` >= '%u'", m_ItemGuids.GetNextAfterMaxUsed());
    CharacterDatabase.PExecute("DELETE FROM `guild_bank_item` WHERE `item_guid` >= '%u'", m_ItemGuids.GetNextAfterMaxUsed());
    CharacterDatabase.CommitTransaction();

    result = WorldDatabase.Query("SELECT MAX(`guid`) FROM `gameobject`");
    if (result)
    {
        m_FirstTemporaryGameObjectGuid = (*result)[0].GetUInt32() + 1;
        delete result;
    }

    result = CharacterDatabase.Query("SELECT MAX(`id`) FROM `auction`");
    if (result)
    {
        m_AuctionIds.Set((*result)[0].GetUInt32() + 1);
        delete result;
    }

    result = CharacterDatabase.Query("SELECT MAX(`id`) FROM `mail`");
    if (result)
    {
        m_MailIds.Set((*result)[0].GetUInt32() + 1);
        delete result;
    }

    result = CharacterDatabase.Query("SELECT MAX(`guid`) FROM `corpse`");
    if (result)
    {
        m_CorpseGuids.Set((*result)[0].GetUInt32() + 1);
        delete result;
    }

    result = CharacterDatabase.Query("SELECT MAX(`arenateamid`) FROM `arena_team`");
    if (result)
    {
        m_ArenaTeamIds.Set((*result)[0].GetUInt32() + 1);
        delete result;
    }

    result = CharacterDatabase.Query("SELECT MAX(`setguid`) FROM `character_equipmentsets`");
    if (result)
    {
        m_EquipmentSetIds.Set((*result)[0].GetUInt64() + 1);
        delete result;
    }

    result = CharacterDatabase.Query("SELECT MAX(`guildid`) FROM `guild`");
    if (result)
    {
        m_GuildIds.Set((*result)[0].GetUInt32() + 1);
        delete result;
    }

    result = CharacterDatabase.Query("SELECT MAX(`groupId`) FROM `groups`");
    if (result)
    {
        m_GroupGuids.Set((*result)[0].GetUInt32() + 1);
        delete result;
    }

    // setup reserved ranges for static guids spawn
    m_StaticCreatureGuids.Set(m_FirstTemporaryCreatureGuid);
    m_FirstTemporaryCreatureGuid += sWorld.getConfig(CONFIG_UINT32_GUID_RESERVE_SIZE_CREATURE);

    m_StaticGameObjectGuids.Set(m_FirstTemporaryGameObjectGuid);
    m_FirstTemporaryGameObjectGuid += sWorld.getConfig(CONFIG_UINT32_GUID_RESERVE_SIZE_GAMEOBJECT);
}


/**
 * @brief Loads exploration base experience values by level.
 */
void ObjectMgr::LoadExplorationBaseXP()
{
    uint32 count = 0;
    QueryResult* result = WorldDatabase.Query("SELECT `level`,`basexp` FROM `exploration_basexp`");

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();
        sLog.outString(">> Loaded %u BaseXP definitions", count);
        sLog.outString();
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        bar.step();

        Field* fields = result->Fetch();
        uint32 level  = fields[0].GetUInt32();
        uint32 basexp = fields[1].GetUInt32();
        mBaseXPTable[level] = basexp;
        ++count;
    }
    while (result->NextRow());

    delete result;

    sLog.outString(">> Loaded %u BaseXP definitions", count);
    sLog.outString();
}

/**
 * @brief Gets the exploration base experience for a level.
 *
 * @param level The player level.
 * @return The configured base exploration XP, or 0 if missing.
 */
uint32 ObjectMgr::GetBaseXP(uint32 level) const
{
    BaseXPMap::const_iterator itr = mBaseXPTable.find(level);
    return itr != mBaseXPTable.end() ? itr->second : 0;
}

/**
 * @brief Gets the XP required for the next player level.
 *
 * @param level The current player level index.
 * @return The XP requirement, or 0 if out of range.
 */
uint32 ObjectMgr::GetXPForLevel(uint32 level) const
{
    if (level < mPlayerXPperLevel.size())
    {
        return mPlayerXPperLevel[level];
    }
    return 0;
}

/**
 * @brief Loads pet name fragments used for random pet naming.
 */
void ObjectMgr::LoadPetNames()
{
    uint32 count = 0;
    QueryResult* result = WorldDatabase.Query("SELECT `word`,`entry`,`half` FROM `pet_name_generation`");

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();
        sLog.outString(">> Loaded %u pet name parts", count);
        sLog.outString();
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        bar.step();

        Field* fields = result->Fetch();
        std::string word = fields[0].GetString();
        uint32 entry     = fields[1].GetUInt32();
        bool   half      = fields[2].GetBool();
        if (half)
        {
            PetHalfName1[entry].push_back(word);
        }
        else
        {
            PetHalfName0[entry].push_back(word);
        }
        ++count;
    }
    while (result->NextRow());
    delete result;

    sLog.outString(">> Loaded %u pet name parts", count);
    sLog.outString();
}

/**
 * @brief Initializes the next generated pet number from existing pets.
 */
void ObjectMgr::LoadPetNumber()
{
    QueryResult* result = CharacterDatabase.Query("SELECT MAX(`id`) FROM `character_pet`");
    if (result)
    {
        Field* fields = result->Fetch();
        m_PetNumbers.Set(fields[0].GetUInt32() + 1);
        delete result;
    }

    BarGoLink bar(1);
    bar.step();

    sLog.outString(">> Loaded the max pet number: %d", m_PetNumbers.GetNextAfterMaxUsed() - 1);
    sLog.outString();
}

/**
 * @brief Generates a random or fallback pet name for a creature entry.
 *
 * @param entry The creature entry id.
 * @return The generated pet name.
 */
std::string ObjectMgr::GeneratePetName(uint32 entry)
{
    std::vector<std::string>& list0 = PetHalfName0[entry];
    std::vector<std::string>& list1 = PetHalfName1[entry];

    if (list0.empty() || list1.empty())
    {
        CreatureInfo const* cinfo = GetCreatureTemplate(entry);
        char const* petname = GetPetName(cinfo->Family, sWorld.GetDefaultDbcLocale());
        if (!petname)
        {
            petname = cinfo->Name;
        }
        return std::string(petname);
    }

    return *(list0.begin() + urand(0, list0.size() - 1)) + *(list1.begin() + urand(0, list1.size() - 1));
}

/**
 * @brief Loads persistent corpse records from the character database.
 */
void ObjectMgr::LoadCorpses()
{
    uint32 count = 0;
    //                                                    0            1       2                  3                  4                  5                   6
    QueryResult* result = CharacterDatabase.Query("SELECT `corpse`.`guid`, `player`, `corpse`.`position_x`, `corpse`.`position_y`, `corpse`.`position_z`, `corpse`.`orientation`, `corpse`.`map`, "
                          //   7     8             9           10           11        12      13       14             15              16                17         18
                          "`time`, `corpse_type`, `instance`, `phaseMask`, `gender`, `race`, `class`, `playerBytes`, `playerBytes2`, `equipmentCache`, `guildId`, `playerFlags` FROM `corpse` "
                          "JOIN `characters` ON `player` = `characters`.`guid` "
                          "LEFT JOIN `guild_member` ON `player`=`guild_member`.`guid` WHERE `corpse_type` <> 0");

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();
        sLog.outString(">> Loaded %u corpses", count);
        sLog.outString();
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        bar.step();

        Field* fields = result->Fetch();

        uint32 guid = fields[0].GetUInt32();

        Corpse* corpse = new Corpse;
        if (!corpse->LoadFromDB(guid, fields))
        {
            delete corpse;
            continue;
        }

        sObjectAccessor.AddCorpse(corpse);

        ++count;
    }
    while (result->NextRow());
    delete result;

    sLog.outString(">> Loaded %u corpses", count);
    sLog.outString();
}

/**
 * @brief Loads point-of-interest definitions used by NPC map markers.
 */
void ObjectMgr::LoadPointsOfInterest()
{
    mPointsOfInterest.clear();                              // need for reload case

    uint32 count = 0;

    //                                                0      1  2  3      4     5
    QueryResult* result = WorldDatabase.Query("SELECT `entry`, `x`, `y`, `icon`, `flags`, `data`, `icon_name` FROM `points_of_interest`");

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();
        sLog.outErrorDb(">> Loaded 0 Points of Interest definitions. DB table `points_of_interest` is empty.");
        sLog.outString();
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();
        bar.step();

        uint32 point_id = fields[0].GetUInt32();

        PointOfInterest POI;
        POI.x                    = fields[1].GetFloat();
        POI.y                    = fields[2].GetFloat();
        POI.icon                 = fields[3].GetUInt32();
        POI.flags                = fields[4].GetUInt32();
        POI.data                 = fields[5].GetUInt32();
        POI.icon_name            = fields[6].GetCppString();

        if (!MaNGOS::IsValidMapCoord(POI.x, POI.y))
        {
            sLog.outErrorDb("Table `points_of_interest` (Entry: %u) have invalid coordinates (X: %f Y: %f), ignored.", point_id, POI.x, POI.y);
            continue;
        }

        mPointsOfInterest[point_id] = POI;

        ++count;
    }
    while (result->NextRow());

    delete result;

    sLog.outString(">> Loaded %u Points of Interest definitions", count);
}

void ObjectMgr::LoadQuestPOI()
{
    mQuestPOIMap.clear();                              // need for reload case

    uint32 count = 0;

    //                                                0        1      2         3      4          5        6     7
    QueryResult* result = WorldDatabase.Query("SELECT `questId`, `poiId`, `objIndex`, `mapId`, `mapAreaId`, `floorId`, `unk3`, `unk4` FROM `quest_poi`");

    if (!result)
    {
        BarGoLink bar(1);

        bar.step();

        sLog.outString();
        sLog.outErrorDb(">> Loaded 0 quest POI definitions. DB table `quest_poi` is empty.");
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();
        bar.step();

        uint32 questId          = fields[0].GetUInt32();
        uint32 poiId            = fields[1].GetUInt32();
        int32  objIndex         = fields[2].GetInt32();
        uint32 mapId            = fields[3].GetUInt32();
        uint32 mapAreaId        = fields[4].GetUInt32();
        uint32 floorId          = fields[5].GetUInt32();
        uint32 unk3             = fields[6].GetUInt32();
        uint32 unk4             = fields[7].GetUInt32();

        QuestPOI POI(poiId, objIndex, mapId, mapAreaId, floorId, unk3, unk4);

        mQuestPOIMap[questId].push_back(POI);

        ++count;
    }
    while (result->NextRow());

    delete result;

    QueryResult* points = WorldDatabase.Query("SELECT `questId`, `poiId`, `x`, `y` FROM `quest_poi_points`");

    if (points)
    {
        do
        {
            Field* pointFields  = points->Fetch();

            uint32 questId      = pointFields[0].GetUInt32();
            uint32 poiId        = pointFields[1].GetUInt32();
            int32  x            = pointFields[2].GetInt32();
            int32  y            = pointFields[3].GetInt32();

            QuestPOIVector& vect = mQuestPOIMap[questId];

            for (QuestPOIVector::iterator itr = vect.begin(); itr != vect.end(); ++itr)
            {
                if (itr->PoiId != poiId)
                {
                    continue;
                }

                QuestPOIPoint point(x, y);
                itr->points.push_back(point);
                break;
            }
        }
        while (points->NextRow());

        delete points;
    }

    sLog.outString();
    sLog.outString(">> Loaded %u quest POI definitions", count);
}


static char SERVER_SIDE_SPELL[] = "MaNGOS server-side spell";

struct SQLSpellLoader : public SQLStorageLoaderBase<SQLSpellLoader, SQLHashStorage>
{
    template<class S, class D>
    void default_fill(uint32 field_pos, S src, D& dst)
    {
        if (field_pos == LOADED_SPELLDBC_FIELD_POS_EQUIPPED_ITEM_CLASS)
        {
            dst = D(-1);
        }
        else
        {
            dst = D(src);
        }
    }

    void default_fill_to_str(uint32 field_pos, char const* /*src*/, char*& dst)
    {
        if (field_pos == LOADED_SPELLDBC_FIELD_POS_SPELLNAME_0)
        {
            dst = SERVER_SIDE_SPELL;
        }
        else
        {
            dst = new char[1];
            *dst = 0;
        }
    }
};

void ObjectMgr::LoadSpellTemplate()
{
    SQLSpellLoader loader;
    loader.Load(sSpellTemplate);

    sLog.outString(">> Loaded %u spell definitions", sSpellTemplate.GetRecordCount());
    sLog.outString();

    for (uint32 i = 1; i < sSpellTemplate.GetMaxEntry(); ++i)
    {
        // check data correctness
        SpellEntry const* spellEntry = sSpellTemplate.LookupEntry<SpellEntry>(i);
        if (!spellEntry)
        {
            continue;
        }

        // insert serverside spell data
        if (sSpellStore.GetNumRows() <= i)
        {
            sLog.outErrorDb("Loading Spell Template for spell %u, index out of bounds (max = %u)", i, sSpellStore.GetNumRows());
            continue;
        }
        else
        {
            sSpellStore.InsertEntry(const_cast<SpellEntry*>(spellEntry), i);
        }
    }
}

/**
 * @brief Removes stored creature spawn data and its grid mapping.
 *
 * @param guid The creature spawn GUID.
 */
void ObjectMgr::DeleteCreatureData(uint32 guid)
{
    // remove mapid*cellid -> guid_set map
    CreatureData const* data = GetCreatureData(guid);
    if (data)
    {
        RemoveCreatureFromGrid(guid, data);
    }

    mCreatureDataMap.erase(guid);
}

/**
 * @brief Removes stored gameobject spawn data and its grid mapping.
 *
 * @param guid The gameobject spawn GUID.
 */
void ObjectMgr::DeleteGOData(uint32 guid)
{
    // remove mapid*cellid -> guid_set map
    GameObjectData const* data = GetGOData(guid);
    if (data)
    {
        RemoveGameobjectFromGrid(guid, data);
    }

    mGameObjectDataMap.erase(guid);
}

/**
 * @brief Adds corpse cell lookup data for a player corpse.
 *
 * @param mapid The map id.
 * @param cellid The cell id.
 * @param player_guid The owning player GUID low part.
 * @param instance The instance id.
 */
void ObjectMgr::AddCorpseCellData(uint32 mapid, uint32 cellid, uint32 player_guid, uint32 instance)
{
    // corpses are always added to spawn mode 0 and they are spawned by their instance id
    CellObjectGuids& cell_guids = mMapObjectGuids[MAKE_PAIR32(mapid, 0)][cellid];
    cell_guids.corpses[player_guid] = instance;
}

/**
 * @brief Removes corpse cell lookup data for a player corpse.
 *
 * @param mapid The map id.
 * @param cellid The cell id.
 * @param player_guid The owning player GUID low part.
 */
void ObjectMgr::DeleteCorpseCellData(uint32 mapid, uint32 cellid, uint32 player_guid)
{
    // corpses are always added to spawn mode 0 and they are spawned by their instance id
    CellObjectGuids& cell_guids = mMapObjectGuids[MAKE_PAIR32(mapid, 0)][cellid];
    cell_guids.corpses.erase(player_guid);
}


/**
 * @brief Gets the internal locale index for a locale constant.
 *
 * @param loc The locale constant.
 * @return The locale index, or -1 for the default locale.
 */
int ObjectMgr::GetIndexForLocale(LocaleConstant loc)
{
    if (loc == LOCALE_enUS)
    {
        return -1;
    }

    for (size_t i = 0; i < m_LocalForIndex.size(); ++i)
    {
        if (m_LocalForIndex[i] == loc)
        {
            return i;
        }
    }

    return -1;
}

/**
 * @brief Gets the locale constant mapped to an internal locale index.
 *
 * @param i The locale index.
 * @return The mapped locale constant, or the default locale if out of range.
 */
LocaleConstant ObjectMgr::GetLocaleForIndex(int i)
{
    if (i < 0 || i >= (int32)m_LocalForIndex.size())
    {
        return LOCALE_enUS;
    }

    return m_LocalForIndex[i];
}

/**
 * @brief Gets or creates the internal locale index for a locale constant.
 *
 * @param loc The locale constant.
 * @return The locale index, or -1 for the default locale.
 */
int ObjectMgr::GetOrNewIndexForLocale(LocaleConstant loc)
{
    if (loc == LOCALE_enUS)
    {
        return -1;
    }

    for (size_t i = 0; i < m_LocalForIndex.size(); ++i)
    {
        if (m_LocalForIndex[i] == loc)
        {
            return i;
        }
    }

    m_LocalForIndex.push_back(loc);
    return m_LocalForIndex.size() - 1;
}


/**
 * @brief Logs a formatted script text lookup error for a string entry.
 *
 * @param entry The string entry id.
 * @param text The printf-style error text.
 */
inline void _DoStringError(int32 entry, char const* text, ...)
{
    MANGOS_ASSERT(text);

    char buf[256];
    va_list ap;
    va_start(ap, text);
    vsnprintf(buf, 256, text, ap);
    va_end(ap);

    if (entry <= MAX_CREATURE_AI_TEXT_STRING_ID)            // script library error
    {
        sLog.outErrorScriptLib("%s", buf);
    }
    else if (entry <= MIN_CREATURE_AI_TEXT_STRING_ID)       // eventAI error
    {
        sLog.outErrorEventAI("%s", buf);
    }
    else if (entry < MIN_DB_SCRIPT_STRING_ID)               // mangos string error
    {
        sLog.outError("%s", buf);
    }
    else // if (entry > MIN_DB_SCRIPT_STRING_ID)            // DB script text error
    {
        sLog.outErrorDb("DB-SCRIPTS: %s", buf);
    }
}

/**
 * @brief Loads localized string templates from a database table.
 *
 * @param db The database to query.
 * @param table The source table name.
 * @param min_value The inclusive lower id bound.
 * @param max_value The exclusive upper id bound.
 * @param extra_content true to also load sound/chat metadata.
 * @return true if the load succeeded; otherwise, false.
 */
bool ObjectMgr::LoadMangosStrings(DatabaseType& db, char const* table, int32 min_value, int32 max_value, bool extra_content)
{
    int32 start_value = min_value;
    int32 end_value   = max_value;
    // some string can have negative indexes range
    if (start_value < 0)
    {
        if (end_value >= start_value)
        {
            sLog.outErrorDb("Table '%s' attempt loaded with invalid range (%d - %d), strings not loaded.", table, min_value, max_value);
            return false;
        }

        // real range (max+1,min+1) exaple: (-10,-1000) -> -999...-10+1
        std::swap(start_value, end_value);
        ++start_value;
        ++end_value;
    }
    else
    {
        if (start_value >= end_value)
        {
            sLog.outErrorDb("Table '%s' attempt loaded with invalid range (%d - %d), strings not loaded.", table, min_value, max_value);
            return false;
        }
    }

    // cleanup affected map part for reloading case
    for (MangosStringLocaleMap::iterator itr = mMangosStringLocaleMap.begin(); itr != mMangosStringLocaleMap.end();)
    {
        if (itr->first >= start_value && itr->first < end_value)
        {
            mMangosStringLocaleMap.erase(itr++);
        }
        else
        {
            ++itr;
        }
    }

    sLog.outString("Loading texts from %s%s", table, extra_content ? ", with additional data" : "");

    QueryResult* result = db.PQuery("SELECT `entry`,`content_default`,`content_loc1`,`content_loc2`,`content_loc3`,`content_loc4`,`content_loc5`,`content_loc6`,`content_loc7`,`content_loc8` %s FROM %s",
                                    extra_content ? ",sound,type,language,emote" : "", table);

    if (!result)
    {
        BarGoLink bar(1);

        bar.step();

        sLog.outString();
        if (min_value == MIN_MANGOS_STRING_ID)              // error only in case internal strings
        {
            sLog.outErrorDb(">> Loaded 0 mangos strings. DB table `%s` is empty. Can not continue.", table);
        }
        else
        {
            sLog.outString(">> Loaded 0 string templates. DB table `%s` is empty.", table);
        }
        return false;
    }

    uint32 count = 0;

    BarGoLink bar(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();
        bar.step();

        int32 entry = fields[0].GetInt32();

        if (entry == 0)
        {
            _DoStringError(start_value, "Table `%s` contain reserved entry 0, ignored.", table);
            continue;
        }
        else if (entry < start_value || entry >= end_value)
        {
            _DoStringError(start_value, "Table `%s` contain entry %i out of allowed range (%d - %d), ignored.", table, entry, min_value, max_value);
            continue;
        }

        MangosStringLocale& data = mMangosStringLocaleMap[entry];

        if (!data.Content.empty())
        {
            _DoStringError(entry, "Table `%s` contain data for already loaded entry  %i (from another table?), ignored.", table, entry);
            continue;
        }

        data.Content.resize(1);
        ++count;

        // 0 -> default, idx in to idx+1
        data.Content[0] = fields[1].GetCppString();

        for (int i = 1; i < MAX_LOCALE; ++i)
        {
            std::string str = fields[i + 1].GetCppString();
            if (!str.empty())
            {
                int idx = GetOrNewIndexForLocale(LocaleConstant(i));
                if (idx >= 0)
                {
                    // 0 -> default, idx in to idx+1
                    if ((int32)data.Content.size() <= idx + 1)
                    {
                        data.Content.resize(idx + 2);
                    }

                    data.Content[idx + 1] = str;
                }
            }
        }

        // Load additional string content if necessary
        if (extra_content)
        {
            data.SoundId     = fields[10].GetUInt32();
            data.Type        = fields[11].GetUInt32();
            data.LanguageId  = Language(fields[12].GetUInt32());
            data.Emote       = fields[13].GetUInt32();

            if (data.SoundId && !sSoundEntriesStore.LookupEntry(data.SoundId))
            {
                _DoStringError(entry, "Entry %i in table `%s` has soundId %u but sound does not exist.", entry, table, data.SoundId);
                data.SoundId = 0;
            }

            if (!GetLanguageDescByID(data.LanguageId))
            {
                _DoStringError(entry, "Entry %i in table `%s` using Language %u but Language does not exist.", entry, table, uint32(data.LanguageId));
                data.LanguageId = LANG_UNIVERSAL;
            }

            if (data.Type > CHAT_TYPE_ZONE_YELL)
            {
                _DoStringError(entry, "Entry %i in table `%s` has Type %u but this Chat Type does not exist.", entry, table, data.Type);
                data.Type = CHAT_TYPE_SAY;
            }

            if (data.Emote && !sEmotesStore.LookupEntry(data.Emote))
            {
                _DoStringError(entry, "Entry %i in table `%s` has Emote %u but emote does not exist.", entry, table, data.Emote);
                data.Emote = EMOTE_ONESHOT_NONE;
            }
        }
    }
    while (result->NextRow());

    delete result;

    if (min_value == MIN_MANGOS_STRING_ID)
    {
        sLog.outString(">> Loaded %u MaNGOS strings from table %s", count, table);
    }
    else
    {
        sLog.outString(">> Loaded %u %s templates from %s", count, extra_content ? "text" : "string", table);
    }
    sLog.outString();

    m_loadedStringCount[min_value] = count;

    return true;
}

/**
 * @brief Gets a localized MaNGOS string entry.
 *
 * @param entry The string entry id.
 * @param locale_idx The internal locale index.
 * @return The localized text, or a fallback error string.
 */
const char* ObjectMgr::GetMangosString(int32 entry, int locale_idx) const
{
    // locale_idx==-1 -> default, locale_idx >= 0 in to idx+1
    // Content[0] always exist if exist MangosStringLocale
    if (MangosStringLocale const* msl = GetMangosStringLocale(entry))
    {
        if ((int32)msl->Content.size() > locale_idx + 1 && !msl->Content[locale_idx + 1].empty())
        {
            return msl->Content[locale_idx + 1].c_str();
        }
        else
        {
            return msl->Content[0].c_str();
        }
    }

    _DoStringError(entry, "Entry %i not found but requested", entry);

    return "<error>";
}


// Check if a player meets condition conditionId
bool ObjectMgr::IsPlayerMeetToCondition(uint16 conditionId, Player const* pPlayer, Map const* map, WorldObject const* source, ConditionSource conditionSourceType) const
{
    if (const PlayerCondition* condition = sConditionStorage.LookupEntry<PlayerCondition>(conditionId))
    {
        return condition->Meets(pPlayer, map, source, conditionSourceType);
    }

    return false;
}

bool ObjectMgr::CheckDeclinedNames(std::wstring mainpart, DeclinedName const& names)
{
    for (int i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
    {
        std::wstring wname;
        if (!Utf8toWStr(names.name[i], wname))
        {
            return false;
        }

        if (mainpart != GetMainPartOfName(wname, i + 1))
        {
            return false;
        }
    }
    return true;
}

// Attention: make sure to keep this list in sync with ConditionSource to avoid array
//            out of bounds access! It is accessed with ConditionSource as index!
char const* conditionSourceToStr[] =
{
    "loot system",
    "referencing loot",
    "gossip menu",
    "gossip menu option",
    "event AI",
    "hardcoded",
    "vendor's item check",
    "spell_area check",
    "npc_spellclick_spells check", // Unused. For 3.x and later.
    "DBScript engine"
};

// Checks if player meets the condition
bool PlayerCondition::Meets(Player const* player, Map const* map, WorldObject const* source, ConditionSource conditionSourceType) const
{
    DEBUG_LOG("Condition-System: Check condition %u, type %i - called from %s with params plr: %s, map %i, src %s",
              m_entry, m_condition, conditionSourceToStr[conditionSourceType], player ? player->GetGuidStr().c_str() : "<NULL>", map ? map->GetId() : -1, source ? source->GetGuidStr().c_str() : "<NULL>");

    if (!CheckParamRequirements(player, map, source, conditionSourceType))
    {
        return false;
    }

    switch (m_condition)
    {
        case CONDITION_NOT:
            // Checked on load
            return !sConditionStorage.LookupEntry<PlayerCondition>(m_value1)->Meets(player, map, source, conditionSourceType);
        case CONDITION_OR:
            // Checked on load
            return sConditionStorage.LookupEntry<PlayerCondition>(m_value1)->Meets(player, map, source, conditionSourceType) || sConditionStorage.LookupEntry<PlayerCondition>(m_value2)->Meets(player, map, source, conditionSourceType);
        case CONDITION_AND:
            // Checked on load
            return sConditionStorage.LookupEntry<PlayerCondition>(m_value1)->Meets(player, map, source, conditionSourceType) && sConditionStorage.LookupEntry<PlayerCondition>(m_value2)->Meets(player, map, source, conditionSourceType);
        case CONDITION_NONE:
            return true;                                    // empty condition, always met
        case CONDITION_AURA:
            return player->HasAura(m_value1, SpellEffectIndex(m_value2));
        case CONDITION_ITEM:
            return player->HasItemCount(m_value1, m_value2);
        case CONDITION_ITEM_EQUIPPED:
            return player->HasItemOrGemWithIdEquipped(m_value1, 1);
        case CONDITION_AREAID:
        {
            uint32 zone, area;
            WorldObject const* searcher = source ? source : player;
            searcher->GetZoneAndAreaId(zone, area);
            return (zone == m_value1 || area == m_value1) == (m_value2 == 0);
        }
        case CONDITION_REPUTATION_RANK_MIN:
        {
            FactionEntry const* faction = sFactionStore.LookupEntry(m_value1);
            return faction && player->GetReputationMgr().GetRank(faction) >= ReputationRank(m_value2);
        }
        case CONDITION_TEAM:
        {
            if (conditionSourceType == CONDITION_FROM_REFERING_LOOT && sWorld.getConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_INTERACTION_AUCTION))
            {
                return true;
            }
            else
            {
                return uint32(player->GetTeam()) == m_value1;
            }
        }
        case CONDITION_SKILL:
            return player->HasSkill(m_value1) && player->GetBaseSkillValue(m_value1) >= m_value2;
        case CONDITION_QUESTREWARDED:
            return player->GetQuestRewardStatus(m_value1);
        case CONDITION_QUESTTAKEN:
            return player->IsCurrentQuest(m_value1, m_value2);
        case CONDITION_AD_COMMISSION_AURA:
        {
            Unit::SpellAuraHolderMap const& auras = player->GetSpellAuraHolderMap();
            for (Unit::SpellAuraHolderMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
            {
                if ((itr->second->GetSpellProto()->HasAttribute(SPELL_ATTR_CASTABLE_WHILE_MOUNTED) || itr->second->GetSpellProto()->HasAttribute(SPELL_ATTR_ABILITY)) && itr->second->GetSpellProto()->SpellVisual[0] == 3580)
                {
                    return true;
                }
            }
            return false;
        }
        case CONDITION_NO_AURA:
            return !player->HasAura(m_value1, SpellEffectIndex(m_value2));
        case CONDITION_ACTIVE_GAME_EVENT:
            return sGameEventMgr.IsActiveEvent(m_value1);
        case CONDITION_AREA_FLAG:
        {
            WorldObject const* searcher = source ? source : player;
            if (AreaTableEntry const* pAreaEntry = GetAreaEntryByAreaID(searcher->GetAreaId()))
            {
                if ((!m_value1 || (pAreaEntry->flags & m_value1)) && (!m_value2 || !(pAreaEntry->flags & m_value2)))
                {
                    return true;
                }
            }
            return false;
        }
        case CONDITION_RACE_CLASS:
            if ((!m_value1 || (player->getRaceMask() & m_value1)) && (!m_value2 || (player->getClassMask() & m_value2)))
            {
                return true;
            }
            return false;
        case CONDITION_LEVEL:
        {
            switch (m_value2)
            {
                case 0: return player->getLevel() == m_value1;
                case 1: return player->getLevel() >= m_value1;
                case 2: return player->getLevel() <= m_value1;
            }
            return false;
        }
        case CONDITION_NOITEM:
            return !player->HasItemCount(m_value1, m_value2);
        case CONDITION_SPELL:
        {
            switch (m_value2)
            {
                case 0: return player->HasSpell(m_value1);
                case 1: return !player->HasSpell(m_value1);
            }
            return false;
        }
        case CONDITION_INSTANCE_SCRIPT:
        {
            if (!map)
            {
                map = player ? player->GetMap() : source->GetMap();
            }

            if (InstanceData* data = map->GetInstanceData())
            {
                return data->CheckConditionCriteriaMeet(player, m_value1, source, conditionSourceType);
            }
            return false;
        }
        case CONDITION_QUESTAVAILABLE:
        {
            return player->CanTakeQuest(sObjectMgr.GetQuestTemplate(m_value1), false);
        }
        case CONDITION_ACHIEVEMENT:
        {
            switch (m_value2)
            {
                case 0: return player->GetAchievementMgr().HasAchievement(m_value1);
                case 1: return !player->GetAchievementMgr().HasAchievement(m_value1);
            }
            return false;
        }
        case CONDITION_ACHIEVEMENT_REALM:
        {
            AchievementEntry const* ach = sAchievementStore.LookupEntry(m_value1);
            switch (m_value2)
            {
                case 0: return sAchievementMgr.IsRealmCompleted(ach);
                case 1: return !sAchievementMgr.IsRealmCompleted(ach);
            }
            return false;
        }
        case CONDITION_QUEST_NONE:
        {
            if (!player->IsCurrentQuest(m_value1) && !player->GetQuestRewardStatus(m_value1))
            {
                return true;
            }
            return false;
        }
        case CONDITION_ITEM_WITH_BANK:
            return player->HasItemCount(m_value1, m_value2, true);
        case CONDITION_NOITEM_WITH_BANK:
            return !player->HasItemCount(m_value1, m_value2, true);
        case CONDITION_NOT_ACTIVE_GAME_EVENT:
            return !sGameEventMgr.IsActiveEvent(m_value1);
        case CONDITION_ACTIVE_HOLIDAY:
            return sGameEventMgr.IsActiveHoliday(HolidayIds(m_value1));
        case CONDITION_NOT_ACTIVE_HOLIDAY:
            return !sGameEventMgr.IsActiveHoliday(HolidayIds(m_value1));
        case CONDITION_LEARNABLE_ABILITY:
        {
            // Already know the spell
            if (player->HasSpell(m_value1))
            {
                return false;
            }

            // If item defined, check if player has the item already.
            if (m_value2)
            {
                // Hard coded item count. This should be ok, since the intention with this condition is to have
                // a all-in-one check regarding items that learn some ability (primary/secondary tradeskills).
                // Commonly, items like this is unique and/or are not expected to be obtained more than once.
                if (player->HasItemCount(m_value2, 1, true))
                {
                    return false;
                }
            }

            bool isSkillOk = false;

            SkillLineAbilityMapBounds bounds = sSpellMgr.GetSkillLineAbilityMapBounds(m_value1);

            for (SkillLineAbilityMap::const_iterator itr = bounds.first; itr != bounds.second; ++itr)
            {
                const SkillLineAbilityEntry* skillInfo = itr->second;

                if (!skillInfo)
                {
                    continue;
                }

                // doesn't have skill
                if (!player->HasSkill(skillInfo->skillId))
                {
                    return false;
                }

                // doesn't match class
                if (skillInfo->classmask && (skillInfo->classmask & player->getClassMask()) == 0)
                {
                    return false;
                }

                // doesn't match race
                if (skillInfo->racemask && (skillInfo->racemask & player->getRaceMask()) == 0)
                {
                    return false;
                }

                // skill level too low
                if (skillInfo->min_value > player->GetSkillValue(skillInfo->skillId))
                {
                    return false;
                }

                isSkillOk = true;
                break;
            }

            if (isSkillOk)
            {
                return true;
            }

            return false;
        }
        case CONDITION_SKILL_BELOW:
        {
            if (m_value2 == 1)
            {
                return !player->HasSkill(m_value1);
            }
            else
            {
                return player->HasSkill(m_value1) && player->GetBaseSkillValue(m_value1) < m_value2;
            }
        }
        case CONDITION_REPUTATION_RANK_MAX:
        {
            FactionEntry const* faction = sFactionStore.LookupEntry(m_value1);
            return faction && player->GetReputationMgr().GetRank(faction) <= ReputationRank(m_value2);
        }
        case CONDITION_COMPLETED_ENCOUNTER:
        {
            if (!map)
            {
                map = player ? player->GetMap() : source->GetMap();
            }
            if (!map->IsDungeon())
            {
                sLog.outErrorDb("CONDITION_COMPLETED_ENCOUNTER (entry %u) is used outside of a dungeon (on Map %u) by %s", m_entry, player->GetMapId(), player->GetGuidStr().c_str());
                return false;
            }

            uint32 completedEncounterMask = ((DungeonMap*)map)->GetPersistanceState()->GetCompletedEncountersMask();
            DungeonEncounterEntry const* dbcEntry1 = sDungeonEncounterStore.LookupEntry(m_value1);
            DungeonEncounterEntry const* dbcEntry2 = sDungeonEncounterStore.LookupEntry(m_value2);
            // Check that on proper map
            if (dbcEntry1->mapId != map->GetId())
            {
                sLog.outErrorDb("CONDITION_COMPLETED_ENCOUNTER (entry %u, DungeonEncounterEntry %u) is used on wrong map (used on Map %u) by %s", m_entry, m_value1, player->GetMapId(), player->GetGuidStr().c_str());
                return false;
            }
            // Select matching difficulties
            if (map->GetDifficulty() != Difficulty(dbcEntry1->Difficulty))
            {
                dbcEntry1 = NULL;
            }
            if (dbcEntry2 && map->GetDifficulty() != Difficulty(dbcEntry2->Difficulty))
            {
                dbcEntry2 = NULL;
            }

            return completedEncounterMask & ((dbcEntry1 ? 1 << dbcEntry1->encounterIndex : 0) | (dbcEntry2 ? 1 << dbcEntry2->encounterIndex : 0));
        }
        case CONDITION_SOURCE_AURA:
        {
            if (!source->isType(TYPEMASK_UNIT))
            {
                sLog.outErrorDb("CONDITION_SOURCE_AURA (entry %u) is used for non unit source (source %s) by %s", m_entry, source->GetGuidStr().c_str(), player->GetGuidStr().c_str());
                return false;
            }
            return ((Unit*)source)->HasAura(m_value1, SpellEffectIndex(m_value2));
        }
        case CONDITION_LAST_WAYPOINT:
        {
            if (source->GetTypeId() != TYPEID_UNIT)
            {
                sLog.outErrorDb("CONDITION_LAST_WAYPOINT (entry %u) is used for non creature source (source %s) by %s", m_entry, source->GetGuidStr().c_str(), player->GetGuidStr().c_str());
                return false;
            }
            uint32 lastReachedWp = ((Creature*)source)->GetMotionMaster()->getLastReachedWaypoint();
            switch (m_value2)
            {
                case 0: return m_value1 == lastReachedWp;
                case 1: return m_value1 <= lastReachedWp;
                case 2: return m_value1 > lastReachedWp;
            }
            return false;
        }
        case CONDITION_XP_USER:
        {
            switch (m_value1)
            {
                case 0: return player->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_XP_USER_DISABLED);
                case 1: return !player->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_XP_USER_DISABLED);
            }
            return false;
        }
        case CONDITION_GENDER:
            return player->getGender() == m_value1;
        case CONDITION_DEAD_OR_AWAY:
            switch (m_value1)
            {
                case 0:                                     // Player dead or out of range
                    return !player || !player->IsAlive() || (m_value2 && source && !source->IsWithinDistInMap(player, m_value2));
                case 1:                                     // All players in Group dead or out of range
                    if (!player)
                    {
                        return true;
                    }
                    if (Group const* grp = player->GetGroup())
                    {
                        for (GroupReference const* itr = grp->GetFirstMember(); itr != NULL; itr = itr->next())
                        {
                            Player const* pl = itr->getSource();
                            if (pl && pl->IsAlive() && !pl->isGameMaster() && (!m_value2 || !source || source->IsWithinDistInMap(pl, m_value2)))
                            {
                                return false;
                            }
                        }
                        return true;
                    }
                    else
                    {
                        return !player->IsAlive() || (m_value2 && source && !source->IsWithinDistInMap(player, m_value2));
                    }
                case 2:                                     // All players in instance dead or out of range
                    for (Map::PlayerList::const_iterator itr = map->GetPlayers().begin(); itr != map->GetPlayers().end(); ++itr)
                    {
                        Player const* plr = itr->getSource();
                        if (plr && plr->IsAlive() && !plr->isGameMaster() && (!m_value2 || !source || source->IsWithinDistInMap(plr, m_value2)))
                        {
                            return false;
                        }
                    }
                    return true;
                case 3:                                     // Creature source is dead
                    return !source || source->GetTypeId() != TYPEID_UNIT || !((Unit*)source)->IsAlive();
            }
        case CONDITION_CREATURE_IN_RANGE:
        {
            Creature* creature = NULL;

            MaNGOS::NearestCreatureEntryWithLiveStateInObjectRangeCheck creature_check(*player, m_value1, true, false, m_value2, true);
            MaNGOS::CreatureLastSearcher<MaNGOS::NearestCreatureEntryWithLiveStateInObjectRangeCheck> searcher(creature, creature_check);
            Cell::VisitGridObjects(player, searcher, m_value2);

            return creature;
        }
        case CONDITION_GAMEOBJECT_IN_RANGE:
        {
            GameObject* pGo = NULL;

            if (source)
            {
                MaNGOS::NearestGameObjectEntryInObjectRangeCheck go_check(*source, m_value1, m_value2);
                MaNGOS::GameObjectLastSearcher<MaNGOS::NearestGameObjectEntryInObjectRangeCheck> searcher(pGo, go_check);

                Cell::VisitGridObjects(source, searcher, m_value2);
            }
            return pGo;
        }
        default:
            return false;
    }
}

// Which params must be provided to a Condition
bool PlayerCondition::CheckParamRequirements(Player const* pPlayer, Map const* map, WorldObject const* source, ConditionSource conditionSourceType) const
{
    switch (m_condition)
    {
        case CONDITION_NOT:
        case CONDITION_AND:
        case CONDITION_OR:
        case CONDITION_NONE:
        case CONDITION_ACTIVE_GAME_EVENT:
        case CONDITION_ACHIEVEMENT_REALM:
        case CONDITION_NOT_ACTIVE_GAME_EVENT:
        case CONDITION_ACTIVE_HOLIDAY:
        case CONDITION_NOT_ACTIVE_HOLIDAY:
            break;
        case CONDITION_AREAID:
        case CONDITION_AREA_FLAG:
            if (!pPlayer && !source)
            {
                sLog.outErrorDb("CONDITION %u type %u used with bad parameters, called from %s, used with plr: %s, map %i, src %s",
                                m_entry, m_condition, conditionSourceToStr[conditionSourceType], pPlayer ? pPlayer->GetGuidStr().c_str() : "NULL", map ? map->GetId() : -1, source ? source->GetGuidStr().c_str() : "NULL");
                return false;
            }
            break;
        case CONDITION_INSTANCE_SCRIPT:
        case CONDITION_COMPLETED_ENCOUNTER:
            if (!pPlayer && !source && !map)
            {
                sLog.outErrorDb("CONDITION %u type %u used with bad parameters, called from %s, used with plr: %s, map %i, src %s",
                                m_entry, m_condition, conditionSourceToStr[conditionSourceType], pPlayer ? pPlayer->GetGuidStr().c_str() : "NULL", map ? map->GetId() : -1, source ? source->GetGuidStr().c_str() : "NULL");
                return false;
            }
            break;
        case CONDITION_SOURCE_AURA:
        case CONDITION_LAST_WAYPOINT:
            if (!source)
            {
                sLog.outErrorDb("CONDITION %u type %u used with bad parameters, called from %s, used with plr: %s, map %i, src %s",
                                m_entry, m_condition, conditionSourceToStr[conditionSourceType], pPlayer ? pPlayer->GetGuidStr().c_str() : "NULL", map ? map->GetId() : -1, source ? source->GetGuidStr().c_str() : "NULL");
                return false;
            }
            break;
        case CONDITION_DEAD_OR_AWAY:
            switch (m_value1)
            {
                case 0:                                     // Player dead or out of range
                case 1:                                     // All players in Group dead or out of range
                case 2:                                     // All players in instance dead or out of range
                    if (m_value2 && !source)
                    {
                        sLog.outErrorDb("CONDITION_DEAD_OR_AWAY %u - called from %s without source, but source expected for range check", m_entry, conditionSourceToStr[conditionSourceType]);
                        return false;
                    }
                    if (m_value1 != 2)
                    {
                        return true;
                    }
                    // Case 2 (Instance map only)
                    if (!map && (pPlayer || source))
                    {
                        map = source ? source->GetMap() : pPlayer->GetMap();
                    }
                    if (!map || !map->Instanceable())
                    {
                        sLog.outErrorDb("CONDITION_DEAD_OR_AWAY %u (Player in instance case) - called from %s without map param or from non-instanceable map %i", m_entry,  conditionSourceToStr[conditionSourceType], map ? map->GetId() : -1);
                        return false;
                    }
                case 3:                                     // Creature source is dead
                    return true;
            }
            break;
        default:
            if (!pPlayer)
            {
                sLog.outErrorDb("CONDITION %u type %u used with bad parameters, called from %s, used with plr: %s, map %i, src %s",
                                m_entry, m_condition, conditionSourceToStr[conditionSourceType], pPlayer ? pPlayer->GetGuidStr().c_str() : "NULL", map ? map->GetId() : -1, source ? source->GetGuidStr().c_str() : "NULL");
                return false;
            }
            break;
    }
    return true;
}

// Verification of condition values validity
bool PlayerCondition::IsValid(uint16 entry, ConditionType condition, uint32 value1, uint32 value2)
{
    switch (condition)
    {
        case CONDITION_NOT:
        {
            if (value1 >= entry)
            {
                sLog.outErrorDb("CONDITION_NOT (entry %u, type %d) has invalid value1 %u, must be lower than entry, skipped", entry, condition, value1);
                return false;
            }
            const PlayerCondition* condition1 = sConditionStorage.LookupEntry<PlayerCondition>(value1);
            if (!condition1)
            {
                sLog.outErrorDb("CONDITION_NOT (entry %u, type %d) has value1 %u without proper condition, skipped", entry, condition, value1);
                return false;
            }
            break;
        }
        case CONDITION_OR:
        case CONDITION_AND:
        {
            if (value1 >= entry)
            {
                sLog.outErrorDb("CONDITION _AND or _OR (entry %u, type %d) has invalid value1 %u, must be lower than entry, skipped", entry, condition, value1);
                return false;
            }
            if (value2 >= entry)
            {
                sLog.outErrorDb("CONDITION _AND or _OR (entry %u, type %d) has invalid value2 %u, must be lower than entry, skipped", entry, condition, value2);
                return false;
            }
            const PlayerCondition* condition1 = sConditionStorage.LookupEntry<PlayerCondition>(value1);
            if (!condition1)
            {
                sLog.outErrorDb("CONDITION _AND or _OR (entry %u, type %d) has value1 %u without proper condition, skipped", entry, condition, value1);
                return false;
            }
            const PlayerCondition* condition2 = sConditionStorage.LookupEntry<PlayerCondition>(value2);
            if (!condition2)
            {
                sLog.outErrorDb("CONDITION _AND or _OR (entry %u, type %d) has value2 %u without proper condition, skipped", entry, condition, value2);
                return false;
            }
            break;
        }
        case CONDITION_AURA:
        case CONDITION_SOURCE_AURA:
        {
            if (!sSpellStore.LookupEntry(value1))
            {
                sLog.outErrorDb("Aura condition (entry %u, type %u) requires to have non existing spell (Id: %d), skipped", entry, condition, value1);
                return false;
            }
            if (value2 >= MAX_EFFECT_INDEX)
            {
                sLog.outErrorDb("Aura condition (entry %u, type %u) requires to have non existing effect index (%u) (must be 0..%u), skipped", entry, condition, value2, MAX_EFFECT_INDEX - 1);
                return false;
            }
            break;
        }
        case CONDITION_ITEM:
        case CONDITION_NOITEM:
        case CONDITION_ITEM_WITH_BANK:
        case CONDITION_NOITEM_WITH_BANK:
        {
            ItemPrototype const* proto = ObjectMgr::GetItemPrototype(value1);
            if (!proto)
            {
                sLog.outErrorDb("Item condition (entry %u, type %u) requires to have non existing item (%u), skipped", entry, condition, value1);
                return false;
            }

            if (value2 < 1)
            {
                sLog.outErrorDb("Item condition (entry %u, type %u) useless with count < 1, skipped", entry, condition);
                return false;
            }
            break;
        }
        case CONDITION_ITEM_EQUIPPED:
        {
            ItemPrototype const* proto = ObjectMgr::GetItemPrototype(value1);
            if (!proto)
            {
                sLog.outErrorDb("ItemEquipped condition (entry %u, type %u) requires to have non existing item (%u) equipped, skipped", entry, condition, value1);
                return false;
            }
            break;
        }
        case CONDITION_AREAID:
        {
            AreaTableEntry const* areaEntry = GetAreaEntryByAreaID(value1);
            if (!areaEntry)
            {
                sLog.outErrorDb("Zone condition (entry %u, type %u) requires to be in non existing area (%u), skipped", entry, condition, value1);
                return false;
            }

            if (value2 > 1)
            {
                sLog.outErrorDb("Zone condition (entry %u, type %u) has invalid argument %u (must be 0..1), skipped", entry, condition, value2);
                return false;
            }
            break;
        }
        case CONDITION_REPUTATION_RANK_MIN:
        case CONDITION_REPUTATION_RANK_MAX:
        {
            FactionEntry const* factionEntry = sFactionStore.LookupEntry(value1);
            if (!factionEntry)
            {
                sLog.outErrorDb("Reputation condition (entry %u, type %u) requires to have reputation non existing faction (%u), skipped", entry, condition, value1);
                return false;
            }

            if (value2 >= MAX_REPUTATION_RANK)
            {
                sLog.outErrorDb("Reputation condition (entry %u, type %u) has invalid rank requirement (value2 = %u) - must be between %u and %u, skipped", entry, condition, value2, MIN_REPUTATION_RANK, MAX_REPUTATION_RANK - 1);
                return false;
            }
            break;
        }
        case CONDITION_TEAM:
        {
            if (value1 != ALLIANCE && value1 != HORDE)
            {
                sLog.outErrorDb("Team condition (entry %u, type %u) specifies unknown team (%u), skipped", entry, condition, value1);
                return false;
            }
            break;
        }
        case CONDITION_SKILL:
        case CONDITION_SKILL_BELOW:
        {
            SkillLineEntry const* pSkill = sSkillLineStore.LookupEntry(value1);
            if (!pSkill)
            {
                sLog.outErrorDb("Skill condition (entry %u, type %u) specifies non-existing skill (%u), skipped", entry, condition, value1);
                return false;
            }
            if (value2 < 1 || value2 > sWorld.GetConfigMaxSkillValue())
            {
                sLog.outErrorDb("Skill condition (entry %u, type %u) specifies invalid skill value (%u), skipped", entry, condition, value2);
                return false;
            }
            break;
        }
        case CONDITION_QUESTREWARDED:
        case CONDITION_QUESTTAKEN:
        case CONDITION_QUESTAVAILABLE:
        case CONDITION_QUEST_NONE:
        {
            Quest const* Quest = sObjectMgr.GetQuestTemplate(value1);
            if (!Quest)
            {
                sLog.outErrorDb("Quest condition (entry %u, type %u) specifies non-existing quest (%u), skipped", entry, condition, value1);
                return false;
            }

            if (value2 && condition != CONDITION_QUESTTAKEN)
            {
                sLog.outErrorDb("Quest condition (entry %u, type %u) has useless data in value2 (%u)!", entry, condition, value2);
            }
            break;
        }
        case CONDITION_AD_COMMISSION_AURA:
        {
            if (value1)
            {
                sLog.outErrorDb("Quest condition (entry %u, type %u) has useless data in value1 (%u)!", entry, condition, value1);
            }
            if (value2)
            {
                sLog.outErrorDb("Quest condition (entry %u, type %u) has useless data in value2 (%u)!", entry, condition, value2);
            }
            break;
        }
        case CONDITION_NO_AURA:
        {
            if (!sSpellStore.LookupEntry(value1))
            {
                sLog.outErrorDb("Aura condition (entry %u, type %u) requires to have non existing spell (Id: %d), skipped", entry, condition, value1);
                return false;
            }
            if (value2 > MAX_EFFECT_INDEX)
            {
                sLog.outErrorDb("Aura condition (entry %u, type %u) requires to have non existing effect index (%u) (must be 0..%u), skipped", entry, condition, value2, MAX_EFFECT_INDEX - 1);
                return false;
            }
            break;
        }
        case CONDITION_ACTIVE_GAME_EVENT:
        case CONDITION_NOT_ACTIVE_GAME_EVENT:
        {
            if (!sGameEventMgr.IsValidEvent(value1))
            {
                sLog.outErrorDb("(Not)Active event condition (entry %u, type %u) requires existing event id (%u), skipped", entry, condition, value1);
                return false;
            }
            break;
        }
        case CONDITION_AREA_FLAG:
        {
            if (!value1 && !value2)
            {
                sLog.outErrorDb("Area flag condition (entry %u, type %u) has both values like 0, skipped", entry, condition);
                return false;
            }
            break;
        }
        case CONDITION_RACE_CLASS:
        {
            if (!value1 && !value2)
            {
                sLog.outErrorDb("Race_class condition (entry %u, type %u) has both values like 0, skipped", entry, condition);
                return false;
            }

            if (value1 && !(value1 & RACEMASK_ALL_PLAYABLE))
            {
                sLog.outErrorDb("Race_class condition (entry %u, type %u) has invalid player class %u, skipped", entry, condition, value1);
                return false;
            }

            if (value2 && !(value2 & CLASSMASK_ALL_PLAYABLE))
            {
                sLog.outErrorDb("Race_class condition (entry %u, type %u) has invalid race mask %u, skipped", entry, condition, value2);
                return false;
            }
            break;
        }
        case CONDITION_LEVEL:
        {
            if (!value1 || value1 > sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL))
            {
                sLog.outErrorDb("Level condition (entry %u, type %u)has invalid level %u, skipped", entry, condition, value1);
                return false;
            }

            if (value2 > 2)
            {
                sLog.outErrorDb("Level condition (entry %u, type %u) has invalid argument %u (must be 0..2), skipped", entry, condition, value2);
                return false;
            }

            break;
        }
        case CONDITION_SPELL:
        {
            if (!sSpellStore.LookupEntry(value1))
            {
                sLog.outErrorDb("Spell condition (entry %u, type %u) requires to have non existing spell (Id: %d), skipped", entry, condition, value1);
                return false;
            }

            if (value2 > 1)
            {
                sLog.outErrorDb("Spell condition (entry %u, type %u) has invalid argument %u (must be 0..1), skipped", entry, condition, value2);
                return false;
            }

            break;
        }
        case CONDITION_INSTANCE_SCRIPT:
            break;
        case CONDITION_ACHIEVEMENT:
        case CONDITION_ACHIEVEMENT_REALM:
        {
            if (!sAchievementStore.LookupEntry(value1))
            {
                sLog.outErrorDb("Achievement condition (entry %u, type %u) requires to have non existing achievement (Id: %d), skipped", entry, condition, value1);
                return false;
            }

            if (value2 > 1)
            {
                sLog.outErrorDb("Achievement condition (entry %u, type %u) has invalid argument %u (must be 0..1), skipped", entry, condition, value2);
                return false;
            }

            break;
        }
        case CONDITION_ACTIVE_HOLIDAY:
        case CONDITION_NOT_ACTIVE_HOLIDAY:
        {
            if (!sHolidaysStore.LookupEntry(value1))
            {
                sLog.outErrorDb("(Not)Active holiday condition (entry %u, type %u) requires existing holiday id (%u), skipped", entry, condition, value1);
                return false;
            }
            break;
        }
        case CONDITION_LEARNABLE_ABILITY:
        {
            SkillLineAbilityMapBounds bounds = sSpellMgr.GetSkillLineAbilityMapBounds(value1);

            if (bounds.first == bounds.second)
            {
                sLog.outErrorDb("Learnable ability condition (entry %u, type %u) has spell id %u defined, but this spell is not listed in SkillLineAbility and can not be used, skipping.", entry, condition, value1);
                return false;
            }

            if (value2)
            {
                ItemPrototype const* proto = ObjectMgr::GetItemPrototype(value2);
                if (!proto)
                {
                    sLog.outErrorDb("Learnable ability condition (entry %u, type %u) has item entry %u defined but item does not exist, skipping.", entry, condition, value2);
                    return false;
                }
            }

            break;
        }
        case CONDITION_COMPLETED_ENCOUNTER:
        {
            DungeonEncounterEntry const* dbcEntry1 = sDungeonEncounterStore.LookupEntry(value1);
            DungeonEncounterEntry const* dbcEntry2 = sDungeonEncounterStore.LookupEntry(value2);
            if (!dbcEntry1)
            {
                sLog.outErrorDb("Completed Encounter condition (entry %u, type %u) has an unknown DungeonEncounter entry %u defined (in value1), skipping.", entry, condition, value1);
                return false;
            }
            if (value2 && !dbcEntry2)
            {
                sLog.outErrorDb("Completed Encounter condition (entry %u, type %u) has an unknown DungeonEncounter entry %u defined (in value2), skipping.", entry, condition, value2);
                return false;
            }
            if (dbcEntry2 && dbcEntry1->mapId != dbcEntry2->mapId)
            {
                sLog.outErrorDb("Completed Encounter condition (entry %u, type %u) has different mapIds for both encounters, skipping.", entry, condition);
                return false;
            }
            break;
        }
        case CONDITION_LAST_WAYPOINT:
        {
            if (value2 > 2)
            {
                sLog.outErrorDb("Last Waypoint condition (entry %u, type %u) has an invalid value in value2. (Has %u, supported 0, 1, or 2), skipping.", entry, condition, value2);
                return false;
            }
            break;
        }
        case CONDITION_XP_USER:
        {
            if (value1 > 1)
            {
                sLog.outErrorDb("XP user condition (entry %u, type %u) has invalid argument %u (must be 0..1), skipped", entry, condition, value1);
                return false;
            }

            if (value2)
            {
                sLog.outErrorDb("XP user condition (entry %u, type %u) has useless data in value2 (%u)!", entry, condition, value2);
            }

            break;
        }
        case CONDITION_GENDER:
        {
            if (value1 >= MAX_GENDER)
            {
                sLog.outErrorDb("Gender condition (entry %u, type %u) has an invalid value in value1. (Has %u, must be smaller than %u), skipping.", entry, condition, value1, MAX_GENDER);
                return false;
            }
            break;
        }
        case CONDITION_DEAD_OR_AWAY:
        {
            if (value1 >= 4)
            {
                sLog.outErrorDb("Dead condition (entry %u, type %u) has an invalid value in value1. (Has %u, must be smaller than 4), skipping.", entry, condition, value1);
                return false;
            }
            break;
        }
        case CONDITION_CREATURE_IN_RANGE:
        {
            if (!sCreatureStorage.LookupEntry<CreatureInfo> (value1))
            {
                sLog.outErrorDb("Creature in range condition (entry %u, type %u) has an invalid value in value1. (Creature %u does not exist in the database), skipping.", entry, condition, value1);
                return false;
            }
            if (value2 <= 0)
            {
                sLog.outErrorDb("Creature in range condition (entry %u, type %u) has an invalid value in value2. (Range %u must be greater than 0), skipping.", entry, condition, value2);
                return false;
            }
            break;
        }
        case CONDITION_GAMEOBJECT_IN_RANGE:
        {
            if (!sGOStorage.LookupEntry<GameObjectInfo>(value1))
            {
                sLog.outErrorDb("Game object in range condition (entry %u, type %u) has an invalid value in value1 (gameobject). (Game object %u does not exist in the database), skipping.", entry, condition, value1);
                return false;
            }
            if (value2 <= 0)
            {
                sLog.outErrorDb("Game object in range condition (entry %u, type %u) has an invalid value in value2 (range). (Range %u must be greater than 0), skipping.", entry, condition, value2);
                return false;
            }
            break;
        }
        case CONDITION_NONE:
            break;
        default:
            sLog.outErrorDb("Condition entry %u has bad type of %d, skipped ", entry, condition);
            return false;
    }
    return true;
}

// Check if a condition can be used without providing a player param
bool PlayerCondition::CanBeUsedWithoutPlayer(uint16 entry)
{
    PlayerCondition const* condition = sConditionStorage.LookupEntry<PlayerCondition>(entry);
    if (!condition)
    {
        return false;
    }

    switch (condition->m_condition)
    {
        case CONDITION_NOT:
            return CanBeUsedWithoutPlayer(condition->m_value1);
        case CONDITION_AND:
        case CONDITION_OR:
            return CanBeUsedWithoutPlayer(condition->m_value1) && CanBeUsedWithoutPlayer(condition->m_value2);
        case CONDITION_NONE:
        case CONDITION_ACTIVE_GAME_EVENT:
        case CONDITION_ACHIEVEMENT_REALM:
        case CONDITION_NOT_ACTIVE_GAME_EVENT:
        case CONDITION_ACTIVE_HOLIDAY:
        case CONDITION_NOT_ACTIVE_HOLIDAY:
        case CONDITION_AREAID:
        case CONDITION_AREA_FLAG:
        case CONDITION_INSTANCE_SCRIPT:
        case CONDITION_COMPLETED_ENCOUNTER:
        case CONDITION_SOURCE_AURA:
        case CONDITION_LAST_WAYPOINT:
            return true;
        default:
            return false;
    }
}

/**
 * @brief Determines the training range type used by a skill line.
 *
 * @param pSkill The skill line entry.
 * @param racial True if the skill is racial.
 * @return SkillRangeType The applicable skill range type.
 */
SkillRangeType GetSkillRangeType(SkillLineEntry const* pSkill, bool racial)
{
    switch (pSkill->categoryId)
    {
        case SKILL_CATEGORY_LANGUAGES:
            return SKILL_RANGE_LANGUAGE;
        case SKILL_CATEGORY_WEAPON:
            if (pSkill->id != SKILL_FIST_WEAPONS)
            {
                return SKILL_RANGE_LEVEL;
            }
            else
            {
                return SKILL_RANGE_MONO;
            }
        case SKILL_CATEGORY_ARMOR:
        case SKILL_CATEGORY_CLASS:
            if (pSkill->id != SKILL_LOCKPICKING)
            {
                return SKILL_RANGE_MONO;
            }
            else
            {
                return SKILL_RANGE_LEVEL;
            }
        case SKILL_CATEGORY_SECONDARY:
        case SKILL_CATEGORY_PROFESSION:
            // not set skills for professions and racial abilities
            if (IsProfessionSkill(pSkill->id))
            {
                return SKILL_RANGE_RANK;
            }
            else if (racial)
            {
                return SKILL_RANGE_NONE;
            }
            else
            {
                return SKILL_RANGE_MONO;
            }
        default:
        case SKILL_CATEGORY_ATTRIBUTES:                     // not found in dbc
        case SKILL_CATEGORY_GENERIC:                        // only GENERIC(DND)
            return SKILL_RANGE_NONE;
    }
}

void ObjectMgr::LoadMailLevelRewards()
{
    m_mailLevelRewardMap.clear();                           // for reload case

    uint32 count = 0;
    QueryResult* result = WorldDatabase.Query("SELECT `level`, `raceMask`, `mailTemplateId`, `senderEntry` FROM `mail_level_reward`");

    if (!result)
    {
        BarGoLink bar(1);

        bar.step();

        sLog.outString();
        sLog.outErrorDb(">> Loaded `mail_level_reward`, table is empty!");
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        bar.step();

        Field* fields = result->Fetch();

        uint8 level           = fields[0].GetUInt8();
        uint32 raceMask       = fields[1].GetUInt32();
        uint32 mailTemplateId = fields[2].GetUInt32();
        uint32 senderEntry    = fields[3].GetUInt32();

        if (level > MAX_LEVEL)
        {
            sLog.outErrorDb("Table `mail_level_reward` have data for level %u that more supported by client (%u), ignoring.", level, MAX_LEVEL);
            continue;
        }

        if (!(raceMask & RACEMASK_ALL_PLAYABLE))
        {
            sLog.outErrorDb("Table `mail_level_reward` have raceMask (%u) for level %u that not include any player races, ignoring.", raceMask, level);
            continue;
        }

        if (!sMailTemplateStore.LookupEntry(mailTemplateId))
        {
            sLog.outErrorDb("Table `mail_level_reward` have invalid mailTemplateId (%u) for level %u that invalid not include any player races, ignoring.", mailTemplateId, level);
            continue;
        }

        if (!GetCreatureTemplate(senderEntry))
        {
            sLog.outErrorDb("Table `mail_level_reward` have nonexistent sender creature entry (%u) for level %u that invalid not include any player races, ignoring.", senderEntry, level);
            continue;
        }

        m_mailLevelRewardMap[level].push_back(MailLevelReward(raceMask, mailTemplateId, senderEntry));

        ++count;
    }
    while (result->NextRow());
    delete result;

    sLog.outString();
    sLog.outString(">> Loaded %u level dependent mail rewards,", count);
}

/**
 * @brief Loads trainer spell data from a database table.
 *
 * @param tableName The source table name.
 * @param isTemplates true when loading trainer templates instead of direct trainer entries.
 */
/* This function is supposed to take care of three things:
 *  1) Load Transports on Map or on Continents
 *  2) Load Active Npcs on Map or Continents
 *  3) Load Everything dependend on config setting LoadAllGridsOnMaps
 *
 *  This function is currently WIP, hence parts exist only as draft.
 */
ObjectMgr::LivingWorldStartupStats ObjectMgr::LoadActiveEntities(Map* _map)
{
    // Special case on startup - load continents
    if (!_map)
    {
        s_livingWorldStats = ObjectMgr::LivingWorldStartupStats();
        s_livingWorldStartupPass = true;

        uint32 continents[] = {0, 1, 369, 530};
        for (int i = 0; i < countof(continents); ++i)
        {
            _map = sMapMgr.FindMap(continents[i]);
            if (!_map)
            {
                _map = sMapMgr.CreateMap(continents[i], NULL);
            }

            if (_map)
            {
                LoadActiveEntities(_map);
            }
            else
            {
                sLog.outError("ObjectMgr::LoadActiveEntities - Unable to create Map %u", continents[i]);
            }
        }

        s_livingWorldStartupPass = false;
        return s_livingWorldStats;
    }

    bool collectLivingWorldStats = s_livingWorldStartupPass;
    bool forceLoad = sWorld.isForceLoadMap(_map->GetId());

    uint32 mapTransportCount = 0;
    uint32 forceLoadRequests = 0;
    uint32 uniqueGridCount = 0;
    uint32 newlyLoaded = 0;
    uint32 alreadyLoaded = 0;
    uint32 activeCreatureGuids = 0;

    std::set<std::pair<uint32, uint32> > uniqueGrids;

    if (collectLivingWorldStats)
    {
        MapManager::TransportMap::const_iterator transportMapItr = sMapMgr.m_TransportsByMap.find(_map->GetId());
        if (transportMapItr != sMapMgr.m_TransportsByMap.end())
        {
            mapTransportCount = uint32(transportMapItr->second.size());
        }

        std::pair<ActiveCreatureGuidsOnMap::const_iterator, ActiveCreatureGuidsOnMap::const_iterator> activeBounds = m_activeCreatures.equal_range(_map->GetId());
        for (ActiveCreatureGuidsOnMap::const_iterator itr = activeBounds.first; itr != activeBounds.second; ++itr)
        {
            ++activeCreatureGuids;
        }
    }

    // Load active objects for _map
    if (forceLoad)
    {
        for (CreatureDataMap::const_iterator itr = mCreatureDataMap.begin(); itr != mCreatureDataMap.end(); ++itr)
        {
            if (itr->second.mapid == _map->GetId())
            {
                if (collectLivingWorldStats)
                {
                    ++forceLoadRequests;
                    GridPair gridPair = MaNGOS::ComputeGridPair(itr->second.posX, itr->second.posY);
                    uniqueGrids.insert(std::make_pair(gridPair.x_coord, gridPair.y_coord));

                    if (_map->IsLoaded(itr->second.posX, itr->second.posY))
                    {
                        ++alreadyLoaded;
                    }
                    else
                    {
                        ++newlyLoaded;
                    }
                }

                _map->ForceLoadGrid(itr->second.posX, itr->second.posY);
            }
        }
    }
    else                                                    // Normal case - Load all npcs that are active
    {
        std::pair<ActiveCreatureGuidsOnMap::const_iterator, ActiveCreatureGuidsOnMap::const_iterator> bounds = m_activeCreatures.equal_range(_map->GetId());
        for (ActiveCreatureGuidsOnMap::const_iterator itr = bounds.first; itr != bounds.second; ++itr)
        {
            CreatureData const& data = mCreatureDataMap[itr->second];
            {
                if (collectLivingWorldStats)
                {
                    ++forceLoadRequests;
                    GridPair gridPair = MaNGOS::ComputeGridPair(data.posX, data.posY);
                    uniqueGrids.insert(std::make_pair(gridPair.x_coord, gridPair.y_coord));

                    if (_map->IsLoaded(data.posX, data.posY))
                    {
                        ++alreadyLoaded;
                    }
                    else
                    {
                        ++newlyLoaded;
                    }
                }

                _map->ForceLoadGrid(data.posX, data.posY);
            }
        }
    }

    if (collectLivingWorldStats)
    {
        uniqueGridCount = uint32(uniqueGrids.size());

        s_livingWorldStats.totalUniqueGrids += uniqueGridCount;
        s_livingWorldStats.totalNewlyLoaded += newlyLoaded;
        s_livingWorldStats.totalMapTransports += mapTransportCount;
        if (forceLoad)
        {
            ++s_livingWorldStats.forcedMaps;
        }

        if (forceLoad)
        {
            sLog.outString("[LivingWorld] map %u: force-load=ON, creature-rows=%u, ForceLoadGrid-requests=%u, unique-grids=%u, newly-loaded=%u (explicit-locks-set), already-loaded=%u, extra-active-creatures=%u, map-transports=%u",
                           _map->GetId(), forceLoadRequests, forceLoadRequests, uniqueGridCount, newlyLoaded, alreadyLoaded, activeCreatureGuids, mapTransportCount);
        }
        else
        {
            sLog.outString("[LivingWorld] map %u: force-load=OFF, extra-active-creatures=%u, ForceLoadGrid-requests=%u, unique-grids=%u, newly-loaded=%u (explicit-locks-set), already-loaded=%u, map-transports=%u",
                           _map->GetId(), activeCreatureGuids, forceLoadRequests, uniqueGridCount, newlyLoaded, alreadyLoaded, mapTransportCount);
        }
    }

    return ObjectMgr::LivingWorldStartupStats();
}

void ObjectMgr::AddVendorItem(uint32 entry, uint32 item, uint32 maxcount, uint32 incrtime, uint32 extendedcost)
{
    VendorItemData& vList = m_mCacheVendorItemMap[entry];
    vList.AddItem(item, maxcount, incrtime, extendedcost, 0);

    WorldDatabase.PExecuteLog("INSERT INTO `npc_vendor` (`entry`,`item`,`maxcount`,`incrtime`,`extendedcost`) VALUES('%u','%u','%u','%u','%u')", entry, item, maxcount, incrtime, extendedcost);
}

/**
 * @brief Removes a vendor item from a creature vendor list and database.
 *
 * @param entry The vendor creature entry.
 * @param item The item entry.
 * @return true if the item was removed; otherwise, false.
 */
bool ObjectMgr::RemoveVendorItem(uint32 entry, uint32 item)
{
    CacheVendorItemMap::iterator  iter = m_mCacheVendorItemMap.find(entry);
    if (iter == m_mCacheVendorItemMap.end())
    {
        return false;
    }

    if (!iter->second.RemoveItem(item))
    {
        return false;
    }

    WorldDatabase.PExecuteLog("DELETE FROM `npc_vendor` WHERE `entry`='%u' AND `item`='%u'", entry, item);
    return true;
}

/**
 * @brief Validates a vendor item definition for a vendor or vendor template.
 *
 * @param isTemplate true when validating a vendor template.
 * @param tableName The source table name.
 * @param vendor_entry The vendor or template entry id.
 * @param item_id The item entry id.
 * @param maxcount The limited stock count.
 * @param incrtime The stock replenishment interval.
 * @param conditionId The optional condition id.
 * @param pl Optional player used for command feedback.
 * @param skip_vendors Optional set used to suppress repeated vendor errors.
 * @return true if the vendor item definition is valid; otherwise, false.
 */
bool ObjectMgr::IsVendorItemValid(bool isTemplate, char const* tableName, uint32 vendor_entry, uint32 item_id, uint32 maxcount, uint32 incrtime, uint32 ExtendedCost, uint16 conditionId, Player* pl, std::set<uint32>* skip_vendors) const
{
    char const* idStr = isTemplate ? "vendor template" : "vendor";
    CreatureInfo const* cInfo = NULL;

    if (!isTemplate)
    {
        cInfo = GetCreatureTemplate(vendor_entry);
        if (!cInfo)
        {
            if (pl)
            {
                ChatHandler(pl).SendSysMessage(LANG_COMMAND_VENDORSELECTION);
            }
            else
            {
                sLog.outErrorDb("Table `%s` has data for nonexistent creature (Entry: %u), ignoring", tableName, vendor_entry);
            }
            return false;
        }

        if (!(cInfo->NpcFlags & UNIT_NPC_FLAG_VENDOR))
        {
            if (!skip_vendors || skip_vendors->count(vendor_entry) == 0)
            {
                if (pl)
                {
                    ChatHandler(pl).SendSysMessage(LANG_COMMAND_VENDORSELECTION);
                }
                else
                {
                    sLog.outErrorDb("Table `%s` has data for creature (Entry: %u) without vendor flag, ignoring", tableName, vendor_entry);
                }

                if (skip_vendors)
                {
                    skip_vendors->insert(vendor_entry);
                }
            }
            return false;
        }
    }

    if (!GetItemPrototype(item_id))
    {
        if (pl)
        {
            ChatHandler(pl).PSendSysMessage(LANG_ITEM_NOT_FOUND, item_id);
        }
        else
            sLog.outErrorDb("Table `%s` for %s %u contain nonexistent item (%u), ignoring",
                            tableName, idStr, vendor_entry, item_id);
        return false;
    }

    if (ExtendedCost && !sItemExtendedCostStore.LookupEntry(ExtendedCost))
    {
        if (pl)
        {
            ChatHandler(pl).PSendSysMessage(LANG_EXTENDED_COST_NOT_EXIST, ExtendedCost);
        }
        else
            sLog.outErrorDb("Table `%s` contain item (Entry: %u) with wrong ExtendedCost (%u) for %s %u, ignoring",
                            tableName, item_id, ExtendedCost, idStr, vendor_entry);
        return false;
    }

    if (maxcount > 0 && incrtime == 0)
    {
        if (pl)
        {
            ChatHandler(pl).PSendSysMessage("MaxCount!=0 (%u) but IncrTime==0", maxcount);
        }
        else
            sLog.outErrorDb("Table `%s` has `maxcount` (%u) for item %u of %s %u but `incrtime`=0, ignoring",
                            tableName, maxcount, item_id, idStr, vendor_entry);
        return false;
    }
    else if (maxcount == 0 && incrtime > 0)
    {
        if (pl)
        {
            ChatHandler(pl).PSendSysMessage("MaxCount==0 but IncrTime<>=0");
        }
        else
            sLog.outErrorDb("Table `%s` has `maxcount`=0 for item %u of %s %u but `incrtime`<>0, ignoring",
                            tableName, item_id, idStr, vendor_entry);
        return false;
    }

    if (conditionId && !sConditionStorage.LookupEntry<PlayerCondition>(conditionId))
    {
        sLog.outErrorDb("Table `%s` has `condition_id`=%u for item %u of %s %u but this condition is not valid, ignoring", tableName, conditionId, item_id, idStr, vendor_entry);
        return false;
    }

    VendorItemData const* vItems = isTemplate ? GetNpcVendorTemplateItemList(vendor_entry) : GetNpcVendorItemList(vendor_entry);
    VendorItemData const* tItems = isTemplate ? NULL : GetNpcVendorTemplateItemList(vendor_entry);

    if (!vItems && !tItems)
    {
        return true;                                        // later checks for non-empty lists
    }

    if (vItems && vItems->FindItemCostPair(item_id, ExtendedCost))
    {
        if (pl)
        {
            ChatHandler(pl).PSendSysMessage(LANG_ITEM_ALREADY_IN_LIST, item_id, ExtendedCost);
        }
        else
        {
            sLog.outErrorDb("Table `%s` has duplicate items %u (with extended cost %u) for %s %u, ignoring",
                            tableName, item_id, ExtendedCost, idStr, vendor_entry);
        }
        return false;
    }

    if (!isTemplate)
    {
        if (tItems && tItems->FindItemCostPair(item_id, ExtendedCost))
        {
            if (pl)
            {
                ChatHandler(pl).PSendSysMessage(LANG_ITEM_ALREADY_IN_LIST, item_id, ExtendedCost);
            }
            else
            {
                if (!cInfo->VendorTemplateId)
                    sLog.outErrorDb("Table `%s` has duplicate items %u (with extended cost %u) for %s %u, ignoring",
                                    tableName, item_id, ExtendedCost, idStr, vendor_entry);
                else
                    sLog.outErrorDb("Table `%s` has duplicate items %u (with extended cost %u) for %s %u (or possible in vendor template %u), ignoring",
                                    tableName, item_id, ExtendedCost, idStr, vendor_entry, cInfo->VendorTemplateId);
            }
            return false;
        }
    }

    uint32 countItems = vItems ? vItems->GetItemCount() : 0;
    countItems += tItems ? tItems->GetItemCount() : 0;

    if (countItems >= MAX_VENDOR_ITEMS)
    {
        if (pl)
        {
            ChatHandler(pl).SendSysMessage(LANG_COMMAND_ADDVENDORITEMITEMS);
        }
        else
            sLog.outErrorDb("Table `%s` has too many items (%u >= %i) for %s %u, ignoring",
                            tableName, countItems, MAX_VENDOR_ITEMS, idStr, vendor_entry);
        return false;
    }

    return true;
}

/**
 * @brief Registers a loaded group in the object manager.
 *
 * @param group The group to add.
 */
void ObjectMgr::AddGroup(Group* group)
{
    mGroupMap[group->GetId()] = group ;
}

/**
 * @brief Unregisters a loaded group from the object manager.
 *
 * @param group The group to remove.
 */
void ObjectMgr::RemoveGroup(Group* group)
{
    mGroupMap.erase(group->GetId());
}

void ObjectMgr::AddArenaTeam(ArenaTeam* arenaTeam)
{
    mArenaTeamMap[arenaTeam->GetId()] = arenaTeam;
}

void ObjectMgr::RemoveArenaTeam(uint32 Id)
{
    mArenaTeamMap.erase(Id);
}

// Functions for scripting access
bool LoadMangosStrings(DatabaseType& db, char const* table, int32 start_value, int32 end_value, bool extra_content)
{
    // MAX_DB_SCRIPT_STRING_ID is max allowed negative value for scripts (scrpts can use only more deep negative values
    // start/end reversed for negative values
    if (start_value > MAX_DB_SCRIPT_STRING_ID || end_value >= start_value)
    {
        sLog.outErrorDb("Table '%s' attempt loaded with reserved by mangos range (%d - %d), strings not loaded.", table, start_value, end_value + 1);
        return false;
    }

    return sObjectMgr.LoadMangosStrings(db, table, start_value, end_value, extra_content);
}


/**
 * @brief Retrieves a creature template from the global creature store.
 *
 * @param entry The creature template entry.
 * @return CreatureInfo const* The matching creature template, or null if missing.
 */
CreatureInfo const* GetCreatureTemplateStore(uint32 entry)
{
    return sCreatureStorage.LookupEntry<CreatureInfo>(entry);
}

/**
 * @brief Retrieves a quest template from the object manager.
 *
 * @param entry The quest template entry.
 * @return Quest const* The matching quest template, or null if missing.
 */
Quest const* GetQuestTemplateStore(uint32 entry)
{
    return sObjectMgr.GetQuestTemplate(entry);
}

/**
 * @brief Retrieves localized MaNGOS string data by entry id.
 *
 * @param entry The localized string entry.
 * @return MangosStringLocale const* The matching localized string data, or null if missing.
 */
MangosStringLocale const* GetMangosStringData(int32 entry)
{
    return sObjectMgr.GetMangosStringLocale(entry);
}

/**
 * @brief Evaluates whether a creature spawn matches the current search criteria.
 *
 * @param dataPair The creature data pair being tested.
 * @return true if the search can stop early; otherwise, false.
 */
bool FindCreatureData::operator()(CreatureDataPair const& dataPair)
{
    // skip wrong entry ids
    if (i_id && dataPair.second.id != i_id)
    {
        return false;
    }

    if (!i_anyData)
    {
        i_anyData = &dataPair;
    }

    // without player we can't find more stricted cases, so use fouded
    if (!i_player)
    {
        return true;
    }

    // skip diff. map cases
    if (dataPair.second.mapid != i_player->GetMapId())
    {
        return false;
    }

    float new_dist = i_player->GetDistance2d(dataPair.second.posX, dataPair.second.posY);

    if (!i_mapData || new_dist < i_mapDist)
    {
        i_mapData = &dataPair;
        i_mapDist = new_dist;
    }

    // skip not spawned (in any state),
    uint16 pool_id = sPoolMgr.IsPartOfAPool<Creature>(dataPair.first);
    if (pool_id && !i_player->GetMap()->GetPersistentState()->IsSpawnedPoolObject<Creature>(dataPair.first))
    {
        return false;
    }

    if (!i_spawnedData || new_dist < i_spawnedDist)
    {
        i_spawnedData = &dataPair;
        i_spawnedDist = new_dist;
    }

    return false;
}

/**
 * @brief Gets the best matching creature spawn data found by the search.
 *
 * @return The selected creature data pair, or null if none matched.
 */
CreatureDataPair const* FindCreatureData::GetResult() const
{
    if (i_spawnedData)
    {
        return i_spawnedData;
    }

    if (i_mapData)
    {
        return i_mapData;
    }

    return i_anyData;
}

/**
 * @brief Evaluates whether a gameobject spawn matches the current search criteria.
 *
 * @param dataPair The gameobject data pair being tested.
 * @return true if the search can stop early; otherwise, false.
 */
bool FindGOData::operator()(GameObjectDataPair const& dataPair)
{
    // skip wrong entry ids
    if (i_id && dataPair.second.id != i_id)
    {
        return false;
    }

    if (!i_anyData)
    {
        i_anyData = &dataPair;
    }

    // without player we can't find more stricted cases, so use fouded
    if (!i_player)
    {
        return true;
    }

    // skip diff. map cases
    if (dataPair.second.mapid != i_player->GetMapId())
    {
        return false;
    }

    float new_dist = i_player->GetDistance2d(dataPair.second.posX, dataPair.second.posY);

    if (!i_mapData || new_dist < i_mapDist)
    {
        i_mapData = &dataPair;
        i_mapDist = new_dist;
    }

    // skip not spawned (in any state)
    uint16 pool_id = sPoolMgr.IsPartOfAPool<GameObject>(dataPair.first);
    if (pool_id && !i_player->GetMap()->GetPersistentState()->IsSpawnedPoolObject<GameObject>(dataPair.first))
    {
        return false;
    }

    if (!i_spawnedData || new_dist < i_spawnedDist)
    {
        i_spawnedData = &dataPair;
        i_spawnedDist = new_dist;
    }

    return false;
}

/**
 * @brief Gets the best matching gameobject spawn data found by the search.
 *
 * @return The selected gameobject data pair, or null if none matched.
 */
GameObjectDataPair const* FindGOData::GetResult() const
{
    if (i_mapData)
    {
        return i_mapData;
    }

    if (i_spawnedData)
    {
        return i_spawnedData;
    }

    return i_anyData;
}

/**
 * @brief Displays localized scripted text, sound, and emote output from a source object.
 *
 * @param source The speaking world object.
 * @param entry The text entry id.
 * @param target The optional target unit for whispers.
 * @return true if the text was displayed successfully; otherwise false.
 */
bool DoDisplayText(WorldObject* source, int32 entry, Unit const* target /*=NULL*/)
{
    MangosStringLocale const* data = sObjectMgr.GetMangosStringLocale(entry);

    if (!data)
    {
        _DoStringError(entry, "DoScriptText with source %s could not find text entry %i.", source->GetGuidStr().c_str(), entry);
        return false;
    }

    if (data->SoundId)
    {
        if (data->Type == CHAT_TYPE_ZONE_YELL)
        {
            source->GetMap()->PlayDirectSoundToMap(data->SoundId, source->GetZoneId());
        }
        else if (data->Type == CHAT_TYPE_WHISPER || data->Type == CHAT_TYPE_BOSS_WHISPER)
        {
            // An error will be displayed for the text
            if (target && target->GetTypeId() == TYPEID_PLAYER)
            {
                source->PlayDirectSound(data->SoundId, (Player const*)target);
            }
        }
        else
        {
            source->PlayDirectSound(data->SoundId);
        }
    }

    if (data->Emote)
    {
        if (source->GetTypeId() == TYPEID_UNIT || source->GetTypeId() == TYPEID_PLAYER)
        {
            ((Unit*)source)->HandleEmote(data->Emote);
        }
        else
        {
            _DoStringError(entry, "DoDisplayText entry %i tried to process emote for invalid source %s", entry, source->GetGuidStr().c_str());
            return false;
        }
    }

    if ((data->Type == CHAT_TYPE_WHISPER || data->Type == CHAT_TYPE_BOSS_WHISPER) && (!target || target->GetTypeId() != TYPEID_PLAYER))
    {
        _DoStringError(entry, "DoDisplayText entry %i can not whisper without target unit (TYPEID_PLAYER).", entry);
        return false;
    }

    source->MonsterText(data, target);
    return true;
}



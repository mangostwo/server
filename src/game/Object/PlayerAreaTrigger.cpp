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
#include "DisableMgr.h"
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif /* ENABLE_ELUNA */

#include <cmath>
/**
 * @brief Sends the appropriate transfer-aborted feedback for an area lock failure.
 *
 * @param mapEntry The destination map entry.
 * @param at The triggering area trigger, if any.
 * @param lockStatus The evaluated area lock status.
 * @param miscRequirement Extra requirement data used by some messages.
 */
void Player::SendTransferAbortedByLockStatus(MapEntry const* mapEntry, AreaLockStatus lockStatus, uint32 miscRequirement)
{
    MANGOS_ASSERT(mapEntry);

    DEBUG_LOG("SendTransferAbortedByLockStatus: Called for %s on map %u, LockAreaStatus %u, miscRequirement %u)", GetGuidStr().c_str(), mapEntry->MapID, lockStatus, miscRequirement);

    switch (lockStatus)
    {
        case AREA_LOCKSTATUS_TOO_LOW_LEVEL:
            GetSession()->SendAreaTriggerMessage(GetSession()->GetMangosString(LANG_LEVEL_MINREQUIRED), miscRequirement);
            break;
        case AREA_LOCKSTATUS_ZONE_IN_COMBAT:
            GetSession()->SendTransferAborted(mapEntry->MapID, TRANSFER_ABORT_ZONE_IN_COMBAT);
            break;
        case AREA_LOCKSTATUS_INSTANCE_IS_FULL:
            GetSession()->SendTransferAborted(mapEntry->MapID, TRANSFER_ABORT_MAX_PLAYERS);
            break;
        case AREA_LOCKSTATUS_QUEST_NOT_COMPLETED:
            if (mapEntry->MapID == 269)                     // Exception for Black Morass
            {
                GetSession()->SendAreaTriggerMessage("%s", GetSession()->GetMangosString(LANG_TELEREQ_QUEST_BLACK_MORASS));
                break;
            }
            else if (mapEntry->IsContinent())               // do not report anything for quest areatrigge
            {
                DEBUG_LOG("SendTransferAbortedByLockStatus: LockAreaStatus %u, do not teleport, no message sent (mapId %u)", lockStatus, mapEntry->MapID);
                break;
            }
            // No break here!
            [[fallthrough]];
        case AREA_LOCKSTATUS_MISSING_ITEM:
            GetSession()->SendTransferAborted(mapEntry->MapID, TRANSFER_ABORT_DIFFICULTY, GetDifficulty(mapEntry->IsRaid()));
            break;
        case AREA_LOCKSTATUS_MISSING_DIFFICULTY:
        {
            Difficulty difficulty = GetDifficulty(mapEntry->IsRaid());
            GetSession()->SendTransferAborted(mapEntry->MapID, TRANSFER_ABORT_DIFFICULTY, difficulty > RAID_DIFFICULTY_10MAN_HEROIC ? RAID_DIFFICULTY_10MAN_HEROIC : difficulty);
            break;
        }
        case AREA_LOCKSTATUS_INSUFFICIENT_EXPANSION:
            GetSession()->SendTransferAborted(mapEntry->MapID, TRANSFER_ABORT_INSUF_EXPAN_LVL, miscRequirement);
            if (sObjectMgr.GetMapEntranceTrigger(mapEntry->MapID))
            {
                GetSession()->SendAreaTriggerMessage(GetSession()->GetMangosString(LANG_LEVEL_MINREQUIRED_AND_ITEM), sObjectMgr.GetItemPrototype(miscRequirement)->Name1);
            }
            break;
        case AREA_LOCKSTATUS_NOT_ALLOWED:
            GetSession()->SendTransferAborted(mapEntry->MapID, TRANSFER_ABORT_MAP_NOT_ALLOWED);
            break;
        case AREA_LOCKSTATUS_RAID_LOCKED:
            GetSession()->SendTransferAborted(mapEntry->MapID, TRANSFER_ABORT_NEED_GROUP);
            break;
        case AREA_LOCKSTATUS_UNKNOWN_ERROR:
            GetSession()->SendTransferAborted(mapEntry->MapID, TRANSFER_ABORT_ERROR);
            break;
        case AREA_LOCKSTATUS_OK:
            sLog.outError("SendTransferAbortedByLockStatus: LockAreaStatus AREA_LOCKSTATUS_OK received for %s (mapId %u)", GetGuidStr().c_str(), mapEntry->MapID);
            MANGOS_ASSERT(false);
            break;
        default:
            sLog.outError("SendTransfertAbortedByLockstatus: unhandled LockAreaStatus %u, when %s attempts to enter in map %u", lockStatus, GetGuidStr().c_str(), mapEntry->MapID);
            break;
    }
}


/**
 * @brief Evaluates whether the player can use an area trigger into another map.
 *
 * @param at The area trigger being used.
 * @param miscRequirement Output requirement data for failure messaging.
 * @return The evaluated area lock status.
 */
AreaLockStatus Player::GetAreaTriggerLockStatus(AreaTrigger const* at, Difficulty difficulty, uint32& miscRequirement)
{
    miscRequirement = 0;

    if (!at)
    {
        return AREA_LOCKSTATUS_UNKNOWN_ERROR;
    }

    MapEntry const* mapEntry = sMapStore.LookupEntry(at->target_mapId);
    if (!mapEntry)
    {
        return AREA_LOCKSTATUS_UNKNOWN_ERROR;
    }

    bool isRegularTargetMap = !mapEntry->IsDungeon() || GetDifficulty(mapEntry->IsRaid()) == REGULAR_DIFFICULTY;

    MapDifficultyEntry const* mapDiff = GetMapDifficultyData(at->target_mapId, difficulty);
    if (mapEntry->IsDungeon() && !mapDiff)
    {
        return AREA_LOCKSTATUS_MISSING_DIFFICULTY;
    }

    // Expansion requirement
    if (GetSession()->Expansion() < mapEntry->Expansion())
    {
        miscRequirement = mapEntry->Expansion();
        return AREA_LOCKSTATUS_INSUFFICIENT_EXPANSION;
    }

    // Gamemaster can always enter
    if (isGameMaster())
    {
        return AREA_LOCKSTATUS_OK;
    }

    // Level Requirements
    if (getLevel() < at->requiredLevel && !sWorld.getConfig(CONFIG_BOOL_INSTANCE_IGNORE_LEVEL))
    {
        miscRequirement = at->requiredLevel;
        return AREA_LOCKSTATUS_TOO_LOW_LEVEL;
    }
    if (!isRegularTargetMap && !sWorld.getConfig(CONFIG_BOOL_INSTANCE_IGNORE_LEVEL) && getLevel() < uint32(maxLevelForExpansion[mapEntry->Expansion()]))
    {
        miscRequirement = maxLevelForExpansion[mapEntry->Expansion()];
        return AREA_LOCKSTATUS_TOO_LOW_LEVEL;
    }

    // Raid Requirements
    if (mapEntry->IsRaid() && !sWorld.getConfig(CONFIG_BOOL_INSTANCE_IGNORE_RAID))
        if (!GetGroup() || !GetGroup()->isRaidGroup())
        {
            return AREA_LOCKSTATUS_RAID_LOCKED;
        }

    // Item Requirements: must have requiredItem OR requiredItem2, report the first one that's missing
    if (at->requiredItem)
    {
        if (!HasItemCount(at->requiredItem, 1) &&
                (!at->requiredItem2 || !HasItemCount(at->requiredItem2, 1)))
        {
            miscRequirement = at->requiredItem;
            return AREA_LOCKSTATUS_MISSING_ITEM;
        }
    }
    else if (at->requiredItem2 && !HasItemCount(at->requiredItem2, 1))
    {
        miscRequirement = at->requiredItem2;
        return AREA_LOCKSTATUS_MISSING_ITEM;
    }
    // Heroic item requirements
    if (!isRegularTargetMap && at->heroicKey)
    {
        if (!HasItemCount(at->heroicKey, 1) && (!at->heroicKey2 || !HasItemCount(at->heroicKey2, 1)))
        {
            miscRequirement = at->heroicKey;
            return AREA_LOCKSTATUS_MISSING_ITEM;
        }
    }
    else if (!isRegularTargetMap && at->heroicKey2 && !HasItemCount(at->heroicKey2, 1))
    {
        miscRequirement = at->heroicKey2;
        return AREA_LOCKSTATUS_MISSING_ITEM;
    }

    // Quest Requirements
    if (isRegularTargetMap && at->requiredQuest && !GetQuestRewardStatus(at->requiredQuest))
    {
        miscRequirement = at->requiredQuest;
        return AREA_LOCKSTATUS_QUEST_NOT_COMPLETED;
    }
    if (!isRegularTargetMap && at->requiredQuestHeroic && !GetQuestRewardStatus(at->requiredQuestHeroic))
    {
        miscRequirement = at->requiredQuestHeroic;
        return AREA_LOCKSTATUS_QUEST_NOT_COMPLETED;
    }

    // If the map is not created, assume it is possible to enter it.
    DungeonPersistentState* state = GetBoundInstanceSaveForSelfOrGroup(at->target_mapId);
    Map* map = sMapMgr.FindMap(at->target_mapId, state ? state->GetInstanceId() : 0);

    // ToDo add achievement check

    // Map's state check
    if (map && map->IsDungeon())
    {
        // cannot enter if the instance is full (player cap), GMs don't count
        if (((DungeonMap*)map)->GetPlayersCountExceptGMs() >= ((DungeonMap*)map)->GetMaxPlayers())
        {
            return AREA_LOCKSTATUS_INSTANCE_IS_FULL;
        }

        // In Combat check
        if (map && map->GetInstanceData() && map->GetInstanceData()->IsEncounterInProgress())
        {
            return AREA_LOCKSTATUS_ZONE_IN_COMBAT;
        }

        // Bind Checks
        InstancePlayerBind* pBind = GetBoundInstance(at->target_mapId, GetDifficulty(mapEntry->IsRaid()));
        if (pBind && pBind->perm && pBind->state != state)
        {
            return AREA_LOCKSTATUS_HAS_BIND;
        }
        if (pBind && pBind->perm && pBind->state != map->GetPersistentState())
        {
            return AREA_LOCKSTATUS_HAS_BIND;
        }
    }

    return AREA_LOCKSTATUS_OK;
};



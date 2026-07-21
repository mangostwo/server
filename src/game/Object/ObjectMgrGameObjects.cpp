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
 * @file ObjectMgrGameObjects.cpp
 * @brief Cohesion split of ObjectMgr.cpp -- gameobject spawn loading and grid placement.
 */

#include <map>
#include "Utilities/PackedValues.h"
#include "ObjectMgr.h"
#include "Database/DatabaseEnv.h"
#include "Policies/Singleton.h"
#include "Log.h"
#include "ProgressBar.h"
#include "DBCStores.h"
#include "SQLStorages.h"
#include "MapManager.h"
#include "GridDefines.h"
#include "DisableMgr.h"

/**
 * @brief Loads gameobject spawn records and validates their database data.
 */
void ObjectMgr::LoadGameObjects()
{
    uint32 count = 0;

    //                                                              0                    1                  2                   3
    QueryResult* result = WorldDatabase.Query("SELECT `gameobject`.`guid`, `gameobject`.`id`, `gameobject`.`map`, `gameobject`.`position_x`, "
    //                                   4                          5                          6                           7
                          "`gameobject`.`position_y`, `gameobject`.`position_z`, `gameobject`.`orientation`, `gameobject`.`rotation0`, "
    //                                   8                         9                         10                        11
                          "`gameobject`.`rotation1`, `gameobject`.`rotation2`, `gameobject`.`rotation3`, `gameobject`.`spawntimesecs`, "
    //                                   12                           13                    14                        15
                          "`gameobject`.`animprogress`, `gameobject`.`state`, `gameobject`.`spawnMask`, `gameobject`.`phaseMask`, "
    //                                             16                          17                                       18
                          "`game_event_gameobject`.`event`, `pool_gameobject`.`pool_entry`, `pool_gameobject_template`.`pool_entry` "
                          "FROM `gameobject` "
                          "LEFT OUTER JOIN `game_event_gameobject` ON `gameobject`.`guid` = `game_event_gameobject`.`guid` "
                          "LEFT OUTER JOIN `pool_gameobject` ON `gameobject`.`guid` = `pool_gameobject`.`guid` "
                          "LEFT OUTER JOIN `pool_gameobject_template` ON `gameobject`.`id` = `pool_gameobject_template`.`id`");

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();
        sLog.outErrorDb(">> Loaded 0 gameobjects. DB table `gameobject` is empty.");
        sLog.outString();
        return;
    }

    // build single time for check spawnmask
    std::map<uint32, uint32> spawnMasks;
    for (uint32 i = 0; i < sMapStore.GetNumRows(); ++i)
    {
        if (sMapStore.LookupEntry(i))
            for (int k = 0; k < MAX_DIFFICULTY; ++k)
            {
                if (GetMapDifficultyData(i, Difficulty(k)))
                {
                    spawnMasks[i] |= (1 << k);
                }
            }
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();
        bar.step();

        uint32 guid         = fields[ 0].GetUInt32();
        uint32 entry        = fields[ 1].GetUInt32();

        if (DisableMgr::IsDisabledFor(DISABLE_TYPE_GAMEOBJECT_SPAWN, guid))
        {
            sLog.outDebug("Gameobject guid %u (entry %u) spawning is disabled.", guid, entry);
            continue;
        }

        GameObjectInfo const* gInfo = GetGameObjectInfo(entry);
        if (!gInfo)
        {
            sLog.outErrorDb("Table `gameobject` has gameobject (GUID: %u) with non existing gameobject entry %u, skipped.", guid, entry);
            continue;
        }

        if (!gInfo->displayId)
        {
            switch (gInfo->type)
            {
                    // can be invisible always and then not req. display id in like case
                case GAMEOBJECT_TYPE_TRAP:
                case GAMEOBJECT_TYPE_SPELL_FOCUS:
                    break;
                default:
                    sLog.outErrorDb("Gameobject (GUID: %u Entry %u GoType: %u) have displayId == 0 and then will always invisible in game.", guid, entry, gInfo->type);
                    break;
            }
        }
        else if (!sGameObjectDisplayInfoStore.LookupEntry(gInfo->displayId))
        {
            sLog.outErrorDb("Gameobject (GUID: %u Entry %u GoType: %u) have invalid displayId (%u), not loaded.", guid, entry, gInfo->type, gInfo->displayId);
            continue;
        }

        GameObjectData& data = mGameObjectDataMap[guid];

        data.id             = entry;
        data.mapid          = fields[ 2].GetUInt32();
        data.posX           = fields[ 3].GetFloat();
        data.posY           = fields[ 4].GetFloat();
        data.posZ           = fields[ 5].GetFloat();
        data.orientation    = fields[ 6].GetFloat();
        data.rotx           = fields[ 7].GetFloat();
        data.roty           = fields[ 8].GetFloat();
        data.rotz           = fields[ 9].GetFloat();
        data.rotw           = fields[10].GetFloat();
        data.spawntimesecs  = fields[11].GetInt32();
        data.animprogress   = fields[12].GetUInt32();
        uint32 go_state     = fields[13].GetUInt32();
        data.spawnMask      = fields[14].GetUInt8();
        data.phaseMask      = fields[15].GetUInt16();
        int16 gameEvent     = fields[16].GetInt16();
        int16 GuidPoolId    = fields[17].GetInt16();
        int16 EntryPoolId   = fields[18].GetInt16();

        MapEntry const* mapEntry = sMapStore.LookupEntry(data.mapid);
        if (!mapEntry)
        {
            sLog.outErrorDb("Table `gameobject` have gameobject (GUID: %u Entry: %u) that spawned at nonexistent map (Id: %u), skip", guid, data.id, data.mapid);
            continue;
        }

        if (data.spawnMask & ~spawnMasks[data.mapid])
            sLog.outErrorDb("Table `gameobject` have gameobject (GUID: %u Entry: %u) that have wrong spawn mask %u including not supported difficulty modes for map (Id: %u), skip", guid, data.id, data.spawnMask, data.mapid);

        if (data.spawntimesecs == 0 && gInfo->IsDespawnAtAction())
        {
            sLog.outErrorDb("Table `gameobject` have gameobject (GUID: %u Entry: %u) with `spawntimesecs` (0) value, but gameobejct marked as despawnable at action.", guid, data.id);
        }

        if (go_state >= MAX_GO_STATE)
        {
            sLog.outErrorDb("Table `gameobject` have gameobject (GUID: %u Entry: %u) with invalid `state` (%u) value, skip", guid, data.id, go_state);
            continue;
        }
        data.go_state       = GOState(go_state);

        if (data.rotx < -1.0f || data.rotx > 1.0f)
        {
            sLog.outErrorDb("Table `gameobject` have gameobject (GUID: %u Entry: %u) with invalid rotation.x (%f) value, skip", guid, data.id, data.rotx);
            continue;
        }

        if (data.roty < -1.0f || data.roty > 1.0f)
        {
            sLog.outErrorDb("Table `gameobject` have gameobject (GUID: %u Entry: %u) with invalid rotation.y (%f) value, skip", guid, data.id, data.roty);
            continue;
        }

        if (data.rotz < -1.0f || data.rotz > 1.0f)
        {
            sLog.outErrorDb("Table `gameobject` have gameobject (GUID: %u Entry: %u) with invalid rotation.z (%f) value, skip", guid, data.id, data.rotz);
            continue;
        }

        if (data.rotw < -1.0f || data.rotw > 1.0f)
        {
            sLog.outErrorDb("Table `gameobject` have gameobject (GUID: %u Entry: %u) with invalid rotation.w (%f) value, skip", guid, data.id, data.rotw);
            continue;
        }

        if (!MapManager::IsValidMapCoord(data.mapid, data.posX, data.posY, data.posZ, data.orientation))
        {
            sLog.outErrorDb("Table `gameobject` have gameobject (GUID: %u Entry: %u) with invalid coordinates, skip", guid, data.id);
            continue;
        }

        if (data.phaseMask == 0)
        {
            sLog.outErrorDb("Table `gameobject` have gameobject (GUID: %u Entry: %u) with `phaseMask`=0 (not visible for anyone), set to 1.", guid, data.id);
            data.phaseMask = 1;
        }

        if (gameEvent == 0 && GuidPoolId == 0 && EntryPoolId == 0) // if not this is to be managed by GameEvent System or Pool system
        {
            AddGameobjectToGrid(guid, &data);
        }


        //uint32 zoneId, areaId;
        //sTerrainMgr.LoadTerrain(data.mapid)->GetZoneAndAreaId(zoneId, areaId, data.posX, data.posY, data.posZ);
        //sLog.outErrorDb("UPDATE gameobject SET zone_id=%u, area_id=%u WHERE guid=%u;", zoneId, areaId, guid);

        ++count;
    }
    while (result->NextRow());

    delete result;

    sLog.outString();
    sLog.outString(">> Loaded %zu gameobjects", mGameObjectDataMap.size());
}

/**
 * @brief Adds a gameobject spawn GUID to the grid lookup for its map cell.
 *
 * @param guid The gameobject spawn GUID.
 * @param data The gameobject spawn data.
 */
void ObjectMgr::AddGameobjectToGrid(uint32 guid, GameObjectData const* data)
{
    uint8 mask = data->spawnMask;
    for (uint8 i = 0; mask != 0; ++i, mask >>= 1)
    {
        if (mask & 1)
        {
            CellPair cell_pair = MaNGOS::ComputeCellPair(data->posX, data->posY);
            uint32 cell_id = (cell_pair.y_coord * TOTAL_NUMBER_OF_CELLS_PER_MAP) + cell_pair.x_coord;

            CellObjectGuids& cell_guids = mMapObjectGuids[MAKE_PAIR32(data->mapid, i)][cell_id];
            cell_guids.gameobjects.insert(guid);
        }
    }
}

/**
 * @brief Removes a gameobject spawn GUID from the grid lookup for its map cell.
 *
 * @param guid The gameobject spawn GUID.
 * @param data The gameobject spawn data.
 */
void ObjectMgr::RemoveGameobjectFromGrid(uint32 guid, GameObjectData const* data)
{
    uint8 mask = data->spawnMask;
    for (uint8 i = 0; mask != 0; ++i, mask >>= 1)
    {
        if (mask & 1)
        {
            CellPair cell_pair = MaNGOS::ComputeCellPair(data->posX, data->posY);
            uint32 cell_id = (cell_pair.y_coord * TOTAL_NUMBER_OF_CELLS_PER_MAP) + cell_pair.x_coord;

            CellObjectGuids& cell_guids = mMapObjectGuids[MAKE_PAIR32(data->mapid, i)][cell_id];
            cell_guids.gameobjects.erase(guid);
        }
    }
}

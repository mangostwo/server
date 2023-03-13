/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2023 MaNGOS <https://getmangos.eu>
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

#include "Chat.h"
#include "Language.h"
#include "G3D/Quat.h"
#include "MapManager.h"
#include "GameEventMgr.h"
#include "ObjectMgr.h"

 /**********************************************************************
     CommandTable : gobjectCommandTable
 /***********************************************************************/

void ChatHandler::ShowAchievementListHelper(AchievementEntry const* achEntry, LocaleConstant loc, time_t const* date /*= NULL*/, Player* target /*= NULL */)
{
    std::string name = achEntry->name[loc];

    ObjectGuid guid = target ? target->GetObjectGuid() : ObjectGuid();

    // |color|Hachievement:achievement_id:player_guid_hex:completed_0_1:mm:dd:yy_from_2000:criteriaMask:0:0:0|h[name]|h|r
    std::ostringstream ss;
    if (m_session)
    {
        ss << achEntry->ID << " - |cffffffff|Hachievement:" << achEntry->ID << ":" << std::hex << guid.GetRawValue() << std::dec;
        if (date)
        {
            // complete date
            tm* aTm = localtime(date);
            ss << ":1:" << aTm->tm_mon + 1 << ":" << aTm->tm_mday << ":" << (aTm->tm_year + 1900 - 2000) << ":";

            // complete criteria mask (all bits set)
            ss << uint32(-1) << ":" << uint32(-1) << ":" << uint32(-1) << ":" << uint32(-1) << ":";
        }
        else
        {
            // complete date
            ss << ":0:0:0:-1:";

            // complete criteria mask
            if (target)
            {
                uint32 criteriaMask[4] = {0, 0, 0, 0};

                if (AchievementMgr const* mgr = target ? &target->GetAchievementMgr() : NULL)
                    if (AchievementCriteriaEntryList const* criteriaList = sAchievementMgr.GetAchievementCriteriaByAchievement(achEntry->ID))
                        for (AchievementCriteriaEntryList::const_iterator itr = criteriaList->begin(); itr != criteriaList->end(); ++itr)
                            if (mgr->IsCompletedCriteria(*itr, achEntry))
                            {
                                criteriaMask[((*itr)->showOrder - 1) / 32] |= (1 << (((*itr)->showOrder - 1) % 32));
                            }

                for (int i = 0; i < 4; ++i)
                {
                    ss << criteriaMask[i] << ":";
                }
            }
            else
            {
                ss << "0:0:0:0:";
            }
        }

        ss << "|h[" << name << " " << localeNames[loc] << "]|h|r";
    }
    else
    {
        ss << achEntry->ID << " - " << name << " " << localeNames[loc];
    }

    if (target && date)
    {
        ss << " [" << TimeToTimestampStr(*date) << "]";
    }

    SendSysMessage(ss.str().c_str());
}

 // delete object by selection or guid
bool ChatHandler::HandleGameObjectDeleteCommand(char* args)
{
    // number or [name] Shift-click form |color|Hgameobject:go_guid|h[name]|h|r
    uint32 lowguid;
    if (!ExtractUint32KeyFromLink(&args, "Hgameobject", lowguid))
    {
        return false;
    }

    if (!lowguid)
    {
        return false;
    }

    GameObject* obj = NULL;

    // by DB guid
    if (GameObjectData const* go_data = sObjectMgr.GetGOData(lowguid))
    {
        obj = GetGameObjectWithGuid(lowguid, go_data->id);
    }

    if (!obj)
    {
        PSendSysMessage(LANG_COMMAND_OBJNOTFOUND, lowguid);
        SetSentErrorMessage(true);
        return false;
    }

    if (ObjectGuid ownerGuid = obj->GetOwnerGuid())
    {
        Unit* owner = sObjectAccessor.GetUnit(*m_session->GetPlayer(), ownerGuid);
        if (!owner || !ownerGuid.IsPlayer())
        {
            PSendSysMessage(LANG_COMMAND_DELOBJREFERCREATURE, obj->GetGUIDLow(), ownerGuid.GetString().c_str());
            SetSentErrorMessage(true);
            return false;
        }

        owner->RemoveGameObject(obj, false);
    }

    obj->SetRespawnTime(0);                                 // not save respawn time
    obj->Delete();
    obj->DeleteFromDB();

    PSendSysMessage(LANG_COMMAND_DELOBJMESSAGE, obj->GetGUIDLow());

    return true;
}

// turn selected object
bool ChatHandler::HandleGameObjectTurnCommand(char* args)
{
    // number or [name] Shift-click form |color|Hgameobject:go_id|h[name]|h|r
    uint32 lowguid;
    if (!ExtractUint32KeyFromLink(&args, "Hgameobject", lowguid))
    {
        return false;
    }

    if (!lowguid)
    {
        return false;
    }

    GameObject* obj = NULL;

    // by DB guid
    if (GameObjectData const* go_data = sObjectMgr.GetGOData(lowguid))
    {
        obj = GetGameObjectWithGuid(lowguid, go_data->id);
    }

    if (!obj)
    {
        PSendSysMessage(LANG_COMMAND_OBJNOTFOUND, lowguid);
        SetSentErrorMessage(true);
        return false;
    }
    // we expect the command to have 4 parameters, describing a rotation quaternion
    // the first param is the scalar component, i.e. the rotation angle
    // the next params are the vector component, i.e. the rotation direction
    // if the first param is not specified, it is considered a rotation to match
    // the player's current orientation

    float rx, ry, rz, o;
    G3D::Quat final_rot;

    if (!ExtractFloat(&args, o))  // if rotation angle is not specified or invalid
    {
         // let's obtain the player's orientation
        if (!ExtractOptFloat(&args, o, m_session->GetPlayer()->GetOrientation()))
        {
            return false; //something is very wrong
        }

        // ok, let's rotate the GO.
        // we first get the original rotation quaternion
        // then we'll create a rotation quat describing the rotation around Z
        G3D::Quat original_rot;
        obj->GetQuaternion(original_rot);

        // the rotation amount around Z-axis
        float deltaO = o - obj->GetOrientationFromQuat(original_rot);

        // multiplying 2 quaternions gives the final rotation
        // quaternion multiplication is not commutative!
        // quaternion multiplication gives a non-unit quat.
        final_rot = G3D::Quat(0.0f, 0.0f, sin(deltaO/2), cos(deltaO/2)) * original_rot;

    }
    else // we have a valid rotation angle and we expect the description of rotation axis via 3 floats
    {
        if (!ExtractOptFloat(&args, rx, 0) || !ExtractOptFloat(&args, ry, 0) || !ExtractOptFloat(&args, rz, 0))
        {
            return false;  // mising components, abort
        }

        float s = sin(o/2);
        float c = cos(o/2);
        final_rot = G3D::Quat(s*rx, s*ry, s*rz, c);
    }

    final_rot.unitize();

    Map* map = obj->GetMap();
    map->Remove(obj, false); //mandatory to remove GO model from m_dyn_tree

    obj->SetQuaternion(final_rot); // this will update internal model rotation matrices
    obj->Relocate(obj->GetPositionX(), obj->GetPositionY(), obj->GetPositionZ(), obj->GetOrientationFromQuat(final_rot));

    map->Add(obj);

    obj->SaveToDB();
    obj->Refresh();

    PSendSysMessage(LANG_COMMAND_TURNOBJMESSAGE, obj->GetGUIDLow(), obj->GetGOInfo()->name, obj->GetGUIDLow());

    return true;
}

// move selected object
bool ChatHandler::HandleGameObjectMoveCommand(char* args)
{
    // number or [name] Shift-click form |color|Hgameobject:go_guid|h[name]|h|r
    uint32 lowguid;
    if (!ExtractUint32KeyFromLink(&args, "Hgameobject", lowguid))
    {
        return false;
    }

    if (!lowguid)
    {
        return false;
    }

    GameObject* obj = NULL;

    // by DB guid
    if (GameObjectData const* go_data = sObjectMgr.GetGOData(lowguid))
    {
        obj = GetGameObjectWithGuid(lowguid, go_data->id);
    }

    if (!obj)
    {
        PSendSysMessage(LANG_COMMAND_OBJNOTFOUND, lowguid);
        SetSentErrorMessage(true);
        return false;
    }

    if (!*args)
    {
        Player* chr = m_session->GetPlayer();

        Map* map = obj->GetMap();
        map->Remove(obj, false);

        obj->Relocate(chr->GetPositionX(), chr->GetPositionY(), chr->GetPositionZ(), obj->GetOrientation());

        map->Add(obj);
    }
    else
    {
        float x;
        if (!ExtractFloat(&args, x))
        {
            return false;
        }

        float y;
        if (!ExtractFloat(&args, y))
        {
            return false;
        }

        float z;
        if (!ExtractFloat(&args, z))
        {
            return false;
        }

        if (!MapManager::IsValidMapCoord(obj->GetMapId(), x, y, z))
        {
            PSendSysMessage(LANG_INVALID_TARGET_COORD, x, y, obj->GetMapId());
            SetSentErrorMessage(true);
            return false;
        }

        Map* map = obj->GetMap();
        map->Remove(obj, false);

        obj->Relocate(x, y, z, obj->GetOrientation());

        map->Add(obj);
    }

    obj->SaveToDB();
    obj->Refresh();

    PSendSysMessage(LANG_COMMAND_MOVEOBJMESSAGE, obj->GetGUIDLow(), obj->GetGOInfo()->name, obj->GetGUIDLow());

    return true;
}

// spawn go
bool ChatHandler::HandleGameObjectAddCommand(char* args)
{
    // number or [name] Shift-click form |color|Hgameobject_entry:go_id|h[name]|h|r
    uint32 id;
    if (!ExtractUint32KeyFromLink(&args, "Hgameobject_entry", id))
    {
        return false;
    }

    if (!id)
    {
        return false;
    }

    int32 spawntimeSecs;
    if (!ExtractOptInt32(&args, spawntimeSecs, 0))
    {
        return false;
    }

    const GameObjectInfo* gInfo = ObjectMgr::GetGameObjectInfo(id);
    if (!gInfo)
    {
        PSendSysMessage(LANG_GAMEOBJECT_NOT_EXIST, id);
        SetSentErrorMessage(true);
        return false;
    }

    if (gInfo->displayId && !sGameObjectDisplayInfoStore.LookupEntry(gInfo->displayId))
    {
        // report to DB errors log as in loading case
        sLog.outErrorDb("Gameobject (Entry %u GoType: %u) have invalid displayId (%u), not spawned.", id, gInfo->type, gInfo->displayId);
        PSendSysMessage(LANG_GAMEOBJECT_HAVE_INVALID_DATA, id);
        SetSentErrorMessage(true);
        return false;
    }

    Player* plr = m_session->GetPlayer();
    float x = float(plr->GetPositionX());
    float y = float(plr->GetPositionY());
    float z = float(plr->GetPositionZ());
    float o = float(plr->GetOrientation());
    Map* map = plr->GetMap();

    // used guids from specially reserved range (can be 0 if no free values)
    uint32 db_lowGUID = sObjectMgr.GenerateStaticGameObjectLowGuid();
    if (!db_lowGUID)
    {
        SendSysMessage(LANG_NO_FREE_STATIC_GUID_FOR_SPAWN);
        SetSentErrorMessage(true);
        return false;
    }

    GameObject* pGameObj = new GameObject;
    if (!pGameObj->Create(db_lowGUID, gInfo->id, map, plr->GetPhaseMaskForSpawn(), x, y, z, o))
    {
        delete pGameObj;
        return false;
    }

    if (spawntimeSecs)
    {
        pGameObj->SetRespawnTime(spawntimeSecs);
    }

    // fill the gameobject data and save to the db
    pGameObj->SaveToDB(map->GetId(), (1 << map->GetSpawnMode()), plr->GetPhaseMaskForSpawn());

    // this will generate a new guid if the object is in an instance
    if (!pGameObj->LoadFromDB(db_lowGUID, map))
    {
        delete pGameObj;
        return false;
    }

    DEBUG_LOG(GetMangosString(LANG_GAMEOBJECT_CURRENT), gInfo->name, db_lowGUID, x, y, z, o);

    map->Add(pGameObj);
    pGameObj->AIM_Initialize();
    sObjectMgr.AddGameobjectToGrid(db_lowGUID, sObjectMgr.GetGOData(db_lowGUID));

    PSendSysMessage(LANG_GAMEOBJECT_ADD, id, gInfo->name, db_lowGUID, x, y, z);
    return true;
}

bool ChatHandler::HandleGameObjectNearCommand(char* args)
{
    float distance;
    if (!ExtractOptFloat(&args, distance, 10.0f))
    {
        return false;
    }

    uint32 count = 0;

    Player* pl = m_session->GetPlayer();
    QueryResult* result = WorldDatabase.PQuery("SELECT `guid`, `id`, `position_x`, `position_y`, `position_z`, `map`, "
                          "(POW(`position_x` - '%f', 2) + POW(`position_y` - '%f', 2) + POW(`position_z` - '%f', 2)) AS order_ "
                          "FROM `gameobject` WHERE `map`='%u' AND (POW(`position_x` - '%f', 2) + POW(`position_y` - '%f', 2) + POW(`position_z` - '%f', 2)) <= '%f' ORDER BY order_",
                          pl->GetPositionX(), pl->GetPositionY(), pl->GetPositionZ(),
                          pl->GetMapId(), pl->GetPositionX(), pl->GetPositionY(), pl->GetPositionZ(), distance * distance);

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 guid = fields[0].GetUInt32();
            uint32 entry = fields[1].GetUInt32();
            float x = fields[2].GetFloat();
            float y = fields[3].GetFloat();
            float z = fields[4].GetFloat();
            int mapid = fields[5].GetUInt16();

            GameObjectInfo const* gInfo = ObjectMgr::GetGameObjectInfo(entry);

            if (!gInfo)
            {
                continue;
            }

            PSendSysMessage(LANG_GO_MIXED_LIST_CHAT, guid, PrepareStringNpcOrGoSpawnInformation<GameObject>(guid).c_str(), entry, guid, gInfo->name, x, y, z, mapid);

            ++count;
        }
        while (result->NextRow());

        delete result;
    }

    PSendSysMessage(LANG_COMMAND_NEAROBJMESSAGE, distance, count);
    return true;
}

bool ChatHandler::HandleGameObjectTargetCommand(char* args)
{
    Player* pl = m_session->GetPlayer();
    QueryResult* result;
    GameEventMgr::ActiveEvents const& activeEventsList = sGameEventMgr.GetActiveEventList();
    if (*args)
    {
        // number or [name] Shift-click form |color|Hgameobject_entry:go_id|h[name]|h|r
        char* cId = ExtractKeyFromLink(&args, "Hgameobject_entry");
        if (!cId)
        {
            return false;
        }

        uint32 id;
        if (ExtractUInt32(&cId, id))
        {
            result = WorldDatabase.PQuery("SELECT `guid`, `id`, `position_x`, `position_y`, `position_z`, `orientation`, `map`, (POW(`position_x` - '%f', 2) + POW(`position_y` - '%f', 2) + POW(`position_z` - '%f', 2)) AS order_ FROM `gameobject` WHERE `map` = '%i' AND `id` = '%u' ORDER BY order_ ASC LIMIT 1",
                                          pl->GetPositionX(), pl->GetPositionY(), pl->GetPositionZ(), pl->GetMapId(), id);
        }
        else
        {
            std::string name = cId;
            WorldDatabase.escape_string(name);
            result = WorldDatabase.PQuery(
                         "SELECT `guid`, `id`, `position_x`, `position_y`, `position_z`, `orientation`, `map`, (POW(`position_x` - %f, 2) + POW(`position_y` - %f, 2) + POW(`position_z` - %f, 2)) AS order_ "
                         "FROM `gameobject`,`gameobject_template` WHERE `gameobject_template`.`entry` = `gameobject`.`id` AND `map` = %i AND `name` " _LIKE_ " " _CONCAT3_("'%%'", "'%s'", "'%%'")" ORDER BY order_ ASC LIMIT 1",
                         pl->GetPositionX(), pl->GetPositionY(), pl->GetPositionZ(), pl->GetMapId(), name.c_str());
        }
    }
    else
    {
        std::ostringstream eventFilter;
        eventFilter << " AND (event IS NULL ";
        bool initString = true;

        for (GameEventMgr::ActiveEvents::const_iterator itr = activeEventsList.begin(); itr != activeEventsList.end(); ++itr)
        {
            if (initString)
            {
                eventFilter  <<  "OR event IN (" << *itr;
                initString = false;
            }
            else
            {
                eventFilter << "," << *itr;
            }
        }

        if (!initString)
        {
            eventFilter << "))";
        }
        else
        {
            eventFilter << ")";
        }

        result = WorldDatabase.PQuery("SELECT `gameobject`.`guid`, `id`, `position_x`, `position_y`, `position_z`, `orientation`, `map`, "
                                      "(POW(`position_x` - %f, 2) + POW(`position_y` - %f, 2) + POW(`position_z` - %f, 2)) AS order_ FROM `gameobject` "
                                      "LEFT OUTER JOIN `game_event_gameobject` on `gameobject`.`guid`=`game_event_gameobject`.`guid` WHERE `map` = '%i' %s ORDER BY order_ ASC LIMIT 10",
                                      pl->GetPositionX(), pl->GetPositionY(), pl->GetPositionZ(), pl->GetMapId(), eventFilter.str().c_str());
    }

    if (!result)
    {
        SendSysMessage(LANG_COMMAND_TARGETOBJNOTFOUND);
        return true;
    }

    bool found = false;
    float x, y, z, o;
    uint32 lowguid, id;
    uint16 mapid, pool_id;

    do
    {
        Field* fields = result->Fetch();
        lowguid = fields[0].GetUInt32();
        id =      fields[1].GetUInt32();
        x =       fields[2].GetFloat();
        y =       fields[3].GetFloat();
        z =       fields[4].GetFloat();
        o =       fields[5].GetFloat();
        mapid =   fields[6].GetUInt16();
        pool_id = sPoolMgr.IsPartOfAPool<GameObject>(lowguid);
        if (!pool_id || pl->GetMap()->GetPersistentState()->IsSpawnedPoolObject<GameObject>(lowguid))
        {
            found = true;
        }
    }
    while (result->NextRow() && (!found));

    delete result;

    if (!found)
    {
        PSendSysMessage(LANG_GAMEOBJECT_NOT_EXIST, id);
        return false;
    }

    GameObjectInfo const* goI = ObjectMgr::GetGameObjectInfo(id);

    if (!goI)
    {
        PSendSysMessage(LANG_GAMEOBJECT_NOT_EXIST, id);
        return false;
    }

    GameObject* target = m_session->GetPlayer()->GetMap()->GetGameObject(ObjectGuid(HIGHGUID_GAMEOBJECT, id, lowguid));

    PSendSysMessage(LANG_GAMEOBJECT_DETAIL, lowguid, goI->name, lowguid, id, x, y, z, mapid, o);

    if (target)
    {
        time_t curRespawnDelay = target->GetRespawnTimeEx() - time(NULL);
        if (curRespawnDelay < 0)
        {
            curRespawnDelay = 0;
        }

        std::string curRespawnDelayStr = secsToTimeString(curRespawnDelay, TimeFormat::ShortText);
        std::string defRespawnDelayStr = secsToTimeString(target->GetRespawnDelay(), TimeFormat::ShortText);

        PSendSysMessage(LANG_COMMAND_RAWPAWNTIMES, defRespawnDelayStr.c_str(), curRespawnDelayStr.c_str());

        ShowNpcOrGoSpawnInformation<GameObject>(target->GetGUIDLow());

        if (target->GetGoType() == GAMEOBJECT_TYPE_DOOR)
        {
            PSendSysMessage(LANG_COMMAND_GO_STATUS_DOOR, target->GetGoState(), target->getLootState(), GetOnOffStr(target->IsCollisionEnabled()), goI->door.startOpen ? "open" : "closed");
        }
        else
        {
            PSendSysMessage(LANG_COMMAND_GO_STATUS, target->GetGoState(), target->getLootState(), GetOnOffStr(target->IsCollisionEnabled()));
        }
    }
    return true;
}

// set phasemask for selected object
bool ChatHandler::HandleGameObjectPhaseCommand(char* args)
{
    // number or [name] Shift-click form |color|Hgameobject:go_id|h[name]|h|r
    uint32 lowguid;
    if (!ExtractUint32KeyFromLink(&args, "Hgameobject", lowguid))
    {
        return false;
    }

    if (!lowguid)
    {
        return false;
    }

    GameObject* obj = NULL;

    // by DB guid
    if (GameObjectData const* go_data = sObjectMgr.GetGOData(lowguid))
    {
        obj = GetGameObjectWithGuid(lowguid, go_data->id);
    }

    if (!obj)
    {
        PSendSysMessage(LANG_COMMAND_OBJNOTFOUND, lowguid);
        SetSentErrorMessage(true);
        return false;
    }

    uint32 phasemask;
    if (!ExtractUInt32(&args, phasemask) || !phasemask)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    obj->SetPhaseMask(phasemask, true);
    obj->SaveToDB();
    return true;
}

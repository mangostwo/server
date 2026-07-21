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
 * @file BattleGroundObjects.cpp
 * @brief Cohesion split of BattleGround.cpp -- world-object/creature spawning
 *        and doors: spawn BG objects/creatures, spawn events, door open/close,
 *        open-door events and DB-load hooks. Same `BattleGround` class; no
 *        behaviour change.
 */

#include "Utilities/PackedValues.h"
#include "Object.h"
#include "Player.h"
#include "BattleGround.h"
#include "BattleGroundMgr.h"
#include "Creature.h"
#include "MapManager.h"
#include "Language.h"
#include "SpellAuras.h"
#include "ArenaTeam.h"
#include "World.h"
#include "Group.h"
#include "ObjectGuid.h"
#include "ObjectMgr.h"
#include "Mail.h"
#include "WorldPacket.h"
#include "Util.h"
#include "Formulas.h"
#include "GridNotifiersImpl.h"
#include "Chat.h"
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif /* ENABLE_ELUNA */

/// <summary>
/// some doors aren't despawned so we can not handle their closing in gameobject::update()
/// it would be nice to correctly implement GO_ACTIVATED state and open/close doors in gameobject code
/// </summary>
/// <param name="guid">The GUID.</param>
void BattleGround::DoorClose(ObjectGuid guid)
{
    GameObject* obj = GetBgMap()->GetGameObject(guid);
    if (obj)
    {
        // if doors are open, close it
        if (obj->getLootState() == GO_ACTIVATED && obj->GetGoState() != GO_STATE_READY)
        {
            // change state to allow door to be closed
            obj->SetLootState(GO_READY);
            obj->UseDoorOrButton(RESPAWN_ONE_DAY);
        }
    }
    else
    {
        sLog.outError("BattleGround: Door %s not found (can not close doors)", guid.GetString().c_str());
    }
}

/// <summary>
/// Doors the open.
/// </summary>
/// <param name="guid">The GUID.</param>
void BattleGround::DoorOpen(ObjectGuid guid)
{
    GameObject* obj = GetBgMap()->GetGameObject(guid);
    if (obj)
    {
        // change state to be sure they will be opened
        obj->SetLootState(GO_READY);
        obj->UseDoorOrButton(RESPAWN_ONE_DAY);
    }
    else
    {
        sLog.outError("BattleGround: Door %s not found! - doors will be closed.", guid.GetString().c_str());
    }
}

/// <summary>
/// Called when [object DB load].
/// </summary>
/// <param name="creature">The creature.</param>
void BattleGround::OnObjectDBLoad(Creature* creature)
{
    const BattleGroundEventIdx eventId = sBattleGroundMgr.GetCreatureEventIndex(creature->GetGUIDLow());
    if (eventId.event1 == BG_EVENT_NONE)
    {
        return;
    }
    m_EventObjects[MAKE_PAIR32(eventId.event1, eventId.event2)].creatures.push_back(creature->GetObjectGuid());
    if (!IsActiveEvent(eventId.event1, eventId.event2))
    {
        SpawnBGCreature(creature->GetObjectGuid(), RESPAWN_ONE_DAY);
    }
}

/// <summary>
/// Gets the single creature GUID.
/// </summary>
/// <param name="event1">The event1.</param>
/// <param name="event2">The event2.</param>
/// <returns></returns>
ObjectGuid BattleGround::GetSingleCreatureGuid(uint8 event1, uint8 event2)
{
    GuidVector::const_iterator itr = m_EventObjects[MAKE_PAIR32(event1, event2)].creatures.begin();
    if (itr != m_EventObjects[MAKE_PAIR32(event1, event2)].creatures.end())
    {
        return *itr;
    }
    return ObjectGuid();
}

/// <summary>
/// Called when [object DB load].
/// </summary>
/// <param name="obj">The obj.</param>
void BattleGround::OnObjectDBLoad(GameObject* obj)
{
    const BattleGroundEventIdx eventId = sBattleGroundMgr.GetGameObjectEventIndex(obj->GetGUIDLow());
    if (eventId.event1 == BG_EVENT_NONE)
    {
        return;
    }
    m_EventObjects[MAKE_PAIR32(eventId.event1, eventId.event2)].gameobjects.push_back(obj->GetObjectGuid());
    if (!IsActiveEvent(eventId.event1, eventId.event2))
    {
        SpawnBGObject(obj->GetObjectGuid(), RESPAWN_ONE_DAY);
    }
    else
    {
        // it's possible, that doors aren't spawned anymore (wsg)
        if (GetStatus() >= STATUS_IN_PROGRESS && IsDoor(eventId.event1, eventId.event2))
        {
            DoorOpen(obj->GetObjectGuid());
        }
    }
}

/// <summary>
/// Determines whether the specified event1 is door.
/// </summary>
/// <param name="event1">The event1.</param>
/// <param name="event2">The event2.</param>
/// <returns></returns>
bool BattleGround::IsDoor(uint8 event1, uint8 event2)
{
    if (event1 == BG_EVENT_DOOR)
    {
        if (event2 > 0)
        {
            sLog.outError("BattleGround too high event2 for event1:%i", event1);
            return false;
        }
        return true;
    }
    return false;
}

/// <summary>
/// Opens the door event.
/// </summary>
/// <param name="event1">The event1.</param>
/// <param name="event2">The event2.</param>
void BattleGround::OpenDoorEvent(uint8 event1, uint8 event2 /*=0*/)
{
    if (!IsDoor(event1, event2))
    {
        sLog.outError("BattleGround:OpenDoorEvent this is no door event1:%u event2:%u", event1, event2);
        return;
    }
    if (!IsActiveEvent(event1, event2))                 // maybe already despawned (eye)
    {
        sLog.outError("BattleGround:OpenDoorEvent this event isn't active event1:%u event2:%u", event1, event2);
        return;
    }
    GuidVector::const_iterator itr = m_EventObjects[MAKE_PAIR32(event1, event2)].gameobjects.begin();
    for (; itr != m_EventObjects[MAKE_PAIR32(event1, event2)].gameobjects.end(); ++itr)
    {
        DoorOpen(*itr);
    }
}

/// <summary>
/// Spawns the event.
/// </summary>
/// <param name="event1">The event1.</param>
/// <param name="event2">The event2.</param>
/// <param name="spawn">The spawn.</param>
void BattleGround::SpawnEvent(uint8 event1, uint8 event2, bool spawn)
{
    // stop if we want to spawn something which was already spawned
    // or despawn something which was already despawned
    if (event2 == BG_EVENT_NONE || (spawn && m_ActiveEvents[event1] == event2)
        || (!spawn && m_ActiveEvents[event1] != event2))
    {
        return;
    }

    if (spawn)
    {
        // if event gets spawned, the current active event mus get despawned
        SpawnEvent(event1, m_ActiveEvents[event1], false);
        m_ActiveEvents[event1] = event2;                    // set this event to active
    }
    else
    {
        m_ActiveEvents[event1] = BG_EVENT_NONE;             // no event active if event2 gets despawned
    }

    GuidVector::const_iterator itr = m_EventObjects[MAKE_PAIR32(event1, event2)].creatures.begin();
    for (; itr != m_EventObjects[MAKE_PAIR32(event1, event2)].creatures.end(); ++itr)
    {
        SpawnBGCreature(*itr, (spawn) ? RESPAWN_IMMEDIATELY : RESPAWN_ONE_DAY);
    }

    GuidVector::const_iterator itr2 = m_EventObjects[MAKE_PAIR32(event1, event2)].gameobjects.begin();
    for (; itr2 != m_EventObjects[MAKE_PAIR32(event1, event2)].gameobjects.end(); ++itr2)
    {
        SpawnBGObject(*itr2, (spawn) ? RESPAWN_IMMEDIATELY : RESPAWN_ONE_DAY);
    }
}

/// <summary>
/// Spawns the BG object.
/// </summary>
/// <param name="guid">The GUID.</param>
/// <param name="respawntime">The respawntime.</param>
void BattleGround::SpawnBGObject(ObjectGuid guid, uint32 respawntime)
{
    Map* map = GetBgMap();

    GameObject* obj = map->GetGameObject(guid);
    if (!obj)
    {
        return;
    }

    if (respawntime == 0)
    {
        // we need to change state from GO_JUST_DEACTIVATED to GO_READY in case battleground is starting again
        if (obj->getLootState() == GO_JUST_DEACTIVATED)
        {
            obj->SetLootState(GO_READY);
        }
        obj->SetRespawnTime(0);
        map->Add(obj);
    }
    else
    {
        map->Add(obj);
        obj->SetRespawnTime(respawntime);
        obj->SetLootState(GO_JUST_DEACTIVATED);
    }
}

/// <summary>
/// Spawns the BG creature.
/// </summary>
/// <param name="guid">The GUID.</param>
/// <param name="respawntime">The respawntime.</param>
void BattleGround::SpawnBGCreature(ObjectGuid guid, uint32 respawntime)
{
    Map* map = GetBgMap();

    Creature* obj = map->GetCreature(guid);
    if (!obj)
    {
        return;
    }

    if (respawntime == 0)
    {
        obj->Respawn();
        map->Add(obj);
    }
    else
    {
        map->Add(obj);
        obj->SetRespawnDelay(respawntime);
        obj->SetDeathState(JUST_DIED);
        obj->RemoveCorpse();
    }
}

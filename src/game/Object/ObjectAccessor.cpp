/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2019  MaNGOS project <https://getmangos.eu>
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

#include "ObjectAccessor.h"
#include "World.h"

#include <forward_list>


Unit*
ObjectAccessor::GetUnit(WorldObject const& u, ObjectGuid guid)
{
    if (!guid || !u.IsInWorld())
        { return nullptr; }

    if (guid.IsPlayer())
        { return FindPlayer(guid); }

    return u.GetMap()->GetAnyTypeCreature(guid);
}

Player*
ObjectAccessor::FindPlayer(ObjectGuid guid, bool inWorld /*= true*/)
{
    if (!guid)
        { return nullptr;}

    return m_playersMap.FindWith([&guid, &inWorld](const ObjectGuid& g, Player* plr)->bool
    {
        return ((g == guid) && (plr->IsInWorld() || !inWorld));
    });
}

Player*
ObjectAccessor::FindPlayerByName(const char* name)
{
    return m_playersMap.FindWith([name](const ObjectGuid& g, Player* plr)->bool
    {
        return (plr->IsInWorld() && (::strcmp(name, plr->GetName()) == 0));
    });
}

void
ObjectAccessor::SaveAllPlayers()
{
    for (auto const& iter : sWorld.GetAllSessions())
    {
        if (Player* player = iter.second->GetPlayer())
        {
            if (player->IsInWorld())
                player->SaveToDB();
        }
    }
}

void
ObjectAccessor::KickPlayer(ObjectGuid guid)
{
    if (Player* p = FindPlayer(guid, false))
    {
        WorldSession* s = p->GetSession();
        s->KickPlayer();                            // mark session to remove at next session list update
        s->LogoutPlayer(false);                     // logout player without waiting next session list update
    }
}


Corpse*
ObjectAccessor::FindCorpse(ObjectGuid guid)
{
    return m_corpsesMap.Find(guid);
}

Corpse*
ObjectAccessor::GetCorpseInMap(ObjectGuid guid, uint32 mapid)
{
    Corpse* ret = m_corpsesMap.Find(guid);
    if (!ret)
        { return nullptr; }
    if (ret->GetMapId() != mapid)
        { return nullptr; }
    return ret;
}

Corpse*
ObjectAccessor::GetCorpseForPlayerGUID(ObjectGuid guid)
{
    Corpse* c = m_player2corpse.Find(guid);

    if (!c)
        { return nullptr; }

    MANGOS_ASSERT(c->GetType() != CORPSE_BONES);
    return c;
}


void Player2Corpse::Remove(Corpse* corpse)
{
    ACE_WRITE_GUARD(LockType, guard, i_lock)

    if( m_objectMap.end() == m_objectMap.find(corpse->GetOwnerGuid()))
        return;

    // build mapid*cellid -> guid_set map
    CellPair cell_pair = MaNGOS::ComputeCellPair(corpse->GetPositionX(), corpse->GetPositionY());
    uint32 cell_id = (cell_pair.y_coord * TOTAL_NUMBER_OF_CELLS_PER_MAP) + cell_pair.x_coord;

    sObjectMgr.DeleteCorpseCellData(corpse->GetMapId(), cell_id, corpse->GetObjectGuid().GetCounter());
    corpse->RemoveFromWorld();

    m_objectMap.erase(corpse->GetOwnerGuid());
}


void
ObjectAccessor::RemoveCorpse(Corpse* corpse)
{
    MANGOS_ASSERT(corpse && corpse->GetType() != CORPSE_BONES);
    m_player2corpse.Remove(corpse);

}

void
Player2Corpse::Insert(Corpse* corpse)
{
    ACE_WRITE_GUARD(LockType, guard, i_lock)
    m_objectMap[corpse->GetOwnerGuid()] = corpse;

    CellPair cell_pair = MaNGOS::ComputeCellPair(corpse->GetPositionX(), corpse->GetPositionY());
    uint32 cell_id = (cell_pair.y_coord * TOTAL_NUMBER_OF_CELLS_PER_MAP) + cell_pair.x_coord;

    sObjectMgr.AddCorpseCellData(corpse->GetMapId(), cell_id, corpse->GetOwnerGuid().GetCounter(), corpse->GetInstanceId());
}

void
ObjectAccessor::AddCorpse(Corpse* corpse)
{
    MANGOS_ASSERT(corpse && corpse->GetType() != CORPSE_BONES);
    m_player2corpse.Insert(corpse);
}

void
ObjectAccessor::AddCorpsesToGrid(GridPair const& gridpair, GridType& grid, Map* map)
{
    m_player2corpse.Do([&gridpair, &grid, map](Corpse* c) -> void
    {
      if (c->GetGrid() == gridpair)
      {
          // verify, if the corpse in our instance (add only corpses which are)
          if (map->Instanceable())
          {
              if (c->GetInstanceId() == map->GetInstanceId())
              {
                  grid.AddWorldObject(c);
              }
          }
          else
          {
              grid.AddWorldObject(c);
          }
      }
    });
}

Corpse*
ObjectAccessor::ConvertCorpseForPlayer(ObjectGuid player_guid, bool insignia)
{
    Corpse* corpse = GetCorpseForPlayerGUID(player_guid);
    if (!corpse)
    {
        return nullptr;
    }

    DEBUG_LOG("Deleting Corpse and spawning bones.");

    // remove corpse from player_guid -> corpse map
    RemoveCorpse(corpse);

    // remove resurrectable corpse from grid object registry (loaded state checked into call)
    // do not load the map if it's not loaded
    Map* map = sMapMgr.FindMap(corpse->GetMapId(), corpse->GetInstanceId());
    if (map)
        { map->Remove(corpse, false); }

    // remove corpse from DB
    corpse->DeleteFromDB();

    Corpse* bones = nullptr;
    // create the bones only if the map and the grid is loaded at the corpse's location
    // ignore bones creating option in case insignia
    if ( map &&
        (insignia || (map->IsBattleGroundOrArena() ? sWorld.getConfig(CONFIG_BOOL_DEATH_BONES_BG_OR_ARENA) : sWorld.getConfig(CONFIG_BOOL_DEATH_BONES_WORLD))) &&
        !map->IsRemovalGrid(corpse->GetPositionX(), corpse->GetPositionY()))
    {
        // Create bones, don't change Corpse
        bones = new Corpse;
        bones->Create(corpse->GetGUIDLow());

        for (int i = 3; i < CORPSE_END; ++i)                // don't overwrite guid and object type
            { bones->SetUInt32Value(i, corpse->GetUInt32Value(i)); }

        bones->SetGrid(corpse->GetGrid());
        bones->Relocate(corpse->GetPositionX(), corpse->GetPositionY(), corpse->GetPositionZ(), corpse->GetOrientation());
        bones->SetPhaseMask(corpse->GetPhaseMask(), false);
        bones->SetUInt32Value(CORPSE_FIELD_FLAGS, CORPSE_FLAG_UNK2 | CORPSE_FLAG_BONES);
        bones->SetGuidValue(CORPSE_FIELD_OWNER, ObjectGuid());

        for (int i = 0; i < EQUIPMENT_SLOT_END; ++i)
        {
            if (corpse->GetUInt32Value(CORPSE_FIELD_ITEM + i))
                { bones->SetUInt32Value(CORPSE_FIELD_ITEM + i, 0); }
        }

        // add bones in grid store if grid loaded where corpse placed
        map->Add(bones);
    }

    // all references to the corpse should be removed at this point
    delete corpse;

    return bones;
}

void
ObjectAccessor::RemoveOldCorpses()
{
    time_t now = time(NULL);
    std::forward_list<ObjectGuid> expired_corpses{};

    m_player2corpse.Do([&now, &expired_corpses](Corpse* c)->void
    {
        if (c->IsExpired(now))
            expired_corpses.emplace_front(c->GetOwnerGuid());
    });

    for (auto g : expired_corpses)
    {
        ConvertCorpseForPlayer(g);
    }
}

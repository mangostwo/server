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

#ifndef MANGOS_OBJECTACCESSOR_H
#define MANGOS_OBJECTACCESSOR_H

#include <ace/RW_Thread_Mutex.h>
#include "Player.h"
#include "Corpse.h"

template <class T>
class HashMapHolder
{
    public:
        HashMapHolder(): i_lock{}, m_objectMap{}
        {}

        void Insert(T* o)
        {
            ACE_WRITE_GUARD(LockType, guard, i_lock)
            m_objectMap[o->GetObjectGuid()] = o;
        }

        void Remove(T* o)
        {
            ACE_WRITE_GUARD(LockType, guard, i_lock)
            m_objectMap.erase(o->GetObjectGuid());
        }

        T* Find(ObjectGuid guid)
        {
            ACE_READ_GUARD_RETURN (LockType, guard, i_lock, nullptr)
            auto const itr = m_objectMap.find(guid);
            return (itr != m_objectMap.end()) ? itr->second : nullptr;
        }

        // bool Predicate(const ObjectGuid& guid, T*)
        template <typename F>
        T* FindWith(F&& pred)
        {
            ACE_READ_GUARD_RETURN (LockType, guard, i_lock, nullptr)
            for(auto const& itr : m_objectMap)
            {
                if(std::forward<F>(pred)(itr.first, itr.second))
                {
                    return itr.second;
                }
            }
            return nullptr;
        }

        // void Worker(const T*)
        template <typename F>
        void Do(F&& work)
        {
            ACE_READ_GUARD(LockType, guard, i_lock)
            for(auto const& itr : m_objectMap)
            {
                std::forward<F>(work)(itr.second);
            }
        }

    protected:
        using LockType = ACE_RW_Thread_Mutex;
        LockType i_lock;
        std::unordered_map<ObjectGuid, T*> m_objectMap;
};

class Player2Corpse: public HashMapHolder<Corpse>
{
    public:
        Player2Corpse() : HashMapHolder<Corpse>(){}
        ~Player2Corpse()
        {
            for (auto& itr : m_objectMap)
            {
                itr.second->RemoveFromWorld();
                delete itr.second;
            }
        }

        void Insert(Corpse* o);
        void Remove(Corpse* o);
};


class ObjectAccessor
{
        ObjectAccessor(const ObjectAccessor&) = delete;
        ObjectAccessor& operator=(const ObjectAccessor&) = delete;

    public:
        ObjectAccessor(): m_playersMap{}, m_corpsesMap{}, m_player2corpse{}
        {}

        // Search player at any map in world and other objects at same map with `obj`
        // Note: recommended use Map::GetUnit version if player also expected at same map only
        Unit* GetUnit(WorldObject const& obj, ObjectGuid guid);

        // Player access
        Player* FindPlayer(ObjectGuid guid, bool inWorld = true);// if need player at specific map better use Map::GetPlayer
        Player* FindPlayerByName(const char* name);
        void KickPlayer(ObjectGuid guid);
        void SaveAllPlayers();

        template <typename F>
        void DoForAllPlayers(F&& pred)
        {
            m_playersMap.Do(pred);
        }

        // Corpse access
        Corpse* FindCorpse(ObjectGuid guid);
        void RemoveCorpse(Corpse* corpse);
        void AddCorpse(Corpse* corpse);
        void AddCorpsesToGrid(GridPair const& gridpair, GridType& grid, Map* map);
        Corpse* GetCorpseForPlayerGUID(ObjectGuid guid);
        Corpse* GetCorpseInMap(ObjectGuid guid, uint32 mapid);
        Corpse* ConvertCorpseForPlayer(ObjectGuid player_guid, bool insignia = false);
        void RemoveOldCorpses();

        // For call from Player/Corpse AddToWorld/RemoveFromWorld only
        void AddObject(Corpse* object) { m_corpsesMap.Insert(object); }
        void AddObject(Player* object) { m_playersMap.Insert(object); }
        void RemoveObject(Corpse* object) { m_corpsesMap.Remove(object); }
        void RemoveObject(Player* object) { m_playersMap.Remove(object); }

        static ObjectAccessor& Instance()
        {
            static ObjectAccessor instance{};
            return instance;
        }

    private:
        HashMapHolder<Player> m_playersMap;
        char _cg1[256];  //cache guard 1
        HashMapHolder<Corpse> m_corpsesMap;
        char _cg2[256]; //cache guard 2
        Player2Corpse         m_player2corpse;
};

#define sObjectAccessor ObjectAccessor::Instance()

#endif

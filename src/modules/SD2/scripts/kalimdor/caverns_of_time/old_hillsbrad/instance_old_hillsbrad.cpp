/**
 * ScriptDev2 is an extension for mangos providing enhanced features for
 * area triggers, creatures, game objects, instances, items, and spells beyond
 * the default database scripting in mangos.
 *
 * Copyright (C) 2006-2013  ScriptDev2 <http://www.scriptdev2.com/>
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

/* ScriptData
SDName: Instance_Old_Hillsbrad
SD%Complete: 75
SDComment: Thrall reset on server restart is not supported, because of core limitation.
SDCategory: Caverns of Time, Old Hillsbrad Foothills
EndScriptData */

#include "precompiled.h"
#include "old_hillsbrad.h"

static const float afInstanceLoc[][4] =
{
    { 2104.51f, 91.96f, 53.14f, 0 },                  // right orcs outside loc
    { 2192.58f, 238.44f, 52.44f, 0 },                 // left orcs outside loc
};

static const float aDrakeSummonLoc[4] = { 2128.43f, 71.01f, 64.42f, 1.74f };

struct is_old_hillsbrad : public InstanceScript
{
    is_old_hillsbrad() : InstanceScript("instance_old_hillsbrad") {}

    class instance_old_hillsbrad : public ScriptedInstance
    {
    public:
        instance_old_hillsbrad(Map* pMap) : ScriptedInstance(pMap),
            m_uiBarrelCount(0),
            m_uiThrallEventCount(0),
            m_uiThrallResetTimer(0)
        {
            Initialize();
        }

        ~instance_old_hillsbrad() {}

        void Initialize() override
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
        }

        void OnPlayerEnter(Player* pPlayer) override
        {
            // ToDo: HandleThrallRelocation();
            // Note: this isn't yet supported because of the grid load / unload

            // Spawn Drake if necessary
            if (GetData(TYPE_DRAKE) == DONE || GetData(TYPE_BARREL_DIVERSION) != DONE)
            {
                return;
            }

            if (GetSingleCreatureFromStorage(NPC_DRAKE, true))
            {
                return;
            }

            pPlayer->SummonCreature(NPC_DRAKE, aDrakeSummonLoc[0], aDrakeSummonLoc[1], aDrakeSummonLoc[2], aDrakeSummonLoc[3], TEMPSUMMON_DEAD_DESPAWN, 0);
        }

        void OnCreatureCreate(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_THRALL:
            case NPC_TARETHA:
            case NPC_EROZION:
            case NPC_ARMORER:
            case NPC_TARREN_MILL_PROTECTOR:
            case NPC_TARREN_MILL_LOOKOUT:
            case NPC_YOUNG_BLANCHY:
            case NPC_DRAKE:
            case NPC_SKARLOC:
            case NPC_EPOCH:
                m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
                break;
            case NPC_ORC_PRISONER:
                // Sort the orcs which are inside the houses
                if (pCreature->GetPositionZ() > 53.4f)
                {
                    if (pCreature->GetPositionY() > 150.0f)
                    {
                        m_lLeftPrisonersList.push_back(pCreature->GetObjectGuid());
                    }
                    else
                    {
                        m_lRightPrisonersList.push_back(pCreature->GetObjectGuid());
                    }
                }
                break;
            }
        }

        void OnObjectCreate(GameObject* pGo) override
        {
            if (pGo->GetEntry() == GO_ROARING_FLAME)
            {
                m_lRoaringFlamesList.push_back(pGo->GetObjectGuid());
            }
            else if (pGo->GetEntry() == GO_PRISON_DOOR)
            {
                m_mGoEntryGuidStore[GO_PRISON_DOOR] = pGo->GetObjectGuid();
            }
        }

        void OnCreatureEnterCombat(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_DRAKE:
                SetData(TYPE_DRAKE, IN_PROGRESS);
                DoUpdateWorldState(WORLD_STATE_OH, 0);
                break;
            case NPC_SKARLOC: SetData(TYPE_SKARLOC, IN_PROGRESS); break;
            case NPC_EPOCH:   SetData(TYPE_EPOCH, IN_PROGRESS);   break;
            }
        }

        void OnCreatureEvade(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_DRAKE:   SetData(TYPE_DRAKE, FAIL);   break;
            case NPC_SKARLOC: SetData(TYPE_SKARLOC, FAIL); break;
            case NPC_EPOCH:   SetData(TYPE_EPOCH, FAIL);   break;
            }
        }

        void OnCreatureDeath(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_DRAKE:   SetData(TYPE_DRAKE, DONE);   break;
            case NPC_SKARLOC: SetData(TYPE_SKARLOC, DONE); break;
            case NPC_EPOCH:   SetData(TYPE_EPOCH, DONE);   break;
            }
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            switch (uiType)
            {
            case TYPE_BARREL_DIVERSION:
                m_auiEncounter[uiType] = uiData;
                if (uiData == IN_PROGRESS)
                {
                    if (m_uiBarrelCount >= MAX_BARRELS)
                    {
                        return;
                    }

                    // Update barrels used and world state
                    ++m_uiBarrelCount;
                    DoUpdateWorldState(WORLD_STATE_OH, m_uiBarrelCount);

                    debug_log("SD2: Instance Old Hillsbrad: go_barrel_old_hillsbrad count %u", m_uiBarrelCount);

                    // Set encounter to done, and spawn Liutenant Drake
                    if (m_uiBarrelCount == MAX_BARRELS)
                    {
                        UpdateLodgeQuestCredit();

                        if (Player* pPlayer = GetPlayerInMap())
                        {
                            pPlayer->SummonCreature(NPC_DRAKE, aDrakeSummonLoc[0], aDrakeSummonLoc[1], aDrakeSummonLoc[2], aDrakeSummonLoc[3], TEMPSUMMON_DEAD_DESPAWN, 0);

                            // set the houses on fire
                            for (GuidList::const_iterator itr = m_lRoaringFlamesList.begin(); itr != m_lRoaringFlamesList.end(); ++itr)
                            {
                                DoRespawnGameObject(*itr, 30 * MINUTE);
                            }

                            // move the orcs outside the houses
                            float fX, fY, fZ;
                            for (GuidList::const_iterator itr = m_lRightPrisonersList.begin(); itr != m_lRightPrisonersList.end(); ++itr)
                            {
                                if (Creature* pOrc = instance->GetCreature(*itr))
                                {
                                    pOrc->GetRandomPoint(afInstanceLoc[0][0], afInstanceLoc[0][1], afInstanceLoc[0][2], 10.0f, fX, fY, fZ);
                                    pOrc->SetWalk(false);
                                    pOrc->GetMotionMaster()->MovePoint(0, fX, fY, fZ);
                                }
                            }
                            for (GuidList::const_iterator itr = m_lLeftPrisonersList.begin(); itr != m_lLeftPrisonersList.end(); ++itr)
                            {
                                if (Creature* pOrc = instance->GetCreature(*itr))
                                {
                                    pOrc->GetRandomPoint(afInstanceLoc[1][0], afInstanceLoc[1][1], afInstanceLoc[1][2], 10.0f, fX, fY, fZ);
                                    pOrc->SetWalk(false);
                                    pOrc->GetMotionMaster()->MovePoint(0, fX, fY, fZ);
                                }
                            }
                        }
                        else
                        {
                            debug_log("SD2: Instance Old Hillsbrad: SetData (Type: %u Data %u) cannot find any pPlayer.", uiType, uiData);
                        }

                        SetData(TYPE_BARREL_DIVERSION, DONE);
                    }
                }
                break;
            case TYPE_THRALL_EVENT:
                // nothing to do if already done and thrall respawn
                if (GetData(TYPE_THRALL_EVENT) == DONE)
                {
                    return;
                }
                m_auiEncounter[uiType] = uiData;
                if (uiData == FAIL)
                {
                    // despawn the bosses if necessary
                    if (Creature* pSkarloc = GetSingleCreatureFromStorage(NPC_SKARLOC, true))
                    {
                        pSkarloc->ForcedDespawn();
                    }
                    if (Creature* pEpoch = GetSingleCreatureFromStorage(NPC_EPOCH, true))
                    {
                        pEpoch->ForcedDespawn();
                    }

                    if (m_uiThrallEventCount <= MAX_WIPE_COUNTER)
                    {
                        ++m_uiThrallEventCount;
                        debug_log("SD2: Instance Old Hillsbrad: Thrall event failed %u times.", m_uiThrallEventCount);

                        // reset Thrall on timer
                        m_uiThrallResetTimer = 30000;
                    }
                    // If we already respawned Thrall too many times, the event is failed for good
                    else if (m_uiThrallEventCount > MAX_WIPE_COUNTER)
                    {
                        debug_log("SD2: Instance Old Hillsbrad: Thrall event failed %u times. Reset instance required.", m_uiThrallEventCount);
                    }
                }
                break;
            case TYPE_DRAKE:
            case TYPE_SKARLOC:
            case TYPE_ESCORT_BARN:
            case TYPE_ESCORT_INN:
            case TYPE_EPOCH:
                m_auiEncounter[uiType] = uiData;
                debug_log("SD2: Instance Old Hillsbrad: Thrall event type %u adjusted to data %u.", uiType, uiData);
                break;
            }

            if (uiData == DONE)
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << m_auiEncounter[0] << " " << m_auiEncounter[1] << " " << m_auiEncounter[2] << " "
                    << m_auiEncounter[3] << " " << m_auiEncounter[4] << " " << m_auiEncounter[5] << " "
                    << m_auiEncounter[6];

                m_strInstData = saveStream.str();

                SaveToDB();
                OUT_SAVE_INST_DATA_COMPLETE;
            }
        }

        uint32 GetData(uint32 uiType) const override
        {
            if (uiType < MAX_ENCOUNTER)
            {
                return m_auiEncounter[uiType];
            }

            if (uiType == TYPE_THRALL_EVENT_COUNT)
                return m_uiThrallEventCount;

            return 0;
        }

        uint32 GetThrallEventCount() { return m_uiThrallEventCount; }

        const char* Save() const override { return m_strInstData.c_str(); }
        void Load(const char* chrIn) override
        {
            if (!chrIn)
            {
                OUT_LOAD_INST_DATA_FAIL;
                return;
            }

            OUT_LOAD_INST_DATA(chrIn);

            std::istringstream loadStream(chrIn);
            loadStream >> m_auiEncounter[0] >> m_auiEncounter[1] >> m_auiEncounter[2] >> m_auiEncounter[3]
                >> m_auiEncounter[4] >> m_auiEncounter[5] >> m_auiEncounter[6];

            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            {
                if (m_auiEncounter[i] == IN_PROGRESS)
                {
                    m_auiEncounter[i] = NOT_STARTED;
                }
            }

            // custom reload - if the escort event or the Epoch event are not done, then reset the escort
            // this is done, because currently we cannot handle Thrall relocation on server reset
            if (m_auiEncounter[5] != DONE)
            {
                m_auiEncounter[2] = NOT_STARTED;
                m_auiEncounter[3] = NOT_STARTED;
                m_auiEncounter[4] = NOT_STARTED;
            }

            OUT_LOAD_INST_DATA_COMPLETE;
        }

        void Update(uint32 uiDiff) override
        {
            if (m_uiThrallResetTimer)
            {
                if (m_uiThrallResetTimer <= uiDiff)
                {
                    HandleThrallRelocation();
                    m_uiThrallResetTimer = 0;
                }
                else
                {
                    m_uiThrallResetTimer -= uiDiff;
                }
            }
        }

    protected:
        void HandleThrallRelocation()
        {
            // reset instance data
            SetData(TYPE_THRALL_EVENT, IN_PROGRESS);

            if (Creature* pThrall = GetSingleCreatureFromStorage(NPC_THRALL))
            {
                debug_log("SD2: Instance Old Hillsbrad: Thrall relocation");

                if (!pThrall->IsAlive())
                {
                    pThrall->Respawn();
                }

                // epoch failed, reloc to inn
                if (GetData(TYPE_ESCORT_INN) == DONE)
                {
                    pThrall->GetMap()->CreatureRelocation(pThrall, 2660.57f, 659.173f, 61.9370f, 5.76f);
                }
                // barn to inn failed, reloc to barn
                else if (GetData(TYPE_ESCORT_BARN) == DONE)
                {
                    pThrall->GetMap()->CreatureRelocation(pThrall, 2486.91f, 626.356f, 58.0761f, 4.66f);
                }
                // keep to barn failed, reloc to keep
                else if (GetData(TYPE_SKARLOC) == DONE)
                {
                    pThrall->GetMap()->CreatureRelocation(pThrall, 2063.40f, 229.509f, 64.4883f, 2.23f);
                }
                // prison to keep failed, reloc to prison
                else
                {
                    pThrall->GetMap()->CreatureRelocation(pThrall, 2231.89f, 119.95f, 82.2979f, 4.21f);
                }
            }
        }

        void UpdateLodgeQuestCredit()
        {
            Map::PlayerList const& players = instance->GetPlayers();

            if (!players.isEmpty())
            {
                for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                {
                    if (Player* pPlayer = itr->getSource())
                    {
                        pPlayer->KilledMonsterCredit(NPC_LODGE_QUEST_TRIGGER);
                    }
                }
            }
        }

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strInstData;

        uint32 m_uiBarrelCount;
        uint32 m_uiThrallEventCount;
        uint32 m_uiThrallResetTimer;

        GuidList m_lRoaringFlamesList;
        GuidList m_lLeftPrisonersList;
        GuidList m_lRightPrisonersList;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_old_hillsbrad(pMap);
    }
};

struct event_go_barrel_old_hillsbrad : public MapEventScript
{
    event_go_barrel_old_hillsbrad() : MapEventScript("event_go_barrel_old_hillsbrad") {}

    bool OnReceived(uint32 /*uiEventId*/, Object* pSource, Object* pTarget, bool bIsStart) override
    {
        if (bIsStart && pSource->GetTypeId() == TYPEID_PLAYER)
        {
            if (InstanceData* pInstance = ((Player*)pSource)->GetInstanceData())
            {
                if (pInstance->GetData(TYPE_BARREL_DIVERSION) == DONE)
                {
                    return true;
                }

                pInstance->SetData(TYPE_BARREL_DIVERSION, IN_PROGRESS);

                // Don't allow players to use same object twice
                if (pTarget->GetTypeId() == TYPEID_GAMEOBJECT)
                {
                    ((GameObject*)pTarget)->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NO_INTERACT);
                }

                return true;
            }
        }
        return false;
    }
};

void AddSC_instance_old_hillsbrad()
{
    Script* s;

    s = new is_old_hillsbrad();
    s->RegisterSelf();
    s = new event_go_barrel_old_hillsbrad();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_old_hillsbrad";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_old_hillsbrad;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "event_go_barrel_old_hillsbrad";
    //pNewScript->pProcessEventId = &ProcessEventId_event_go_barrel_old_hillsbrad;
    //pNewScript->RegisterSelf();
}

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
SDName: Instance_Blood_Furnace
SD%Complete: 75
SDComment:
SDCategory: Blood Furnace
EndScriptData */

#include "precompiled.h"
#include "blood_furnace.h"

enum
{
    SPELL_CHANNELING = 39123,
};

// Random Magtheridon taunt
static const int32 aRandomTaunt[] = { -1544000, -1544001, -1544002, -1544003, -1544004, -1544005 };

struct BroggokEventInfo
{
    BroggokEventInfo() : m_bIsCellOpened(false), m_uiKilledOrcCount(0) {}

    ObjectGuid m_cellGuid;
    bool m_bIsCellOpened;
    uint8 m_uiKilledOrcCount;
    GuidSet m_sSortedOrcGuids;
};

struct SortByAngle
{
    SortByAngle(WorldObject const* pRef) : m_pRef(pRef) {}
    bool operator()(WorldObject* pLeft, WorldObject* pRight)
    {
        return m_pRef->GetAngle(pLeft) < m_pRef->GetAngle(pRight);
    }
    WorldObject const* m_pRef;
};

struct is_blood_furnace : public InstanceScript
{
    is_blood_furnace() : InstanceScript("instance_blood_furnace") {}

    class instance_blood_furnace : public ScriptedInstance
    {
    public:
        instance_blood_furnace(Map* pMap) : ScriptedInstance(pMap),
            m_uiBroggokEventTimer(30000),
            m_uiBroggokEventPhase(0),
            m_uiRandYellTimer(90000)
        {
            Initialize();
        }

        void Initialize() override
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
        }

        void OnCreatureCreate(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_BROGGOK:
            case NPC_KELIDAN_THE_BREAKER:
            case NPC_MAGTHERIDON:
                m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
                break;

            case NPC_NASCENT_FEL_ORC:
                m_luiNascentOrcGuids.push_back(pCreature->GetObjectGuid());
                break;
            case NPC_SHADOWMOON_CHANNELER:
                m_lChannelersGuids.push_back(pCreature->GetObjectGuid());
                break;
            }
        }

        void OnObjectCreate(GameObject* pGo) override
        {
            switch (pGo->GetEntry())
            {
            case GO_DOOR_MAKER_FRONT:                           // the maker front door
                break;
            case GO_DOOR_MAKER_REAR:                            // the maker rear door
                if (m_auiEncounter[TYPE_THE_MAKER_EVENT] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_DOOR_BROGGOK_FRONT:                         // broggok front door
                break;
            case GO_DOOR_BROGGOK_REAR:                          // broggok rear door
                if (m_auiEncounter[TYPE_BROGGOK_EVENT] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_DOOR_KELIDAN_EXIT:                          // kelidan exit door
                if (m_auiEncounter[TYPE_KELIDAN_EVENT] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_DOOR_FINAL_EXIT:                            // final exit door
                if (m_auiEncounter[TYPE_KELIDAN_EVENT] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;

            case GO_PRISON_CELL_BROGGOK_1: m_aBroggokEvent[0].m_cellGuid = pGo->GetObjectGuid(); return;
            case GO_PRISON_CELL_BROGGOK_2: m_aBroggokEvent[1].m_cellGuid = pGo->GetObjectGuid(); return;
            case GO_PRISON_CELL_BROGGOK_3: m_aBroggokEvent[2].m_cellGuid = pGo->GetObjectGuid(); return;
            case GO_PRISON_CELL_BROGGOK_4: m_aBroggokEvent[3].m_cellGuid = pGo->GetObjectGuid(); return;

            default:
                return;
            }
            m_mGoEntryGuidStore[pGo->GetEntry()] = pGo->GetObjectGuid();
        }

        void OnCreatureDeath(Creature* pCreature) override
        {
            if (m_auiEncounter[TYPE_BROGGOK_EVENT] != IN_PROGRESS)
            {
                return;
            }

            if (pCreature->GetEntry() == NPC_NASCENT_FEL_ORC)
            {
                uint8 uiClearedCells = 0;
                for (uint8 i = 0; i < std::min<uint32>(m_uiBroggokEventPhase, MAX_ORC_WAVES); ++i)
                {
                    if (m_aBroggokEvent[i].m_sSortedOrcGuids.size() == m_aBroggokEvent[i].m_uiKilledOrcCount)
                    {
                        ++uiClearedCells;
                        continue;
                    }

                    // Increase kill counter, if we found a mob of this cell
                    if (m_aBroggokEvent[i].m_sSortedOrcGuids.find(pCreature->GetObjectGuid()) != m_aBroggokEvent[i].m_sSortedOrcGuids.end())
                    {
                        m_aBroggokEvent[i].m_uiKilledOrcCount++;
                    }

                    if (m_aBroggokEvent[i].m_sSortedOrcGuids.size() == m_aBroggokEvent[i].m_uiKilledOrcCount)
                    {
                        ++uiClearedCells;
                    }
                }

                // Increase phase when all opened cells are cleared
                if (uiClearedCells == m_uiBroggokEventPhase)
                {
                    DoNextBroggokEventPhase();
                }
            }
        }

        void OnCreatureEvade(Creature* pCreature) override
        {
            if (m_auiEncounter[TYPE_BROGGOK_EVENT] == FAIL)
            {
                return;
            }

            if (pCreature->GetEntry() == NPC_BROGGOK)
            {
                SetData(TYPE_BROGGOK_EVENT, FAIL);
            }

            else if (pCreature->GetEntry() == NPC_NASCENT_FEL_ORC)
            {
                for (uint8 i = 0; i < std::min<uint32>(m_uiBroggokEventPhase, MAX_ORC_WAVES); ++i)
                {
                    if (m_aBroggokEvent[i].m_sSortedOrcGuids.find(pCreature->GetObjectGuid()) != m_aBroggokEvent[i].m_sSortedOrcGuids.end())
                    {
                        SetData(TYPE_BROGGOK_EVENT, FAIL);
                    }
                }
            }
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            switch (uiType)
            {
            case TYPE_THE_MAKER_EVENT:
                if (uiData == IN_PROGRESS)
                {
                    DoUseDoorOrButton(GO_DOOR_MAKER_FRONT);
                }
                if (uiData == FAIL)
                {
                    DoUseDoorOrButton(GO_DOOR_MAKER_FRONT);
                }
                if (uiData == DONE)
                {
                    DoUseDoorOrButton(GO_DOOR_MAKER_FRONT);
                    DoUseDoorOrButton(GO_DOOR_MAKER_REAR);
                }
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_BROGGOK_EVENT:
                if (m_auiEncounter[uiType] == uiData)
                {
                    return;
                }

                // Combat door; the exit door is opened in event
                DoUseDoorOrButton(GO_DOOR_BROGGOK_FRONT);
                if (uiData == IN_PROGRESS)
                {
                    if (m_uiBroggokEventPhase <= MAX_ORC_WAVES)
                    {
                        m_uiBroggokEventPhase = 0;
                        DoSortBroggokOrcs();
                        // open first cage
                        DoNextBroggokEventPhase();
                    }
                }
                else if (uiData == FAIL)
                {
                    // On wipe we reset only the orcs; if the party wipes at the boss itself then the orcs don't reset
                    if (m_uiBroggokEventPhase <= MAX_ORC_WAVES)
                    {
                        for (uint8 i = 0; i < MAX_ORC_WAVES; ++i)
                        {
                            // Reset Orcs
                            if (!m_aBroggokEvent[i].m_bIsCellOpened)
                            {
                                continue;
                            }

                            m_aBroggokEvent[i].m_uiKilledOrcCount = 0;
                            for (GuidSet::const_iterator itr = m_aBroggokEvent[i].m_sSortedOrcGuids.begin(); itr != m_aBroggokEvent[i].m_sSortedOrcGuids.end(); ++itr)
                            {
                                if (Creature* pOrc = instance->GetCreature(*itr))
                                {
                                    if (!pOrc->IsAlive())
                                    {
                                        pOrc->Respawn();
                                    }
                                }
                            }

                            // Close Door
                            DoUseDoorOrButton(m_aBroggokEvent[i].m_cellGuid);
                            m_aBroggokEvent[i].m_bIsCellOpened = false;
                        }
                    }
                }
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_KELIDAN_EVENT:
                if (uiData == DONE)
                {
                    DoUseDoorOrButton(GO_DOOR_KELIDAN_EXIT);
                    DoUseDoorOrButton(GO_DOOR_FINAL_EXIT);
                }
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_ADD_DO_SETUP:
            {
                GuidList lAddGuids = m_lChannelersGuids;
                m_lChannelersGuids.clear();

                // Sort Adds to vector if not already done
                if (!lAddGuids.empty())
                {
                    m_vAddGuids.reserve(lAddGuids.size());
                    std::list<Creature*> lAdds;
                    for (GuidList::const_iterator itr = lAddGuids.begin(); itr != lAddGuids.end(); ++itr)
                    {
                        if (Creature* pAdd = instance->GetCreature(*itr))
                        {
                            lAdds.push_back(pAdd);
                        }
                    }
                    // Sort them by angle
                    lAdds.sort(SortByAngle(GetSingleCreatureFromStorage(NPC_KELIDAN_THE_BREAKER)));
                    for (std::list<Creature*>::const_iterator itr = lAdds.begin(); itr != lAdds.end(); ++itr)
                    {
                        m_vAddGuids.push_back((*itr)->GetObjectGuid());
                    }
                }

                // Respawn killed adds
                for (GuidVector::const_iterator itr = m_vAddGuids.begin(); itr != m_vAddGuids.end(); ++itr)
                {
                    Creature* pAdd = instance->GetCreature(*itr);
                    if (pAdd && !pAdd->IsAlive())
                    {
                        pAdd->Respawn();
                    }
                }

                // Cast pentagram
                uint8 s = m_vAddGuids.size();
                for (uint8 i = 0; i < s; ++i)
                {
                    Creature* pCaster = instance->GetCreature(m_vAddGuids[i]);
                    Creature* pTarget = instance->GetCreature(m_vAddGuids[(i + 2) % s]);
                    if (pCaster && pTarget)
                    {
                        pCaster->CastSpell(pTarget, SPELL_CHANNELING, false);
                    }
                }
                }
                return;
            case TYPE_BROGGOK_DO_MOVE:
                if (Creature *broggok = GetSingleCreatureFromStorage(NPC_BROGGOK))
                {
                    float dx, dy;
                    float fRespX, fRespY, fRespZ;
                    broggok->GetRespawnCoord(fRespX, fRespY, fRespZ);
                    GetMovementDistanceForIndex(uiData, dx, dy);
                    broggok->GetMotionMaster()->MovePoint(POINT_EVENT_COMBAT, dx, dy, fRespZ);
                }
                return;
            default:
                script_error_log("Instance Blood Furnace SetData with Type %u Data %u, but this is not implemented.", uiType, uiData);
                return;
            }

            if (uiData == DONE)
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << m_auiEncounter[0] << " " << m_auiEncounter[1] << " " << m_auiEncounter[2];

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

            return 0;
        }

        void SetData64(uint32 type, uint64 data) override
        {
            switch (type)
            {
            case TYPE_ADD_AGGROED:
                if (Unit *victim = instance->GetUnit(ObjectGuid(data)))
                {
                    // Let all adds attack
                    for (GuidList::const_iterator itr = m_lChannelersGuids.begin(); itr != m_lChannelersGuids.end(); ++itr)
                    {
                        Creature* pAdd = instance->GetCreature(*itr);
                        if (pAdd && !pAdd->getVictim())
                        {
                            pAdd->AI()->AttackStart(victim);
                        }
                    }
                }
                break;
            default:
                break;
            }
        }

        void Update(uint32 uiDiff) override
        {
            // Broggok Event: For the last wave we don't check the timer; the boss is released only when all mobs die, also the timer is only active on heroic
            if (m_auiEncounter[TYPE_BROGGOK_EVENT] == IN_PROGRESS && m_uiBroggokEventPhase < MAX_ORC_WAVES && !instance->IsRegularDifficulty())
            {
                if (m_uiBroggokEventTimer < uiDiff)
                {
                    DoNextBroggokEventPhase();
                }
                else
                {
                    m_uiBroggokEventTimer -= uiDiff;
                }
            }

            if (m_uiRandYellTimer < uiDiff)
            {
                if (Creature* pMagtheridon = GetSingleCreatureFromStorage(NPC_MAGTHERIDON))
                {
                    DoScriptText(aRandomTaunt[urand(0, 5)], pMagtheridon);
                    m_uiRandYellTimer = 90000;
                }
            }
            else
            {
                m_uiRandYellTimer -= uiDiff;
            }
        }

        void Load(const char* chrIn) override
        {
            if (!chrIn)
            {
                OUT_LOAD_INST_DATA_FAIL;
                return;
            }

            OUT_LOAD_INST_DATA(chrIn);

            std::istringstream loadStream(chrIn);
            loadStream >> m_auiEncounter[0] >> m_auiEncounter[1] >> m_auiEncounter[2];

            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            if (m_auiEncounter[i] == IN_PROGRESS || m_auiEncounter[i] == FAIL)
            {
                m_auiEncounter[i] = NOT_STARTED;
            }

            OUT_LOAD_INST_DATA_COMPLETE;
        }

        const char* Save() const override { return m_strInstData.c_str(); }

        // Helper function to calculate the position to where the orcs should move
        // For case of orc-indexes the difference, for Braggok his position
        void GetMovementDistanceForIndex(uint32 uiIndex, float& dx, float& dy)
        {
            GameObject* pDoor[2];

            if (uiIndex < MAX_ORC_WAVES)
            {
                // Use doors 0, 1 for index 0 or 1; and use doors 2, 3 for index 2 or 3
                pDoor[0] = instance->GetGameObject(m_aBroggokEvent[(uiIndex / 2) * 2].m_cellGuid);
                pDoor[1] = instance->GetGameObject(m_aBroggokEvent[(uiIndex / 2) * 2 + 1].m_cellGuid);
            }
            else
            {
                // Use doors 0 and 3 for Braggok case (which means the middle point is the center of the room)
                pDoor[0] = instance->GetGameObject(m_aBroggokEvent[0].m_cellGuid);
                pDoor[1] = instance->GetGameObject(m_aBroggokEvent[3].m_cellGuid);
            }

            if (!pDoor[0] || !pDoor[1])
            {
                return;
            }

            if (uiIndex < MAX_ORC_WAVES)
            {
                dx = (pDoor[0]->GetPositionX() + pDoor[1]->GetPositionX()) / 2 - pDoor[uiIndex % 2]->GetPositionX();
                dy = (pDoor[0]->GetPositionY() + pDoor[1]->GetPositionY()) / 2 - pDoor[uiIndex % 2]->GetPositionY();
            }
            else
            {
                dx = (pDoor[0]->GetPositionX() + pDoor[1]->GetPositionX()) / 2;
                dy = (pDoor[0]->GetPositionY() + pDoor[1]->GetPositionY()) / 2;
            }
        }

    private:
        // Sort all nascent orcs in the instance in order to get only those near broggok doors
        void DoSortBroggokOrcs()
        {
            for (GuidList::const_iterator itr = m_luiNascentOrcGuids.begin(); itr != m_luiNascentOrcGuids.end(); ++itr)
            {
                if (Creature* pOrc = instance->GetCreature(*itr))
                {
                    for (uint8 i = 0; i < MAX_ORC_WAVES; ++i)
                    {
                        if (GameObject* pDoor = instance->GetGameObject(m_aBroggokEvent[i].m_cellGuid))
                        {
                            if (pOrc->IsWithinDistInMap(pDoor, 15.0f))
                            {
                                m_aBroggokEvent[i].m_sSortedOrcGuids.insert(pOrc->GetObjectGuid());
                                if (!pOrc->IsAlive())
                                {
                                    pOrc->Respawn();
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }

        void DoNextBroggokEventPhase()
        {
            // Get Movement Position
            float dx, dy;
            GetMovementDistanceForIndex(m_uiBroggokEventPhase, dx, dy);

            // Open door to the final boss now and move boss to the center of the room
            if (m_uiBroggokEventPhase >= MAX_ORC_WAVES)
            {
                DoUseDoorOrButton(GO_DOOR_BROGGOK_REAR);

                if (Creature* pBroggok = GetSingleCreatureFromStorage(NPC_BROGGOK))
                {
                    pBroggok->SetWalk(false);
                    pBroggok->GetMotionMaster()->MovePoint(0, dx, dy, pBroggok->GetPositionZ());
                }
            }
            else
            {
                // Open cage door
                if (!m_aBroggokEvent[m_uiBroggokEventPhase].m_bIsCellOpened)
                {
                    DoUseDoorOrButton(m_aBroggokEvent[m_uiBroggokEventPhase].m_cellGuid);
                }

                m_aBroggokEvent[m_uiBroggokEventPhase].m_bIsCellOpened = true;

                for (GuidSet::const_iterator itr = m_aBroggokEvent[m_uiBroggokEventPhase].m_sSortedOrcGuids.begin(); itr != m_aBroggokEvent[m_uiBroggokEventPhase].m_sSortedOrcGuids.end(); ++itr)
                {
                    if (Creature* pOrc = instance->GetCreature(*itr))
                    {
                        // Remove unit flags from npcs
                        pOrc->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        pOrc->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

                        // Move them out of the cages
                        pOrc->SetWalk(false);
                        pOrc->GetMotionMaster()->MovePoint(0, pOrc->GetPositionX() + dx, pOrc->GetPositionY() + dy, pOrc->GetPositionZ());
                    }
                }
            }

            // Prepare for further handling
            m_uiBroggokEventTimer = 30000;
            ++m_uiBroggokEventPhase;
        }

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strInstData;

        BroggokEventInfo m_aBroggokEvent[MAX_ORC_WAVES];

        uint32 m_uiBroggokEventTimer;                       // Timer for opening the event cages; only on heroic mode = 30 secs
        uint32 m_uiBroggokEventPhase;
        uint32 m_uiRandYellTimer;                           // Random yell for Magtheridon

        GuidList m_luiNascentOrcGuids;
        GuidList m_lChannelersGuids;
        GuidVector m_vAddGuids;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_blood_furnace(pMap);
    }
};

struct go_prison_cell_lever : public GameObjectScript
{
    go_prison_cell_lever() : GameObjectScript("go_prison_cell_lever") {}

    bool OnUse(Player* /*pPlayer*/, GameObject* pGo) override
    {
        ScriptedInstance* pInstance = (ScriptedInstance*)pGo->GetInstanceData();

        if (!pInstance)
        {
            return false;
        }

        // Set broggok event in progress
        if (pInstance->GetData(TYPE_BROGGOK_EVENT) != DONE && pInstance->GetData(TYPE_BROGGOK_EVENT) != IN_PROGRESS)
        {
            pInstance->SetData(TYPE_BROGGOK_EVENT, IN_PROGRESS);

            // Yell intro
            if (Creature* pBroggok = pInstance->GetSingleCreatureFromStorage(NPC_BROGGOK))
            {
                DoScriptText(SAY_BROGGOK_INTRO, pBroggok);
            }
        }

        return false;
    }
};

void AddSC_instance_blood_furnace()
{
    Script* s;

    s = new is_blood_furnace();
    s->RegisterSelf();
    s = new go_prison_cell_lever();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_blood_furnace";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_blood_furnace;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "go_prison_cell_lever";
    //pNewScript->pGOUse = &GOUse_go_prison_cell_lever;
    //pNewScript->RegisterSelf();
}

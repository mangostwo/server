/* Copyright (C) 2006 - 2013 ScriptDev2 <http://www.scriptdev2.com/>
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
 */

/* ScriptData
SDName: Instance_culling_of_stratholme
SD%Complete: 80%
SDComment:
SDCategory: Culling of Stratholme
EndScriptData */

#include "precompiled.h"
#include "culling_of_stratholme.h"

enum
{
    MAX_ARTHAS_SPAWN_POS        = 5,
    MAX_GRAIN_CRATES            = 5,
    SAY_CHROMIE_HURRY           = -1000000,                         // TODO

    WHISPER_CHROMIE_CRATES      = -1595001,
};

struct sSpawnLocation
{
    float m_fX, m_fY, m_fZ, m_fO;
};

static sSpawnLocation m_aArthasSpawnLocs[] =                // need tuning
{
    {1969.73f, 1287.12f, 145.48f, 3.14f},
    {2049.43f, 1287.43f, 142.75f, 0.06f},
    {2365.54f, 1194.85f, 131.98f, 0.47f},
    {2534.46f, 1125.99f, 130.75f, 0.27f},
    {2363.77f, 1406.31f, 128.64f, 3.23f}
};

static sSpawnLocation m_aChromieSpawnLocs[] =               // need tuning, escpecially EndPositions!
{
    {1814.46f, 1283.97f, 142.30f, 4.32f},                   // near bridge
    {2319.562f, 1506.409f, 152.0499f, 3.78f},               // End
    {1811.52f, 1285.92f, 142.37f, 4.47f},                   // Hourglass, near bridge
    {2186.42f, 1323.77f, 129.91f, 0.0f},                    // Hourglass, End
};

instance_culling_of_stratholme::instance_culling_of_stratholme(Map* pMap) : ScriptedInstance(pMap),
    m_bStartedInnEvent(false),
    m_uiGrainCrateCount(0),
    m_uiRemoveCrateStateTimer(0),
    m_uiArthasRespawnTimer(0)
{
    Initialize();
}

void instance_culling_of_stratholme::Initialize()
{
    memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
}

void instance_culling_of_stratholme::OnCreatureCreate(Creature* pCreature)
{
    switch (pCreature->GetEntry())
    {
        case NPC_CHROMIE_ENTRANCE:
        case NPC_CHROMIE_END:
        case NPC_ARTHAS:
        case NPC_MICHAEL_BELFAST:
        case NPC_HEARTHSINGER_FORRESTEN:
        case NPC_FRAS_SIABI:
        case NPC_FOOTMAN_JAMES:
        case NPC_MAL_CORRICKS:
        case NPC_GRYAN_STOUTMANTLE:
        case NPC_ROGER_OWENS:
        case NPC_SERGEANT_MORIGAN:
        case NPC_JENA_ANDERSON:
        case NPC_MALCOM_MOORE:
        case NPC_BARTLEBY_BATTSON:
        case NPC_PATRICIA_O_REILLY:
        case NPC_LORDAERON_CRIER:
        case NPC_INFINITE_CORRUPTER:
            m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
            break;

        case NPC_GRAIN_CRATE_HELPER:            m_luiCratesBunnyGUIDs.push_back(pCreature->GetObjectGuid()); break;
        case NPC_LORDAERON_FOOTMAN:             m_luiFootmanGUIDs.push_back(pCreature->GetObjectGuid());     break;

        case NPC_STRATHOLME_CITIZEN:
        case NPC_STRATHOLME_RESIDENT:
            if (m_auiEncounter[TYPE_ARTHAS_INTRO_EVENT] == DONE)
                pCreature->UpdateEntry(NPC_ZOMBIE);
            else
                m_luiResidentGUIDs.push_back(pCreature->GetObjectGuid());
            break;
        case NPC_AGIATED_STRATHOLME_CITIZEN:    m_lAgiatedCitizenGUIDList.push_back(pCreature->GetObjectGuid());  break;
        case NPC_AGIATED_STRATHOLME_RESIDENT:   m_lAgiatedResidentGUIDList.push_back(pCreature->GetObjectGuid()); break;
    }
}

void instance_culling_of_stratholme::OnObjectCreate(GameObject* pGo)
{
    switch (pGo->GetEntry())
    {
        case GO_DOOR_BOOKCASE:
            if (m_auiEncounter[TYPE_EPOCH_EVENT] == DONE)
                pGo->SetGoState(GO_STATE_ACTIVE);
            break;
        case GO_DARK_RUNED_CHEST:
        case GO_DARK_RUNED_CHEST_H:
            break;

        default:
            return;
    }
    m_mGoEntryGuidStore[pGo->GetEntry()] = pGo->GetObjectGuid();
}

void instance_culling_of_stratholme::DoChromieWhisper(int32 iEntry)
{
    if (Creature* pChromie = GetSingleCreatureFromStorage(NPC_CHROMIE_ENTRANCE))
    {
        Map::PlayerList const& players = instance->GetPlayers();
        if (!players.isEmpty())
        {
            for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
            {
                if (Player* pPlayer = itr->getSource())
                    DoScriptText(iEntry, pChromie, pPlayer);
            }
        }
    }
}

void instance_culling_of_stratholme::SetData(uint32 uiType, uint32 uiData)
{
    switch (uiType)
    {
        case TYPE_GRAIN_EVENT:
            m_auiEncounter[uiType] = uiData;
            if (uiData == SPECIAL)
                DoUpdateWorldState(WORLD_STATE_CRATES, 1);
            else if (uiData == IN_PROGRESS)
            {
                // safety check
                if (m_uiGrainCrateCount >= MAX_GRAIN_CRATES)
                    return;

                ++m_uiGrainCrateCount;
                DoUpdateWorldState(WORLD_STATE_CRATES_COUNT, m_uiGrainCrateCount);

                if (m_uiGrainCrateCount == MAX_GRAIN_CRATES)
                {
                    m_uiRemoveCrateStateTimer = 20000;
                    SetData(TYPE_GRAIN_EVENT, DONE);
                }
            }
            break;
        case TYPE_ARTHAS_INTRO_EVENT:
            m_auiEncounter[uiType] = uiData;
            break;
        case TYPE_ARTHAS_ESCORT_EVENT:
            m_auiEncounter[uiType] = uiData;
            break;
        case TYPE_MEATHOOK_EVENT:
            m_auiEncounter[uiType] = uiData;
            if (uiData == DONE)
                SetData(TYPE_SALRAMM_EVENT, IN_PROGRESS);
            break;
        case TYPE_SALRAMM_EVENT:
            m_auiEncounter[uiType] = uiData;
            if (uiData == DONE)
                DoUpdateWorldState(WORLD_STATE_WAVE, 0);    // Remove WaveCounter
            break;
        case TYPE_EPOCH_EVENT:
            m_auiEncounter[uiType] = uiData;
            break;
        case TYPE_MALGANIS_EVENT:
            m_auiEncounter[uiType] = uiData;
            if (uiData == DONE)
            {
                DoToggleGameObjectFlags(instance->IsRegularDifficulty() ? GO_DARK_RUNED_CHEST : GO_DARK_RUNED_CHEST_H, GO_FLAG_NO_INTERACT, false);
                DoRespawnGameObject(instance->IsRegularDifficulty() ? GO_DARK_RUNED_CHEST : GO_DARK_RUNED_CHEST_H, 30 * MINUTE);

                if (Player* pPlayer = GetPlayerInMap())
                    DoSpawnChromieIfNeeded(pPlayer);
            }
            break;
        case TYPE_INFINITE_CORRUPTER_TIME:
            m_auiEncounter[uiType] = uiData;
            if (!uiData)
            {
                DoUpdateWorldState(WORLD_STATE_TIME, 0);    // Remove Timer
                DoUpdateWorldState(WORLD_STATE_TIME_COUNTER, 0);
            }
            else
                DoUpdateWorldState(WORLD_STATE_TIME_COUNTER, uiData / (MINUTE * IN_MILLISECONDS));
            break;
        case TYPE_INFINITE_CORRUPTER:
            m_auiEncounter[uiType] = uiData;
            switch (uiData)
            {
                case IN_PROGRESS:
                    if (!m_auiEncounter[TYPE_INFINITE_CORRUPTER_TIME])
                        SetData(TYPE_INFINITE_CORRUPTER_TIME, MINUTE * 25 * IN_MILLISECONDS);
                    DoUpdateWorldState(WORLD_STATE_TIME, 1);// Show Timer
                    break;
                case DONE:
                    SetData(TYPE_INFINITE_CORRUPTER_TIME, 0);
                    break;
                case SPECIAL:
                    DoChromieWhisper(SAY_CHROMIE_HURRY);
                    break;
                case FAIL:
                    SetData(TYPE_INFINITE_CORRUPTER_TIME, 0);
                    if (Creature* pCorrupter = GetSingleCreatureFromStorage(NPC_INFINITE_CORRUPTER))
                        if (pCorrupter->isAlive())
                            pCorrupter->ForcedDespawn();
                    break;
            }
            break;
    }

    if (uiData == DONE || (uiType == TYPE_INFINITE_CORRUPTER && uiData == FAIL))
    {
        OUT_SAVE_INST_DATA;

        std::ostringstream saveStream;
        saveStream << m_auiEncounter[0] << " " << m_auiEncounter[1] << " " << m_auiEncounter[2] << " "
                   << m_auiEncounter[3] << " " << m_auiEncounter[4] << " " << m_auiEncounter[5] << " "
                   << m_auiEncounter[6] << " " << m_auiEncounter[7] << " " << m_auiEncounter[8];

        m_strInstData = saveStream.str();

        SaveToDB();
        OUT_SAVE_INST_DATA_COMPLETE;
    }
}

void instance_culling_of_stratholme::Load(const char* chrIn)
{
    if (!chrIn)
    {
        OUT_LOAD_INST_DATA_FAIL;
        return;
    }

    OUT_LOAD_INST_DATA(chrIn);

    std::istringstream loadStream(chrIn);
    loadStream >> m_auiEncounter[0] >> m_auiEncounter[1] >> m_auiEncounter[2] >> m_auiEncounter[3]
               >> m_auiEncounter[4] >> m_auiEncounter[5] >> m_auiEncounter[6] >> m_auiEncounter[7] >> m_auiEncounter[8];

    for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
    {
        if (i != TYPE_INFINITE_CORRUPTER_TIME)
        {
            if (m_auiEncounter[i] == IN_PROGRESS)
                m_auiEncounter[i] = NOT_STARTED;
        }
    }

    // If already started counting down time, the event is "in progress"
    if (m_auiEncounter[TYPE_INFINITE_CORRUPTER_TIME])
        m_auiEncounter[TYPE_INFINITE_CORRUPTER] = IN_PROGRESS;

    OUT_LOAD_INST_DATA_COMPLETE;
}

void instance_culling_of_stratholme::OnPlayerEnter(Player* pPlayer)
{
    // spawn Chromie and Arthas
    DoSpawnArthasIfNeeded(pPlayer);
    DoSpawnChromieIfNeeded(pPlayer);

    // Show World States if needed
    if (m_auiEncounter[TYPE_GRAIN_EVENT] == IN_PROGRESS || m_auiEncounter[TYPE_GRAIN_EVENT] == SPECIAL)
        pPlayer->SendUpdateWorldState(WORLD_STATE_CRATES, 1);      // Show Crates Counter
    else
        pPlayer->SendUpdateWorldState(WORLD_STATE_CRATES, 0);      // Remove Crates Counter

    if (m_auiEncounter[TYPE_MEATHOOK_EVENT] == IN_PROGRESS)
        pPlayer->SendUpdateWorldState(WORLD_STATE_WAVE, 1);        // Add WaveCounter
    else if (m_auiEncounter[TYPE_SALRAMM_EVENT] == IN_PROGRESS)
        pPlayer->SendUpdateWorldState(WORLD_STATE_WAVE, 6);        // Add WaveCounter
    else
        pPlayer->SendUpdateWorldState(WORLD_STATE_WAVE, 0);        // Remove WaveCounter

    if (m_auiEncounter[TYPE_INFINITE_CORRUPTER_TIME])
        pPlayer->SendUpdateWorldState(WORLD_STATE_TIME, 1);        // Show Timer
    else
        pPlayer->SendUpdateWorldState(WORLD_STATE_TIME, 0);        // Remove Timer
}

uint32 instance_culling_of_stratholme::GetData(uint32 uiType) const
{
    if (uiType < MAX_ENCOUNTER)
        return m_auiEncounter[uiType];

    return 0;
}

uint8 instance_culling_of_stratholme::GetInstancePosition()
{
    if (m_auiEncounter[TYPE_MALGANIS_EVENT] == DONE)
        return POS_INSTANCE_FINISHED;
    else if (m_auiEncounter[TYPE_ARTHAS_ESCORT_EVENT] == DONE)
        return POS_ARTHAS_MALGANIS;
    else if (m_auiEncounter[TYPE_EPOCH_EVENT] == DONE)
        return POS_ARTHAS_ESCORTING;
    else if (m_auiEncounter[TYPE_SALRAMM_EVENT] == DONE)
        return POS_ARTHAS_TOWNHALL;
    else if (m_auiEncounter[TYPE_MEATHOOK_EVENT] == DONE)
        return POS_ARTHAS_WAVES;
    else if (m_auiEncounter[TYPE_ARTHAS_INTRO_EVENT] == DONE)
        return POS_ARTHAS_WAVES;
    else if (m_auiEncounter[TYPE_GRAIN_EVENT] == DONE)
        return POS_ARTHAS_INTRO;
    else
        return 0;
}

static bool sortFromEastToWest(Creature* pFirst, Creature* pSecond)
{
    return pFirst && pSecond && pFirst->GetPositionY() < pSecond->GetPositionY();
}

static bool sortFromSouthToNorth(Creature* pFirst, Creature* pSecond)
{
    return pFirst && pSecond && pFirst->GetPositionX() < pSecond->GetPositionX();
}

// return the ordered list of Grain Crate Helpers
void instance_culling_of_stratholme::GetCratesBunnyOrderedList(std::list<Creature*>& lList)
{
    std::list<Creature*> lCratesBunnyList;
    for (GuidList::const_iterator itr = m_luiCratesBunnyGUIDs.begin(); itr != m_luiCratesBunnyGUIDs.end(); ++itr)
    {
        if (Creature* pBunny = instance->GetCreature(*itr))
            lCratesBunnyList.push_back(pBunny);
    }
    if (lCratesBunnyList.empty())
        return;

    lCratesBunnyList.sort(sortFromEastToWest);
    lList = lCratesBunnyList;
}

Creature* instance_culling_of_stratholme::GetStratIntroFootman()
{
    std::list<Creature*> lFootmanList;
    for (GuidList::const_iterator itr = m_luiFootmanGUIDs.begin(); itr != m_luiFootmanGUIDs.end(); ++itr)
    {
        if (Creature* pFootman = instance->GetCreature(*itr))
            lFootmanList.push_back(pFootman);
    }

    if (lFootmanList.empty())
        return NULL;
    else
    {
        lFootmanList.sort(sortFromSouthToNorth);
        return *lFootmanList.begin();
    }
}

void instance_culling_of_stratholme::GetResidentOrderedList(std::list<Creature*>& lList)
{
    std::list<Creature*> lResidentList;
    for (GuidList::const_iterator itr = m_luiResidentGUIDs.begin(); itr != m_luiResidentGUIDs.end(); ++itr)
    {
        if (Creature* pResident = instance->GetCreature(*itr))
            lResidentList.push_back(pResident);
    }
    if (lResidentList.empty())
        return;

    lResidentList.sort(sortFromSouthToNorth);
    lList = lResidentList;
}

void instance_culling_of_stratholme::ArthasJustDied()
{
    m_uiArthasRespawnTimer = 10000;                         // TODO, could be instant
}

void instance_culling_of_stratholme::DoSpawnArthasIfNeeded(Unit* pSummoner)
{
    if (!pSummoner)
        return;

    Creature* pArthas = GetSingleCreatureFromStorage(NPC_ARTHAS, true);
    if (pArthas && pArthas->isAlive())
        return;

    uint8 uiPosition = GetInstancePosition();
    if (uiPosition && uiPosition <= MAX_ARTHAS_SPAWN_POS)
        pSummoner->SummonCreature(NPC_ARTHAS, m_aArthasSpawnLocs[uiPosition - 1].m_fX, m_aArthasSpawnLocs[uiPosition - 1].m_fY, m_aArthasSpawnLocs[uiPosition - 1].m_fZ, m_aArthasSpawnLocs[uiPosition - 1].m_fO, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10000);
}

// Atm here only new Chromies are spawned - despawning depends on Mangos featuring such a thing
// The hourglass also is not yet spawned/ relocated.
void instance_culling_of_stratholme::DoSpawnChromieIfNeeded(Unit* pSummoner)
{
    if (!pSummoner)
        return;

    if (GetInstancePosition() == POS_INSTANCE_FINISHED)
    {
        Creature* pChromie = GetSingleCreatureFromStorage(NPC_CHROMIE_END, true);
        if (!pChromie)
            pSummoner->SummonCreature(NPC_CHROMIE_END, m_aChromieSpawnLocs[1].m_fX, m_aChromieSpawnLocs[1].m_fY, m_aChromieSpawnLocs[1].m_fZ, m_aChromieSpawnLocs[1].m_fO, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10000);
    }
    else if (GetInstancePosition() >= POS_ARTHAS_INTRO)
    {
        Creature* pChromie = GetSingleCreatureFromStorage(NPC_CHROMIE_ENTRANCE, true);
        if (!pChromie)
            pSummoner->SummonCreature(NPC_CHROMIE_ENTRANCE, m_aChromieSpawnLocs[0].m_fX, m_aChromieSpawnLocs[0].m_fY, m_aChromieSpawnLocs[0].m_fZ, m_aChromieSpawnLocs[0].m_fO, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10000);
    }
}

// Function to check if the grain event can progress
bool instance_culling_of_stratholme::CanGrainEventProgress(Creature* pCrate)
{
    if (!pCrate)
        return false;

    if (m_sGrainCratesGuidSet.find(pCrate->GetObjectGuid()) != m_sGrainCratesGuidSet.end())
        return false;

    if (GetData(TYPE_GRAIN_EVENT) != DONE)
        SetData(TYPE_GRAIN_EVENT, IN_PROGRESS);

    m_sGrainCratesGuidSet.insert(pCrate->GetObjectGuid());
    return true;
}

void instance_culling_of_stratholme::DoEventAtTriggerIfCan(uint32 uiTriggerId)
{
    switch (uiTriggerId)
    {
        case AREATRIGGER_INN:
            if (m_bStartedInnEvent)
                return;

            // start dialogue
            if (Creature* pMichael = GetSingleCreatureFromStorage(NPC_MICHAEL_BELFAST))
            {
                pMichael->SetStandState(UNIT_STAND_STATE_STAND);
                pMichael->GetMotionMaster()->MoveWaypoint();
            }
            m_bStartedInnEvent = true;
            break;
    }
}

void instance_culling_of_stratholme::Update(uint32 uiDiff)
{
    // 25min Run - decrease time, update worldstate every ~20s
    // as the time is always saved by m_auiEncounter[TYPE_INFINITE_CORRUPTER_TIME], there is no need for an extra timer
    if (m_auiEncounter[TYPE_INFINITE_CORRUPTER_TIME])
    {
        if (m_auiEncounter[TYPE_INFINITE_CORRUPTER_TIME] <= uiDiff)
            SetData(TYPE_INFINITE_CORRUPTER, FAIL);
        else
        {
            m_auiEncounter[TYPE_INFINITE_CORRUPTER_TIME] -= uiDiff;
            if (m_auiEncounter[TYPE_INFINITE_CORRUPTER_TIME] / IN_MILLISECONDS % 20 == 0)
                SetData(TYPE_INFINITE_CORRUPTER_TIME, m_auiEncounter[TYPE_INFINITE_CORRUPTER_TIME]);
        }

        // This part is needed for a small "hurry up guys" note, TODO, verify 20min
        if (m_auiEncounter[TYPE_INFINITE_CORRUPTER] == IN_PROGRESS && m_auiEncounter[TYPE_INFINITE_CORRUPTER_TIME] <= 24 * MINUTE * IN_MILLISECONDS)
            SetData(TYPE_INFINITE_CORRUPTER, SPECIAL);
    }

    // Small Timer, to remove Grain-Crate WorldState and Spawn Second Chromie
    if (m_uiRemoveCrateStateTimer)
    {
        if (m_uiRemoveCrateStateTimer <= uiDiff)
        {
            if (Player* pPlayer = GetPlayerInMap())
                DoSpawnChromieIfNeeded(pPlayer);

            DoUpdateWorldState(WORLD_STATE_CRATES, 0);
            DoChromieWhisper(WHISPER_CHROMIE_CRATES);
            m_uiRemoveCrateStateTimer = 0;
        }
        else
            m_uiRemoveCrateStateTimer -= uiDiff;
    }

    // Respawn Arthas after some time
    if (m_uiArthasRespawnTimer)
    {
        if (m_uiArthasRespawnTimer <= uiDiff)
        {
            if (Player* pPlayer = GetPlayerInMap())
                DoSpawnArthasIfNeeded(pPlayer);

            m_uiArthasRespawnTimer = 0;
        }
        else
            m_uiArthasRespawnTimer -= uiDiff;
    }
}

InstanceData* GetInstanceData_instance_culling_of_stratholme(Map* pMap)
{
    return new instance_culling_of_stratholme(pMap);
}

bool AreaTrigger_at_culling_of_stratholme(Player* pPlayer, AreaTriggerEntry const* pAt)
{
    if (pAt->id == AREATRIGGER_INN)
    {
        if (pPlayer->isGameMaster() || pPlayer->isDead())
            return false;

        if (instance_culling_of_stratholme* pInstance = (instance_culling_of_stratholme*)pPlayer->GetInstanceData())
            pInstance->DoEventAtTriggerIfCan(pAt->id);
    }

    return false;
}

void AddSC_instance_culling_of_stratholme()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "instance_culling_of_stratholme";
    pNewScript->GetInstanceData = &GetInstanceData_instance_culling_of_stratholme;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "at_culling_of_stratholme";
    pNewScript->pAreaTrigger = &AreaTrigger_at_culling_of_stratholme;
    pNewScript->RegisterSelf();
}

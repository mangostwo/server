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
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

/* ScriptData
SDName: instance_trial_of_the_champion
SD%Complete: 20
SDComment: Basic support
SDCategory: Crusader Coliseum, Trial of the Champion
EndScriptData */

#include "precompiled.h"
#include "trial_of_the_champion.h"

/* Trial of the Champion encounters:
0 - Grand Champions
1 - Argent Champion
2 - Black Knight
*/

static const DialogueEntryTwoSide aTocDialogues[] =
{
    // ToDo:
    {0, 0, 0, 0 , 0}
};

instance_trial_of_the_champion::instance_trial_of_the_champion(Map* pMap) : ScriptedInstance(pMap), DialogueHelper(aTocDialogues),
    m_uiTeam(TEAM_NONE),
    m_uiHeraldEntry(0),
    m_uiGrandChampionEntry(0)
{
    Initialize();
}

void instance_trial_of_the_champion::Initialize()
{
    memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
    InitializeDialogueHelper(this);
}

void instance_trial_of_the_champion::OnPlayerEnter(Player* pPlayer)
{
    if (!m_uiTeam)
    {
        m_uiTeam = pPlayer->GetTeam();
        SetDialogueSide(m_uiTeam == ALLIANCE);

        m_uiHeraldEntry = m_uiTeam == ALLIANCE ? NPC_ARELAS_BRIGHTSTAR : NPC_JAEREN_SUNSWORN;

        // set a random grand champion
        m_uiGrandChampionEntry = urand(0, 1) ? NPC_EADRIC : NPC_PALETRESS;

        if (m_vChampionsEntries.empty())
        {
            m_vChampionsEntries.resize(MAX_CHAMPIONS_AVAILABLE);

            // fill vector array with entries from creature array
            for (uint8 i = 0; i < MAX_CHAMPIONS_AVAILABLE; ++i)
                m_vChampionsEntries[i] = m_uiTeam == ALLIANCE ? aHordeChampions[i] : aAllianceChampions[i];

            // set a random champion list
            std::random_shuffle(m_vChampionsEntries.begin(), m_vChampionsEntries.end());
        }
    }

    DoSummonHeraldIfNeeded();
}

void instance_trial_of_the_champion::OnCreatureCreate(Creature* pCreature)
{
    switch (pCreature->GetEntry())
    {
        case NPC_JAEREN_SUNSWORN:
        case NPC_ARELAS_BRIGHTSTAR:
        case NPC_TIRION_FORDRING:
        case NPC_VARIAN_WRYNN:
        case NPC_THRALL:
        case NPC_JACOB_ALERIUS:
        case NPC_AMBROSE_BOLTSPARK:
        case NPC_COLOSOS:
        case NPC_JAELYNE_EVENSONG:
        case NPC_LANA_STOUTHAMMER:
        case NPC_MOKRA_SKULLCRUSHER:
        case NPC_ERESSEA_DAWNSINGER:
        case NPC_RUNOK_WILDMANE:
        case NPC_ZULTORE:
        case NPC_DEATHSTALKER_VISCERI:
        case NPC_EADRIC:
        case NPC_PALETRESS:
            break;
        default:
            return;
    }

    m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
}

void instance_trial_of_the_champion::OnObjectCreate(GameObject* pGo)
{
    switch (pGo->GetEntry())
    {
        case GO_MAIN_GATE:
        case GO_NORTH_GATE:
        case GO_CHAMPIONS_LOOT:
        case GO_CHAMPIONS_LOOT_H:
        case GO_EADRIC_LOOT:
        case GO_EADRIC_LOOT_H:
        case GO_PALETRESS_LOOT:
        case GO_PALETRESS_LOOT_H:
            break;
        default:
            return;
    }

    m_mGoEntryGuidStore[pGo->GetEntry()] = pGo->GetObjectGuid();
}

void instance_trial_of_the_champion::SetData(uint32 uiType, uint32 uiData)
{
    switch (uiType)
    {
        case TYPE_GRAND_CHAMPIONS:
            m_auiEncounter[uiType] = uiData;
            if (uiData == DONE)
                DoRespawnGameObject(instance->IsRegularDifficulty() ? GO_CHAMPIONS_LOOT : GO_CHAMPIONS_LOOT_H, 30 * MINUTE);
            break;
        case TYPE_ARGENT_CHAMPION:
            m_auiEncounter[uiType] = uiData;
            if (uiData == DONE)
            {
                if (m_uiGrandChampionEntry == NPC_EADRIC)
                    DoRespawnGameObject(instance->IsRegularDifficulty() ? GO_EADRIC_LOOT : GO_EADRIC_LOOT_H, 30 * MINUTE);
                else if (m_uiGrandChampionEntry == NPC_PALETRESS)
                    DoRespawnGameObject(instance->IsRegularDifficulty() ? GO_PALETRESS_LOOT : GO_PALETRESS_LOOT_H, 30 * MINUTE);
            }
            break;
        case TYPE_BLACK_KNIGHT:
            m_auiEncounter[uiType] = uiData;
            break;
        default:
            script_error_log("Instance Trial of The Champion: ERROR SetData = %u for type %u does not exist/not implemented.", uiType, uiData);
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

uint32 instance_trial_of_the_champion::GetData(uint32 uiType) const
{
    if (uiType < MAX_ENCOUNTER)
        return m_auiEncounter[uiType];

    return 0;
}

void instance_trial_of_the_champion::Load(const char* chrIn)
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
    {
        if (m_auiEncounter[i] == IN_PROGRESS)
            m_auiEncounter[i] = NOT_STARTED;
    }

    OUT_LOAD_INST_DATA_COMPLETE;
}

void instance_trial_of_the_champion::DoSummonHeraldIfNeeded()
{
    Player* pPlayer = GetPlayerInMap();
    if (!pPlayer)
        return;

    if (GetSingleCreatureFromStorage(m_uiHeraldEntry, true))
        return;

    pPlayer->SummonCreature(m_uiHeraldEntry, aHeraldPositions[0][0], aHeraldPositions[0][1], aHeraldPositions[0][2], aHeraldPositions[0][3], TEMPSUMMON_DEAD_DESPAWN, 0);
}

void instance_trial_of_the_champion::JustDidDialogueStep(int32 iEntry)
{
    // ToDo:
}

InstanceData* GetInstanceData_instance_trial_of_the_champion(Map* pMap)
{
    return new instance_trial_of_the_champion(pMap);
}

void AddSC_instance_trial_of_the_champion()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "instance_trial_of_the_champion";
    pNewScript->GetInstanceData = &GetInstanceData_instance_trial_of_the_champion;
    pNewScript->RegisterSelf();
}

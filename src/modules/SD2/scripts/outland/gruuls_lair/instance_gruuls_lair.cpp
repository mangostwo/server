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
SDName: Instance_Gruuls_Lair
SD%Complete: 100
SDComment:
SDCategory: Gruul's Lair
EndScriptData */

#include "precompiled.h"
#include "gruuls_lair.h"

/* Gruuls Lair encounters:
1 - High King Maulgar event
2 - Gruul event
*/

struct is_gruuls_lair : public InstanceScript
{
    is_gruuls_lair() : InstanceScript("instance_gruuls_lair") {}

    class instance_gruuls_lair : public ScriptedInstance
    {
    public:
        instance_gruuls_lair(Map* pMap) : ScriptedInstance(pMap),
            m_uiCouncilMembersDied(0)
        {
            Initialize();
        }

        void Initialize() override
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
        }

        bool IsEncounterInProgress() const override
        {
            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            if (m_auiEncounter[i] == IN_PROGRESS)
            {
                return true;
            }

            return false;
        }

        void OnCreatureCreate(Creature* pCreature) override
        {
            if (pCreature->GetEntry() == NPC_MAULGAR)
            {
                m_mNpcEntryGuidStore[NPC_MAULGAR] = pCreature->GetObjectGuid();
            }
        }

        void OnObjectCreate(GameObject* pGo) override
        {
            switch (pGo->GetEntry())
            {
            case GO_PORT_GRONN_1:
                if (m_auiEncounter[TYPE_MAULGAR_EVENT] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_PORT_GRONN_2:
                break;

            default:
                return;
            }
            m_mGoEntryGuidStore[pGo->GetEntry()] = pGo->GetObjectGuid();
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            switch (uiType)
            {
            case TYPE_MAULGAR_EVENT:
                if (uiData == SPECIAL)
                {
                    ++m_uiCouncilMembersDied;

                    if (m_uiCouncilMembersDied == MAX_COUNCIL)
                    {
                        SetData(TYPE_MAULGAR_EVENT, DONE);
                    }
                    // Don't store special data
                    break;
                }
                if (uiData == FAIL)
                {
                    m_uiCouncilMembersDied = 0;
                }
                if (uiData == DONE)
                {
                    DoUseDoorOrButton(GO_PORT_GRONN_1);
                }
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_GRUUL_EVENT:
                DoUseDoorOrButton(GO_PORT_GRONN_2);
                m_auiEncounter[uiType] = uiData;
                break;
            }

            if (uiData == DONE)
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << m_auiEncounter[0] << " " << m_auiEncounter[1];

                m_strSaveData = saveStream.str();

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

        const char* Save() const override { return m_strSaveData.c_str(); }
        void Load(const char* chrIn) override
        {
            if (!chrIn)
            {
                OUT_LOAD_INST_DATA_FAIL;
                return;
            }

            OUT_LOAD_INST_DATA(chrIn);

            std::istringstream loadStream(chrIn);

            loadStream >> m_auiEncounter[0] >> m_auiEncounter[1];

            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            if (m_auiEncounter[i] == IN_PROGRESS)
            {
                m_auiEncounter[i] = NOT_STARTED;
            }

            OUT_LOAD_INST_DATA_COMPLETE;
        }

    private:
        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strSaveData;

        uint8 m_uiCouncilMembersDied;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_gruuls_lair(pMap);
    }
};

void AddSC_instance_gruuls_lair()
{
    Script* s;

    s = new is_gruuls_lair();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_gruuls_lair";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_gruuls_lair;
    //pNewScript->RegisterSelf();
}

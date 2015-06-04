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
SDName: Instance_Utgarde_Keep
SD%Complete: 20%
SDComment:
SDCategory: Utgarde Keep
EndScriptData */

#include "precompiled.h"
#include "utgarde_keep.h"

struct is_utgarde_keep : public InstanceScript
{
    is_utgarde_keep() : InstanceScript("instance_utgarde_keep") {}

    class instance_utgarde_keep : public ScriptedInstance
    {
    public:
        instance_utgarde_keep(Map* pMap) : ScriptedInstance(pMap),
            m_bKelesethAchievFailed(false)
        {
            Initialize();
        }

        ~instance_utgarde_keep() {}

        void Initialize() override
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
        }

        void OnCreatureCreate(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_KELESETH:
            case NPC_SKARVALD:
            case NPC_DALRONN:
            case NPC_INGVAR:
                m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
                break;
            }
        }

        void OnObjectCreate(GameObject* pGo) override
        {
            switch (pGo->GetEntry())
            {
            case GO_BELLOW_1:
                if (m_auiEncounter[TYPE_BELLOW_1] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_BELLOW_2:
                if (m_auiEncounter[TYPE_BELLOW_2] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_BELLOW_3:
                if (m_auiEncounter[TYPE_BELLOW_3] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_FORGEFIRE_1:
                if (m_auiEncounter[TYPE_BELLOW_1] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_FORGEFIRE_2:
                if (m_auiEncounter[TYPE_BELLOW_2] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_FORGEFIRE_3:
                if (m_auiEncounter[TYPE_BELLOW_3] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_PORTCULLIS_EXIT_1:
            case GO_PORTCULLIS_EXIT_2:
                if (m_auiEncounter[TYPE_INGVAR] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_PORTCULLIS_COMBAT:
                break;

            default:
                return;
            }
            m_mGoEntryGuidStore[pGo->GetEntry()] = pGo->GetObjectGuid();
        }

        void OnCreatureDeath(Creature* pCreature) override
        {
            if (pCreature->GetEntry() == NPC_FROST_TOMB)
                m_bKelesethAchievFailed = true;
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            switch (uiType)
            {
            case TYPE_KELESETH:
                if (uiData == IN_PROGRESS)
                    m_bKelesethAchievFailed = false;
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_SKARVALD_DALRONN:
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_INGVAR:
                if (m_auiEncounter[uiType] == uiData)
                    return;
                DoUseDoorOrButton(GO_PORTCULLIS_COMBAT);
                if (uiData == DONE)
                {
                    DoUseDoorOrButton(GO_PORTCULLIS_EXIT_1);
                    DoUseDoorOrButton(GO_PORTCULLIS_EXIT_2);
                }
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_BELLOW_1:
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_BELLOW_2:
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_BELLOW_3:
                m_auiEncounter[uiType] = uiData;
                break;
            }

            if (uiData == DONE)
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << m_auiEncounter[0] << " " << m_auiEncounter[1] << " " << m_auiEncounter[2] << " "
                    << m_auiEncounter[3] << " " << m_auiEncounter[4] << " " << m_auiEncounter[5];

                m_strInstData = saveStream.str();

                SaveToDB();
                OUT_SAVE_INST_DATA_COMPLETE;
            }
        }

        uint32 GetData(uint32 uiType) const override
        {
            if (uiType < MAX_ENCOUNTER)
                return m_auiEncounter[uiType];

            return 0;
        }

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
            loadStream >> m_auiEncounter[0] >> m_auiEncounter[1] >> m_auiEncounter[2] >> m_auiEncounter[3] >> m_auiEncounter[4] >> m_auiEncounter[5];

            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            {
                if (m_auiEncounter[i] == IN_PROGRESS)
                    m_auiEncounter[i] = NOT_STARTED;
            }

            OUT_LOAD_INST_DATA_COMPLETE;
        }

        bool CheckAchievementCriteriaMeet(uint32 uiCriteriaId, Player const* pSource, Unit const* pTarget, uint32 uiMiscValue1 /* = 0*/) const override
        {
            if (uiCriteriaId == ACHIEV_CRIT_ON_THE_ROCKS)
                return !m_bKelesethAchievFailed;

            return false;
        }

    private:
        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strInstData;

        bool m_bKelesethAchievFailed;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_utgarde_keep(pMap);
    }
};

void AddSC_instance_utgarde_keep()
{
    Script* s;

    s = new is_utgarde_keep();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_utgarde_keep";
    //pNewScript->GetInstanceData = GetInstanceData_instance_utgarde_keep;
    //pNewScript->RegisterSelf();
}

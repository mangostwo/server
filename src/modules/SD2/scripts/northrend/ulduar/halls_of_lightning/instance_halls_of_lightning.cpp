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
SDName: Instance_Halls_of_Lightning
SD%Complete: 90%
SDComment: All ready.
SDCategory: Halls of Lightning
EndScriptData */

#include "precompiled.h"
#include "halls_of_lightning.h"

/* Halls of Lightning encounters:
0 - General Bjarngrim
1 - Volkhan
2 - Ionar
3 - Loken
*/

struct is_halls_of_lightning : public InstanceScript
{
    is_halls_of_lightning() : InstanceScript("instance_halls_of_lightning") {}

    class instance_halls_of_lightning : public ScriptedInstance
    {
    public:
        instance_halls_of_lightning(Map* pMap) : ScriptedInstance(pMap),
            m_bLightningStruck(false),
            m_bIsShatterResistant(false)
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
            case NPC_BJARNGRIM:
            case NPC_IONAR:
            case NPC_VOLKHAN_ANVIL:
                m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
                break;
            }
        }

        void OnObjectCreate(GameObject* pGo) override
        {
            switch (pGo->GetEntry())
            {
            case GO_VOLKHAN_DOOR:
                if (m_auiEncounter[TYPE_VOLKHAN] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_IONAR_DOOR:
                if (m_auiEncounter[TYPE_IONAR] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_LOKEN_THRONE:
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
            case TYPE_BJARNGRIM:
                if (uiData == SPECIAL)
                    m_bLightningStruck = true;
                else if (uiData == FAIL)
                    m_bLightningStruck = false;
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_VOLKHAN:
                if (uiData == DONE)
                    DoUseDoorOrButton(GO_VOLKHAN_DOOR);
                else if (uiData == IN_PROGRESS)
                    m_bIsShatterResistant = true;
                else if (uiData == SPECIAL)
                    m_bIsShatterResistant = false;
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_IONAR:
                if (uiData == DONE)
                    DoUseDoorOrButton(GO_IONAR_DOOR);
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_LOKEN:
                if (uiData == IN_PROGRESS)
                    DoStartTimedAchievement(ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE, ACHIEV_START_LOKEN_ID);
                else if (uiData == DONE)
                {
                    if (GameObject* pGlobe = GetSingleGameObjectFromStorage(GO_LOKEN_THRONE))
                        pGlobe->SendGameObjectCustomAnim(pGlobe->GetObjectGuid());
                }
                m_auiEncounter[uiType] = uiData;
                break;
            }

            if (uiData == DONE)
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << m_auiEncounter[0] << " " << m_auiEncounter[1] << " " << m_auiEncounter[2] << " " << m_auiEncounter[3];

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

        bool CheckAchievementCriteriaMeet(uint32 uiCriteriaId, Player const* pSource, Unit const* pTarget, uint32 uiMiscValue1 /* = 0*/) const override
        {
            switch (uiCriteriaId)
            {
            case ACHIEV_CRIT_LIGHTNING:
                return m_bLightningStruck;
            case ACHIEV_CRIT_RESISTANT:
                return m_bIsShatterResistant;
            }

            return false;
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
            loadStream >> m_auiEncounter[0] >> m_auiEncounter[1] >> m_auiEncounter[2] >> m_auiEncounter[3];

            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            {
                if (m_auiEncounter[i] == IN_PROGRESS)
                    m_auiEncounter[i] = NOT_STARTED;
            }

            OUT_LOAD_INST_DATA_COMPLETE;
        }

    private:
        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strInstData;

        bool m_bLightningStruck;
        bool m_bIsShatterResistant;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_halls_of_lightning(pMap);
    }
};

void AddSC_instance_halls_of_lightning()
{
    Script* s;

    s = new is_halls_of_lightning();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_halls_of_lightning";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_halls_of_lightning;
    //pNewScript->RegisterSelf();
}

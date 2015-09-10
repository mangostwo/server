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
SDName: instance_nexus
SD%Complete: 75%
SDComment:
SDCategory: The Nexus
EndScriptData */

#include "precompiled.h"
#include "nexus.h"

struct is_nexus : public InstanceScript
{
    is_nexus() : InstanceScript("instance_nexus") {}

    class instance_nexus : public ScriptedInstance
    {
    public:
        instance_nexus(Map* pMap) : ScriptedInstance(pMap)
        {
            Initialize();

            for (uint8 i = 0; i < MAX_SPECIAL_ACHIEV_CRITS; ++i)
                m_abAchievCriteria[i] = false;
        }

        void Initialize() override
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
        }

        void OnObjectCreate(GameObject* pGo) override
        {
            switch (pGo->GetEntry())
            {
            case GO_CONTAINMENT_SPHERE_TELESTRA:
                if (m_auiEncounter[TYPE_TELESTRA] == DONE)
                    pGo->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NO_INTERACT);
                break;
            case GO_CONTAINMENT_SPHERE_ANOMALUS:
                if (m_auiEncounter[TYPE_ANOMALUS] == DONE)
                    pGo->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NO_INTERACT);
                break;
            case GO_CONTAINMENT_SPHERE_ORMOROK:
                if (m_auiEncounter[TYPE_ORMOROK] == DONE)
                    pGo->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NO_INTERACT);
                break;

            default:
                return;
            }
            m_mGoEntryGuidStore[pGo->GetEntry()] = pGo->GetObjectGuid();
        }

        void OnCreatureCreate(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_ORMOROK:
            case NPC_KERISTRASZA:
                m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
                break;
            }
        }

        uint32 GetData(uint32 uiType) const override
        {
            if (uiType < MAX_ENCOUNTER)
                return m_auiEncounter[uiType];

            return 0;
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            switch (uiType)
            {
            case TYPE_TELESTRA:
                m_auiEncounter[uiType] = uiData;
                if (uiData == IN_PROGRESS)
                    SetSpecialAchievementCriteria(TYPE_ACHIEV_SPLIT_PERSONALITY, true);
                if (uiData == DONE)
                    DoToggleGameObjectFlags(GO_CONTAINMENT_SPHERE_TELESTRA, GO_FLAG_NO_INTERACT, false);
                break;
            case TYPE_ANOMALUS:
                m_auiEncounter[uiType] = uiData;
                if (uiData == IN_PROGRESS)
                    SetSpecialAchievementCriteria(TYPE_ACHIEV_CHAOS_THEORY, true);
                if (uiData == DONE)
                    DoToggleGameObjectFlags(GO_CONTAINMENT_SPHERE_ANOMALUS, GO_FLAG_NO_INTERACT, false);
                break;
            case TYPE_ORMOROK:
                m_auiEncounter[uiType] = uiData;
                if (uiData == DONE)
                    DoToggleGameObjectFlags(GO_CONTAINMENT_SPHERE_ORMOROK, GO_FLAG_NO_INTERACT, false);
                break;
            case TYPE_KERISTRASZA:
                m_auiEncounter[uiType] = uiData;
                if (uiData == IN_PROGRESS)
                    m_sIntenseColdFailPlayers.clear();
                break;
            case TYPE_INTENSE_COLD_FAILED:
                // Insert the players who fail the achiev and haven't been already inserted in the set
                if (m_sIntenseColdFailPlayers.find(uiData) == m_sIntenseColdFailPlayers.end())
                    m_sIntenseColdFailPlayers.insert(uiData);
                break;
            case TYPE_ACHIEV_SPLIT_PERSONALITY:
            case TYPE_ACHIEV_CHAOS_THEORY:
                SetSpecialAchievementCriteria(uiType - TYPE_ACHIEV_CHAOS_THEORY, bool(uiData));
                return;
            default:
                script_error_log("Instance Nexus: ERROR SetData = %u for type %u does not exist/not implemented.", uiType, uiData);
                return;
            }

            if (m_auiEncounter[TYPE_TELESTRA] == SPECIAL && m_auiEncounter[TYPE_ANOMALUS] == SPECIAL && m_auiEncounter[TYPE_ORMOROK] == SPECIAL)
            {
                // release Keristrasza from her prison here
                m_auiEncounter[TYPE_KERISTRASZA] = SPECIAL;

                Creature* pCreature = GetSingleCreatureFromStorage(NPC_KERISTRASZA);
                if (pCreature && pCreature->IsAlive())
                {
                    pCreature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE | UNIT_FLAG_OOC_NOT_ATTACKABLE);
                    pCreature->RemoveAurasDueToSpell(SPELL_FROZEN_PRISON);
                }
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

        bool CheckAchievementCriteriaMeet(uint32 uiCriteriaId, Player const* pSource, Unit const* pTarget, uint32 uiMiscValue1 /* = 0*/) const override
        {
            switch (uiCriteriaId)
            {
            case ACHIEV_CRIT_CHAOS_THEORY:
                return m_abAchievCriteria[0];
            case ACHIEV_CRIT_SPLIT_PERSONALITY:
                return m_abAchievCriteria[1];
            case ACHIEV_CRIT_INTENSE_COLD:
                // Return true if not found in the set
                return m_sIntenseColdFailPlayers.find(pSource->GetGUIDLow()) == m_sIntenseColdFailPlayers.end();

            default:
                return false;
            }
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
        void SetSpecialAchievementCriteria(uint32 uiType, bool bIsMet)
        {
            if (uiType < MAX_SPECIAL_ACHIEV_CRITS)
                m_abAchievCriteria[uiType] = bIsMet;
        }

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strInstData;

        bool m_abAchievCriteria[MAX_SPECIAL_ACHIEV_CRITS];

        std::set<uint32> m_sIntenseColdFailPlayers;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_nexus(pMap);
    }
};

struct go_containment_sphere : public GameObjectScript
{
    go_containment_sphere() : GameObjectScript("go_containment_sphere") {}

    bool OnUse(Player* /*pPlayer*/, GameObject* pGo) override
    {
        ScriptedInstance* pInstance = (ScriptedInstance*)pGo->GetInstanceData();

        if (!pInstance)
            return false;

        switch (pGo->GetEntry())
        {
        case GO_CONTAINMENT_SPHERE_TELESTRA: pInstance->SetData(TYPE_TELESTRA, SPECIAL); break;
        case GO_CONTAINMENT_SPHERE_ANOMALUS: pInstance->SetData(TYPE_ANOMALUS, SPECIAL); break;
        case GO_CONTAINMENT_SPHERE_ORMOROK:  pInstance->SetData(TYPE_ORMOROK, SPECIAL);  break;
        }

        pGo->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NO_INTERACT);
        return false;
    }
};

void AddSC_instance_nexus()
{
    Script* s;

    s = new is_nexus();
    s->RegisterSelf();

    s = new go_containment_sphere();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_nexus";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_nexus;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "go_containment_sphere";
    //pNewScript->pGOUse = &GOUse_go_containment_sphere;
    //pNewScript->RegisterSelf();
}

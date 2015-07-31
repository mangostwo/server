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
SDName: Instance_Shadow_Labyrinth
SD%Complete: 85
SDComment: Some cleanup left along with save
SDCategory: Auchindoun, Shadow Labyrinth
EndScriptData */

#include "precompiled.h"
#include "shadow_labyrinth.h"

/* Shadow Labyrinth encounters:
1 - Ambassador Hellmaw event
2 - Blackheart the Inciter event
3 - Grandmaster Vorpil event
4 - Murmur event
*/

struct is_shadow_labyrinth : public InstanceScript
{
    is_shadow_labyrinth() : InstanceScript("instance_shadow_labyrinth") {}

    class instance_shadow_labyrinth : public ScriptedInstance
    {
    public:
        instance_shadow_labyrinth(Map* pMap) : ScriptedInstance(pMap)
        {
            Initialize();
        }

        void Initialize() override
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
        }

        void OnObjectCreate(GameObject* pGo) override
        {
            switch (pGo->GetEntry())
            {
            case GO_REFECTORY_DOOR:
                if (m_auiEncounter[2] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_SCREAMING_HALL_DOOR:
                if (m_auiEncounter[3] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
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
            case NPC_VORPIL:
            case NPC_HELLMAW:
                m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
                break;
            }
        }

        void OnCreatureDeath(Creature* pCreature) override
        {
            // unbanish Hellmaw when all Cabal Ritualists are dead
            if (pCreature->GetEntry() == NPC_CABAL_RITUALIST)
            {
                m_sRitualistsAliveGUIDSet.erase(pCreature->GetObjectGuid());

                if (m_sRitualistsAliveGUIDSet.empty())
                {
                    if (Creature* pHellmaw = GetSingleCreatureFromStorage(NPC_HELLMAW))
                    {
                        // yell intro and remove banish aura
                        DoScriptText(SAY_HELLMAW_INTRO, pHellmaw);
                        pHellmaw->GetMotionMaster()->MoveWaypoint();
                        pHellmaw->RemoveAurasDueToSpell(SPELL_BANISH);
                    }
                }
            }
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            switch (uiType)
            {
            case TYPE_HELLMAW:
                m_auiEncounter[0] = uiData;
                break;

            case TYPE_INCITER:
                if (uiData == DONE)
                    DoUseDoorOrButton(GO_REFECTORY_DOOR);
                m_auiEncounter[1] = uiData;
                break;

            case TYPE_VORPIL:
                if (uiData == DONE)
                    DoUseDoorOrButton(GO_SCREAMING_HALL_DOOR);
                m_auiEncounter[2] = uiData;
                break;

            case TYPE_MURMUR:
                m_auiEncounter[3] = uiData;
                break;
            }

            if (uiData == DONE)
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << m_auiEncounter[0] << " " << m_auiEncounter[1] << " "
                    << m_auiEncounter[2] << " " << m_auiEncounter[3];

                m_strInstData = saveStream.str();

                SaveToDB();
                OUT_SAVE_INST_DATA_COMPLETE;
            }
        }

        uint32 GetData(uint32 uiType) const override
        {
            switch (uiType)
            {
            case TYPE_HELLMAW:  return m_auiEncounter[0];
            case TYPE_INCITER:  return m_auiEncounter[1];
            case TYPE_VORPIL:   return m_auiEncounter[2];
            case TYPE_MURMUR:   return m_auiEncounter[3];
            case TYPE_IS_UNBANISHED: return uint32(m_sRitualistsAliveGUIDSet.empty());

            default:
                return 0;
            }
        }

        void SetData64(uint32 uiType, uint64 uiGuid) override
        {
            // If Hellmaw already completed, just ignore
            if (GetData(TYPE_HELLMAW) == DONE)
                return;

            // Note: this is handled in Acid. The purpose is check which Cabal Ritualists is alive, in case of server reset
            // The function is triggered by eventAI on generic timer
            if (uiType == DATA_CABAL_RITUALIST)
                m_sRitualistsAliveGUIDSet.insert(ObjectGuid(uiGuid));
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
                {
                    m_auiEncounter[i] = NOT_STARTED;
                }
            }

            OUT_LOAD_INST_DATA_COMPLETE;
        }

    private:
        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strInstData;

        GuidSet m_sRitualistsAliveGUIDSet;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_shadow_labyrinth(pMap);
    }
};

void AddSC_instance_shadow_labyrinth()
{
    Script* s;

    s = new is_shadow_labyrinth();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_shadow_labyrinth";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_shadow_labyrinth;
    //pNewScript->RegisterSelf();
}

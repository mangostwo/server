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
SDName: Instance_Steam_Vault
SD%Complete: 80
SDComment: Instance script and access panel GO
SDCategory: Coilfang Resevoir, The Steamvault
EndScriptData */

#include "precompiled.h"
#include "steam_vault.h"

/* Steam Vaults encounters:
1 - Hydromancer Thespia Event
2 - Mekgineer Steamrigger Event
3 - Warlord Kalithresh Event
*/

struct is_steam_vault : public InstanceScript
{
    is_steam_vault() : InstanceScript("instance_steam_vault") {}

    class instance_steam_vault : public ScriptedInstance
    {
    public:
        instance_steam_vault(Map* pMap) : ScriptedInstance(pMap)
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
            case NPC_STEAMRIGGER:
            case NPC_KALITHRESH:
                m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
                break;
            case NPC_NAGA_DISTILLER:
                m_lNagaDistillerGuidList.push_back(pCreature->GetObjectGuid());
                break;
            }
        }

        void OnObjectCreate(GameObject* pGo) override
        {
            switch (pGo->GetEntry())
            {
            case GO_MAIN_CHAMBERS_DOOR:
                if (m_auiEncounter[TYPE_HYDROMANCER_THESPIA] == SPECIAL && m_auiEncounter[TYPE_MEKGINEER_STEAMRIGGER] == SPECIAL)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_ACCESS_PANEL_HYDRO:
                if (m_auiEncounter[TYPE_HYDROMANCER_THESPIA] == DONE)
                {
                    pGo->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NO_INTERACT);
                }
                break;
            case GO_ACCESS_PANEL_MEK:
                if (m_auiEncounter[TYPE_MEKGINEER_STEAMRIGGER] == DONE)
                {
                    pGo->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NO_INTERACT);
                }
                break;
            default:
                return;
            }
            m_mGoEntryGuidStore[pGo->GetEntry()] = pGo->GetObjectGuid();
        }

        void OnCreatureDeath(Creature* pCreature) override
        {
            // Break the Warlord spell on the Distiller death
            if (pCreature->GetEntry() == NPC_NAGA_DISTILLER)
            {
                if (Creature* pWarlord = GetSingleCreatureFromStorage(NPC_KALITHRESH))
                {
                    pWarlord->InterruptNonMeleeSpells(false);
                }
            }
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            switch (uiType)
            {
            case TYPE_HYDROMANCER_THESPIA:
                if (uiData == DONE)
                {
                    DoToggleGameObjectFlags(GO_ACCESS_PANEL_HYDRO, GO_FLAG_NO_INTERACT, false);
                }
                if (uiData == SPECIAL)
                {
                    if (GetData(TYPE_MEKGINEER_STEAMRIGGER) == SPECIAL)
                    {
                        DoUseDoorOrButton(GO_MAIN_CHAMBERS_DOOR);
                    }
                }
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_MEKGINEER_STEAMRIGGER:
                if (uiData == DONE)
                {
                    DoToggleGameObjectFlags(GO_ACCESS_PANEL_MEK, GO_FLAG_NO_INTERACT, false);
                }
                if (uiData == SPECIAL)
                {
                    if (GetData(TYPE_HYDROMANCER_THESPIA) == SPECIAL)
                    {
                        DoUseDoorOrButton(GO_MAIN_CHAMBERS_DOOR);
                    }
                }
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_WARLORD_KALITHRESH:
                DoUseDoorOrButton(GO_MAIN_CHAMBERS_DOOR);
                if (uiData == FAIL)
                {
                    // Reset Distiller flags - respawn is handled by DB
                    for (GuidList::const_iterator itr = m_lNagaDistillerGuidList.begin(); itr != m_lNagaDistillerGuidList.end(); ++itr)
                    {
                        if (Creature* pDistiller = instance->GetCreature(*itr))
                        {
                            if (!pDistiller->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
                            {
                                pDistiller->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                            }
                        }
                    }
                }
                m_auiEncounter[uiType] = uiData;
                break;
            }

            if (uiData == DONE || uiData == SPECIAL)
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
            loadStream >> m_auiEncounter[0] >> m_auiEncounter[1] >> m_auiEncounter[2];

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

        GuidList m_lNagaDistillerGuidList;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_steam_vault(pMap);
    }
};

struct go_main_chambers_access_panel : public GameObjectScript
{
    go_main_chambers_access_panel() : GameObjectScript("go_main_chambers_access_panel") {}

    bool OnUse(Player* /*pPlayer*/, GameObject* pGo) override
    {
        ScriptedInstance* pInstance = (ScriptedInstance*)pGo->GetInstanceData();

        if (!pInstance)
        {
            return true;
        }

        if (pGo->GetEntry() == GO_ACCESS_PANEL_HYDRO)
        {
            pInstance->SetData(TYPE_HYDROMANCER_THESPIA, SPECIAL);
        }
        else if (pGo->GetEntry() == GO_ACCESS_PANEL_MEK)
        {
            pInstance->SetData(TYPE_MEKGINEER_STEAMRIGGER, SPECIAL);
        }

        return false;
    }
};

void AddSC_instance_steam_vault()
{
    Script* s;

    s = new is_steam_vault();
    s->RegisterSelf();
    s = new go_main_chambers_access_panel();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "go_main_chambers_access_panel";
    //pNewScript->pGOUse = &GOUse_go_main_chambers_access_panel;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_steam_vault";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_steam_vault;
    //pNewScript->RegisterSelf();
}

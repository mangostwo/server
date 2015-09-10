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
SDName: Instance_Serpent_Shrine
SD%Complete: 90
SDComment:
SDCategory: Coilfang Resevoir, Serpent Shrine Cavern
EndScriptData */

#include "precompiled.h"
#include "serpent_shrine.h"

/* Serpentshrine cavern encounters:
0 - Hydross The Unstable event
1 - Leotheras The Blind Event
2 - The Lurker Below Event
3 - Fathom-Lord Karathress Event
4 - Morogrim Tidewalker Event
5 - Lady Vashj Event
*/

struct is_serpentshrine_cavern : public InstanceScript
{
    is_serpentshrine_cavern() : InstanceScript("instance_serpent_shrine") {}

    class instance_serpentshrine_cavern : public ScriptedInstance
    {
    public:
        instance_serpentshrine_cavern(Map* pMap) : ScriptedInstance(pMap),
            m_uiSpellBinderCount(0)
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
            {
                if (m_auiEncounter[i] == IN_PROGRESS)
                {
                    return true;
                }
            }

            return false;
        }

        void OnCreatureCreate(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_LADYVASHJ:
            case NPC_SHARKKIS:
            case NPC_TIDALVESS:
            case NPC_CARIBDIS:
            case NPC_LEOTHERAS:
            case NPC_HYDROSS_THE_UNSTABLE:
                m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
                break;
            case NPC_GREYHEART_SPELLBINDER:
                m_lSpellBindersGUIDList.push_back(pCreature->GetObjectGuid());
                break;
            case NPC_HYDROSS_BEAM_HELPER:
                m_lBeamHelpersGUIDList.push_back(pCreature->GetObjectGuid());
                break;
            case NPC_SHIELD_GENERATOR:
                m_lShieldGeneratorGUIDList.push_back(pCreature->GetObjectGuid());
                break;
            case NPC_COILFANG_PRIESTESS:
            case NPC_COILFANG_SHATTERER:
            case NPC_VASHJIR_HONOR_GUARD:
            case NPC_GREYHEART_TECHNICIAN:
                // Filter only the mobs spawned on the platforms
                if (pCreature->GetPositionZ() > 0)
                {
                    m_sPlatformMobsGUIDSet.insert(pCreature->GetObjectGuid());
                }
                break;
            }
        }

        void OnCreatureEnterCombat(Creature* pCreature) override
        {
            // Interrupt spell casting on aggro
            if (pCreature->GetEntry() == NPC_GREYHEART_SPELLBINDER)
            {
                pCreature->InterruptNonMeleeSpells(false);
            }
        }

        void OnCreatureDeath(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_GREYHEART_SPELLBINDER:
                ++m_uiSpellBinderCount;

                if (m_uiSpellBinderCount == MAX_SPELLBINDERS)
                {
                    if (Creature* pLeotheras = GetSingleCreatureFromStorage(NPC_LEOTHERAS))
                    {
                        pLeotheras->RemoveAurasDueToSpell(SPELL_LEOTHERAS_BANISH);
                        pLeotheras->SetInCombatWithZone();
                    }
                }
                break;
            case NPC_COILFANG_PRIESTESS:
            case NPC_COILFANG_SHATTERER:
            case NPC_VASHJIR_HONOR_GUARD:
            case NPC_GREYHEART_TECHNICIAN:
                if (m_sPlatformMobsGUIDSet.find(pCreature->GetObjectGuid()) != m_sPlatformMobsGUIDSet.end())
                {
                    m_sPlatformMobsAliveGUIDSet.erase(pCreature->GetObjectGuid());
                }
                break;
            }
        }

        void OnObjectCreate(GameObject* pGo) override
        {
            switch (pGo->GetEntry())
            {
            case GO_SHIELD_GENERATOR_1:
            case GO_SHIELD_GENERATOR_2:
            case GO_SHIELD_GENERATOR_3:
            case GO_SHIELD_GENERATOR_4:
                m_mGoEntryGuidStore[pGo->GetEntry()] = pGo->GetObjectGuid();
                break;
            }
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            switch (uiType)
            {
            case TYPE_HYDROSS_EVENT:
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_LEOTHERAS_EVENT:
                m_auiEncounter[uiType] = uiData;
                if (uiData == FAIL)
                {
                    for (GuidList::const_iterator itr = m_lSpellBindersGUIDList.begin(); itr != m_lSpellBindersGUIDList.end(); ++itr)
                    {
                        if (Creature* pSpellBinder = instance->GetCreature(*itr))
                        {
                            pSpellBinder->Respawn();
                        }
                    }

                    m_uiSpellBinderCount = 0;
                }
                break;
            case TYPE_THELURKER_EVENT:
            case TYPE_KARATHRESS_EVENT:
            case TYPE_MOROGRIM_EVENT:
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_LADYVASHJ_EVENT:
                m_auiEncounter[uiType] = uiData;
                if (uiData == FAIL)
                {
                    // interrupt the shield
                    for (GuidList::const_iterator itr = m_lShieldGeneratorGUIDList.begin(); itr != m_lShieldGeneratorGUIDList.end(); ++itr)
                    {
                        if (Creature* pGenerator = instance->GetCreature(*itr))
                        {
                            pGenerator->InterruptNonMeleeSpells(false);
                        }
                    }

                    // reset generators
                    DoToggleGameObjectFlags(GO_SHIELD_GENERATOR_1, GO_FLAG_NO_INTERACT, false);
                    DoToggleGameObjectFlags(GO_SHIELD_GENERATOR_2, GO_FLAG_NO_INTERACT, false);
                    DoToggleGameObjectFlags(GO_SHIELD_GENERATOR_3, GO_FLAG_NO_INTERACT, false);
                    DoToggleGameObjectFlags(GO_SHIELD_GENERATOR_4, GO_FLAG_NO_INTERACT, false);
                }
                break;
            case TYPE_DO_HANDLE_BEAMS:
                for (GuidList::const_iterator itr = m_lBeamHelpersGUIDList.begin(); itr != m_lBeamHelpersGUIDList.end(); ++itr)
                {
                    if (Creature* pBeam = instance->GetCreature(*itr))
                    {
                        if (uiData)
                        {
                            pBeam->InterruptNonMeleeSpells(false);
                        }
                        else
                        {
                            Creature *hydross = GetSingleCreatureFromStorage(NPC_HYDROSS_THE_UNSTABLE);
                            if (hydross)
                                pBeam->CastSpell(hydross, SPELL_BLUE_BEAM, false);
                        }
                    }
                }
                return;
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
            {
                return m_auiEncounter[uiType];
            }

            return 0;
        }

        void SetData64(uint32 uiType, uint64 uiGuid) override
        {
            switch (uiType)
            {
            case DATA_WATERSTATE_EVENT:
                // Note: this is handled in Acid. The purpose is check which npc from the platform set is alive
                // The function is triggered by eventAI on generic timer
                if (uiType == DATA_WATERSTATE_EVENT)
                {
                    if (m_sPlatformMobsGUIDSet.find(ObjectGuid(uiGuid)) != m_sPlatformMobsGUIDSet.end())
                    {
                        m_sPlatformMobsAliveGUIDSet.insert(ObjectGuid(uiGuid));
                    }
                }
                break;
            case TYPE_DO_VASHJ_GENERATORS:
                if (Creature *vashj = instance->GetCreature(ObjectGuid(uiGuid)))
                {
                    for (GuidList::const_iterator itr = m_lShieldGeneratorGUIDList.begin(); itr != m_lShieldGeneratorGUIDList.end(); ++itr)
                    {
                        if (Creature* pGenerator = instance->GetCreature(*itr))
                        {
                            pGenerator->CastSpell(vashj, SPELL_MAGIC_BARRIER, false);
                        }
                    }
                }
                break;
            default:
                break;
            }
        }

        bool CheckConditionCriteriaMeet(Player const* pPlayer, uint32 uiInstanceConditionId, WorldObject const* pConditionSource, uint32 conditionSourceType) const override
        {
            switch (uiInstanceConditionId)
            {
            case INSTANCE_CONDITION_ID_LURKER:
                return GetData(TYPE_THELURKER_EVENT) != DONE;
            case INSTANCE_CONDITION_ID_SCALDING_WATER:
                return m_sPlatformMobsAliveGUIDSet.empty();
            }

            script_error_log("instance_serpentshrine_cavern::CheckConditionCriteriaMeet called with unsupported Id %u. Called with param plr %s, src %s, condition source type %u",
                uiInstanceConditionId, pPlayer ? pPlayer->GetGuidStr().c_str() : "NULL", pConditionSource ? pConditionSource->GetGuidStr().c_str() : "NULL", conditionSourceType);
            return false;
        }

        void GetBeamHelpersGUIDList(GuidList& lList) { lList = m_lBeamHelpersGUIDList; }
        void GetShieldGeneratorsGUIDList(GuidList& lList) { lList = m_lShieldGeneratorGUIDList; }

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
                >> m_auiEncounter[4] >> m_auiEncounter[5];

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

        uint32 m_uiSpellBinderCount;

        GuidList m_lSpellBindersGUIDList;
        GuidList m_lBeamHelpersGUIDList;
        GuidList m_lShieldGeneratorGUIDList;
        GuidSet m_sPlatformMobsGUIDSet;
        GuidSet m_sPlatformMobsAliveGUIDSet;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_serpentshrine_cavern(pMap);
    }
};

void AddSC_instance_serpentshrine_cavern()
{
    Script* s;

    s = new is_serpentshrine_cavern();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_serpent_shrine";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_serpentshrine_cavern;
    //pNewScript->RegisterSelf();
}

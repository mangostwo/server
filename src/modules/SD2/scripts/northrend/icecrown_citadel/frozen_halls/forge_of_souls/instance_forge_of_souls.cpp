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
SDName: instance_forge_of_souls
SD%Complete: 90%
SDComment: TODO: Movement of the extro-event is missing, implementation unclear!
SDCategory: The Forge of Souls
EndScriptData */

#include "precompiled.h"
#include "forge_of_souls.h"

struct sIntoEventNpcSpawnLocations
{
    uint32 uiEntryHorde, uiEntryAlliance;
    float fSpawnX, fSpawnY, fSpawnZ, fSpawnO;
};

/* Still TODO
** We have 12 npc-entries to do moving, and often different waypoints for one entry
** Best way to handle the paths of these mobs is still open
*/

struct sExtroEventNpcLocations
{
    uint32 uiEntryHorde, uiEntryAlliance;
    float fStartO, fEndO;                                   // Orientation for Spawning
    float fSpawnX, fSpawnY, fSpawnZ;
    float fEndX, fEndY, fEndZ;
};

// TODO: verify Horde - Entries
const sIntoEventNpcSpawnLocations aEventBeginLocations[3] =
{
    { NPC_SILVANA_BEGIN, NPC_JAINA_BEGIN, 4901.25439f, 2206.861f, 638.8166f, 5.88175964f },
    { NPC_DARK_RANGER_KALIRA, NPC_ARCHMAGE_ELANDRA, 4899.709961f, 2205.899902f, 638.817017f, 5.864306f },
    { NPC_DARK_RANGER_LORALEN, NPC_ARCHMAGE_KORELN, 4903.160156f, 2213.090088f, 638.817017f, 0.1745329f }
};

const sExtroEventNpcLocations aEventEndLocations[18] =
{
    // Horde Entry              Ally Entry                 O_Spawn     O_End      SpawnPos                             EndPos
    { NPC_SILVANA_END, NPC_JAINA_END, 0.8901179f, 0.890118f, 5606.34033f, 2436.32129f, 705.9351f, 5638.404f, 2477.154f, 708.6932f },
    { NPC_COLISEUM_CHAMPION_H_F, NPC_COLISEUM_CHAMPION_A_F, 0.9773844f, 1.780236f, 5593.632f, 2428.57983f, 705.9351f, 5695.879f, 2522.944f, 714.6915f },
    { NPC_COLISEUM_CHAMPION_H_F, NPC_COLISEUM_CHAMPION_A_F, 1.1519173f, 1.78023f, 5594.079f, 2425.111f, 705.9351f, 5692.123f, 2522.613f, 714.6915f },
    { NPC_COLISEUM_CHAMPION_H_F, NPC_COLISEUM_CHAMPION_A_F, 0.6108652f, 0.296706f, 5597.932f, 2421.78125f, 705.9351f, 5669.314f, 2540.029f, 714.6915f },
    { NPC_COLISEUM_CHAMPION_H_F, NPC_COLISEUM_CHAMPION_A_F, 1.0471975f, 5.358161f, 5598.03564f, 2429.37671f, 705.9351f, 5639.267f, 2520.912f, 708.6959f },
    { NPC_COLISEUM_CHAMPION_H_F, NPC_COLISEUM_CHAMPION_A_F, 0.8901179f, 0.112373f, 5600.836f, 2421.35938f, 705.9351f, 5668.145f, 2543.854f, 714.6915f },
    { NPC_COLISEUM_CHAMPION_H_F, NPC_COLISEUM_CHAMPION_A_F, 0.8901179f, 5.358161f, 5600.848f, 2429.54517f, 705.9351f, 5639.961f, 2522.936f, 708.6959f },
    { NPC_COLISEUM_CHAMPION_H_F, NPC_COLISEUM_CHAMPION_A_F, 0.8901179f, 5.347504f, 5601.46533f, 2426.77075f, 705.9351f, 5643.156f, 2525.342f, 708.6958f },
    { NPC_COLISEUM_CHAMPION_H_F, NPC_COLISEUM_CHAMPION_A_F, 1.1519173f, 0.232039f, 5601.587f, 2418.60425f, 705.9351f, 5670.483f, 2536.204f, 714.6915f },
    { NPC_DARK_RANGER_LORALEN, NPC_ARCHMAGE_KORELN, 0.7853982f, 3.717551f, 5606.35059f, 2432.88013f, 705.9351f, 5688.9f, 2538.981f, 714.6915f },
    { NPC_DARK_RANGER_KALIRA, NPC_ARCHMAGE_ELANDRA, 0.9599311f, 4.694936f, 5602.80371f, 2435.66846f, 705.9351f, 5685.069f, 2541.771f, 714.6915f },
    { NPC_COLISEUM_CHAMPION_H_T, NPC_COLISEUM_CHAMPION_A_P, 0.8552113f, 1.958489f, 5589.8125f, 2421.27075f, 705.9351f, 5669.351f, 2472.626f, 708.6959f },
    { NPC_COLISEUM_CHAMPION_H_T, NPC_COLISEUM_CHAMPION_A_P, 0.8552113f, 2.111848f, 5592.2666f, 2419.37842f, 705.9351f, 5665.927f, 2470.574f, 708.6959f },
    { NPC_COLISEUM_CHAMPION_H_T, NPC_COLISEUM_CHAMPION_A_P, 0.9075712f, 2.196496f, 5594.64746f, 2417.10767f, 705.9351f, 5662.503f, 2468.522f, 708.6958f },
    { NPC_COLISEUM_CHAMPION_H_M, NPC_COLISEUM_CHAMPION_A_M, 1.0646508f, 0.837758f, 5585.49854f, 2418.22925f, 705.9351f, 5624.832f, 2473.713f, 708.6959f },
    { NPC_COLISEUM_CHAMPION_H_M, NPC_COLISEUM_CHAMPION_A_M, 0.9424777f, 0.837758f, 5586.80029f, 2416.97388f, 705.9351f, 5627.443f, 2472.236f, 708.6959f },
    { NPC_COLISEUM_CHAMPION_H_M, NPC_COLISEUM_CHAMPION_A_M, 0.9250245f, 0.977384f, 5591.653f, 2412.89771f, 705.9351f, 5637.912f, 2465.69f, 708.6959f },
    { NPC_COLISEUM_CHAMPION_H_M, NPC_COLISEUM_CHAMPION_A_M, 0.8726646f, 0.977384f, 5593.93652f, 2410.875f, 705.9351f, 5642.629f, 2474.331f, 708.6959f }
};

struct is_forge_of_souls : public InstanceScript
{
    is_forge_of_souls() : InstanceScript("instance_forge_of_souls") {}

    class  instance_forge_of_souls : public ScriptedInstance
    {
    public:
        instance_forge_of_souls(Map* pMap) : ScriptedInstance(pMap),
            m_bCriteriaPhantomBlastFailed(false),
            m_uiTeam(0)
        {
            Initialize();
        }

        ~instance_forge_of_souls() {}

        void Initialize() override
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
        }

        void OnCreatureCreate(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_BRONJAHM:
                m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
                break;

            case NPC_CORRUPTED_SOUL_FRAGMENT:
                m_luiSoulFragmentAliveGUIDs.push_back(pCreature->GetObjectGuid());
                break;
            }
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            switch (uiType)
            {
            case TYPE_BRONJAHM:
                m_auiEncounter[0] = uiData;

                // Despawn remaining adds and clear list
                for (GuidList::const_iterator itr = m_luiSoulFragmentAliveGUIDs.begin(); itr != m_luiSoulFragmentAliveGUIDs.end(); ++itr)
                {
                    if (Creature* pFragment = instance->GetCreature(*itr))
                        pFragment->ForcedDespawn();
                }
                m_luiSoulFragmentAliveGUIDs.clear();
                break;
            case TYPE_DEVOURER_OF_SOULS:
                m_auiEncounter[1] = uiData;
                if (uiData == DONE)
                    ProcessEventNpcs(GetPlayerInMap(), true);
                break;
            case TYPE_ACHIEV_PHANTOM_BLAST:
                m_bCriteriaPhantomBlastFailed = (uiData == FAIL);
                return;
            }

            if (uiData == DONE)
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << m_auiEncounter[0] << " " << m_auiEncounter[1];

                m_strInstData = saveStream.str();

                SaveToDB();
                OUT_SAVE_INST_DATA_COMPLETE;
            }
        }

        uint32 GetData(uint32 uiType) const override
        {
            switch (uiType)
            {
            case TYPE_BRONJAHM:
                return m_auiEncounter[0];
            case TYPE_DEVOURER_OF_SOULS:
                return m_auiEncounter[1];
            default:
                return 0;
            }
        }

        void SetData64(uint32 uiType, uint64 uiData) override
        {
            if (uiType == DATA_SOULFRAGMENT_REMOVE)
                m_luiSoulFragmentAliveGUIDs.remove(ObjectGuid(uiData));
        }

        void OnPlayerEnter(Player* pPlayer) override
        {
            if (!m_uiTeam)                                          // very first player to enter
            {
                m_uiTeam = pPlayer->GetTeam();
                ProcessEventNpcs(pPlayer, false);
            }
        }

        bool CheckAchievementCriteriaMeet(uint32 uiCriteriaId, Player const* pSource, Unit const* pTarget, uint32 uiMiscValue1 /* = 0*/) const override
        {
            switch (uiCriteriaId)
            {
            case ACHIEV_CRIT_SOUL_POWER:
                return m_luiSoulFragmentAliveGUIDs.size() >= 4;
            case ACHIEV_CRIT_PHANTOM_BLAST:
                return !m_bCriteriaPhantomBlastFailed;
            default:
                return 0;
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
            loadStream >> m_auiEncounter[0] >> m_auiEncounter[1];

            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            {
                if (m_auiEncounter[i] == IN_PROGRESS)
                    m_auiEncounter[i] = NOT_STARTED;
            }

            OUT_LOAD_INST_DATA_COMPLETE;
        }

    private:
        void ProcessEventNpcs(Player* pPlayer, bool bChanged)
        {
            if (!pPlayer)
                return;

            if (m_auiEncounter[0] != DONE || m_auiEncounter[1] != DONE)
            {
                // Spawn Begin Mobs
                for (uint8 i = 0; i < countof(aEventBeginLocations); ++i)
                {
                    if (Creature* pSummon = pPlayer->SummonCreature(m_uiTeam == HORDE ? aEventBeginLocations[i].uiEntryHorde : aEventBeginLocations[i].uiEntryAlliance,
                        aEventBeginLocations[i].fSpawnX, aEventBeginLocations[i].fSpawnY, aEventBeginLocations[i].fSpawnZ, aEventBeginLocations[i].fSpawnO, TEMPSUMMON_DEAD_DESPAWN, 24 * HOUR * IN_MILLISECONDS))
                        m_lEventMobGUIDs.push_back(pSummon->GetObjectGuid());
                }
            }
            else
            {
                // if bChanged, despawn Begin Mobs, spawn End Mobs at Spawn, else spawn EndMobs at End
                if (bChanged)
                {
                    for (GuidList::const_iterator itr = m_lEventMobGUIDs.begin(); itr != m_lEventMobGUIDs.end(); ++itr)
                    {
                        if (Creature* pSummoned = instance->GetCreature(*itr))
                            pSummoned->ForcedDespawn();
                    }

                    for (uint8 i = 0; i < countof(aEventEndLocations); ++i)
                    {
                        pPlayer->SummonCreature(m_uiTeam == HORDE ? aEventEndLocations[i].uiEntryHorde : aEventEndLocations[i].uiEntryAlliance,
                            aEventEndLocations[i].fSpawnX, aEventEndLocations[i].fSpawnY, aEventEndLocations[i].fSpawnZ, aEventEndLocations[i].fStartO, TEMPSUMMON_DEAD_DESPAWN, 24 * HOUR * IN_MILLISECONDS);

                        // TODO: Let the NPCs Move along their paths
                    }
                }
                else
                {
                    // Summon at end, without event
                    for (uint8 i = 0; i < countof(aEventEndLocations); ++i)
                    {
                        pPlayer->SummonCreature(m_uiTeam == HORDE ? aEventEndLocations[i].uiEntryHorde : aEventEndLocations[i].uiEntryAlliance,
                            aEventEndLocations[i].fEndX, aEventEndLocations[i].fEndY, aEventEndLocations[i].fEndZ, aEventEndLocations[i].fEndO, TEMPSUMMON_DEAD_DESPAWN, 24 * HOUR * IN_MILLISECONDS);
                    }
                }
            }
        }

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strInstData;

        bool m_bCriteriaPhantomBlastFailed;

        uint32 m_uiTeam;                                    // Team of first entered player, used to set if Jaina or Silvana to spawn

        GuidList m_luiSoulFragmentAliveGUIDs;
        GuidList m_lEventMobGUIDs;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_forge_of_souls(pMap);
    }
};

void AddSC_instance_forge_of_souls()
{
    Script* s;

    s = new is_forge_of_souls();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_forge_of_souls";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_forge_of_souls;
    //pNewScript->RegisterSelf();
}

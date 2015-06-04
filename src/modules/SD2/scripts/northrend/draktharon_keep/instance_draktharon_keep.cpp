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
SDName: instance_draktharon_keep
SD%Complete: 50%
SDComment:
SDCategory: Drak'Tharon Keep
EndScriptData */

#include "precompiled.h"
#include "draktharon_keep.h"

struct NovosCrystalInfo
{
    ObjectGuid m_crystalGuid;
    ObjectGuid m_channelGuid;
    bool m_bWasUsed;
};

enum
{
    // Achievement Criterias to be handled with SD2
    ACHIEV_CRIT_BETTER_OFF_DREAD= 7318,
    ACHIEV_CRIT_CONSUME_JUNCTION= 7579,
    ACHIEV_CRIT_OH_NOVOS        = 7361,

    SPELL_SUMMON_INVADER_1      = 49456,            // summon 27709
    SPELL_SUMMON_INVADER_2      = 49457,            // summon 27753
};

struct is_draktharon_keep : public InstanceScript
{
    is_draktharon_keep() : InstanceScript("instance_draktharon_keep") {}

    class instance_draktharon_keep : public ScriptedInstance
    {
    public:
        instance_draktharon_keep(Map* pMap) : ScriptedInstance(pMap),
            m_uiDreadAddsKilled(0),
            m_bNovosAddGrounded(false),
            m_bTrollgoreConsume(true),
            m_uiNovosCrystalIndex(0)
        {
            Initialize();
        }

        ~instance_draktharon_keep() {}

        void Initialize() override
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            switch (uiType)
            {
            case TYPE_TROLLGORE:
                if (uiData == IN_PROGRESS)
                    m_bTrollgoreConsume = true;
                if (uiData == SPECIAL)
                    m_bTrollgoreConsume = false;
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_NOVOS:
                if (uiData == IN_PROGRESS)
                {
                    // Sort the dummies
                    DoSortNovosDummies();

                    // Cast some visual spells
                    Creature* pTarget = instance->GetCreature(m_novosChannelGuid);
                    for (uint8 i = 0; i < MAX_CRYSTALS; ++i)
                    {
                        Creature* pCaster = instance->GetCreature(m_aNovosCrystalInfo[i].m_channelGuid);
                        if (pCaster && pTarget)
                            pCaster->CastSpell(pTarget, SPELL_BEAM_CHANNEL, false);

                        m_aNovosCrystalInfo[i].m_bWasUsed = false;
                    }

                    // Achievement related
                    m_bNovosAddGrounded = false;
                }
                else if (uiData == SPECIAL)
                {
                    // Achievement related
                    m_bNovosAddGrounded = true;
                }
                else if (uiData == FAIL)
                {
                    // Interrupt casted spells
                    for (uint8 i = 0; i < MAX_CRYSTALS; ++i)
                    {
                        Creature* pDummy = instance->GetCreature(m_aNovosCrystalInfo[i].m_channelGuid);
                        if (pDummy)
                            pDummy->InterruptNonMeleeSpells(false);
                        // And reset used crystals
                        if (m_aNovosCrystalInfo[i].m_bWasUsed)
                            DoUseDoorOrButton(m_aNovosCrystalInfo[i].m_crystalGuid);
                    }
                }
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_KING_DRED:
                if (uiData == IN_PROGRESS)
                    m_uiDreadAddsKilled = 0;
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_THARONJA:
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_DATA_NOVOS_CRYSTAL_INDEX:
                DoHandleCrystal(uint8(uiData));
                return;
            case TYPE_DO_TROLLGORE:
                DoSummonDrakkariInvaders();
                return;
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
            switch (uiType)
            {
            case TYPE_DATA_NOVOS_CRYSTAL_INDEX:
                return uint32(m_uiNovosCrystalIndex);
            default:
                return 0;
            }
        }

        void OnCreatureEnterCombat(Creature* pCreature) override
        {
            if (pCreature->GetEntry() == NPC_KING_DRED)
                SetData(TYPE_KING_DRED, IN_PROGRESS);
        }

        void OnCreatureEvade(Creature* pCreature) override
        {
            if (pCreature->GetEntry() == NPC_KING_DRED)
                SetData(TYPE_KING_DRED, FAIL);
        }

        void OnCreatureDeath(Creature* pCreature) override
        {
            if ((pCreature->GetEntry() == NPC_DRAKKARI_GUTRIPPER || pCreature->GetEntry() == NPC_DRAKKARI_SCYTHECLAW) && m_auiEncounter[TYPE_KING_DRED] == IN_PROGRESS)
                ++m_uiDreadAddsKilled;

            if (pCreature->GetEntry() == NPC_KING_DRED)
                SetData(TYPE_KING_DRED, DONE);
        }

        void OnCreatureCreate(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_NOVOS:
            case NPC_TROLLGORE:
                m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
                break;
            case NPC_CRYSTAL_CHANNEL_TARGET:
                m_lNovosDummyGuids.push_back(pCreature->GetObjectGuid());
                break;
            case NPC_WORLD_TRIGGER:
                if (pCreature->GetPositionZ() > 30.0f)
                    m_vTriggerGuids.push_back(pCreature->GetObjectGuid());
                else
                    m_trollgoreCornerTriggerGuid = pCreature->GetObjectGuid();
                break;
            }
        }

        void OnObjectCreate(GameObject* pGo) override
        {
            switch (pGo->GetEntry())
            {
            case GO_CRYSTAL_SW: m_aNovosCrystalInfo[0].m_crystalGuid = pGo->GetObjectGuid(); break;
            case GO_CRYSTAL_NW: m_aNovosCrystalInfo[1].m_crystalGuid = pGo->GetObjectGuid(); break;
            case GO_CRYSTAL_SE: m_aNovosCrystalInfo[2].m_crystalGuid = pGo->GetObjectGuid(); break;
            case GO_CRYSTAL_NE: m_aNovosCrystalInfo[3].m_crystalGuid = pGo->GetObjectGuid(); break;
            }
        }

        void GetTrollgoreOutsideTriggers(GuidVector& vTriggers) { vTriggers = m_vTriggerGuids; }
        ObjectGuid GetTrollgoreCornerTrigger() { return m_trollgoreCornerTriggerGuid; }

        bool CheckAchievementCriteriaMeet(uint32 uiCriteriaId, Player const* pSource, Unit const* pTarget, uint32 uiMiscValue1 /* = 0*/) const override
        {
            switch (uiCriteriaId)
            {
            case ACHIEV_CRIT_BETTER_OFF_DREAD: return m_uiDreadAddsKilled >= 6;
            case ACHIEV_CRIT_OH_NOVOS:         return !m_bNovosAddGrounded;
            case ACHIEV_CRIT_CONSUME_JUNCTION: return m_bTrollgoreConsume;
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

        void SetData64(uint32 uiType, uint64 uiData) override
        {
            switch (uiType)
            {
            case DATA64_NOVOS_CRYSTAL_HANDLER:
            {
                Creature* pCrystalHandler = instance->GetCreature(ObjectGuid(uiData));
                Creature* pTarget = NULL;
                m_uiNovosCrystalIndex = MAX_CRYSTALS;

                for (uint8 i = 0; i < MAX_CRYSTALS; ++i)
                {
                    Creature* pDummy = instance->GetCreature(m_aNovosCrystalInfo[i].m_channelGuid);
                    // Return the nearest 'unused' crystal dummy
                    // unused means, that the crystal was not already used, and the dummy-npc doesn't have the aura that will trigger the use on remove
                    if (pDummy && !m_aNovosCrystalInfo[i].m_bWasUsed && (!pTarget || pCrystalHandler->GetDistanceOrder(pDummy, pTarget)) && !pDummy->HasAura(aCrystalHandlerDeathSpells[i]))
                    {
                        pTarget = pDummy;
                        m_uiNovosCrystalIndex = i;
                    }
                }
            }
                break;
            default:
                break;
            }
        }

        uint64 GetData64(uint32 uiType) const override
        {
            switch (uiType)
            {
            case DATA64_NOVOS_SUMMON_DUMMY:
                if (!m_vSummonDummyGuids.empty())
                    m_vSummonDummyGuids[urand(0, m_vSummonDummyGuids.size() - 1)].GetRawValue();
                break;
            case DATA64_NOVOS_CRYSTAL_HANDLER:
                return m_uiNovosCrystalIndex < MAX_CRYSTALS ? m_aNovosCrystalInfo[m_uiNovosCrystalIndex].m_channelGuid : 0;
            default:
                break;
            }
            return 0;
        }

    private:
        void DoSortNovosDummies()
        {
            // Sorting once is good enough
            if (m_lNovosDummyGuids.empty())
                return;

            Creature* pNovos = GetSingleCreatureFromStorage(NPC_NOVOS);
            if (!pNovos)
                return;

            // First sort the Dummies to the Crystals
            for (uint8 i = 0; i < MAX_CRYSTALS; ++i)
            {
                GameObject* pCrystal = instance->GetGameObject(m_aNovosCrystalInfo[i].m_crystalGuid);
                if (!pCrystal)
                    continue;

                for (GuidList::iterator itr = m_lNovosDummyGuids.begin(); itr != m_lNovosDummyGuids.end();)
                {
                    Creature* pDummy = instance->GetCreature(*itr);
                    if (!pDummy)
                    {
                        m_lNovosDummyGuids.erase(itr++);
                        continue;
                    }

                    // Check if dummy fits to crystal
                    if (pCrystal->IsWithinDistInMap(pDummy, INTERACTION_DISTANCE, false))
                    {
                        m_aNovosCrystalInfo[i].m_channelGuid = pDummy->GetObjectGuid();
                        m_lNovosDummyGuids.erase(itr);
                        break;
                    }

                    ++itr;
                }
            }

            // Find the crystal channel target (above Novos)
            float fNovosX, fNovosY, fNovosZ;
            pNovos->GetRespawnCoord(fNovosX, fNovosY, fNovosZ);
            for (GuidList::iterator itr = m_lNovosDummyGuids.begin(); itr != m_lNovosDummyGuids.end();)
            {
                Creature* pDummy = instance->GetCreature(*itr);
                if (!pDummy)
                {
                    m_lNovosDummyGuids.erase(itr++);
                    continue;
                }

                // As the wanted dummy is exactly above Novos, check small range, and only 2d
                if (pDummy->IsWithinDist2d(fNovosX, fNovosY, 5.0f))
                {
                    m_novosChannelGuid = pDummy->GetObjectGuid();
                    m_lNovosDummyGuids.erase(itr);
                    break;
                }

                ++itr;
            }

            // Summon positions (at end of stairs)
            for (GuidList::iterator itr = m_lNovosDummyGuids.begin(); itr != m_lNovosDummyGuids.end();)
            {
                Creature* pDummy = instance->GetCreature(*itr);
                if (!pDummy)
                {
                    m_lNovosDummyGuids.erase(itr++);
                    continue;
                }

                // The wanted dummies are quite above Novos
                if (pDummy->GetPositionZ() > fNovosZ + 20.0f)
                {
                    m_vSummonDummyGuids.push_back(pDummy->GetObjectGuid());
                    m_lNovosDummyGuids.erase(itr++);
                }
                else
                    ++itr;
            }

            // Clear remaining (unused) dummies
            m_lNovosDummyGuids.clear();
        }

        void DoHandleCrystal(uint8 uiIndex)
        {
            m_aNovosCrystalInfo[uiIndex].m_bWasUsed = true;

            DoUseDoorOrButton(m_aNovosCrystalInfo[uiIndex].m_crystalGuid);

            if (Creature* pDummy = instance->GetCreature(m_aNovosCrystalInfo[uiIndex].m_channelGuid))
                pDummy->InterruptNonMeleeSpells(false);
        }

        // Wrapper to handle the drakkari invaders summon
        void DoSummonDrakkariInvaders()
        {
            // check if there are there are at least 2 triggers in the vector
            if (m_vTriggerGuids.size() < 2)
                return;

            if (roll_chance_i(30))
            {
                // Summon a troll in the corner and 2 trolls in the air
                if (Creature* pTrigger = instance->GetCreature(GetTrollgoreCornerTrigger()))
                    pTrigger->CastSpell(pTrigger, roll_chance_i(20) ? SPELL_SUMMON_INVADER_1 : SPELL_SUMMON_INVADER_2, true, NULL, NULL, m_mNpcEntryGuidStore[NPC_TROLLGORE]);

                // get two random outside triggers
                uint8 uiMaxTriggers = m_vTriggerGuids.size();
                uint8 uiPos1 = urand(0, uiMaxTriggers - 1);
                uint8 uiPos2 = (uiPos1 + urand(1, uiMaxTriggers - 1)) % uiMaxTriggers;

                if (Creature* pTrigger = instance->GetCreature(m_vTriggerGuids[uiPos1]))
                    pTrigger->CastSpell(pTrigger, roll_chance_i(30) ? SPELL_SUMMON_INVADER_1 : SPELL_SUMMON_INVADER_2, true, NULL, NULL, m_mNpcEntryGuidStore[NPC_TROLLGORE]);
                if (Creature* pTrigger = instance->GetCreature(m_vTriggerGuids[uiPos2]))
                    pTrigger->CastSpell(pTrigger, roll_chance_i(30) ? SPELL_SUMMON_INVADER_1 : SPELL_SUMMON_INVADER_2, true, NULL, NULL, m_mNpcEntryGuidStore[NPC_TROLLGORE]);
            }
            else
            {
                // Summon 3 trolls in the air
                for (uint8 i = 0; i < m_vTriggerGuids.size(); ++i)
                {
                    if (Creature* pTrigger = instance->GetCreature(m_vTriggerGuids[i]))
                        pTrigger->CastSpell(pTrigger, roll_chance_i(30) ? SPELL_SUMMON_INVADER_1 : SPELL_SUMMON_INVADER_2, true, NULL, NULL, m_mNpcEntryGuidStore[NPC_TROLLGORE]);
                }
            }
        }

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strInstData;

        uint32 m_uiDreadAddsKilled;
        bool m_bNovosAddGrounded;
        bool m_bTrollgoreConsume;

        ObjectGuid m_novosChannelGuid;
        ObjectGuid m_trollgoreCornerTriggerGuid;
        uint8 m_uiNovosCrystalIndex;

        NovosCrystalInfo m_aNovosCrystalInfo[MAX_CRYSTALS];

        GuidVector m_vSummonDummyGuids;
        GuidList m_lNovosDummyGuids;
        GuidVector m_vTriggerGuids;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_draktharon_keep(pMap);
    }
};

void AddSC_instance_draktharon_keep()
{
    Script* s;

    s = new is_draktharon_keep();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_draktharon_keep";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_draktharon_keep;
    //pNewScript->RegisterSelf();
}

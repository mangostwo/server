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
SDName: instance_pinnacle
SD%Complete: 75%
SDComment:
SDCategory: Utgarde Pinnacle
EndScriptData */

#include "precompiled.h"
#include "utgarde_pinnacle.h"

static const float aOrbPositions[2][3] =
{
    { 238.6077f, -460.7103f, 112.5671f },                 // Orb lift up
    { 279.26f, -452.1f, 110.0f },                    // Orb center stop
};

static const uint32 aGortokMiniBosses[MAX_ENCOUNTER] = { NPC_WORGEN, NPC_FURBOLG, NPC_JORMUNGAR, NPC_RHINO };

struct is_pinnacle : public InstanceScript
{
    is_pinnacle() : InstanceScript("instance_pinnacle") {}

    class instance_pinnacle : public ScriptedInstance
    {
    public:
        instance_pinnacle(Map* pMap) : ScriptedInstance(pMap),
            m_uiGortokOrbTimer(0),
            m_uiGortokOrbPhase(0)
        {
            Initialize();
        }

        void Initialize() override
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));

            for (uint8 i = 0; i < MAX_SPECIAL_ACHIEV_CRITS; ++i)
                m_abAchievCriteria[i] = false;
        }

        void OnCreatureCreate(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_FURBOLG:
            case NPC_WORGEN:
            case NPC_JORMUNGAR:
            case NPC_RHINO:
            case NPC_BJORN:
            case NPC_HALDOR:
            case NPC_RANULF:
            case NPC_TORGYN:
            case NPC_SKADI:
            case NPC_GRAUF:
                m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
                break;
            case NPC_WORLD_TRIGGER:
                if (pCreature->GetPositionX() < 250.0f)
                    m_gortokEventTriggerGuid = pCreature->GetObjectGuid();
                else if (pCreature->GetPositionX() > 400.0f && pCreature->GetPositionX() < 500.0f)
                    m_skadiMobsTriggerGuid = pCreature->GetObjectGuid();
                break;
            case NPC_YMIRJAR_HARPOONER:
            case NPC_YMIRJAR_WARRIOR:
            case NPC_YMIRJAR_WITCH_DOCTOR:
                m_lskadiGauntletMobsList.push_back(pCreature->GetObjectGuid());
                break;
            }
        }

        void OnObjectCreate(GameObject* pGo) override
        {
            switch (pGo->GetEntry())
            {
            case GO_DOOR_SKADI:
                if (m_auiEncounter[TYPE_SKADI] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_DOOR_YMIRON:
                if (m_auiEncounter[TYPE_YMIRON] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            default:
                return;
            }
            m_mGoEntryGuidStore[pGo->GetEntry()] = pGo->GetObjectGuid();
        }

        void OnCreatureEvade(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_FURBOLG:
            case NPC_WORGEN:
            case NPC_JORMUNGAR:
            case NPC_RHINO:
                SetData(TYPE_GORTOK, FAIL);
                break;
            case NPC_YMIRJAR_WARRIOR:
            case NPC_YMIRJAR_WITCH_DOCTOR:
            case NPC_YMIRJAR_HARPOONER:
                // Handle Skadi gauntlet reset. Used instead of using spell 49308
                SetData(TYPE_SKADI, FAIL);
                break;
            }
        }

        void OnCreatureDeath(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_FURBOLG:
            case NPC_WORGEN:
            case NPC_JORMUNGAR:
            case NPC_RHINO:
                m_uiGortokOrbTimer = 3000;
                break;
            }
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            switch (uiType)
            {
            case TYPE_SVALA:
                if (uiData == IN_PROGRESS || uiData == FAIL)
                    SetSpecialAchievementCriteria(TYPE_ACHIEV_INCREDIBLE_HULK, false);
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_GORTOK:
                if (uiData == IN_PROGRESS)
                {
                    if (Creature* pOrb = instance->GetCreature(m_gortokEventTriggerGuid))
                    {
                        pOrb->SetLevitate(true);
                        pOrb->CastSpell(pOrb, SPELL_ORB_VISUAL, true);
                        pOrb->GetMotionMaster()->MovePoint(0, aOrbPositions[0][0], aOrbPositions[0][1], aOrbPositions[0][2]);

                        m_uiGortokOrbTimer = 2000;
                    }
                }
                else if (uiData == FAIL)
                {
                    if (Creature* pOrb = instance->GetCreature(m_gortokEventTriggerGuid))
                    {
                        if (!pOrb->IsAlive())
                            pOrb->Respawn();
                        else
                            pOrb->RemoveAllAuras();

                        // For some reasone the Orb doesn't evade automatically
                        pOrb->GetMotionMaster()->MoveTargetedHome();
                    }

                    for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
                    {
                        // Reset each miniboss
                        if (Creature* pTemp = GetSingleCreatureFromStorage(aGortokMiniBosses[i]))
                        {
                            if (!pTemp->IsAlive())
                                pTemp->Respawn();

                            pTemp->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        }
                    }

                    m_uiGortokOrbPhase = 0;
                }
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_SKADI:
                // Don't process the event twice
                if (m_auiEncounter[uiType] == uiData)
                    return;
                switch (uiData)
                {
                case DONE:
                    DoUseDoorOrButton(GO_DOOR_SKADI);
                    break;
                case SPECIAL:
                    // Prepare achievements
                    SetSpecialAchievementCriteria(TYPE_ACHIEV_LOVE_SKADI, true);
                    DoStartTimedAchievement(ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE, ACHIEV_START_SKADI_ID);

                    m_auiEncounter[uiType] = uiData;
                    return;
                case FAIL:
                    // Handle Grauf evade - if event is in phase 1
                    if (Creature* pGrauf = GetSingleCreatureFromStorage(NPC_GRAUF))
                        pGrauf->AI()->EnterEvadeMode();

                    // no break;
                case NOT_STARTED:
                    // Despawn all summons
                    for (GuidList::const_iterator itr = m_lskadiGauntletMobsList.begin(); itr != m_lskadiGauntletMobsList.end(); ++itr)
                    {
                        if (Creature* pYmirjar = instance->GetCreature(*itr))
                            pYmirjar->ForcedDespawn();
                    }

                    // Reset position
                    if (Creature* pGrauf = GetSingleCreatureFromStorage(NPC_GRAUF))
                        pGrauf->GetMotionMaster()->MoveTargetedHome();

                    // no break;
                case IN_PROGRESS:

                    // Remove the summon aura on phase 2 or fail
                    if (Creature* pTrigger = instance->GetCreature(m_skadiMobsTriggerGuid))
                        pTrigger->RemoveAllAuras();
                    break;
                }
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_YMIRON:
                if (uiData == DONE)
                    DoUseDoorOrButton(GO_DOOR_YMIRON);
                else if (uiData == IN_PROGRESS)
                    SetSpecialAchievementCriteria(TYPE_ACHIEV_KINGS_BANE, true);
                else if (uiData == SPECIAL)
                    SetSpecialAchievementCriteria(TYPE_ACHIEV_KINGS_BANE, false);
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_ACHIEV_KINGS_BANE:
            case TYPE_ACHIEV_LOVE_SKADI:
            case TYPE_ACHIEV_INCREDIBLE_HULK:
                SetSpecialAchievementCriteria(uiType - TYPE_ACHIEV_INCREDIBLE_HULK, bool(uiData));
                return;
            default:
                script_error_log("Instance Pinnacle: SetData = %u for type %u does not exist/not implemented.", uiType, uiData);
                return;
            }

            // Saving also SPECIAL for this instance
            if (uiData == DONE || uiData == SPECIAL)
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

        void SetData64(uint32 uiType, uint64 uiData) override
        {
            switch (uiType)
            {
            case DATA64_GORTHOK_EVENT_STARTER:
                m_gortokEventStarterGuid = ObjectGuid(uiData);
                break;
            default:
                break;
            }
        }

        uint64 GetData64(uint32 uiType) const override
        {
            switch (uiType)
            {
            case DATA64_GORTHOK_EVENT_STARTER:
                return m_gortokEventStarterGuid.GetRawValue();
            case DATA64_SKADI_MOBS_TRIGGER:
                return m_skadiMobsTriggerGuid.GetRawValue();
            default:
                break;
            }
            return 0;
        }

        bool CheckAchievementCriteriaMeet(uint32 uiCriteriaId, Player const* pSource, Unit const* pTarget, uint32 uiMiscValue1 /* = 0*/) const override
        {
            switch (uiCriteriaId)
            {
            case ACHIEV_CRIT_INCREDIBLE_HULK:
                return m_abAchievCriteria[TYPE_ACHIEV_INCREDIBLE_HULK - TYPE_ACHIEV_INCREDIBLE_HULK];
            case ACHIEV_CRIT_GIRL_LOVES_SKADI:
                return m_abAchievCriteria[TYPE_ACHIEV_LOVE_SKADI - TYPE_ACHIEV_INCREDIBLE_HULK];
            case ACHIEV_CRIT_KINGS_BANE:
                return m_abAchievCriteria[TYPE_ACHIEV_KINGS_BANE - TYPE_ACHIEV_INCREDIBLE_HULK];

            default:
                return false;
            }
        }

        //void SetGortokEventStarter(ObjectGuid playerGuid) { m_gortokEventStarterGuid = playerGuid; }
        //ObjectGuid GetGortokEventStarter() { return m_gortokEventStarterGuid; }
        //ObjectGuid GetSkadiMobsTrigger() { return m_skadiMobsTriggerGuid; }

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

        void Update(uint32 uiDiff) override
        {
            if (m_uiGortokOrbTimer)
            {
                if (m_uiGortokOrbTimer <= uiDiff)
                {
                    if (!m_uiGortokOrbPhase)
                    {
                        if (Creature* pOrb = instance->GetCreature(m_gortokEventTriggerGuid))
                            pOrb->GetMotionMaster()->MovePoint(0, aOrbPositions[1][0], aOrbPositions[1][1], aOrbPositions[1][2]);

                        m_uiGortokOrbTimer = 18000;
                    }
                    // Awaken Gortok if this is the last phase
                    else
                    {
                        uint8 uiMaxOrbPhase = instance->IsRegularDifficulty() ? 3 : 5;
                        uint32 uiSpellId = m_uiGortokOrbPhase == uiMaxOrbPhase ? SPELL_AWAKEN_GORTOK : SPELL_AWAKEN_SUBBOSS;

                        if (Creature* pOrb = instance->GetCreature(m_gortokEventTriggerGuid))
                        {
                            pOrb->CastSpell(pOrb, uiSpellId, false);

                            if (m_uiGortokOrbPhase == uiMaxOrbPhase)
                                pOrb->ForcedDespawn(10000);
                        }

                        m_uiGortokOrbTimer = 0;
                    }
                    ++m_uiGortokOrbPhase;
                }
                else
                    m_uiGortokOrbTimer -= uiDiff;
            }
        }

    private:
        void SetSpecialAchievementCriteria(uint32 uiType, bool bIsMet)
        {
            if (uiType < MAX_SPECIAL_ACHIEV_CRITS)
                m_abAchievCriteria[uiType] = bIsMet;
        }

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        bool m_abAchievCriteria[MAX_SPECIAL_ACHIEV_CRITS];
        std::string m_strInstData;

        uint32 m_uiGortokOrbTimer;
        uint8 m_uiGortokOrbPhase;

        ObjectGuid m_gortokEventTriggerGuid;
        ObjectGuid m_gortokEventStarterGuid;
        ObjectGuid m_skadiMobsTriggerGuid;

        GuidList m_lskadiGauntletMobsList;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_pinnacle(pMap);
    }
};

void AddSC_instance_pinnacle()
{
    Script* s;

    s = new is_pinnacle();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_pinnacle";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_pinnacle;
    //pNewScript->RegisterSelf();
}

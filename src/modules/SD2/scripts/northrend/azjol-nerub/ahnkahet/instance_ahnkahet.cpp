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
SDName: instance_ahnkahet
SD%Complete: 75
SDComment:
SDCategory: Ahn'kahet
EndScriptData */

#include "precompiled.h"
#include "ahnkahet.h"
#include "TemporarySummon.h"

enum
{
    SPELL_JE_SACRIFICE_VISUAL   = 56133,
    SPELL_JE_BEAM_VISUAL        = 56312,
    SPELL_SUMMON_SWARM_GUARDIAN = 56120,
    SPELL_SUMMON_SWARMERS       = 56119,
    SPELL_TA_BEAM_VISUAL        = 60342,        // Visual spell, used before Taldaram is lowered to the ground
};

struct is_ahnkahet : public InstanceScript
{
    is_ahnkahet() : InstanceScript("instance_ahnkahet") {}

    class instance_ahnkahet : public ScriptedInstance
    {
    public:
        instance_ahnkahet(Map* pMap) : ScriptedInstance(pMap),
            m_bRespectElders(false),
            m_bVolunteerWork(false),
            m_uiDevicesActivated(0),
            m_uiInitiatesKilled(0),
            m_uiTwistedVisageCount(0)
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
            case NPC_ELDER_NADOX:
            case NPC_TALDARAM:
            case NPC_JEDOGA_SHADOWSEEKER:
            case NPC_HERALD_VOLAZJ:
                m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
                break;
            case NPC_AHNKAHAR_GUARDIAN_EGG:
                m_GuardianEggList.push_back(pCreature->GetObjectGuid());
                break;
            case NPC_AHNKAHAR_SWARM_EGG:
                m_SwarmerEggList.push_back(pCreature->GetObjectGuid());
                break;
            case NPC_JEDOGA_CONTROLLER:
                // Sort the controllers based on their purpose
                if (pCreature->GetPositionZ() > 30.0f)
                    // Used for Taldaram visual
                    m_lJedogaControllersGuidList.push_back(pCreature->GetObjectGuid());
                else if (pCreature->GetPositionZ() > 20.0f)
                    // Used for Jedoga visual
                    m_lJedogaEventControllersGuidList.push_back(pCreature->GetObjectGuid());
                else if (pCreature->GetPositionZ() < -16.0f)
                    // Used for Jedoga sacrifice
                    m_jedogaSacrificeController = pCreature->GetObjectGuid();
                break;
            case NPC_TWISTED_VISAGE_1:
            case NPC_TWISTED_VISAGE_2:
            case NPC_TWISTED_VISAGE_3:
            case NPC_TWISTED_VISAGE_4:
            case NPC_TWISTED_VISAGE_5:
                ++m_uiTwistedVisageCount;
                break;
            }
        }

        void OnObjectCreate(GameObject* pGo) override
        {
            switch (pGo->GetEntry())
            {
            case GO_DOOR_TALDARAM:
                if (m_auiEncounter[TYPE_TALDARAM] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_VORTEX:
                if (m_auiEncounter[TYPE_TALDARAM] == SPECIAL)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;

            case GO_ANCIENT_DEVICE_L:
            case GO_ANCIENT_DEVICE_R:
                if (m_auiEncounter[TYPE_NADOX] == DONE)
                    pGo->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NO_INTERACT);
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
            case NPC_TWISTED_VISAGE_1:
            case NPC_TWISTED_VISAGE_2:
            case NPC_TWISTED_VISAGE_3:
            case NPC_TWISTED_VISAGE_4:
            case NPC_TWISTED_VISAGE_5:
                --m_uiTwistedVisageCount;

                // When all Twisted Visages were killed or despawned switch back to combat phase
                if (!m_uiTwistedVisageCount)
                {
                    // Clear Insanity
                    if (Creature* pVolazj = GetSingleCreatureFromStorage(NPC_HERALD_VOLAZJ))
                    {
                        pVolazj->CastSpell(pVolazj, SPELL_INSANITY_CLEAR, true);
                        pVolazj->RemoveAllAuras();
                    }

                    // Clear insanity manually for now, because the spell won't hit phased players
                    HandleInsanityClear();

                    SetData(TYPE_VOLAZJ, IN_PROGRESS);
                }

                pCreature->ForcedDespawn();
                break;
            }
        }

        void OnCreatureDeath(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_TWILIGHT_INITIATE:
                ++m_uiInitiatesKilled;

                // If all initiates are killed, then land Jedoga and stop the channeling
                if (m_uiInitiatesKilled == MAX_INITIATES)
                {
                    if (Creature* pJedoga = GetSingleCreatureFromStorage(NPC_JEDOGA_SHADOWSEEKER))
                        pJedoga->GetMotionMaster()->MovePoint(1, aJedogaLandingLoc[0], aJedogaLandingLoc[1], aJedogaLandingLoc[2]);

                    for (GuidList::const_iterator itr = m_lJedogaEventControllersGuidList.begin(); itr != m_lJedogaEventControllersGuidList.end(); ++itr)
                    {
                        if (Creature* pTemp = instance->GetCreature(*itr))
                            pTemp->InterruptNonMeleeSpells(false);
                    }
                }

                break;
            case NPC_TWISTED_VISAGE_1:
            case NPC_TWISTED_VISAGE_2:
            case NPC_TWISTED_VISAGE_3:
            case NPC_TWISTED_VISAGE_4:
            case NPC_TWISTED_VISAGE_5:
                pCreature->CastSpell(pCreature, SPELL_TWISTED_VISAGE_DEATH, true);

                --m_uiTwistedVisageCount;

                // When all Twisted Visages were killed or despawned switch back to combat phase
                if (!m_uiTwistedVisageCount)
                {
                    // Clear Insanity
                    if (Creature* pVolazj = GetSingleCreatureFromStorage(NPC_HERALD_VOLAZJ))
                    {
                        pVolazj->CastSpell(pVolazj, SPELL_INSANITY_CLEAR, true);
                        pVolazj->RemoveAllAuras();
                    }

                    // Clear insanity manually for now, because the spell won't hit phased players
                    HandleInsanityClear();

                    SetData(TYPE_VOLAZJ, IN_PROGRESS);
                }
                else
                {
                    // Switch Insanity
                    if (Creature* pVolazj = GetSingleCreatureFromStorage(NPC_HERALD_VOLAZJ))
                        pVolazj->CastSpell(pVolazj, SPELL_INSANITY_SWITCH, true);

                    // Handle insanity switch manually, because the boss can't hit phased players
                    if (pCreature->IsTemporarySummon())
                    {
                        TemporarySummon* pTemporary = (TemporarySummon*)pCreature;

                        // Switch insanity phase for the master player
                        if (Player* pPlayer = instance->GetPlayer(pTemporary->GetSummonerGuid()))
                            HandleInsanitySwitch(pPlayer);
                    }
                }
                break;
            }
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            debug_log("SD2: Instance Ahn'Kahet: SetData received for type %u with data %u", uiType, uiData);

            switch (uiType)
            {
            case TYPE_NADOX:
                m_auiEncounter[uiType] = uiData;
                if (uiData == IN_PROGRESS)
                    m_bRespectElders = true;
                else if (uiData == SPECIAL)
                    m_bRespectElders = false;
                else if (uiData == DONE)
                {
                    DoToggleGameObjectFlags(GO_ANCIENT_DEVICE_L, GO_FLAG_NO_INTERACT, false);
                    DoToggleGameObjectFlags(GO_ANCIENT_DEVICE_R, GO_FLAG_NO_INTERACT, false);
                }
                break;
            case TYPE_TALDARAM:
                if (uiData == SPECIAL)
                {
                    ++m_uiDevicesActivated;

                    if (m_uiDevicesActivated == 2)
                    {
                        m_auiEncounter[uiType] = uiData;
                        DoUseDoorOrButton(GO_VORTEX);

                        // Lower Taldaram
                        if (Creature* pTaldaram = GetSingleCreatureFromStorage(NPC_TALDARAM))
                            pTaldaram->GetMotionMaster()->MovePoint(1, aTaldaramLandingLoc[0], aTaldaramLandingLoc[1], aTaldaramLandingLoc[2]);

                        // Interrupt the channeling
                        for (GuidList::const_iterator itr = m_lJedogaControllersGuidList.begin(); itr != m_lJedogaControllersGuidList.end(); ++itr)
                        {
                            if (Creature* pTemp = instance->GetCreature(*itr))
                                pTemp->InterruptNonMeleeSpells(false);
                        }
                    }
                }
                else if (uiData == DONE)
                {
                    m_auiEncounter[uiType] = uiData;
                    DoUseDoorOrButton(GO_DOOR_TALDARAM);
                }
                break;
            case TYPE_JEDOGA:
                m_auiEncounter[uiType] = uiData;
                if (uiData == IN_PROGRESS)
                    m_bVolunteerWork = true;
                else if (uiData == SPECIAL)
                    m_bVolunteerWork = false;
                else if (uiData == FAIL)
                    m_uiInitiatesKilled = 0;
                break;
            case TYPE_AMANITAR:
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_VOLAZJ:
                m_auiEncounter[uiType] = uiData;
                if (uiData == IN_PROGRESS)
                {
                    m_uiTwistedVisageCount = 0;
                    m_lInsanityPlayersGuidList.clear();
                }
                break;
            case TYPE_DO_JEDOGA:
                switch (uiData)
                {
                case 0:
                    if (Creature* pTemp = instance->GetCreature(m_jedogaSacrificeController))
                        pTemp->CastSpell(pTemp, SPELL_JE_SACRIFICE_VISUAL, false);
                    break;
                case 1:
                    if (Creature* pTemp = instance->GetCreature(m_jedogaSacrificeController))
                        pTemp->RemoveAurasDueToSpell(SPELL_JE_SACRIFICE_VISUAL);
                    break;
                case 2:
                    for (GuidList::const_iterator itr = m_lJedogaEventControllersGuidList.begin(); itr != m_lJedogaEventControllersGuidList.end(); ++itr)
                    {
                        if (Creature* pTemp = instance->GetCreature(*itr))
                            pTemp->CastSpell(GetSingleCreatureFromStorage(NPC_JEDOGA_SHADOWSEEKER), SPELL_JE_BEAM_VISUAL, false);
                    }
                    break;
                default:
                    break;
                }
                return;
            case TYPE_DO_NADOX:
                switch (uiData)
                {
                case 0:
                    if (Creature* pGuardianEgg = instance->GetCreature(SelectRandomGuardianEggGuid()))
                        pGuardianEgg->CastSpell(pGuardianEgg, SPELL_SUMMON_SWARM_GUARDIAN, false);
                    break;
                case 1:
                    // There are 2 Swarmers summoned at a timer; @TODO check to do not use single mob twice
                    if (Creature* pSwarmerEgg = instance->GetCreature(SelectRandomSwarmerEggGuid()))
                    {
                        for (uint8 i = 0; i < 2; ++i)
                            pSwarmerEgg->CastSpell(pSwarmerEgg, SPELL_SUMMON_SWARMERS, false);
                    }
                    break;
                default:
                    break;
                }
                return;
            case TYPE_DO_TALDARAM:
                for (GuidList::const_iterator itr = m_lJedogaControllersGuidList.begin(); itr != m_lJedogaControllersGuidList.end(); ++itr)
                {
                    if (Creature* pTemp = instance->GetCreature(*itr))
                        pTemp->CastSpell(GetSingleCreatureFromStorage(NPC_TALDARAM), SPELL_TA_BEAM_VISUAL, false);
                }
                return;

            default:
                script_error_log("Instance Ahn'Kahet: ERROR SetData = %u for type %u does not exist/not implemented.", uiType, uiData);
                break;
            }

            // For some encounters Special data needs to be saved
            if (uiData == DONE || (uiData == SPECIAL && uiType == TYPE_TALDARAM))
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << m_auiEncounter[0] << " " << m_auiEncounter[1] << " " << m_auiEncounter[2] << " " << m_auiEncounter[3]
                    << " " << m_auiEncounter[4];

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

        void SetData64(uint32 uiType, uint64 uiGuid) override
        {
            // Store all the players hit by the insanity spell in order to use them for the phasing switch / clear
            if (uiType == DATA_INSANITY_PLAYER)
            {
                if (Player* pPlayer = instance->GetPlayer(ObjectGuid(uiGuid)))
                    m_lInsanityPlayersGuidList.push_back(pPlayer->GetObjectGuid());
            }
        }

        bool CheckAchievementCriteriaMeet(uint32 uiCriteriaId, Player const* pSource, Unit const* pTarget, uint32 uiMiscValue1 /* = 0*/) const override
        {
            switch (uiCriteriaId)
            {
            case ACHIEV_CRIT_RESPECT_ELDERS:
                return m_bRespectElders;
            case ACHIEV_CRIT_VOLUNTEER_WORK:
                return m_bVolunteerWork;

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
            loadStream >> m_auiEncounter[0] >> m_auiEncounter[1] >> m_auiEncounter[2] >> m_auiEncounter[3] >> m_auiEncounter[4];

            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            {
                if (m_auiEncounter[i] == IN_PROGRESS)
                    m_auiEncounter[i] = NOT_STARTED;
            }

            OUT_LOAD_INST_DATA_COMPLETE;
        }

    private:
        void HandleInsanityClear()
        {
            for (GuidList::const_iterator itr = m_lInsanityPlayersGuidList.begin(); itr != m_lInsanityPlayersGuidList.end(); ++itr)
            {
                if (Player* pPlayer = instance->GetPlayer(*itr))
                    pPlayer->RemoveSpellsCausingAura(SPELL_AURA_PHASE);
            }
        }

        void HandleInsanitySwitch(Player* pPlayer)
        {
            // Get the phase aura id
            std::list<Aura*> lAuraList = pPlayer->GetAurasByType(SPELL_AURA_PHASE);
            if (lAuraList.empty())
                return;

            uint32 uiPhaseAura = (*lAuraList.begin())->GetId();

            std::list<Player*> lSamePhasePlayers;
            std::vector<Player*> vOtherPhasePlayers;

            // Sort the insanity players, into those which have same phase and others
            for (GuidList::const_iterator itr = m_lInsanityPlayersGuidList.begin(); itr != m_lInsanityPlayersGuidList.end(); ++itr)
            {
                if (Player* pTemp = instance->GetPlayer(*itr))
                {
                    if (pTemp->HasAura(uiPhaseAura))
                        lSamePhasePlayers.push_back(pTemp);
                    // Check only for alive players
                    else if (pTemp->IsAlive())
                        vOtherPhasePlayers.push_back(pTemp);
                }
            }

            // This shouldn't happen
            if (vOtherPhasePlayers.empty())
                return;

            // Get the phase aura of the new selected player
            Player* pNewPlayer = vOtherPhasePlayers[urand(0, vOtherPhasePlayers.size() - 1)];

            // Get the phase aura id
            std::list<Aura*> lNewAuraList = pNewPlayer->GetAurasByType(SPELL_AURA_PHASE);
            if (lNewAuraList.empty())
                return;

            uint32 uiNewPhaseAura = (*lNewAuraList.begin())->GetId();

            // Move the same phase players to the new phase
            for (std::list<Player*>::const_iterator itr = lSamePhasePlayers.begin(); itr != lSamePhasePlayers.end(); ++itr)
                (*itr)->CastSpell((*itr), uiNewPhaseAura, true);
        }

        ObjectGuid SelectRandomGuardianEggGuid()
        {
            if (m_GuardianEggList.empty())
                return ObjectGuid();

            GuidList::iterator iter = m_GuardianEggList.begin();
            advance(iter, urand(0, m_GuardianEggList.size() - 1));

            return *iter;
        }

        ObjectGuid SelectRandomSwarmerEggGuid()
        {
            if (m_SwarmerEggList.empty())
                return ObjectGuid();

            GuidList::iterator iter = m_SwarmerEggList.begin();
            advance(iter, urand(0, m_SwarmerEggList.size() - 1));

            return *iter;
        }

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strInstData;

        bool m_bRespectElders;
        bool m_bVolunteerWork;

        uint8 m_uiDevicesActivated;
        uint8 m_uiInitiatesKilled;
        uint8 m_uiTwistedVisageCount;

        ObjectGuid m_jedogaSacrificeController;

        GuidList m_GuardianEggList;
        GuidList m_SwarmerEggList;
        GuidList m_lJedogaControllersGuidList;
        GuidList m_lJedogaEventControllersGuidList;
        GuidList m_lInsanityPlayersGuidList;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_ahnkahet(pMap);
    }
};

void AddSC_instance_ahnkahet()
{
    Script* s;

    s = new is_ahnkahet();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_ahnkahet";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_ahnkahet;
    //pNewScript->RegisterSelf();
}

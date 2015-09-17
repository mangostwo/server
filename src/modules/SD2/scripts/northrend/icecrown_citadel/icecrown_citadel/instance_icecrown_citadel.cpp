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
SDName: instance_icecrown_citadel
SD%Complete: 20%
SDComment: Just basic stuff
SDCategory: Icecrown Citadel
EndScriptData */

#include "precompiled.h"
#include "icecrown_citadel.h"

enum
{
    // Marrowgar
    SAY_MARROWGAR_INTRO             = -1631001,

    // Deathwhisper
    SAY_DEATHWHISPER_SPEECH_1       = -1631011,
    SAY_DEATHWHISPER_SPEECH_2       = -1631012,
    SAY_DEATHWHISPER_SPEECH_3       = -1631013,
    SAY_DEATHWHISPER_SPEECH_4       = -1631014,
    SAY_DEATHWHISPER_SPEECH_5       = -1631015,
    SAY_DEATHWHISPER_SPEECH_6       = -1631016,
    SAY_DEATHWHISPER_SPEECH_7       = -1631017,

    // Festergut
    SAY_STINKY_DIES                 = -1631081,
    // Rotface
    SAY_PRECIOUS_DIES               = -1631070,
};

static const DialogueEntry aCitadelDialogue[] =
{
    {SAY_DEATHWHISPER_SPEECH_1,  NPC_LADY_DEATHWHISPER,  12000},
    {SAY_DEATHWHISPER_SPEECH_2,  NPC_LADY_DEATHWHISPER,  11000},
    {SAY_DEATHWHISPER_SPEECH_3,  NPC_LADY_DEATHWHISPER,  10000},
    {SAY_DEATHWHISPER_SPEECH_4,  NPC_LADY_DEATHWHISPER,  9000},
    {SAY_DEATHWHISPER_SPEECH_5,  NPC_LADY_DEATHWHISPER,  10000},
    {SAY_DEATHWHISPER_SPEECH_6,  NPC_LADY_DEATHWHISPER,  10000},
    {SAY_DEATHWHISPER_SPEECH_7,  NPC_LADY_DEATHWHISPER,  0},
    {0, 0, 0},
};

struct sortFromNorthToSouth
{
    sortFromNorthToSouth(Map *pMap) : _map(pMap) {}

    bool operator()(ObjectGuid uiLeft, ObjectGuid uiRight)
    {
        Unit *pFirst = _map->GetUnit(uiLeft);
        Unit *pSecond = _map->GetUnit(uiRight);
        return pFirst && pSecond && pFirst->GetPositionX() < pSecond->GetPositionX();
    }
private:
    Map *_map;
};

static const uint32 aLeftSummonedCultists[3] = { NPC_CULT_ADHERENT, NPC_CULT_FANATIC, NPC_CULT_ADHERENT };
static const uint32 aRightSummonedCultists[3] = { NPC_CULT_FANATIC, NPC_CULT_ADHERENT, NPC_CULT_FANATIC };

struct is_icecrown_citadel : public InstanceScript
{
    is_icecrown_citadel() : InstanceScript("instance_icecrown_citadel") {}

    class instance_icecrown_citadel : public ScriptedInstance, private DialogueHelper
    {
    public:
        instance_icecrown_citadel(Map* pMap) : ScriptedInstance(pMap), DialogueHelper(aCitadelDialogue),
            m_uiTeam(0),
            m_uiPutricideValveTimer(0),
            m_bHasMarrowgarIntroYelled(false),
            m_bHasDeathwhisperIntroYelled(false),
            m_bHasRimefangLanded(false),
            m_bHasSpinestalkerLanded(false),
            m_uiAreaTriggerId(0)
        {
            m_bIsLeftSideSummon = roll_chance_i(50);
            Initialize();
        }

        void Initialize() override
        {
            InitializeDialogueHelper(this);
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
        }

        bool IsEncounterInProgress() const override
        {
            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            {
                if (m_auiEncounter[i] == IN_PROGRESS)
                    return true;
            }

            return false;
        }

        void OnPlayerEnter(Player* pPlayer) override
        {
            if (!m_uiTeam)                                          // very first player to enter
                m_uiTeam = pPlayer->GetTeam();
        }

        void OnCreatureCreate(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_LORD_MARROWGAR:
            case NPC_LADY_DEATHWHISPER:
            case NPC_DEATHBRINGER_SAURFANG:
            case NPC_FESTERGUT:
            case NPC_ROTFACE:
            case NPC_PROFESSOR_PUTRICIDE:
            case NPC_TALDARAM:
            case NPC_VALANAR:
            case NPC_KELESETH:
            case NPC_LANATHEL_INTRO:
            case NPC_VALITHRIA:
            case NPC_SINDRAGOSA:
            case NPC_LICH_KING:
            case NPC_TIRION:
            case NPC_RIMEFANG:
            case NPC_SPINESTALKER:
            case NPC_VALITHRIA_COMBAT_TRIGGER:
            case NPC_BLOOD_ORB_CONTROL:
                m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
                break;
            case NPC_DEATHWHISPER_SPAWN_STALKER:
                if (pCreature->GetPositionZ() > 60.0f)
                    m_middleStalkerGuid = pCreature->GetObjectGuid();
                else if (pCreature->GetPositionY() < 2215.0f)
                    m_lRightStalkers.push_back(pCreature->GetObjectGuid());
                else
                    m_lLeftStalkers.push_back(pCreature->GetObjectGuid());
                return;
            }
        }

        void OnObjectCreate(GameObject* pGo) override
        {
            switch (pGo->GetEntry())
            {
            case GO_ICEWALL_1:
            case GO_ICEWALL_2:
            case GO_ORATORY_DOOR:
                if (m_auiEncounter[TYPE_MARROWGAR] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_DEATHWHISPER_ELEVATOR:
                // ToDo: set in motion when TYPE_LADY_DEATHWHISPER == DONE
                break;
            case GO_SAURFANG_DOOR:
            case GO_SCIENTIST_DOOR:
            case GO_CRIMSON_HALL_DOOR:
            case GO_GREEN_DRAGON_ENTRANCE:
                if (m_auiEncounter[TYPE_DEATHBRINGER_SAURFANG] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_ORANGE_TUBE:
                if (m_auiEncounter[TYPE_FESTERGUT] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_GREEN_TUBE:
                if (m_auiEncounter[TYPE_ROTFACE] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_SCIENTIST_DOOR_GREEN:
                // If both Festergut and Rotface are DONE, set as ACTIVE_ALTERNATIVE
                if (m_auiEncounter[TYPE_FESTERGUT] == DONE && m_auiEncounter[TYPE_ROTFACE] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);
                else if (m_auiEncounter[TYPE_ROTFACE] == DONE)
                    pGo->SetGoState(GO_STATE_READY);
                break;
            case GO_SCIENTIST_DOOR_ORANGE:
                // If both Festergut and Rotface are DONE, set as ACTIVE_ALTERNATIVE
                if (m_auiEncounter[TYPE_FESTERGUT] == DONE && m_auiEncounter[TYPE_ROTFACE] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);
                else if (m_auiEncounter[TYPE_FESTERGUT] == DONE)
                    pGo->SetGoState(GO_STATE_READY);
                break;
            case GO_SCIENTIST_DOOR_COLLISION:
                if (m_auiEncounter[TYPE_FESTERGUT] == DONE && m_auiEncounter[TYPE_ROTFACE] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_COUNCIL_DOOR_1:
            case GO_COUNCIL_DOOR_2:
                if (m_auiEncounter[TYPE_BLOOD_PRINCE_COUNCIL] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_GREEN_DRAGON_EXIT:
                if (m_auiEncounter[TYPE_VALITHRIA] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_SAURFANG_CACHE:
            case GO_SAURFANG_CACHE_25:
            case GO_SAURFANG_CACHE_10_H:
            case GO_SAURFANG_CACHE_25_H:
                m_mGoEntryGuidStore[GO_SAURFANG_CACHE] = pGo->GetObjectGuid();
                return;
            case GO_GUNSHIP_ARMORY_A:
            case GO_GUNSHIP_ARMORY_A_25:
            case GO_GUNSHIP_ARMORY_A_10H:
            case GO_GUNSHIP_ARMORY_A_25H:
                m_mGoEntryGuidStore[GO_GUNSHIP_ARMORY_A] = pGo->GetObjectGuid();
                return;
            case GO_GUNSHIP_ARMORY_H:
            case GO_GUNSHIP_ARMORY_H_25:
            case GO_GUNSHIP_ARMORY_H_10H:
            case GO_GUNSHIP_ARMORY_H_25H:
                m_mGoEntryGuidStore[GO_GUNSHIP_ARMORY_H] = pGo->GetObjectGuid();
                return;
            case GO_DREAMWALKER_CACHE:
            case GO_DREAMWALKER_CACHE_25:
            case GO_DREAMWALKER_CACHE_10_H:
            case GO_DREAMWALKER_CACHE_25_H:
                m_mGoEntryGuidStore[GO_DREAMWALKER_CACHE] = pGo->GetObjectGuid();
                return;
            case GO_ICESHARD_1:
            case GO_ICESHARD_2:
            case GO_ICESHARD_3:
            case GO_ICESHARD_4:
            case GO_FROSTY_WIND:
            case GO_FROSTY_EDGE:
            case GO_SNOW_EDGE:
            case GO_ARTHAS_PLATFORM:
            case GO_ARTHAS_PRECIPICE:
            case GO_MARROWGAR_DOOR:
            case GO_BLOODPRINCE_DOOR:
            case GO_SINDRAGOSA_ENTRANCE:
            case GO_VALITHRIA_DOOR_1:
            case GO_VALITHRIA_DOOR_2:
            case GO_VALITHRIA_DOOR_3:
            case GO_VALITHRIA_DOOR_4:
            case GO_ICECROWN_GRATE:
            case GO_SINDRAGOSA_SHORTCUT_ENTRANCE:
            case GO_SINDRAGOSA_SHORTCUT_EXIT:
            case GO_ORANGE_PLAGUE:
            case GO_GREEN_PLAGUE:
            case GO_ORANGE_VALVE:
            case GO_GREEN_VALVE:
                break;
            }
            m_mGoEntryGuidStore[pGo->GetEntry()] = pGo->GetObjectGuid();
        }

        void OnCreatureDeath(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_STINKY:
                if (Creature* pFestergut = GetSingleCreatureFromStorage(NPC_FESTERGUT))
                {
                    if (pFestergut->IsAlive())
                        DoScriptText(SAY_STINKY_DIES, pFestergut);
                }
                break;
            case NPC_PRECIOUS:
                if (Creature* pRotface = GetSingleCreatureFromStorage(NPC_ROTFACE))
                {
                    if (pRotface->IsAlive())
                        DoScriptText(SAY_PRECIOUS_DIES, pRotface);
                }
                break;
            }
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            switch (uiType)
            {
            case TYPE_MARROWGAR:
                m_auiEncounter[uiType] = uiData;
                DoUseDoorOrButton(GO_MARROWGAR_DOOR);
                if (uiData == DONE)
                {
                    DoUseDoorOrButton(GO_ICEWALL_1);
                    DoUseDoorOrButton(GO_ICEWALL_2);

                    // Note: this door use may not be correct. In theory the door should be already opened
                    DoUseDoorOrButton(GO_ORATORY_DOOR);
                }
                break;
            case TYPE_LADY_DEATHWHISPER:
                m_auiEncounter[uiType] = uiData;
                DoUseDoorOrButton(GO_ORATORY_DOOR);
                if (uiData == IN_PROGRESS)
                {
                    m_lRightStalkers.sort(sortFromNorthToSouth(instance));
                    m_lLeftStalkers.sort(sortFromNorthToSouth(instance));
                }
                // ToDo: set the elevateor in motion when TYPE_LADY_DEATHWHISPER == DONE
                break;
            case TYPE_GUNSHIP_BATTLE:
                m_auiEncounter[uiType] = uiData;
                if (uiData == DONE)
                    DoRespawnGameObject(m_uiTeam == ALLIANCE ? GO_GUNSHIP_ARMORY_A : GO_GUNSHIP_ARMORY_H, 60 * MINUTE);
                break;
            case TYPE_DEATHBRINGER_SAURFANG:
                m_auiEncounter[uiType] = uiData;
                if (uiData == DONE)
                {
                    DoUseDoorOrButton(GO_SAURFANG_DOOR);
                    DoRespawnGameObject(GO_SAURFANG_CACHE, 60 * MINUTE);

                    // Note: these doors may not be correct. In theory the doors should be already opened
                    DoUseDoorOrButton(GO_SCIENTIST_DOOR);
                    DoUseDoorOrButton(GO_CRIMSON_HALL_DOOR);
                    DoUseDoorOrButton(GO_GREEN_DRAGON_ENTRANCE);
                }
                break;
            case TYPE_FESTERGUT:
                m_auiEncounter[uiType] = uiData;
                DoUseDoorOrButton(GO_ORANGE_PLAGUE);
                if (uiData == DONE)
                    DoToggleGameObjectFlags(GO_ORANGE_VALVE, GO_FLAG_NO_INTERACT, false);
                break;
            case TYPE_ROTFACE:
                m_auiEncounter[uiType] = uiData;
                DoUseDoorOrButton(GO_GREEN_PLAGUE);
                if (uiData == DONE)
                    DoToggleGameObjectFlags(GO_GREEN_VALVE, GO_FLAG_NO_INTERACT, false);
                break;
            case TYPE_PROFESSOR_PUTRICIDE:
                m_auiEncounter[uiType] = uiData;
                DoUseDoorOrButton(GO_SCIENTIST_DOOR);
                break;
            case TYPE_BLOOD_PRINCE_COUNCIL:
                m_auiEncounter[uiType] = uiData;
                DoUseDoorOrButton(GO_CRIMSON_HALL_DOOR);
                if (uiData == DONE)
                {
                    DoUseDoorOrButton(GO_COUNCIL_DOOR_1);
                    DoUseDoorOrButton(GO_COUNCIL_DOOR_2);
                }
                break;
            case TYPE_QUEEN_LANATHEL:
                m_auiEncounter[uiType] = uiData;
                DoUseDoorOrButton(GO_BLOODPRINCE_DOOR);
                if (uiData == DONE)
                    DoUseDoorOrButton(GO_ICECROWN_GRATE);
                break;
            case TYPE_VALITHRIA:
                m_auiEncounter[uiType] = uiData;
                DoUseDoorOrButton(GO_GREEN_DRAGON_ENTRANCE);
                // Side doors
                DoUseDoorOrButton(GO_VALITHRIA_DOOR_1);
                DoUseDoorOrButton(GO_VALITHRIA_DOOR_2);
                // Some doors are used only in 25 man mode
                if (instance->GetDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL || instance->GetDifficulty() == RAID_DIFFICULTY_25MAN_HEROIC)
                {
                    DoUseDoorOrButton(GO_VALITHRIA_DOOR_3);
                    DoUseDoorOrButton(GO_VALITHRIA_DOOR_4);
                }
                if (uiData == DONE)
                {
                    DoUseDoorOrButton(GO_GREEN_DRAGON_EXIT);
                    DoUseDoorOrButton(GO_SINDRAGOSA_ENTRANCE);
                    DoRespawnGameObject(GO_DREAMWALKER_CACHE, 60 * MINUTE);
                }
                break;
            case TYPE_SINDRAGOSA:
                m_auiEncounter[uiType] = uiData;
                DoUseDoorOrButton(GO_SINDRAGOSA_ENTRANCE);
                break;
            case TYPE_LICH_KING:
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_DATA_AREATRIGGERS:
                m_uiAreaTriggerId = uiData;
                return;
            case TYPE_DO_PREPARE_PROF_DOOR:
                DoPreparePutricideDoor();
                return;
            case TYPE_DO_SUMMON_CULTIST_WAVE:
                DoSummonCultistWave(bool(uiData));
                return;
            default:
                script_error_log("Instance Icecrown Citadel: ERROR SetData = %u for type %u does not exist/not implemented.", uiType, uiData);
                return;
            }

            if (uiData == DONE)
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;

                saveStream << m_auiEncounter[0] << " " << m_auiEncounter[1] << " " << m_auiEncounter[2] << " "
                    << m_auiEncounter[3] << " " << m_auiEncounter[4] << " " << m_auiEncounter[5] << " "
                    << m_auiEncounter[6] << " " << m_auiEncounter[7] << " " << m_auiEncounter[8] << " "
                    << m_auiEncounter[9] << " " << m_auiEncounter[10] << " " << m_auiEncounter[11];

                m_strInstData = saveStream.str();

                SaveToDB();
                OUT_SAVE_INST_DATA_COMPLETE;
            }
        }

        uint32 GetData(uint32 uiType) const override
        {
            if (uiType < MAX_ENCOUNTER)
                return m_auiEncounter[uiType];

            // Difficulty wrappers
            switch (uiType)
            {
            case TYPE_DATA_IS_HEROIC:
                return uint32(instance->GetDifficulty() == RAID_DIFFICULTY_10MAN_HEROIC || instance->GetDifficulty() == RAID_DIFFICULTY_25MAN_HEROIC);
            case TYPE_DATA_IS_25MAN:
                return uint32(instance->GetDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL || instance->GetDifficulty() == RAID_DIFFICULTY_25MAN_HEROIC);
            default:
                break;
            }
            return 0;
        }

        void SetData64(uint32 uiType, uint64 uiData) override
        {
            switch (uiType)
            {
            case TYPE_DATA_AREATRIGGERS:
                DoHandleCitadelAreaTrigger(instance->GetPlayer(ObjectGuid(uiData)));
                break;
            default:
                break;
            }
        }

        const char* Save() const override { return m_strInstData.c_str(); }
        void Load(const char* strIn) override
        {
            if (!strIn)
            {
                OUT_LOAD_INST_DATA_FAIL;
                return;
            }

            OUT_LOAD_INST_DATA(strIn);

            std::istringstream loadStream(strIn);
            loadStream >> m_auiEncounter[0] >> m_auiEncounter[1] >> m_auiEncounter[2] >> m_auiEncounter[3]
                >> m_auiEncounter[4] >> m_auiEncounter[5] >> m_auiEncounter[6] >> m_auiEncounter[7] >> m_auiEncounter[8]
                >> m_auiEncounter[9] >> m_auiEncounter[10] >> m_auiEncounter[11];

            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            {
                if (m_auiEncounter[i] == IN_PROGRESS)
                    m_auiEncounter[i] = NOT_STARTED;
            }

            OUT_LOAD_INST_DATA_COMPLETE;
        }

        bool CheckAchievementCriteriaMeet(uint32 uiCriteriaId, Player const* pSource, Unit const* pTarget = NULL, uint32 uiMiscvalue1 = 0) const override
        {
            // @ToDo:
            return false;
        }

        void Update(uint32 uiDiff) override
        {
            DialogueUpdate(uiDiff);

            if (m_uiPutricideValveTimer)
            {
                if (m_uiPutricideValveTimer <= uiDiff)
                {
                    // Open the pathway to Putricide when the timer expires
                    DoUseDoorOrButton(GO_SCIENTIST_DOOR_COLLISION);
                    if (GameObject* pDoor = GetSingleGameObjectFromStorage(GO_SCIENTIST_DOOR_GREEN))
                        pDoor->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);
                    if (GameObject* pDoor = GetSingleGameObjectFromStorage(GO_SCIENTIST_DOOR_ORANGE))
                        pDoor->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);

                    m_uiPutricideValveTimer = 0;
                }
                else
                    m_uiPutricideValveTimer -= uiDiff;
            }
        }

    private:
        // Wrapper to handle the adds summmoning by Lady Deathwhisper
        void DoSummonCultistWave(bool isPhaseOne)
        {
            Creature* lady = instance->GetCreature(ObjectGuid(m_mNpcEntryGuidStore[NPC_LADY_DEATHWHISPER]));
            if (!lady || lady->IsDead())
                return;

            // On 25 man mode we need to summon on all points
            if (GetData(TYPE_DATA_IS_25MAN))
            {
                GuidList::const_iterator rcit = m_lRightStalkers.begin();
                GuidList::const_iterator lcit = m_lLeftStalkers.begin();
                for (uint8 i = 0; i < 3; ++i, ++rcit, ++lcit)
                {
                    if (Creature* pStalker = instance->GetCreature(*lcit))
                        lady->SummonCreature(aLeftSummonedCultists[i], pStalker->GetPositionX(), pStalker->GetPositionY(), pStalker->GetPositionZ(), 0, TEMPSUMMON_DEAD_DESPAWN, 0);
                    if (Creature* pStalker = instance->GetCreature(*rcit))
                        lady->SummonCreature(aRightSummonedCultists[i], pStalker->GetPositionX(), pStalker->GetPositionY(), pStalker->GetPositionZ(), 0, TEMPSUMMON_DEAD_DESPAWN, 0);
                }

                if (Creature* pStalker = instance->GetCreature(m_middleStalkerGuid))
                    lady->SummonCreature(roll_chance_i(50) ? NPC_CULT_FANATIC : NPC_CULT_ADHERENT, pStalker->GetPositionX(), pStalker->GetPositionY(), pStalker->GetPositionZ(), 0.0f, TEMPSUMMON_CORPSE_DESPAWN, 0);
            }
            // On 10 man mode we summon on the left or on the right
            else
            {
                // Summon just 1 add in phase 2 heroic
                if (GetData(TYPE_DATA_IS_HEROIC) && !isPhaseOne)
                {
                    if (Creature* pStalker = instance->GetCreature(m_middleStalkerGuid))
                        lady->SummonCreature(roll_chance_i(50) ? NPC_CULT_FANATIC : NPC_CULT_ADHERENT, pStalker->GetPositionX(), pStalker->GetPositionY(), pStalker->GetPositionZ(), 0.0f, TEMPSUMMON_CORPSE_DESPAWN, 0);
                }
                else
                {
                    GuidList::const_iterator cit = m_bIsLeftSideSummon ? m_lLeftStalkers.begin() : m_lRightStalkers.begin();
                    for (uint8 i = 0; i < 3; ++i, ++cit)
                    {
                        if (Creature* pStalker = instance->GetCreature(*cit))
                            lady->SummonCreature(m_bIsLeftSideSummon ? aLeftSummonedCultists[i] : aRightSummonedCultists[i], pStalker->GetPositionX(), pStalker->GetPositionY(), pStalker->GetPositionZ(), 0, TEMPSUMMON_DEAD_DESPAWN, 0);
                    }

                    // change sides for next summoning
                    m_bIsLeftSideSummon = !m_bIsLeftSideSummon;
                }
            }
        }

        // Open Putricide door in a few seconds
        void DoPreparePutricideDoor() { m_uiPutricideValveTimer = 15000; }

        void DoHandleCitadelAreaTrigger(Player* pPlayer)
        {
            if (m_uiAreaTriggerId == AREATRIGGER_MARROWGAR_INTRO && !m_bHasMarrowgarIntroYelled)
            {
                if (Creature* pMarrowgar = GetSingleCreatureFromStorage(NPC_LORD_MARROWGAR))
                {
                    DoScriptText(SAY_MARROWGAR_INTRO, pMarrowgar);
                    m_bHasMarrowgarIntroYelled = true;
                }
            }
            else if (m_uiAreaTriggerId == AREATRIGGER_DEATHWHISPER_INTRO && !m_bHasDeathwhisperIntroYelled)
            {
                StartNextDialogueText(SAY_DEATHWHISPER_SPEECH_1);
                m_bHasDeathwhisperIntroYelled = true;
            }
            else if (m_uiAreaTriggerId == AREATRIGGER_SINDRAGOSA_PLATFORM)
            {
                if (Creature* pSindragosa = GetSingleCreatureFromStorage(NPC_SINDRAGOSA))
                {
                    if (pSindragosa->IsAlive() && !pSindragosa->IsInCombat())
                        pSindragosa->SetInCombatWithZone();
                }
                else
                {
                    if (!m_bHasRimefangLanded)
                    {
                        if (Creature* pRimefang = GetSingleCreatureFromStorage(NPC_RIMEFANG))
                        {
                            pRimefang->AI()->AttackStart(pPlayer);
                            m_bHasRimefangLanded = true;
                        }
                    }

                    if (!m_bHasSpinestalkerLanded)
                    {
                        if (Creature* pSpinestalker = GetSingleCreatureFromStorage(NPC_SPINESTALKER))
                        {
                            pSpinestalker->AI()->AttackStart(pPlayer);
                            m_bHasSpinestalkerLanded = true;
                        }
                    }
                }
            }
        }

        std::string m_strInstData;
        uint32 m_auiEncounter[MAX_ENCOUNTER];

        uint32 m_uiTeam;                                    // Team of first entered player, used on the Gunship event
        uint32 m_uiPutricideValveTimer;
        uint32 m_uiAreaTriggerId;

        bool m_bHasMarrowgarIntroYelled;
        bool m_bHasDeathwhisperIntroYelled;
        bool m_bHasRimefangLanded;
        bool m_bHasSpinestalkerLanded;
        bool m_bIsLeftSideSummon;

        GuidList m_lRightStalkers;
        GuidList m_lLeftStalkers;
        ObjectGuid m_middleStalkerGuid;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_icecrown_citadel(pMap);
    }
};

struct at_icecrown_citadel : public AreaTriggerScript
{
    at_icecrown_citadel() : AreaTriggerScript("at_icecrown_citadel") {}

    bool OnTrigger(Player* pPlayer, AreaTriggerEntry const* pAt) override
    {
        if (pAt->id == AREATRIGGER_MARROWGAR_INTRO || pAt->id == AREATRIGGER_DEATHWHISPER_INTRO ||
            pAt->id == AREATRIGGER_SINDRAGOSA_PLATFORM)
        {
            if (pPlayer->isGameMaster() || pPlayer->IsDead())
                return false;

            if (InstanceData* pInstance = pPlayer->GetInstanceData())
            {
                pInstance->SetData(TYPE_DATA_AREATRIGGERS, pAt->id);
                pInstance->SetData64(TYPE_DATA_AREATRIGGERS, pPlayer->GetObjectGuid().GetRawValue());
            }
        }

        return true;
    }
};

struct event_gameobject_citadel_valve : public MapEventScript
{
    event_gameobject_citadel_valve() : MapEventScript("event_gameobject_citadel_valve") {}

    bool OnReceived(uint32 /*uiEventId*/, Object* pSource, Object* /*pTarget*/, bool bIsStart) override
    {
        if (bIsStart && pSource->GetTypeId() == TYPEID_PLAYER)
        {
            if (InstanceData* pInstance = ((Player*)pSource)->GetInstanceData())
            {
                // Note: the Tubes and doors are activated by DB script
                if (pInstance->GetData(TYPE_FESTERGUT) == DONE && pInstance->GetData(TYPE_ROTFACE) == DONE)
                    pInstance->SetData(TYPE_DO_PREPARE_PROF_DOOR, 0);

                return false;
            }
        }
        return false;
    }
};

void AddSC_instance_icecrown_citadel()
{
    Script* s;

    s = new is_icecrown_citadel();
    s->RegisterSelf();

    s = new at_icecrown_citadel();
    s->RegisterSelf();

    s = new event_gameobject_citadel_valve();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_icecrown_citadel";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_icecrown_citadel;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "at_icecrown_citadel";
    //pNewScript->pAreaTrigger = &AreaTrigger_at_icecrown_citadel;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "event_gameobject_citadel_valve";
    //pNewScript->pProcessEventId = &ProcessEventId_event_gameobject_citadel_valve;
    //pNewScript->RegisterSelf();
}

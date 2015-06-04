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
SDName: instance_trial_of_the_crusader
SD%Complete: 100
SDComment:
SDCategory: Crusader Coliseum
EndScriptData */

#include "precompiled.h"
#include "trial_of_the_crusader.h"

/* Trial Of The Crusader encounters:
0 - Wipe Count
1 - Northrend Beasts
2 - Jaraxxus
3 - Faction Champions
4 - Twin Valkyr
5 - Anubarak
*/

enum
{
    SAY_TIRION_RAID_INTRO_LONG          = -1649000,
    SAY_RAID_TRIALS_INTRO               = -1649001,

    // Northrend Beasts
    SAY_TIRION_BEAST_1                  = -1649002,
    SAY_VARIAN_BEAST_1                  = -1649003,
    SAY_GARROSH_BEAST_1                 = -1649004,
    SAY_TIRION_BEAST_SLAY               = -1649007,
    SAY_TIRION_BEAST_WIPE               = -1649008,

    // Jaraxxus Encounter
    SAY_TIRION_JARAXXUS_INTRO_1         = -1649009,
    SAY_WILFRED_JARAXXUS_INTRO_1        = -1649010,
    SAY_WILFRED_JARAXXUS_INTRO_2        = -1649011,
    SAY_WILFRED_JARAXXUS_INTRO_3        = -1649012,
    SAY_JARAXXUS_JARAXXAS_INTRO_1       = -1649013,
    SAY_WILFRED_DEATH                   = -1649014,
    SAY_TIRION_JARAXXUS_INTRO_2         = -1649015,
    SAY_JARAXXUS_DEATH                  = -1649043,
    SAY_TIRION_JARAXXUS_EXIT_1          = -1649016,
    SAY_GARROSH_JARAXXUS_EXIT_1         = -1649017,
    SAY_VARIAN_JARAXXUS_SLAY            = -1649018,
    SAY_TIRION_JARAXXUS_EXIT_2          = -1649019,

    // Faction-Champions
    SAY_TIRION_PVP_INTRO_1              = -1649020,
    SAY_GARROSH_PVP_A_INTRO_1           = -1649021,
    SAY_VARIAN_PVP_H_INTRO_1            = -1649022,
    SAY_TIRION_PVP_INTRO_2              = -1649023,
    SAY_VARIAN_PVP_H_INTRO_2            = -1649024,
    SAY_GARROSH_PVP_A_INTRO_2           = -1649025,
    SAY_VARIAN_PVP_A_WIN                = -1649026,
    SAY_GARROSH_PVP_H_WIN               = -1649027,
    SAY_TIRION_PVP_WIN                  = -1649028,

    // Twin Valkyrs
    SAY_TIRION_TWINS_INTRO              = -1649029,
    SAY_RAID_INTRO_SHORT                = -1649030,
    SAY_VARIAN_TWINS_A_WIN              = -1649031,
    SAY_GARROSH_TWINS_H_WIN             = -1649032,
    SAY_TIRION_TWINS_WIN                = -1649033,

    // Anub'Arak Encounter
    SAY_LKING_ANUB_INTRO_1              = -1649034,
    SAY_TIRION_ABUN_INTRO_1             = -1649035,
    SAY_LKING_ANUB_INTRO_2              = -1649036,
    SAY_LKING_ANUB_INTRO_3              = -1649037,

    // Epilogue
    SAY_TIRION_EPILOGUE                 = -1649075,
};

static const DialogueEntryTwoSide aTocDialogues[] =
{
    // Beasts related, summons in between not handled here
    {TYPE_NORTHREND_BEASTS, 0, 0, 0,                        24000},
    {SAY_TIRION_BEAST_1,            NPC_TIRION_A, 0, 0,     12000},
    {SAY_VARIAN_BEAST_1,            NPC_VARIAN,         SAY_GARROSH_BEAST_1,        NPC_GARROSH,  0},
    {SAY_TIRION_BEAST_SLAY,         NPC_TIRION_A, 0, 0,     8000},
    {NPC_RAMSEY_2, 0, 0, 0,                                 0},
    {SAY_TIRION_BEAST_WIPE,         NPC_TIRION_A, 0, 0,     8000},
    {NPC_RAMSEY_1, 0, 0, 0,                                 0},
    // Jaruxxus (Intro)
    {TYPE_JARAXXUS, 0, 0, 0,                                1000},
    {SAY_TIRION_JARAXXUS_INTRO_1,   NPC_TIRION_A, 0, 0,     6000},
    {NPC_FIZZLEBANG, 0, 0, 0,                               26000},
    {SAY_WILFRED_JARAXXUS_INTRO_1,  NPC_FIZZLEBANG, 0, 0,   10000},
    {SAY_WILFRED_JARAXXUS_INTRO_2,  NPC_FIZZLEBANG, 0, 0,   7000},
    {EVENT_OPEN_PORTAL, 0, 0, 0,                            5000},
    {SAY_WILFRED_JARAXXUS_INTRO_3,  NPC_FIZZLEBANG, 0, 0,   12000}, // Summon also Jaraxxus
    {SAY_JARAXXUS_JARAXXAS_INTRO_1, NPC_JARAXXUS, 0, 0,     6000},
    {SAY_WILFRED_DEATH,             NPC_FIZZLEBANG, 0, 0,   1000},
    {EVENT_KILL_FIZZLEBANG, 0, 0, 0,                        5000},  // Kill Fizzlebang
    {SAY_TIRION_JARAXXUS_INTRO_2,   NPC_TIRION_A, 0, 0,     6000},
    {EVENT_JARAXXUS_START_ATTACK, 0, 0, 0,                  0},
    // Jaruxxus (Outro)
    {SAY_JARAXXUS_DEATH,            NPC_JARAXXUS, 0, 0,     6000},  // Jaraxxus Death
    {SAY_TIRION_JARAXXUS_EXIT_1,    NPC_TIRION_A, 0, 0,     5000},
    {SAY_GARROSH_JARAXXUS_EXIT_1,   NPC_GARROSH, 0, 0,      11000},
    {SAY_VARIAN_JARAXXUS_SLAY,      NPC_VARIAN, 0, 0,       8000},
    {SAY_TIRION_JARAXXUS_EXIT_2,    NPC_TIRION_A, 0, 0,     19000},
    {NPC_RAMSEY_3, 0, 0, 0,                                 0},
    // Grand Champions
    {SAY_TIRION_PVP_INTRO_1,        NPC_TIRION_A, 0, 0,     9000},
    {SAY_GARROSH_PVP_A_INTRO_1,     NPC_GARROSH,        SAY_VARIAN_PVP_H_INTRO_1,   NPC_VARIAN, 14000},
    {SAY_TIRION_PVP_INTRO_2,        NPC_TIRION_A, 0, 0,     1000},
    {TYPE_FACTION_CHAMPIONS, 0, 0, 0,                       5000},
    {SAY_GARROSH_PVP_A_INTRO_2,     NPC_GARROSH,        SAY_VARIAN_PVP_H_INTRO_2,   NPC_VARIAN, 0},
    {SAY_VARIAN_PVP_A_WIN,          NPC_VARIAN,         SAY_GARROSH_PVP_H_WIN,      NPC_GARROSH, 4000},
    {SAY_TIRION_PVP_WIN,            NPC_TIRION_A, 0, 0,     27000},
    {NPC_RAMSEY_4, 0, 0, 0,                                 0},
    // Twin Valkyrs
    {TYPE_TWIN_VALKYR, 0, 0, 0,                             17000},
    {EVENT_SUMMON_TWINS, 0, 0, 0,                           0},
    {EVENT_TWINS_KILLED, 0, 0, 0,                           2000},
    {NPC_RAMSEY_5, 0, 0, 0,                                 4000},
    {SAY_VARIAN_TWINS_A_WIN,        NPC_VARIAN,         SAY_GARROSH_TWINS_H_WIN,    NPC_GARROSH, 1000},
    {SAY_TIRION_TWINS_WIN,          NPC_TIRION_A, 0, 0,     0},
    // Anub'arak
    {TYPE_ANUBARAK, 0, 0, 0,                                19000},
    {SAY_LKING_ANUB_INTRO_1,        NPC_THE_LICHKING, 0, 0, 4000},
    {EVENT_ARTHAS_PORTAL, 0, 0, 0,                          2000},
    {EVENT_SUMMON_THE_LICHKING, 0, 0, 0,                    3000},
    {SAY_TIRION_ABUN_INTRO_1,       NPC_TIRION_A, 0, 0,     8000},
    {SAY_LKING_ANUB_INTRO_2,        NPC_THE_LICHKING_VISUAL, 0, 0, 18500},
    {EVENT_DESTROY_FLOOR, 0, 0, 0,                          2500},
    {SAY_LKING_ANUB_INTRO_3,        NPC_THE_LICHKING, 0, 0, 0},
    {0, 0, 0, 0 , 0}
};

struct is_trial_of_the_crusader : public InstanceScript
{
    is_trial_of_the_crusader() : InstanceScript("instance_trial_of_the_crusader") {}

    class instance_trial_of_the_crusader : public ScriptedInstance, private DialogueHelper
    {
    public:
        instance_trial_of_the_crusader(Map* pMap) : ScriptedInstance(pMap), DialogueHelper(aTocDialogues),
            m_uiTeam(TEAM_NONE)
        {
            Initialize();
        }

        void Initialize() override
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
            InitializeDialogueHelper(this);
        }

        bool IsEncounterInProgress() const override
        {
            for (uint8 i = TYPE_NORTHREND_BEASTS; i < MAX_ENCOUNTER; ++i)
            {
                if (m_auiEncounter[i] == IN_PROGRESS)
                    return true;
            }

            return false;
        }

        void OnCreatureCreate(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_FIZZLEBANG:
                DoUseDoorOrButton(GO_MAIN_GATE);
            case NPC_TIRION_A:
            case NPC_TIRION_B:
            case NPC_VARIAN:
            case NPC_GARROSH:
            case NPC_JARAXXUS:
            case NPC_OPEN_PORTAL_TARGET:
            case NPC_EYDIS:
            case NPC_WORLD_TRIGGER_LARGE:
            case NPC_THE_LICHKING:
            case NPC_THE_LICHKING_VISUAL:
                break;
            default:
                return;
            }

            m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
        }

        void OnObjectCreate(GameObject* pGo) override
        {
            switch (pGo->GetEntry())
            {
            case GO_COLISEUM_FLOOR:
                if (m_auiEncounter[TYPE_TWIN_VALKYR] == DONE)
                {
                    pGo->SetDisplayId(DISPLAYID_DESTROYED_FLOOR);
                    pGo->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_UNK_10 | GO_FLAG_NODESPAWN);
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_TRIBUTE_CHEST_10H_01:
            case GO_TRIBUTE_CHEST_10H_25:
            case GO_TRIBUTE_CHEST_10H_45:
            case GO_TRIBUTE_CHEST_10H_50:
            case GO_TRIBUTE_CHEST_25H_01:
            case GO_TRIBUTE_CHEST_25H_25:
            case GO_TRIBUTE_CHEST_25H_45:
            case GO_TRIBUTE_CHEST_25H_50:
            case GO_MAIN_GATE:
            case GO_WEB_DOOR:
            case GO_PORTAL_DALARAN:
                break;
            case GO_CRUSADERS_CACHE:
            case GO_CRUSADERS_CACHE_25:
            case GO_CRUSADERS_CACHE_10_H:
            case GO_CRUSADERS_CACHE_25_H:
                m_mGoEntryGuidStore[GO_CRUSADERS_CACHE] = pGo->GetObjectGuid();
                return;
            default:
                return;
            }
            m_mGoEntryGuidStore[pGo->GetEntry()] = pGo->GetObjectGuid();
        }

        void OnPlayerEnter(Player* pPlayer) override
        {
            if (m_uiTeam)
                return;

            m_uiTeam = pPlayer->GetTeam();
            SetDialogueSide(m_uiTeam == ALLIANCE);

            DoSummonRamsey(0);

            // Show wipe world state on heroic difficulty
            if (IsHeroicDifficulty())
            {
                pPlayer->SendUpdateWorldState(WORLD_STATE_WIPES, 1);
                pPlayer->SendUpdateWorldState(WORLD_STATE_WIPES_COUNT, MAX_WIPES_ALLOWED >= GetData(TYPE_WIPE_COUNT) ? MAX_WIPES_ALLOWED - GetData(TYPE_WIPE_COUNT) : 0);
            }
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            switch (uiType)
            {
            case TYPE_WIPE_COUNT:
                // Update data before updating worldstate
                m_auiEncounter[uiType] = uiData;
                if (IsHeroicDifficulty())
                    DoUpdateWorldState(WORLD_STATE_WIPES_COUNT, MAX_WIPES_ALLOWED >= GetData(TYPE_WIPE_COUNT) ? MAX_WIPES_ALLOWED - GetData(TYPE_WIPE_COUNT) : 0);
                break;
            case TYPE_NORTHREND_BEASTS:
                if (uiData == SPECIAL)
                {
                    if (Creature* pTirion = GetSingleCreatureFromStorage(NPC_TIRION_A))
                        DoScriptText(m_auiEncounter[uiType] != FAIL ? SAY_TIRION_RAID_INTRO_LONG : SAY_RAID_TRIALS_INTRO, pTirion);
                    StartNextDialogueText(TYPE_NORTHREND_BEASTS);
                }
                else if (uiData == FAIL)
                {
                    SetData(TYPE_WIPE_COUNT, m_auiEncounter[TYPE_WIPE_COUNT] + 1);
                    StartNextDialogueText(SAY_TIRION_BEAST_WIPE);
                }
                else if (uiData == DONE)
                    StartNextDialogueText(SAY_TIRION_BEAST_SLAY);
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_JARAXXUS:
                if (uiData == SPECIAL)
                    // TODO - What happen in wipe case?
                    StartNextDialogueText(TYPE_JARAXXUS);
                else if (uiData == FAIL)
                {
                    SetData(TYPE_WIPE_COUNT, m_auiEncounter[TYPE_WIPE_COUNT] + 1);
                    StartNextDialogueText(NPC_RAMSEY_2);
                }
                else if (uiData == DONE)
                    StartNextDialogueText(SAY_JARAXXUS_DEATH);
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_FACTION_CHAMPIONS:
                if (uiData == SPECIAL)
                    StartNextDialogueText(m_auiEncounter[uiType] != FAIL ? uint8(SAY_TIRION_PVP_INTRO_1) : uint8(TYPE_FACTION_CHAMPIONS));
                else if (uiData == FAIL)
                {
                    SetData(TYPE_WIPE_COUNT, m_auiEncounter[TYPE_WIPE_COUNT] + 1);
                    StartNextDialogueText(NPC_RAMSEY_3);
                }
                else if (uiData == DONE)
                {
                    DoRespawnGameObject(GO_CRUSADERS_CACHE, 60 * MINUTE);
                    StartNextDialogueText(SAY_VARIAN_PVP_A_WIN);
                }
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_TWIN_VALKYR:
                if (uiData == SPECIAL)
                {
                    if (Creature* pTirion = GetSingleCreatureFromStorage(NPC_TIRION_A))
                        DoScriptText(m_auiEncounter[uiType] != FAIL ? SAY_TIRION_TWINS_INTRO : SAY_RAID_INTRO_SHORT, pTirion);
                    StartNextDialogueText(TYPE_TWIN_VALKYR);
                }
                else if (uiData == FAIL)
                {
                    SetData(TYPE_WIPE_COUNT, m_auiEncounter[TYPE_WIPE_COUNT] + 1);
                    StartNextDialogueText(NPC_RAMSEY_4);
                }
                else if (uiData == DONE)
                    StartNextDialogueText(EVENT_TWINS_KILLED);
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_ANUBARAK:
                if (uiData == SPECIAL)
                    StartNextDialogueText(TYPE_ANUBARAK);
                else if (uiData == FAIL)
                    SetData(TYPE_WIPE_COUNT, m_auiEncounter[TYPE_WIPE_COUNT] + 1);
                else if (uiData == DONE)
                    DoHandleEventEpilogue();
                // Handle combat door
                if (uiData != SPECIAL)
                    DoUseDoorOrButton(GO_WEB_DOOR);
                m_auiEncounter[uiType] = uiData;
                break;
            default:
                script_error_log("Instance Trial of The Crusader: ERROR SetData = %u for type %u does not exist/not implemented.", uiType, uiData);
                return;
            }

            if (uiData == DONE || uiType == TYPE_WIPE_COUNT)
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
                return m_auiEncounter[uiType];

            if (uiType == TYPE_DATA_IS_HEROIC)
                return uint32(IsHeroicDifficulty());

            return 0;
        }

        // Difficulty wrappers
        bool IsHeroicDifficulty() const { return instance->GetDifficulty() == RAID_DIFFICULTY_10MAN_HEROIC || instance->GetDifficulty() == RAID_DIFFICULTY_25MAN_HEROIC; }
        bool Is25ManDifficulty() const { return instance->GetDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL || instance->GetDifficulty() == RAID_DIFFICULTY_25MAN_HEROIC; }

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

            for (uint8 i = TYPE_NORTHREND_BEASTS; i < MAX_ENCOUNTER; ++i)
            if (m_auiEncounter[i] == IN_PROGRESS)            // Do not load an encounter as "In Progress" - reset it instead.
                m_auiEncounter[i] = NOT_STARTED;

            OUT_LOAD_INST_DATA_COMPLETE;
        }

        void Update(uint32 uiDiff) { DialogueUpdate(uiDiff); }

    private:
        void DoSummonRamsey(uint32 uiEntry)
        {
            Player* pPlayer = GetPlayerInMap();
            if (!pPlayer)
                return;

            if (uiEntry)
                ;
            // For initial case, figure which Ramsay to summon
            else if (m_auiEncounter[TYPE_TWIN_VALKYR] == DONE)
                uiEntry = NPC_RAMSEY_5;
            else if (m_auiEncounter[TYPE_FACTION_CHAMPIONS] == DONE)
                uiEntry = NPC_RAMSEY_4;
            else if (m_auiEncounter[TYPE_JARAXXUS] == DONE)
                uiEntry = NPC_RAMSEY_3;
            else if (m_auiEncounter[TYPE_NORTHREND_BEASTS] == DONE)
                uiEntry = NPC_RAMSEY_2;
            else
                uiEntry = NPC_RAMSEY_1;

            pPlayer->SummonCreature(uiEntry, aRamsayPositions[0][0], aRamsayPositions[0][1], aRamsayPositions[0][2], aRamsayPositions[0][3], TEMPSUMMON_DEAD_DESPAWN, 0);
        }

        void JustDidDialogueStep(int32 iEntry) override
        {
            switch (iEntry)
            {
            case NPC_RAMSEY_1:
            case NPC_RAMSEY_2:
            case NPC_RAMSEY_3:
            case NPC_RAMSEY_4:
            case NPC_RAMSEY_5:
                DoSummonRamsey(iEntry);
                break;
            case SAY_VARIAN_BEAST_1:
                if (Player* pPlayer = GetPlayerInMap())
                {
                    if (Creature* pBeasts = pPlayer->SummonCreature(NPC_BEAST_COMBAT_STALKER, aSpawnPositions[0][0], aSpawnPositions[0][1], aSpawnPositions[0][2], aSpawnPositions[0][3], TEMPSUMMON_DEAD_DESPAWN, 0))
                        pBeasts->SummonCreature(NPC_GORMOK, aSpawnPositions[1][0], aSpawnPositions[1][1], aSpawnPositions[1][2], aSpawnPositions[1][3], TEMPSUMMON_DEAD_DESPAWN, 0);
                }
                break;
            case NPC_FIZZLEBANG:
                if (Player* pPlayer = GetPlayerInMap())
                    pPlayer->SummonCreature(NPC_FIZZLEBANG, aSpawnPositions[5][0], aSpawnPositions[5][1], aSpawnPositions[5][2], aSpawnPositions[5][3], TEMPSUMMON_DEAD_DESPAWN, 0);
                break;
            case SAY_WILFRED_JARAXXUS_INTRO_1:
                DoUseDoorOrButton(GO_MAIN_GATE); // Close main gate
                break;
            case SAY_WILFRED_JARAXXUS_INTRO_2:
                if (Creature* pFizzlebang = GetSingleCreatureFromStorage(NPC_FIZZLEBANG))
                {
                    pFizzlebang->SummonCreature(NPC_PURPLE_RUNE, aSpawnPositions[11][0], aSpawnPositions[11][1], aSpawnPositions[11][2], aSpawnPositions[11][3], TEMPSUMMON_TIMED_DESPAWN, 15000);
                    pFizzlebang->CastSpell(pFizzlebang, SPELL_OPEN_PORTAL, false);
                }
                break;
            case EVENT_OPEN_PORTAL:
                if (Creature* pOpenPortalTarget = GetSingleCreatureFromStorage(NPC_OPEN_PORTAL_TARGET))
                {
                    pOpenPortalTarget->CastSpell(pOpenPortalTarget, SPELL_WILFRED_PORTAL, true);
                    pOpenPortalTarget->ForcedDespawn(9000);
                }
                break;
            case SAY_WILFRED_JARAXXUS_INTRO_3:
                if (Player* pPlayer = GetPlayerInMap())
                if (Creature* pJaraxxus = pPlayer->SummonCreature(NPC_JARAXXUS, aSpawnPositions[6][0], aSpawnPositions[6][1], aSpawnPositions[6][2], aSpawnPositions[6][3], TEMPSUMMON_DEAD_DESPAWN, 0))
                    pJaraxxus->GetMotionMaster()->MovePoint(POINT_COMBAT_POSITION, aMovePositions[3][0], aMovePositions[3][1], aMovePositions[3][2]);
                break;
            case EVENT_KILL_FIZZLEBANG:
                if (Creature* pJaraxxus = GetSingleCreatureFromStorage(NPC_JARAXXUS))
                    pJaraxxus->CastSpell(pJaraxxus, SPELL_FEL_LIGHTNING_KILL, true);
                break;
            case EVENT_JARAXXUS_START_ATTACK:
                if (Creature* pJaraxxus = GetSingleCreatureFromStorage(NPC_JARAXXUS))
                    pJaraxxus->SetInCombatWithZone();
                break;
            case EVENT_SUMMON_TWINS:
                if (Player* pPlayer = GetPlayerInMap())
                {
                    pPlayer->SummonCreature(NPC_FJOLA, aSpawnPositions[7][0], aSpawnPositions[7][1], aSpawnPositions[7][2], aSpawnPositions[7][3], TEMPSUMMON_DEAD_DESPAWN, 0);
                    pPlayer->SummonCreature(NPC_EYDIS, aSpawnPositions[8][0], aSpawnPositions[8][1], aSpawnPositions[8][2], aSpawnPositions[8][3], TEMPSUMMON_DEAD_DESPAWN, 0);
                }
                break;
            case SAY_LKING_ANUB_INTRO_1:
                if (Player* pPlayer = GetPlayerInMap())
                    pPlayer->SummonCreature(NPC_WORLD_TRIGGER_LARGE, aSpawnPositions[9][0], aSpawnPositions[9][1], aSpawnPositions[9][2], aSpawnPositions[9][3], TEMPSUMMON_DEAD_DESPAWN, 0);
                break;
            case EVENT_ARTHAS_PORTAL:
                if (Creature* pWorldTriggerLarge = GetSingleCreatureFromStorage(NPC_WORLD_TRIGGER_LARGE))
                    pWorldTriggerLarge->CastSpell(pWorldTriggerLarge, SPELL_ARTHAS_PORTAL, true);
                break;
            case EVENT_SUMMON_THE_LICHKING:
                if (Player* pPlayer = GetPlayerInMap())
                    pPlayer->SummonCreature(NPC_THE_LICHKING_VISUAL, aSpawnPositions[10][0], aSpawnPositions[10][1], aSpawnPositions[10][2], aSpawnPositions[10][3], TEMPSUMMON_DEAD_DESPAWN, 0);
                break;
            case EVENT_DESTROY_FLOOR:
                if (GameObject* pColiseumFloor = GetSingleGameObjectFromStorage(GO_COLISEUM_FLOOR))
                {
                    pColiseumFloor->SetDisplayId(DISPLAYID_DESTROYED_FLOOR);
                    pColiseumFloor->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_UNK_10 | GO_FLAG_NODESPAWN);
                    pColiseumFloor->SetGoState(GO_STATE_ACTIVE);
                }

                if (Creature* pLichKingVisual = GetSingleCreatureFromStorage(NPC_THE_LICHKING_VISUAL))
                {
                    pLichKingVisual->CastSpell(pLichKingVisual, SPELL_FROSTNOVA, true);
                    // pLichKingVisual->CastSpell(pLichKingVisual, SPELL_CORPSE_TELEPORT, true); // NYI
                    pLichKingVisual->ForcedDespawn();
                }

                if (Creature* pLichKing = GetSingleCreatureFromStorage(NPC_THE_LICHKING))
                    pLichKing->CastSpell(pLichKing, SPELL_DESTROY_FLOOR_KNOCKUP, true);

                if (Creature* pWorldTriggerLarge = GetSingleCreatureFromStorage(NPC_WORLD_TRIGGER_LARGE))
                    pWorldTriggerLarge->ForcedDespawn();
                break;
            }
        }

        void DoHandleEventEpilogue()
        {
            Player* pPlayer = GetPlayerInMap();
            if (!pPlayer)
                return;

            // Spawn Tirion and the mage
            if (Creature* pTirion = pPlayer->SummonCreature(NPC_TIRION_B, aSpawnPositions[12][0], aSpawnPositions[12][1], aSpawnPositions[12][2], aSpawnPositions[12][3], TEMPSUMMON_CORPSE_DESPAWN, 0))
                DoScriptText(SAY_TIRION_EPILOGUE, pTirion);

            pPlayer->SummonCreature(NPC_ARGENT_MAGE, aSpawnPositions[13][0], aSpawnPositions[13][1], aSpawnPositions[13][2], aSpawnPositions[13][3], TEMPSUMMON_CORPSE_DESPAWN, 0);

            DoRespawnGameObject(GO_PORTAL_DALARAN, 60 * MINUTE);

            // Spawn the chest for heroic difficulty
            if (IsHeroicDifficulty())
            {
                if (GetData(TYPE_WIPE_COUNT) == 0)
                    DoRespawnGameObject(Is25ManDifficulty() ? GO_TRIBUTE_CHEST_25H_50 : GO_TRIBUTE_CHEST_10H_50, 60 * MINUTE);
                else if (GetData(TYPE_WIPE_COUNT) < 5)
                    DoRespawnGameObject(Is25ManDifficulty() ? GO_TRIBUTE_CHEST_25H_45 : GO_TRIBUTE_CHEST_10H_45, 60 * MINUTE);
                else if (GetData(TYPE_WIPE_COUNT) < 25)
                    DoRespawnGameObject(Is25ManDifficulty() ? GO_TRIBUTE_CHEST_25H_25 : GO_TRIBUTE_CHEST_10H_25, 60 * MINUTE);
                else
                    DoRespawnGameObject(Is25ManDifficulty() ? GO_TRIBUTE_CHEST_25H_01 : GO_TRIBUTE_CHEST_10H_01, 60 * MINUTE);
            }
        }

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strInstData;

        Team m_uiTeam;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_trial_of_the_crusader(pMap);
    }
};

void AddSC_instance_trial_of_the_crusader()
{
    Script* s;

    s = new is_trial_of_the_crusader();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_trial_of_the_crusader";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_trial_of_the_crusader;
    //pNewScript->RegisterSelf();
}

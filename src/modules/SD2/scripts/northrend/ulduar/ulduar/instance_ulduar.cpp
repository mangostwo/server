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
*/

/* ScriptData
SDName: instance_ulduar
SD%Complete:
SDComment:
SDCategory: Ulduar
EndScriptData */

#include "precompiled.h"
#include "ulduar.h"

enum
{
    SAY_PRE_LEVIATHAN_1                     = -1603239,
    SAY_PRE_LEVIATHAN_2                     = -1603240,
    SAY_PRE_LEVIATHAN_3                     = -1603241,

    SAY_FREYA_HELP                          = -1603009,
    SAY_HODIR_HELP                          = -1603093,
    SAY_THORIM_HELP                         = -1603155,
    SAY_MIMIRON_HELP                        = -1603195,
    
    SAY_GROUNDED                            = -1603040,

    SPELL_KEEPER_ACTIVE                     = 62647,
    SPELL_CLEAR_INSANE                      = 63122,       // clear all the sanity and insane on wipe / death
    SPELL_THREAT                            = 65146,       // used by npc 33816 to apply threat to Razorscale
    SPELL_RUNIC_SMASH                       = 62465,       // cast by npcs 33140 and 33141

    SPEED_RATE_HELPERS                      = 8,
};

static const DialogueEntry aUlduarDialogue[] =
{
    {SAY_PRE_LEVIATHAN_1,       NPC_BRONZEBEARD_RADIO,      7000},
    {SAY_PRE_LEVIATHAN_2,       NPC_BRONZEBEARD_RADIO,      5000},
    {SAY_PRE_LEVIATHAN_3,       NPC_BRONZEBEARD_RADIO,      2000},
    {NPC_LEVIATHAN,             0,                          0},
    {0, 0, 0}
};

struct UlduarKeeperSpawns
{
    float fX, fY, fZ, fO;
    uint32 uiEntry, uiType;
    int32 iText;
};

static UlduarKeeperSpawns m_aKeepersSpawnLocs[] =
{
    {1945.682f, 33.34201f,  411.4408f, 5.270f, NPC_KEEPER_FREYA,   TYPE_FREYA, 0},
    {2028.766f, 17.42014f,  411.4446f, 3.857f, NPC_KEEPER_MIMIRON, TYPE_MIMIRON, 0},
    {1945.761f, -81.52171f, 411.4407f, 1.029f, NPC_KEEPER_HODIR,   TYPE_HODIR, 0},
    {2028.822f, -65.73573f, 411.4426f, 2.460f, NPC_KEEPER_THORIM,  TYPE_THORIM, 0},
};

static UlduarKeeperSpawns m_aKeeperHelperLocs[] =
{
    {2036.873f,  25.42513f, 338.4984f, 3.909f, NPC_FREYA_HELPER,   TYPE_KEEPER_FREYA,   SAY_FREYA_HELP},
    {2036.658f, -73.58822f, 338.4985f, 2.460f, NPC_MIMIRON_HELPER, TYPE_KEEPER_MIMIRON, SAY_MIMIRON_HELP},
    {1939.045f, -90.87457f, 338.5426f, 0.994f, NPC_HODIR_HELPER,   TYPE_KEEPER_HODIR,   SAY_HODIR_HELP},
    {1939.148f,  42.49035f, 338.5427f, 5.235f, NPC_THORIM_HELPER,  TYPE_KEEPER_THORIM,  SAY_THORIM_HELP},
};

struct UlduarSpawn
{
    float fX, fY, fZ, fO;
    uint32 uiEntry;
};

struct UlduarSpawnTwoSide
{
    float fX, fY, fZ, fO;
    uint32 uiAllyEntry, uiHordeEntry;
};

// spawn locations for Brann and Doren at the archivum
static const float afBrannArchivumSpawnPos[4] = { 1554.274f, 142.1644f, 427.273f, 3.61f };
static const float afProspectorSpawnPos[4] = { 1556.469f, 143.5023f, 427.2918f, 4.04f };

// note: original spawn loc is 607.9199f, -12.90516f, 409.887f but we won't use it because it's too far and grid won't be loaded that far
static const float afLeviathanSpawnPos[4] = { 422.8898f, -13.32677f, 409.8839f, 3.12f };
static const float afLeviathanMovePos[4] = { 296.5809f, -11.55668f, 409.8278f, 3.12f };

// Note: coordinates are guessed, but pretty close to what they should be
// ToDo: spawn additional Engineers, Demolitionists, Mages and Liquid Pyrite near the columns
// ToDo: do ckeck for horde entries
static const UlduarSpawn afReinforcementsNormal[] =
{
    { 118.797f, -26.9963f, 409.80f, 3.14f, NPC_SALVAGED_SIEGE_ENGINE },
    { 118.847f, -43.758f, 409.80f, 3.15f, NPC_SALVAGED_SIEGE_ENGINE },
    { 116.602f, 8.464f, 409.80f, 3.10f, NPC_SALVAGED_CHOPPER },
    { 116.859f, -4.199f, 409.80f, 3.12f, NPC_SALVAGED_CHOPPER },
    { 122.479f, 25.093f, 410.60f, 3.10f, NPC_SALVAGED_DEMOLISHER },
    { 123.022f, 39.671f, 409.80f, 3.10f, NPC_SALVAGED_DEMOLISHER },
};

// ToDo: do check for horde entries
static const UlduarSpawn afReinforcementsHeroic[] =
{
    { 106.359f, -35.269f, 409.80f, 3.12f, NPC_SALVAGED_SIEGE_ENGINE },
    { 135.351f, -20.767f, 409.80f, 3.15f, NPC_SALVAGED_SIEGE_ENGINE },
    { 135.408f, -50.178f, 409.80f, 3.12f, NPC_SALVAGED_SIEGE_ENGINE },
    { 116.429f, 4.036f, 409.79f, 3.10f, NPC_SALVAGED_CHOPPER },
    { 116.272f, -0.013f, 409.79f, 3.10f, NPC_SALVAGED_CHOPPER },
    { 116.948f, -8.351f, 409.79f, 3.10f, NPC_SALVAGED_CHOPPER },
    { 137.523f, 32.346f, 409.80f, 3.12f, NPC_SALVAGED_DEMOLISHER },
    { 112.818f, 18.981f, 409.83f, 3.10f, NPC_SALVAGED_DEMOLISHER },
    { 112.700f, 47.884f, 409.79f, 3.10f, NPC_SALVAGED_DEMOLISHER },
};

static const UlduarSpawnTwoSide afHodirHelpersNormal[] =
{
    { 1999.903f, -230.4966f, 432.7581f, 1.53589f, NPC_DRUID_ALLIANCE_N, NPC_DRUID_HORDE_N },
    { 2010.058f, -243.4553f, 432.7672f, 1.361357f, NPC_SHAMAN_ALLIANCE_N, NPC_SHAMAN_HORDE_N },
    { 2021.118f, -236.6482f, 432.7672f, 1.937315f, NPC_MAGE_ALLIANCE_N, NPC_MAGE_HORDE_N },
    { 1983.751f, -243.3579f, 432.7672f, 1.570796f, NPC_PRIEST_ALLIANCE_N, NPC_PRIEST_HORDE_N },
};

static const UlduarSpawnTwoSide afHodirHelpersHeroic[] =
{
    { 2013.37f, -240.331f, 432.687f, 1.80463f, NPC_DRUID_ALLIANCE_H, NPC_DRUID_HORDE_H },
    { 1983.89f, -240.369f, 432.687f, 1.37658f, NPC_SHAMAN_ALLIANCE_H, NPC_SHAMAN_HORDE_H },
    { 2000.9f, -231.232f, 432.687f, 1.59846f, NPC_MAGE_ALLIANCE_H, NPC_MAGE_HORDE_H },
    { 1997.88f, -239.394f, 432.687f, 1.4237f, NPC_PRIEST_ALLIANCE_H, NPC_PRIEST_HORDE_H },
};

static const UlduarSpawnTwoSide afThorimSpawns[] =
{
    { 2127.24f, -251.309f, 419.7935f, 5.899213f, NPC_SOLDIER_HORDE, NPC_SOLDIER_ALLIANCE },
    { 2123.316f, -254.7708f, 419.7886f, 6.178465f, NPC_SOLDIER_HORDE, NPC_SOLDIER_ALLIANCE },
    { 2120.431f, -259.0431f, 419.6813f, 6.122538f, NPC_SOLDIER_HORDE, NPC_SOLDIER_ALLIANCE },
    { 2145.503f, -256.3357f, 419.7306f, 3.520873f, NPC_CAPTAIN_HORDE, NPC_CAPTAIN_ALLIANCE },
};

struct is_ulduar : public InstanceScript
{
    is_ulduar() : InstanceScript("instance_ulduar") {}

    class instance_ulduar : public ScriptedInstance, private DialogueHelper
    {
    public:
        instance_ulduar(Map* pMap)
            : ScriptedInstance(pMap), DialogueHelper(aUlduarDialogue),
            m_bHelpersLoaded(false),
            m_bSmashSide(true),
            m_uiAlgalonTimer(MINUTE * IN_MILLISECONDS),
            m_uiYoggResetTimer(0),
            m_uiShatterAchievTimer(0),
            m_uiGauntletStatus(0),
            m_uiStairsSpawnTimer(0),
            m_uiSlayedArenaMobs(0),
            m_uiRazorCurrentHarpoon(0),
            m_uiThorimSmashIndex(0)
        {
            Initialize();
        }

        ~instance_ulduar() {}

        void Initialize() override
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
            memset(&m_auiHardBoss, 0, sizeof(m_auiHardBoss));
            memset(&m_auiUlduarKeepers, 0, sizeof(m_auiUlduarKeepers));
            memset(&m_auiUlduarTowers, 0, sizeof(m_auiUlduarTowers));

            InitializeDialogueHelper(this);

            for (uint8 i = 0; i < MAX_SPECIAL_ACHIEV_CRITS; ++i)
                m_abAchievCriteria[i] = false;
        }

        bool IsEncounterInProgress() const override
        {
            for (uint8 i = 0; i <= TYPE_ALGALON; ++i)
            {
                if (m_auiEncounter[i] == IN_PROGRESS)
                    return true;
            }
            return false;
        }

        void OnPlayerEnter(Player* pPlayer) override
        {
            // spawn Flame Leviathan if necessary
            if (GetData(TYPE_LEVIATHAN) == SPECIAL || GetData(TYPE_LEVIATHAN) == FAIL)
            {
                if (!GetSingleCreatureFromStorage(NPC_LEVIATHAN, true))
                {
                    pPlayer->SummonCreature(NPC_LEVIATHAN, afLeviathanMovePos[0], afLeviathanMovePos[1], afLeviathanMovePos[2], afLeviathanMovePos[3], TEMPSUMMON_DEAD_DESPAWN, 0, true);
                    DoCallLeviathanHelp();
                }
            }

            // spawn Brann at the archivum if necessary
            if (GetData(TYPE_ASSEMBLY) == DONE)
            {
                if (!GetSingleCreatureFromStorage(NPC_BRANN_ARCHIVUM, true))
                {
                    pPlayer->SummonCreature(NPC_BRANN_ARCHIVUM, afBrannArchivumSpawnPos[0], afBrannArchivumSpawnPos[1], afBrannArchivumSpawnPos[2], afBrannArchivumSpawnPos[3], TEMPSUMMON_DEAD_DESPAWN, 0, true);
                    pPlayer->SummonCreature(instance->IsRegularDifficulty() ? NPC_PROSPECTOR_DOREN : NPC_PROSPECTOR_DOREN_H, afProspectorSpawnPos[0], afProspectorSpawnPos[1], afProspectorSpawnPos[2], afProspectorSpawnPos[3], TEMPSUMMON_DEAD_DESPAWN, 0, true);
                }
            }

            // spawn Algalon and init world states if necessary
            if (GetData(TYPE_ALGALON_TIMER))
            {
                if (!GetSingleCreatureFromStorage(NPC_ALGALON, true))
                    pPlayer->SummonCreature(NPC_ALGALON, afAlgalonMovePos[0], afAlgalonMovePos[1], afAlgalonMovePos[2], afAlgalonMovePos[3], TEMPSUMMON_DEAD_DESPAWN, 0, true);

                pPlayer->SendUpdateWorldState(WORLD_STATE_TIMER, 1);
                pPlayer->SendUpdateWorldState(WORLD_STATE_TIMER_COUNT, GetData(TYPE_ALGALON_TIMER));
            }

            // spawn frienly keepers in the central hall, keeper helpers for Yogg-Saron and all the other faction npcs
            if (!m_bHelpersLoaded)
            {
                for (uint8 i = 0; i < countof(m_aKeepersSpawnLocs); ++i)
                {
                    if (GetData(m_aKeepersSpawnLocs[i].uiType) == DONE)
                        pPlayer->SummonCreature(m_aKeepersSpawnLocs[i].uiEntry, m_aKeepersSpawnLocs[i].fX, m_aKeepersSpawnLocs[i].fY, m_aKeepersSpawnLocs[i].fZ, m_aKeepersSpawnLocs[i].fO, TEMPSUMMON_CORPSE_DESPAWN, 0, true);
                }

                for (uint8 i = 0; i < countof(m_aKeeperHelperLocs); ++i)
                {
                    if (GetData(m_aKeeperHelperLocs[i].uiType) == DONE)
                        pPlayer->SummonCreature(m_aKeeperHelperLocs[i].uiEntry, m_aKeeperHelperLocs[i].fX, m_aKeeperHelperLocs[i].fY, m_aKeeperHelperLocs[i].fZ, m_aKeeperHelperLocs[i].fO, TEMPSUMMON_CORPSE_DESPAWN, 0, true);
                }

                DoSpawnHodirNpcs(pPlayer);
                m_bHelpersLoaded = true;
            }
        }

        void OnPlayerDeath(Player* pPlayer) override
        {
            if (IsEncounterInProgress())
                SetData(TYPE_CHAMPION_FAILED, DONE);
        }

        void OnCreatureCreate(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_LEVIATHAN:
            case NPC_EXPLORER_DELLORAH:
            case NPC_BRANN_BRONZEBEARD:
            case NPC_ORBITAL_SUPPORT:
            case NPC_RAZORSCALE:
            case NPC_EXPEDITION_COMMANDER:
            case NPC_XT002:
            case NPC_HEART_DECONSTRUCTOR:

            case NPC_STEELBREAKER:
            case NPC_MOLGEIM:
            case NPC_BRUNDIR:
            case NPC_KOLOGARN:
            case NPC_RIGHT_ARM:
            case NPC_LEFT_ARM:
            case NPC_AURIAYA:
            case NPC_FERAL_DEFENDER:
            case NPC_BRANN_ARCHIVUM:
            case NPC_BRANN_ALGALON:

            case NPC_LEVIATHAN_MK:
            case NPC_LEVIATHAN_MK_TURRET:
            case NPC_COMPUTER:
            case NPC_VX001:
            case NPC_AERIAL_UNIT:
            case NPC_WORLD_TRIGGER_FLAMES:
            case NPC_RUNIC_COLOSSUS:
            case NPC_RUNE_GIANT:
            case NPC_SIF:
            case NPC_JORMUNGAR_BEHEMOTH:
            case NPC_THORIM_COMBAT_TRIGGER:
            case NPC_ELDER_BRIGHTLEAF:
            case NPC_ELDER_IRONBRACH:
            case NPC_ELDER_STONEBARK:
            case NPC_VEZAX:
            case NPC_SARONITE_ANIMUS:
            case NPC_YOGGSARON:
            case NPC_SARA:
            case NPC_YOGG_BRAIN:
            case NPC_VOICE_OF_YOGG:
            case NPC_ALGALON:

            case NPC_MIMIRON:
            case NPC_HODIR:
            case NPC_THORIM:
            case NPC_FREYA:
            case NPC_THORIM_HELPER:
            case NPC_MIMIRON_HELPER:
            case NPC_HODIR_HELPER:
            case NPC_FREYA_HELPER:

            case NPC_YSERA:
            case NPC_NELTHARION:
            case NPC_MALYGOS:
            case NPC_ALEXSTRASZA:
            case NPC_GARONA:
            case NPC_KING_LLANE:
            case NPC_LICH_KING:
            case NPC_IMMOLATED_CHAMPION:
            case NPC_YOGGSARON_ILLUSION:
                break;

            case NPC_ULDUAR_COLOSSUS:
                if (pCreature->GetPositionX() > 300.0f)
                    m_sColossusGuidSet.insert(pCreature->GetObjectGuid());
                return;
            case NPC_EXPEDITION_DEFENDER:
                m_lDefendersGuids.push_back(pCreature->GetObjectGuid());
                return;
            case NPC_EXPEDITION_ENGINEER:
                m_lEngineersGuids.push_back(pCreature->GetObjectGuid());
                return;
            case NPC_EXPEDITION_TRAPPER:
                m_lTrappersGuids.push_back(pCreature->GetObjectGuid());
                return;
            case NPC_RAZORSCALE_CONTROLLER:
                // sort the controllers which are assigned to harpoons and allow the central one into the mail guid store
                if (pCreature->GetPositionY() > -145.0f)
                {
                    m_lHarpoonDummyGuids.push_back(pCreature->GetObjectGuid());
                    return;
                }
                break;
            case NPC_XT_TOY_PILE:
                m_vToyPileGuidVector.push_back(pCreature->GetObjectGuid());
                return;
            case NPC_RUBBLE_STALKER:
                if (pCreature->GetPositionY() > -10.0f)
                    m_rightKoloStalkerGuid = pCreature->GetObjectGuid();
                else
                    m_leftKoloStalkerGuid = pCreature->GetObjectGuid();
                return;
            case NPC_THORIM_EVENT_BUNNY:
                // sort the event bunnies between the arena and tribune spawns; the platform spawns are ignored for the moment
                if (pCreature->GetPositionZ() < 420.0f)
                    m_lThorimBunniesGuids.push_back(pCreature->GetObjectGuid());
                else if (pCreature->GetPositionZ() > 438.5f)
                    m_lUpperBunniesGuids.push_back(pCreature->GetObjectGuid());
                return;
            case NPC_THUNDER_ORB:
                // get only the upper ones; the lower ones are searched dynamically in order to be paired correctly
                if (pCreature->GetPositionZ() > 430.0f)
                    m_lUpperThunderOrbsGuids.push_back(pCreature->GetObjectGuid());
                return;
            case NPC_LEFT_HAND_BUNNY:
                m_lLeftHandBunniesGuids.push_back(pCreature->GetObjectGuid());
                return;
            case NPC_RIGHT_HAND_BUNNY:
                m_lRightHandBunniesGuids.push_back(pCreature->GetObjectGuid());
                return;
            case NPC_OMINOUS_CLOUD:
                m_lOminousCloudsGuids.push_back(pCreature->GetObjectGuid());
                return;
            case NPC_VEZAX_BUNNY:
                if (pCreature->GetPositionY() < 100.0f)
                    m_animusVezaxBunnyGuid = pCreature->GetObjectGuid();
                else
                    m_vaporVezaxBunnyGuid = pCreature->GetObjectGuid();
                return;

            default:
                return;
            }
            m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
        }

        void OnCreatureEnterCombat(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_RUNE_GIANT:
                m_uiStairsSpawnTimer = 0;
                break;
            }
        }

        void OnCreatureDeath(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_SANCTUM_SENTRY:
                if (GetData(TYPE_AURIAYA) == IN_PROGRESS)
                    SetSpecialAchievementCriteria(TYPE_ACHIEV_CAT_LADY, false);
                break;
            case NPC_ULDUAR_COLOSSUS:
            {
                if (m_sColossusGuidSet.find(pCreature->GetObjectGuid()) != m_sColossusGuidSet.end())
                    m_sColossusGuidSet.erase(pCreature->GetObjectGuid());

                // start pre Leviathan event
                if (m_sColossusGuidSet.empty())
                {
                    StartNextDialogueText(SAY_PRE_LEVIATHAN_1);
                    SetData(TYPE_LEVIATHAN, SPECIAL);
                    SetData(TYPE_LEVIATHAN_GAUNTLET, DONE);
                    pCreature->SummonCreature(NPC_LEVIATHAN, afLeviathanSpawnPos[0], afLeviathanSpawnPos[1], afLeviathanSpawnPos[2], afLeviathanSpawnPos[3], TEMPSUMMON_DEAD_DESPAWN, 0, true);
                }
            }
                break;
            case NPC_DRUID_HORDE_N:
            case NPC_DRUID_HORDE_H:
            case NPC_SHAMAN_HORDE_N:
            case NPC_SHAMAN_HORDE_H:
            case NPC_MAGE_HORDE_N:
            case NPC_MAGE_HORDE_H:
            case NPC_PRIEST_HORDE_N:
            case NPC_PRIEST_HORDE_H:
            case NPC_DRUID_ALLIANCE_N:
            case NPC_DRUID_ALLIANCE_H:
            case NPC_SHAMAN_ALLIANCE_N:
            case NPC_SHAMAN_ALLIANCE_H:
            case NPC_MAGE_ALLIANCE_N:
            case NPC_MAGE_ALLIANCE_H:
            case NPC_PRIEST_ALLIANCE_N:
            case NPC_PRIEST_ALLIANCE_H:
                if (GetData(TYPE_HODIR) == IN_PROGRESS)
                    SetSpecialAchievementCriteria(TYPE_ACHIEV_COOL_FRIENDS, false);
                break;
            case NPC_JORMUNGAR_BEHEMOTH:
            case NPC_SOLDIER_ALLIANCE:
            case NPC_CAPTAIN_ALLIANCE:
            case NPC_SOLDIER_HORDE:
            case NPC_CAPTAIN_HORDE:
            case NPC_DARK_RUNE_ACOLYTE:
                ++m_uiSlayedArenaMobs;

                // start combat when all 4 faction soldiers, the Acolyte and the Jormungar are dead
                if (m_uiSlayedArenaMobs == 6)
                {
                    if (Creature* pThorim = GetSingleCreatureFromStorage(NPC_THORIM))
                        pThorim->SetInCombatWithZone();
                }
                break;
            case NPC_RUNIC_COLOSSUS:
                m_uiStairsSpawnTimer = 30000;
                DoUseDoorOrButton(GO_RUNED_STONE_DOOR);
                break;
            case NPC_RUNE_GIANT:
                DoUseDoorOrButton(GO_THORIM_STONE_DOOR);
                break;
            case NPC_SARONITE_ANIMUS:
                if (Creature* pVezax = GetSingleCreatureFromStorage(NPC_VEZAX))
                {
                    if (pVezax->IsAlive())
                    {
                        pCreature->AI()->SendAIEvent(AI_EVENT_CUSTOM_C, pCreature, pVezax);
                        SetData(TYPE_VEZAX_HARD, DONE);
                    }
                }
                break;
            }
        }

        void OnObjectCreate(GameObject* pGo) override
        {
            switch (pGo->GetEntry())
            {
                // -----------------    Doors & Other   -----------------
                // The siege
            case GO_SHIELD_WALL:
                break;
            case GO_LIGHTNING_DOOR:
                if (m_auiEncounter[TYPE_LEVIATHAN] == SPECIAL || m_auiEncounter[TYPE_LEVIATHAN] == FAIL)
                    pGo->SetGoState(GO_STATE_READY);
                break;
            case GO_LEVIATHAN_GATE:
                if (m_auiEncounter[TYPE_LEVIATHAN] != NOT_STARTED)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_XT002_GATE:
                pGo->SetGoState(GO_STATE_READY);
                if (m_auiEncounter[TYPE_XT002] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                if (m_auiEncounter[TYPE_LEVIATHAN] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_HODIR_CRYSTAL:
                if (m_auiUlduarTowers[0] == FAIL)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_THORIM_CRYSTAL:
                if (m_auiUlduarTowers[1] == FAIL)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_FREYA_CRYSTAL:
                if (m_auiUlduarTowers[2] == FAIL)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_MIMIRON_CRYSTAL:
                if (m_auiUlduarTowers[3] == FAIL)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;

                // Archivum
            case GO_IRON_ENTRANCE_DOOR:
                break;
            case GO_ARCHIVUM_DOOR:
                if (m_auiEncounter[TYPE_ASSEMBLY])
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
                // Celestial Planetarium
            case GO_CELESTIAL_ACCES:
            case GO_CELESTIAL_ACCES_H:
                // Note: weird, but unless flag is set, client will not respond as expected
                pGo->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_LOCKED);
                break;
            case GO_CELESTIAL_DOOR_1:
            case GO_CELESTIAL_DOOR_2:
                if (m_auiEncounter[TYPE_ALGALON] != NOT_STARTED)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_CELESTIAL_DOOR_COMBAT:
            case GO_UNIVERSE_FLOOR:
            case GO_UNIVERSE_FLOOR_COMBAT:
            case GO_AZEROTH_GLOBE:
                break;
                // Shattered Hallway
            case GO_KOLOGARN_BRIDGE:
                if (m_auiEncounter[TYPE_KOLOGARN] == DONE)
                    pGo->SetGoState(GO_STATE_READY);
                break;

                // -----------------    The Keepers    -----------------
                // Hodir
            case GO_HODIR_EXIT:
            case GO_HODIR_ICE_WALL:
                if (m_auiEncounter[TYPE_HODIR] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_HODIR_ENTER:
                break;
                // Mimiron
            case GO_MIMIRON_BUTTON:
            case GO_MIMIRON_DOOR_1:
            case GO_MIMIRON_DOOR_2:
            case GO_MIMIRON_DOOR_3:
            case GO_MIMIRON_ELEVATOR:
                // Thorim
            case GO_DARK_IRON_PORTCULIS:
            case GO_RUNED_STONE_DOOR:
            case GO_THORIM_STONE_DOOR:
            case GO_LIGHTNING_FIELD:
            case GO_DOOR_LEVER:
                break;

                // Prison
            case GO_ANCIENT_GATE:
                if (m_auiEncounter[TYPE_MIMIRON] == DONE && m_auiEncounter[TYPE_HODIR] == DONE && m_auiEncounter[TYPE_THORIM] == DONE && m_auiEncounter[TYPE_FREYA] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_VEZAX_GATE:
                if (m_auiEncounter[TYPE_VEZAX] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_YOGG_GATE:
            case GO_BRAIN_DOOR_CHAMBER:
            case GO_BRAIN_DOOR_ICECROWN:
            case GO_BRAIN_DOOR_STORMWIND:
                break;

                // -----------------    Chests    -----------------
                // Kologarn
            case GO_CACHE_OF_LIVING_STONE_10:
            case GO_CACHE_OF_LIVING_STONE_25:

                // Hodir
            case GO_CACHE_OF_WINTER_10:
            case GO_CACHE_OF_WINTER_25:
            case GO_CACHE_OF_RARE_WINTER_10:
            case GO_CACHE_OF_RARE_WINTER_25:

                // Thorim
            case GO_CACHE_OF_STORMS_10:
            case GO_CACHE_OF_STORMS_25:
            case GO_CACHE_OF_STORMS_10_H:
            case GO_CACHE_OF_STORMS_25_H:

                // Mimiron
            case GO_CACHE_OF_INOV_10:
            case GO_CACHE_OF_INOV_25:
            case GO_CACHE_OF_INOV_10_H:
            case GO_CACHE_OF_INOV_25_H:

                // Alagon
            case GO_GIFT_OF_OBSERVER_10:
            case GO_GIFT_OF_OBSERVER_25:
                break;

            case GO_BROKEN_HARPOON:
                m_vBrokenHarpoonsGuids.push_back(pGo->GetObjectGuid());
                return;
            case GO_HARPOON_GUN_1:
            case GO_HARPOON_GUN_2:
            case GO_HARPOON_GUN_3:
            case GO_HARPOON_GUN_4:
                m_lRepairedHarpoonsGuids.push_back(pGo->GetObjectGuid());
                return;

            default:
                return;
            }
            m_mGoEntryGuidStore[pGo->GetEntry()] = pGo->GetObjectGuid();
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            switch (uiType)
            {
                // Siege of Ulduar
            case TYPE_LEVIATHAN:
                m_auiEncounter[uiType] = uiData;
                if (uiData != SPECIAL)
                    DoUseDoorOrButton(GO_SHIELD_WALL);
                if (uiData == IN_PROGRESS)
                {
                    // make sure that the Lightning door is closed when engaged in combat
                    if (GameObject* pDoor = GetSingleGameObjectFromStorage(GO_LIGHTNING_DOOR))
                    {
                        if (pDoor->GetGoState() != GO_STATE_READY)
                            DoUseDoorOrButton(GO_LIGHTNING_DOOR);
                    }

                    SetSpecialAchievementCriteria(TYPE_ACHIEV_SHUTOUT, true);
                }
                else if (uiData == DONE)
                {
                    DoUseDoorOrButton(GO_XT002_GATE);
                    DoUseDoorOrButton(GO_LIGHTNING_DOOR);
                }
                else if (uiData == FAIL)
                    DoCallLeviathanHelp();
                break;
            case TYPE_IGNIS:
                m_auiEncounter[uiType] = uiData;
                if (uiData == IN_PROGRESS)
                {
                    DoStartTimedAchievement(ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE, ACHIEV_START_IGNIS_ID);
                    SetSpecialAchievementCriteria(TYPE_ACHIEV_SHATTERED, false);
                }
                break;
            case TYPE_RAZORSCALE:
                if (uiData == IN_PROGRESS)
                    SetSpecialAchievementCriteria(TYPE_ACHIEV_QUICK_SHAVE, true);
                else if (uiData == FAIL)
                {
                    // reset the commander
                    if (Creature* pCommander = GetSingleCreatureFromStorage(NPC_EXPEDITION_COMMANDER))
                        pCommander->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);

                    // reset all creatures
                    for (GuidList::const_iterator itr = m_lDefendersGuids.begin(); itr != m_lDefendersGuids.end(); ++itr)
                    {
                        if (Creature* pDefender = instance->GetCreature(*itr))
                        {
                            if (!pDefender->IsAlive())
                                pDefender->Respawn();
                            else
                                pDefender->GetMotionMaster()->MoveTargetedHome();
                        }
                    }
                    for (GuidList::const_iterator itr = m_lEngineersGuids.begin(); itr != m_lEngineersGuids.end(); ++itr)
                    {
                        if (Creature* pEngineer = instance->GetCreature(*itr))
                        {
                            if (!pEngineer->IsAlive())
                                pEngineer->Respawn();
                            else
                                pEngineer->GetMotionMaster()->MoveTargetedHome();
                        }
                    }
                    for (GuidList::const_iterator itr = m_lTrappersGuids.begin(); itr != m_lTrappersGuids.end(); ++itr)
                    {
                        if (Creature* pTrapper = instance->GetCreature(*itr))
                        {
                            if (!pTrapper->IsAlive())
                                pTrapper->Respawn();
                            else
                                pTrapper->GetMotionMaster()->MoveTargetedHome();
                        }
                    }
                    for (GuidList::const_iterator itr = m_lHarpoonDummyGuids.begin(); itr != m_lHarpoonDummyGuids.end(); ++itr)
                    {
                        if (Creature* pHarpoon = instance->GetCreature(*itr))
                            pHarpoon->InterruptNonMeleeSpells(false);
                    }

                    // reset Harpoons: respawn the broken ones and despawn the repaired ones
                    for (GuidVector::const_iterator itr = m_vBrokenHarpoonsGuids.begin(); itr != m_vBrokenHarpoonsGuids.end(); ++itr)
                    {
                        if (GameObject* pHarpoon = instance->GetGameObject(*itr))
                        {
                            if (!pHarpoon->isSpawned())
                                pHarpoon->Respawn();
                        }
                    }
                    for (GuidList::const_iterator itr = m_lRepairedHarpoonsGuids.begin(); itr != m_lRepairedHarpoonsGuids.end(); ++itr)
                    {
                        if (GameObject* pHarpoon = instance->GetGameObject(*itr))
                        {
                            if (pHarpoon->isSpawned())
                                pHarpoon->SetLootState(GO_JUST_DEACTIVATED);
                        }
                    }
                }
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_XT002:
                m_auiEncounter[uiType] = uiData;
                DoUseDoorOrButton(GO_XT002_GATE);
                if (uiData == IN_PROGRESS)
                {
                    DoStartTimedAchievement(ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE, ACHIEV_START_XT002_ID);
                    SetSpecialAchievementCriteria(TYPE_ACHIEV_NERF_ENG, true);
                }
                break;

                // Antechamber of Ulduar
            case TYPE_ASSEMBLY:
                // Don't set the same encounter data twice
                if (uiData == m_auiEncounter[uiType])
                    return;
                m_auiEncounter[uiType] = uiData;
                // don't continue for encounter = special
                if (uiData == SPECIAL)
                    return;
                DoUseDoorOrButton(GO_IRON_ENTRANCE_DOOR);
                if (uiData == DONE)
                {
                    DoUseDoorOrButton(GO_ARCHIVUM_DOOR);

                    if (Player* pPlayer = GetPlayerInMap())
                    {
                        pPlayer->SummonCreature(NPC_BRANN_ARCHIVUM, afBrannArchivumSpawnPos[0], afBrannArchivumSpawnPos[1], afBrannArchivumSpawnPos[2], afBrannArchivumSpawnPos[3], TEMPSUMMON_DEAD_DESPAWN, 0, true);
                        pPlayer->SummonCreature(instance->IsRegularDifficulty() ? NPC_PROSPECTOR_DOREN : NPC_PROSPECTOR_DOREN_H, afProspectorSpawnPos[0], afProspectorSpawnPos[1], afProspectorSpawnPos[2], afProspectorSpawnPos[3], TEMPSUMMON_DEAD_DESPAWN, 0, true);
                    }
                }
                else if (uiData == IN_PROGRESS)
                {
                    SetSpecialAchievementCriteria(TYPE_ACHIEV_BRUNDIR, true);
                    SetSpecialAchievementCriteria(TYPE_ACHIEV_MOLGEIM, true);
                    SetSpecialAchievementCriteria(TYPE_ACHIEV_STEELBREAKER, true);
                    SetSpecialAchievementCriteria(TYPE_ACHIEV_STUNNED, true);
                }
                break;
            case TYPE_KOLOGARN:
                m_auiEncounter[uiType] = uiData;
                if (uiData == DONE)
                {
                    DoRespawnGameObject(instance->IsRegularDifficulty() ? GO_CACHE_OF_LIVING_STONE_10 : GO_CACHE_OF_LIVING_STONE_25, 30 * MINUTE);
                    DoUseDoorOrButton(GO_KOLOGARN_BRIDGE);
                }
                else if (uiData == IN_PROGRESS)
                {
                    SetSpecialAchievementCriteria(TYPE_ACHIEV_RUBBLE, false);
                    SetSpecialAchievementCriteria(TYPE_ACHIEV_DISARMED, false);
                    SetSpecialAchievementCriteria(TYPE_ACHIEV_LOOKS_KILL, true);
                    SetSpecialAchievementCriteria(TYPE_ACHIEV_OPEN_ARMS, true);
                }
                break;
            case TYPE_AURIAYA:
                m_auiEncounter[uiType] = uiData;
                if (uiData == IN_PROGRESS)
                {
                    SetSpecialAchievementCriteria(TYPE_ACHIEV_CAT_LADY, true);
                    SetSpecialAchievementCriteria(TYPE_ACHIEV_NINE_LIVES, false);
                }
                break;

                // Keepers of Ulduar
            case TYPE_MIMIRON:
                // Don't set the same encounter data twice
                if (uiData == m_auiEncounter[uiType])
                    return;
                m_auiEncounter[uiType] = uiData;
                DoUseDoorOrButton(GO_MIMIRON_DOOR_1);
                DoUseDoorOrButton(GO_MIMIRON_DOOR_2);
                DoUseDoorOrButton(GO_MIMIRON_DOOR_3);
                if (uiData == DONE)
                {
                    if (GetData(TYPE_MIMIRON_HARD) == DONE)
                        DoRespawnGameObject(instance->IsRegularDifficulty() ? GO_CACHE_OF_INOV_10_H : GO_CACHE_OF_INOV_25_H, 30 * MINUTE);
                    else
                        DoRespawnGameObject(instance->IsRegularDifficulty() ? GO_CACHE_OF_INOV_10 : GO_CACHE_OF_INOV_25, 30 * MINUTE);

                    SpawnFriendlyKeeper(NPC_KEEPER_MIMIRON);
                    DoOpenMadnessDoorIfCan();
                }
                else if (uiData == IN_PROGRESS)
                    DoToggleGameObjectFlags(GO_MIMIRON_BUTTON, GO_FLAG_NO_INTERACT, true);
                else if (uiData == FAIL)
                {
                    // reset objects
                    DoToggleGameObjectFlags(GO_MIMIRON_BUTTON, GO_FLAG_NO_INTERACT, false);

                    if (GameObject* pButton = GetSingleGameObjectFromStorage(GO_MIMIRON_BUTTON))
                        pButton->ResetDoorOrButton();
                    if (GameObject* pElevator = GetSingleGameObjectFromStorage(GO_MIMIRON_ELEVATOR))
                        pElevator->SetGoState(GO_STATE_ACTIVE);

                    // reset vehicles
                    if (Creature* pLeviathan = GetSingleCreatureFromStorage(NPC_LEVIATHAN_MK))
                    {
                        pLeviathan->RemoveSpellsCausingAura(SPELL_AURA_CONTROL_VEHICLE);
                        pLeviathan->AI()->EnterEvadeMode();
                    }
                    if (Creature* pVx001 = GetSingleCreatureFromStorage(NPC_VX001, true))
                    {
                        pVx001->RemoveSpellsCausingAura(SPELL_AURA_CONTROL_VEHICLE);
                        pVx001->ForcedDespawn(1000);
                    }
                    if (Creature* pAerial = GetSingleCreatureFromStorage(NPC_AERIAL_UNIT, true))
                    {
                        pAerial->RemoveSpellsCausingAura(SPELL_AURA_CONTROL_VEHICLE);
                        pAerial->ForcedDespawn(1000);
                    }
                    if (Creature* pMimiron = GetSingleCreatureFromStorage(NPC_MIMIRON))
                        pMimiron->AI()->EnterEvadeMode();
                    if (Creature* pComputer = GetSingleCreatureFromStorage(NPC_COMPUTER))
                        pComputer->AI()->EnterEvadeMode();

                    SetData(TYPE_MIMIRON_HARD, FAIL);
                }
                break;
            case TYPE_HODIR:
                m_auiEncounter[uiType] = uiData;
                DoUseDoorOrButton(GO_HODIR_ENTER);
                if (uiData == DONE)
                {
                    DoUseDoorOrButton(GO_HODIR_ICE_WALL);
                    DoUseDoorOrButton(GO_HODIR_EXIT);

                    DoToggleGameObjectFlags(instance->IsRegularDifficulty() ? GO_CACHE_OF_WINTER_10 : GO_CACHE_OF_WINTER_25, GO_FLAG_NO_INTERACT, false);
                    if (GetData(TYPE_HODIR_HARD) == DONE)
                        DoToggleGameObjectFlags(instance->IsRegularDifficulty() ? GO_CACHE_OF_RARE_WINTER_10 : GO_CACHE_OF_RARE_WINTER_25, GO_FLAG_NO_INTERACT, false);

                    SpawnFriendlyKeeper(NPC_KEEPER_HODIR);
                    DoOpenMadnessDoorIfCan();
                }
                else if (uiData == FAIL)
                {
                    if (GameObject* pChest = GetSingleGameObjectFromStorage(instance->IsRegularDifficulty() ? GO_CACHE_OF_RARE_WINTER_10 : GO_CACHE_OF_RARE_WINTER_25))
                        pChest->Respawn();

                    if (Player* pPlayer = GetPlayerInMap())
                        DoSpawnHodirNpcs(pPlayer);

                    SetData(TYPE_HODIR_HARD, FAIL);
                }
                else if (uiData == IN_PROGRESS)
                {
                    SetSpecialAchievementCriteria(TYPE_ACHIEV_CHEESE_FREEZE, true);
                    SetSpecialAchievementCriteria(TYPE_ACHIEV_COOL_FRIENDS, true);
                }
                break;
            case TYPE_THORIM:
                m_auiEncounter[uiType] = uiData;
                DoUseDoorOrButton(GO_LIGHTNING_FIELD);
                if (uiData == IN_PROGRESS)
                {
                    DoToggleGameObjectFlags(GO_DOOR_LEVER, GO_FLAG_NO_INTERACT, false);
                    SetSpecialAchievementCriteria(TYPE_ACHIEV_LIGHTNING, true);
                }
                else if (uiData == DONE)
                {
                    if (GetData(TYPE_THORIM_HARD) == DONE)
                    {
                        DoRespawnGameObject(instance->IsRegularDifficulty() ? GO_CACHE_OF_STORMS_10_H : GO_CACHE_OF_STORMS_25_H, 30 * MINUTE);
                        DoToggleGameObjectFlags(instance->IsRegularDifficulty() ? GO_CACHE_OF_STORMS_10_H : GO_CACHE_OF_STORMS_25_H, GO_FLAG_NO_INTERACT, false);
                    }
                    else
                    {
                        DoRespawnGameObject(instance->IsRegularDifficulty() ? GO_CACHE_OF_STORMS_10 : GO_CACHE_OF_STORMS_25, 30 * MINUTE);
                        DoToggleGameObjectFlags(instance->IsRegularDifficulty() ? GO_CACHE_OF_STORMS_10 : GO_CACHE_OF_STORMS_25, GO_FLAG_NO_INTERACT, false);
                    }

                    SpawnFriendlyKeeper(NPC_KEEPER_THORIM);
                    DoOpenMadnessDoorIfCan();
                }
                else if (uiData == FAIL)
                {
                    DoToggleGameObjectFlags(GO_DOOR_LEVER, GO_FLAG_NO_INTERACT, true);
                    if (GameObject* pDoor = GetSingleGameObjectFromStorage(GO_RUNED_STONE_DOOR))
                        pDoor->ResetDoorOrButton();
                    if (GameObject* pDoor = GetSingleGameObjectFromStorage(GO_THORIM_STONE_DOOR))
                        pDoor->ResetDoorOrButton();
                    if (Creature* pColossus = GetSingleCreatureFromStorage(NPC_RUNIC_COLOSSUS))
                    {
                        if (pColossus->IsAlive())
                            pColossus->AI()->EnterEvadeMode();
                    }

                    m_uiStairsSpawnTimer = 0;
                    m_uiSlayedArenaMobs = 0;
                }
                break;
            case TYPE_FREYA:
                m_auiEncounter[uiType] = uiData;
                if (uiData == DONE)
                {
                    // despawn elders which are still alive on event complete
                    if (Creature* pElder = GetSingleCreatureFromStorage(NPC_ELDER_BRIGHTLEAF))
                    {
                        if (pElder->IsAlive())
                            pElder->ForcedDespawn();
                    }
                    if (Creature* pElder = GetSingleCreatureFromStorage(NPC_ELDER_IRONBRACH))
                    {
                        if (pElder->IsAlive())
                            pElder->ForcedDespawn();
                    }
                    if (Creature* pElder = GetSingleCreatureFromStorage(NPC_ELDER_STONEBARK))
                    {
                        if (pElder->IsAlive())
                            pElder->ForcedDespawn();
                    }

                    SpawnFriendlyKeeper(NPC_KEEPER_FREYA);
                    DoOpenMadnessDoorIfCan();
                }
                break;

                // Ulduar Prison
            case TYPE_VEZAX:
                m_auiEncounter[uiType] = uiData;
                if (uiData == DONE)
                    DoUseDoorOrButton(GO_VEZAX_GATE);
                else if (uiData == IN_PROGRESS)
                    SetSpecialAchievementCriteria(TYPE_ACHIEV_SHADOWDODGER, true);
                break;
            case TYPE_YOGGSARON:
                // Don't set the same encounter data twice
                if (uiData == m_auiEncounter[uiType])
                    return;
                m_auiEncounter[uiType] = uiData;
                DoUseDoorOrButton(GO_YOGG_GATE);
                if (uiData == FAIL || uiData == DONE)
                {
                    // reset/cleanup encounter
                    for (GuidList::const_iterator itr = m_lOminousCloudsGuids.begin(); itr != m_lOminousCloudsGuids.end(); ++itr)
                    {
                        if (Creature* pCloud = instance->GetCreature(*itr))
                            pCloud->ForcedDespawn();
                    }

                    if (Creature* pVoice = GetSingleCreatureFromStorage(NPC_VOICE_OF_YOGG))
                    {
                        pVoice->CastSpell(pVoice, SPELL_CLEAR_INSANE, true);
                        pVoice->ForcedDespawn();
                    }

                    if (Creature* pSara = GetSingleCreatureFromStorage(NPC_SARA))
                        pSara->ForcedDespawn();
                    if (Creature* pBrain = GetSingleCreatureFromStorage(NPC_YOGG_BRAIN))
                        pBrain->ForcedDespawn();

                    // reset illusion doors
                    if (GameObject* pDoor = GetSingleGameObjectFromStorage(GO_BRAIN_DOOR_CHAMBER))
                        pDoor->ResetDoorOrButton();
                    if (GameObject* pDoor = GetSingleGameObjectFromStorage(GO_BRAIN_DOOR_ICECROWN))
                        pDoor->ResetDoorOrButton();
                    if (GameObject* pDoor = GetSingleGameObjectFromStorage(GO_BRAIN_DOOR_STORMWIND))
                        pDoor->ResetDoorOrButton();

                    // reset all helpers
                    for (uint8 i = 0; i < countof(m_aKeeperHelperLocs); ++i)
                    {
                        if (GetData(m_aKeeperHelperLocs[i].uiType == DONE))
                        {
                            if (Creature* pHelper = GetSingleCreatureFromStorage(m_aKeeperHelperLocs[i].uiEntry))
                            {
                                pHelper->AI()->EnterEvadeMode();
                                pHelper->CastSpell(pHelper, SPELL_KEEPER_ACTIVE, true);
                            }
                        }
                    }

                    // full reset only on fail
                    if (uiData == FAIL)
                        m_uiYoggResetTimer = 60000;
                }
                else if (uiData == IN_PROGRESS)
                {
                    DoStartTimedAchievement(ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE, ACHIEV_START_YOGG_ID);

                    SetSpecialAchievementCriteria(TYPE_ACHIEV_DRIVE_CRAZY, true);
                }
                break;

                // Celestial Planetarium
            case TYPE_ALGALON:
                m_auiEncounter[uiType] = uiData;
                if (uiData != SPECIAL)
                {
                    // environment gameobjects
                    DoUseDoorOrButton(GO_AZEROTH_GLOBE);
                    DoUseDoorOrButton(GO_UNIVERSE_FLOOR);
                    DoUseDoorOrButton(GO_UNIVERSE_FLOOR_COMBAT);
                    DoUseDoorOrButton(GO_CELESTIAL_DOOR_COMBAT);
                }
                if (uiData == DONE)
                {
                    DoUpdateWorldState(WORLD_STATE_TIMER, 0);
                    DoRespawnGameObject(instance->IsRegularDifficulty() ? GO_GIFT_OF_OBSERVER_10 : GO_GIFT_OF_OBSERVER_25, 30 * MINUTE);
                }
                else if (uiData == FAIL)
                {
                    // only despawn when time is over
                    if (GetData(TYPE_ALGALON_TIMER) == 0)
                    {
                        DoUpdateWorldState(WORLD_STATE_TIMER, 0);
                        if (Creature* pAlgalon = GetSingleCreatureFromStorage(NPC_ALGALON))
                            pAlgalon->AI()->SendAIEvent(AI_EVENT_CUSTOM_A, pAlgalon, pAlgalon);
                    }
                }
                break;
            case TYPE_ALGALON_TIMER:
                m_auiEncounter[uiType] = uiData;
                DoUpdateWorldState(WORLD_STATE_TIMER_COUNT, m_auiEncounter[uiType]);
                break;
            case TYPE_CHAMPION_FAILED:
                m_auiEncounter[uiType] = uiData;
                break;

                // Hard modes (not saved)
            case TYPE_LEVIATHAN_HARD:
                m_auiHardBoss[0] = uiData;
                return;
            case TYPE_XT002_HARD:
                m_auiHardBoss[1] = uiData;
                return;
            case TYPE_HODIR_HARD:
                m_auiHardBoss[2] = uiData;
                return;
            case TYPE_THORIM_HARD:
                m_auiHardBoss[3] = uiData;
                return;
            case TYPE_MIMIRON_HARD:
                m_auiHardBoss[4] = uiData;
                return;
            case TYPE_FREYA_HARD:
                m_auiHardBoss[5] = uiData;
                return;
            case TYPE_VEZAX_HARD:
                m_auiHardBoss[6] = uiData;
                return;
            case TYPE_YOGGSARON_HARD:
                m_auiHardBoss[7] = uiData;
                return;

                // Ulduar keepers
            case TYPE_KEEPER_HODIR:
                if (uiData == m_auiUlduarKeepers[0] || uiData != DONE)
                    return;
                SpawnKeeperHelper(NPC_HODIR_HELPER);
                m_auiUlduarKeepers[0] = uiData;
                break;
            case TYPE_KEEPER_THORIM:
                if (uiData == m_auiUlduarKeepers[1] || uiData != DONE)
                    return;
                SpawnKeeperHelper(NPC_THORIM_HELPER);
                m_auiUlduarKeepers[1] = uiData;
                break;
            case TYPE_KEEPER_FREYA:
                if (uiData == m_auiUlduarKeepers[2] || uiData != DONE)
                    return;
                SpawnKeeperHelper(NPC_FREYA_HELPER);
                m_auiUlduarKeepers[2] = uiData;
                break;
            case TYPE_KEEPER_MIMIRON:
                if (uiData == m_auiUlduarKeepers[3] || uiData != DONE)
                    return;
                SpawnKeeperHelper(NPC_MIMIRON_HELPER);
                m_auiUlduarKeepers[3] = uiData;
                break;

                // Ulduar towers
            case TYPE_TOWER_HODIR:
                if (m_auiUlduarTowers[0] == uiData)
                    return;
                if (uiData == FAIL)
                    DoUseDoorOrButton(GO_HODIR_CRYSTAL);
                m_auiUlduarTowers[0] = uiData;
                break;
            case TYPE_TOWER_THORIM:
                if (m_auiUlduarTowers[1] == uiData)
                    return;
                if (uiData == FAIL)
                    DoUseDoorOrButton(GO_THORIM_CRYSTAL);
                m_auiUlduarTowers[1] = uiData;
                break;
            case TYPE_TOWER_FREYA:
                if (m_auiUlduarTowers[2] == uiData)
                    return;
                if (uiData == FAIL)
                    DoUseDoorOrButton(GO_FREYA_CRYSTAL);
                m_auiUlduarTowers[2] = uiData;
                break;
            case TYPE_TOWER_MIMIRON:
                if (m_auiUlduarTowers[3] == uiData)
                    return;
                if (uiData == FAIL)
                    DoUseDoorOrButton(GO_MIMIRON_CRYSTAL);
                m_auiUlduarTowers[3] = uiData;
                break;

                // Other types - not saved
            case TYPE_LEVIATHAN_GAUNTLET:
                m_uiGauntletStatus = uiData;
                return;
            case TYPE_ACHIEV_SHADOWDODGER:
            case TYPE_ACHIEV_DRIVE_CRAZY:
            case TYPE_ACHIEV_BACK_NATURE:
            case TYPE_ACHIEV_LIGHTNING:
            case TYPE_ACHIEV_COOL_FRIENDS:
            case TYPE_ACHIEV_CHEESE_FREEZE:
            case TYPE_ACHIEV_DISARMED:
            case TYPE_ACHIEV_OPEN_ARMS:
            case TYPE_ACHIEV_LOOKS_KILL:
            case TYPE_ACHIEV_RUBBLE:
            case TYPE_ACHIEV_NERF_ENG:
            case TYPE_ACHIEV_SHUTOUT:
            case TYPE_ACHIEV_QUICK_SHAVE:
            case TYPE_ACHIEV_SHATTERED:
            case TYPE_ACHIEV_STUNNED:
            case TYPE_ACHIEV_MOLGEIM:
            case TYPE_ACHIEV_BRUNDIR:
            case TYPE_ACHIEV_STEELBREAKER:
            case TYPE_ACHIEV_NINE_LIVES:
            case TYPE_ACHIEV_CAT_LADY:
                SetSpecialAchievementCriteria(uiType - TYPE_ACHIEV_CAT_LADY, bool(uiData));
                return;
            case TYPE_DO_SHATTERED_EVENT:
                DoProcessShatteredEvent();
                return;
            case TYPE_DO_MOVE_DEFENDERS:
                for (GuidList::const_iterator itr = m_lDefendersGuids.begin(); itr != m_lDefendersGuids.end(); ++itr)
                {
                    if (Creature* pDefender = instance->GetCreature(*itr))
                    {
                        pDefender->CastSpell(pDefender, SPELL_THREAT, true);
                        pDefender->SetWalk(false);
                        pDefender->GetMotionMaster()->MoveWaypoint();
                    }
                }
                return;
            case TYPE_DATA_CURRENT_HARPOON:
                m_uiRazorCurrentHarpoon = uint8(uiData);
                return;
            case TYPE_DO_MOVE_TO_HARPOON:
                if (GameObject* pHarpoon = instance->GetGameObject(m_vBrokenHarpoonsGuids[m_uiRazorCurrentHarpoon]))
                {
                    ++m_uiRazorCurrentHarpoon;
                    float fX, fY, fZ;
                    uint8 uiIndex = 1;
                    for (GuidList::const_iterator itr = m_lEngineersGuids.begin(); itr != m_lEngineersGuids.end(); ++itr)
                    {
                        if (Creature* pEngineer = instance->GetCreature(*itr))
                        {
                            pHarpoon->GetNearPoint(pHarpoon, fX, fY, fZ, 0, INTERACTION_DISTANCE, M_PI_F / 4 * uiIndex);

                            // ToDo: maybe there should be some emotes here
                            pEngineer->SetWalk(false);
                            pEngineer->GetMotionMaster()->MovePoint(1, fX, fY, fZ);
                            ++uiIndex;
                        }
                    }
                }
                return;
            case TYPE_DO_HARPOONS_OK:
                for (GuidList::const_iterator itr = m_lEngineersGuids.begin(); itr != m_lEngineersGuids.end(); ++itr)
                {
                    if (Creature* pEngineer = instance->GetCreature(*itr))
                        pEngineer->GetMotionMaster()->MoveTargetedHome();
                }
                return;
            case TYPE_DO_TRAPPERS_MOVE:
                if (Creature* pController = GetSingleCreatureFromStorage(NPC_RAZORSCALE_CONTROLLER))
                {
                    float fX, fY, fZ;
                    uint8 uiIndex = 5;
                    uint32 uiSpeedRate = GetSingleCreatureFromStorage(NPC_RAZORSCALE)->GetSpeedRate(MOVE_RUN);
                    for (GuidList::const_iterator itr = m_lTrappersGuids.begin(); itr != m_lTrappersGuids.end(); ++itr)
                    {
                        if (Creature* pTrapper = instance->GetCreature(*itr))
                        {
                            pController->GetNearPoint(pController, fX, fY, fZ, 0, 50.0f, M_PI_F / 4 * uiIndex);

                            pTrapper->SetWalk(false);
                            uiSpeedRate = pTrapper->GetSpeedRate(MOVE_RUN);
                            pTrapper->SetSpeedRate(MOVE_RUN, SPEED_RATE_HELPERS);
                            pTrapper->GetMotionMaster()->MovePoint(1, fX, fY, fZ);
                            pTrapper->SetSpeedRate(MOVE_RUN, uiSpeedRate);
                            ++uiIndex;
                        }
                    }
                    // yell that Razor is grounded
                    if (Creature* pCommander = GetSingleCreatureFromStorage(NPC_EXPEDITION_COMMANDER))
                        DoScriptText(SAY_GROUNDED, pCommander);
                }
                return;
            case TYPE_DO_TRAPPERS_CAST:
                if (Creature* razorscale = GetSingleCreatureFromStorage(NPC_RAZORSCALE))
                {
                    for (GuidList::const_iterator itr = m_lTrappersGuids.begin(); itr != m_lTrappersGuids.end(); ++itr)
                    {
                        if (Creature* pTrapper = instance->GetCreature(*itr))
                            pTrapper->CastSpell(razorscale, uiData, false);
                    }
                }
                return;
            case TYPE_DO_TRAPPERS_EVADE:
                for (GuidList::const_iterator itr = m_lTrappersGuids.begin(); itr != m_lTrappersGuids.end(); ++itr)
                {
                    if (Creature* pTrapper = instance->GetCreature(*itr))
                        pTrapper->AI()->EnterEvadeMode();
                }
                return;
            case TYPE_DO_THORIM_SPAWN_PACK:
                if (Creature* thorim = GetSingleCreatureFromStorage(NPC_THORIM))
                {
                    std::vector<Creature*> vBunnies;
                    for (GuidList::const_iterator itr = m_lUpperBunniesGuids.begin(); itr != m_lUpperBunniesGuids.end(); ++itr)
                    {
                        if (Creature* pBunny = instance->GetCreature(*itr))
                            vBunnies.push_back(pBunny);
                    }
                    std::random_shuffle(vBunnies.begin(), vBunnies.end());

                    uint8 uiMaxCommoners = urand(6, 7);
                    if (uiMaxCommoners > vBunnies.size() - 1)
                        uiMaxCommoners = vBunnies.size();

                    for (uint8 i = 0; i < uiMaxCommoners; ++i)
                        thorim->SummonCreature(NPC_DARK_RUNE_COMMONER, vBunnies[i]->GetPositionX(), vBunnies[i]->GetPositionY(), vBunnies[i]->GetPositionZ(), 0, TEMPSUMMON_DEAD_DESPAWN, 0);
                }
                return;
            case TYPE_DATA_THORIM_SMASH_SIDE:
                m_bSmashSide = bool(uiData);
                return;
            case TYPE_DATA_THORIM_SMASHINDEX:
                m_uiThorimSmashIndex = uint8(uiData);
                return;
            case TYPE_DO_YOGG_DESPAWN_CLOUDS:
                for (GuidList::const_iterator itr = m_lOminousCloudsGuids.begin(); itr != m_lOminousCloudsGuids.end(); ++itr)
                {
                    if (Creature* pCloud = instance->GetCreature(*itr))
                        pCloud->ForcedDespawn();
                }
                return;
            }

            if (uiData == DONE || uiData == FAIL || uiData == SPECIAL || uiType == TYPE_ALGALON_TIMER)
            {
                OUT_SAVE_INST_DATA;

                // Save all encounters, hard bosses, keepers and teleporters
                std::ostringstream saveStream;
                saveStream << m_auiEncounter[0] << " " << m_auiEncounter[1] << " " << m_auiEncounter[2] << " "
                    << m_auiEncounter[3] << " " << m_auiEncounter[4] << " " << m_auiEncounter[5] << " "
                    << m_auiEncounter[6] << " " << m_auiEncounter[7] << " " << m_auiEncounter[8] << " "
                    << m_auiEncounter[9] << " " << m_auiEncounter[10] << " " << m_auiEncounter[11] << " "
                    << m_auiEncounter[12] << " " << m_auiEncounter[13] << " " << m_auiEncounter[14] << " "
                    << m_auiEncounter[15] << " " << m_auiUlduarKeepers[0] << " " << m_auiUlduarKeepers[1] << " "
                    << m_auiUlduarKeepers[2] << " " << m_auiUlduarKeepers[3] << " " << m_auiUlduarTowers[0] << " "
                    << m_auiUlduarTowers[1] << " " << m_auiUlduarTowers[2] << " " << m_auiUlduarTowers[3];

                m_strInstData = saveStream.str();

                SaveToDB();
                OUT_SAVE_INST_DATA_COMPLETE;
            }
        }

        uint32 GetData(uint32 uiType) const override
        {
            switch (uiType)
            {
            case TYPE_LEVIATHAN:
                return m_auiEncounter[0];
            case TYPE_IGNIS:
                return m_auiEncounter[1];
            case TYPE_RAZORSCALE:
                return m_auiEncounter[2];
            case TYPE_XT002:
                return m_auiEncounter[3];
            case TYPE_ASSEMBLY:
                return m_auiEncounter[4];
            case TYPE_KOLOGARN:
                return m_auiEncounter[5];
            case TYPE_AURIAYA:
                return m_auiEncounter[6];
            case TYPE_MIMIRON:
                return m_auiEncounter[7];
            case TYPE_HODIR:
                return m_auiEncounter[8];
            case TYPE_THORIM:
                return m_auiEncounter[9];
            case TYPE_FREYA:
                return m_auiEncounter[10];
            case TYPE_VEZAX:
                return m_auiEncounter[11];
            case TYPE_YOGGSARON:
                return m_auiEncounter[12];
            case TYPE_ALGALON:
                return m_auiEncounter[13];
            case TYPE_ALGALON_TIMER:
                return m_auiEncounter[14];
            case TYPE_CHAMPION_FAILED:
                return m_auiEncounter[15];

                // Hard modes
            case TYPE_LEVIATHAN_HARD:
                return m_auiHardBoss[0];
            case TYPE_XT002_HARD:
                return m_auiHardBoss[1];
            case TYPE_HODIR_HARD:
                return m_auiHardBoss[2];
            case TYPE_THORIM_HARD:
                return m_auiHardBoss[3];
            case TYPE_MIMIRON_HARD:
                return m_auiHardBoss[4];
            case TYPE_FREYA_HARD:
                return m_auiHardBoss[5];
            case TYPE_VEZAX_HARD:
                return m_auiHardBoss[6];
            case TYPE_YOGGSARON_HARD:
                return m_auiHardBoss[7];

                // Ulduar Keepers
            case TYPE_KEEPER_HODIR:
                return m_auiUlduarKeepers[0];
            case TYPE_KEEPER_THORIM:
                return m_auiUlduarKeepers[1];
            case TYPE_KEEPER_FREYA:
                return m_auiUlduarKeepers[2];
            case TYPE_KEEPER_MIMIRON:
                return m_auiUlduarKeepers[3];

                // Ulduar Towers
            case TYPE_TOWER_HODIR:
                return m_auiUlduarTowers[0];
            case TYPE_TOWER_THORIM:
                return m_auiUlduarTowers[1];
            case TYPE_TOWER_FREYA:
                return m_auiUlduarTowers[2];
            case TYPE_TOWER_MIMIRON:
                return m_auiUlduarTowers[3];

            case TYPE_LEVIATHAN_GAUNTLET:
                return m_uiGauntletStatus;
            case TYPE_DATA_CURRENT_HARPOON:
                return m_uiRazorCurrentHarpoon;
            }

            return 0;
        }

        uint64 GetData64(uint32 uiType) const override
        {
            switch (uiType)
            {
            case DATA64_VEZAX_ANIMUS:
                return m_animusVezaxBunnyGuid.GetRawValue();
            case DATA64_VEZAX_VAPOR:
                return m_vaporVezaxBunnyGuid.GetRawValue();
            case DATA64_KOLO_STALKER_RIGHT:
                return m_rightKoloStalkerGuid.GetRawValue();
            case DATA64_KOLO_STALKER_LEFT:
                return m_leftKoloStalkerGuid.GetRawValue();
            case DATA64_BROKEN_HARPOON:
                if (m_uiRazorCurrentHarpoon > 0)
                    return m_vBrokenHarpoonsGuids[m_uiRazorCurrentHarpoon - 1].GetRawValue();
            case DATA64_THORIM_RANDOM_ORB:
                if (!m_lUpperThunderOrbsGuids.empty())
                {
                    GuidList::const_iterator iter = m_lUpperThunderOrbsGuids.begin();
                    advance(iter, urand(0, m_lUpperThunderOrbsGuids.size() - 1));

                    return (*iter).GetRawValue();
                }
                break;
            case DATA64_RANDOM_UPPER_BUNNY:
                if (!m_lUpperBunniesGuids.empty())
                {
                    GuidList::const_iterator iter = m_lUpperBunniesGuids.begin();
                    advance(iter, urand(0, m_lUpperBunniesGuids.size() - 1));

                    return (*iter).GetRawValue();
                }
                break;
            case DATA64_THORIM_BUNNY:
                if (!m_lThorimBunniesGuids.empty())
                {
                    if (Creature* pSource = instance->GetCreature(m_thorimCreature))
                    {
                        std::list<Creature*> lBunnies;
                        for (GuidList::const_iterator itr = m_lThorimBunniesGuids.begin(); itr != m_lThorimBunniesGuids.end(); ++itr)
                        {
                            if (Creature* pBunny = instance->GetCreature(*itr))
                                lBunnies.push_back(pBunny);
                        }

                        lBunnies.sort(ObjectDistanceOrder(pSource));
                        return lBunnies.front()->GetObjectGuid().GetRawValue();
                    }
                }
                break;
            case DATA64_XT_TOY_PILE:
                return m_vToyPileGuidVector[urand(0, m_vToyPileGuidVector.size() - 1)].GetRawValue();
            default:
                break;
            }
            return 0;
        }

        void SetData64(uint32 uiType, uint64 uiData) override
        {
            switch (uiType)
            {
            case DATA64_THORIM_TRIGGERED_BY:
                DoSpawnThorimNpcs(ObjectGuid(uiData));
                break;
            case DATA64_THORIM_BUNNY:
                m_thorimCreature = ObjectGuid(uiData);
                break;
            case DATA64_THORIM_SMASH_CASTER:
                if (Creature* m_creature = instance->GetCreature(ObjectGuid(uiData)))
                {
                    GuidList* m_lCurrentSmashBunnies = m_bSmashSide ? &m_lLeftHandBunniesGuids : &m_lRightHandBunniesGuids;
                    for (GuidList::const_iterator itr = m_lCurrentSmashBunnies->begin(); itr != m_lCurrentSmashBunnies->end(); ++itr)
                    {
                        if (Creature* pBunny = m_creature->GetMap()->GetCreature(*itr))
                        {
                            // use 12 and 16 as multipliers in order to get the perfect combination
                            if (pBunny->GetPositionY() > m_creature->GetPositionY() + 12 * m_uiThorimSmashIndex &&
                                pBunny->GetPositionY() < m_creature->GetPositionY() + 16 * m_uiThorimSmashIndex)
                                pBunny->CastSpell(pBunny, SPELL_RUNIC_SMASH, false);
                        }
                    }
                }
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
                >> m_auiEncounter[4] >> m_auiEncounter[5] >> m_auiEncounter[6] >> m_auiEncounter[7]
                >> m_auiEncounter[8] >> m_auiEncounter[9] >> m_auiEncounter[10] >> m_auiEncounter[11]
                >> m_auiEncounter[12] >> m_auiEncounter[13] >> m_auiEncounter[14] >> m_auiEncounter[15]
                >> m_auiUlduarKeepers[0] >> m_auiUlduarKeepers[1] >> m_auiUlduarKeepers[2] >> m_auiUlduarKeepers[3]
                >> m_auiUlduarTowers[0] >> m_auiUlduarTowers[1] >> m_auiUlduarTowers[2] >> m_auiUlduarTowers[3];

            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            {
                if (m_auiEncounter[i] == IN_PROGRESS)
                    m_auiEncounter[i] = NOT_STARTED;
            }

            OUT_LOAD_INST_DATA_COMPLETE;
        }

        bool CheckAchievementCriteriaMeet(uint32 uiCriteriaId, Player const* pSource, Unit const* pTarget, uint32 uiMiscValue1 /* = 0*/) const override
        {
            switch (uiCriteriaId)
            {
            case ACHIEV_CRIT_SARONITE_N:
            case ACHIEV_CRIT_SARONITE_H:
                return GetData(TYPE_VEZAX_HARD) == DONE;
            case ACHIEV_CRIT_SHADOWDODGER_N:
            case ACHIEV_CRIT_SHADOWDODGER_H:
                return m_abAchievCriteria[TYPE_ACHIEV_SHADOWDODGER - TYPE_ACHIEV_CAT_LADY];
            case ACHIEV_CRIT_CAT_LADY_N:
            case ACHIEV_CRIT_CAT_LADY_H:
                return m_abAchievCriteria[TYPE_ACHIEV_CAT_LADY - TYPE_ACHIEV_CAT_LADY];
            case ACHIEV_CRIT_NINE_LIVES_N:
            case ACHIEV_CRIT_NINE_LIVES_H:
                return m_abAchievCriteria[TYPE_ACHIEV_NINE_LIVES - TYPE_ACHIEV_CAT_LADY];
            case ACHIEV_CRIT_BRUNDIR_N:
            case ACHIEV_CRIT_BRUNDIR_H:
                if (GetData(TYPE_ASSEMBLY) == SPECIAL)
                    return m_abAchievCriteria[TYPE_ACHIEV_BRUNDIR - TYPE_ACHIEV_CAT_LADY];
            case ACHIEV_CRIT_MOLGEIM_N:
            case ACHIEV_CRIT_MOLGEIM_H:
                if (GetData(TYPE_ASSEMBLY) == SPECIAL)
                    return m_abAchievCriteria[TYPE_ACHIEV_MOLGEIM - TYPE_ACHIEV_CAT_LADY];
            case ACHIEV_CRIT_STEELBREAKER_N:
            case ACHIEV_CRIT_STEELBREAKER_H:
                if (GetData(TYPE_ASSEMBLY) == SPECIAL)
                    return m_abAchievCriteria[TYPE_ACHIEV_STEELBREAKER - TYPE_ACHIEV_CAT_LADY];
            case ACHIEV_CRIT_STUNNED_BRUND_N:
            case ACHIEV_CRIT_STUNNED_STEEL_N:
            case ACHIEV_CRIT_STUNNED_MOLG_N:
            case ACHIEV_CRIT_STUNNED_BRUND_H:
            case ACHIEV_CRIT_STUNNED_STEEL_H:
            case ACHIEV_CRIT_STUNNED_MOLG_H:
                if (GetData(TYPE_ASSEMBLY) == SPECIAL)
                    return m_abAchievCriteria[TYPE_ACHIEV_STUNNED - TYPE_ACHIEV_CAT_LADY];
            case ACHIEV_CRIT_SHATTERED_N:
            case ACHIEV_CRIT_SHATTERED_H:
                return m_abAchievCriteria[TYPE_ACHIEV_SHATTERED - TYPE_ACHIEV_CAT_LADY];
            case ACHIEV_CRIT_HEARTBREAKER_N:
            case ACHIEV_CRIT_HEARTBREAKER_H:
                return GetData(TYPE_XT002_HARD) == DONE;
            case ACHIEV_CRIT_QUICK_SHAVE_N:
            case ACHIEV_CRIT_QUICK_SHAVE_H:
                return m_abAchievCriteria[TYPE_ACHIEV_QUICK_SHAVE - TYPE_ACHIEV_CAT_LADY];
            case ACHIEV_CRIT_SHUTOUT_N:
            case ACHIEV_CRIT_SHUTOUT_H:
                return m_abAchievCriteria[TYPE_ACHIEV_SHUTOUT - TYPE_ACHIEV_CAT_LADY];
            case ACHIEV_CRIT_ORB_BOMB_N:
            case ACHIEV_CRIT_ORB_BOMB_H:
                return GetData(TYPE_LEVIATHAN_HARD) >= 1;
            case ACHIEV_CRIT_ORB_DEV_N:
            case ACHIEV_CRIT_ORB_DEV_H:
                return GetData(TYPE_LEVIATHAN_HARD) >= 2;
            case ACHIEV_CRIT_ORB_NUKED_N:
            case ACHIEV_CRIT_ORB_NUKED_H:
                return GetData(TYPE_LEVIATHAN_HARD) >= 3;
            case ACHIEV_CRIT_ORBITUARY_N:
            case ACHIEV_CRIT_ORBITUARY_H:
                return GetData(TYPE_LEVIATHAN_HARD) == 4;
            case ACHIEV_CRIT_NERF_ENG_N:
            case ACHIEV_CRIT_NERF_ENG_H:
                return m_abAchievCriteria[TYPE_ACHIEV_NERF_ENG - TYPE_ACHIEV_CAT_LADY];
            case ACHIEV_CRIT_RUBBLE_ROLL_N:
            case ACHIEV_CRIT_RUBBLE_ROLL_H:
                return m_abAchievCriteria[TYPE_ACHIEV_RUBBLE - TYPE_ACHIEV_CAT_LADY];
            case ACHIEV_CRIT_LOOKS_KILL_N:
            case ACHIEV_CRIT_LOOKS_KILL_H:
                return m_abAchievCriteria[TYPE_ACHIEV_LOOKS_KILL - TYPE_ACHIEV_CAT_LADY];
            case ACHIEV_CRIT_OPEN_ARMS_N:
            case ACHIEV_CRIT_OPEN_ARMS_H:
                return m_abAchievCriteria[TYPE_ACHIEV_OPEN_ARMS - TYPE_ACHIEV_CAT_LADY];
            case ACHIEV_CRIT_DISARMED_N:
            case ACHIEV_CRIT_DISARMED_H:
                return m_abAchievCriteria[TYPE_ACHIEV_DISARMED - TYPE_ACHIEV_CAT_LADY];
            case ACHIEV_CRIT_RARE_CACHE_N:
            case ACHIEV_CRIT_RARE_CACHE_H:
                return GetData(TYPE_HODIR_HARD) == DONE;
            case ACHIEV_CRIT_CHEESE_N:
            case ACHIEV_CRIT_CHEESE_H:
                return m_abAchievCriteria[TYPE_ACHIEV_CHEESE_FREEZE - TYPE_ACHIEV_CAT_LADY];
            case ACHIEV_CRIT_COOL_FRIENDS_N:
            case ACHIEV_CRIT_COOL_FRIENDS_H:
                return m_abAchievCriteria[TYPE_ACHIEV_COOL_FRIENDS - TYPE_ACHIEV_CAT_LADY];
            case ACHIEV_CRIT_LOSE_ILLUSION_N:
            case ACHIEV_CRIT_LOSE_ILLUSION_H:
                return GetData(TYPE_THORIM_HARD) == DONE;
            case ACHIEV_CRIT_LIGHTNING_N:
            case ACHIEV_CRIT_LIGHTNING_H:
                return m_abAchievCriteria[TYPE_ACHIEV_LIGHTNING - TYPE_ACHIEV_CAT_LADY];
            case ACHIEV_CRIT_BACK_NATURE_N:
            case ACHIEV_CRIT_BACK_NATURE_H:
                return m_abAchievCriteria[TYPE_ACHIEV_BACK_NATURE - TYPE_ACHIEV_CAT_LADY];
            case ACHIEV_CRIT_KNOCK_1_N:
            case ACHIEV_CRIT_KNOCK_1_H:
                return GetData(TYPE_FREYA_HARD) >= 1;
            case ACHIEV_CRIT_KNOCK_2_N:
            case ACHIEV_CRIT_KNOCK_2_H:
                return GetData(TYPE_FREYA_HARD) >= 2;
            case ACHIEV_CRIT_KNOCK_3_N:
            case ACHIEV_CRIT_KNOCK_3_H:
                return GetData(TYPE_FREYA_HARD) == 3;
            case ACHIEV_CRIT_FIREFIGHTER_N:
            case ACHIEV_CRIT_FIREFIGHTER_H:
                return GetData(TYPE_MIMIRON_HARD) == DONE;
            case ACHIEV_CRIT_THREE_LIGHTS_N:
            case ACHIEV_CRIT_THREE_LIGHTS_H:
                return GetData(TYPE_YOGGSARON_HARD) <= 3;
            case ACHIEV_CRIT_TWO_LIGHTS_N:
            case ACHIEV_CRIT_TWO_LIGHTS_H:
                return GetData(TYPE_YOGGSARON_HARD) <= 2;
            case ACHIEV_CRIT_ONE_LIGHT_N:
            case ACHIEV_CRIT_ONE_LIGHT_H:
                return GetData(TYPE_YOGGSARON_HARD) <= 1;
            case ACHIEV_CRIT_ALONE_DARK_N:
            case ACHIEV_CRIT_ALONE_DARK_H:
                return GetData(TYPE_YOGGSARON_HARD) == 0;
            case ACHIEV_CRIT_DRIVE_CRAZY_N:
            case ACHIEV_CRIT_DRIVE_CRAZY_H:
                return m_abAchievCriteria[TYPE_ACHIEV_DRIVE_CRAZY - TYPE_ACHIEV_CAT_LADY];
                // Champion / Conquerer of Ulduar
            case ACHIEV_CRIT_CHAMP_LEVI:
            case ACHIEV_CRIT_CHAMP_RAZOR:
            case ACHIEV_CRIT_CHAMP_XT:
            case ACHIEV_CRIT_CHAMP_IGNIS:
            case ACHIEV_CRIT_CHAMP_MIMIRON:
            case ACHIEV_CRIT_CHAMP_KOLO:
            case ACHIEV_CRIT_CHAMP_VEZAX:
            case ACHIEV_CRIT_CHAMP_YOGG:
            case ACHIEV_CRIT_CHAMP_AURIAYA:
            case ACHIEV_CRIT_CHAMP_THORIM:
            case ACHIEV_CRIT_CHAMP_HODIR:
            case ACHIEV_CRIT_CHAMP_FREYA:
            case ACHIEV_CRIT_CHAMP_COUNCIL:
            case ACHIEV_CRIT_CONQ_LEVI:
            case ACHIEV_CRIT_CONQ_RAZOR:
            case ACHIEV_CRIT_CONQ_XT:
            case ACHIEV_CRIT_CONQ_IGNIS:
            case ACHIEV_CRIT_CONQ_KOLO:
            case ACHIEV_CRIT_CONQ_MIMIRON:
            case ACHIEV_CRIT_CONQ_VEZAX:
            case ACHIEV_CRIT_CONQ_AURIAYA:
            case ACHIEV_CRIT_CONQ_YOGG:
            case ACHIEV_CRIT_CONQ_THORIM:
            case ACHIEV_CRIT_CONQ_FREYA:
            case ACHIEV_CRIT_CONQ_COUNCIL:
            case ACHIEV_CRIT_CONQ_HODIR:
            {
                // First, check if all bosses are killed (except the last encounter)
                uint8 uiEncounterDone = 0;
                for (uint8 i = 0; i < TYPE_YOGGSARON; ++i)
                if (m_auiEncounter[i] == DONE)
                    ++uiEncounterDone;

                return uiEncounterDone >= 13 && GetData(TYPE_CHAMPION_FAILED) != DONE;
            }

            default:
                return false;
            }
        }

        bool CheckConditionCriteriaMeet(Player const* pPlayer, uint32 uiInstanceConditionId, WorldObject const* pConditionSource, uint32 conditionSourceType) const override
        {
            switch (uiInstanceConditionId)
            {
            case INSTANCE_CONDITION_ID_NORMAL_MODE:
            case INSTANCE_CONDITION_ID_HARD_MODE:
            case INSTANCE_CONDITION_ID_HARD_MODE_2:
            case INSTANCE_CONDITION_ID_HARD_MODE_3:
            case INSTANCE_CONDITION_ID_HARD_MODE_4:
            {
                if (!pConditionSource)
                    break;

                uint32 uiCondId = 0;
                switch (pConditionSource->GetEntry())
                {
                case NPC_LEVIATHAN:
                    uiCondId = GetData(TYPE_LEVIATHAN_HARD);
                    break;
                case NPC_XT002:
                    if (GetData(TYPE_XT002_HARD) == DONE)
                        uiCondId = 1;
                    break;
                case NPC_VEZAX:
                    if (GetData(TYPE_VEZAX_HARD) == DONE)
                        uiCondId = 1;
                    break;
                case NPC_YOGGSARON:
                    uiCondId = 4 - GetData(TYPE_YOGGSARON_HARD);
                    break;
                }

                return uiCondId == uiInstanceConditionId;
            }
            case INSTANCE_CONDITION_ID_ULDUAR:
                if (!pConditionSource)
                    break;

                // handle vehicle spell clicks - are available only after the gauntlet was started by gossip or when Leviathan is active
                return GetData(TYPE_LEVIATHAN_GAUNTLET) == IN_PROGRESS || GetData(TYPE_LEVIATHAN) == SPECIAL || GetData(TYPE_LEVIATHAN) == FAIL;
            }

            script_error_log("instance_ulduar::CheckConditionCriteriaMeet called with unsupported Id %u. Called with param plr %s, src %s, condition source type %u",
                uiInstanceConditionId, pPlayer ? pPlayer->GetGuidStr().c_str() : "NULL", pConditionSource ? pConditionSource->GetGuidStr().c_str() : "NULL", conditionSourceType);
            return false;
        }

        void Update(uint32 uiDiff) override
        {
            DialogueUpdate(uiDiff);

            if (GetData(TYPE_IGNIS) == IN_PROGRESS)
            {
                if (m_uiShatterAchievTimer)
                {
                    // Just set the timer to 0 when it expires
                    if (m_uiShatterAchievTimer <= uiDiff)
                        m_uiShatterAchievTimer = 0;
                    else
                        m_uiShatterAchievTimer -= uiDiff;
                }
            }

            if (GetData(TYPE_ALGALON_TIMER))
            {
                if (m_uiAlgalonTimer <= uiDiff)
                {
                    --m_auiEncounter[TYPE_ALGALON_TIMER];
                    SetData(TYPE_ALGALON_TIMER, m_auiEncounter[TYPE_ALGALON_TIMER]);
                    m_uiAlgalonTimer = MINUTE * IN_MILLISECONDS;

                    if (m_auiEncounter[TYPE_ALGALON_TIMER] == 0)
                        SetData(TYPE_ALGALON, FAIL);
                }
                else
                    m_uiAlgalonTimer -= uiDiff;
            }

            if (m_uiYoggResetTimer)
            {
                if (m_uiYoggResetTimer <= uiDiff)
                {
                    // reset encounter
                    for (GuidList::const_iterator itr = m_lOminousCloudsGuids.begin(); itr != m_lOminousCloudsGuids.end(); ++itr)
                    {
                        if (Creature* pCloud = instance->GetCreature(*itr))
                            pCloud->Respawn();
                    }

                    if (Creature* pVoice = GetSingleCreatureFromStorage(NPC_VOICE_OF_YOGG))
                        pVoice->Respawn();
                    if (Creature* pSara = GetSingleCreatureFromStorage(NPC_SARA))
                        pSara->Respawn();
                    if (Creature* pBrain = GetSingleCreatureFromStorage(NPC_YOGG_BRAIN))
                        pBrain->Respawn();

                    m_uiYoggResetTimer = 0;
                }
                else
                    m_uiYoggResetTimer -= uiDiff;
            }

            if (m_uiStairsSpawnTimer)
            {
                if (m_uiStairsSpawnTimer <= uiDiff)
                {
                    // Note: this part involves a lot of guesswork
                    // These are the stairs npcs which are summoned before engaging the Rune Giant; both npcs have waypoint movement
                    if (Creature* pGiant = GetSingleCreatureFromStorage(NPC_RUNE_GIANT))
                    {
                        if (urand(0, 1))
                            pGiant->SummonCreature(NPC_HONOR_GUARD_STAIRS, 2101.2f, -434.135f, 438.331f, 0, TEMPSUMMON_DEAD_DESPAWN, 0);
                        else
                            pGiant->SummonCreature(NPC_RUNE_ACOLYTE_STAIRS, 2100.41f, -446.712f, 438.331f, 0, TEMPSUMMON_DEAD_DESPAWN, 0);
                    }
                    m_uiStairsSpawnTimer = urand(20000, 30000);
                }
                else
                    m_uiStairsSpawnTimer -= uiDiff;
            }
        }

    private:
        void SetSpecialAchievementCriteria(uint32 uiType, bool bIsMet)
        {
            if (uiType < MAX_SPECIAL_ACHIEV_CRITS)
                m_abAchievCriteria[uiType] = bIsMet;
        }

        void DoProcessShatteredEvent()
        {
            // If timer is already running set achiev criteria to true, else start the timer
            if (m_uiShatterAchievTimer)
                SetSpecialAchievementCriteria(TYPE_ACHIEV_SHATTERED, true);
            else
                m_uiShatterAchievTimer = 5000;
        }

        void DoSpawnThorimNpcs(ObjectGuid plGuid)
        {
            if (GetData(TYPE_THORIM) != DONE)
            {
                if (Player* pSummoner = instance->GetPlayer(plGuid))
                for (uint8 i = 0; i < countof(afThorimSpawns); ++i)
                    pSummoner->SummonCreature(pSummoner->GetTeam() == ALLIANCE ? afThorimSpawns[i].uiAllyEntry : afThorimSpawns[i].uiHordeEntry, afThorimSpawns[i].fX, afThorimSpawns[i].fY, afThorimSpawns[i].fZ, afThorimSpawns[i].fO, TEMPSUMMON_DEAD_DESPAWN, 0, true);
            }
        }

        void JustDidDialogueStep(int32 iEntry) override
        {
            switch (iEntry)
            {
            case SAY_PRE_LEVIATHAN_1:
            case SAY_PRE_LEVIATHAN_2:
            case SAY_PRE_LEVIATHAN_3:
                DoOrSimulateScriptTextForThisInstance(iEntry, NPC_BRONZEBEARD_RADIO);
                break;
            case NPC_LEVIATHAN:
                // move the leviathan in the arena
                if (Creature* pLeviathan = GetSingleCreatureFromStorage(NPC_LEVIATHAN))
                {
                    // the boss has increased speed for this move; handled as custom
                    float fSpeedRate = pLeviathan->GetSpeedRate(MOVE_RUN);
                    pLeviathan->SetWalk(false);
                    pLeviathan->SetSpeedRate(MOVE_RUN, 5);
                    pLeviathan->GetMotionMaster()->MovePoint(1, afLeviathanMovePos[0], afLeviathanMovePos[1], afLeviathanMovePos[2]);
                    pLeviathan->SetSpeedRate(MOVE_RUN, fSpeedRate);

                    // modify respawn / home position to the center of arena
                    pLeviathan->SetRespawnCoord(afLeviathanMovePos[0], afLeviathanMovePos[1], afLeviathanMovePos[2], afLeviathanMovePos[3]);
                }

                // Note: starting 4.x this gate is a GO 33 and it's destroyed at this point
                DoUseDoorOrButton(GO_LEVIATHAN_GATE);
                break;
            }
        }

        // Spawn the friendly keepers in the central chamber
        void SpawnFriendlyKeeper(uint32 uiWho)
        {
            Player* pPlayer = GetPlayerInMap();
            if (!pPlayer)
                return;

            switch (uiWho)
            {
            case NPC_KEEPER_MIMIRON: pPlayer->SummonCreature(uiWho, m_aKeepersSpawnLocs[1].fX, m_aKeepersSpawnLocs[1].fY, m_aKeepersSpawnLocs[1].fZ, m_aKeepersSpawnLocs[1].fO, TEMPSUMMON_CORPSE_DESPAWN, 0, true); break;
            case NPC_KEEPER_HODIR:   pPlayer->SummonCreature(uiWho, m_aKeepersSpawnLocs[2].fX, m_aKeepersSpawnLocs[2].fY, m_aKeepersSpawnLocs[2].fZ, m_aKeepersSpawnLocs[2].fO, TEMPSUMMON_CORPSE_DESPAWN, 0, true); break;
            case NPC_KEEPER_THORIM:  pPlayer->SummonCreature(uiWho, m_aKeepersSpawnLocs[3].fX, m_aKeepersSpawnLocs[3].fY, m_aKeepersSpawnLocs[3].fZ, m_aKeepersSpawnLocs[3].fO, TEMPSUMMON_CORPSE_DESPAWN, 0, true); break;
            case NPC_KEEPER_FREYA:   pPlayer->SummonCreature(uiWho, m_aKeepersSpawnLocs[0].fX, m_aKeepersSpawnLocs[0].fY, m_aKeepersSpawnLocs[0].fZ, m_aKeepersSpawnLocs[0].fO, TEMPSUMMON_CORPSE_DESPAWN, 0, true); break;
            }
        }

        // Spawn the keeper helpers for Yogg-Saron
        void SpawnKeeperHelper(uint32 uiWho)
        {
            Player* pPlayer = GetPlayerInMap();
            if (!pPlayer)
                return;

            switch (uiWho)
            {
            case NPC_MIMIRON_HELPER:
                if (Creature* pKeeper = pPlayer->SummonCreature(uiWho, m_aKeeperHelperLocs[1].fX, m_aKeeperHelperLocs[1].fY, m_aKeeperHelperLocs[1].fZ, m_aKeeperHelperLocs[1].fO, TEMPSUMMON_CORPSE_DESPAWN, 0, true))
                {
                    DoScriptText(m_aKeeperHelperLocs[1].iText, pKeeper);
                    pKeeper->CastSpell(pKeeper, SPELL_KEEPER_ACTIVE, false);
                }
                break;
            case NPC_HODIR_HELPER:
                if (Creature* pKeeper = pPlayer->SummonCreature(uiWho, m_aKeeperHelperLocs[2].fX, m_aKeeperHelperLocs[2].fY, m_aKeeperHelperLocs[2].fZ, m_aKeeperHelperLocs[2].fO, TEMPSUMMON_CORPSE_DESPAWN, 0, true))
                {
                    DoScriptText(m_aKeeperHelperLocs[2].iText, pKeeper);
                    pKeeper->CastSpell(pKeeper, SPELL_KEEPER_ACTIVE, false);
                }
                break;
            case NPC_THORIM_HELPER:
                if (Creature* pKeeper = pPlayer->SummonCreature(uiWho, m_aKeeperHelperLocs[3].fX, m_aKeeperHelperLocs[3].fY, m_aKeeperHelperLocs[3].fZ, m_aKeeperHelperLocs[3].fO, TEMPSUMMON_CORPSE_DESPAWN, 0, true))
                {
                    DoScriptText(m_aKeeperHelperLocs[3].iText, pKeeper);
                    pKeeper->CastSpell(pKeeper, SPELL_KEEPER_ACTIVE, false);
                }
                break;
            case NPC_FREYA_HELPER:
                if (Creature* pKeeper = pPlayer->SummonCreature(uiWho, m_aKeeperHelperLocs[0].fX, m_aKeeperHelperLocs[0].fY, m_aKeeperHelperLocs[0].fZ, m_aKeeperHelperLocs[0].fO, TEMPSUMMON_CORPSE_DESPAWN, 0, true))
                {
                    DoScriptText(m_aKeeperHelperLocs[0].iText, pKeeper);
                    pKeeper->CastSpell(pKeeper, SPELL_KEEPER_ACTIVE, false);
                }
                break;
            }
        }

        void DoSpawnHodirNpcs(Player* pSummoner)
        {
            if (GetData(TYPE_HODIR) != DONE)
            {
                for (uint8 i = 0; i < countof(afHodirHelpersNormal); ++i)
                    pSummoner->SummonCreature(pSummoner->GetTeam() == ALLIANCE ? afHodirHelpersNormal[i].uiAllyEntry : afHodirHelpersNormal[i].uiHordeEntry, afHodirHelpersNormal[i].fX, afHodirHelpersNormal[i].fY, afHodirHelpersNormal[i].fZ, afHodirHelpersNormal[i].fO, TEMPSUMMON_DEAD_DESPAWN, 0, true);

                if (!instance->IsRegularDifficulty())
                {
                    for (uint8 i = 0; i < countof(afHodirHelpersHeroic); ++i)
                        pSummoner->SummonCreature(pSummoner->GetTeam() == ALLIANCE ? afHodirHelpersHeroic[i].uiAllyEntry : afHodirHelpersHeroic[i].uiHordeEntry, afHodirHelpersHeroic[i].fX, afHodirHelpersHeroic[i].fY, afHodirHelpersHeroic[i].fZ, afHodirHelpersHeroic[i].fO, TEMPSUMMON_DEAD_DESPAWN, 0, true);
                }
            }
        }

        // Used in order to unlock the door to Vezax
        void DoOpenMadnessDoorIfCan()
        {
            if (m_auiEncounter[TYPE_MIMIRON] == DONE && m_auiEncounter[TYPE_HODIR] == DONE && m_auiEncounter[TYPE_THORIM] == DONE && m_auiEncounter[TYPE_FREYA] == DONE)
                DoUseDoorOrButton(GO_ANCIENT_GATE);
        }

        // function which will handle the Flame Leviathan backup spawns
        void DoCallLeviathanHelp()
        {
            Creature* pLeviathan = GetSingleCreatureFromStorage(NPC_LEVIATHAN);
            if (!pLeviathan)
                return;

            for (uint8 i = 0; i < countof(afReinforcementsNormal); ++i)
                pLeviathan->SummonCreature(afReinforcementsNormal[i].uiEntry, afReinforcementsNormal[i].fX, afReinforcementsNormal[i].fY, afReinforcementsNormal[i].fZ, afReinforcementsNormal[i].fO, TEMPSUMMON_DEAD_DESPAWN, 0, true);

            if (!instance->IsRegularDifficulty())
            {
                for (uint8 i = 0; i < countof(afReinforcementsHeroic); ++i)
                    pLeviathan->SummonCreature(afReinforcementsHeroic[i].uiEntry, afReinforcementsHeroic[i].fX, afReinforcementsHeroic[i].fY, afReinforcementsHeroic[i].fZ, afReinforcementsHeroic[i].fO, TEMPSUMMON_DEAD_DESPAWN, 0, true);
            }
        }

        std::string m_strInstData;
        uint32 m_auiEncounter[MAX_ENCOUNTER];
        uint32 m_auiHardBoss[HARD_MODE_ENCOUNTER];
        uint32 m_auiUlduarKeepers[KEEPER_ENCOUNTER];
        uint32 m_auiUlduarTowers[KEEPER_ENCOUNTER];
        bool m_abAchievCriteria[MAX_SPECIAL_ACHIEV_CRITS];

        bool m_bHelpersLoaded;
        bool m_bSmashSide;

        uint32 m_uiAlgalonTimer;
        uint32 m_uiYoggResetTimer;
        uint32 m_uiShatterAchievTimer;
        uint32 m_uiGauntletStatus;
        uint32 m_uiStairsSpawnTimer;
        uint8 m_uiThorimSmashIndex;
        uint8 m_uiSlayedArenaMobs;
        uint8 m_uiRazorCurrentHarpoon;

        ObjectGuid m_leftKoloStalkerGuid;
        ObjectGuid m_rightKoloStalkerGuid;
        ObjectGuid m_animusVezaxBunnyGuid;
        ObjectGuid m_vaporVezaxBunnyGuid;
        ObjectGuid m_thorimCreature;

        GuidVector m_vToyPileGuidVector;
        GuidVector m_vBrokenHarpoonsGuids;
        GuidList m_lEngineersGuids;
        GuidList m_lTrappersGuids;
        GuidList m_lDefendersGuids;
        GuidList m_lHarpoonDummyGuids;
        GuidList m_lRepairedHarpoonsGuids;
        GuidList m_lThorimBunniesGuids;
        GuidList m_lUpperBunniesGuids;
        GuidList m_lUpperThunderOrbsGuids;
        GuidList m_lLeftHandBunniesGuids;
        GuidList m_lRightHandBunniesGuids;
        GuidList m_lOminousCloudsGuids;
        GuidSet m_sColossusGuidSet;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_ulduar(pMap);
    }
};

struct event_ulduar_spell_shatter : public MapEventScript   //EVENT_ID_SPELL_SHATTER
{
    event_ulduar_spell_shatter() : MapEventScript("event_ulduar_spell_shatter") {}

    bool OnReceived(uint32 uiEventId, Object* pSource, Object* /*pTarget*/, bool /*bIsStart*/) override
    {
        if (pSource->GetTypeId() == TYPEID_UNIT)
        {
            if (InstanceData* pInstance = ((Creature*)pSource)->GetInstanceData())
            {
                pInstance->SetData(TYPE_DO_SHATTERED_EVENT, 0);
                return true;
            }
        }
        return false;
    }
};

struct event_ulduar_shutdown : public MapEventScript    //EVENT_ID_SHUTDOWN
{
    event_ulduar_shutdown() : MapEventScript("event_ulduar_shutdown") {}

    bool OnReceived(uint32 uiEventId, Object* pSource, Object* /*pTarget*/, bool /*bIsStart*/) override
    {
        if (pSource->GetTypeId() == TYPEID_UNIT)
        {
            if (InstanceData* pInstance = ((Creature*)pSource)->GetInstanceData())
            {
                pInstance->SetData(TYPE_ACHIEV_SHUTOUT, uint32(false));
                return true;
            }
        }
        return false;
    }
};

struct event_ulduar_scrap_repair : public MapEventScript    //EVENT_ID_SCRAP_REPAIR
{
    event_ulduar_scrap_repair() : MapEventScript("event_ulduar_scrap_repair") {}

    bool OnReceived(uint32 uiEventId, Object* pSource, Object* /*pTarget*/, bool /*bIsStart*/) override
    {
        if (pSource->GetTypeId() == TYPEID_UNIT)
        {
            if (InstanceData* pInstance = ((Creature*)pSource)->GetInstanceData())
            {
                pInstance->SetData(TYPE_ACHIEV_NERF_ENG, uint32(false));
                return true;
            }
        }
        return false;
    }
};

void AddSC_instance_ulduar()
{
    Script* s;

    s = new is_ulduar();
    s->RegisterSelf();

    s = new event_ulduar_spell_shatter();
    s->RegisterSelf();
    s = new event_ulduar_shutdown();
    s->RegisterSelf();
    s = new event_ulduar_scrap_repair();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_ulduar";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_ulduar;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "event_ulduar";
    //pNewScript->pProcessEventId = &ProcessEventId_event_ulduar;
    //pNewScript->RegisterSelf();
}

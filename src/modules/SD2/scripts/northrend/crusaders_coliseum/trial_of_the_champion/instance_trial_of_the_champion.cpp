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
SDName: instance_trial_of_the_champion
SD%Complete: 20
SDComment: Basic support
SDCategory: Crusader Coliseum, Trial of the Champion
EndScriptData */

#include "precompiled.h"
#include "trial_of_the_champion.h"

/* Trial of the Champion encounters:
0 - Grand Champions
1 - Argent Champion
2 - Black Knight
*/

enum
{
    // grand champions
    SAY_HERALD_HORDE_CHALLENGE              = -1650000,
    SAY_HERALD_HORDE_WARRIOR                = -1650001,
    SAY_HERALD_HORDE_MAGE                   = -1650002,
    SAY_HERALD_HORDE_SHAMAN                 = -1650003,
    SAY_HERALD_HORDE_HUNTER                 = -1650004,
    SAY_HERALD_HORDE_ROGUE                  = -1650005,

    SAY_HERALD_ALLIANCE_CHALLENGE           = -1650006,
    SAY_HERALD_ALLIANCE_WARRIOR             = -1650007,
    SAY_HERALD_ALLIANCE_MAGE                = -1650008,
    SAY_HERALD_ALLIANCE_SHAMAN              = -1650009,
    SAY_HERALD_ALLIANCE_HUNTER              = -1650010,
    SAY_HERALD_ALLIANCE_ROGUE               = -1650011,

    SAY_TIRION_CHALLENGE_WELCOME            = -1650012,
    SAY_TIRION_FIRST_CHALLENGE              = -1650013,
    SAY_THRALL_ALLIANCE_CHALLENGE           = -1650014,
    SAY_GARROSH_ALLIANCE_CHALLENGE          = -1650015,
    SAY_VARIAN_HORDE_CHALLENGE              = -1650016,
    SAY_TIRION_CHALLENGE_BEGIN              = -1650017,

    // argent champion
    SAY_TIRION_ARGENT_CHAMPION              = -1650028,
    SAY_TIRION_ARGENT_CHAMPION_BEGIN        = -1650029,
    SAY_HERALD_EADRIC                       = -1650030,
    SAY_HERALD_PALETRESS                    = -1650031,
    EMOTE_HORDE_ARGENT_CHAMPION             = -1650032,
    EMOTE_ALLIANCE_ARGENT_CHAMPION          = -1650033,
    SAY_EADRIC_INTRO                        = -1650034,
    SAY_PALETRESS_INTRO_1                   = -1650035,
    SAY_PALETRESS_INTRO_2                   = -1650036,

    // black knight
    SAY_TIRION_ARGENT_CHAMPION_COMPLETE     = -1650037,
    SAY_HERALD_BLACK_KNIGHT_SPAWN           = -1650038,
    SAY_BLACK_KNIGHT_INTRO_1                = -1650039,
    SAY_TIRION_BLACK_KNIGHT_INTRO_2         = -1650040,
    SAY_BLACK_KNIGHT_INTRO_3                = -1650041,
    SAY_BLACK_KNIGHT_INTRO_4                = -1650042,

    // event complete
    SAY_TIRION_EPILOG_1                     = -1650043,
    SAY_TIRION_EPILOG_2                     = -1650044,
    SAY_VARIAN_ALLIANCE_EPILOG_3            = -1650045,
    SAY_THRALL_HORDE_EPILOG_3               = -1650046,

    // other texts
    SAY_GARROSH_OTHER_1                     = -1650047,
    SAY_THRALL_OTHER_2                      = -1650048,
    SAY_GARROSH_OTHER_3                     = -1650049,
    SAY_VARIAN_OTHER_4                      = -1650050,
    SAY_VARIAN_OTHER_5                      = -1650051,

    // spells
    SPELL_ARGENT_GET_PLAYER_COUNT           = 66986,
    SPELL_ARGENT_SUMMON_CHAMPION_1          = 66654,
    SPELL_ARGENT_SUMMON_CHAMPION_2          = 66671,
    SPELL_ARGENT_SUMMON_CHAMPION_3          = 66673,
    SPELL_ARGENT_SUMMON_CHAMPION_WAVE       = 67295,
    SPELL_ARGENT_SUMMON_BOSS_4              = 67396,
    SPELL_ARGENT_HERALD_FEIGN_DEATH         = 66804,
};

static const DialogueEntryTwoSide aTocDialogues[] =
{
    // ToDo:
    {0, 0, 0, 0 , 0}
};

static const float aHeraldPositions[3][4] =
{
    { 745.606f, 619.705f, 411.172f, 4.66003f },                           // Spawn position
    { 732.524f, 663.007f, 412.393f, 0.0f },                               // Gate movement position
    { 743.377f, 630.240f, 411.073f, 0.0f },                               // Near center position
};

// data that provides grand champion entry, vehicle mount, trash champions with the spawn locations as well as crowd stalker and emote entry
struct ChampionsData
{
    uint32 uiEntry, uiMount, uiChampion, uiCrowdStalker;
    int32 iEmoteEntry;
    float fX, fY, fZ, fO;
};

// ToDo: add coords
static const ChampionsData aAllianceChampions[MAX_CHAMPIONS_AVAILABLE] =
{
    { NPC_ALLIANCE_WARRIOR, NPC_ALLIANCE_WARRIOR_MOUNT, NPC_ALLIANCE_WARRIOR_CHAMPION, NPC_SPECTATOR_HUMAN, EMOTE_HUMANS },
    { NPC_ALLIANCE_MAGE, NPC_ALLIANCE_MAGE_MOUNT, NPC_ALLIANCE_MAGE_CHAMPION, NPC_SPECTATOR_GNOME, EMOTE_GNOMES },
    { NPC_ALLIANCE_SHAMAN, NPC_ALLIANCE_SHAMAN_MOUNT, NPC_ALLIANCE_SHAMAN_CHAMPION, NPC_SPECTATOR_DRAENEI, EMOTE_DRAENEI },
    { NPC_ALLIANCE_HUNTER, NPC_ALLIANCE_HUNTER_MOUNT, NPC_ALLIANCE_HUNTER_CHAMPION, NPC_SPECTATOR_NIGHT_ELF, EMOTE_NIGHT_ELVES },
    { NPC_ALLIANCE_ROGUE, NPC_ALLIANCE_ROGUE_MOUNT, NPC_ALLIANCE_ROGUE_CHAMPION, NPC_SPECTATOR_DWARF, EMOTE_DWARVES }
};

// ToDo: add coords
static const ChampionsData aHordeChampions[MAX_CHAMPIONS_AVAILABLE] =
{
    { NPC_HORDE_WARRIOR, NPC_HORDE_WARRIOR_MOUNT, NPC_HORDE_WARRIOR_CHAMPION, NPC_SPECTATOR_ORC, EMOTE_ORCS },
    { NPC_HORDE_MAGE, NPC_HORDE_MAGE_MOUNT, NPC_HORDE_MAGE_CHAMPION, NPC_SPECTATOR_BLOOD_ELF, EMOTE_BLOOD_ELVES },
    { NPC_HORDE_SHAMAN, NPC_HORDE_SHAMAN_MOUNT, NPC_HORDE_SHAMAN_CHAMPION, NPC_SPECTATOR_TAUREN, EMOTE_TAUREN },
    { NPC_HORDE_HUNTER, NPC_HORDE_HUNTER_MOUNT, NPC_HORDE_HUNTER_CHAMPION, NPC_SPECTATOR_TROLL, EMOTE_TROLLS },
    { NPC_HORDE_ROGUE, NPC_HORDE_ROGUE_MOUNT, NPC_HORDE_ROGUE_CHAMPION, NPC_SPECTATOR_UNDEAD, EMOTE_UNDEAD }
};

// data that provides spawn coordinates and entry for the player mounts
struct ChampionsMountsData
{
    uint32 uiEntryAlliance, uiEntryHorde;
    float fX, fY, fZ, fO;
};

static const ChampionsMountsData aTrialChampionsMounts[MAX_CHAMPIONS_MOUNTS] =
{
    { NPC_WARHORSE_ALLIANCE, NPC_WARHORSE_HORDE, 720.569f, 571.285f, 412.475f, 1.064f },
    { NPC_WARHORSE_ALLIANCE, NPC_WARHORSE_HORDE, 722.363f, 660.745f, 412.468f, 4.834f },
    { NPC_WARHORSE_ALLIANCE, NPC_WARHORSE_HORDE, 699.943f, 643.370f, 412.474f, 5.777f },
    { NPC_WARHORSE_ALLIANCE, NPC_WARHORSE_HORDE, 768.255f, 661.606f, 412.470f, 4.555f },
    { NPC_WARHORSE_ALLIANCE, NPC_WARHORSE_HORDE, 787.439f, 584.969f, 412.476f, 2.478f },
    { NPC_WARHORSE_ALLIANCE, NPC_WARHORSE_HORDE, 793.009f, 592.667f, 412.475f, 2.652f },
    { NPC_WARHORSE_ALLIANCE, NPC_WARHORSE_HORDE, 704.943f, 651.330f, 412.475f, 5.602f },
    { NPC_WARHORSE_ALLIANCE, NPC_WARHORSE_HORDE, 702.967f, 587.649f, 412.475f, 0.610f },
    { NPC_WARHORSE_ALLIANCE, NPC_WARHORSE_HORDE, 712.594f, 576.260f, 412.476f, 0.890f },
    { NPC_WARHORSE_ALLIANCE, NPC_WARHORSE_HORDE, 774.898f, 573.736f, 412.475f, 2.146f },
    { NPC_WARHORSE_ALLIANCE, NPC_WARHORSE_HORDE, 790.490f, 646.533f, 412.474f, 3.717f },
    { NPC_WARHORSE_ALLIANCE, NPC_WARHORSE_HORDE, 777.564f, 660.300f, 412.467f, 4.345f },
    { NPC_BATTLEWORG_ALLIANCE, NPC_BATTLEWORG_HORDE, 705.497f, 583.944f, 412.476f, 0.698f },
    { NPC_BATTLEWORG_ALLIANCE, NPC_BATTLEWORG_HORDE, 790.177f, 589.059f, 412.475f, 2.565f },
    { NPC_BATTLEWORG_ALLIANCE, NPC_BATTLEWORG_HORDE, 702.165f, 647.267f, 412.475f, 5.689f },
    { NPC_BATTLEWORG_ALLIANCE, NPC_BATTLEWORG_HORDE, 717.443f, 660.646f, 412.467f, 4.921f },
    { NPC_BATTLEWORG_ALLIANCE, NPC_BATTLEWORG_HORDE, 716.665f, 573.495f, 412.475f, 0.977f },
    { NPC_BATTLEWORG_ALLIANCE, NPC_BATTLEWORG_HORDE, 793.052f, 642.851f, 412.474f, 3.630f },
    { NPC_BATTLEWORG_ALLIANCE, NPC_BATTLEWORG_HORDE, 726.826f, 661.201f, 412.472f, 4.660f },
    { NPC_BATTLEWORG_ALLIANCE, NPC_BATTLEWORG_HORDE, 773.097f, 660.733f, 412.467f, 4.450f },
    { NPC_BATTLEWORG_ALLIANCE, NPC_BATTLEWORG_HORDE, 788.016f, 650.788f, 412.475f, 3.804f },
    { NPC_BATTLEWORG_ALLIANCE, NPC_BATTLEWORG_HORDE, 700.531f, 591.927f, 412.475f, 0.523f },
    { NPC_BATTLEWORG_ALLIANCE, NPC_BATTLEWORG_HORDE, 770.486f, 571.552f, 412.475f, 2.059f },
    { NPC_BATTLEWORG_ALLIANCE, NPC_BATTLEWORG_HORDE, 778.741f, 576.049f, 412.476f, 2.234f },
};

struct is_trial_of_the_champion : public InstanceScript
{
    is_trial_of_the_champion() : InstanceScript("instance_trial_of_the_champion") {}

    class instance_trial_of_the_champion : public ScriptedInstance, private DialogueHelper
    {
    public:
        instance_trial_of_the_champion(Map* pMap) : ScriptedInstance(pMap), DialogueHelper(aTocDialogues),
            m_uiTeam(TEAM_NONE),
            m_uiHeraldEntry(0),
            m_uiGrandChampionEntry(0)
        {
            Initialize();
        }

        void Initialize() override
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
            InitializeDialogueHelper(this);
        }

        void OnPlayerEnter(Player* pPlayer) override
        {
            if (!m_uiTeam)
            {
                m_uiTeam = pPlayer->GetTeam();
                SetDialogueSide(m_uiTeam == ALLIANCE);

                m_uiHeraldEntry = m_uiTeam == ALLIANCE ? NPC_ARELAS_BRIGHTSTAR : NPC_JAEREN_SUNSWORN;

                // set a random grand champion
                m_uiGrandChampionEntry = urand(0, 1) ? NPC_EADRIC : NPC_PALETRESS;

                if (m_vChampionsIndex.empty())
                {
                    m_vChampionsIndex.resize(MAX_CHAMPIONS_AVAILABLE);

                    // fill vector array with indexes from creature array
                    for (uint8 i = 0; i < MAX_CHAMPIONS_AVAILABLE; ++i)
                        m_vChampionsIndex[i] = i;

                    // set a random champion list
                    std::random_shuffle(m_vChampionsIndex.begin(), m_vChampionsIndex.end());
                }
            }

            DoSummonHeraldIfNeeded(pPlayer);
        }

        void OnCreatureCreate(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_JAEREN_SUNSWORN:
            case NPC_ARELAS_BRIGHTSTAR:
            case NPC_TIRION_FORDRING:
            case NPC_VARIAN_WRYNN:
            case NPC_THRALL:
            case NPC_GARROSH:
            case NPC_ALLIANCE_WARRIOR:
            case NPC_ALLIANCE_MAGE:
            case NPC_ALLIANCE_SHAMAN:
            case NPC_ALLIANCE_HUNTER:
            case NPC_ALLIANCE_ROGUE:
            case NPC_HORDE_WARRIOR:
            case NPC_HORDE_MAGE:
            case NPC_HORDE_SHAMAN:
            case NPC_HORDE_HUNTER:
            case NPC_HORDE_ROGUE:
            case NPC_EADRIC:
            case NPC_PALETRESS:
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
            case GO_MAIN_GATE:
            case GO_NORTH_GATE:
            case GO_CHAMPIONS_LOOT:
            case GO_CHAMPIONS_LOOT_H:
            case GO_EADRIC_LOOT:
            case GO_EADRIC_LOOT_H:
            case GO_PALETRESS_LOOT:
            case GO_PALETRESS_LOOT_H:
                break;
            default:
                return;
            }

            m_mGoEntryGuidStore[pGo->GetEntry()] = pGo->GetObjectGuid();
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            switch (uiType)
            {
            case TYPE_GRAND_CHAMPIONS:
                m_auiEncounter[uiType] = uiData;
                if (uiData == DONE)
                    DoRespawnGameObject(instance->IsRegularDifficulty() ? GO_CHAMPIONS_LOOT : GO_CHAMPIONS_LOOT_H, 30 * MINUTE);
                break;
            case TYPE_ARGENT_CHAMPION:
                m_auiEncounter[uiType] = uiData;
                if (uiData == DONE)
                {
                    if (m_uiGrandChampionEntry == NPC_EADRIC)
                        DoRespawnGameObject(instance->IsRegularDifficulty() ? GO_EADRIC_LOOT : GO_EADRIC_LOOT_H, 30 * MINUTE);
                    else if (m_uiGrandChampionEntry == NPC_PALETRESS)
                        DoRespawnGameObject(instance->IsRegularDifficulty() ? GO_PALETRESS_LOOT : GO_PALETRESS_LOOT_H, 30 * MINUTE);
                }
                break;
            case TYPE_BLACK_KNIGHT:
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_DO_PREPARE_CHAMPIONS:
                DoPrepareChampions(bool(uiData));
                return;
            default:
                script_error_log("Instance Trial of The Champion: ERROR SetData = %u for type %u does not exist/not implemented.", uiType, uiData);
                return;
            }

            if (uiData == DONE)
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
                return m_auiEncounter[uiType];

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
                    m_auiEncounter[i] = NOT_STARTED;
            }

            OUT_LOAD_INST_DATA_COMPLETE;
        }

        void Update(uint32 uiDiff) override { DialogueUpdate(uiDiff); }

    private:
        void DoPrepareChampions(bool bSkipIntro) {} //@TODO

        void JustDidDialogueStep(int32 iEntry) override {}  //@TODO

        void DoSummonHeraldIfNeeded(Unit* pSummoner)
        {
            if (!pSummoner)
                return;

            if (GetSingleCreatureFromStorage(m_uiHeraldEntry, true))
                return;

            pSummoner->SummonCreature(m_uiHeraldEntry, aHeraldPositions[0][0], aHeraldPositions[0][1], aHeraldPositions[0][2], aHeraldPositions[0][3], TEMPSUMMON_DEAD_DESPAWN, 0);

            // summon champion mounts if required
            if (GetData(TYPE_GRAND_CHAMPIONS) != DONE)
            {
                for (uint8 i = 0; i < MAX_CHAMPIONS_MOUNTS; ++i)
                    pSummoner->SummonCreature(m_uiTeam == ALLIANCE ? aTrialChampionsMounts[i].uiEntryAlliance : aTrialChampionsMounts[i].uiEntryHorde, aTrialChampionsMounts[i].fX, aTrialChampionsMounts[i].fY, aTrialChampionsMounts[i].fZ, aTrialChampionsMounts[i].fO, TEMPSUMMON_DEAD_DESPAWN, 0);
            }
        }

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strInstData;

        Team m_uiTeam;

        uint32 m_uiHeraldEntry;
        uint32 m_uiGrandChampionEntry;

        std::vector<uint8> m_vChampionsIndex;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_trial_of_the_champion(pMap);
    }
};

void AddSC_instance_trial_of_the_champion()
{
    Script* s;

    s = new is_trial_of_the_champion();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_trial_of_the_champion";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_trial_of_the_champion;
    //pNewScript->RegisterSelf();
}

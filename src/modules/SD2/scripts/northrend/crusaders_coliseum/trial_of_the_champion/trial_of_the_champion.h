/* Copyright (C) 2006 - 2013 ScriptDev2 <http://www.scriptdev2.com/>
 * This program is free software licensed under GPL version 2
 * Please see the included DOCS/LICENSE.TXT for more information */

#ifndef DEF_TRIAL_OF_THE_CHAMPION_H
#define DEF_TRIAL_OF_THE_CHAMPION_H

enum
{
    MAX_ENCOUNTER                       = 3,
    MAX_CHAMPIONS_AVAILABLE             = 5,
    MAX_CHAMPIONS_ARENA                 = 3,
    MAX_CHAMPIONS_MOUNTS                = 24,

    TYPE_GRAND_CHAMPIONS                = 0,
    TYPE_ARGENT_CHAMPION                = 1,
    TYPE_BLACK_KNIGHT                   = 2,

    // event handler
    NPC_ARELAS_BRIGHTSTAR               = 35005,                        // alliance
    NPC_JAEREN_SUNSWORN                 = 35004,                        // horde
    NPC_TIRION_FORDRING                 = 34996,
    NPC_VARIAN_WRYNN                    = 34990,
    NPC_THRALL                          = 34994,
    NPC_GARROSH                         = 34995,

    // champions alliance
    NPC_ALLIANCE_WARRIOR                = 34705,                        // Jacob Alerius
    NPC_ALLIANCE_WARRIOR_MOUNT          = 35637,                        // Jacob vehicle mount
    NPC_ALLIANCE_WARRIOR_CHAMPION       = 35328,                        // Stormwind Champion
    NPC_ALLIANCE_MAGE                   = 34702,                        // Ambrose Boltspark
    NPC_ALLIANCE_MAGE_MOUNT             = 35633,                        // Ambrose vehicle mount
    NPC_ALLIANCE_MAGE_CHAMPION          = 35331,                        // Gnomeregan Champion
    NPC_ALLIANCE_SHAMAN                 = 34701,                        // Colosos
    NPC_ALLIANCE_SHAMAN_MOUNT           = 35768,                        // Colosos vehicle mount
    NPC_ALLIANCE_SHAMAN_CHAMPION        = 35330,                        // Exodar Champion
    NPC_ALLIANCE_HUNTER                 = 34657,                        // Jaelyne Evensong
    NPC_ALLIANCE_HUNTER_MOUNT           = 34658,                        // Jaelyne vehicle mount
    NPC_ALLIANCE_HUNTER_CHAMPION        = 35332,                        // Darnassus Champion
    NPC_ALLIANCE_ROGUE                  = 34703,                        // Lana Stouthammer
    NPC_ALLIANCE_ROGUE_MOUNT            = 35636,                        // Lana vehicle mount
    NPC_ALLIANCE_ROGUE_CHAMPION         = 35329,                        // Ironforge Champion

    // champions horde
    NPC_HORDE_WARRIOR                   = 35572,                        // Mokra Skullcrusher
    NPC_HORDE_WARRIOR_MOUNT             = 35638,                        // Mokra vehicle mount
    NPC_HORDE_WARRIOR_CHAMPION          = 35314,                        // Orgrimmar Champion
    NPC_HORDE_MAGE                      = 35569,                        // Eressea Dawnsinger
    NPC_HORDE_MAGE_MOUNT                = 35635,                        // Eressea vehicle mount
    NPC_HORDE_MAGE_CHAMPION             = 35326,                        // Silvermoon Champion
    NPC_HORDE_SHAMAN                    = 35571,                        // Runok Wildmane
    NPC_HORDE_SHAMAN_MOUNT              = 35640,                        // Runok vehicle mount
    NPC_HORDE_SHAMAN_CHAMPION           = 35325,                        // Thunder Bluff Champion
    NPC_HORDE_HUNTER                    = 35570,                        // Zul'tore
    NPC_HORDE_HUNTER_MOUNT              = 35641,                        // Zul'tore vehicle mount
    NPC_HORDE_HUNTER_CHAMPION           = 35323,                        // Sen'jin Champion
    NPC_HORDE_ROGUE                     = 35617,                        // Deathstalker Visceri
    NPC_HORDE_ROGUE_MOUNT               = 35634,                        // Visceri vehicle mount
    NPC_HORDE_ROGUE_CHAMPION            = 35327,                        // Undercity Champion

    // spectators triggers
    NPC_WORLD_TRIGGER                   = 22515,
    NPC_SPECTATOR_GENERIC               = 35016,
    NPC_SPECTATOR_HORDE                 = 34883,
    NPC_SPECTATOR_ALLIANCE              = 34887,
    NPC_SPECTATOR_HUMAN                 = 34900,
    NPC_SPECTATOR_ORC                   = 34901,
    NPC_SPECTATOR_TROLL                 = 34902,
    NPC_SPECTATOR_TAUREN                = 34903,
    NPC_SPECTATOR_BLOOD_ELF             = 34904,
    NPC_SPECTATOR_UNDEAD                = 34905,
    NPC_SPECTATOR_DWARF                 = 34906,
    NPC_SPECTATOR_DRAENEI               = 34908,
    NPC_SPECTATOR_NIGHT_ELF             = 34909,
    NPC_SPECTATOR_GNOME                 = 34910,

    // mounts
    NPC_WARHORSE_ALLIANCE               = 36557,                        // alliance mount vehicle
    NPC_WARHORSE_HORDE                  = 35644,                        // dummy - part of the decorations
    NPC_BATTLEWORG_ALLIANCE             = 36559,                        // dummy - part of the decorations
    NPC_BATTLEWORG_HORDE                = 36558,                        // horde mount vehicle

    // argent challegers
    NPC_EADRIC                          = 35119,
    NPC_PALETRESS                       = 34928,
    // trash mobs
    NPC_ARGENT_LIGHTWIELDER             = 35309,
    NPC_ARGENT_MONK                     = 35305,
    NPC_ARGENT_PRIESTESS                = 35307,

    // black knight
    NPC_BLACK_KNIGHT                    = 35451,
    NPC_BLACK_KNIGHT_GRYPHON            = 35491,
    // risen zombies
    NPC_RISEN_JAEREN                    = 35545,
    NPC_RISEN_ARELAS                    = 35564,
    NPC_RISEN_CHAMPION                  = 35590,

    // doors
    GO_MAIN_GATE                        = 195647,
    GO_NORTH_GATE                       = 195650,                       // combat door

    // chests
    GO_CHAMPIONS_LOOT                   = 195709,
    GO_CHAMPIONS_LOOT_H                 = 195710,
    GO_EADRIC_LOOT                      = 195374,
    GO_EADRIC_LOOT_H                    = 195375,
    GO_PALETRESS_LOOT                   = 195323,
    GO_PALETRESS_LOOT_H                 = 195324,

    // fireworks
    GO_FIREWORKS_RED_1                  = 180703,
    GO_FIREWORKS_RED_2                  = 180708,
    GO_FIREWORKS_BLUE_1                 = 180720,
    GO_FIREWORKS_BLUE_2                 = 180723,
    GO_FIREWORKS_WHITE_1                = 180728,
    GO_FIREWORKS_WHITE_2                = 180730,
    GO_FIREWORKS_YELLOW_1               = 180736,
    GO_FIREWORKS_YELLOW_2               = 180738,

    // area triggers - purpose unk
    // AREATRIGGER_ID_TOC_1             = 5491,
    // AREATRIGGER_ID_TOC_2             = 5492,

    // emotes
    EMOTE_BLOOD_ELVES                   = -1650018,
    EMOTE_TROLLS                        = -1650019,
    EMOTE_TAUREN                        = -1650020,
    EMOTE_UNDEAD                        = -1650021,
    EMOTE_ORCS                          = -1650022,
    EMOTE_DWARVES                       = -1650023,
    EMOTE_GNOMES                        = -1650024,
    EMOTE_NIGHT_ELVES                   = -1650025,
    EMOTE_HUMANS                        = -1650026,
    EMOTE_DRAENEI                       = -1650027,
};

static const float aHeraldPositions[3][4] =
{
    {745.606f, 619.705f, 411.172f, 4.66003f},                           // Spawn position
    {732.524f, 663.007f, 412.393f, 0.0f},                               // Gate movement position
    {743.377f, 630.240f, 411.073f, 0.0f},                               // Near center position
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
    { NPC_ALLIANCE_WARRIOR, NPC_ALLIANCE_WARRIOR_MOUNT, NPC_ALLIANCE_WARRIOR_CHAMPION, NPC_SPECTATOR_HUMAN,     EMOTE_HUMANS },
    { NPC_ALLIANCE_MAGE,    NPC_ALLIANCE_MAGE_MOUNT,    NPC_ALLIANCE_MAGE_CHAMPION,    NPC_SPECTATOR_GNOME,     EMOTE_GNOMES },
    { NPC_ALLIANCE_SHAMAN,  NPC_ALLIANCE_SHAMAN_MOUNT,  NPC_ALLIANCE_SHAMAN_CHAMPION,  NPC_SPECTATOR_DRAENEI,   EMOTE_DRAENEI },
    { NPC_ALLIANCE_HUNTER,  NPC_ALLIANCE_HUNTER_MOUNT,  NPC_ALLIANCE_HUNTER_CHAMPION,  NPC_SPECTATOR_NIGHT_ELF, EMOTE_NIGHT_ELVES },
    { NPC_ALLIANCE_ROGUE,   NPC_ALLIANCE_ROGUE_MOUNT,   NPC_ALLIANCE_ROGUE_CHAMPION,   NPC_SPECTATOR_DWARF,     EMOTE_DWARVES }
};

// ToDo: add coords
static const ChampionsData aHordeChampions[MAX_CHAMPIONS_AVAILABLE] =
{
    { NPC_HORDE_WARRIOR, NPC_HORDE_WARRIOR_MOUNT, NPC_HORDE_WARRIOR_CHAMPION, NPC_SPECTATOR_ORC,       EMOTE_ORCS },
    { NPC_HORDE_MAGE,    NPC_HORDE_MAGE_MOUNT,    NPC_HORDE_MAGE_CHAMPION,    NPC_SPECTATOR_BLOOD_ELF, EMOTE_BLOOD_ELVES },
    { NPC_HORDE_SHAMAN,  NPC_HORDE_SHAMAN_MOUNT,  NPC_HORDE_SHAMAN_CHAMPION,  NPC_SPECTATOR_TAUREN,    EMOTE_TAUREN },
    { NPC_HORDE_HUNTER,  NPC_HORDE_HUNTER_MOUNT,  NPC_HORDE_HUNTER_CHAMPION,  NPC_SPECTATOR_TROLL,     EMOTE_TROLLS },
    { NPC_HORDE_ROGUE,   NPC_HORDE_ROGUE_MOUNT,   NPC_HORDE_ROGUE_CHAMPION,   NPC_SPECTATOR_UNDEAD,    EMOTE_UNDEAD }
};

// data that provides spawn coordinates and entry for the player mounts
struct ChampionsMountsData
{
    uint32 uiEntryAlliance, uiEntryHorde;
    float fX, fY, fZ, fO;
};

static const ChampionsMountsData aTrialChampionsMounts[MAX_CHAMPIONS_MOUNTS] =
{
    {NPC_WARHORSE_ALLIANCE,   NPC_WARHORSE_HORDE,   720.569f, 571.285f, 412.475f, 1.064f},
    {NPC_WARHORSE_ALLIANCE,   NPC_WARHORSE_HORDE,   722.363f, 660.745f, 412.468f, 4.834f},
    {NPC_WARHORSE_ALLIANCE,   NPC_WARHORSE_HORDE,   699.943f, 643.370f, 412.474f, 5.777f},
    {NPC_WARHORSE_ALLIANCE,   NPC_WARHORSE_HORDE,   768.255f, 661.606f, 412.470f, 4.555f},
    {NPC_WARHORSE_ALLIANCE,   NPC_WARHORSE_HORDE,   787.439f, 584.969f, 412.476f, 2.478f},
    {NPC_WARHORSE_ALLIANCE,   NPC_WARHORSE_HORDE,   793.009f, 592.667f, 412.475f, 2.652f},
    {NPC_WARHORSE_ALLIANCE,   NPC_WARHORSE_HORDE,   704.943f, 651.330f, 412.475f, 5.602f},
    {NPC_WARHORSE_ALLIANCE,   NPC_WARHORSE_HORDE,   702.967f, 587.649f, 412.475f, 0.610f},
    {NPC_WARHORSE_ALLIANCE,   NPC_WARHORSE_HORDE,   712.594f, 576.260f, 412.476f, 0.890f},
    {NPC_WARHORSE_ALLIANCE,   NPC_WARHORSE_HORDE,   774.898f, 573.736f, 412.475f, 2.146f},
    {NPC_WARHORSE_ALLIANCE,   NPC_WARHORSE_HORDE,   790.490f, 646.533f, 412.474f, 3.717f},
    {NPC_WARHORSE_ALLIANCE,   NPC_WARHORSE_HORDE,   777.564f, 660.300f, 412.467f, 4.345f},
    {NPC_BATTLEWORG_ALLIANCE, NPC_BATTLEWORG_HORDE, 705.497f, 583.944f, 412.476f, 0.698f},
    {NPC_BATTLEWORG_ALLIANCE, NPC_BATTLEWORG_HORDE, 790.177f, 589.059f, 412.475f, 2.565f},
    {NPC_BATTLEWORG_ALLIANCE, NPC_BATTLEWORG_HORDE, 702.165f, 647.267f, 412.475f, 5.689f},
    {NPC_BATTLEWORG_ALLIANCE, NPC_BATTLEWORG_HORDE, 717.443f, 660.646f, 412.467f, 4.921f},
    {NPC_BATTLEWORG_ALLIANCE, NPC_BATTLEWORG_HORDE, 716.665f, 573.495f, 412.475f, 0.977f},
    {NPC_BATTLEWORG_ALLIANCE, NPC_BATTLEWORG_HORDE, 793.052f, 642.851f, 412.474f, 3.630f},
    {NPC_BATTLEWORG_ALLIANCE, NPC_BATTLEWORG_HORDE, 726.826f, 661.201f, 412.472f, 4.660f},
    {NPC_BATTLEWORG_ALLIANCE, NPC_BATTLEWORG_HORDE, 773.097f, 660.733f, 412.467f, 4.450f},
    {NPC_BATTLEWORG_ALLIANCE, NPC_BATTLEWORG_HORDE, 788.016f, 650.788f, 412.475f, 3.804f},
    {NPC_BATTLEWORG_ALLIANCE, NPC_BATTLEWORG_HORDE, 700.531f, 591.927f, 412.475f, 0.523f},
    {NPC_BATTLEWORG_ALLIANCE, NPC_BATTLEWORG_HORDE, 770.486f, 571.552f, 412.475f, 2.059f},
    {NPC_BATTLEWORG_ALLIANCE, NPC_BATTLEWORG_HORDE, 778.741f, 576.049f, 412.476f, 2.234f},
};

class instance_trial_of_the_champion : public ScriptedInstance, private DialogueHelper
{
    public:
        instance_trial_of_the_champion(Map* pMap);

        void Initialize() override;

        void OnPlayerEnter(Player* pPlayer) override;

        void OnCreatureCreate(Creature* pCreature) override;
        void OnObjectCreate(GameObject* pGo) override;

        void SetData(uint32 uiType, uint32 uiData) override;
        uint32 GetData(uint32 uiType) const override;

        const char* Save() const override { return m_strInstData.c_str(); }
        void Load(const char* chrIn) override;
        
        void DoPrepareChampions(bool bSkipIntro);

        void Update(uint32 uiDiff) override { DialogueUpdate(uiDiff); }

    private:
        void JustDidDialogueStep(int32 iEntry) override;

        void DoSummonHeraldIfNeeded(Unit* pSummoner);

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strInstData;

        Team m_uiTeam;

        uint32 m_uiHeraldEntry;
        uint32 m_uiGrandChampionEntry;

        std::vector<uint8> m_vChampionsIndex;
};

#endif

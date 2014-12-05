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

    TYPE_GRAND_CHAMPIONS                = 0,
    TYPE_ARGENT_CHAMPION                = 1,
    TYPE_BLACK_KNIGHT                   = 2,
    
    // event handler
    NPC_ARELAS_BRIGHTSTAR               = 35005,                        // alliance
    NPC_JAEREN_SUNSWORN                 = 35004,                        // horde
    NPC_TIRION_FORDRING                 = 34996,
    NPC_VARIAN_WRYNN                    = 34990,
    NPC_THRALL                          = 34994,

    // champions alliance
    NPC_JACOB_ALERIUS                   = 34705,                        // warrior
    NPC_AMBROSE_BOLTSPARK               = 34702,                        // mage
    NPC_COLOSOS                         = 34701,                        // shaman
    NPC_JAELYNE_EVENSONG                = 34657,                        // hunter
    NPC_LANA_STOUTHAMMER                = 34703,                        // rogue

    // champions horde
    NPC_MOKRA_SKULLCRUSHER              = 35572,                        // warrior
    NPC_ERESSEA_DAWNSINGER              = 35569,                        // mage
    NPC_RUNOK_WILDMANE                  = 35571,                        // shaman
    NPC_ZULTORE                         = 35570,                        // hunter
    NPC_DEATHSTALKER_VISCERI            = 35617,                        // rogue

    // argent challegers
    NPC_EADRIC                          = 35119,
    NPC_PALETRESS                       = 34928,
    // trash mobs
    NPC_ARGENT_LIGHTWIELDER             = 35309,
    NPC_ARGENT_MONK                     = 35305,
    NPC_ARGENT_PRIESTESS                = 35307,

    // black knight
    NPC_BLACK_KNIGHT                    = 35451,
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

    // area triggers - purpose unk
    // AREATRIGGER_ID_TOC_1             = 5491,
    // AREATRIGGER_ID_TOC_2             = 5492,

    // spells
    SPELL_ARGENT_GET_PLAYER_COUNT       = 66986,
    SPELL_ARGENT_SUMMON_CHAMPION_1      = 66654,
    SPELL_ARGENT_SUMMON_CHAMPION_2      = 66671,
    SPELL_ARGENT_SUMMON_CHAMPION_3      = 66673,
    SPELL_ARGENT_SUMMON_CHAMPION_WAVE   = 67295,
    SPELL_ARGENT_SUMMON_BOSS_4          = 67396,
    SPELL_ARGENT_HERALD_FEIGN_DEATH     = 66804,
};

static const float aHeraldPositions[3][4] =
{
    {745.606f, 619.705f, 411.172f, 4.66003f},                           // Spawn position
    {732.524f, 663.007f, 412.393f, 0.0f},                               // Gate movement position
    {743.377f, 630.240f, 411.073f, 0.0f},                               // Near center position
};

static const uint32 aAllianceChampions[MAX_CHAMPIONS_AVAILABLE] = { NPC_JACOB_ALERIUS, NPC_AMBROSE_BOLTSPARK, NPC_COLOSOS, NPC_JAELYNE_EVENSONG, NPC_LANA_STOUTHAMMER };
static const uint32 aHordeChampions[MAX_CHAMPIONS_AVAILABLE] = { NPC_MOKRA_SKULLCRUSHER, NPC_ERESSEA_DAWNSINGER, NPC_RUNOK_WILDMANE, NPC_ZULTORE, NPC_DEATHSTALKER_VISCERI };

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

        void Update(uint32 uiDiff) override { DialogueUpdate(uiDiff); }

    private:
        void JustDidDialogueStep(int32 iEntry) override;

        void DoSummonHeraldIfNeeded();

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strInstData;

        Team m_uiTeam;

        uint32 m_uiHeraldEntry;
        uint32 m_uiGrandChampionEntry;

        std::vector<uint32> m_vChampionsEntries;
};

#endif

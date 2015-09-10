/* Copyright (C) 2006 - 2013 ScriptDev2 <http://www.scriptdev2.com/>
 * This program is free software licensed under GPL version 2
 * Please see the included DOCS/LICENSE.TXT for more information */

#ifndef DEF_VIOLET_H
#define DEF_VIOLET_H

enum
{
    MAX_ENCOUNTER               = 10,
    MAX_MINIBOSSES              = 6,

    TYPE_MAIN                   = 0,
    TYPE_SEAL                   = 1,
    TYPE_PORTAL                 = 2,
    TYPE_LAVANTHOR              = 3,
    TYPE_MORAGG                 = 4,
    TYPE_EREKEM                 = 5,
    TYPE_ICHORON                = 6,
    TYPE_XEVOZZ                 = 7,
    TYPE_ZURAMAT                = 8,
    TYPE_CYANIGOSA              = 9,
    TYPE_DO_SINCLARI_BEGIN      = MAX_ENCOUNTER,
    TYPE_DATA_PORTAL_NUMBER     = MAX_ENCOUNTER + 1,
    TYPE_DO_RELEASE_BOSS        = MAX_ENCOUNTER + 2,
    TYPE_DATA_PORTAL_ELITE      = MAX_ENCOUNTER + 3,
    TYPE_DATA_IS_TRASH_PORTAL   = MAX_ENCOUNTER + 4,
    TYPE_DATA_GET_MOB_NORMAL    = MAX_ENCOUNTER + 5,

    DATA64_CRYSTAL_ACTIVATOR    = 0,
    DATA64_CRYSTAL_ACTIVATOR_INT= 1,
    DATA64_SABOTEUR             = 2,

    WORLD_STATE_ID              = 3816,
    WORLD_STATE_SEAL            = 3815,
    WORLD_STATE_PORTALS         = 3810,

    GO_INTRO_CRYSTAL            = 193615,
    GO_PRISON_CRYSTAL           = 193611,
    GO_PRISON_SEAL_DOOR         = 191723,

    GO_CELL_LAVANTHOR           = 191566,
    GO_CELL_MORAGG              = 191606,
    GO_CELL_ZURAMAT             = 191565,
    GO_CELL_EREKEM              = 191564,
    GO_CELL_EREKEM_GUARD_L      = 191563,
    GO_CELL_EREKEM_GUARD_R      = 191562,
    GO_CELL_XEVOZZ              = 191556,
    GO_CELL_ICHORON             = 191722,

    NPC_EVENT_CONTROLLER        = 30883,
    NPC_PORTAL_INTRO            = 31011,
    NPC_PORTAL                  = 30679,
    NPC_PORTAL_ELITE            = 32174,
    NPC_DOOR_SEAL               = 30896,

    NPC_SINCLARI                = 30658,
    NPC_SINCLARI_ALT            = 32204,                    // yeller for seal weakening and summoner for portals
    NPC_HOLD_GUARD              = 30659,

    NPC_EREKEM                  = 29315,
    NPC_EREKEM_GUARD            = 29395,
    NPC_MORAGG                  = 29316,
    NPC_ICHORON                 = 29313,
    NPC_XEVOZZ                  = 29266,
    NPC_LAVANTHOR               = 29312,
    NPC_ZURAMAT                 = 29314,
    NPC_CYANIGOSA               = 31134,

    NPC_PORTAL_GUARDIAN         = 30660,
    NPC_PORTAL_KEEPER           = 30695,

    NPC_AZURE_INVADER           = 30661,
    NPC_AZURE_SPELLBREAKER      = 30662,
    NPC_AZURE_BINDER            = 30663,
    NPC_AZURE_MAGE_SLAYER       = 30664,
    NPC_MAGE_HUNTER             = 30665,
    NPC_AZURE_CAPTAIN           = 30666,
    NPC_AZURE_SORCEROR          = 30667,
    NPC_AZURE_RAIDER            = 30668,
    NPC_AZURE_STALKER           = 32191,

    NPC_VOID_SENTRY             = 29364,                    // Npc checked for Zuramat achiev
    NPC_ICHORON_SUMMON_TARGET   = 29326,                    // Npc which summons the Ichoron globules

    // used for intro
    NPC_AZURE_BINDER_INTRO      = 31007,
    NPC_AZURE_INVADER_INTRO     = 31008,
    NPC_AZURE_SPELLBREAKER_INTRO = 31009,
    NPC_AZURE_MAGE_SLAYER_INTRO = 31010,

    NPC_AZURE_SABOTEUR          = 31079,

    NPC_DEFENSE_SYSTEM          = 30837,
    NPC_DEFENSE_DUMMY_TARGET    = 30857,

    // 'Ghosts' for Killed mobs after Wipe
    NPC_ARAKKOA                 = 32226,
    NPC_ARAKKOA_GUARD           = 32228,
    NPC_VOID_LORD               = 32230,
    NPC_ETHERAL                 = 32231,
    NPC_SWIRLING                = 32234,
    NPC_WATCHER                 = 32235,
    NPC_LAVA_HOUND              = 32237,

    SPELL_DEFENSE_SYSTEM_VISUAL = 57887,
    SPELL_DEFENSE_SYSTEM_SPAWN  = 57886,
    SPELL_LIGHTNING_INTRO       = 60038,                    // intro kill spells, also related to spell 58152
    SPELL_ARCANE_LIGHTNING      = 57930,                    // damage spells, related to spell 57912

    SPELL_DESTROY_DOOR_SEAL     = 58040,                    // spell periodic cast by misc
    SPELL_TELEPORTATION_PORTAL  = 57687,                    // visual aura, but possibly not used? creature_template model for portals are same

    SPELL_SHIELD_DISRUPTION     = 58291,                    // dummy when opening a cell
    SPELL_SIMPLE_TELEPORT       = 12980,                    // used after a cell has been opened - not sure if the id is correct

    SPELL_PORTAL_PERIODIC       = 58008,                    // most likely the tick for each summon (tick each 15 seconds)
    SPELL_PORTAL_CHANNEL        = 58012,                    // the blue "stream" between portal and guardian/keeper
    SPELL_PORTAL_BEAM           = 56046,                    // large beam, unsure if really used here (or possible for something different)

    SPELL_PORTAL_VISUAL_1       = 57872,                    // no idea, but is possibly related based on it's visual appearence
    SPELL_PORTAL_VISUAL_2       = 57630,

    SAY_SEAL_75                 = -1608002,
    SAY_SEAL_50                 = -1608003,
    SAY_SEAL_5                  = -1608004,

    SAY_RELEASE_EREKEM          = -1608008,
    SAY_RELEASE_ICHORON         = -1608009,
    SAY_RELEASE_XEVOZZ          = -1608010,
    SAY_RELEASE_ZURAMAT         = -1608011,

    EMOTE_GUARDIAN_PORTAL       = -1608005,
    EMOTE_DRAGONFLIGHT_PORTAL   = -1608006,
    EMOTE_KEEPER_PORTAL         = -1608007,

    MAX_NORMAL_PORTAL           = 8,

    ACHIEV_CRIT_DEFENSELES      = 6803,                     // event achiev - 1816
    ACHIEV_CRIT_DEHYDRATATION   = 7320,                     // Ichoron achiev - 2041
    ACHIEV_CRIT_VOID_DANCE      = 7587,                     // Zuramat achiev - 2153
};

static const float fSealAttackLoc[3] = {1858.027f, 804.11f, 44.008f};

enum ePortalType
{
    PORTAL_TYPE_NORM = 0,
    PORTAL_TYPE_SQUAD,
    PORTAL_TYPE_BOSS,
};

struct PortalData
{
    ePortalType pPortalType;
    float fX, fY, fZ, fOrient;
};

static const PortalData afPortalLocation[] =
{
    {PORTAL_TYPE_NORM, 1936.07f, 803.198f, 53.3749f, 3.1241f},  // balcony
    {PORTAL_TYPE_NORM, 1877.51f, 850.104f, 44.6599f, 4.7822f},  // erekem
    {PORTAL_TYPE_NORM, 1890.64f, 753.471f, 48.7224f, 1.7104f},  // moragg
    {PORTAL_TYPE_SQUAD, 1911.06f, 802.103f, 38.6465f, 2.8908f}, // below balcony
    {PORTAL_TYPE_SQUAD, 1928.06f, 763.256f, 51.3167f, 2.3905f}, // bridge
    {PORTAL_TYPE_SQUAD, 1924.26f, 847.661f, 47.1591f, 4.0202f}, // zuramat
    {PORTAL_TYPE_NORM, 1914.16f, 832.527f, 38.6441f, 3.5160f},  // xevozz
    {PORTAL_TYPE_NORM, 1857.30f, 764.145f, 38.6543f, 0.8339f},  // lavanthor
    {PORTAL_TYPE_BOSS, 1890.73f, 803.309f, 38.4001f, 2.4139f},  // center
};
#endif

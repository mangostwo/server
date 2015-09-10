/* Copyright (C) 2006 - 2013 ScriptDev2 <http://www.scriptdev2.com/>
 * This program is free software licensed under GPL version 2
 * Please see the included DOCS/LICENSE.TXT for more information */

#ifndef DEF_TRIAL_OF_THE_CRUSADER_H
#define DEF_TRIAL_OF_THE_CRUSADER_H

enum
{
    MAX_ENCOUNTER               = 6,
    MAX_WIPES_ALLOWED           = 50,

    TYPE_WIPE_COUNT             = 0,
    TYPE_NORTHREND_BEASTS       = 1,
    TYPE_JARAXXUS               = 2,
    TYPE_FACTION_CHAMPIONS      = 3,
    TYPE_TWIN_VALKYR            = 4,
    TYPE_ANUBARAK               = 5,

    EVENT_OPEN_PORTAL           = 6,
    EVENT_KILL_FIZZLEBANG       = 7,
    EVENT_JARAXXUS_START_ATTACK = 8,
    EVENT_SUMMON_TWINS          = 9,
    EVENT_TWINS_KILLED          = 10,
    EVENT_ARTHAS_PORTAL         = 11,
    EVENT_SUMMON_THE_LICHKING   = 12,
    EVENT_DESTROY_FLOOR         = 13,
    TYPE_DATA_IS_HEROIC         = 14,

    NPC_BEAST_COMBAT_STALKER    = 36549,
    NPC_GORMOK                  = 34796,
    NPC_ACIDMAW                 = 35144,
    NPC_DREADSCALE              = 34799,
    NPC_ICEHOWL                 = 34797,
    NPC_JARAXXUS                = 34780,
    NPC_FJOLA                   = 34497,
    NPC_EYDIS                   = 34496,
    NPC_ANUBARAK                = 34564,

    NPC_TIRION_A                = 34996,
    NPC_TIRION_B                = 36095,                    // Summoned after his text (Champions, you're alive! Not only have you defeated every challenge of the Trial of the Crusader, but also thwarted Arthas' plans! Your skill and cunning will prove to be a powerful weapon against the Scourge. Well done! Allow one of the Crusade's mages to transport you to the surface!) is said..
    NPC_ARGENT_MAGE             = 36097,                    // Summoned along with Tirion B
    NPC_VARIAN                  = 34990,
    NPC_GARROSH                 = 34995,
    NPC_FIZZLEBANG              = 35458,
    NPC_OPEN_PORTAL_TARGET      = 17965,
    NPC_WORLD_TRIGGER_LARGE     = 22517,                    // Used for Lich King summon event
    NPC_THE_LICHKING            = 16980,
    NPC_THE_LICHKING_VISUAL     = 35877,
    NPC_RAMSEY_1                = 34816,
    NPC_RAMSEY_2                = 35035,
    NPC_RAMSEY_3                = 35766,
    NPC_RAMSEY_4                = 35770,
    NPC_RAMSEY_5                = 35771,
    NPC_RAMSEY_6                = 35895,                    // Unknown what these three NPCs are used for, maybe horde events?
    NPC_RAMSEY_7                = 35909,
    NPC_RAMSEY_8                = 35910,

    NPC_PURPLE_RUNE             = 35651,

    GO_MAIN_GATE                = 195647,
    GO_WEST_GATE                = 195648,                   // entrance gate
    GO_SOUTH_GATE               = 195649,                   // south and north doors are used to allow the Champions to enter the arena
    GO_NORTH_GATE               = 195650,
    GO_COLISEUM_FLOOR           = 195527,
    GO_WEB_DOOR                 = 195485,
    GO_PORTAL_DALARAN           = 195682,

    GO_CRUSADERS_CACHE          = 195631,
    GO_CRUSADERS_CACHE_25       = 195632,
    GO_CRUSADERS_CACHE_10_H     = 195633,
    GO_CRUSADERS_CACHE_25_H     = 195635,

    GO_TRIBUTE_CHEST_10H_01     = 195665,
    GO_TRIBUTE_CHEST_10H_25     = 195666,
    GO_TRIBUTE_CHEST_10H_45     = 195667,
    GO_TRIBUTE_CHEST_10H_50     = 195668,

    GO_TRIBUTE_CHEST_25H_01     = 195669,
    GO_TRIBUTE_CHEST_25H_25     = 195670,
    GO_TRIBUTE_CHEST_25H_45     = 195671,
    GO_TRIBUTE_CHEST_25H_50     = 195672,

    SPELL_OPEN_PORTAL           = 67864,
    SPELL_FEL_LIGHTNING_KILL    = 67888,
    SPELL_WILFRED_PORTAL        = 68424,
    SPELL_ARTHAS_PORTAL         = 51807,
    SPELL_FROSTNOVA             = 68198,
    SPELL_CORPSE_TELEPORT       = 69016, // NYI
    SPELL_DESTROY_FLOOR_KNOCKUP = 68193,

    DISPLAYID_DESTROYED_FLOOR   = 9060,
    POINT_COMBAT_POSITION       = 10,

    WORLD_STATE_WIPES           = 4390,
    WORLD_STATE_WIPES_COUNT     = 4389,
};

static const float aRamsayPositions[2][4] =
{
    {559.1528f, 90.55729f, 395.2734f, 5.078908f},           // Summon Position
    {563.556f,  78.72571f, 395.2125f, 0}                    // Movement Position
};

static const float aSpawnPositions[][4] =
{
    {563.8941f, 137.3333f, 405.8467f, 0},                   // Beast combat stalker (Summoned when SAY_VARIAN_BEAST_1)
    {563.9358f, 229.8299f, 394.8061f, 4.694936f},           // Gormok (vehicle) (Summoned when SAY_VARIAN_BEAST_1)
    {564.3301f, 232.1549f, 394.8188f, 1.621917f},           // Dreadscale (Summoned when Tirion says SAY_TIRION_BEAST_2)
    {549.5139f, 170.1389f, 394.7965f, 5.009095f},           // Acidmaw (Summoned(?) 14s after Dreadscale)
    {563.6081f, 228.1491f, 394.7057f, 4.664022f},           // Icehowl (Summoned when SAY_TIRION_BEAST_3)
    {563.6007f, 208.5278f, 395.2696f, 4.729842f},           // Fizzlebang
    {563.8264f, 140.6563f, 393.9861f, 4.694936f},           // Jaraxxus
    {571.684f,  204.9028f, 399.263f,  4.590216f},           // Fjola
    {555.4514f, 205.8889f, 399.2634f, 4.886922f},           // Eydis
    {563.6996f, 175.9826f, 394.5042f, 4.694936f},           // World Trigger Large
    {563.5712f, 174.8351f, 394.4954f, 4.712389f},           // Lich King
    {563.6858f, 139.4323f, 393.9862f, 4.694936f},           // Purple Rune / Center Position
    {648.9169f, 131.0209f, 141.6159f, 0},                   // Tirion B
    {649.1610f, 142.0399f, 141.3060f, 0},                   // Argent mage
};

static const float aMovePositions[][3] =
{
    {563.748f,  179.766f,  394.4862f},                      // Gormok
    {576.5347f, 168.9514f, 394.7064f},                      // Dreadscale
    {563.8577f, 176.5885f, 394.4417f},                      // Icehowl
    {563.7223f, 131.2344f, 393.9901f},                      // Jaraxxus
};
#endif

/* Copyright (C) 2006 - 2013 ScriptDev2 <http://www.scriptdev2.com/>
 * This program is free software licensed under GPL version 2
 * Please see the included DOCS/LICENSE.TXT for more information */

#ifndef DEF_OBSIDIAN_SANCTUM_H
#define DEF_OBSIDIAN_SANCTUM_H

enum
{
    MAX_ENCOUNTER               = 1,

    TYPE_SARTHARION_EVENT       = 1,
    // internal used types for achievement
    TYPE_ALIVE_DRAGONS          = 2,
    TYPE_VOLCANO_BLOW_FAILED    = 3,

    TYPE_DATA_PORTAL_ON         = 4,
    TYPE_DATA_PORTAL_OFF        = 5,
    TYPE_DATA_PORTAL_STATUS     = 6,

    DATA64_FIRE_CYCLONE         = 0,

    MAX_TWILIGHT_DRAGONS        = 3,

    TYPE_PORTAL_TENEBRON        = 0,
    TYPE_PORTAL_SHADRON         = 1,
    TYPE_PORTAL_VESPERON        = 2,

    NPC_SARTHARION              = 28860,
    NPC_TENEBRON                = 30452,
    NPC_SHADRON                 = 30451,
    NPC_VESPERON                = 30449,

    NPC_FIRE_CYCLONE            = 30648,

    GO_TWILIGHT_PORTAL          = 193988,

    // Achievement related
    ACHIEV_CRIT_VOLCANO_BLOW_N  = 7326,                     // achievs 2047, 2048 (Go When the Volcano Blows) -- This is individual achievement!
    ACHIEV_CRIT_VOLCANO_BLOW_H  = 7327,
    ACHIEV_DRAGONS_ALIVE_1_N    = 7328,                     // achievs 2049, 2052 (Twilight Assist)
    ACHIEV_DRAGONS_ALIVE_1_H    = 7331,
    ACHIEV_DRAGONS_ALIVE_2_N    = 7329,                     // achievs 2050, 2053 (Twilight Duo)
    ACHIEV_DRAGONS_ALIVE_2_H    = 7332,
    ACHIEV_DRAGONS_ALIVE_3_N    = 7330,                     // achievs 2051, 2054 (The Twilight Zone)
    ACHIEV_DRAGONS_ALIVE_3_H    = 7333,
};

#endif

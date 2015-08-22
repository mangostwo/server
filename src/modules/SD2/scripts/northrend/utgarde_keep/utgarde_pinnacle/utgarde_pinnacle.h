/* Copyright (C) 2006 - 2013 ScriptDev2 <http://www.scriptdev2.com/>
 * This program is free software licensed under GPL version 2
 * Please see the included DOCS/LICENSE.TXT for more information */

#ifndef DEF_UTG_PINNACLE_H
#define DEF_UTG_PINNACLE_H

enum
{
    MAX_ENCOUNTER                   = 4,
    MAX_SPECIAL_ACHIEV_CRITS        = 3,

    TYPE_SVALA                      = 0,
    TYPE_GORTOK                     = 1,
    TYPE_SKADI                      = 2,
    TYPE_YMIRON                     = 3,

    TYPE_ACHIEV_INCREDIBLE_HULK     = 4,
    TYPE_ACHIEV_LOVE_SKADI          = 5,
    TYPE_ACHIEV_KINGS_BANE          = 6,

    DATA64_GORTHOK_EVENT_STARTER    = 0,
    DATA64_SKADI_MOBS_TRIGGER       = 1,

    GO_STASIS_GENERATOR             = 188593,
    GO_DOOR_SKADI                   = 192173,
    GO_DOOR_YMIRON                  = 192174,

    NPC_WORLD_TRIGGER               = 22515,

    NPC_GRAUF                       = 26893,
    NPC_SKADI                       = 26693,
    NPC_YMIRJAR_WARRIOR             = 26690,
    NPC_YMIRJAR_WITCH_DOCTOR        = 26691,
    NPC_YMIRJAR_HARPOONER           = 26692,
    // NPC_FLAME_BREATH_TRIGGER     = 28351,            // triggers the freezing cloud spell in script
    // NPC_WORLD_TRIGGER_LARGE      = 23472,            // only one spawn in this instance - casts 49308 during the gauntlet event

    NPC_FURBOLG                     = 26684,
    NPC_WORGEN                      = 26683,
    NPC_JORMUNGAR                   = 26685,
    NPC_RHINO                       = 26686,

    // Ymiron spirits
    NPC_BJORN                       = 27303,            // front right
    NPC_HALDOR                      = 27307,            // front left
    NPC_RANULF                      = 27308,            // back left
    NPC_TORGYN                      = 27309,            // back right

    ACHIEV_CRIT_INCREDIBLE_HULK     = 7322,             // Svala, achiev - 2043
    ACHIEV_CRIT_GIRL_LOVES_SKADI    = 7595,             // Skadi, achiev - 2156
    ACHIEV_CRIT_KINGS_BANE          = 7598,             // Ymiron, achiev - 2157

    ACHIEV_START_SKADI_ID           = 17726,            // Starts Skadi timed achiev - 1873

    // Gortok event spells
    SPELL_ORB_VISUAL                = 48044,
    SPELL_AWAKEN_SUBBOSS            = 47669,
    SPELL_AWAKEN_GORTOK             = 47670,

    // Skadi event spells
    // The reset check spell is cast by npc 23472 every 7 seconds during the event
    // If the spell doesn't hit any player then the event resets
    // SPELL_GAUNTLET_RESET_CHECK   = 49308,            // for the moment we don't use this because of the lack of core support
};
#endif

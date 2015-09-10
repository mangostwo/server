/* Copyright (C) 2006 - 2013 ScriptDev2 <http://www.scriptdev2.com/>
 * This program is free software licensed under GPL version 2
 * Please see the included DOCS/LICENSE.TXT for more information */

#ifndef DEF_OCULUS_H
#define DEF_OCULUS_H

/* Encounters
 * Drakos           = 0
 * Varos            = 1
 * Urom             = 2
 * Eregos           = 3
 */

enum
{
    MAX_ENCOUNTER                   = 4,

    TYPE_DRAKOS                     = 0,
    TYPE_VAROS                      = 1,
    TYPE_UROM                       = 2,
    TYPE_EREGOS                     = 3,
    TYPE_DATA_SHIELD_BROKEN         = MAX_ENCOUNTER,

    DATA_CONSTRUCTS_EVENT           = 1,                // DO NOT CHANGE! Used by Acid. - used to check the Centrifuge Constructs alive

    NPC_DRAKOS                      = 27654,
    NPC_VAROS                       = 27447,
    NPC_UROM                        = 27655,
    NPC_EREGOS                      = 27656,
    NPC_CENTRIFUGE_CONSTRUCT        = 27641,

    NPC_ETERNOS                     = 27659,            // bronze
    NPC_VERDISA                     = 27657,            // emerald
    NPC_BELGARISTRASZ               = 27658,            // ruby
    NPC_IMAGE_OF_BELGARISTRASZ      = 28012,

    // Vehicle entries
    NPC_EMERALD_DRAKE               = 27692,
    NPC_AMBER_DRAKE                 = 27755,
    NPC_RUBY_DRAKE                  = 27756,

    // Cages in which the friendly dragons are hold
    GO_DRAGON_CAGE_DOOR             = 193995,

    // Loot
    GO_CACHE_EREGOS                 = 191349,
    GO_CACHE_EREGOS_H               = 193603,

    SPELL_DEATH_SPELL               = 50415,            // summons 28012

    // Instance event yells
    SAY_EREGOS_SPAWN                = -1578010,

    // world states to show how many constructs are still alive
    WORLD_STATE_CONSTRUCTS          = 3524,
    WORLD_STATE_CONSTRUCTS_COUNT    = 3486,

    ACHIEV_START_EREGOS_ID          = 18153,            // eregos timed kill achiev
};

static const float aOculusBossSpawnLocs[2][4] =
{
    {1177.47f, 937.722f, 527.405f, 2.21657f},           // Urom
    {1077.04f, 1086.21f, 655.497f, 4.18879f}            // Eregos
};
#endif

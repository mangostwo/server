/* Copyright (C) 2006 - 2013 ScriptDev2 <http://www.scriptdev2.com/>
 * This program is free software licensed under GPL version 2
 * Please see the included DOCS/LICENSE.TXT for more information */

#ifndef DEF_RUBY_SANCTUM_H
#define DEF_RUBY_SANCTUM_H

enum
{
    MAX_ENCOUNTER                   = 4,

    TYPE_SAVIANA                    = 0,
    TYPE_BALTHARUS                  = 1,
    TYPE_ZARITHRIAN                 = 2,
    TYPE_HALION                     = 3,

    TYPE_DATA_IS_25MAN              = MAX_ENCOUNTER,

    DATA64_SUMMON_FLAMECALLERS      = 0,

    NPC_HALION_REAL                 = 39863,            // Halion - Physical Realm NPC
    NPC_HALION_TWILIGHT             = 40142,            // Halion - Twilight Realm NPC
    NPC_HALION_CONTROLLER           = 40146,

    NPC_SHADOW_ORB_1                = 40083,            // shadow orbs for Halion encounter
    NPC_SHADOW_ORB_2                = 40100,
    NPC_SHADOW_ORB_3                = 40468,            // heroic only
    NPC_SHADOW_ORB_4                = 40469,            // heroic only

    NPC_SAVIANA                     = 39747,            // minibosses
    NPC_BALTHARUS                   = 39751,
    NPC_ZARITHRIAN                  = 39746,

    NPC_XERESTRASZA                 = 40429,            // friendly npc, used for some cinematic and quest
    NPC_ZARITHRIAN_SPAWN_STALKER    = 39794,

    GO_TWILIGHT_PORTAL_ENTER_1      = 202794,           // Portals used in the Halion encounter
    GO_TWILIGHT_PORTAL_ENTER_2      = 202795,
    GO_TWILIGHT_PORTAL_LEAVE        = 202796,

    GO_FIRE_FIELD                   = 203005,           // Xerestrasza flame door
    GO_FLAME_WALLS                  = 203006,           // Zarithrian flame walls
    GO_FLAME_RING                   = 203007,           // Halion flame ring
    GO_TWILIGHT_FLAME_RING          = 203624,           // Halion flame ring - twilight version

    GO_BURNING_TREE_1               = 203036,           // Trees which burn when Halion appears
    GO_BURNING_TREE_2               = 203037,
    GO_BURNING_TREE_3               = 203035,
    GO_BURNING_TREE_4               = 203034,

    // Spells used to summon Halion
    SPELL_FIRE_PILLAR               = 76006,
    SPELL_FIERY_EXPLOSION           = 76010,
    SPELL_CLEAR_DEBUFFS             = 75396,            // cast by the controller on encounter reset

    SAY_HALION_SPAWN                = -1724024,

    // world state to show corporeality in Halion encounter - phase 3
    WORLD_STATE_CORP_PHYSICAL       = 5049,
    WORLD_STATE_CORP_TWILIGHT       = 5050,
    WORLD_STATE_CORPOREALITY        = 5051,

    SPELL_SUMMON_FLAMECALLER        = 74398,
};

#endif

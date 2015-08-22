/* Copyright (C) 2006 - 2013 ScriptDev2 <http://www.scriptdev2.com/>
 * This program is free software licensed under GPL version 2
 * Please see the included DOCS/LICENSE.TXT for more information */

#ifndef DEF_DRAKTHARON_KEEP_H
#define DEF_DRAKTHARON_KEEP_H

enum
{
    MAX_ENCOUNTER                   = 4,

    TYPE_TROLLGORE                  = 0,
    TYPE_NOVOS                      = 1,
    TYPE_KING_DRED                  = 2,
    TYPE_THARONJA                   = 3,
    TYPE_DATA_NOVOS_CRYSTAL_INDEX   = MAX_ENCOUNTER,
    TYPE_DO_TROLLGORE               = MAX_ENCOUNTER + 1,

    DATA64_NOVOS_SUMMON_DUMMY       = 0,
    DATA64_NOVOS_CRYSTAL_HANDLER    = 1,

    NPC_NOVOS                       = 26631,
    NPC_KING_DRED                   = 27483,
    NPC_TROLLGORE                   = 26630,

    // Adds of King Dred Encounter - deaths counted for achievement
    NPC_DRAKKARI_GUTRIPPER          = 26641,
    NPC_DRAKKARI_SCYTHECLAW         = 26628,
    NPC_WORLD_TRIGGER               = 22515,

    // Novos Encounter
    SPELL_BEAM_CHANNEL              = 52106,
    SPELL_CRYSTAL_HANDLER_DEATH_1   = 47336,
    SPELL_CRYSTAL_HANDLER_DEATH_2   = 55801,
    SPELL_CRYSTAL_HANDLER_DEATH_3   = 55803,
    SPELL_CRYSTAL_HANDLER_DEATH_4   = 55805,

    MAX_CRYSTALS                    = 4,
    NPC_CRYSTAL_CHANNEL_TARGET      = 26712,
    GO_CRYSTAL_SW                   = 189299,
    GO_CRYSTAL_NE                   = 189300,
    GO_CRYSTAL_NW                   = 189301,
    GO_CRYSTAL_SE                   = 189302,
};

static const uint32 aCrystalHandlerDeathSpells[MAX_CRYSTALS] =
{SPELL_CRYSTAL_HANDLER_DEATH_1, SPELL_CRYSTAL_HANDLER_DEATH_2, SPELL_CRYSTAL_HANDLER_DEATH_3, SPELL_CRYSTAL_HANDLER_DEATH_4};

#endif

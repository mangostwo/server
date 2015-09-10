/* Copyright (C) 2006 - 2013 ScriptDev2 <http://www.scriptdev2.com/>
 * This program is free software licensed under GPL version 2
 * Please see the included DOCS/LICENSE.TXT for more information */

#ifndef DEF_HALLS_OF_STONE_H
#define DEF_HALLS_OF_STONE_H

enum
{
    MAX_ENCOUNTER           = 4,

    TYPE_TRIBUNAL           = 0,
    TYPE_MAIDEN             = 1,
    TYPE_KRYSTALLUS         = 2,
    TYPE_SJONNIR            = 3,
    TYPE_ACHIEV_BRANN_SPANKIN   = MAX_ENCOUNTER,
    TYPE_DO_SPAWN_DWARF         = MAX_ENCOUNTER + 1,
    TYPE_DO_MARNAK_SPEAK        = MAX_ENCOUNTER + 2,    //order should be met in ScriptedInstance
    TYPE_DO_ABEDNEUM_SPEAK      = MAX_ENCOUNTER + 3,
    TYPE_DO_KADDRAK_SPEAK       = MAX_ENCOUNTER + 4,
    TYPE_DO_MARNAK_ACTIVATE     = MAX_ENCOUNTER + 5,    //order: here too
    TYPE_DO_ABEDNEUM_ACTIVATE   = MAX_ENCOUNTER + 6,
    TYPE_DO_KADDRAK_ACTIVATE    = MAX_ENCOUNTER + 7,

    NPC_BRANN_HOS           = 28070,

    NPC_KADDRAK             = 30898,
    NPC_ABEDNEUM            = 30899,
    NPC_MARNAK              = 30897,
    NPC_TRIBUNAL_OF_AGES    = 28234,
    NPC_WORLDTRIGGER        = 22515,
    NPC_DARK_MATTER         = 28235,                        // used by the Tribunal event
    NPC_LIGHTNING_STALKER   = 28130,                        // used by the Tribunal event as spawn point for the dwarfs
    NPC_IRON_SLUDGE         = 28165,                        // checked in the Sjonnir achiev
    NPC_SJONNIR             = 27978,

    NPC_DARK_RUNE_PROTECTOR     = 27983,
    NPC_DARK_RUNE_STORMCALLER   = 27984,
    NPC_IRON_GOLEM_CUSTODIAN    = 27985,

    GO_DOOR_MAIDEN          = 191292,
    GO_DOOR_TRIBUNAL        = 191294,                       // possibly closed during event?
    GO_DOOR_TO_TRIBUNAL     = 191295,
    GO_DOOR_SJONNIR         = 191296,

    GO_TRIBUNAL_CHEST       = 190586,
    GO_TRIBUNAL_CHEST_H     = 193996,

    GO_TRIBUNAL_HEAD_RIGHT  = 191670,                       // marnak
    GO_TRIBUNAL_HEAD_CENTER = 191669,                       // abedneum
    GO_TRIBUNAL_HEAD_LEFT   = 191671,                       // kaddrak

    GO_TRIBUNAL_CONSOLE     = 193907,
    GO_TRIBUNAL_FLOOR       = 191527,

    GO_SJONNIR_CONSOLE      = 193906,

    SPELL_DARK_MATTER_START = 51001,                        // Channeled spells used by the Tribunal event

    MAX_FACES               = 3,
    FACE_MARNAK             = 0,
    FACE_ABEDNEUM           = 1,
    FACE_KADDRAK            = 2,

    MAX_ACHIEV_SLUDGES      = 5,

    ACHIEV_START_MAIDEN_ID  = 20383,

    ACHIEV_CRIT_BRANN       = 7590,                         // Brann, achiev 2154
    ACHIEV_CRIT_ABUSE_OOZE  = 7593,                         // Snonnir, achiev 2155
};
#endif

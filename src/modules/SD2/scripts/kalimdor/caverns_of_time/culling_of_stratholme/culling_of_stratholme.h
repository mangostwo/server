/* Copyright (C) 2006 - 2013 ScriptDev2 <http://www.scriptdev2.com/>
 * This program is free software licensed under GPL version 2
 * Please see the included DOCS/LICENSE.TXT for more information */

#ifndef DEF_CULLING_OF_STRATHOLME_H
#define DEF_CULLING_OF_STRATHOLME_H

enum
{
    MAX_ENCOUNTER                   = 10,
    MAX_SCOURGE_WAVES               = 10,
    MAX_SCOURGE_TYPE_PER_WAVE       = 4,

    TYPE_GRAIN_EVENT                = 0,                    // crates with plagued grain identified
    TYPE_ARTHAS_INTRO_EVENT         = 1,                    // Arhas Speech and Walk to Gates and short intro with MalGanis
    TYPE_MEATHOOK_EVENT             = 2,                    // Waves 1-5
    TYPE_SALRAMM_EVENT              = 3,                    // Waves 6-10
    TYPE_ARTHAS_TOWNHALL_EVENT      = 4,                    // Townhall escort event
    TYPE_EPOCH_EVENT                = 5,                    // Townhall Event, Boss Killed
    TYPE_ARTHAS_ESCORT_EVENT        = 6,                    // Burning city escort event
    TYPE_MALGANIS_EVENT             = 7,                    // Malganis
    TYPE_INFINITE_CORRUPTER_TIME    = 8,                    // Time for 25min Timer
    TYPE_INFINITE_CORRUPTER         = 9,                    // Infinite corruptor event
    TYPE_DO_AREATRIGGER             = 10,
    TYPE_DATA64_PLAYER_BOTH         = 11,
    TYPE_DATA64_PLAYER_ARTHAS       = 12,
    TYPE_DATA64_AD_TARGET           = 13,
    TYPE_DATA64_SPAWNING            = 14,

    // Main Encounter NPCs
    NPC_CHROMIE_INN                 = 26527,
    NPC_CHROMIE_ENTRANCE            = 27915,
    NPC_CHROMIE_END                 = 30997,
    NPC_HOURGLASS                   = 28656,
    NPC_LORDAERON_CRIER             = 27913,
    NPC_ARTHAS                      = 26499,

    // Dungeon bosses
    NPC_MEATHOOK                    = 26529,
    NPC_SALRAMM_THE_FLESHCRAFTER    = 26530,
    NPC_LORD_EPOCH                  = 26532,
    NPC_MALGANIS                    = 26533,

    // Inn Event related NPC
    NPC_MICHAEL_BELFAST             = 30571,
    NPC_HEARTHSINGER_FORRESTEN      = 30551,
    NPC_FRAS_SIABI                  = 30552,
    NPC_FOOTMAN_JAMES               = 30553,
    NPC_MAL_CORRICKS                = 31017,
    NPC_GRYAN_STOUTMANTLE           = 30561,

    // Grain Event NPCs
    NPC_ROGER_OWENS                 = 27903,
    NPC_SERGEANT_MORIGAN            = 27877,
    NPC_JENA_ANDERSON               = 27885,
    NPC_MALCOM_MOORE                = 27891,                // Not (yet?) spawned
    NPC_BARTLEBY_BATTSON            = 27907,
    NPC_GRAIN_CRATE_HELPER          = 27827,
    // NPC_CRATES_BUNNY             = 30996,

    // Intro Event NPCs
    NPC_JAINA_PROUDMOORE            = 26497,
    NPC_UTHER_LIGHTBRINGER          = 26528,
    NPC_KNIGHT_SILVERHAND           = 28612,
    NPC_LORDAERON_FOOTMAN           = 27745,
    NPC_HIGH_ELF_MAGE_PRIEST        = 27747,
    NPC_STRATHOLME_CITIZEN          = 28167,
    NPC_STRATHOLME_RESIDENT         = 28169,

    // Mobs in Stratholme (to despawn) -- only here for sake of completeness handling remains open (mangos feature)
    NPC_MAGISTRATE_BARTHILAS        = 30994,
    NPC_STEPHANIE_SINDREE           = 31019,
    NPC_LEEKA_TURNER                = 31027,
    NPC_SOPHIE_AAREN                = 31021,
    NPC_ROBERT_PIERCE               = 31025,
    NPC_GEORGE_GOODMAN              = 31022,

    // Others NPCs in Stratholme
    NPC_EMERY_NEILL                 = 30570,
    NPC_EDWARD_ORRICK               = 31018,
    NPC_OLIVIA_ZENITH               = 31020,

    // Townhall Event NPCs
    NPC_AGIATED_STRATHOLME_CITIZEN  = 31126,
    NPC_AGIATED_STRATHOLME_RESIDENT = 31127,
    NPC_PATRICIA_O_REILLY           = 31028,

    // Scourge waves
    NPC_ENRAGING_GHOUL              = 27729,
    NPC_ACOLYTE                     = 27731,
    NPC_MASTER_NECROMANCER          = 27732,
    NPC_CRYPT_FIEND                 = 27734,
    NPC_PATCHWORK_CONSTRUCT         = 27736,
    NPC_TOMB_STALKER                = 28199,
    NPC_DARK_NECROMANCER            = 28200,
    NPC_BILE_GOLEM                  = 28201,
    NPC_DEVOURING_GHOUL             = 28249,
    NPC_ZOMBIE                      = 27737,
    // NPC_INVISIBLE_STALKER        = 20562,

    // Infinite dragons
    NPC_TOWNHALL_CITIZEN            = 28340,
    NPC_TOWNHALL_RESIDENT           = 28341,
    NPC_INFINITE_ADVERSARY          = 27742,
    NPC_INFINITE_AGENT              = 27744,
    NPC_INFINITE_HUNTER             = 27743,
    NPC_TIME_RIFT                   = 28409,
    NPC_TIME_RIFT_BIG               = 28439,

    // Heroic event npcs
    NPC_INFINITE_CORRUPTER          = 32273,
    NPC_GUARDIAN_OF_TIME            = 32281,

    // Gameobjects
    GO_DOOR_BOOKCASE                = 188686,
    GO_CITY_ENTRANCE_GATE           = 191788,
    GO_DARK_RUNED_CHEST             = 190663,
    GO_DARK_RUNED_CHEST_H           = 193597,

    GO_SUSPICIOUS_GRAIN_CRATE       = 190094,
    GO_CRATE_HIGHLIGHT              = 190117,
    GO_PLAGUE_GRAIN_CRATE           = 190095,

    // World States
    WORLD_STATE_CRATES              = 3479,
    WORLD_STATE_CRATES_COUNT        = 3480,
    WORLD_STATE_WAVE                = 3504,
    WORLD_STATE_TIME                = 3932,
    WORLD_STATE_TIME_COUNTER        = 3931,

    // Areatrigger
    AREATRIGGER_INN                 = 5291,
    /*
    5085 before bridge - could be Uther SpawnPos
    5148 ini entrance
    5181 ini exit
    5249 fras siabis store
    5250 leeking shields...(store)
    5251 bar in stratholme
    5252 Aaren flowers
    5253 Angelicas boutique
    5256 townhall
    5291 Inn */

    // Achievements
    ACHIEV_CRIT_ZOMBIEFEST          = 7180,                 // achiev 1872
};

enum CoSArcaneDisruption
{
    SAY_SOLDIERS_REPORT = -1595000,

    SPELL_ARCANE_DISRUPTION = 49590,
    SPELL_CRATES_KILL_CREDIT = 58109,
};

#endif

/* Copyright (C) 2006 - 2013 ScriptDev2 <http://www.scriptdev2.com/>
 * This program is free software licensed under GPL version 2
 * Please see the included DOCS/LICENSE.TXT for more information */

#ifndef DEF_AZJOL_NERUB_H
#define DEF_AZJOL_NERUB_H

enum
{
    MAX_ENCOUNTER               = 3,

    TYPE_KRIKTHIR               = 0,
    TYPE_HADRONOX               = 1,
    TYPE_ANUBARAK               = 2,
    TYPE_DO_HADRONOX            = MAX_ENCOUNTER,

    DATA64_ANUB_TRIGGER         = 0,
    DATA64_ANUB_ASSASIN         = 1,
    DATA64_ANUB_GUARDIAN        = 2,
    DATA64_ANUB_DARTER          = 3,

    NPC_KRIKTHIR                = 28684,
    NPC_HADRONOX                = 28921,
    NPC_ANUBARAK                = 29120,

    SAY_SEND_GROUP_1            = -1601004,
    SAY_SEND_GROUP_2            = -1601005,
    SAY_SEND_GROUP_3            = -1601006,

    NPC_GASHRA                  = 28730,
    NPC_NARJIL                  = 28729,
    NPC_SILTHIK                 = 28731,
    NPC_ANUBAR_CRUSHER          = 28922,

    NPC_WORLD_TRIGGER           = 22515,
    NPC_WORLD_TRIGGER_LARGE     = 23472,

    GO_DOOR_KRIKTHIR            = 192395,
    GO_DOOR_ANUBARAK_1          = 192396,
    GO_DOOR_ANUBARAK_2          = 192397,
    GO_DOOR_ANUBARAK_3          = 192398,

    SAY_CRUSHER_AGGRO           = -1601025,
    SAY_CRUSHER_SPECIAL         = -1601026,

    ACHIEV_START_ANUB_ID        = 20381,

    ACHIEV_CRITERIA_WATCH_DIE   = 4240,         // Krikthir, achiev 1296
    ACHIEV_CRITERIA_DENIED      = 4244,         // Hadronox, achiev 1297
};

#endif

/* Copyright (C) 2006 - 2013 ScriptDev2 <http://www.scriptdev2.com/>
 * This program is free software licensed under GPL version 2
 * Please see the included DOCS/LICENSE.TXT for more information */

#ifndef DEF_HALLS_OF_LIGHTNING_H
#define DEF_HALLS_OF_LIGHTNING_H

enum
{
    MAX_ENCOUNTER           = 4,

    TYPE_BJARNGRIM          = 0,
    TYPE_VOLKHAN            = 1,
    TYPE_IONAR              = 2,
    TYPE_LOKEN              = 3,

    NPC_BJARNGRIM           = 28586,
    NPC_VOLKHAN             = 28587,
    NPC_IONAR               = 28546,
    NPC_LOKEN               = 28923,
    NPC_VOLKHAN_ANVIL       = 28823,

    GO_VOLKHAN_DOOR         = 191325,                       //_doors07
    GO_IONAR_DOOR           = 191326,                       //_doors05
    // GO_LOKEN_DOOR         = 191324,                      //_doors02
    GO_LOKEN_THRONE         = 192654,

    ACHIEV_START_LOKEN_ID   = 20384,

    ACHIEV_CRIT_LIGHTNING   = 6835,                         // Bjarngrim, achiev 1834
    ACHIEV_CRIT_RESISTANT   = 7321,                         // Volkhan, achiev 2042
};
#endif

/* Copyright (C) 2006 - 2013 ScriptDev2 <http://www.scriptdev2.com/>
 * This program is free software licensed under GPL version 2
 * Please see the included DOCS/LICENSE.TXT for more information */

#ifndef DEF_NEXUS_H
#define DEF_NEXUS_H

enum
{
    MAX_ENCOUNTER                  = 4,
    MAX_SPECIAL_ACHIEV_CRITS       = 2,

    TYPE_TELESTRA                  = 0,
    TYPE_ANOMALUS                  = 1,
    TYPE_ORMOROK                   = 2,
    TYPE_KERISTRASZA               = 3,
    TYPE_INTENSE_COLD_FAILED       = 4,

    TYPE_ACHIEV_CHAOS_THEORY       = 5,     //order to be met in ScriptedInstance
    TYPE_ACHIEV_SPLIT_PERSONALITY  = 6,

    NPC_TELESTRA                   = 26731,
    NPC_ANOMALUS                   = 26763,
    NPC_ORMOROK                    = 26794,
    NPC_KERISTRASZA                = 26723,

    GO_CONTAINMENT_SPHERE_TELESTRA = 188526,
    GO_CONTAINMENT_SPHERE_ANOMALUS = 188527,
    GO_CONTAINMENT_SPHERE_ORMOROK  = 188528,

    SPELL_FROZEN_PRISON             = 47854,

    ACHIEV_CRIT_CHAOS_THEORY        = 7316,                 // Anomalus, achiev 2037
    ACHIEV_CRIT_INTENSE_COLD        = 7315,                 // Keristrasza, achiev 2036
    ACHIEV_CRIT_SPLIT_PERSONALITY   = 7577,                 // Telestra, achiev 2150
};
#endif

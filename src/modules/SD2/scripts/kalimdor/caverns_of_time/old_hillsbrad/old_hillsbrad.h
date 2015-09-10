/**
 * ScriptDev2 is an extension for mangos providing enhanced features for
 * area triggers, creatures, game objects, instances, items, and spells beyond
 * the default database scripting in mangos.
 *
 * Copyright (C) 2006-2013  ScriptDev2 <http://www.scriptdev2.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

#ifndef DEF_OLD_HILLSBRAD_H
#define DEF_OLD_HILLSBRAD_H

enum
{
    MAX_ENCOUNTER                   = 7,
    MAX_BARRELS                     = 5,
    MAX_WIPE_COUNTER                = 20,

    TYPE_BARREL_DIVERSION           = 0,                    // barrel event
    TYPE_DRAKE                      = 1,                    // first boss
    TYPE_SKARLOC                    = 2,                    // prison to keep - boss
    TYPE_ESCORT_BARN                = 3,                    // keep to barn
    TYPE_ESCORT_INN                 = 4,                    // barn to inn
    TYPE_EPOCH                      = 5,                    // inn event
    TYPE_THRALL_EVENT               = 6,                    // global event
    TYPE_THRALL_EVENT_COUNT         = MAX_ENCOUNTER,

    // event npcs
    NPC_THRALL                      = 17876,
    NPC_TARETHA                     = 18887,
    NPC_EROZION                     = 18723,
    NPC_LODGE_QUEST_TRIGGER         = 20155,
    NPC_ARMORER                     = 18764,
    NPC_ORC_PRISONER                = 18598,

    NPC_TARREN_MILL_GUARDSMAN       = 18092,
    NPC_TARREN_MILL_PROTECTOR       = 18093,
    NPC_TARREN_MILL_LOOKOUT         = 18094,
    NPC_YOUNG_BLANCHY               = 18651,

    // bosses
    NPC_DRAKE                       = 17848,
    NPC_SKARLOC                     = 17862,
    NPC_EPOCH                       = 18096,

    GO_ROARING_FLAME                = 182592,
    GO_PRISON_DOOR                  = 184393,

    QUEST_ENTRY_HILLSBRAD           = 10282,
    QUEST_ENTRY_DIVERSION           = 10283,
    QUEST_ENTRY_ESCAPE              = 10284,
    QUEST_ENTRY_RETURN              = 10285,

    WORLD_STATE_OH                  = 2436,
};

#endif

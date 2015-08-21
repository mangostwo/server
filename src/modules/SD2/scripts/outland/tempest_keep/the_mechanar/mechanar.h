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

#ifndef DEF_MECHANAR_H
#define DEF_MECHANAR_H

enum
{
    MAX_ENCOUNTER           = 5,
    MAX_BRIDGE_LOCATIONS    = 7,
    MAX_BRIDGE_TRASH        = 4,

    TYPE_GYRO_KILL          = 0,
    TYPE_IRON_HAND          = 1,
    TYPE_CAPACITUS          = 2,
    TYPE_SEPETHREA          = 3,
    TYPE_PATHALEON          = 4,

    NPC_GYRO_KILL           = 19218,
    NPC_IRON_HAND           = 19710,
    NPC_LORD_CAPACITUS      = 19219,
    // NPC_SEPETHREA         = 19221,
    NPC_PATHALEON           = 19220,

    // bridge event related
    NPC_ASTROMAGE           = 19168,
    NPC_PHYSICIAN           = 20990,
    NPC_CENTURION           = 19510,
    NPC_ENGINEER            = 20988,
    NPC_NETHERBINDER        = 20059,
    NPC_FORGE_DESTROYER     = 19735,

    GO_FACTORY_ELEVATOR     = 183788,

    SPELL_ETHEREAL_TELEPORT = 34427,

    SAY_PATHALEON_INTRO     = -1554028,
};

#endif

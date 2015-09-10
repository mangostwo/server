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

#ifndef DEF_SERPENT_SHRINE_H
#define DEF_SERPENT_SHRINE_H

enum
{
    MAX_ENCOUNTER                   = 6,
    MAX_SPELLBINDERS                = 3,

    TYPE_HYDROSS_EVENT              = 0,
    TYPE_KARATHRESS_EVENT           = 1,
    TYPE_LADYVASHJ_EVENT            = 2,
    TYPE_LEOTHERAS_EVENT            = 3,
    TYPE_MOROGRIM_EVENT             = 4,
    TYPE_THELURKER_EVENT            = 5,
    TYPE_DO_HANDLE_BEAMS            = MAX_ENCOUNTER,
    TYPE_DO_VASHJ_GENERATORS        = MAX_ENCOUNTER+1,

    DATA_WATERSTATE_EVENT           = 1,                    // DO NOT CHANGE! Used by Acid. - used to check the mobs for the water event.

    // NPC_KARATHRESS                = 21214,
    NPC_CARIBDIS                    = 21964,
    NPC_SHARKKIS                    = 21966,
    NPC_TIDALVESS                   = 21965,
    NPC_LEOTHERAS                   = 21215,
    NPC_LADYVASHJ                   = 21212,
    NPC_GREYHEART_SPELLBINDER       = 21806,
    NPC_HYDROSS_BEAM_HELPER         = 21933,
    NPC_HYDROSS_THE_UNSTABLE        = 21216,
    NPC_SHIELD_GENERATOR            = 19870,

    // waterstate event related
    NPC_COILFANG_PRIESTESS          = 21220,
    NPC_COILFANG_SHATTERER          = 21301,
    NPC_VASHJIR_HONOR_GUARD         = 21218,
    NPC_GREYHEART_TECHNICIAN        = 21263,

    GO_SHIELD_GENERATOR_1           = 185051,
    GO_SHIELD_GENERATOR_2           = 185052,
    GO_SHIELD_GENERATOR_3           = 185053,
    GO_SHIELD_GENERATOR_4           = 185054,

    // Objects and doors no longer used since 2.4.0
    // GO_CONSOLE_HYDROSS            = 185117,
    // GO_CONSOLE_LURKER             = 185118,
    // GO_CONSOLE_LEOTHERAS          = 185115,
    // GO_CONSOLE_KARATHRESS         = 185114,
    // GO_CONSOLE_MOROGRIM           = 185116,
    // GO_CONSOLE_VASHJ              = 184568,
    // GO_BRIDGE_PART_1              = 184203,
    // GO_BRIDGE_PART_2              = 184204,
    // GO_BRIDGE_PART_3              = 184205,

    SPELL_LEOTHERAS_BANISH          = 37546,
    SPELL_BLUE_BEAM                 = 38015,
    SPELL_MAGIC_BARRIER             = 38112,
};

#endif

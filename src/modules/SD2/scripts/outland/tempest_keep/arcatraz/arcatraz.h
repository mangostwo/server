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

#ifndef DEF_ARCATRAZ_H
#define DEF_ARCATRAZ_H

enum
{
    MAX_ENCOUNTER                   = 10,
    MAX_WARDENS                     = 7,

    TYPE_ENTRANCE                   = 0,
    TYPE_ZEREKETH                   = 1,
    TYPE_DALLIAH                    = 2,
    TYPE_SOCCOTHRATES               = 3,
    TYPE_HARBINGERSKYRISS           = 4,                    // Handled with ACID (FAIL of 20905, 20906, 20908, 20909, 20910, 20911)
    TYPE_WARDEN_1                   = 5,                    // Handled with ACID (20905 - Blazing Trickster, 20906 - Phase-Hunter)
    TYPE_WARDEN_2                   = 6,
    TYPE_WARDEN_3                   = 7,                    // Handled with ACID (20908 - Akkiris Lightning-Waker, 20909 - Sulfuron Magma-Thrower)
    TYPE_WARDEN_4                   = 8,                    // Handled with ACID (20910 - Twilight Drakonaar, 20911 - Blackwing Drakonaar)
    TYPE_WARDEN_5                   = 9,

    NPC_DALLIAH                     = 20885,
    NPC_SOCCOTHRATES                = 20886,
    NPC_MELLICHAR                   = 20904,                // Skyriss will kill this unit
    NPC_PRISON_APHPA_POD            = 21436,
    NPC_PRISON_BETA_POD             = 21437,
    NPC_PRISON_DELTA_POD            = 21438,
    NPC_PRISON_GAMMA_POD            = 21439,
    NPC_PRISON_BOSS_POD             = 21440,

    // intro event related
    NPC_PROTEAN_NIGHTMARE           = 20864,
    NPC_PROTEAN_HORROR              = 20865,
    NPC_ARCATRAZ_WARDEN             = 20859,
    NPC_ARCATRAZ_DEFENDER           = 20857,

    // Harbinger Skyriss event related (trash mobs are scripted in ACID)
    NPC_BLAZING_TRICKSTER           = 20905,                // phase 1
    NPC_PHASE_HUNTER                = 20906,
    NPC_MILLHOUSE                   = 20977,                // phase 2
    NPC_AKKIRIS                     = 20908,                // phase 3
    NPC_SULFURON                    = 20909,
    NPC_TW_DRAKONAAR                = 20910,                // phase 4
    NPC_BL_DRAKONAAR                = 20911,
    NPC_SKYRISS                     = 20912,                // phase 5

    GO_CORE_SECURITY_FIELD_ALPHA    = 184318,               // Door opened when Wrath-Scryer Soccothrates dies
    GO_CORE_SECURITY_FIELD_BETA     = 184319,               // Door opened when Dalliah the Doomsayer dies
    GO_SEAL_SPHERE                  = 184802,               // Shield 'protecting' mellichar
    GO_POD_ALPHA                    = 183961,               // Pod first boss wave
    GO_POD_BETA                     = 183963,               // Pod second boss wave
    GO_POD_DELTA                    = 183964,               // Pod third boss wave
    GO_POD_GAMMA                    = 183962,               // Pod fourth boss wave
    GO_POD_OMEGA                    = 183965,               // Pod fifth boss wave

    SPELL_TARGET_OMEGA              = 36852,                // Visual spell used by Mellichar
};

static const float aDalliahStartPos[4] = {118.6038f, 96.84682f, 22.44115f, 1.012f};
static const float aSoccotharesStartPos[4] = {122.1035f, 192.7203f, 22.44115f, 5.235f};

#endif

/* Copyright (C) 2006 - 2013 ScriptDev2 <http://www.scriptdev2.com/>
 * This program is free software licensed under GPL version 2
 * Please see the included DOCS/LICENSE.TXT for more information */

#ifndef DEF_TRIAL_OF_THE_CHAMPION_H
#define DEF_TRIAL_OF_THE_CHAMPION_H

enum
{
    MAX_ENCOUNTER                       = 3,
    MAX_CHAMPIONS_AVAILABLE             = 5,
    MAX_CHAMPIONS_ARENA                 = 3,
    MAX_CHAMPIONS_MOUNTS                = 24,

    TYPE_GRAND_CHAMPIONS                = 0,
    TYPE_ARGENT_CHAMPION                = 1,
    TYPE_BLACK_KNIGHT                   = 2,
    TYPE_DO_PREPARE_CHAMPIONS           = MAX_ENCOUNTER,

    // event handler
    NPC_ARELAS_BRIGHTSTAR               = 35005,                        // alliance
    NPC_JAEREN_SUNSWORN                 = 35004,                        // horde
    NPC_TIRION_FORDRING                 = 34996,
    NPC_VARIAN_WRYNN                    = 34990,
    NPC_THRALL                          = 34994,
    NPC_GARROSH                         = 34995,

    // champions alliance
    NPC_ALLIANCE_WARRIOR                = 34705,                        // Jacob Alerius
    NPC_ALLIANCE_WARRIOR_MOUNT          = 35637,                        // Jacob vehicle mount
    NPC_ALLIANCE_WARRIOR_CHAMPION       = 35328,                        // Stormwind Champion
    NPC_ALLIANCE_MAGE                   = 34702,                        // Ambrose Boltspark
    NPC_ALLIANCE_MAGE_MOUNT             = 35633,                        // Ambrose vehicle mount
    NPC_ALLIANCE_MAGE_CHAMPION          = 35331,                        // Gnomeregan Champion
    NPC_ALLIANCE_SHAMAN                 = 34701,                        // Colosos
    NPC_ALLIANCE_SHAMAN_MOUNT           = 35768,                        // Colosos vehicle mount
    NPC_ALLIANCE_SHAMAN_CHAMPION        = 35330,                        // Exodar Champion
    NPC_ALLIANCE_HUNTER                 = 34657,                        // Jaelyne Evensong
    NPC_ALLIANCE_HUNTER_MOUNT           = 34658,                        // Jaelyne vehicle mount
    NPC_ALLIANCE_HUNTER_CHAMPION        = 35332,                        // Darnassus Champion
    NPC_ALLIANCE_ROGUE                  = 34703,                        // Lana Stouthammer
    NPC_ALLIANCE_ROGUE_MOUNT            = 35636,                        // Lana vehicle mount
    NPC_ALLIANCE_ROGUE_CHAMPION         = 35329,                        // Ironforge Champion

    // champions horde
    NPC_HORDE_WARRIOR                   = 35572,                        // Mokra Skullcrusher
    NPC_HORDE_WARRIOR_MOUNT             = 35638,                        // Mokra vehicle mount
    NPC_HORDE_WARRIOR_CHAMPION          = 35314,                        // Orgrimmar Champion
    NPC_HORDE_MAGE                      = 35569,                        // Eressea Dawnsinger
    NPC_HORDE_MAGE_MOUNT                = 35635,                        // Eressea vehicle mount
    NPC_HORDE_MAGE_CHAMPION             = 35326,                        // Silvermoon Champion
    NPC_HORDE_SHAMAN                    = 35571,                        // Runok Wildmane
    NPC_HORDE_SHAMAN_MOUNT              = 35640,                        // Runok vehicle mount
    NPC_HORDE_SHAMAN_CHAMPION           = 35325,                        // Thunder Bluff Champion
    NPC_HORDE_HUNTER                    = 35570,                        // Zul'tore
    NPC_HORDE_HUNTER_MOUNT              = 35641,                        // Zul'tore vehicle mount
    NPC_HORDE_HUNTER_CHAMPION           = 35323,                        // Sen'jin Champion
    NPC_HORDE_ROGUE                     = 35617,                        // Deathstalker Visceri
    NPC_HORDE_ROGUE_MOUNT               = 35634,                        // Visceri vehicle mount
    NPC_HORDE_ROGUE_CHAMPION            = 35327,                        // Undercity Champion

    // spectators triggers
    NPC_WORLD_TRIGGER                   = 22515,
    NPC_SPECTATOR_GENERIC               = 35016,
    NPC_SPECTATOR_HORDE                 = 34883,
    NPC_SPECTATOR_ALLIANCE              = 34887,
    NPC_SPECTATOR_HUMAN                 = 34900,
    NPC_SPECTATOR_ORC                   = 34901,
    NPC_SPECTATOR_TROLL                 = 34902,
    NPC_SPECTATOR_TAUREN                = 34903,
    NPC_SPECTATOR_BLOOD_ELF             = 34904,
    NPC_SPECTATOR_UNDEAD                = 34905,
    NPC_SPECTATOR_DWARF                 = 34906,
    NPC_SPECTATOR_DRAENEI               = 34908,
    NPC_SPECTATOR_NIGHT_ELF             = 34909,
    NPC_SPECTATOR_GNOME                 = 34910,

    // mounts
    NPC_WARHORSE_ALLIANCE               = 36557,                        // alliance mount vehicle
    NPC_WARHORSE_HORDE                  = 35644,                        // dummy - part of the decorations
    NPC_BATTLEWORG_ALLIANCE             = 36559,                        // dummy - part of the decorations
    NPC_BATTLEWORG_HORDE                = 36558,                        // horde mount vehicle

    // argent challegers
    NPC_EADRIC                          = 35119,
    NPC_PALETRESS                       = 34928,
    // trash mobs
    NPC_ARGENT_LIGHTWIELDER             = 35309,
    NPC_ARGENT_MONK                     = 35305,
    NPC_ARGENT_PRIESTESS                = 35307,

    // black knight
    NPC_BLACK_KNIGHT                    = 35451,
    NPC_BLACK_KNIGHT_GRYPHON            = 35491,
    // risen zombies
    NPC_RISEN_JAEREN                    = 35545,
    NPC_RISEN_ARELAS                    = 35564,
    NPC_RISEN_CHAMPION                  = 35590,

    // doors
    GO_MAIN_GATE                        = 195647,
    GO_NORTH_GATE                       = 195650,                       // combat door

    // chests
    GO_CHAMPIONS_LOOT                   = 195709,
    GO_CHAMPIONS_LOOT_H                 = 195710,
    GO_EADRIC_LOOT                      = 195374,
    GO_EADRIC_LOOT_H                    = 195375,
    GO_PALETRESS_LOOT                   = 195323,
    GO_PALETRESS_LOOT_H                 = 195324,

    // fireworks
    GO_FIREWORKS_RED_1                  = 180703,
    GO_FIREWORKS_RED_2                  = 180708,
    GO_FIREWORKS_BLUE_1                 = 180720,
    GO_FIREWORKS_BLUE_2                 = 180723,
    GO_FIREWORKS_WHITE_1                = 180728,
    GO_FIREWORKS_WHITE_2                = 180730,
    GO_FIREWORKS_YELLOW_1               = 180736,
    GO_FIREWORKS_YELLOW_2               = 180738,

    // area triggers - purpose unk
    // AREATRIGGER_ID_TOC_1             = 5491,
    // AREATRIGGER_ID_TOC_2             = 5492,

    // emotes
    EMOTE_BLOOD_ELVES                   = -1650018,
    EMOTE_TROLLS                        = -1650019,
    EMOTE_TAUREN                        = -1650020,
    EMOTE_UNDEAD                        = -1650021,
    EMOTE_ORCS                          = -1650022,
    EMOTE_DWARVES                       = -1650023,
    EMOTE_GNOMES                        = -1650024,
    EMOTE_NIGHT_ELVES                   = -1650025,
    EMOTE_HUMANS                        = -1650026,
    EMOTE_DRAENEI                       = -1650027,
};
#endif

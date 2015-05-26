# MaNGOS is a full featured server for World of Warcraft, supporting
# the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
#
# Copyright (C) 2005-2015  MaNGOS project <http://getmangos.eu>
# Copyright (C) 2006-2013  ScriptDev2 project <http://www.scriptdev2.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

include(MacroMangosSourceGroup)

#Add PCH Files
set(mangosscript_PCH "${CMAKE_CURRENT_SOURCE_DIR}/precompiled.h")
if(PCH)
    LIST(APPEND mangosscript_PCH "${CMAKE_CURRENT_SOURCE_DIR}/precompiled.cpp")
endif()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/base
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/shared
    ${CMAKE_SOURCE_DIR}/src/framework
    ${CMAKE_SOURCE_DIR}/src/game/Server
    ${CMAKE_SOURCE_DIR}/src/game/BattleGround
    ${CMAKE_SOURCE_DIR}/src/game/WorldHandlers
    ${CMAKE_SOURCE_DIR}/src/game/Object
    ${CMAKE_SOURCE_DIR}/src/game/References
    ${CMAKE_SOURCE_DIR}/src/game/MotionGenerators
    ${CMAKE_SOURCE_DIR}/src/game/vmap
    ${CMAKE_SOURCE_DIR}/src/shared
    ${CMAKE_SOURCE_DIR}/src/shared/Utilities
    ${CMAKE_SOURCE_DIR}/src/shared/Log
    ${CMAKE_SOURCE_DIR}/src/shared/DataStores
    ${CMAKE_SOURCE_DIR}/src/shared/Common
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src/shared
    ${ACE_INCLUDE_DIR}
    ${MYSQL_INCLUDE_DIR}
)

# Define the SD2 library
set(mangosscript_LIB_SRCS ${mangosscript_PCH})

#Include Files
file(GLOB sources_include include/*.cpp include/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_include})
    source_group("Include Files" FILES ${sources_include})

#System Files
file(GLOB sources_system system/*.cpp system/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_system})
    source_group("System Files" FILES ${sources_system})

#Base Files
file(GLOB sources_base base/*.cpp base/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_base})
    source_group("Base Files" FILES ${sources_base})

#Battleground Scripts
file(GLOB sources_battlegrounds scripts/battlegrounds/*.cpp scripts/battlegrounds/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_battlegrounds})
    source_group("Battleground Scripts" FILES ${sources_battlegrounds})

#World Scripts
file(GLOB sources_world scripts/world/*.cpp scripts/world/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_world})
    source_group("World Scripts" FILES ${sources_world})

#Eastern Kingdom Scripts
file(GLOB sources_eastern_kingdoms scripts/eastern_kingdoms/*.cpp scripts/eastern_kingdoms/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_eastern_kingdoms})
    source_group("Eastern Kingdoms Scripts" FILES ${sources_eastern_kingdoms})

#Kalimdor Scripts
file(GLOB sources_kalimdor scripts/kalimdor/*.cpp scripts/kalimdor/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_kalimdor})
    source_group("Kalimdor Scripts" FILES ${sources_kalimdor})
  
#Outlands Scripts
file(GLOB sources_outlands scripts/outland/*.cpp scripts/outland/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_outlands})
    source_group("Outlands Scripts" FILES ${sources_outlands})
  
#Northrend Scripts
file(GLOB sources_northrend scripts/northrend/*.cpp scripts/northrend/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_northrend})
    source_group("Northrend Scripts" FILES ${sources_northrend})

#Instance: Blackrock Depths Scripts
file(GLOB sources_instance_ek_brd scripts/eastern_kingdoms/blackrock_mountain/blackrock_depths/*.cpp scripts/eastern_kingdoms/blackrock_mountain/blackrock_depths/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_ek_brd})
    source_group("Eastern Kingdoms Scripts\\Instances\\Blackrock Depths" FILES ${sources_instance_ek_brd})

#Instance: Blackrock Spire Scripts
file(GLOB sources_instance_ek_brs scripts/eastern_kingdoms/blackrock_mountain/blackrock_spire/*.cpp scripts/eastern_kingdoms/blackrock_mountain/blackrock_spire/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_ek_brs})
    source_group("Eastern Kingdoms Scripts\\Instances\\Blackrock Spire" FILES ${sources_instance_ek_brs})

#Instance: Deadmines Scripts
file(GLOB sources_instance_ek_tdm scripts/eastern_kingdoms/deadmines/*.cpp scripts/eastern_kingdoms/deadmines/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_ek_tdm})
    source_group("Eastern Kingdoms Scripts\\Instances\\Deadmines" FILES ${sources_instance_ek_tdm})

#Instance: Gnomeregan Scripts
file(GLOB sources_instance_ek_gno scripts/eastern_kingdoms/gnomeregan/*.cpp scripts/eastern_kingdoms/gnomeregan/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_ek_gno})
    source_group("Eastern Kingdoms Scripts\\Instances\\Gnomeregan" FILES ${sources_instance_ek_gno})

#Instance: Scarlet Monastery Scripts
file(GLOB sources_instance_ek_sm scripts/eastern_kingdoms/scarlet_monastery/*.cpp scripts/eastern_kingdoms/scarlet_monastery/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_ek_sm})
    source_group("Eastern Kingdoms Scripts\\Instances\\Scarlet Monastery" FILES ${sources_instance_ek_sm})

#Instance: Scholomance Scripts
file(GLOB sources_instance_ek_scholo scripts/eastern_kingdoms/scholomance/*.cpp scripts/eastern_kingdoms/scholomance/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_ek_scholo})
    source_group("Eastern Kingdoms Scripts\\Instances\\Scholomance" FILES ${sources_instance_ek_scholo})

#Instance: Uldaman Scripts
file(GLOB sources_instance_ek_uld scripts/eastern_kingdoms/uldaman/*.cpp scripts/eastern_kingdoms/uldaman/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_ek_uld})
    source_group("Eastern Kingdoms Scripts\\Instances\\Uldaman" FILES ${sources_instance_ek_uld})

#Instance: Shadowfang Keep Scripts
file(GLOB sources_instance_ek_sfk scripts/eastern_kingdoms/shadowfang_keep/*.cpp scripts/eastern_kingdoms/shadowfang_keep/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_ek_sfk})
    source_group("Eastern Kingdoms Scripts\\Instances\\Shadowfang Keep" FILES ${sources_instance_ek_sfk})

#Instance: Sunken Temple Scripts
file(GLOB sources_instance_ek_st scripts/eastern_kingdoms/sunken_temple/*.cpp scripts/eastern_kingdoms/sunken_temple/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_ek_st})
    source_group("Eastern Kingdoms Scripts\\Instances\\Sunken Temple" FILES ${sources_instance_ek_st})

#Instance: Stratholme Scripts
file(GLOB sources_instance_ek_strat scripts/eastern_kingdoms/stratholme/*.cpp scripts/eastern_kingdoms/stratholme/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_ek_strat})
    source_group("Eastern Kingdoms Scripts\\Instances\\Stratholme" FILES ${sources_instance_ek_strat})

#Instance: Magisters Terrace Scripts
file(GLOB sources_instance_ek_mt scripts/eastern_kingdoms/magisters_terrace/*.cpp scripts/eastern_kingdoms/magisters_terrace/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_ek_mt})
    source_group("Eastern Kingdoms Scripts\\Instances\\Magisters Terrace" FILES ${sources_instance_ek_mt})
  
#Raid: Karazhan Scripts
file(GLOB sources_raid_ek_kara scripts/eastern_kingdoms/karazhan/*.cpp scripts/eastern_kingdoms/karazhan/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_raid_ek_kara})
    source_group("Eastern Kingdoms Scripts\\Raids\\Karazhan" FILES ${sources_raid_ek_kara})
  
#Raid: Sunwell Plateau Scripts
file(GLOB sources_raid_ek_sp scripts/eastern_kingdoms/sunwell_plateau/*.cpp scripts/eastern_kingdoms/sunwell_plateau/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_raid_ek_sp})
    source_group("Eastern Kingdoms Scripts\\Raids\\Sunwell Plateau" FILES ${sources_raid_ek_sp})
  
#Raid: Zul'Aman Scripts
file(GLOB sources_raid_ek_za scripts/eastern_kingdoms/zulaman/*.cpp scripts/eastern_kingdoms/zulaman/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_raid_ek_za})
    source_group("Eastern Kingdoms Scripts\\Raids\\Zul'Aman" FILES ${sources_raid_ek_za})
  
#Raid: Molten Core Scripts
file(GLOB sources_raid_ek_mc scripts/eastern_kingdoms/blackrock_mountain/molten_core/*.cpp scripts/eastern_kingdoms/blackrock_mountain/molten_core/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_raid_ek_mc})
    source_group("Eastern Kingdoms Scripts\\Raids\\Molten Core" FILES ${sources_raid_ek_mc})

#Raid: Blackwing Lair Scripts
file(GLOB sources_raid_ek_bwl scripts/eastern_kingdoms/blackrock_mountain/blackwing_lair/*.cpp scripts/eastern_kingdoms/blackrock_mountain/blackwing_lair/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_raid_ek_bwl})
    source_group("Eastern Kingdoms Scripts\\Raids\\Blackwing Lair" FILES ${sources_raid_ek_bwl})

#Raid: Zul Gurub Scripts
file(GLOB sources_raid_ek_zg scripts/eastern_kingdoms/zulgurub/*.cpp scripts/eastern_kingdoms/zulgurub/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_raid_ek_zg})
    source_group("Eastern Kingdoms Scripts\\Raids\\Zul Gurub" FILES ${sources_raid_ek_zg})

#Instance: Blackfathom Deeps Scripts
file(GLOB sources_instance_k_bdf scripts/kalimdor/blackfathom_deeps/*.cpp scripts/kalimdor/blackfathom_deeps/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_k_bdf})
    source_group("Kalimdor Scripts\\Instances\\Blackfathom Deeps" FILES ${sources_instance_k_bdf})

#Instance: Dire Maul Scripts
file(GLOB sources_instance_k_dm scripts/kalimdor/dire_maul/*.cpp scripts/kalimdor/dire_maul/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_k_dm})
    source_group("Kalimdor Scripts\\Instances\\Dire Maul" FILES ${sources_instance_k_dm})

#Instance: Maraudon Scripts
file(GLOB sources_instance_k_mara scripts/kalimdor/maraudon/*.cpp scripts/kalimdor/maraudon/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_k_mara})
    source_group("Kalimdor Scripts\\Instances\\Maraudon Scripts" FILES ${sources_instance_k_mara})

#Instance: Razorfen Downs Scripts
file(GLOB sources_instance_k_rfd scripts/kalimdor/razorfen_downs/*.cpp scripts/kalimdor/razorfen_downs/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_k_rfd})
    source_group("Kalimdor Scripts\\Instances\\Razorfen Downs" FILES ${sources_instance_k_rfd})

#Instance: Razorfen Kraul Scripts
file(GLOB sources_instance_k_rfk scripts/kalimdor/razorfen_kraul/*.cpp scripts/kalimdor/razorfen_kraul/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_k_rfk})
    source_group("Kalimdor Scripts\\Instances\\Razorfen Kraul" FILES ${sources_instance_k_rfk})

#Instance: Wailing Caverns Scripts
file(GLOB sources_instance_k_wc scripts/kalimdor/wailing_caverns/*.cpp scripts/kalimdor/wailing_caverns/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_k_wc})
    source_group("Kalimdor Scripts\\Instances\\Wailing Caverns" FILES ${sources_instance_k_wc})
  
#Instance: Caverns of Time - Black Morass
file(GLOB sources_instance_k_cot_bm scripts/kalimdor/caverns_of_time/dark_portal/*.cpp scripts/kalimdor/caverns_of_time/dark_portal/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_k_cot_bm})
    source_group("Kalimdor Scripts\\Instances\\Caverns of Time\\Black Morass" FILES ${sources_instance_k_cot_bm})

#Instance: Caverns of Time - Old Hillsbrad
file(GLOB sources_instance_k_cot_oh scripts/kalimdor/caverns_of_time/old_hillsbrad/*.cpp scripts/kalimdor/caverns_of_time/old_hillsbrad/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_k_cot_oh})
    source_group("Kalimdor Scripts\\Instances\\Caverns of Time\\Old Hillsbrad" FILES ${sources_instance_k_cot_oh})

#Instance: Caverns of Time - Culling of Stratholme
file(GLOB sources_instance_k_cot_cos scripts/kalimdor/caverns_of_time/culling_of_stratholme/*.cpp scripts/kalimdor/caverns_of_time/culling_of_stratholme/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_k_cot_cos})
    source_group("Kalimdor Scripts\\Instances\\Caverns of Time\\Culling of Stratholme" FILES ${sources_instance_k_cot_cos})
  
#Instance: Zul Farrak Scripts
file(GLOB sources_instance_k_zf scripts/kalimdor/zulfarrak/*.cpp scripts/kalimdor/zulfarrak/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_k_zf})
    source_group("Kalimdor Scripts\\Instances\\Zul Farrak" FILES ${sources_instance_k_zf})

#Raid: Caverns of Time - Mount Hyjal
file(GLOB sources_instance_k_cot_mh scripts/kalimdor/caverns_of_time/hyjal/*.cpp scripts/kalimdor/caverns_of_time/hyjal/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_k_cot_mh})
    source_group("Kalimdor Scripts\\Raids\\Caverns of Time\\Mount Hyjal" FILES ${sources_instance_k_cot_mh})
  
#Raid: Onyxias Lair Scripts
file(GLOB sources_raid_k_ony scripts/kalimdor/onyxias_lair/*.cpp scripts/kalimdor/onyxias_lair/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_raid_k_ony})
    source_group("Kalimdor Scripts\\Raids\\Onyxias Lair" FILES ${sources_raid_k_ony})

#Raid: Ruins of AQ Scripts
file(GLOB sources_raid_k_raq scripts/kalimdor/ruins_of_ahnqiraj/*.cpp scripts/kalimdor/ruins_of_ahnqiraj/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_raid_k_raq})
    source_group("Kalimdor Scripts\\Raids\\Ruins of AQ" FILES ${sources_raid_k_raq})

#Raid: Temple of AQ Scripts
file(GLOB sources_raid_k_taq scripts/kalimdor/temple_of_ahnqiraj/*.cpp scripts/kalimdor/temple_of_ahnqiraj/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_raid_k_taq})
    source_group("Kalimdor Scripts\\Raids\\Temple of AQ" FILES ${sources_raid_k_taq})

#Instance: Auchindoun - Auchenai Crypts
file(GLOB sources_instance_ol_aac scripts/outland/auchindoun/auchenai_crypts/*.cpp scripts/outland/auchindoun/auchenai_crypts/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_ol_aac})
    source_group("Outlands Scripts\\Instances\\Auchindoun\\Auchenai Crypts" FILES ${sources_instance_ol_aac})

#Instance: Auchindoun - Mana Tombs
file(GLOB sources_instance_ol_amt scripts/outland/auchindoun/mana_tombs/*.cpp scripts/outland/auchindoun/mana_tombs/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_ol_amt})
    source_group("Outlands Scripts\\Instances\\Auchindoun\\Mana Tombs" FILES ${sources_instance_ol_amt})
  
#Instance: Auchindoun - Sethekk Halls
file(GLOB sources_instance_ol_ash scripts/outland/auchindoun/sethekk_halls/*.cpp scripts/outland/auchindoun/sethekk_halls/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_ol_ash})
    source_group("Outlands Scripts\\Instances\\Auchindoun\\Sethekk Halls" FILES ${sources_instance_ol_ash})
  
#Instance: Auchindoun - Shadow Labyrinth
file(GLOB sources_instance_ol_asl scripts/outland/auchindoun/shadow_labyrinth/*.cpp scripts/outland/auchindoun/shadow_labyrinth/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_ol_asl})
    source_group("Outlands Scripts\\Instances\\Auchindoun\\Shadow Labyrinth" FILES ${sources_instance_ol_asl})

#Instance: Coilfang Reservoir - Slave Pens
file(GLOB sources_instance_ol_csp scripts/outland/coilfang_reservoir/slave_pens/*.cpp scripts/outland/coilfang_reservoir/slave_pens/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_ol_csp})
    source_group("Outlands Scripts\\Instances\\Coilfang Reservoir\\Slave Pens" FILES ${sources_instance_ol_csp})
  
#Instance: Coilfang Reservoir - Steam Vaults
file(GLOB sources_instance_ol_cst scripts/outland/coilfang_reservoir/steam_vault/*.cpp scripts/outland/coilfang_reservoir/steam_vault/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_ol_cst})
    source_group("Outlands Scripts\\Instances\\Coilfang Reservoir\\Steam Vaults" FILES ${sources_instance_ol_cst})
  
#Instance: Coilfang Reservoir - Underbog
file(GLOB sources_instance_ol_cu scripts/outland/coilfang_reservoir/underbog/*.cpp scripts/outland/coilfang_reservoir/underbog/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_ol_cu})
    source_group("Outlands Scripts\\Instances\\Coilfang Reservoir\\Underbog" FILES ${sources_instance_ol_cu})
  
#Instance: Hellfire Citadel - Blood Furnace
file(GLOB sources_raid_ol_hbf scripts/outland/hellfire_citadel/blood_furnace/*.cpp scripts/outland/hellfire_citadel/blood_furnace/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_raid_ol_hbf})
    source_group("Outlands Scripts\\Instances\\Hellfire Citadel\\Blood Furnace" FILES ${sources_raid_ol_hbf})
  
#Instance: Hellfire Citadel - Hellfire Ramparts
file(GLOB sources_raid_ol_hhr scripts/outland/hellfire_citadel/hellfire_ramparts/*.cpp scripts/outland/hellfire_citadel/hellfire_ramparts/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_raid_ol_hhr})
    source_group("Outlands Scripts\\Instances\\Hellfire Citadel\\Hellfire Ramparts" FILES ${sources_raid_ol_hhr})
  
#Instance: Hellfire Citadel - Shattered Halls
file(GLOB sources_raid_ol_hsh scripts/outland/hellfire_citadel/shattered_halls/*.cpp scripts/outland/hellfire_citadel/shattered_halls/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_raid_ol_hsh})
    source_group("Outlands Scripts\\Instances\\Hellfire Citadel\\Shattered Halls" FILES ${sources_raid_ol_hsh})  

#Instance: Tempest Keep - Arcatraz
file(GLOB sources_raid_ol_ta scripts/outland/tempest_keep/arcatraz/*.cpp scripts/outland/tempest_keep/arcatraz/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_raid_ol_ta})
    source_group("Outlands Scripts\\Instances\\Tempest Keep\\Arcatraz" FILES ${sources_raid_ol_ta})
  
#Instance: Tempest Keep - Botanica
file(GLOB sources_raid_ol_tb scripts/outland/tempest_keep/botanica/*.cpp scripts/outland/tempest_keep/botanica/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_raid_ol_tb})
    source_group("Outlands Scripts\\Instances\\Tempest Keep\\Botanica" FILES ${sources_raid_ol_tb})
  
#Instance: Tempest Keep - The Mechanar
file(GLOB sources_raid_ol_ttm scripts/outland/tempest_keep/the_mechanar/*.cpp scripts/outland/tempest_keep/the_mechanar/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_raid_ol_ttm})
    source_group("Outlands Scripts\\Instances\\Tempest Keep\\The Mechanar" FILES ${sources_raid_ol_ttm})
  
#Raid: Hellfire Citadel - Magtheridons Lair
file(GLOB sources_raid_ol_hml scripts/outland/hellfire_citadel/magtheridons_lair/*.cpp scripts/outland/hellfire_citadel/magtheridons_lair/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_raid_ol_hml})
    source_group("Outlands Scripts\\Raids\\Hellfire Citadel\\Magtheridons Lair" FILES ${sources_raid_ol_hml})
  
#Raid: Tempest Keep - The Eye
file(GLOB sources_raid_ol_tte scripts/outland/tempest_keep/the_eye/*.cpp scripts/outland/tempest_keep/the_eye/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_raid_ol_tte})
    source_group("Outlands Scripts\\Raids\\Tempest Keep\\The Eye" FILES ${sources_raid_ol_tte})
  
#Raid: Coilfang Reservoir - Serpentshrine Cavern
file(GLOB sources_raid_ol_csc scripts/outland/coilfang_reservoir/serpent_shrine/*.cpp scripts/outland/coilfang_reservoir/serpent_shrine/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_raid_ol_csc})
    source_group("Outlands Scripts\\Raids\\Coilfang Reservoir\\Serpentshrine Cavern" FILES ${sources_raid_ol_csc})
  
#Raid: Black Temple
file(GLOB sources_raid_ol_bt scripts/outland/black_temple/*.cpp scripts/outland/black_temple/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_raid_ol_bt})
    source_group("Outlands Scripts\\Raids\\Black Temple" FILES ${sources_raid_ol_bt})
  
#Raid: Gruul's Lair
file(GLOB sources_raid_ol_gl scripts/outland/gruuls_lair/*.cpp scripts/outland/gruuls_lair/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_raid_ol_gl})
    source_group("Outlands Scripts\\Raids\\Gruul's Lair" FILES ${sources_raid_ol_gl})

#Instance: Azjol Nerub
file(GLOB sources_instance_nr_aznj scripts/northrend/azjol-nerub/azjol-nerub/*.cpp scripts/northrend/azjol-nerub/azjol-nerub/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_nr_aznj})
    source_group("Northrend Scripts\\Instances\\Azjol Nerub" FILES ${sources_instance_nr_aznj})

#Instance: Ahn'Kahet
file(GLOB sources_instance_nr_ahnk scripts/northrend/azjol-nerub/ahnkahet/*.cpp scripts/northrend/azjol-nerub/ahnkahet/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_nr_ahnk})
    source_group("Northrend Scripts\\Instances\\Ahn'Kahet" FILES ${sources_instance_nr_ahnk})

#Instance: Trial of the Champion
file(GLOB sources_instance_nr_totch scripts/northrend/crusaders_coliseum/trial_of_the_champion/*.cpp scripts/northrend/crusaders_coliseum/trial_of_the_champion/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_nr_totch})
    source_group("Northrend Scripts\\Instances\\Trial of the Champion" FILES ${sources_instance_nr_totch})

#Instance: Drak'Tharon Keep
file(GLOB sources_instance_nr_dtk scripts/northrend/draktharon_keep/*.cpp scripts/northrend/draktharon_keep/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_nr_dtk})
    source_group("Northrend Scripts\\Instances\\Drak'Tharon Keep" FILES ${sources_instance_nr_dtk})
  
#Instance: Gundrak
file(GLOB sources_instance_nr_gdk scripts/northrend/gundrak/*.cpp scripts/northrend/gundrak/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_nr_gdk})
    source_group("Northrend Scripts\\Instances\\Gundrak" FILES ${sources_instance_nr_gdk})
  
#Instance: Frozen Halls: Forge of Souls
file(GLOB sources_instance_nr_fh_fos scripts/northrend/icecrown_citadel/frozen_halls/forge_of_souls/*.cpp scripts/northrend/icecrown_citadel/frozen_halls/forge_of_souls/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_nr_fh_fos})
    source_group("Northrend Scripts\\Instances\\Forge of Souls" FILES ${sources_instance_nr_fh_fos})

#Instance: Frozen Halls: Halls of Reflection
file(GLOB sources_instance_nr_fh_hor scripts/northrend/icecrown_citadel/frozen_halls/halls_of_reflection/*.cpp scripts/northrend/icecrown_citadel/frozen_halls/halls_of_reflection/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_nr_fh_hor})
    source_group("Northrend Scripts\\Instances\\Halls of Reflection" FILES ${sources_instance_nr_fh_hor})

#Instance: Frozen Halls: Pit of Saron
file(GLOB sources_instance_nr_fh_pos scripts/northrend/icecrown_citadel/frozen_halls/pit_of_saron/*.cpp scripts/northrend/icecrown_citadel/frozen_halls/pit_of_saron/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_nr_fh_pos})
    source_group("Northrend Scripts\\Instances\\Pit of Saron" FILES ${sources_instance_nr_fh_pos})
  
#Instance: The Nexus
file(GLOB sources_instance_nr_nex scripts/northrend/nexus/nexus/*.cpp scripts/northrend/nexus/nexus/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_nr_nex})
    source_group("Northrend Scripts\\Instances\\The Nexus" FILES ${sources_instance_nr_nex})
  
#Instance: The Oculus
file(GLOB sources_instance_nr_oculus scripts/northrend/nexus/oculus/*.cpp scripts/northrend/nexus/oculus/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_nr_oculus})
    source_group("Northrend Scripts\\Instances\\The Oculus" FILES ${sources_instance_nr_oculus})
  
#Instance: Halls of Lightning
file(GLOB sources_instance_nr_uld_hol scripts/northrend/ulduar/halls_of_lightning/*.cpp scripts/northrend/ulduar/halls_of_lightning/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_nr_uld_hol})
    source_group("Northrend Scripts\\Instances\\Halls of Lightning" FILES ${sources_instance_nr_uld_hol})
  
#Instance: Halls of Stone
file(GLOB sources_instance_nr_uld_hos scripts/northrend/ulduar/halls_of_stone/*.cpp scripts/northrend/ulduar/halls_of_stone/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_nr_uld_hos})
    source_group("Northrend Scripts\\Instances\\Halls of Stone" FILES ${sources_instance_nr_uld_hos})
  
#Instance: Utgarde Keep
file(GLOB sources_instance_nr_uk scripts/northrend/utgarde_keep/utgarde_keep/*.cpp scripts/northrend/utgarde_keep/utgarde_keep/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_nr_uk})
    source_group("Northrend Scripts\\Instances\\Utgarde Keep" FILES ${sources_instance_nr_uk})

#Instance: Utgarde Pinnacle
file(GLOB sources_instance_nr_up scripts/northrend/utgarde_keep/utgarde_pinnacle/*.cpp scripts/northrend/utgarde_keep/utgarde_pinnacle/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_nr_up})
    source_group("Northrend Scripts\\Instances\\Utgarde Pinnacle" FILES ${sources_instance_nr_up})
  
#Instance: Violet Hold
file(GLOB sources_instance_nr_vh scripts/northrend/violet_hold/*.cpp scripts/northrend/violet_hold/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_instance_nr_vh})
    source_group("Northrend Scripts\\Instances\\Violet Hold" FILES ${sources_instance_nr_vh})

#Raid: Naxxramas
file(GLOB sources_raid_nr_naxx scripts/northrend/naxxramas/*.cpp scripts/northrend/naxxramas/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_raid_nr_naxx})
    source_group("Northrend Scripts\\Raids\\Naxxramas" FILES ${sources_raid_nr_naxx})
  
#Raid: Vault of Archavon
file(GLOB sources_raid_nr_voa scripts/northrend/vault_of_archavon/*.cpp scripts/northrend/vault_of_archavon/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_raid_nr_voa})
    source_group("Northrend Scripts\\Raids\\Vault of Archavon" FILES ${sources_raid_nr_voa})
  
#Raid: Obsidian Sanctum
file(GLOB sources_raid_nr_os scripts/northrend/obsidian_sanctum/*.cpp scripts/northrend/obsidian_sanctum/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_raid_nr_os})
    source_group("Northrend Scripts\\Raids\\Obsidian Sanctum" FILES ${sources_raid_nr_os})
  
#Raid: Ruby Sanctum
file(GLOB sources_raid_nr_rs scripts/northrend/ruby_sanctum/*.cpp scripts/northrend/ruby_sanctum/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_raid_nr_rs})
    source_group("Northrend Scripts\\Raids\\Ruby Sanctum" FILES ${sources_raid_nr_rs})
  
#Raid: Icecrown Citadel
file(GLOB sources_raid_nr_icc scripts/northrend/icecrown_citadel/icecrown_citadel/*.cpp scripts/northrend/icecrown_citadel/icecrown_citadel/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_raid_nr_icc})
    source_group("Northrend Scripts\\Raids\\Icecrown Citadel" FILES ${sources_raid_nr_icc})
  
#Raid: Trial of the Crusader
file(GLOB sources_raid_nr_totc scripts/northrend/crusaders_coliseum/trial_of_the_crusader/*.cpp scripts/northrend/crusaders_coliseum/trial_of_the_crusader/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_raid_nr_totc})
    source_group("Northrend Scripts\\Raids\\Trial of the Crusader" FILES ${sources_raid_nr_totc})
  
#Raid: Eye of Eternity
file(GLOB sources_raid_nr_eoe scripts/northrend/nexus/eye_of_eternity/*.cpp scripts/northrend/nexus/eye_of_eternity/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_raid_nr_eoe})
    source_group("Northrend Scripts\\Raids\\Eye of Eternity" FILES ${sources_raid_nr_eoe})
  
#Raid: Ulduar
file(GLOB sources_raid_nr_uld scripts/northrend/ulduar/ulduar/*.cpp scripts/northrend/ulduar/ulduar/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_raid_nr_uld})
    source_group("Northrend Scripts\\Raids\\Ulduar" FILES ${sources_raid_nr_uld})
  
#WorldScript: Scarlet Enclave
file(GLOB sources_world_scarlet_enclave scripts/eastern_kingdoms/scarlet_enclave/*.cpp scripts/eastern_kingdoms/scarlet_enclave/*.h)
    LIST(APPEND mangosscript_LIB_SRCS ${sources_world_scarlet_enclave})
    source_group("Eastern Kingdoms Scripts\\World\\Scarlet Enclave" FILES ${sources_world_scarlet_enclave})
    
add_library(mangosscript STATIC ${mangosscript_LIB_SRCS})
  
# Generate PCH
if(PCH)
    ADD_CXX_PCH(mangosscript ${mangosscript_PCH})
endif()

if(NOT ACE_USE_EXTERNAL)
    add_dependencies(mangosscript ace)
endif()

if(UNIX)
    set(mangosscript_LINK_FLAGS "-pthread")
    if(APPLE)
        set(mangosscript_LINK_FLAGS "-framework Carbon ${mangosscript_LINK_FLAGS}")
        # Needed for the linking because of the missing symbols
        set(mangosscript_LINK_FLAGS "-Wl,-undefined -Wl,dynamic_lookup ${mangosscript_LINK_FLAGS}")
    endif()

    if(APPLE)
        set(mangosscript_PROPERTIES INSTALL_NAME_DIR ${LIBS_DIR})
    else()
        set(mangosscript_PROPERTIES INSTALL_RPATH ${LIBS_DIR})
    endif()

    # Run out of build tree
    set(mangosscript_PROPERTIES
        ${mangosscript_PROPERTIES}
        BUILD_WITH_INSTALL_RPATH OFF
    )

    set_target_properties(mangosscript PROPERTIES
        LINK_FLAGS ${mangosscript_LINK_FLAGS}
        ${mangosscript_PROPERTIES}
    )
endif()
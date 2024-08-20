#
# Copyright (C) 2005-2022 MaNGOS project <http://getmangos.com/>
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
#

#Base files
file(GLOB Playerbot_Source ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/*.h)
source_group("Player Bot" FILES ${Playerbot_Source})
file(GLOB AHbot_Source ${CMAKE_CURRENT_SOURCE_DIR}/ahbot/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/ahbot/*.h)
source_group("AH Bot" FILES ${AHbot_Source})

set(SRC_GRP_BOTS
        ${Playerbot_Source}
        ${AHbot_Source}
        )
#Strategy files
file(GLOB Playerbot_Strategy ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/strategy/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/strategy/*.h)
source_group("Player Bot\\Strategies" FILES ${Playerbot_Strategy})
LIST(APPEND SRC_GRP_BOTS ${Playerbot_Strategy})

#Action files
file(GLOB Playerbot_Actions ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/strategy/actions/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/strategy/actions/*.h)
source_group("Player Bot\\Strategies\\Actions" FILES ${Playerbot_Actions})
LIST(APPEND SRC_GRP_BOTS ${Playerbot_Actions})

#Generic files
file(GLOB Playerbot_Generic ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/strategy/generic/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/strategy/generic/*.h)
source_group("Player Bot\\Strategies\\Generic" FILES ${Playerbot_Generic})
LIST(APPEND SRC_GRP_BOTS ${Playerbot_Generic})

#Trigger files
file(GLOB Playerbot_Triggers ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/strategy/triggers/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/strategy/triggers/*.h)
source_group("Player Bot\\Strategies\\Triggers" FILES ${Playerbot_Triggers})
LIST(APPEND SRC_GRP_BOTS ${Playerbot_Triggers})


#Value files
file(GLOB Playerbot_Values ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/strategy/values/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/strategy/values/*.h)
source_group("Player Bot\\Strategies\\Values" FILES ${Playerbot_Values})
LIST(APPEND SRC_GRP_BOTS ${Playerbot_Values})

## Class files
#Deathknight AI
file(GLOB Playerbot_Deathknight ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/strategy/deathknight/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/strategy/deathknight/*.h)
source_group("Player Bot\\Strategies\\Deathknight" FILES ${Playerbot_Deathknight})
LIST(APPEND SRC_GRP_BOTS ${Playerbot_Deathknight})

#Druid AI
file(GLOB Playerbot_Druid ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/strategy/druid/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/strategy/druid/*.h)
source_group("Player Bot\\Strategies\\Druid" FILES ${Playerbot_Druid})
LIST(APPEND SRC_GRP_BOTS ${Playerbot_Druid})

#Hunter AI
file(GLOB Playerbot_Hunter ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/strategy/hunter/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/strategy/hunter/*.h)
source_group("Player Bot\\Strategies\\Hunter" FILES ${Playerbot_Hunter})
LIST(APPEND SRC_GRP_BOTS ${Playerbot_Hunter})

#Mage AI
file(GLOB Playerbot_Mage ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/strategy/mage/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/strategy/mage/*.h)
source_group("Player Bot\\Strategies\\Mage" FILES ${Playerbot_Mage})
LIST(APPEND SRC_GRP_BOTS ${Playerbot_Mage})

#Paladin AI
file(GLOB Playerbot_Paladin ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/strategy/paladin/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/strategy/paladin/*.h)
source_group("Player Bot\\Strategies\\Paladin" FILES ${Playerbot_Paladin})
LIST(APPEND SRC_GRP_BOTS ${Playerbot_Paladin})

#Priest AI
file(GLOB Playerbot_Priest ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/strategy/priest/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/strategy/priest/*.h)
source_group("Player Bot\\Strategies\\Priest" FILES ${Playerbot_Priest})
LIST(APPEND SRC_GRP_BOTS ${Playerbot_Priest})

#Rogue AI
file(GLOB Playerbot_Rogue ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/strategy/rogue/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/strategy/rogue/*.h)
source_group("Player Bot\\Strategies\\Rogue" FILES ${Playerbot_Rogue})
LIST(APPEND SRC_GRP_BOTS ${Playerbot_Rogue})

#Shaman AI
file(GLOB Playerbot_Shaman ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/strategy/shaman/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/strategy/shaman/*.h)
source_group("Player Bot\\Strategies\\Shaman" FILES ${Playerbot_Shaman})
LIST(APPEND SRC_GRP_BOTS ${Playerbot_Shaman})

#Warlock AI
file(GLOB Playerbot_Warlock ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/strategy/warlock/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/strategy/warlock/*.h)
source_group("Player Bot\\Strategies\\Warlock" FILES ${Playerbot_Warlock})
LIST(APPEND SRC_GRP_BOTS ${Playerbot_Warlock})

#Warrior AI
file(GLOB Playerbot_Warrior ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/strategy/warrior/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/playerbot/strategy/warrior/*.h)
source_group("Player Bot\\Strategies\\Warrior" FILES ${Playerbot_Warrior})
LIST(APPEND SRC_GRP_BOTS ${Playerbot_Warrior})

# Install config files
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/playerbot/aiplayerbot.conf.dist.in ${CMAKE_CURRENT_BINARY_DIR}/aiplayerbot.conf.dist)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/aiplayerbot.conf.dist DESTINATION ${CONF_INSTALL_DIR})

add_library(bots STATIC ${SRC_GRP_BOTS})

target_include_directories(bots
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/playerbot
        ${CMAKE_CURRENT_SOURCE_DIR}/ahbot
        )

target_link_libraries(bots
        PUBLIC
        game)

# Generate PCH
if(PCH)
    ADD_CXX_PCH(bots ${CMAKE_CURRENT_SOURCE_DIR}/botpch.h ${CMAKE_CURRENT_SOURCE_DIR}/botpch.cpp)
endif()
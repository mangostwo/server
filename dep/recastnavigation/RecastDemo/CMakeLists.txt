# MaNGOS is a full featured server for World of Warcraft, supporting
# the following clients: 1.12.x, 2.4.3, 3.2.5a, 4.2.3 and 5.4.8
#
# Copyright (C) 2005-2014  MaNGOS project <http://getmangos.eu>
#
# ***** BEGIN GPL LICENSE BLOCK *****
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
# ***** END GPL LICENSE BLOCK *****
#
# World of Warcraft, and all World of Warcraft or Warcraft art, images,
# and lore are copyrighted by Blizzard Entertainment, Inc.

include(MacroMangosSourceGroup)

#-----------------------------------------------------------------------------
# Define the RecastDemo dependencies
find_package(OpenGL REQUIRED)
find_package(SDL REQUIRED)

#-----------------------------------------------------------------------------
# Define the RecastDemo executable
file(GLOB sources Source/*.cpp)
file(GLOB headers Include/*.h)

set(RecastDemo_LIB_SRCS ${sources} ${headers})

mangos_source_group(${RecastDemo_LIB_SRCS})

include_directories(
    ${SDL_INCLUDE_DIR}
    ${OPENGL_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Contrib
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../DebugUtils/Include/
    ${CMAKE_CURRENT_SOURCE_DIR}/../Detour/Include/
    ${CMAKE_CURRENT_SOURCE_DIR}/../Recast/Include/
)

#-----------------------------------------------------------------------------
# Build the RecastDemo executable
if(WIN32)
    add_executable(RecastDemo WIN32 ${RecastDemo_LIB_SRCS})
else()
    add_executable(RecastDemo ${RecastDemo_LIB_SRCS})
endif()
target_link_libraries(RecastDemo DebugUtils Detour Recast ${SDL_LIBRARY} ${OPENGL_LIBRARIES})

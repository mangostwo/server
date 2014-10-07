#
# This code is part of MaNGOS. Contributor & Copyright details are in AUTHORS/THANKS.
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

cmake_minimum_required (VERSION 2.8)

project (MANGOS_VMAP_ASSEMB_IO)

set(EXECUTABLE_NAME "vmap-assembler")

# comment next line to disable debug mode
#TODO: These should be general, not with the -D part for windows to work too?
add_definitions("-DIOMAP_DEBUG")
add_definitions("-DNO_CORE_FUNCS")

include_directories(
    "${CMAKE_SOURCE_DIR}/src/shared"
    "${CMAKE_SOURCE_DIR}/src/game/vmap/"
    "${CMAKE_SOURCE_DIR}/dep/include/g3dlite/"
    "${CMAKE_SOURCE_DIR}/dep/ACE_wrappers/"
    "${CMAKE_BINARY_DIR}/dep/ACE_wrappers"
    "${CMAKE_SOURCE_DIR}/src/framework/"
)

add_executable(${EXECUTABLE_NAME} vmap_assembler.cpp)

target_link_libraries(${EXECUTABLE_NAME} vmap g3dlite z)

install(TARGETS ${EXECUTABLE_NAME} DESTINATION "${BIN_DIR}/${TOOLS_DIR}")

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

project(MANGOS_MAP_EXTRACTOR)

set(EXECUTABLE_NAME "map-extractor")

add_subdirectory(loadlib)

include_directories(
    "${CMAKE_SOURCE_DIR}/dep/libmpq/"
    "${MANGOS_MAP_EXTRACTOR_SOURCE_DIR}/loadlib"
)

link_directories(${MANGOS_MAP_EXTRACTOR_SOURCE_DIR}/loadlib)

add_executable(${EXECUTABLE_NAME} dbcfile.cpp mpq_libmpq.cpp System.cpp)

target_link_libraries(${EXECUTABLE_NAME} libmpq loadlib)

install(TARGETS ${EXECUTABLE_NAME} DESTINATION "${BIN_DIR}/${TOOLS_DIR}")
if(WIN32 AND MSVC)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/\${BUILD_TYPE}/${EXECUTABLE_NAME}.pdb" DESTINATION "${BIN_DIR}/${TOOLS_DIR}" CONFIGURATIONS Debug)
endif()

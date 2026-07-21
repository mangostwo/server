# MaNGOS is a full featured server for World of Warcraft, supporting
# the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
#
# Copyright (C) 2005-2025 MaNGOS <https://www.getmangos.eu>
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

add_subdirectory(Extractor_projects)

# Extractor_projects is an untouched submodule; see cmake/SubmoduleCompat.cmake.
# It needs the old Auth/md5.h, and it uses std::thread without ever declaring the
# dependency -- that linked before only because ACE dragged pthread in.
foreach(_extractor map-extractor vmap-extractor mmap-extractor)
    if(TARGET ${_extractor})
        mangos_submodule_compat(${_extractor}
            "${CMAKE_SOURCE_DIR}/src/shared/Compat/extractors")
        target_link_libraries(${_extractor} PRIVATE Threads::Threads)
    endif()
endforeach()

# Used for install targets
set(TOOLS_DIR "tools")

#install documentation and generation scripts
install(
    FILES
       "${CMAKE_CURRENT_SOURCE_DIR}/Extractor_Binaries/ExtractResources.sh"
       "${CMAKE_CURRENT_SOURCE_DIR}/Extractor_Binaries/make_vmaps.bat"
      # "${CMAKE_CURRENT_SOURCE_DIR}/Extractor_Binaries/mmap_excluded.txt"
      # "${CMAKE_CURRENT_SOURCE_DIR}/Extractor_Binaries/MoveMapGen.sh"
      # "${CMAKE_CURRENT_SOURCE_DIR}/Extractor_Binaries/offmesh.txt"
        "${CMAKE_CURRENT_SOURCE_DIR}/Extractor_Binaries/README.txt"
    DESTINATION
        "${BIN_DIR}/${TOOLS_DIR}"
)


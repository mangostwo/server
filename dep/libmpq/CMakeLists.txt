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

file(GLOB sources_mpq libmpq/*.c libmpq/*.h)

set(mpq_STAT_SRCS
  ${sources_mpq}
)

if( UNIX )
  include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/dep/zlib
    ${CMAKE_SOURCE_DIR}/dep/bzip2
  )
elseif( WIN32 )
  include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/win
    ${CMAKE_SOURCE_DIR}/dep/zlib
    ${CMAKE_SOURCE_DIR}/dep/bzip2
  )
endif()

#Configure libmpq, without using it's ./configure etc.
include(CheckIncludeFiles)
include(CheckFunctionExists)

check_include_files(dlfcn.h HAVE_DLFCN_H)
check_function_exists(fseeko HAVE_FSEEKO)
check_include_files(inttypes.h HAVE_INTTYPES_H)
set(HAVE_LIBBZ2 1)
set(HAVE_LIBZ 1)
check_include_files(memory.h HAVE_MEMORY_H)
check_include_files(stdint.h HAVE_STDINT_H)
check_include_files(stdlib.h HAVE_STDLIB_H)
check_include_files(strings.h HAVE_STRINGS_H)
check_include_files(string.h HAVE_STRING_H)
check_include_files(sys/stat.h HAVE_SYS_STAT_H)
check_include_files(sys/types.h HAVE_SYS_TYPES_H)
check_include_files(unistd.h HAVE_UNISTD_H)

#Taken from https://github.com/jonathanslenders/fluidsynth/blob/master/cmake_admin/CheckSTDC.cmake
check_include_files("dlfcn.h;stdint.h;stddef.h;inttypes.h;stdlib.h;strings.h;string.h;float.h" StandardHeadersExist)
if (StandardHeadersExist)
  set(STDC_HEADERS 1)
else()
  set(STDC_HEADERS 0)
endif()

set(LIBMPQ_PACKAGE_VERSION "0.4.2")
set(LIBMPQ_PACKAGE_NAME "libmpq")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_SOURCE_DIR}/config.h)

#Add the library
add_library(libmpq STATIC ${mpq_STAT_SRCS})

include_directories( ${ZLIB_INCLUDE_DIRS} )
target_link_libraries(libmpq ${ZLIB_LIBRARIES} bzip2)

set_target_properties(libmpq PROPERTIES LINKER_LANGUAGE CXX)

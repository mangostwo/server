# This code is part of MaNGOS. Contributor & Copyright details are in AUTHORS/THANKS.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

cmake_minimum_required (VERSION 2.8)

project (MANGOS_IOMAP_EXTRACTOR)

LINK_DIRECTORIES( ${LINK_DIRECTORIES} ${CMAKE_SOURCE_DIR}/../../dep/libmpq/libmpq/.libs/ )
add_executable(vmapextractor adtfile.cpp  dbcfile.cpp gameobject_extract.cpp model.cpp  mpq_libmpq.cpp  vmapexport.cpp  wdtfile.cpp  wmo.cpp)
target_link_libraries(vmapextractor mpq bz2 z)

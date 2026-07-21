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

# =============================================================================
# Unit tests. Built only with -DWITH_TESTS=1.
#
# Scope is deliberately narrow: the pieces that are pure functions over bytes and
# whose failure modes are invisible in ordinary use -- packet reassembly under
# fragmentation, and a zero-padding bug that shows about one login in 256.
# Nothing here needs a database, a socket or a world.
#
# No test framework is vendored; see TestHarness.h.
# =============================================================================

set(SRC_GRP_TESTS
    main.cpp
    TestHarness.h
    BigNumberTest.cpp
    ByteBufferTest.cpp
    ByteBufferStressTest.cpp
    CodecStressTest.cpp
    CryptoStressTest.cpp
    NetworkStressTest.cpp
    PacketCodecTest.cpp
)
source_group("tests" FILES ${SRC_GRP_TESTS})

add_executable(mangos_tests ${SRC_GRP_TESTS})

target_link_libraries(mangos_tests
    PRIVATE
        proto
        shared
        Threads::Threads
        mangos_openssl_strict
)

add_test(NAME mangos_tests COMMAND mangos_tests)

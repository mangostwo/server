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
    SessionMailboxTest.cpp
    SessionProtocolPolicyTest.cpp
    DatabaseConcurrencyTest.cpp
    OpenSSLProviderTest.cpp
    ClientParserTest.cpp
    TerrainModelTest.cpp
    TileSerializerTest.cpp
    NavBinningTest.cpp
    DynamicCollisionTest.cpp
    # Compiled in, not linked from `game`: game.lib pulls the whole server, down to the
    # database globals that only mangosd defines. These two know nothing of it.
    ${CMAKE_SOURCE_DIR}/src/game/WorldHandlers/DynamicCollision.cpp
    ${CMAKE_SOURCE_DIR}/src/game/WorldHandlers/GameObjectModel.cpp
    ${CMAKE_SOURCE_DIR}/src/game/Server/SessionMailbox.cpp
    ByteBufferStressTest.cpp
    CodecStressTest.cpp
    CryptoStressTest.cpp
    NetworkStressTest.cpp
    PacketCodecTest.cpp
)
source_group("tests" FILES ${SRC_GRP_TESTS})

add_executable(mangos_tests ${SRC_GRP_TESTS})

target_include_directories(mangos_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/game/WorldHandlers
        ${CMAKE_SOURCE_DIR}/src/game/Server)

target_link_libraries(mangos_tests
    PRIVATE
        proto
        shared
        extractor_client
        extractor_data
        Threads::Threads
        mangos_openssl_strict
)

if(WIN32)
    # Co-locate the OpenSSL crypto runtime next to the test executable.
    #
    # OPENSSL_ROOT_DIR alone is not a usable anchor: FindOpenSSL routinely resolves via
    # OPENSSL_INCLUDE_DIR / the library paths without it ever being set, in which case
    # "${OPENSSL_ROOT_DIR}/bin" degrades to "/bin" and, with NO_DEFAULT_PATH, the search cannot
    # succeed -- turning a missing test-only DLL into a FATAL_ERROR that kills the whole configure.
    # Installs also disagree on whether the DLLs live in <prefix>/bin or directly in <prefix>, and
    # the runtime name carries the major version (libcrypto-3-x64.dll vs libcrypto-4-x64.dll).
    #
    # Same defect and same fix as mangos/realmd PR #32.
    set(_mangos_openssl_hints)
    if(OPENSSL_ROOT_DIR)
        list(APPEND _mangos_openssl_hints "${OPENSSL_ROOT_DIR}" "${OPENSSL_ROOT_DIR}/bin")
    endif()
    if(OPENSSL_INCLUDE_DIR)
        get_filename_component(_mangos_openssl_prefix "${OPENSSL_INCLUDE_DIR}" DIRECTORY)
        list(APPEND _mangos_openssl_hints "${_mangos_openssl_prefix}" "${_mangos_openssl_prefix}/bin")
    endif()
    foreach(_mangos_openssl_lib IN ITEMS ${OPENSSL_CRYPTO_LIBRARY} ${LIB_EAY_RELEASE} ${LIB_EAY})
        if(_mangos_openssl_lib)
            get_filename_component(_mangos_openssl_libdir "${_mangos_openssl_lib}" DIRECTORY)
            list(APPEND _mangos_openssl_hints
                "${_mangos_openssl_libdir}" "${_mangos_openssl_libdir}/../bin")
        endif()
    endforeach()

    # Prefer the runtime whose major version matches the OpenSSL we actually linked against, rather
    # than a fixed preference order -- otherwise a stale libcrypto from a previous install lying
    # around in a searched directory can win over the correct one.
    string(REGEX MATCH "^[0-9]+" _mangos_openssl_major "${OPENSSL_VERSION}")
    set(_mangos_crypto_names)
    if(_mangos_openssl_major)
        list(APPEND _mangos_crypto_names
            "libcrypto-${_mangos_openssl_major}-x64.dll" "libcrypto-${_mangos_openssl_major}.dll")
    endif()
    list(APPEND _mangos_crypto_names libcrypto-x64.dll libcrypto.dll)

    find_file(MANGOS_TEST_OPENSSL_CRYPTO_DLL
        NAMES ${_mangos_crypto_names}
        HINTS ${_mangos_openssl_hints})

    # A missing test-only runtime must not break configuring the server.
    if(MANGOS_TEST_OPENSSL_CRYPTO_DLL)
        add_custom_command(TARGET mangos_tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${MANGOS_TEST_OPENSSL_CRYPTO_DLL}"
                "$<TARGET_FILE_DIR:mangos_tests>")
    else()
        message(WARNING
            "OpenSSL crypto runtime DLL not found; mangos_tests will rely on it being on PATH. "
            "Set OPENSSL_ROOT_DIR to have it copied next to the test executable.")
    endif()

    get_filename_component(
        MANGOS_TEST_MYSQL_DLL_DIR "${MySQL_LIBRARY}" DIRECTORY)
    get_filename_component(
        MANGOS_TEST_MYSQL_DLL_NAME "${MySQL_LIBRARY}" NAME)
    string(REPLACE ".lib" ".dll" MANGOS_TEST_MYSQL_DLL_NAME
        "${MANGOS_TEST_MYSQL_DLL_NAME}")
    add_custom_command(TARGET mangos_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MANGOS_TEST_MYSQL_DLL_DIR}/${MANGOS_TEST_MYSQL_DLL_NAME}"
            "$<TARGET_FILE_DIR:mangos_tests>")

endif()

add_test(NAME mangos_tests COMMAND mangos_tests)

if(WIN32)
    set_tests_properties(mangos_tests PROPERTIES
        ENVIRONMENT "OPENSSL_MODULES=${OPENSSL_ROOT_DIR}/bin")
endif()

add_test(NAME world_network_boundary
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_ROOT=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckWorldNetworkBoundary.cmake)

add_test(NAME build_policy
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_ROOT=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckBuildPolicy.cmake)

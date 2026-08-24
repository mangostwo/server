# SPDX-License-Identifier: GPL-3.0-or-later
#
# MaNGOS is a full featured server for World of Warcraft, supporting
# the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
#
# Copyright (C) 2005-2026 MaNGOS <https://www.getmangos.eu>
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
# along with this program. If not, see <https://www.gnu.org/licenses/>.

function(read_required RELATIVE_PATH OUTPUT)
    set(ABSOLUTE_PATH "${SOURCE_ROOT}/${RELATIVE_PATH}")
    if(NOT EXISTS "${ABSOLUTE_PATH}")
        message(FATAL_ERROR "Missing Warden boundary source: ${RELATIVE_PATH}")
    endif()
    file(READ "${ABSOLUTE_PATH}" CONTENT)
    set(${OUTPUT} "${CONTENT}" PARENT_SCOPE)
endfunction()

function(require_ordered CODE LABEL)
    set(REMAINING "${CODE}")
    foreach(TOKEN IN LISTS ARGN)
        string(FIND "${REMAINING}" "${TOKEN}" OFFSET)
        if(OFFSET EQUAL -1)
            message(FATAL_ERROR "${LABEL}: missing '${TOKEN}'")
        endif()
        string(LENGTH "${TOKEN}" TOKEN_LENGTH)
        math(EXPR CONSUMED "${OFFSET} + ${TOKEN_LENGTH}")
        string(SUBSTRING "${REMAINING}" ${CONSUMED} -1 REMAINING)
    endforeach()
endfunction()

function(slice_between CODE START_TOKEN END_TOKEN OUTPUT)
    string(FIND "${CODE}" "${START_TOKEN}" START_AT)
    string(FIND "${CODE}" "${END_TOKEN}" END_AT)
    if(START_AT EQUAL -1 OR END_AT EQUAL -1 OR END_AT LESS_EQUAL START_AT)
        message(FATAL_ERROR
            "Unable to isolate '${START_TOKEN}' before '${END_TOKEN}'")
    endif()
    math(EXPR LENGTH "${END_AT} - ${START_AT}")
    string(SUBSTRING "${CODE}" ${START_AT} ${LENGTH} SLICE)
    set(${OUTPUT} "${SLICE}" PARENT_SCOPE)
endfunction()

function(read_code RELATIVE_PATH OUTPUT)
    set(ABSOLUTE_PATH "${SOURCE_ROOT}/${RELATIVE_PATH}")
    if(NOT EXISTS "${ABSOLUTE_PATH}")
        message(FATAL_ERROR "Missing Warden boundary source: ${RELATIVE_PATH}")
    endif()

    # Assertions operate on code, not explanatory wording in comments.
    file(STRINGS "${ABSOLUTE_PATH}" RAW_LINES)
    set(IN_BLOCK OFF)
    set(CODE_ONLY "")
    foreach(LINE IN LISTS RAW_LINES)
        if(IN_BLOCK)
            string(FIND "${LINE}" "*/" CLOSE_AT)
            if(CLOSE_AT EQUAL -1)
                continue()
            endif()
            math(EXPR CLOSE_AT "${CLOSE_AT} + 2")
            string(SUBSTRING "${LINE}" ${CLOSE_AT} -1 LINE)
            set(IN_BLOCK OFF)
        endif()

        string(REGEX REPLACE "/\\*[^*]*\\*+([^/*][^*]*\\*+)*/" " " LINE
            "${LINE}")
        string(FIND "${LINE}" "/*" OPEN_AT)
        if(NOT OPEN_AT EQUAL -1)
            string(SUBSTRING "${LINE}" 0 ${OPEN_AT} LINE)
            set(IN_BLOCK ON)
        endif()
        string(REGEX REPLACE "//.*$" "" LINE "${LINE}")
        string(APPEND CODE_ONLY "${LINE}\n")
    endforeach()
    set(${OUTPUT} "${CODE_ONLY}" PARENT_SCOPE)
endfunction()

function(require_count TEXT PATTERN EXPECTED DESCRIPTION)
    string(REGEX MATCHALL "${PATTERN}" MATCHES "${TEXT}")
    list(LENGTH MATCHES COUNT)
    if(NOT COUNT EQUAL EXPECTED)
        message(FATAL_ERROR
            "Warden boundary: ${DESCRIPTION}; expected ${EXPECTED}, found ${COUNT}")
    endif()
endfunction()

function(require_full_header PATH)
    file(READ "${PATH}" CONTENT)
    foreach(MARKER IN ITEMS
        "SPDX-License-Identifier: GPL-3.0-or-later"
        "MaNGOS is a full featured server for World of Warcraft"
        "Copyright (C) 2005-"
        "MaNGOS <https://www.getmangos.eu>"
        "GNU General Public License"
        "You should have received a copy")
        string(FIND "${CONTENT}" "${MARKER}" MARKER_AT)
        if(MARKER_AT EQUAL -1)
            message(FATAL_ERROR
                "Warden boundary: incomplete source header in ${PATH}; missing ${MARKER}")
        endif()
    endforeach()
endfunction()

read_required("src/game/WorldHandlers/World.cpp" WORLD_CPP)
read_required("src/game/WorldHandlers/WorldSessionMgr.cpp" SESSION_MGR_CPP)
read_required("src/game/Server/WorldSession.cpp" SESSION_CPP)
read_required("src/game/Server/WorldGateway.cpp" GATEWAY_CPP)
read_required("src/game/WorldHandlers/CharacterHandler.cpp" CHARACTER_CPP)
read_required("src/game/WorldHandlers/WardenHandler.cpp" WARDEN_HANDLER_CPP)

read_code("src/game/Server/WorldGateway.cpp" GATEWAY_CODE)
read_code("src/game/Server/WorldSession.cpp" SESSION_CODE)
read_code("src/game/Server/OpcodeTable.cpp" OPCODE_CODE)
read_code("src/game/WorldHandlers/World.cpp" WORLD_CODE)
read_code("src/game/WorldHandlers/WorldSessionMgr.cpp" SESSION_MGR_CODE)
read_code("src/game/WorldHandlers/CharacterHandler.cpp" CHARACTER_CODE)
read_code("src/game/WorldHandlers/WardenHandler.cpp" WARDEN_HANDLER_CODE)
read_code("src/game/Server/WardenCheckCatalogLoader.cpp" CATALOG_LOADER_CODE)
read_code("src/game/Warden/WardenPacketCodec.cpp" PACKET_CODEC_CODE)
read_code("src/tests/WardenPacketCodecTest.cpp" PACKET_CODEC_TEST_CODE)
read_code("src/proto/PacketCodec.h" PROTO_PACKET_CODE)
read_code("src/mangosd/Master.cpp" MASTER_CODE)

slice_between("${WORLD_CPP}" "World::AddSession_(WorldSession* s)"
    "void World::VerifyDataIntegrity()" ADD_SESSION_CODE)
slice_between("${WORLD_CPP}" "void World::UpdateSessions(uint32 diff)"
    "// This handles the issued and queued CLI/RA commands" UPDATE_SESSIONS_CODE)
slice_between("${SESSION_CPP}"
    "void WorldSession::OnAuthenticatedAdmission()"
    "void WorldSession::SizeError" ADMISSION_CODE)
slice_between("${CHARACTER_CPP}" "void WorldSession::HandleCharEnum("
    "void WorldSession::HandleCharEnumOpcode" CHAR_ENUM_CODE)
slice_between("${CHARACTER_CPP}"
    "void WorldSession::HandlePlayerLoginOpcode"
    "void WorldSession::HandlePlayerLogin(" PLAYER_LOGIN_CODE)

require_ordered("${ADD_SESSION_CODE}" "immediate authenticated admission"
    "packet << uint8(AUTH_OK)"
    "s->SendPacket(&packet)"
    "s->SendAddonsInfo()"
    "SMSG_CLIENTCACHE_VERSION"
    "s->SendTutorialsData()"
    "s->OnAuthenticatedAdmission()"
    "UpdateMaxSessionCounters()")

require_ordered("${SESSION_MGR_CPP}" "queued authenticated admission"
    "pop_sess->SendAuthWaitQue(0)"
    "pop_sess->SendAddonsInfo()"
    "SMSG_CLIENTCACHE_VERSION"
    "pop_sess->SendAccountDataTimes(GLOBAL_CACHE_MASK)"
    "pop_sess->SendTutorialsData()"
    "pop_sess->OnAuthenticatedAdmission()"
    "m_QueuedSessions.pop_front()")

require_ordered("${UPDATE_SESSIONS_CODE}" "queue release during session reap"
    "if (!pSession->Update(updater))"
    "RemoveQueuedSession(pSession)"
    "m_sessions.erase(itr)")

require_ordered("${CHAR_ENUM_CODE}" "character-list bootstrap"
    "SendPacket(&data)"
    "StartWardenBootstrap()")
require_ordered("${PLAYER_LOGIN_CODE}" "player-login bootstrap safety net"
    "if (PlayerLoading() || GetPlayer() != NULL)"
    "return"
    "StartWardenBootstrap()"
    "m_playerLoading = true")
require_ordered("${UPDATE_SESSIONS_CODE}" "Warden deadline ownership"
    "pSession->UpdateWarden(diff)"
    "pSession->Update(updater)")

string(REGEX MATCHALL "HandleEncrypted\\(" HANDLES "${WARDEN_HANDLER_CPP}")
list(LENGTH HANDLES HANDLE_COUNT)
string(REGEX MATCHALL "rfinish\\(" FINISHES "${WARDEN_HANDLER_CPP}")
list(LENGTH FINISHES FINISH_COUNT)
if(NOT HANDLE_COUNT EQUAL 1 OR NOT FINISH_COUNT EQUAL 1)
    message(FATAL_ERROR
        "Grouped Warden handler must forward and finish exactly one body")
endif()
if(WARDEN_HANDLER_CPP MATCHES "switch[ \t\r\n]*\\(")
    message(FATAL_ERROR "Grouped Warden handler may not decode inner commands")
endif()
require_ordered("${WARDEN_HANDLER_CPP}" "grouped Warden ingress"
    "m_warden->HandleEncrypted(body)"
    "recvData.rfinish()"
    "FinalizeWardenDisengagement()")

if(ADMISSION_CODE MATCHES "StartWardenBootstrap|m_warden->Start")
    message(FATAL_ERROR "Authenticated admission must create Warden inertly")
endif()

if(ADMISSION_CODE MATCHES "m_sessions|AddSession_|RemoveSession")
    message(FATAL_ERROR "WorldSession admission hook may not mutate m_sessions")
endif()

string(REGEX MATCHALL "WardenIncidentStore::Instance\\(\\)\\.Load\\(" LOADS
    "${GATEWAY_CPP}")
list(LENGTH LOADS LOAD_COUNT)
if(NOT LOAD_COUNT EQUAL 1)
    message(FATAL_ERROR
        "WorldGateway::Attach must contain exactly one Warden history load")
endif()
if(SESSION_CPP MATCHES
    "WardenIncidentStore::Instance\\(\\)\\.Load\\(")
    message(FATAL_ERROR "WorldSession may not query Warden incident history")
endif()
if(WORLD_CPP MATCHES "WardenIncidentStore::Instance\\(\\)\\.Load\\(" OR
    SESSION_MGR_CPP MATCHES
        "WardenIncidentStore::Instance\\(\\)\\.Load\\(")
    message(FATAL_ERROR
        "World update files may not query Warden incident history")
endif()

read_code("src/game/Server/WardenIncidentStore.cpp" INCIDENT_STORE_CODE)
slice_between("${INCIDENT_STORE_CODE}"
    "WardenIncidentWriteResult WardenIncidentStore::Record("
    "}\n}" INCIDENT_RECORD_CODE)
if(INCIDENT_RECORD_CODE MATCHES
        "(^|[^A-Za-z0-9_])Load[ \\t\\r\\n]*\\(" OR
    INCIDENT_RECORD_CODE MATCHES "\\.PQuery[ \\t\\r\\n]*\\(")
    message(FATAL_ERROR
        "Warden incident commit may not synchronously reload history on the world thread")
endif()
require_ordered("${INCIDENT_RECORD_CODE}" "same-second Warden ban promotion"
    "INSERT INTO `account_banned`"
    "ON DUPLICATE KEY UPDATE"
    "`unbandate` = VALUES(`unbandate`)"
    "`active` = VALUES(`active`)")

read_code("src/game/Warden/WardenManager.cpp" WARDEN_MANAGER_CODE)
require_ordered("${WARDEN_MANAGER_CODE}" "Warden crypto initialization guard"
    "WardenCryptoContext crypto"
    "if (!crypto.Initialize(sessionKey))"
    "return nullptr"
    "WardenCheckProfile const* selected")

file(GLOB PURE_WARDEN_SOURCES
    "${SOURCE_ROOT}/src/game/Warden/*.h"
    "${SOURCE_ROOT}/src/game/Warden/*.cpp")

# The opcode table has one grouped Warden ingress. Inner command identifiers
# are data owned by WardenServer, never additional world-opcode handlers.
require_count("${OPCODE_CODE}" "CMSG_WARDEN_DATA" 1
    "opcode table must register CMSG_WARDEN_DATA exactly once")
require_count("${OPCODE_CODE}"
    "CMSG_WARDEN_DATA[^;]*HandleWardenDataOpcode" 1
    "CMSG_WARDEN_DATA must use the grouped handler")
require_count("${WARDEN_HANDLER_CODE}"
    "void WorldSession::HandleWardenDataOpcode[ \\t]*\\(" 1
    "grouped Warden handler must have one definition")
require_count("${WARDEN_HANDLER_CODE}"
    "#[ \\t]*include[ \\t]*[\"<]WardenServer\\.h[\">]" 1
    "grouped handler must include its state-machine dependency directly")
read_code("src/game/Server/WorldSession.h" SESSION_HEADER_CODE)
if(SESSION_HEADER_CODE MATCHES
    "HandleWarden(Module|Hash|Check|Mem|Lua|Mpq|Proc|Page|Driver)[A-Za-z0-9_]*Opcode")
    message(FATAL_ERROR
        "Warden boundary: inner Warden commands may not become opcode handlers")
endif()
require_count("${SESSION_CODE}" "SMSG_WARDEN_DATA" 1
    "session send adapter must create exactly one outer Warden wrapper")

foreach(LEGACY_FILE IN ITEMS
    "src/game/Warden/Warden.h"
    "src/game/Warden/Warden.cpp"
    "src/game/Warden/WardenCheckMgr.h"
    "src/game/Warden/WardenCheckMgr.cpp"
    "src/game/Warden/WardenMac.h"
    "src/game/Warden/WardenMac.cpp"
    "src/game/Warden/WardenWin.h"
    "src/game/Warden/WardenWin.cpp"
    "src/game/Warden/Modules/WardenModuleMac.h"
    "src/game/Warden/Modules/WardenModuleWin.h"
    "src/shared/Auth/WardenKeyGeneration.h")
    if(EXISTS "${SOURCE_ROOT}/${LEGACY_FILE}")
        message(FATAL_ERROR
            "Warden boundary: legacy implementation file returned: ${LEGACY_FILE}")
    endif()
endforeach()

file(GLOB WARDEN_SERVER_SOURCES
    "${SOURCE_ROOT}/src/game/Server/Warden*.h"
    "${SOURCE_ROOT}/src/game/Server/Warden*.cpp")
set(WARDEN_PRODUCT_SOURCES ${PURE_WARDEN_SOURCES} ${WARDEN_SERVER_SOURCES})
list(APPEND WARDEN_PRODUCT_SOURCES
    "${SOURCE_ROOT}/src/game/Server/WorldSession.cpp"
    "${SOURCE_ROOT}/src/game/Server/WorldSession.h"
    "${SOURCE_ROOT}/src/game/WorldHandlers/WardenHandler.cpp"
    "${SOURCE_ROOT}/src/game/WorldHandlers/WorldConfig.cpp")

foreach(PRODUCT_SOURCE IN LISTS WARDEN_PRODUCT_SOURCES)
    file(RELATIVE_PATH PRODUCT_RELATIVE "${SOURCE_ROOT}" "${PRODUCT_SOURCE}")
    read_code("${PRODUCT_RELATIVE}" PRODUCT_CODE)
    string(TOUPPER "${PRODUCT_CODE}" PRODUCT_CODE_UPPER)

    foreach(LEGACY_PATTERN IN ITEMS
        "WARDEN_CMSG_" "WARDEN_SMSG_" "WARDEN_ACTION_"
        "CONFIG_BOOL_WARDEN_WIN_ENABLED"
        "CONFIG_BOOL_WARDEN_OSX_ENABLED"
        "CONFIG_UINT32_WARDEN_CLIENT_FAIL_ACTION"
        "WARDEN_ACTION" "WARDEN_LOG")
        if(PRODUCT_CODE_UPPER MATCHES "${LEGACY_PATTERN}")
            message(FATAL_ERROR
            "Warden boundary: legacy token ${LEGACY_PATTERN} in ${PRODUCT_RELATIVE}")
        endif()
    endforeach()
    if(PRODUCT_CODE MATCHES
        "(class|struct|enum)[ \\t]+(Warden|WardenCheckMgr|WardenMac|WardenWin|WardenOpcodes)([^A-Za-z0-9_]|$)" OR
        PRODUCT_CODE_UPPER MATCHES
        "(^|[^A-Z0-9_])(MEM_CHECK|PROC_CHECK|PAGE_CHECK_A|PAGE_CHECK_B|DRIVER_CHECK|LUA_STR_CHECK|MPQ_CHECK|MODULE_CHECK|STATE_INITIAL|STATE_RESTING|STATE_REQUESTED_[A-Z_]+|STATE_SENT_MODULE)([^A-Z0-9_]|$)")
        message(FATAL_ERROR
            "Warden boundary: legacy class or constant in ${PRODUCT_RELATIVE}")
    endif()
    if(PRODUCT_CODE MATCHES
        "(FROM|INTO|UPDATE|JOIN)[ \\t\\r\\n]+`warden`([^A-Za-z0-9_]|$)")
        message(FATAL_ERROR
            "Warden boundary: legacy World table `warden` in ${PRODUCT_RELATIVE}")
    endif()

    foreach(TBC_PATTERN IN ITEMS
        "(^|[^0-9])8606([^0-9]|$)"
        "0X00257970" "0X00254080" "0X00254E00" "0X00255290"
        "0X00307200" "0X00349850"
        "WOW\\.EXE" "576F572E657865"
        "0X57[ \\t\\r\\n]*,[ \\t\\r\\n]*0X6F[ \\t\\r\\n]*,[ \\t\\r\\n]*0X57[ \\t\\r\\n]*,[ \\t\\r\\n]*0X2E")
        if(PRODUCT_CODE_UPPER MATCHES "${TBC_PATTERN}")
            message(FATAL_ERROR
                "Warden boundary: TBC or named-executable fact ${TBC_PATTERN} in ${PRODUCT_RELATIVE}")
        endif()
    endforeach()

    if(PRODUCT_CODE_UPPER MATCHES
        "AREATABLE|DBFILESCLIENT|0X007DA8C0|[\"']OKAY[\"']")
        message(FATAL_ERROR
            "Warden boundary: production check content must come from warden_checks: ${PRODUCT_RELATIVE}")
    endif()
endforeach()

# Pure protocol/state files cannot reach world/session/link/database ownership.
foreach(PURE_SOURCE IN LISTS PURE_WARDEN_SOURCES)
    file(RELATIVE_PATH PURE_RELATIVE "${SOURCE_ROOT}" "${PURE_SOURCE}")
    read_code("${PURE_RELATIVE}" PURE_LAYER_CODE)
    foreach(FORBIDDEN_LAYER_PATTERN IN ITEMS
        "#[ \\t]*include[ \\t]*[\"<](Database/|World\\.h|WorldSession\\.h|Player\\.h|IClientLink\\.h|SessionMailbox\\.h|WorldGateway\\.h|WardenAuditStore\\.h|WardenIncidentStore\\.h)"
        "(^|[^A-Za-z0-9_])(LoginDatabase|WorldDatabase|CharacterDatabase|WorldSession|IClientLink|KickPlayer|BanAccount)([^A-Za-z0-9_]|$)")
        if(PURE_LAYER_CODE MATCHES "${FORBIDDEN_LAYER_PATTERN}")
            message(FATAL_ERROR
                "Warden boundary: pure layer dependency ${FORBIDDEN_LAYER_PATTERN} in ${PURE_RELATIVE}")
        endif()
    endforeach()
endforeach()

# The 10,229-byte result-body ceiling is derived from the client's 10,240-byte
# packet cap, four-byte opcode, and seven-byte Warden result envelope.
require_count("${PROTO_PACKET_CODE}"
    "MAX_CLIENT_PACKET_SIZE[ \\t]*=[ \\t]*10240" 1
    "client packet cap must remain 10,240 bytes")
require_count("${PACKET_CODEC_CODE}"
    "size_t constexpr ClientOpcodeBytes[ \\t]*=[ \\t]*sizeof\\(uint32\\)" 1
    "Warden codec must account for the client opcode")
require_count("${PACKET_CODEC_CODE}"
    "size_t constexpr CheckResultEnvelopeBytes" 1
    "Warden codec must define the seven-byte result envelope")
require_count("${PACKET_CODEC_CODE}"
    "static_assert[ \\t\\r\\n]*\\([ \\t\\r\\n]*proto::MAX_CLIENT_PACKET_SIZE" 1
    "Warden transport arithmetic must be compile-time guarded")
require_count("${PACKET_CODEC_CODE}"
    "MaxTransportResultBodyBytes[^;]*proto::MAX_CLIENT_PACKET_SIZE[^;]*ClientOpcodeBytes[^;]*CheckResultEnvelopeBytes" 1
    "Warden result ceiling must be derived by exact subtraction")
require_count("${PACKET_CODEC_TEST_CODE}"
    "MaximumResultBodyBytes[ \\t]*=[ \\t]*10229" 1
    "transport regression must pin the exact 10,229-byte body")
require_count("${PACKET_CODEC_TEST_CODE}"
    "static_assert[^;]*proto::MAX_CLIENT_PACKET_SIZE[^;]*sizeof\\(uint32\\)" 1
    "transport regression must prove the exact arithmetic")

# One aliased, count-derived statement owns the binary-safe catalogue snapshot.
require_count("${CATALOG_LOADER_CODE}" "WorldDatabase\\.Query[ \\t]*\\(" 1
    "catalogue loader must execute one snapshot query")
require_count("${CATALOG_LOADER_CODE}"
    "SELECT[ \\t]+COUNT\\(\\*\\)[ \\t]+AS[ \\t]+`snapshot_count`[ \\t]+FROM[ \\t]+`warden_checks`" 1
    "catalogue query must derive its source count")
require_count("${CATALOG_LOADER_CODE}" "LEFT[ \\t]+JOIN" 1
    "catalogue query must contain one LEFT JOIN")
require_count("${CATALOG_LOADER_CODE}"
    "LEFT[ \\t]+JOIN[ \\t]+`warden_checks`[ \\t]+AS[ \\t]+`checks`[ \\t]+ON[ \\t]+TRUE" 1
    "catalogue rows must be joined through the checks alias")
foreach(BINARY_FIELD IN ITEMS platform locale module request expected)
    require_count("${CATALOG_LOADER_CODE}"
        "HEX\\([ \\t]*`checks`\\.`${BINARY_FIELD}`[ \\t]*\\)" 1
        "catalogue query must project checks.${BINARY_FIELD} through HEX once")
endforeach()
if(CATALOG_LOADER_CODE MATCHES "GetCppString[ \\t]*\\(")
    message(FATAL_ERROR
        "Warden boundary: binary catalogue fields may not use GetCppString")
endif()

require_count("${MASTER_CODE}"
    "WardenCheckCatalogLoader[ \\t]*\\([ \\t]*\\)[ \\t]*\\.LoadAndPublish[ \\t]*\\(" 1
    "mangosd must publish the catalogue exactly once")
require_ordered("${MASTER_CODE}" "mandatory Warden startup order"
    "WardenCheckCatalogLoader().LoadAndPublish()"
    "sWorld.SetInitialWorldSettings()"
    "sWorldNetwork.Start")

# Preserve the legacy account projection indices and append exact locale at 9.
require_ordered("${GATEWAY_CODE}" "WorldGateway account projection"
    "`id`, "
    "`gmlevel`, "
    "`sessionkey`, "
    "`last_ip`, "
    "`locked`, "
    "`expansion`, "
    "`mutetime`, "
    "`locale`, "
    "`os`, "
    "`client_locale` "
    "FROM `account`")
require_count("${GATEWAY_CODE}" "`client_locale`" 1
    "gateway account query must append exact locale once")

# Pin the two intentionally asymmetric authenticated admission sequences.
require_ordered("${WORLD_CODE}" "immediate authenticated admission sends"
    "packet << uint8(AUTH_OK)"
    "s->SendPacket(&packet)"
    "s->SendAddonsInfo()"
    "SMSG_CLIENTCACHE_VERSION"
    "s->SendTutorialsData()"
    "s->OnAuthenticatedAdmission()"
    "UpdateMaxSessionCounters()")
require_ordered("${SESSION_MGR_CODE}" "queued authenticated admission sends"
    "pop_sess->SendAuthWaitQue(0)"
    "pop_sess->SendAddonsInfo()"
    "SMSG_CLIENTCACHE_VERSION"
    "pop_sess->SendAccountDataTimes(GLOBAL_CACHE_MASK)"
    "pop_sess->SendTutorialsData()"
    "pop_sess->OnAuthenticatedAdmission()"
    "m_QueuedSessions.pop_front()")
require_ordered("${WORLD_CODE}" "Warden update before packet dispatch"
    "pSession->UpdateWarden(diff)"
    "pSession->Update(updater)")

slice_between("${SESSION_CODE}" "WorldSession::~WorldSession()"
    "void WorldSession::OnAuthenticatedAdmission()" DESTRUCTOR_CODE)
string(FIND "${DESTRUCTOR_CODE}" "{" DESTRUCTOR_BODY_AT)
if(DESTRUCTOR_BODY_AT EQUAL -1)
    message(FATAL_ERROR "Warden boundary: destructor body is missing")
endif()
math(EXPR DESTRUCTOR_BODY_AT "${DESTRUCTOR_BODY_AT} + 1")
string(SUBSTRING "${DESTRUCTOR_CODE}" ${DESTRUCTOR_BODY_AT} -1
    DESTRUCTOR_BODY)
string(STRIP "${DESTRUCTOR_BODY}" DESTRUCTOR_BODY)
string(FIND "${DESTRUCTOR_BODY}" "DrainWardenPendingConfirmations()"
    DESTRUCTOR_FIRST_AT)
if(NOT DESTRUCTOR_FIRST_AT EQUAL 0)
    message(FATAL_ERROR
        "Warden boundary: destructor must drain confirmations as its first statement")
endif()
require_ordered("${DESTRUCTOR_CODE}" "Warden destructor custody"
    "DrainWardenPendingConfirmations()"
    "m_mailbox->Close()")
require_count("${DESTRUCTOR_CODE}"
    "DrainWardenPendingConfirmations[ \\t]*\\(" 1
    "destructor must drain pending confirmations exactly once")

slice_between("${SESSION_CODE}"
    "void WorldSession::HandleWardenLifecycle("
    "void WorldSession::HandleWardenEvidenceBatch(" LIFECYCLE_CODE)
require_ordered("${LIFECYCLE_CODE}" "closed-link send failure disposition"
    "event.failure == warden::WardenFailure::SendFailure"
    "!m_link || m_link->IsClosed()"
    "RequestWardenDisengagement()"
    "return"
    "PersistWardenOperationalAudit(event.failure)")
require_ordered("${LIFECYCLE_CODE}" "operational failure disposition"
    "PersistWardenOperationalAudit(event.failure)"
    "DrainWardenPendingConfirmations()"
    "RequestWardenDisengagement()")
if(LIFECYCLE_CODE MATCHES "WardenIncidentStore|BanAccount")
    message(FATAL_ERROR
        "Warden boundary: operational failures may not create incidents or bans")
endif()

slice_between("${SESSION_CODE}"
    "void WorldSession::FinalizeWardenDisengagement("
    "void WorldSession::PersistWardenAudit(" FINALIZE_CODE)
require_ordered("${FINALIZE_CODE}" "deferred Warden teardown"
    "DrainWardenPendingConfirmations()"
    "m_warden.reset()"
    "m_wardenPolicy.reset()")

slice_between("${SESSION_CODE}"
    "void WorldSession::PersistWardenAudit("
    "void WorldSession::PersistWardenOperationalAudit(" AUDIT_CODE)
if(AUDIT_CODE MATCHES
    "KickPlayer|WardenIncidentStore|account_banned|BanAccount")
    message(FATAL_ERROR
        "Warden boundary: non-actionable audits may not enforce")
endif()

slice_between("${SESSION_CODE}"
    "void WorldSession::PersistWardenOperationalAudit("
    "void WorldSession::PersistWardenIncidentAndKick(" OPERATIONAL_AUDIT_CODE)
if(OPERATIONAL_AUDIT_CODE MATCHES
    "KickPlayer|WardenIncidentStore|account_banned|BanAccount")
    message(FATAL_ERROR
        "Warden boundary: operational audits may not enforce")
endif()

slice_between("${SESSION_CODE}"
    "void WorldSession::PersistWardenIncidentAndKick("
    "void WorldSession::StartWardenBootstrap(" INCIDENT_CODE)
require_ordered("${INCIDENT_CODE}" "confirmed incident persistence"
    "DrainWardenPendingConfirmations()"
    "WardenIncidentStore::Instance().Record"
    "RequestWardenDisengagement()"
    "KickPlayer()")
require_count("${INCIDENT_CODE}"
    "WardenIncidentStore::Instance[ \\t]*\\([ \\t]*\\)[ \\t]*\\.[ \\t]*Record" 1
    "confirmed incident must attempt one durable write")

slice_between("${SESSION_CODE}"
    "void WorldSession::StartWardenBootstrap("
    "void WorldSession::UpdateWarden(" START_CODE)
require_ordered("${START_CODE}" "bootstrap wrapper"
    "!m_link || m_link->IsClosed()"
    "m_warden->Start()"
    "FinalizeWardenDisengagement()")

slice_between("${SESSION_CODE}"
    "void WorldSession::UpdateWarden("
    "void WorldSession::QueuePacket(" UPDATE_WARDEN_CODE)
require_ordered("${UPDATE_WARDEN_CODE}" "Warden update wrapper"
    "!m_link || m_link->IsClosed()"
    "m_warden->Update(eligible, diffMs)"
    "FinalizeWardenDisengagement()")
require_count("${UPDATE_WARDEN_CODE}"
    "player[ \\t]*&&[ \\t]*player->IsInWorld[ \\t]*\\([ \\t]*\\)[ \\t]*&&[ \\t]*!m_playerLoading" 1
    "new plans must require an in-world, non-loading player")

read_code("src/game/Warden/WardenServer.cpp" WARDEN_SERVER_CODE)
read_code("src/game/Warden/WardenServer.h" WARDEN_SERVER_HEADER_CODE)
slice_between("${WARDEN_SERVER_CODE}"
    "void WardenServer::HandleCheckResult("
    "if (m_evidenceObserver)" HANDLE_CHECK_RESULT_CODE)
string(FIND "${HANDLE_CHECK_RESULT_CODE}" "batch.evidence.push_back(evidence)"
    LAST_EVIDENCE_AT REVERSE)
string(FIND "${HANDLE_CHECK_RESULT_CODE}" "m_crypto = std::move(crypto)"
    CRYPTO_COMMIT_AT)
if(LAST_EVIDENCE_AT EQUAL -1 OR CRYPTO_COMMIT_AT EQUAL -1 OR
    CRYPTO_COMMIT_AT LESS LAST_EVIDENCE_AT)
    message(FATAL_ERROR
        "Warden receive crypto may commit only after evidence classification succeeds")
endif()
if(WARDEN_SERVER_CODE MATCHES "m_transitionedSinceUpdate" OR
    WARDEN_SERVER_HEADER_CODE MATCHES "m_transitionedSinceUpdate")
    message(FATAL_ERROR
        "Warden boundary: state transitions may not exempt elapsed time")
endif()
read_code("src/game/WorldHandlers/Map.cpp" MAP_CODE)
if(MAP_CODE MATCHES "UpdateWarden[ \\t]*\\(")
    message(FATAL_ERROR "Warden boundary: Map.cpp may not own Warden updates")
endif()

slice_between("${SESSION_CODE}"
    "void WorldSession::HandleWardenLifecycle("
    "void WorldSession::StartWardenBootstrap(" ENFORCEMENT_CODE)
if(ENFORCEMENT_CODE MATCHES
    "GetSecurity[ \\t]*\\(|SEC_[A-Z_]+|gmlevel|GameMaster")
    message(FATAL_ERROR
        "Warden boundary: enforcement may not exempt privileged accounts")
endif()
require_count("${ENFORCEMENT_CODE}"
    "IsCompleteCleanOperatorBatch[ \\t]*\\(" 1
    "healthy output must use the complete clean-batch predicate")
require_count("${ENFORCEMENT_CODE}" "Warden healthy for player %s" 1
    "operator health summary must have one player-facing path")

# Log calls may receive only fixed typed labels and stable identifiers.
set(WARDEN_SESSION_ADAPTER "${ADMISSION_CODE}\n${ENFORCEMENT_CODE}")
if(WARDEN_SESSION_ADAPTER MATCHES
    "m_client(Platform|Locale)[ \\t]*\\.[ \\t]*c_str[ \\t]*\\(")
    message(FATAL_ERROR
        "Warden boundary: raw authenticated identity token reaches a log")
endif()
string(TOUPPER "${WARDEN_SESSION_ADAPTER}" WARDEN_SESSION_ADAPTER_UPPER)
foreach(SECRET_LOG_TOKEN IN ITEMS
    "SESSIONKEY" "CLIENTTICK" "EXPECTEDBYTES" "RETURNEDBYTES"
    "DECRYPTED" "ENCRYPTED" "PAYLOAD" "PACKETBODY" "HASHSEED"
    "REQUESTBYTES")
    if(WARDEN_SESSION_ADAPTER_UPPER MATCHES
        "(SLOG\\.OUT(STRING|ERROR)|DEBUG_LOG)[ \\t]*\\([^;]*${SECRET_LOG_TOKEN}")
        message(FATAL_ERROR
            "Warden boundary: secret or payload token ${SECRET_LOG_TOKEN} reaches logging")
    endif()
endforeach()
require_count("${ADMISSION_CODE}" "SafeWardenLogToken[ \\t]*\\(" 2
    "authenticated platform and locale must each be sanitized for logs")
foreach(DIRECT_HEADER IN ITEMS
    WardenAuditStore WardenConfiguration WardenEnforcementPolicy
    WardenEvidence WardenIncidentStore WardenManager WardenProtocol WardenServer)
    require_count("${SESSION_CODE}"
        "#[ \\t]*include[ \\t]*[\"<]${DIRECT_HEADER}\\.h[\">]" 1
        "WorldSession adapter must directly include ${DIRECT_HEADER}.h")
endforeach()

# Every campaign-created source carries the complete project header. Direct
# include sufficiency is separately proven by the required PCH-off build.
file(GLOB FULL_HEADER_FILES
    "${SOURCE_ROOT}/src/game/Warden/*.h"
    "${SOURCE_ROOT}/src/game/Warden/*.cpp"
    "${SOURCE_ROOT}/src/game/Server/Warden*.h"
    "${SOURCE_ROOT}/src/game/Server/Warden*.cpp"
    "${SOURCE_ROOT}/src/tests/Warden*.h"
    "${SOURCE_ROOT}/src/tests/Warden*.cpp")
list(APPEND FULL_HEADER_FILES
    "${SOURCE_ROOT}/src/game/WorldHandlers/WardenHandler.cpp"
    "${SOURCE_ROOT}/src/tests/CheckWardenBoundary.cmake"
    "${SOURCE_ROOT}/extra/warden/generate_warden_module_resource.py")
foreach(HEADER_FILE IN LISTS FULL_HEADER_FILES)
    require_full_header("${HEADER_FILE}")
endforeach()

message(STATUS "Wrath Warden architecture boundary intact")

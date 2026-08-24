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

read_required("src/game/WorldHandlers/World.cpp" WORLD_CPP)
read_required("src/game/WorldHandlers/WorldSessionMgr.cpp" SESSION_MGR_CPP)
read_required("src/game/Server/WorldSession.cpp" SESSION_CPP)
read_required("src/game/Server/WorldGateway.cpp" GATEWAY_CPP)

slice_between("${WORLD_CPP}" "World::AddSession_(WorldSession* s)"
    "void World::VerifyDataIntegrity()" ADD_SESSION_CODE)
slice_between("${WORLD_CPP}" "void World::UpdateSessions(uint32 diff)"
    "// This handles the issued and queued CLI/RA commands" UPDATE_SESSIONS_CODE)
slice_between("${SESSION_CPP}"
    "void WorldSession::OnAuthenticatedAdmission()"
    "void WorldSession::SizeError" ADMISSION_CODE)

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
if(SESSION_CPP MATCHES "WardenIncidentStore|warden_incident")
    message(FATAL_ERROR "WorldSession may not query Warden incident history")
endif()
if(WORLD_CPP MATCHES "WardenIncidentStore|warden_incident" OR
    SESSION_MGR_CPP MATCHES "WardenIncidentStore|warden_incident")
    message(FATAL_ERROR
        "World update files may not query Warden incident history")
endif()

file(GLOB PURE_WARDEN_SOURCES
    "${SOURCE_ROOT}/src/game/Warden/*.h"
    "${SOURCE_ROOT}/src/game/Warden/*.cpp")
foreach(PURE_SOURCE IN LISTS PURE_WARDEN_SOURCES)
    file(READ "${PURE_SOURCE}" PURE_CODE)
    if(PURE_CODE MATCHES "WardenIncidentStore|warden_incident|LoginDatabase")
        message(FATAL_ERROR
            "Pure Warden source may not query Realm history: ${PURE_SOURCE}")
    endif()
endforeach()

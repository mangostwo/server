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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.

file(READ "${SOURCE_ROOT}/src/game/Server/WorldGateway.h" GATEWAY_HEADER)
file(READ "${SOURCE_ROOT}/src/game/Server/WorldGateway.cpp" GATEWAY_SOURCE)
file(READ "${SOURCE_ROOT}/src/game/Server/WorldSession.h" SESSION_HEADER)
file(READ "${SOURCE_ROOT}/src/game/Server/WorldSession.cpp" SESSION_SOURCE)
file(READ "${SOURCE_ROOT}/src/proto/IWorldGateway.h" PROTO_GATEWAY)
file(READ "${SOURCE_ROOT}/src/proto/ClientConnection.cpp" CLIENT_SOURCE)
file(READ "${SOURCE_ROOT}/src/game/Server/OpcodeTable.cpp" OPCODE_SOURCE)

foreach(REQUIRED_TEXT
    "std::unordered_map<proto::SessionId, std::shared_ptr<SessionMailbox>> m_routes"
    "std::shared_ptr<SessionMailbox> mailbox"
    "WorldSession* published = session.release()"
    "mailbox->Enqueue"
    "mailbox->Close")
  string(FIND "${GATEWAY_HEADER}\n${GATEWAY_SOURCE}"
    "${REQUIRED_TEXT}" POSITION)
  if(POSITION EQUAL -1)
    message(FATAL_ERROR "Missing mailbox route invariant: ${REQUIRED_TEXT}")
  endif()
endforeach()

foreach(FORBIDDEN_TEXT
    "virtual bool OnPing"
    "ClientConnection::HandlePing"
    "m_gateway.OnPing")
  string(FIND "${PROTO_GATEWAY}\n${CLIENT_SOURCE}"
    "${FORBIDDEN_TEXT}" POSITION)
  if(NOT POSITION EQUAL -1)
    message(FATAL_ERROR "Protocol-side game policy remains: ${FORBIDDEN_TEXT}")
  endif()
endforeach()

foreach(REQUIRED_TEXT
    "OPCODE(CMSG_PING,                                      STATUS_AUTHED,   PROCESS_THREADUNSAFE, &WorldSession::HandlePingOpcode)"
    "OPCODE(CMSG_KEEP_ALIVE,                                STATUS_AUTHED,   PROCESS_THREADUNSAFE, &WorldSession::HandleKeepAliveOpcode)"
    "IsAllowedWhileLoginQueued(packet->GetOpcode())"
    "void WorldSession::HandlePingOpcode"
    "void WorldSession::HandleKeepAliveOpcode")
  string(FIND "${OPCODE_SOURCE}\n${SESSION_SOURCE}"
    "${REQUIRED_TEXT}" POSITION)
  if(POSITION EQUAL -1)
    message(FATAL_ERROR "Missing world-thread policy: ${REQUIRED_TEXT}")
  endif()
endforeach()

foreach(FORBIDDEN_TEXT
    "std::unordered_map<proto::SessionId, WorldSession*>"
    "WorldSession* Find("
    "target->QueuePacket"
    "target->KickPlayer")
  string(FIND "${GATEWAY_HEADER}\n${GATEWAY_SOURCE}"
    "${FORBIDDEN_TEXT}" POSITION)
  if(NOT POSITION EQUAL -1)
    message(FATAL_ERROR "Raw session route remains: ${FORBIDDEN_TEXT}")
  endif()
endforeach()

foreach(REQUIRED_TEXT
    "std::shared_ptr<SessionMailbox> m_mailbox"
    "m_mailbox->Close()"
    "m_mailbox->Enqueue"
    "m_mailbox->Next"
    "if (m_link && m_link->IsClosed())"
    "m_link.reset()"
    "return false;                                    // Will remove this session from the world session map")
  string(FIND "${SESSION_HEADER}\n${SESSION_SOURCE}"
    "${REQUIRED_TEXT}" POSITION)
  if(POSITION EQUAL -1)
    message(FATAL_ERROR "Missing session mailbox invariant: ${REQUIRED_TEXT}")
  endif()
endforeach()

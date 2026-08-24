/**
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2026 MaNGOS <https://www.getmangos.eu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef MANGOS_WARDEN_PACKET_CODEC_H
#define MANGOS_WARDEN_PACKET_CODEC_H

#include "WardenCheckPlan.h"
#include "WardenModuleCatalog.h"

#include <cstddef>
#include <variant>
#include <vector>

namespace warden
{
/** Transactional plaintext decode result; failures publish no partial output. */
enum class DecodeStatus : uint8
{
    Ok,
    Empty,
    WrongSize,
    UnsupportedCommand,
    ChecksumMismatch,
    InvalidValue,
    CryptoFailure
};

/** Transactional plaintext encode result. */
enum class EncodeStatus : uint8
{
    Ok,
    InvalidProfile,
    InvalidPlan,
    CryptoFailure
};

/** Worst-case request/result sizes derived without emitting a packet. */
struct WardenCheckPlanBudget
{
    size_t stringCount = 0;
    size_t stringTableBytes = 0;
    size_t requestBodyBytes = 0;
    size_t maximumResultBytes = 0;
};

/** Structural failure returned by the shared plan preflight. */
enum class CheckPlanValidation : uint8
{
    Valid,
    InvalidRequestId,
    Empty,
    InvalidDefinition,
    DuplicateCheckId,
    DuplicateTiming,
    InvalidConfirmation,
    TooManyStrings,
    StringTableTooLarge,
    RequestBodyTooLarge,
    ResultBodyTooLarge,
    TransportResultBodyTooLarge
};

/** Timing response normalized to stability plus the private client tick. */
struct TimingResult
{
    bool stable = false;
    uint32 clientTick = 0;
};

/** Delivered-module status byte for MPQ reads. */
enum class MpqResultStatus : uint8
{
    Success = 0,
    Unavailable = 1
};

/** Private decoded archive digest, cleansed after evidence classification. */
struct MpqResult
{
    MpqResultStatus status = MpqResultStatus::Unavailable;
    Digest20 digest{};
};

/** Delivered-module status byte for Lua lookups. */
enum class LuaResultStatus : uint8
{
    Success = 0,
    Unavailable = 1
};

/** Private decoded Lua result; only classified evidence may leave the server. */
struct LuaResult
{
    LuaResultStatus status = LuaResultStatus::Unavailable;
    std::string text;
};

/** Delivered-module status byte for guarded process-memory reads. */
enum class MemResultStatus : uint8
{
    Success = 0,
    Unavailable = 1
};

/** Private decoded memory result; only classified evidence may leave Warden. */
struct MemResult
{
    MemResultStatus status = MemResultStatus::Unavailable;
    Bytes actualBytes;
};

using CheckResult =
    std::variant<TimingResult, MpqResult, LuaResult, MemResult>;

/** Ordered private results matching one retained CheckPlan. */
struct CheckBatchResult
{
    std::vector<CheckResult> checks;
};

/** Strictly shaped bootstrap command decoded before state validation. */
struct ClientMessage
{
    ClientCommand command = ClientCommand::ModuleMissing;
    // Populated only for the exact 1 + 20-byte HASH_RESULT shape.
    Digest20 hash{};
};

// These functions encode/decode the plaintext inner Warden command. Transport
// encryption and the outer SMSG/CMSG_WARDEN_DATA packet belong to other layers.
/** Encodes the exact module identity/key/size negotiation body. */
Bytes EncodeModuleUse(ModuleProfile const& profile);
/** Encodes one bounded encrypted-module transfer chunk. */
Bytes EncodeModuleCache(ByteView chunk);
/** Encodes the module's exact 16-byte hash challenge. */
Bytes EncodeHashRequest(ModuleProfile const& profile);
// Encodes all three adjacent command-3 records into one body. Output remains
// unchanged if profile validation or folded SHA-1 construction fails.
EncodeStatus EncodeModuleInitialize(ModuleProfile const& profile, Bytes& output);
// Builds a complete command-2 request privately; output changes only on Ok.
EncodeStatus EncodeCheckRequest(ModuleProfile const& profile,
    CheckPlan const& plan, Bytes& output);
CheckPlanValidation InspectCheckPlan(CheckPlan const& plan,
    WardenCheckPlanBudget& budget);
/** Stable non-secret label for plan validation diagnostics. */
char const* ToString(CheckPlanValidation validation);

// Parses exactly the pending ordered plan and publishes no partial result.
DecodeStatus DecodeCheckResult(ByteView body, CheckPlan const& plan,
    CheckBatchResult& result);

// DecodeClient accepts only the four bootstrap commands and their exact sizes;
// trailing bytes are malformed rather than silently ignored.
DecodeStatus DecodeClient(ByteView body, ClientMessage& message);
}

#endif

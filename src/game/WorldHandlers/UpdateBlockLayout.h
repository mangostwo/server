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
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

#ifndef MANGOS_H_UPDATEBLOCKLAYOUT
#define MANGOS_H_UPDATEBLOCKLAYOUT

#include "ByteBuffer.h"

/**
 * The bits of the `uint16` that opens an object's movement block.
 */
enum ObjectUpdateFlags
{
    UPDATEFLAG_NONE                 = 0x0000,
    UPDATEFLAG_SELF                 = 0x0001,
    UPDATEFLAG_TRANSPORT            = 0x0002,
    UPDATEFLAG_HAS_ATTACKING_TARGET = 0x0004,
    UPDATEFLAG_LOWGUID              = 0x0008,
    UPDATEFLAG_HIGHGUID             = 0x0010,
    UPDATEFLAG_LIVING               = 0x0020,
    UPDATEFLAG_HAS_POSITION         = 0x0040,
    UPDATEFLAG_VEHICLE              = 0x0080,
    UPDATEFLAG_POSITION             = 0x0100,
    UPDATEFLAG_ROTATION             = 0x0200
};

namespace UpdateBlock
{
    /**
     * @brief Everything the tail of a movement block can carry, already resolved.
     *
     * The values are the caller's business -- which magic constant a unit's "low guid"
     * word actually holds, whose facing the vehicle block sends. Only the ORDER is this
     * unit's business, and it lives here so it can be pinned by a test.
     */
    struct MovementTail
    {
        uint32 lowGuid = 0;                 ///< UPDATEFLAG_LOWGUID
        uint32 highGuid = 0;                ///< UPDATEFLAG_HIGHGUID
        uint64 attackingTarget = 0;         ///< UPDATEFLAG_HAS_ATTACKING_TARGET, sent packed
        uint32 transportTime = 0;           ///< UPDATEFLAG_TRANSPORT: path progress, not a clock
        uint32 vehicleId = 0;               ///< UPDATEFLAG_VEHICLE: the Vehicle.dbc row id
        float  vehicleFacing = 0.0f;        ///< UPDATEFLAG_VEHICLE: the icon's rotation on the map
        int64  rotation = 0;                ///< UPDATEFLAG_ROTATION, packed quaternion
    };

    /**
     * @brief Write the tail of a movement block: everything after the position section.
     *
     * THE ORDER IS NOT NUMERIC, and nobody guesses it right twice. The 3.3.5a client's
     * parser reads
     *
     *     0x08 LOWGUID -> 0x10 HIGHGUID -> 0x04 HAS_ATTACKING_TARGET
     *          -> 0x02 TRANSPORT -> 0x80 VEHICLE -> 0x200 ROTATION
     *
     * -- confirmed against the 12340 binary, where the vehicle branch is not a flag test
     * at all but a sign-bit test on the low byte of the flag word (`0x80`), sitting
     * between the transport word and the rotation. Put the vehicle block anywhere else
     * and every field after it is read from the wrong offset: the client does not
     * validate, it just believes the stream.
     *
     * That block is also the ONLY reason an Icecrown gunship can be an icon on the zone
     * map. The client fills its battlefield-vehicle list exclusively from the CREATE
     * path, gated on the unit having a Vehicle.dbc row whose Flags carry 0x10000000 --
     * so a vehicle id arriving later, by SMSG_SET_VEHICLE_REC_ID, attaches the component
     * and still never draws an icon.
     */
    void WriteMovementTail(ByteBuffer& data, uint16 updateFlags, MovementTail const& tail);
}

#endif

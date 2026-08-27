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

#include "UpdateBlockLayout.h"

namespace UpdateBlock
{
    void WriteMovementTail(ByteBuffer& data, uint16 updateFlags, MovementTail const& tail)
    {
        // 0x08
        if (updateFlags & UPDATEFLAG_LOWGUID)
        {
            data << uint32(tail.lowGuid);
        }

        // 0x10
        if (updateFlags & UPDATEFLAG_HIGHGUID)
        {
            data << uint32(tail.highGuid);
        }

        // 0x04 -- packed guid of the current target
        if (updateFlags & UPDATEFLAG_HAS_ATTACKING_TARGET)
        {
            data.appendPackGUID(tail.attackingTarget);
        }

        // 0x02
        if (updateFlags & UPDATEFLAG_TRANSPORT)
        {
            data << uint32(tail.transportTime);
        }

        // 0x80 -- in the binary this is a sign-bit test on the low byte, right here,
        // between the transport word and the rotation. Not last, not numeric.
        if (updateFlags & UPDATEFLAG_VEHICLE)
        {
            data << uint32(tail.vehicleId);
            data << float(tail.vehicleFacing);
        }

        // 0x200
        if (updateFlags & UPDATEFLAG_ROTATION)
        {
            data << int64(tail.rotation);
        }
    }
}

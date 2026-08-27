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

#include "TestHarness.h"

#include "UpdateBlockLayout.h"

/**
 * @file
 * @brief The create block's movement tail, pinned byte for byte.
 *
 * THE ORDER IS NOT NUMERIC. The 3.3.5a client reads the tail as
 *
 *     0x08 LOWGUID -> 0x10 HIGHGUID -> 0x04 HAS_ATTACKING_TARGET
 *          -> 0x02 TRANSPORT -> 0x80 VEHICLE -> 0x200 ROTATION
 *
 * which is neither ascending nor descending, and which nobody reconstructs correctly
 * from memory. It was read out of the 12340 binary's parser, where the vehicle branch
 * is not even a flag test -- it is a sign-bit test on the low byte of the flag word,
 * sitting between the transport word and the rotation.
 *
 * The failure mode is why this is worth a test at all: get the order wrong and nothing
 * rejects the packet. The client has no length check and no validation here; it just
 * keeps reading, so every field after the mistake is decoded from the wrong offset.
 * A vehicle id lands in the rotation, a facing becomes a guid, and what you see is a
 * unit at the wrong place -- or nothing at all -- with no error anywhere.
 *
 * These bytes are also the only thing that makes an Icecrown gunship an icon on the
 * zone map: the client fills its battlefield-vehicle list exclusively from the CREATE
 * path, so the vehicle id has to be in this block and not in a later
 * SMSG_SET_VEHICLE_REC_ID.
 */

namespace
{
    std::string TailHex(uint16 flags, UpdateBlock::MovementTail const& tail)
    {
        ByteBuffer buf;
        UpdateBlock::WriteMovementTail(buf, flags, tail);
        return testing::BytesToHex(buf.contents(), buf.size());
    }

    /// A tail whose every field is distinguishable from every other in the hex dump.
    UpdateBlock::MovementTail SampleTail()
    {
        UpdateBlock::MovementTail tail;
        tail.lowGuid = 0x0000000B;                          // what a unit actually sends
        tail.highGuid = 0x0000000B;
        tail.attackingTarget = 0x0000000000000005ULL;       // packs to 01 05
        tail.transportTime = 0x11223344;
        tail.vehicleId = 245;                               // Vehicle.dbc 245, Orgrim's Hammer
        tail.vehicleFacing = 1.0f;                          // 0x3F800000, exact
        tail.rotation = 0x0102030405060708LL;
        return tail;
    }
}

TEST(UpdateBlock_flag_bits_are_the_wire_values)
{
    // Renumbering these is renumbering the protocol. Nothing else in the tree would
    // notice; the client would simply read a different block than the one sent.
    CHECK_EQ(int(UPDATEFLAG_SELF), 0x0001);
    CHECK_EQ(int(UPDATEFLAG_TRANSPORT), 0x0002);
    CHECK_EQ(int(UPDATEFLAG_HAS_ATTACKING_TARGET), 0x0004);
    CHECK_EQ(int(UPDATEFLAG_LOWGUID), 0x0008);
    CHECK_EQ(int(UPDATEFLAG_HIGHGUID), 0x0010);
    CHECK_EQ(int(UPDATEFLAG_LIVING), 0x0020);
    CHECK_EQ(int(UPDATEFLAG_HAS_POSITION), 0x0040);
    CHECK_EQ(int(UPDATEFLAG_VEHICLE), 0x0080);
    CHECK_EQ(int(UPDATEFLAG_POSITION), 0x0100);
    CHECK_EQ(int(UPDATEFLAG_ROTATION), 0x0200);
}

TEST(UpdateBlock_tail_writes_the_whole_queue_in_client_order)
{
    const uint16 flags = UPDATEFLAG_LOWGUID | UPDATEFLAG_HIGHGUID |
                         UPDATEFLAG_HAS_ATTACKING_TARGET | UPDATEFLAG_TRANSPORT |
                         UPDATEFLAG_VEHICLE | UPDATEFLAG_ROTATION;

    //  0b000000            low guid       (0x08)
    //  0b000000            high guid      (0x10)
    //  0105                packed target  (0x04)
    //  44332211            transport time (0x02)
    //  f5000000 0000803f   vehicle id + facing (0x80)
    //  0807060504030201    packed rotation     (0x200)
    CHECK_STR(TailHex(flags, SampleTail()),
              std::string("0b0000000b0000000105"
                          "44332211"
                          "f5000000"
                          "0000803f"
                          "0807060504030201"));
}

TEST(UpdateBlock_tail_order_is_not_the_numeric_order_of_the_flags)
{
    // 0x04 is numerically before 0x08, and goes out AFTER it. Sorting the queue -- the
    // obvious "tidy-up" -- breaks the wire precisely here.
    UpdateBlock::MovementTail tail = SampleTail();

    CHECK_STR(TailHex(UPDATEFLAG_LOWGUID | UPDATEFLAG_HAS_ATTACKING_TARGET, tail),
              std::string("0b0000000105"));
}

TEST(UpdateBlock_vehicle_sits_between_the_transport_word_and_the_rotation)
{
    UpdateBlock::MovementTail tail = SampleTail();

    // Transport time, then the vehicle pair, then the rotation. Moving the vehicle block
    // to the end -- where its 0x80 bit would put it if the order were numeric -- gives
    // the client a rotation where it expects a vehicle id.
    CHECK_STR(TailHex(UPDATEFLAG_TRANSPORT | UPDATEFLAG_VEHICLE | UPDATEFLAG_ROTATION, tail),
              std::string("44332211"
                          "f5000000"
                          "0000803f"
                          "0807060504030201"));
}

TEST(UpdateBlock_vehicle_block_is_a_uint32_id_then_a_float_facing)
{
    UpdateBlock::MovementTail tail;
    tail.vehicleId = 230;                                   // Vehicle.dbc 230, The Skybreaker
    tail.vehicleFacing = -2.0f;                             // 0xC0000000, exact

    // Eight bytes, in that order. The facing is not decoration: the client rotates the
    // map icon by it (SetRotation(orientation) in WorldMapFrame.lua), so a swapped pair
    // here is a ship pointing wherever its vehicle id happened to look like as a float.
    CHECK_STR(TailHex(UPDATEFLAG_VEHICLE, tail), std::string("e6000000000000c0"));
}

TEST(UpdateBlock_absent_flags_write_nothing)
{
    // A create block for a plain gameobject carries none of the tail. Writing a field
    // for a flag that is not set shifts everything the client reads afterwards.
    CHECK_STR(TailHex(UPDATEFLAG_LIVING | UPDATEFLAG_HAS_POSITION, SampleTail()),
              std::string(""));
}

TEST(UpdateBlock_an_empty_attacking_target_still_writes_its_packed_zero)
{
    // No victim is not "no field": the flag is what the client keys on, and a packed
    // zero guid is one byte. Skipping it truncates the block.
    UpdateBlock::MovementTail tail;

    CHECK_STR(TailHex(UPDATEFLAG_HAS_ATTACKING_TARGET, tail), std::string("00"));
}

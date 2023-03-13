/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2023 MaNGOS <https://getmangos.eu>
 * Copyright (C) 2008-2015 TrinityCore <http://www.trinitycore.org/>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

#include "ARC4.h"
#include "tomcrypt.h"

struct ARC4::_internal
{
    rc4_state _rc4;
    uint8 seed[256];
    uint8 key_len;
    _internal() :_rc4{}, key_len(0), seed{0}{}
    rc4_state* rc4() { return &_rc4; }
};

ARC4::ARC4(uint8 len)
{
    pstate = new _internal();
    pstate->key_len = len;
}

ARC4::~ARC4()
{
    delete pstate;
}

void ARC4::Init(uint8 *seed)
{
    memcpy(pstate->seed, seed, pstate->key_len);
    rc4_stream_setup(pstate->rc4(), seed, pstate->key_len);
}

void ARC4::UpdateData(int len, uint8 *data)
{
    rc4_stream_crypt(pstate->rc4(), data, len, data);
}

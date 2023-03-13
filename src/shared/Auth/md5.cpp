/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2023 MaNGOS <https://getmangos.eu>
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

#include "md5.h"
#include "tomcrypt.h"


struct MD5::_internal
{
    hash_state _hs;
    uint8 mDigest[MD5_DIGEST_LENGTH];
    hash_state* hstate() { return &_hs; }
};


MD5::MD5()
{
    pstate = new _internal();
    Initialize();

}

MD5::~MD5()
{
    delete pstate;
}

void MD5::UpdateData(const uint8* dta, int len)
{
    md5_process(pstate->hstate(), dta, len);
}


void MD5::Initialize()
{
    md5_init(pstate->hstate());
}

void MD5::Finalize(uint8* dest)
{
    md5_done(pstate->hstate(), pstate->mDigest);
    if (dest)
        memcpy(dest, pstate->mDigest, MD5_DIGEST_LENGTH);
}

uint8* MD5::GetDigest(void)
{
    return pstate->mDigest;
}

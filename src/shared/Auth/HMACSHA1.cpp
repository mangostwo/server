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

#include "Auth/HMACSHA1.h"
#include "BigNumber.h"
#include "tomcrypt.h"


struct HMACSHA1::_internal
{
    hmac_state _hs;
    hmac_state* hstate() { return &_hs; }
    uint8 mdigest[SHA_DIGEST_LENGTH];

};


HMACSHA1::HMACSHA1(uint32 len, uint8 *seed)
{
    pstate = new _internal();
    register_hash(&sha1_desc);
    hmac_init(pstate->hstate(), find_hash("sha1"), seed, len);
}

HMACSHA1::~HMACSHA1()
{
    delete pstate;
}

void HMACSHA1::UpdateBigNumber(BigNumber *bn)
{
    UpdateData(bn->AsByteArray(), bn->GetNumBytes());
}

void HMACSHA1::UpdateData(const uint8 *data, int length)
{
    hmac_process(pstate->hstate(), data, length);
}

void HMACSHA1::UpdateData(const std::string& str)
{
    hmac_process(pstate->hstate(), (uint8*)str.c_str(), str.length());
}

void HMACSHA1::Finalize()
{
    unsigned long length = SHA_DIGEST_LENGTH;
    hmac_done(pstate->hstate(), pstate->mdigest, &length);
}

uint8 *HMACSHA1::ComputeHash(BigNumber *bn)
{
    UpdateBigNumber(bn);
    Finalize();
    return pstate->mdigest;
}

uint8* HMACSHA1::GetDigest()
{
    return pstate->mdigest;
}
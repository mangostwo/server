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

#include "Auth/BigNumber.h"
#include "tommath.h"
#include "algorithm"
#include <random>

struct BigNumber::_internal
{
    mp_int _mp;
    uint8* _buff;
    _internal() :_mp{}, _buff(nullptr) {}
    ~_internal()
    {
        if (_buff) delete [] _buff;
        mp_clear(&_mp);
    }
    mp_int* mp() { return &_mp; }
};


BigNumber::BigNumber()
{
    state = new _internal();
    mp_init(state->mp());
}

BigNumber::BigNumber(const BigNumber& bn)
{
    state = new _internal();
    mp_init_copy(state->mp(), bn.state->mp());
}

BigNumber::BigNumber(uint32 val)
{
    state = new _internal();
    mp_init_u32(state->mp(), val);
}

BigNumber::BigNumber(const uint8* bytes, int len)
{
    state = new _internal();
    mp_init(state->mp());
    SetBinary(bytes, len);

}
BigNumber::~BigNumber()
{
    if (this->state)
    {
        delete (this->state);
    }
}

void BigNumber::SetDword(uint32 val)
{
    mp_set_u32(state->mp(), val);
}

void BigNumber::SetBinary(const uint8* bytes, int len)
{
    if (this->state->_buff)
        delete[] this->state->_buff;
    this->state->_buff = new uint8[len];
    memcpy(this->state->_buff, bytes, len);
    std::reverse(this->state->_buff, this->state->_buff + len);
    mp_from_ubin(state->mp(), this->state->_buff, len);
}

void BigNumber::SetHexStr(const char* str)
{
    mp_read_radix(state->mp(), str, 16);

}

void BigNumber::SetRand(int numbits)
{
    size_t bb = numbits / 8;
    size_t res = numbits % 8;
    if (res > 0) bb++;
    std::random_device rd;
    std::uniform_int_distribution<uint32> ud(0, 255);
    uint8* rand_arr = new uint8[bb];
    for (size_t i = 0; i < bb; ++i)
        rand_arr[i] = ud(rd);
    if(res>0) rand_arr[0] = rand_arr[0] & ((1 << res) - 1);
    mp_from_ubin(state->mp(), rand_arr, bb);
    delete [] rand_arr;
}

BigNumber& BigNumber::operator=(const BigNumber& bn)
{
    mp_copy(bn.state->mp(), state->mp());
    return *this;
}

BigNumber &BigNumber::operator+=(const BigNumber& bn)
{
    mp_add(this->state->mp(), bn.state->mp(), this->state->mp());
    return *this;
}

BigNumber& BigNumber::operator-=(const BigNumber& bn)
{
    mp_sub(this->state->mp(), bn.state->mp(), this->state->mp());
    return *this;
}

BigNumber& BigNumber::operator*=(const BigNumber& bn)
{
    mp_mul(this->state->mp(), bn.state->mp(), this->state->mp());
    return *this;
}

BigNumber& BigNumber::operator/=(const BigNumber& bn)
{
    mp_div(this->state->mp(), bn.state->mp(), this->state->mp(), nullptr);
    return *this;
}

BigNumber& BigNumber::operator%=(const BigNumber& bn)
{
    mp_mod(this->state->mp(), bn.state->mp(), this->state->mp());
    return *this;
}

BigNumber BigNumber::ModExp(const BigNumber& X, const BigNumber& P)
{
    //ret=(N^^X)mod P
    BigNumber ret;
    mp_exptmod(this->state->mp(), X.state->mp(), P.state->mp(), ret.state->mp());
    return ret;
}

int BigNumber::GetNumBytes(void)
{
    return mp_ubin_size(this->state->mp());
}


bool BigNumber::isZero() const
{
    return mp_iszero(this->state->mp());
}

uint8* BigNumber::AsByteArray(int minSize, bool reversed)
{
    int sz = mp_ubin_size(this->state->mp());
    int length = (minSize > sz) ? minSize : sz;
    if (this->state->_buff)
        delete[] this->state->_buff;

    this->state->_buff = new uint8[length];

    if (minSize == length)
    {
        memset((void*)this->state->_buff, 0, length);
    }

    mp_to_ubin(this->state->mp(), this->state->_buff,sz,nullptr);

    if (reversed)
    {
        std::reverse(this->state->_buff, this->state->_buff + length);
    }

    return this->state->_buff;
}

const unsigned char* BigNumber::AsHexStr()
{
    size_t sz;
    mp_radix_size(this->state->mp(), 16, &sz); //returns req buf size incl. null char
    if (this->state->_buff)
        delete[] this->state->_buff;
    this->state->_buff = new uint8[sz];
    mp_to_radix(this->state->mp(), (char*)this->state->_buff,sz, nullptr, 16);
    return this->state->_buff;
}


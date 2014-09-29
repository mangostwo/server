/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2014  MaNGOS project <http://getmangos.eu>
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

/// \addtogroup realmd
/// @{
/// \file

#ifndef MANGOS_H_AUTHSOCKET
#define MANGOS_H_AUTHSOCKET

#include "Common.h"
#include "Auth/BigNumber.h"
#include "Auth/Sha1.h"
#include "ByteBuffer.h"

#include "BufferedSocket.h"

/**
 * @brief Handle login commands
 *
 */
class AuthSocket: public BufferedSocket
{
    public:
        const static int s_BYTE_SIZE = 32; /**< TODO */

        /**
         * @brief
         *
         */
        AuthSocket();
        /**
         * @brief
         *
         */
        ~AuthSocket();

        /**
         * @brief
         *
         */
        void OnAccept() override;
        /**
         * @brief
         *
         */
        void OnRead() override;
        /**
         * @brief
         *
         * @param sha
         */
        void SendProof(Sha1Hash sha);
        /**
         * @brief
         *
         * @param pkt
         * @param acctid
         */
        void LoadRealmlist(ByteBuffer& pkt, uint32 acctid);

        /**
         * @brief
         *
         * @return bool
         */
        bool _HandleLogonChallenge();
        /**
         * @brief
         *
         * @return bool
         */
        bool _HandleLogonProof();
        /**
         * @brief
         *
         * @return bool
         */
        bool _HandleReconnectChallenge();
        /**
         * @brief
         *
         * @return bool
         */
        bool _HandleReconnectProof();
        /**
         * @brief
         *
         * @return bool
         */
        bool _HandleRealmList();

        /**
         * @brief data transfer handle for patch
         *
         * @return bool
         */
        bool _HandleXferResume();
        /**
         * @brief
         *
         * @return bool
         */
        bool _HandleXferCancel();
        /**
         * @brief
         *
         * @return bool
         */
        bool _HandleXferAccept();

        /**
         * @brief
         *
         * @param rI
         */
        void _SetVSFields(const std::string& rI);

    private:

        BigNumber N, s, g, v; /**< TODO */
        BigNumber b, B; /**< TODO */
        BigNumber K; /**< TODO */
        BigNumber _reconnectProof; /**< TODO */

        bool _authed; /**< TODO */

        std::string _login; /**< TODO */
        std::string _safelogin; /**< TODO */

        std::string _localizationName; /**< Since GetLocaleByName() is _NOT_ bijective, we have to store the locale as a string. Otherwise we can't differ between enUS and enGB, which is important for the patch system */
        uint16 _build; /**< TODO */
        AccountTypes _accountSecurityLevel; /**< TODO */

        ACE_HANDLE patch_; /**< TODO */

        /**
         * @brief
         *
         */
        void InitPatch();
};
#endif
/// @}

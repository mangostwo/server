/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2025 MaNGOS <https://www.getmangos.eu>
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
#include "OpenSSLProvider.h"
#include "Log/Log.h"
#if defined(OPENSSL_VERSION_MAJOR) && (OPENSSL_VERSION_MAJOR >= 3)
#include <openssl/provider.h>
#endif

ARC4::ARC4(uint8 len) : m_cipherContext()
{
#if defined(OPENSSL_VERSION_MAJOR) && (OPENSSL_VERSION_MAJOR >= 3)
    // Provider management is now handled by OpenSSLProviderManager
    if (!m_providerManager.IsInitialized())
    {
        sLog.outError("ARC4: Failed to initialize OpenSSL providers");
        return;
    }
#endif

    if (!m_cipherContext.IsValid())
    {
        sLog.outError("ARC4: Failed to create cipher context");
        return;
    }

    EVP_EncryptInit_ex(m_cipherContext.Get(), EVP_rc4(), NULL, NULL, NULL);
    EVP_CIPHER_CTX_set_key_length(m_cipherContext.Get(), len);
}

ARC4::ARC4(uint8 *seed, uint8 len) : m_cipherContext()
{
#if defined(OPENSSL_VERSION_MAJOR) && (OPENSSL_VERSION_MAJOR >= 3)
    // Provider management is now handled by OpenSSLProviderManager
    if (!m_providerManager.IsInitialized())
    {
        sLog.outError("ARC4: Failed to initialize OpenSSL providers");
        return;
    }
#endif

    if (!m_cipherContext.IsValid())
    {
        sLog.outError("ARC4: Failed to create cipher context");
        return;
    }

    EVP_EncryptInit_ex(m_cipherContext.Get(), EVP_rc4(), NULL, NULL, NULL);
    EVP_CIPHER_CTX_set_key_length(m_cipherContext.Get(), len);
    EVP_EncryptInit_ex(m_cipherContext.Get(), NULL, NULL, seed, NULL);
}

ARC4::~ARC4()
{
    // Cleanup is now handled automatically by RAII wrappers
}

void ARC4::Init(uint8 *seed)
{
    if (m_cipherContext.IsValid())
    {
        EVP_EncryptInit_ex(m_cipherContext.Get(), NULL, NULL, seed, NULL);
    }
}

void ARC4::UpdateData(int len, uint8 *data)
{
    if (!m_cipherContext.IsValid())
    {
        sLog.outError("ARC4: Invalid cipher context, cannot update data");
        return;
    }

    int outlen = 0;
    EVP_EncryptUpdate(m_cipherContext.Get(), data, &outlen, data, len);
    EVP_EncryptFinal_ex(m_cipherContext.Get(), data, &outlen);
}

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

#include "TestHarness.h"

#include "WardenCheckCatalog.h"
#include "WardenCheckFixtures.h"
#include "WardenManager.h"
#include "WardenServer.h"

#include <openssl/crypto.h>
#include <openssl/evp.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

namespace
{
warden::SessionKey TestSessionKey()
{
    warden::SessionKey key{};
    for (uint8 i = 0; i < 39; ++i)
        key[i] = uint8(i + 1);
    return key;
}

warden::Bytes FromHex(char const* text)
{
    auto nibble = [](char value) -> uint8
    {
        if (value >= '0' && value <= '9')
            return uint8(value - '0');
        if (value >= 'a' && value <= 'f')
            return uint8(value - 'a' + 10);
        return uint8(value - 'A' + 10);
    };

    size_t const length = std::strlen(text);
    warden::Bytes bytes;
    bytes.reserve(length / 2);
    for (size_t i = 0; i < length; i += 2)
        bytes.push_back(uint8((nibble(text[i]) << 4) | nibble(text[i + 1])));
    return bytes;
}

template <size_t Size>
std::array<uint8, Size> FixedBytes(char const* text)
{
    warden::Bytes const bytes = FromHex(text);
    std::array<uint8, Size> result{};
    if (bytes.size() == result.size())
        std::copy(bytes.begin(), bytes.end(), result.begin());
    return result;
}

warden::WardenCheckCatalog const& TestCheckCatalog()
{
    static warden::WardenCheckCatalog const catalog =
        warden::test::BuildInitialWardenCatalog();
    return catalog;
}

std::vector<warden::WardenCheckDefinition> TestChecks(uint32 build,
    std::string const& locale, std::initializer_list<uint32> checkIds)
{
    std::vector<warden::WardenCheckDefinition> checks;
    warden::WardenCheckProfile const* profile =
        TestCheckCatalog().Find(build, "Win", locale);
    if (!profile)
        return checks;
    for (uint32 checkId : checkIds)
    {
        auto const found = std::find_if(profile->checks.begin(),
            profile->checks.end(), [checkId](auto const& definition)
            {
                return warden::GetWardenCheckId(definition) == checkId;
            });
        if (found != profile->checks.end())
            checks.push_back(*found);
    }
    return checks;
}

std::vector<warden::WardenCheckDefinition> TestTimingChecks()
{
    return TestChecks(12340, "enUS", {65536});
}

std::vector<warden::WardenCheckDefinition> TestMemChecks()
{
    return TestChecks(12340, "enUS", {65536, 3});
}

bool EnsureTestCatalogPublished()
{
    warden::WardenManager& manager = warden::WardenManager::Instance();
    if (manager.HasPublishedCheckCatalog())
        return true;
    return manager.PublishCheckCatalog(
        std::make_shared<warden::WardenCheckCatalog const>(
            warden::test::BuildInitialWardenCatalog()));
}

bool Digest(EVP_MD const* algorithm, uint8 const* data, size_t size,
    uint8* output, size_t expectedSize)
{
    unsigned int length = 0;
    return EVP_Digest(data, size, output, &length, algorithm, nullptr) == 1 &&
        length == expectedSize;
}

class PeerRc4
{
public:
    ~PeerRc4()
    {
        OPENSSL_cleanse(m_permutation.data(), m_permutation.size());
    }

    void Initialize(std::array<uint8, 16> const& key)
    {
        for (size_t i = 0; i < m_permutation.size(); ++i)
            m_permutation[i] = uint8(i);
        uint32 j = 0;
        for (size_t i = 0; i < m_permutation.size(); ++i)
        {
            j = (j + m_permutation[i] + key[i % key.size()]) & 0xFF;
            std::swap(m_permutation[i], m_permutation[j]);
        }
        m_i = 0;
        m_j = 0;
    }

    void Transform(warden::Bytes& bytes)
    {
        for (uint8& byte : bytes)
        {
            m_i = uint8(m_i + 1);
            m_j = uint8(m_j + m_permutation[m_i]);
            std::swap(m_permutation[m_i], m_permutation[m_j]);
            byte ^= m_permutation[uint8(m_permutation[m_i] +
                m_permutation[m_j])];
        }
    }

private:
    std::array<uint8, 256> m_permutation{};
    uint8 m_i = 0;
    uint8 m_j = 0;
};

class BootstrapPeer
{
public:
    explicit BootstrapPeer(warden::SessionKey const& sessionKey)
    {
        std::array<uint8, 20> left{};
        std::array<uint8, 20> right{};
        std::array<uint8, 20> current{};
        std::array<uint8, 60> input{};
        std::array<uint8, 40> generated{};

        bool success = Digest(EVP_sha1(), sessionKey.data(), 20,
                left.data(), left.size()) &&
            Digest(EVP_sha1(), sessionKey.data() + 20, 20,
                right.data(), right.size());
        size_t offset = 0;
        while (success && offset < 32)
        {
            std::copy(left.begin(), left.end(), input.begin());
            std::copy(current.begin(), current.end(), input.begin() + 20);
            std::copy(right.begin(), right.end(), input.begin() + 40);
            success = Digest(EVP_sha1(), input.data(), input.size(),
                current.data(), current.size());
            size_t const count = std::min(current.size(), size_t(32) - offset);
            if (success)
            {
                std::copy(current.begin(), current.begin() + count,
                    generated.begin() + offset);
                offset += count;
            }
        }

        std::array<uint8, 16> clientKey{};
        std::array<uint8, 16> serverKey{};
        if (success)
        {
            std::copy(generated.begin(), generated.begin() + 16,
                clientKey.begin());
            std::copy(generated.begin() + 16, generated.begin() + 32,
                serverKey.begin());
        }
        m_clientToServer.Initialize(clientKey);
        m_serverToClient.Initialize(serverKey);

        OPENSSL_cleanse(left.data(), left.size());
        OPENSSL_cleanse(right.data(), right.size());
        OPENSSL_cleanse(current.data(), current.size());
        OPENSSL_cleanse(input.data(), input.size());
        OPENSSL_cleanse(generated.data(), generated.size());
        OPENSSL_cleanse(clientKey.data(), clientKey.size());
        OPENSSL_cleanse(serverKey.data(), serverKey.size());
    }

    warden::Bytes DecryptServer(warden::Bytes const& encrypted)
    {
        warden::Bytes plain = encrypted;
        m_serverToClient.Transform(plain);
        return plain;
    }

    warden::Bytes EncryptClient(warden::Bytes plain)
    {
        m_clientToServer.Transform(plain);
        return plain;
    }

    void InstallModuleKeys()
    {
        std::array<uint8, 16> const clientKey = FixedBytes<16>(
            "7F96EEFDA5B63D20A4DF8E00CBF48304");
        std::array<uint8, 16> const serverKey = FixedBytes<16>(
            "C2B7ADEDFCCCA9C2BFB3F85602BA809B");
        m_clientToServer.Initialize(clientKey);
        m_serverToClient.Initialize(serverKey);
    }

private:
    PeerRc4 m_clientToServer;
    PeerRc4 m_serverToClient;
};

struct ManagerLocale
{
    std::string value;
};

struct Harness
{
    explicit Harness(bool allowSend = true,
        std::vector<warden::WardenCheckDefinition> checks =
            TestTimingChecks())
        : peer(TestSessionKey()), sendSucceeds(allowSend)
    {
        warden::WardenModuleCatalog catalog;
        warden::ModuleProfile const* profile = catalog.Find(12340, "Win");
        warden::WardenCryptoContext crypto;
        if (!profile || !crypto.Initialize(TestSessionKey()))
            return;

        server = std::make_unique<warden::WardenServer>(*profile,
            std::move(crypto), MakeSend(), warden::WardenLimits{},
            warden::WardenConfiguration{}, false, MakeLifecycleObserver(),
            MakeEvidenceObserver(), std::move(checks));
    }

    explicit Harness(ManagerLocale locale, bool allowSend = true,
        warden::WardenCreationOptions options = {})
        : peer(TestSessionKey()), sendSucceeds(allowSend)
    {
        if (!EnsureTestCatalogPublished())
            return;
        server = warden::WardenManager::Instance().Create(12340, "Win",
            locale.value, TestSessionKey(), MakeSend(), options,
            MakeLifecycleObserver(), MakeEvidenceObserver());
    }

    void SendClient(warden::Bytes plain)
    {
        warden::Bytes encrypted = peer.EncryptClient(std::move(plain));
        server->HandleEncrypted({encrypted.data(), encrypted.size()});
    }

    warden::SendEncrypted MakeSend()
    {
        return [this](warden::Bytes const& bytes)
        {
            ++sendCalls;
            if (!sendSucceeds)
                return false;
            sent.push_back(bytes);
            return true;
        };
    }

    warden::LifecycleObserver MakeLifecycleObserver()
    {
        return [this](warden::WardenLifecycleEvent const& event)
        {
            events.push_back(event);
        };
    }

    warden::EvidenceBatchObserver MakeEvidenceObserver()
    {
        return [this](warden::WardenEvidenceBatch const& batch)
        {
            evidenceBatches.push_back(batch);
            evidenceEvents.insert(evidenceEvents.end(), batch.evidence.begin(),
                batch.evidence.end());
            if (queueConfirmationId && server)
            {
                queueConfirmationResult =
                    server->QueueConfirmation(queueConfirmationId);
            }
        };
    }

    BootstrapPeer peer;
    bool sendSucceeds = true;
    size_t sendCalls = 0;
    std::vector<warden::Bytes> sent;
    std::vector<warden::WardenLifecycleEvent> events;
    std::vector<warden::WardenEvidenceBatch> evidenceBatches;
    std::vector<warden::WardenEvidence> evidenceEvents;
    uint32 queueConfirmationId = 0;
    bool queueConfirmationResult = false;
    std::unique_ptr<warden::WardenServer> server;
};

warden::Bytes ModuleOk()
{
    return {uint8(warden::ClientCommand::ModuleOk)};
}

warden::Bytes ModuleMissing()
{
    return {uint8(warden::ClientCommand::ModuleMissing)};
}

warden::Bytes CorrectHash()
{
    return FromHex("04568C054C781A972A6037A2290C22B52571A06F4E");
}

warden::Bytes ExactModuleInitialization()
{
    return FromHex(
        "0314006E676F6101000200804F0200C01802003025020010290200"
        "030800F8AC4E0F040000409D410001"
        "030800B4E9D7BA01010020AE460001");
}

bool StartAndReadModuleUse(Harness& harness)
{
    if (!harness.server || !harness.server->Start() || harness.sent.size() != 1)
        return false;
    warden::Bytes const plain = harness.peer.DecryptServer(harness.sent[0]);
    return plain == FromHex(
        "0079C0768D657977D697E10BAD956CCED1"
        "AE25BC51063B77BD363C3EFE0FC173F9"
        "44490000");
}

bool ReachAwaitingHash(Harness& harness)
{
    if (!StartAndReadModuleUse(harness))
        return false;
    harness.SendClient(ModuleOk());
    if (harness.server->GetState() != warden::WardenState::AwaitingHash ||
        harness.sent.size() != 2)
        return false;
    return harness.peer.DecryptServer(harness.sent[1]) ==
        FromHex("054D808D2C77D905C41A6380EC08586AFE");
}

bool ReachModuleReady(Harness& harness)
{
    if (!ReachAwaitingHash(harness))
        return false;
    warden::Bytes encrypted = harness.peer.EncryptClient(CorrectHash());
    harness.peer.InstallModuleKeys();
    harness.server->HandleEncrypted({encrypted.data(), encrypted.size()});
    if (harness.server->GetState() != warden::WardenState::ModuleReady ||
        harness.server->GetFailure() != warden::WardenFailure::None ||
        harness.sent.size() != 3)
        return false;

    bool const initialized = harness.peer.DecryptServer(harness.sent[2]) ==
        ExactModuleInitialization();
    // Model the world update that follows packet handling. No check is
    // eligible yet, but the external ModuleReady transition is now consumed.
    harness.server->Update(false, 0);
    return initialized;
}

bool StartTimingCheck(Harness& harness)
{
    if (!ReachModuleReady(harness))
        return false;

    harness.server->Update(true, 1000);
    if (harness.server->GetState() !=
            warden::WardenState::AwaitingCheckResult ||
        harness.sent.size() != 4)
        return false;

    return harness.peer.DecryptServer(harness.sent.back()) ==
        FromHex("0200287F");
}

bool StartTimingMpqCheck(Harness& harness)
{
    if (!ReachModuleReady(harness))
        return false;

    harness.server->Update(true, 1000);
    if (harness.server->GetState() !=
            warden::WardenState::AwaitingCheckResult ||
        harness.sent.size() != 4)
        return false;

    return harness.peer.DecryptServer(harness.sent.back()) == FromHex(
        "021B444246696C6573436C69656E745C41"
        "7265615461626C652E6462630028E7017F");
}

bool StartTimingMpqLuaCheck(Harness& harness)
{
    if (!ReachModuleReady(harness))
        return false;

    harness.server->Update(true, 1000);
    if (harness.server->GetState() !=
            warden::WardenState::AwaitingCheckResult ||
        harness.sent.size() != 4)
        return false;

    return harness.peer.DecryptServer(harness.sent.back()) == FromHex(
        "021B444246696C6573436C69656E745C41"
        "7265615461626C652E646263044F4B4159"
        "0028E701F4027F");
}

bool StartTimingMpqLuaMemCheck(Harness& harness)
{
    if (!ReachModuleReady(harness))
        return false;

    harness.server->Update(true, 1000);
    if (harness.server->GetState() !=
            warden::WardenState::AwaitingCheckResult ||
        harness.sent.size() != 4)
        return false;

    return harness.peer.DecryptServer(harness.sent.back()) == FromHex(
        "021B444246696C6573436C69656E745C41"
        "7265615461626C652E646263044F4B4159"
        "0028E701F4028C00C0A87D00287F");
}

bool StartTimingMemCheck(Harness& harness)
{
    if (!ReachModuleReady(harness))
        return false;

    harness.server->Update(true, 1000);
    if (harness.server->GetState() !=
            warden::WardenState::AwaitingCheckResult ||
        harness.sent.size() != 4)
        return false;

    return harness.peer.DecryptServer(harness.sent.back()) == FromHex(
        "0200288C00C0A87D00287F");
}
}

TEST(WardenServer_cache_hit_reaches_module_ready)
{
    Harness harness;
    REQUIRE(ReachModuleReady(harness));
    CHECK_EQ(harness.sendCalls, 3u);
    CHECK_EQ(harness.server->GetTransferCount(), uint8(0));
    REQUIRE(harness.events.size() == 1u);
    CHECK(harness.events[0].state == warden::WardenState::ModuleReady);
    CHECK(harness.events[0].failure == warden::WardenFailure::None);
    CHECK_EQ(harness.events[0].transferCount, uint8(0));
}

TEST(WardenServer_cache_miss_transfers_exact_custody_pinned_module_once)
{
    Harness harness;
    REQUIRE(StartAndReadModuleUse(harness));
    harness.SendClient(ModuleMissing());

    REQUIRE(harness.server->GetState() ==
        warden::WardenState::AwaitingTransferResult);
    REQUIRE(harness.sent.size() == 39u);

    warden::Bytes module;
    for (size_t i = 1; i < harness.sent.size(); ++i)
    {
        warden::Bytes const plain = harness.peer.DecryptServer(harness.sent[i]);
        REQUIRE(plain.size() >= 3);
        REQUIRE(plain[0] == uint8(warden::ServerCommand::ModuleCache));
        size_t const chunkSize = size_t(plain[1]) | (size_t(plain[2]) << 8);
        REQUIRE(chunkSize == plain.size() - 3);
        if (i < 38)
            CHECK_EQ(chunkSize, 500u);
        else
            CHECK_EQ(chunkSize, 256u);
        module.insert(module.end(), plain.begin() + 3, plain.end());
    }

    REQUIRE(module.size() == 18756u);
    std::array<uint8, 16> md5{};
    std::array<uint8, 32> sha256{};
    REQUIRE(Digest(EVP_md5(), module.data(), module.size(), md5.data(), md5.size()));
    REQUIRE(Digest(EVP_sha256(), module.data(), module.size(),
        sha256.data(), sha256.size()));
    CHECK_HEX(md5.data(), md5.size(), "79c0768d657977d697e10bad956cced1");
    CHECK_HEX(sha256.data(), sha256.size(),
        "6c68006a2f1fd31e7208204b3f7ceb94a6ce977876e13f2f703e9cd644482289");

    harness.SendClient(ModuleOk());
    REQUIRE(harness.server->GetState() == warden::WardenState::AwaitingHash);
    REQUIRE(harness.sent.size() == 40u);
    warden::Bytes const hashRequest =
        harness.peer.DecryptServer(harness.sent.back());
    CHECK_HEX(hashRequest.data(), hashRequest.size(),
        "054d808d2c77d905c41a6380ec08586afe");

    warden::Bytes encrypted = harness.peer.EncryptClient(CorrectHash());
    harness.peer.InstallModuleKeys();
    harness.server->HandleEncrypted({encrypted.data(), encrypted.size()});
    CHECK(harness.server->GetState() == warden::WardenState::ModuleReady);
    REQUIRE(harness.sent.size() == 41u);
    CHECK(harness.peer.DecryptServer(harness.sent.back()) ==
        ExactModuleInitialization());
    CHECK_EQ(harness.server->GetTransferCount(), uint8(1));
    REQUIRE(harness.events.size() == 1u);
    CHECK(harness.events[0].state == warden::WardenState::ModuleReady);
    CHECK(harness.events[0].failure == warden::WardenFailure::None);
    CHECK_EQ(harness.events[0].transferCount, uint8(1));
}

TEST(WardenServer_initialization_send_failure_is_terminal_without_retry)
{
    Harness harness;
    REQUIRE(ReachAwaitingHash(harness));

    warden::Bytes encrypted = harness.peer.EncryptClient(CorrectHash());
    harness.peer.InstallModuleKeys();
    harness.sendSucceeds = false;
    harness.server->HandleEncrypted({encrypted.data(), encrypted.size()});

    CHECK(harness.server->GetState() == warden::WardenState::Failed);
    CHECK(harness.server->GetFailure() == warden::WardenFailure::SendFailure);
    CHECK_EQ(harness.sendCalls, 3u);
    CHECK_EQ(harness.sent.size(), 2u);
    REQUIRE(harness.events.size() == 1u);
    CHECK(harness.events[0].state == warden::WardenState::Failed);
    CHECK(harness.events[0].failure == warden::WardenFailure::SendFailure);

    size_t const calls = harness.sendCalls;
    harness.server->HandleEncrypted({encrypted.data(), encrypted.size()});
    harness.server->Update(true, 30000);
    CHECK_EQ(harness.sendCalls, calls);
    CHECK_EQ(harness.events.size(), 1u);
}

TEST(WardenServer_second_module_missing_is_terminal_without_retransfer)
{
    Harness harness;
    REQUIRE(StartAndReadModuleUse(harness));
    harness.SendClient(ModuleMissing());
    REQUIRE(harness.sent.size() == 39u);
    for (size_t i = 1; i < harness.sent.size(); ++i)
        harness.peer.DecryptServer(harness.sent[i]);

    harness.SendClient(ModuleMissing());
    CHECK(harness.server->GetState() == warden::WardenState::Failed);
    CHECK(harness.server->GetFailure() ==
        warden::WardenFailure::ModuleDigestMismatch);
    CHECK_EQ(harness.sendCalls, 39u);
    CHECK_EQ(harness.server->GetTransferCount(), uint8(1));
}

TEST(WardenServer_classifies_terminal_protocol_failures)
{
    auto expectInitialFailure = [](warden::Bytes plain,
        warden::WardenFailure expected)
    {
        Harness harness;
        REQUIRE(StartAndReadModuleUse(harness));
        harness.SendClient(std::move(plain));
        CHECK(harness.server->GetState() == warden::WardenState::Failed);
        CHECK(harness.server->GetFailure() == expected);
        size_t const calls = harness.sendCalls;
        harness.server->HandleEncrypted({});
        harness.server->Update(false, 30000);
        CHECK_EQ(harness.sendCalls, calls);
    };

    expectInitialFailure({uint8(warden::ClientCommand::ModuleFailed)},
        warden::WardenFailure::ModuleLoadFailed);
    expectInitialFailure({uint8(warden::ClientCommand::ModuleOk), 0},
        warden::WardenFailure::MalformedPayload);
    expectInitialFailure({2}, warden::WardenFailure::UnexpectedCommand);
    expectInitialFailure(CorrectHash(), warden::WardenFailure::UnexpectedCommand);

    Harness wrongHash;
    REQUIRE(ReachAwaitingHash(wrongHash));
    warden::Bytes hash(21, 0);
    hash[0] = uint8(warden::ClientCommand::HashResult);
    wrongHash.SendClient(hash);
    CHECK(wrongHash.server->GetState() == warden::WardenState::Failed);
    CHECK(wrongHash.server->GetFailure() == warden::WardenFailure::HashMismatch);
}

TEST(WardenServer_replay_and_send_failure_are_terminal)
{
    Harness ready;
    REQUIRE(ReachModuleReady(ready));
    REQUIRE(ready.events.size() == 1u);
    ready.server->Update(false, 30000);
    CHECK_EQ(ready.events.size(), 1u);
    size_t const readyCalls = ready.sendCalls;
    ready.SendClient(ModuleOk());
    CHECK(ready.server->GetFailure() == warden::WardenFailure::Replay);
    CHECK_EQ(ready.sendCalls, readyCalls);
    REQUIRE(ready.events.size() == 2u);
    CHECK(ready.events[1].state == warden::WardenState::Failed);
    CHECK(ready.events[1].failure == warden::WardenFailure::Replay);
    CHECK_EQ(ready.events[1].transferCount, uint8(0));
    ready.SendClient(ModuleOk());
    ready.server->Update(false, 30000);
    CHECK_EQ(ready.events.size(), 2u);

    Harness failedSend(false);
    REQUIRE(failedSend.server != nullptr);
    CHECK(!failedSend.server->Start());
    CHECK(failedSend.server->GetFailure() == warden::WardenFailure::SendFailure);
    CHECK_EQ(failedSend.sendCalls, 1u);
    REQUIRE(failedSend.events.size() == 1u);
    CHECK(failedSend.events[0].state == warden::WardenState::Failed);
    CHECK(failedSend.events[0].failure == warden::WardenFailure::SendFailure);
    CHECK(!failedSend.server->Start());
    failedSend.server->HandleEncrypted({});
    failedSend.server->Update(false, 30000);
    CHECK_EQ(failedSend.sendCalls, 1u);
    CHECK_EQ(failedSend.events.size(), 1u);
}

TEST(WardenServer_deadlines_are_cumulative_in_each_waiting_state)
{
    auto expire = [](Harness& harness)
    {
        // A zero-length update is harmless in every waiting state.
        harness.server->Update(false, 0);
        harness.server->Update(false, 12000);
        harness.server->Update(false, 17999);
        CHECK(harness.server->GetState() != warden::WardenState::Failed);
        harness.server->Update(false, 1);
        CHECK(harness.server->GetState() == warden::WardenState::Failed);
        CHECK(harness.server->GetFailure() ==
            warden::WardenFailure::DeadlineExpired);
    };

    Harness status;
    REQUIRE(StartAndReadModuleUse(status));
    expire(status);

    Harness transfer;
    REQUIRE(StartAndReadModuleUse(transfer));
    transfer.SendClient(ModuleMissing());
    REQUIRE(transfer.server->GetState() ==
        warden::WardenState::AwaitingTransferResult);
    expire(transfer);

    Harness hash;
    REQUIRE(ReachAwaitingHash(hash));
    expire(hash);
}

TEST(WardenServer_new_deadline_charges_every_subsequent_update_interval)
{
    Harness harness;
    REQUIRE(StartAndReadModuleUse(harness));

    // WorldSession charges the old state before dispatching a queued response;
    // every later call therefore represents time owned by the new state.
    harness.server->Update(false, 0);
    harness.SendClient(ModuleOk());
    REQUIRE(harness.server->GetState() == warden::WardenState::AwaitingHash);

    harness.server->Update(false, 29999);
    CHECK(harness.server->GetState() != warden::WardenState::Failed);
    harness.server->Update(false, 1);
    CHECK(harness.server->GetState() == warden::WardenState::Failed);
    CHECK(harness.server->GetFailure() ==
        warden::WardenFailure::DeadlineExpired);
}

TEST(WardenManager_creation_is_inert_for_exact_12340_and_rejects_unknown_ones)
{
    REQUIRE(EnsureTestCatalogPublished());
    size_t calls = 0;
    auto send = [&calls](warden::Bytes const&)
    {
        ++calls;
        return true;
    };

    std::unique_ptr<warden::WardenServer> supported =
        warden::WardenManager::Instance().Create(12340, "Win", "enUS",
            TestSessionKey(), send);
    REQUIRE(supported != nullptr);
    CHECK_EQ(calls, 0u);
    CHECK(supported->Start());
    CHECK_EQ(calls, 1u);
    CHECK(supported->Start());
    CHECK_EQ(calls, 1u);

    std::unique_ptr<warden::WardenServer> unsupported =
        warden::WardenManager::Instance().Create(9999, "Win", "enUS",
            TestSessionKey(), send);
    CHECK(unsupported == nullptr);
    CHECK(warden::WardenManager::Instance().Create(12340, "OSX", "enUS",
        TestSessionKey(), send) == nullptr);
    CHECK_EQ(calls, 1u);
}

TEST(WardenManager_publishes_one_immutable_check_catalogue_snapshot)
{
    warden::WardenManager manager;
    std::shared_ptr<warden::WardenCheckCatalog const> first =
        std::make_shared<warden::WardenCheckCatalog>(
            warden::test::BuildInitialWardenCatalog());
    std::shared_ptr<warden::WardenCheckCatalog const> second =
        std::make_shared<warden::WardenCheckCatalog>(
            warden::test::BuildInitialWardenCatalog());
    std::shared_ptr<warden::WardenCheckCatalog const> empty =
        std::make_shared<warden::WardenCheckCatalog>();

    CHECK(!manager.HasPublishedCheckCatalog());
    CHECK(!manager.PublishCheckCatalog(nullptr));
    CHECK(!manager.PublishCheckCatalog(empty));
    CHECK(!manager.HasPublishedCheckCatalog());
    REQUIRE(manager.PublishCheckCatalog(first));
    CHECK(manager.HasPublishedCheckCatalog());
    warden::WardenCheckProfile const* selected =
        manager.FindCheckProfile(12340, "Win", "enUS");
    REQUIRE(selected != nullptr);
    CHECK_EQ(selected->checks.size(), size_t(4));
    CHECK(!manager.PublishCheckCatalog(empty));
    CHECK(!manager.PublishCheckCatalog(second));
    CHECK(manager.FindCheckProfile(12340, "Win", "enUS") == selected);
}

TEST(WardenManager_configuration_snapshot_defaults_and_reloads_coherently)
{
    warden::WardenManager manager;
    std::shared_ptr<warden::WardenConfiguration const> initial =
        manager.GetConfigurationSnapshot();
    REQUIRE(initial != nullptr);
    CHECK(initial->enforcementMode ==
        warden::WardenEnforcementMode::KickAndBan);
    CHECK_EQ(initial->normalMinSeconds, uint32(30));

    warden::WardenConfiguration published;
    published.enforcementMode = warden::WardenEnforcementMode::Kick;
    published.requireExactProfile = false;
    published.normalMinSeconds = 31;
    published.normalMaxSeconds = 41;
    published.aggressiveMinSeconds = 11;
    published.aggressiveMaxSeconds = 21;
    published.aggressiveThreshold = 6;
    published.banThreshold = 12;
    published.incidentWindowSeconds = 600;
    manager.PublishConfiguration(published);

    std::shared_ptr<warden::WardenConfiguration const> selected =
        manager.GetConfigurationSnapshot();
    REQUIRE(selected != nullptr);
    CHECK(selected->enforcementMode == warden::WardenEnforcementMode::Kick);
    CHECK(!selected->requireExactProfile);
    CHECK_EQ(selected->normalMinSeconds, uint32(31));
    CHECK_EQ(selected->normalMaxSeconds, uint32(41));
    CHECK_EQ(selected->aggressiveThreshold, uint32(6));
    CHECK_EQ(selected->banThreshold, uint32(12));
}

TEST(WardenManager_concurrent_configuration_reads_are_whole_snapshots)
{
    warden::WardenManager manager;
    warden::WardenConfiguration first;
    first.enforcementMode = warden::WardenEnforcementMode::Kick;
    first.requireExactProfile = false;
    first.normalMinSeconds = 31;
    first.normalMaxSeconds = 41;
    first.aggressiveMinSeconds = 11;
    first.aggressiveMaxSeconds = 21;
    first.aggressiveThreshold = 6;
    first.banThreshold = 12;
    first.incidentWindowSeconds = 600;
    warden::WardenConfiguration second;
    second.normalMinSeconds = 32;
    second.normalMaxSeconds = 42;
    second.aggressiveMinSeconds = 12;
    second.aggressiveMaxSeconds = 22;
    second.aggressiveThreshold = 7;
    second.banThreshold = 14;
    second.incidentWindowSeconds = 700;
    manager.PublishConfiguration(first);

    std::atomic<bool> coherent{true};
    std::thread writer([&manager, &first, &second]()
    {
        for (uint32 index = 0; index < 5000; ++index)
            manager.PublishConfiguration(index & 1 ? first : second);
    });
    std::thread reader([&manager, &coherent]()
    {
        for (uint32 index = 0; index < 5000; ++index)
        {
            auto const snapshot = manager.GetConfigurationSnapshot();
            bool const isFirst = snapshot &&
                snapshot->enforcementMode ==
                    warden::WardenEnforcementMode::Kick &&
                !snapshot->requireExactProfile &&
                snapshot->normalMinSeconds == 31 &&
                snapshot->normalMaxSeconds == 41 &&
                snapshot->aggressiveMinSeconds == 11 &&
                snapshot->aggressiveMaxSeconds == 21 &&
                snapshot->aggressiveThreshold == 6 &&
                snapshot->banThreshold == 12 &&
                snapshot->incidentWindowSeconds == 600;
            bool const isSecond = snapshot &&
                snapshot->enforcementMode ==
                    warden::WardenEnforcementMode::KickAndBan &&
                snapshot->requireExactProfile &&
                snapshot->normalMinSeconds == 32 &&
                snapshot->normalMaxSeconds == 42 &&
                snapshot->aggressiveMinSeconds == 12 &&
                snapshot->aggressiveMaxSeconds == 22 &&
                snapshot->aggressiveThreshold == 7 &&
                snapshot->banThreshold == 14 &&
                snapshot->incidentWindowSeconds == 700;
            if (!isFirst && !isSecond)
                coherent.store(false);
        }
    });
    writer.join();
    reader.join();
    CHECK(coherent.load());
}

TEST(WardenManager_selects_content_checks_only_for_the_exact_locale)
{
    Harness enUS(ManagerLocale{"enUS"});
    REQUIRE(StartTimingMpqLuaMemCheck(enUS));

    warden::WardenCreationOptions observe;
    observe.configuration.enforcementMode =
        warden::WardenEnforcementMode::Observe;
    Harness unsupported(ManagerLocale{"itIT"}, true, observe);
    REQUIRE(ReachModuleReady(unsupported));
    unsupported.server->Update(true, 60000);
    CHECK(unsupported.server->GetState() == warden::WardenState::ModuleReady);
    CHECK_EQ(unsupported.sent.size(), size_t(3));
    CHECK(unsupported.evidenceEvents.empty());
}

TEST(WardenManager_enforcing_modes_require_exact_check_profiles)
{
    REQUIRE(EnsureTestCatalogPublished());
    auto send = [](warden::Bytes const&) { return true; };
    std::array<char const*, 10> const locales =
    {{"enUS", "enGB", "deDE", "esES", "esMX", "frFR", "ruRU",
        "koKR", "zhCN", "zhTW"}};

    for (warden::WardenEnforcementMode mode :
        {warden::WardenEnforcementMode::Kick,
            warden::WardenEnforcementMode::KickAndBan})
    {
        warden::WardenCreationOptions options;
        options.configuration.enforcementMode = mode;
        for (char const* locale : locales)
        {
            CHECK(warden::WardenManager::Instance().Create(12340,
                "Win", locale, TestSessionKey(), send,
                options) != nullptr);
        }
        CHECK(warden::WardenManager::Instance().Create(12340,
            "Win", "itIT", TestSessionKey(), send, options) == nullptr);
    }
}

TEST(WardenManager_observe_mode_allows_missing_check_profile)
{
    REQUIRE(EnsureTestCatalogPublished());
    warden::WardenCreationOptions options;
    options.configuration.enforcementMode =
        warden::WardenEnforcementMode::Observe;

    CHECK(warden::WardenManager::Instance().Create(12340, "Win", "itIT",
        TestSessionKey(), [](warden::Bytes const&) { return true; },
        options) != nullptr);
}

TEST(WardenManager_identifies_only_exact_enforcement_profiles)
{
    REQUIRE(EnsureTestCatalogPublished());
    for (char const* locale : {"enUS", "enGB", "deDE", "esES", "esMX",
             "frFR", "ruRU", "koKR", "zhCN", "zhTW"})
        CHECK(warden::IsWardenEnforcementProfile(12340, "Win", locale));

    CHECK(!warden::IsWardenEnforcementProfile(12340, "Win", "itIT"));
    CHECK(!warden::IsWardenEnforcementProfile(12340, "OSX", "enUS"));
    CHECK(!warden::IsWardenEnforcementProfile(9999, "Win", "enUS"));
    CHECK(!warden::IsWardenEnforcementProfile(12340, "Win", ""));
}

TEST(WardenManager_classifies_exact_profile_admission_policy)
{
    using warden::WardenEnforcementMode;
    using warden::WardenProfileDisposition;

    CHECK(warden::ClassifyWardenProfile(WardenEnforcementMode::Observe,
        true, true) == WardenProfileDisposition::Observe);
    CHECK(warden::ClassifyWardenProfile(WardenEnforcementMode::Observe,
        true, false) == WardenProfileDisposition::Observe);
    CHECK(warden::ClassifyWardenProfile(WardenEnforcementMode::Observe,
        false, true) == WardenProfileDisposition::Observe);
    CHECK(warden::ClassifyWardenProfile(WardenEnforcementMode::Observe,
        false, false) == WardenProfileDisposition::Observe);

    for (WardenEnforcementMode mode :
        {WardenEnforcementMode::Kick, WardenEnforcementMode::KickAndBan})
    {
        CHECK(warden::ClassifyWardenProfile(mode, true, true) ==
            WardenProfileDisposition::Enforce);
        CHECK(warden::ClassifyWardenProfile(mode, false, true) ==
            WardenProfileDisposition::Enforce);
        CHECK(warden::ClassifyWardenProfile(mode, true, false) ==
            WardenProfileDisposition::Reject);
        CHECK(warden::ClassifyWardenProfile(mode, false, false) ==
            WardenProfileDisposition::Observe);
    }
}

TEST(WardenManager_forwards_initial_aggressive_state_to_the_planner)
{
    warden::WardenCreationOptions options;
    options.initialAggressive = true;
    Harness harness(ManagerLocale{"enUS"}, true, options);
    REQUIRE(ReachModuleReady(harness));

    harness.server->Update(false, 0);
    CHECK(harness.server->GetState() ==
        warden::WardenState::AwaitingCheckResult);
    CHECK_EQ(harness.sent.size(), size_t(4));
}

TEST(WardenServer_uninitialized_crypto_fails_before_sending)
{
    warden::WardenModuleCatalog catalog;
    warden::ModuleProfile const* profile = catalog.Find(12340, "Win");
    REQUIRE(profile != nullptr);

    size_t calls = 0;
    warden::WardenCryptoContext crypto;
    warden::WardenServer server(*profile, std::move(crypto),
        [&calls](warden::Bytes const&)
        {
            ++calls;
            return true;
        });
    CHECK(!server.Start());
    CHECK(server.GetFailure() == warden::WardenFailure::CryptoFailure);
    CHECK_EQ(calls, 0u);
}

TEST(WardenServer_ignores_prestart_data_and_can_start)
{
    REQUIRE(EnsureTestCatalogPublished());
    size_t calls = 0;
    std::unique_ptr<warden::WardenServer> server =
        warden::WardenManager::Instance().Create(12340, "Win", "enUS",
            TestSessionKey(), [&calls](warden::Bytes const&)
            {
                ++calls;
                return true;
            });
    REQUIRE(server != nullptr);

    uint8 const unsolicited = 0xA5;
    server->HandleEncrypted({&unsolicited, 1});
    CHECK(server->GetState() == warden::WardenState::AwaitingModuleStatus);
    CHECK(server->GetFailure() == warden::WardenFailure::None);
    CHECK_EQ(calls, 0u);

    CHECK(server->Start());
    CHECK(server->GetState() == warden::WardenState::AwaitingModuleStatus);
    CHECK(server->GetFailure() == warden::WardenFailure::None);
    CHECK_EQ(calls, 1u);
}

TEST(WardenServer_sends_one_timing_check_after_eligibility_and_reports_stable)
{
    Harness harness;
    REQUIRE(ReachModuleReady(harness));

    harness.server->Update(false, 60000);
    harness.server->Update(true, 999);
    CHECK(harness.server->GetState() == warden::WardenState::ModuleReady);
    CHECK_EQ(harness.sent.size(), 3u);

    harness.server->Update(true, 1);
    REQUIRE(harness.server->GetState() ==
        warden::WardenState::AwaitingCheckResult);
    REQUIRE(harness.sent.size() == 4u);
    warden::Bytes const request = harness.peer.DecryptServer(harness.sent.back());
    CHECK_HEX(request.data(), request.size(), "0200287f");

    harness.SendClient(FromHex("020500A7D43E250178563412"));
    CHECK(harness.server->GetState() == warden::WardenState::ModuleReady);
    CHECK(harness.server->GetFailure() == warden::WardenFailure::None);
    REQUIRE(harness.evidenceBatches.size() == 1u);
    CHECK_EQ(harness.evidenceBatches[0].requestId, uint32(1));
    CHECK(harness.evidenceBatches[0].purpose ==
        warden::CheckPlanPurpose::Initial);
    REQUIRE(harness.evidenceBatches[0].evidence.size() == 1u);
    REQUIRE(harness.evidenceEvents.size() == 1u);
    warden::WardenEvidence const& evidence = harness.evidenceEvents[0];
    CHECK_EQ(evidence.requestId, uint32(1));
    CHECK_EQ(evidence.checkId, uint32(65536));
    CHECK(evidence.checkType == warden::WardenCheckType::Timing);
    CHECK(evidence.evidenceClass ==
        warden::WardenEvidenceClass::ProtocolHealth);
    CHECK(evidence.outcome == warden::WardenCheckOutcome::Stable);
    CHECK_EQ(evidence.clientTick, uint32(0x12345678));

    // Model the world update that follows packet handling. The elapsed
    // interval predates the response and must not advance the fresh cadence.
    harness.server->Update(true, 0);
    harness.server->Update(true, 60000);
    CHECK_EQ(harness.sent.size(), 5u);
    CHECK(harness.server->GetState() ==
        warden::WardenState::AwaitingCheckResult);
    CHECK_EQ(harness.evidenceEvents.size(), 1u);
}

TEST(WardenServer_reports_unstable_timing_without_protocol_failure)
{
    Harness harness;
    REQUIRE(StartTimingCheck(harness));

    harness.SendClient(FromHex("020500A490E0960078563412"));
    CHECK(harness.server->GetState() == warden::WardenState::ModuleReady);
    CHECK(harness.server->GetFailure() == warden::WardenFailure::None);
    REQUIRE(harness.evidenceEvents.size() == 1u);
    warden::WardenEvidence const& evidence = harness.evidenceEvents[0];
    CHECK_EQ(evidence.checkId, uint32(65536));
    CHECK(evidence.checkType == warden::WardenCheckType::Timing);
    CHECK(evidence.evidenceClass ==
        warden::WardenEvidenceClass::ProtocolHealth);
    CHECK(evidence.outcome == warden::WardenCheckOutcome::Unstable);
    CHECK_EQ(evidence.clientTick, uint32(0x12345678));
}

TEST(WardenServer_combined_check_preserves_stream_and_reports_ordered_match)
{
    Harness harness(true, TestChecks(12340, "enUS", {65536, 1}));
    REQUIRE(StartTimingMpqCheck(harness));

    harness.SendClient(FromHex(
        "021A0009136C8F0104030201008C7CED99"
        "F8DDDD48296551EFE05A2CF27B26F818"));
    CHECK(harness.server->GetState() == warden::WardenState::ModuleReady);
    CHECK(harness.server->GetFailure() == warden::WardenFailure::None);
    REQUIRE(harness.evidenceBatches.size() == 1u);
    CHECK(harness.evidenceBatches[0].purpose ==
        warden::CheckPlanPurpose::Initial);
    REQUIRE(harness.evidenceBatches[0].evidence.size() == 2u);
    REQUIRE(harness.evidenceEvents.size() == 2u);

    warden::WardenEvidence const& timing = harness.evidenceEvents[0];
    CHECK_EQ(timing.requestId, uint32(1));
    CHECK_EQ(timing.checkId, uint32(65536));
    CHECK(timing.checkType == warden::WardenCheckType::Timing);
    CHECK(timing.evidenceClass ==
        warden::WardenEvidenceClass::ProtocolHealth);
    CHECK(timing.outcome == warden::WardenCheckOutcome::Stable);
    CHECK_EQ(timing.clientTick, uint32(0x01020304));

    warden::WardenEvidence const& mpq = harness.evidenceEvents[1];
    CHECK_EQ(mpq.requestId, uint32(1));
    CHECK_EQ(mpq.checkId, uint32(1));
    CHECK(mpq.checkType == warden::WardenCheckType::Mpq);
    CHECK(mpq.evidenceClass ==
        warden::WardenEvidenceClass::Corroboration);
    CHECK(mpq.outcome == warden::WardenCheckOutcome::Match);

    harness.SendClient(ModuleOk());
    CHECK(harness.server->GetState() == warden::WardenState::Failed);
    CHECK(harness.server->GetFailure() == warden::WardenFailure::Replay);
    CHECK_EQ(harness.evidenceEvents.size(), 2u);
}

TEST(WardenServer_valid_mpq_negatives_are_observation_only)
{
    Harness unavailable(true, TestChecks(12340, "enUS", {65536, 1}));
    REQUIRE(StartTimingMpqCheck(unavailable));
    unavailable.SendClient(FromHex("020600C06DA567010403020101"));
    CHECK(unavailable.server->GetState() == warden::WardenState::ModuleReady);
    CHECK(unavailable.server->GetFailure() == warden::WardenFailure::None);
    REQUIRE(unavailable.evidenceEvents.size() == 2u);
    CHECK(unavailable.evidenceEvents[1].outcome ==
        warden::WardenCheckOutcome::Unavailable);
    CHECK(unavailable.evidenceEvents[1].checkType ==
        warden::WardenCheckType::Mpq);
    CHECK(unavailable.evidenceEvents[1].evidenceClass ==
        warden::WardenEvidenceClass::Corroboration);

    Harness mismatch(true, TestChecks(12340, "enUS", {65536, 1}));
    REQUIRE(StartTimingMpqCheck(mismatch));
    mismatch.SendClient(FromHex(
        "021A000F45480201040302010000000000"
        "00000000000000000000000000000000"));
    CHECK(mismatch.server->GetState() == warden::WardenState::ModuleReady);
    CHECK(mismatch.server->GetFailure() == warden::WardenFailure::None);
    REQUIRE(mismatch.evidenceEvents.size() == 2u);
    CHECK(mismatch.evidenceEvents[1].outcome ==
        warden::WardenCheckOutcome::Mismatch);

    Harness bothNegative(true, TestChecks(12340, "enUS", {65536, 1}));
    REQUIRE(StartTimingMpqCheck(bothNegative));
    bothNegative.SendClient(FromHex("02060071EF43C6000403020101"));
    CHECK(bothNegative.server->GetState() ==
        warden::WardenState::ModuleReady);
    REQUIRE(bothNegative.evidenceEvents.size() == 2u);
    CHECK(bothNegative.evidenceEvents[0].outcome ==
        warden::WardenCheckOutcome::Unstable);
    CHECK(bothNegative.evidenceEvents[1].outcome ==
        warden::WardenCheckOutcome::Unavailable);
}

TEST(WardenServer_combined_lua_match_is_classified_without_text_evidence)
{
    Harness harness(true, TestChecks(12340, "enUS", {65536, 1, 2}));
    REQUIRE(StartTimingMpqLuaCheck(harness));

    harness.SendClient(FromHex(
        "0220008ABF61FB0104030201008C7CED99"
        "F8DDDD48296551EFE05A2CF27B26F818"
        "00044F6B6179"));
    CHECK(harness.server->GetState() == warden::WardenState::ModuleReady);
    CHECK(harness.server->GetFailure() == warden::WardenFailure::None);
    REQUIRE(harness.evidenceEvents.size() == 3u);
    CHECK(harness.evidenceEvents[0].checkType ==
        warden::WardenCheckType::Timing);
    CHECK(harness.evidenceEvents[1].checkType ==
        warden::WardenCheckType::Mpq);
    warden::WardenEvidence const& lua = harness.evidenceEvents[2];
    CHECK_EQ(lua.requestId, uint32(1));
    CHECK_EQ(lua.checkId, uint32(2));
    CHECK(lua.checkType == warden::WardenCheckType::Lua);
    CHECK(lua.evidenceClass == warden::WardenEvidenceClass::Corroboration);
    CHECK(lua.outcome == warden::WardenCheckOutcome::Match);

    harness.SendClient(ModuleOk());
    CHECK(harness.server->GetState() == warden::WardenState::Failed);
    CHECK(harness.server->GetFailure() == warden::WardenFailure::Replay);
    CHECK_EQ(harness.evidenceEvents.size(), 3u);
}

TEST(WardenServer_all_ten_locale_content_vectors_report_ordered_matches)
{
    struct ProfileVector
    {
        char const* locale;
        char const* result;
    };
    ProfileVector const vectors[] =
    {
        {"enUS", "0220008ABF61FB0104030201008C7CED99F8DDDD48296551EFE05A2CF27B26F81800044F6B6179"},
        {"enGB", "0220008ABF61FB0104030201008C7CED99F8DDDD48296551EFE05A2CF27B26F81800044F6B6179"},
        {"deDE", "021E00C38B78E60104030201000B4D01BDEB4F47DE030B57D81506093EB887EE0B00024F4B"},
        {"esES", "022300156D84F001040302010020EC8371EC168B4723AF6DE3AFE81D46843726F4000741636570746172"},
        {"esMX", "022300389E7E800104030201000E39F4AF09E3CF08925D41E61FBAC8EE16478FC9000741636570746172"},
        {"frFR", "021E0046D423D8010403020100E6F5A0C5C63056F63097420AE29B47ACA2E4D49600024F4B"},
        {"ruRU", "022000AD5EF5DF010403020100329BF203079002D36E05EBF54BD5746AA37E47C80004D09ED09A"},
        {"koKR", "02220066D34E5B01040302010039BCDE7E67F7DA4A366D15007DBAF3D438338E000006ED9995EC9DB8"},
        {"zhCN", "02220054BF39DD01040302010053538853E7026786EB30FCB247D7E8179A3CAAF80006E7A1AEE5AE9A"},
        {"zhTW", "0222002B576865010403020100ED14F2C71688B1DE9660F9CE04A62D63A9EB297A0006E7A2BAE5AE9A"}
    };

    for (ProfileVector const& vector : vectors)
    {
        REQUIRE(TestCheckCatalog().Find(12340, "Win",
            vector.locale) != nullptr);

        Harness harness(true, TestChecks(12340, vector.locale,
            {65536, 1, 2}));
        REQUIRE(StartTimingMpqLuaCheck(harness));
        harness.SendClient(FromHex(vector.result));

        CHECK(harness.server->GetState() == warden::WardenState::ModuleReady);
        CHECK(harness.server->GetFailure() == warden::WardenFailure::None);
        REQUIRE(harness.evidenceEvents.size() == 3u);
        CHECK(harness.evidenceEvents[1].checkType ==
            warden::WardenCheckType::Mpq);
        CHECK(harness.evidenceEvents[1].outcome ==
            warden::WardenCheckOutcome::Match);
        CHECK(harness.evidenceEvents[2].checkType ==
            warden::WardenCheckType::Lua);
        CHECK(harness.evidenceEvents[2].outcome ==
            warden::WardenCheckOutcome::Match);
    }
}

TEST(WardenServer_valid_lua_negatives_are_observation_only)
{
    Harness unavailable(true,
        TestChecks(12340, "enUS", {65536, 1, 2}));
    REQUIRE(StartTimingMpqLuaCheck(unavailable));
    unavailable.SendClient(FromHex(
        "021B00127E0B170104030201008C7CED99"
        "F8DDDD48296551EFE05A2CF27B26F81801"));
    CHECK(unavailable.server->GetState() == warden::WardenState::ModuleReady);
    CHECK(unavailable.server->GetFailure() == warden::WardenFailure::None);
    REQUIRE(unavailable.evidenceEvents.size() == 3u);
    CHECK(unavailable.evidenceEvents[2].checkType ==
        warden::WardenCheckType::Lua);
    CHECK(unavailable.evidenceEvents[2].outcome ==
        warden::WardenCheckOutcome::Unavailable);

    Harness mismatch(true, TestChecks(12340, "enUS", {65536, 1, 2}));
    REQUIRE(StartTimingMpqLuaCheck(mismatch));
    mismatch.SendClient(FromHex(
        "021F004C4C91E70104030201008C7CED99"
        "F8DDDD48296551EFE05A2CF27B26F818"
        "0003426164"));
    CHECK(mismatch.server->GetState() == warden::WardenState::ModuleReady);
    CHECK(mismatch.server->GetFailure() == warden::WardenFailure::None);
    REQUIRE(mismatch.evidenceEvents.size() == 3u);
    CHECK(mismatch.evidenceEvents[2].checkType ==
        warden::WardenCheckType::Lua);
    CHECK(mismatch.evidenceEvents[2].outcome ==
        warden::WardenCheckOutcome::Mismatch);
}

TEST(WardenServer_malformed_lua_result_publishes_no_partial_evidence)
{
    Harness harness(true, TestChecks(12340, "enUS", {65536, 1, 2}));
    REQUIRE(StartTimingMpqLuaCheck(harness));

    harness.SendClient(FromHex(
        "021B00E0D8BE640104030201008C7CED99"
        "F8DDDD48296551EFE05A2CF27B26F81802"));
    CHECK(harness.server->GetState() == warden::WardenState::Failed);
    CHECK(harness.server->GetFailure() ==
        warden::WardenFailure::MalformedPayload);
    CHECK(harness.evidenceBatches.empty());
    CHECK(harness.evidenceEvents.empty());
}

TEST(WardenServer_mem_results_are_identifier_only_and_ordered)
{
    Harness harness(true, TestMemChecks());
    REQUIRE(StartTimingMemCheck(harness));

    harness.SendClient(FromHex(
        "022E004627DD50010403020100"
        "B9601AD300E8769DF9FFE851FBFFFF688C29AF0068D816AF00B8B3120000E82D"
        "FDFFFFA3441AD300"));
    CHECK(harness.server->GetState() == warden::WardenState::ModuleReady);
    CHECK(harness.server->GetFailure() == warden::WardenFailure::None);
    REQUIRE(harness.evidenceEvents.size() == 2u);
    CHECK(harness.evidenceEvents[0].checkType ==
        warden::WardenCheckType::Timing);
    warden::WardenEvidence const& evidence = harness.evidenceEvents[1];
    CHECK_EQ(evidence.requestId, uint32(1));
    CHECK_EQ(evidence.checkId, uint32(3));
    CHECK(evidence.checkType == warden::WardenCheckType::Mem);
    CHECK(evidence.evidenceClass ==
        warden::WardenEvidenceClass::IntegrityInvariant);
    CHECK(evidence.outcome == warden::WardenCheckOutcome::Match);
}

TEST(WardenServer_batch_observer_can_queue_confirmation_after_state_commit)
{
    Harness harness(true, TestMemChecks());
    harness.queueConfirmationId = 3;
    REQUIRE(StartTimingMemCheck(harness));

    harness.SendClient(FromHex(
        "022E004627DD50010403020100"
        "B9601AD300E8769DF9FFE851FBFFFF688C29AF0068D816AF00B8B3120000E82D"
        "FDFFFFA3441AD300"));

    REQUIRE(harness.evidenceBatches.size() == 1u);
    CHECK_EQ(harness.evidenceBatches[0].requestId, uint32(1));
    CHECK(harness.evidenceBatches[0].purpose ==
        warden::CheckPlanPurpose::Initial);
    REQUIRE(harness.evidenceBatches[0].evidence.size() == 2u);
    CHECK(harness.queueConfirmationResult);
    CHECK(harness.server->GetState() == warden::WardenState::ModuleReady);

    harness.server->Update(true, 0);
    CHECK(harness.server->GetState() ==
        warden::WardenState::AwaitingCheckResult);
    CHECK_EQ(harness.sent.size(), size_t(5));
}

TEST(WardenServer_valid_mem_negatives_are_observation_only)
{
    Harness unavailable(true, TestMemChecks());
    REQUIRE(StartTimingMemCheck(unavailable));
    unavailable.SendClient(FromHex("020600C06DA567010403020101"));
    CHECK(unavailable.server->GetState() == warden::WardenState::ModuleReady);
    CHECK(unavailable.server->GetFailure() == warden::WardenFailure::None);
    REQUIRE(unavailable.evidenceEvents.size() == 2u);
    CHECK(unavailable.evidenceEvents[1].checkType ==
        warden::WardenCheckType::Mem);
    CHECK(unavailable.evidenceEvents[1].outcome ==
        warden::WardenCheckOutcome::Unavailable);

    Harness mismatch(true, TestMemChecks());
    REQUIRE(StartTimingMemCheck(mismatch));
    mismatch.SendClient(FromHex(
        "022E001B7333C8010403020100"
        "0000000000000000000000000000000000000000000000000000000000000000"
        "0000000000000000"));
    CHECK(mismatch.server->GetState() == warden::WardenState::ModuleReady);
    CHECK(mismatch.server->GetFailure() == warden::WardenFailure::None);
    REQUIRE(mismatch.evidenceEvents.size() == 2u);
    CHECK(mismatch.evidenceEvents[1].outcome ==
        warden::WardenCheckOutcome::Mismatch);
    CHECK(mismatch.evidenceEvents[1].evidenceClass ==
        warden::WardenEvidenceClass::IntegrityInvariant);
}

TEST(WardenServer_malformed_mem_result_publishes_no_partial_evidence)
{
    Harness harness(true, TestMemChecks());
    REQUIRE(StartTimingMemCheck(harness));

    harness.SendClient(FromHex("0206008AFC74C1010403020102"));
    CHECK(harness.server->GetState() == warden::WardenState::Failed);
    CHECK(harness.server->GetFailure() ==
        warden::WardenFailure::MalformedPayload);
    CHECK(harness.evidenceBatches.empty());
    CHECK(harness.evidenceEvents.empty());
}

TEST(WardenServer_combined_malformed_result_publishes_no_partial_evidence)
{
    Harness harness(true, TestChecks(12340, "enUS", {65536, 1}));
    REQUIRE(StartTimingMpqCheck(harness));

    harness.SendClient(FromHex("0206008AFC74C1010403020102"));
    CHECK(harness.server->GetState() == warden::WardenState::Failed);
    CHECK(harness.server->GetFailure() ==
        warden::WardenFailure::MalformedPayload);
    CHECK(harness.evidenceBatches.empty());
    CHECK(harness.evidenceEvents.empty());
}

TEST(WardenServer_invalid_internal_mpq_plan_fails_before_sending)
{
    std::vector<warden::WardenCheckDefinition> invalid =
        TestChecks(12340, "enUS", {65536, 1});
    REQUIRE(invalid.size() == 2u);
    std::get<warden::MpqCheckProfile>(invalid[1].payload).checkId = 0;
    Harness harness(true, invalid);
    REQUIRE(ReachModuleReady(harness));

    harness.server->Update(true, 1000);
    CHECK(harness.server->GetState() == warden::WardenState::Failed);
    CHECK(harness.server->GetFailure() ==
        warden::WardenFailure::UnexpectedCommand);
    CHECK_EQ(harness.sent.size(), 3u);
    CHECK(harness.evidenceEvents.empty());
}

TEST(WardenServer_timing_timeout_malformed_unexpected_and_send_fail_are_terminal)
{
    Harness timeout;
    REQUIRE(StartTimingCheck(timeout));
    // Loading blocks only new plan selection. An outstanding request keeps
    // owning its cumulative deadline while the session is not eligible.
    timeout.server->Update(false, 29999);
    CHECK(timeout.server->GetState() ==
        warden::WardenState::AwaitingCheckResult);
    timeout.server->Update(false, 1);
    CHECK(timeout.server->GetState() == warden::WardenState::Failed);
    CHECK(timeout.server->GetFailure() ==
        warden::WardenFailure::DeadlineExpired);

    Harness malformed;
    REQUIRE(StartTimingCheck(malformed));
    malformed.SendClient(FromHex("020500A6D43E250178563412"));
    CHECK(malformed.server->GetState() == warden::WardenState::Failed);
    CHECK(malformed.server->GetFailure() ==
        warden::WardenFailure::MalformedPayload);
    CHECK(malformed.evidenceEvents.empty());

    Harness unexpected;
    REQUIRE(StartTimingCheck(unexpected));
    unexpected.SendClient(ModuleOk());
    CHECK(unexpected.server->GetState() == warden::WardenState::Failed);
    CHECK(unexpected.server->GetFailure() ==
        warden::WardenFailure::UnexpectedCommand);
    CHECK(unexpected.evidenceEvents.empty());

    Harness sendFailure(true, TestChecks(12340, "enUS", {65536, 1}));
    REQUIRE(ReachModuleReady(sendFailure));
    sendFailure.sendSucceeds = false;
    sendFailure.server->Update(true, 1000);
    CHECK(sendFailure.server->GetState() == warden::WardenState::Failed);
    CHECK(sendFailure.server->GetFailure() ==
        warden::WardenFailure::SendFailure);
    CHECK(sendFailure.evidenceEvents.empty());
}

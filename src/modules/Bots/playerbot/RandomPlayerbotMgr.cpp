#include "Config/Config.h"
#include "../botpch.h"
#include "PlayerbotAIConfig.h"
#include "PlayerbotFactory.h"
#include "AccountMgr.h"
#include "ObjectMgr.h"
#include "DatabaseEnv.h"
#include "PlayerbotAI.h"
#include "Player.h"
#include "AiFactory.h"
#include "GuildTaskMgr.h"
#include "PlayerbotCommandServer.h"

#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"

using namespace ai;
using namespace MaNGOS;

INSTANTIATE_SINGLETON_1(RandomPlayerbotMgr);

map<uint8, vector<WorldLocation> > RandomPlayerbotMgr::locsPerLevelCache;

RandomPlayerbotMgr::RandomPlayerbotMgr() : PlayerbotHolder()
{
    sPlayerbotCommandServer.Start();
}

RandomPlayerbotMgr::~RandomPlayerbotMgr()
= default;

uint32 RandomPlayerbotMgr::GetMaxAllowedBotCount()
{
    return GetEventValue(0, "bot_count");
}

void RandomPlayerbotMgr::UpdateAIInternal(uint32 elapsed)
{
    SetNextCheckDelay(sPlayerbotAIConfig.randomBotUpdateInterval * 1000);

    if (!sPlayerbotAIConfig.randomBotAutologin || !sPlayerbotAIConfig.enabled)
    {
        DEBUG_LOG("Random bots are disabled. Check config.");
        return;
    }

    uint32 maxAllowedBotCount = GetMaxAllowedBotCount();
    if (!maxAllowedBotCount)
    {
        maxAllowedBotCount = urand(sPlayerbotAIConfig.minRandomBots, sPlayerbotAIConfig.maxRandomBots);
        SetEventValue(0, "bot_count", maxAllowedBotCount,
            urand(sPlayerbotAIConfig.randomBotCountChangeMinInterval, sPlayerbotAIConfig.randomBotCountChangeMaxInterval));
    }

    set<uint32> bots;
    GetBots(bots);
    uint32 botCount = bots.size();
    DEBUG_LOG("%u random bots are currently available.", botCount);

    if (botCount < maxAllowedBotCount)
    {
        AddRandomBots(bots);
    }

    botCount = bots.size();
    uint32 randomBotsPerInterval = sPlayerbotAIConfig.randomBotLoginAtStartup ? botCount :
                                   urand(sPlayerbotAIConfig.minRandomBotsPerInterval, sPlayerbotAIConfig.maxRandomBotsPerInterval);
    DEBUG_LOG("%u random bots are available, %u bots will be logged in.", botCount, randomBotsPerInterval);

    uint32 botProcessed = 0;
    for (uint32 bot : bots)
    {
        if (botProcessed >= randomBotsPerInterval)
        {
            break;
        }

        if (ProcessBot(bot))
        {
            botProcessed++;
        }
    }

    string out = "Random bots are now scheduled to be processed in the background. Next re-schedule in " +
            to_string(sPlayerbotAIConfig.randomBotUpdateInterval) + " seconds";
    DETAIL_LOG("%s", out.c_str());
    sWorld.SendWorldText(3, out.c_str());

    if (sLog.GetLogLevel() >= LOG_LVL_DETAIL) {
        PrintStats();
    }
}

void RandomPlayerbotMgr::AddRandomBots(set<uint32> &bots)
{
    DETAIL_LOG("Adding random bots...");

    vector<uint32> guids;
    uint32 maxAllowedBotCount = GetMaxAllowedBotCount();
    for (auto accountId : sPlayerbotAIConfig.randomBotAccounts)
    {
        if (!sAccountMgr.GetCharactersCount(accountId))
        {
            DEBUG_LOG("No character in rndbot account %d.", accountId);
            continue;
        }

        QueryResult* result = CharacterDatabase.PQuery("SELECT guid, race FROM characters WHERE account = '%u'", accountId);
        if (!result)
        {
            DEBUG_LOG("Unable to fetch characters for rndbot account %d", accountId);
            continue;
        }

        do
        {
            Field* fields = result->Fetch();
            uint32 guid = fields[0].GetUInt32();
            uint8 race = fields[1].GetUInt8();
            bool alliance = guids.size() % 2 == 0;
            if (bots.find(guid) == bots.end() &&
                ((alliance && IsAlliance(race)) || ((!alliance && !IsAlliance(race))
                    )))
            {
                guids.push_back(guid);
                SetEventValue(guid, "add", 1, urand(sPlayerbotAIConfig.minRandomBotInWorldTime, sPlayerbotAIConfig.maxRandomBotInWorldTime));
                uint32 randomTime = 30 + urand(sPlayerbotAIConfig.randomBotUpdateInterval, sPlayerbotAIConfig.randomBotUpdateInterval * 3);
                ScheduleRandomize(guid, randomTime);
                bots.insert(guid);
                DEBUG_LOG("Random bot %d is now available for login.", guid);
                if (bots.size() >= maxAllowedBotCount) break;
            }
        } while (result->NextRow());
        delete result;
    }
}

void RandomPlayerbotMgr::ScheduleRandomize(uint32 bot, uint32 time)
{
    SetEventValue(bot, "randomize", 1, time);
    SetEventValue(bot, "logout", 1, time + 30 + urand(sPlayerbotAIConfig.randomBotUpdateInterval, sPlayerbotAIConfig.randomBotUpdateInterval * 3));
}

void RandomPlayerbotMgr::ScheduleTeleport(uint32 bot, uint32 time)
{
    if (!time)
    {
        time = 60 + urand(sPlayerbotAIConfig.randomBotUpdateInterval, sPlayerbotAIConfig.randomBotUpdateInterval * 3);
    }
    SetEventValue(bot, "teleport", 1, time);
}

bool RandomPlayerbotMgr::ProcessBot(uint32 bot)
{
    DEBUG_LOG("Processing bot %d", bot);
    Player* player = GetPlayerBot(bot);
    if (!IsRandomBot(bot))
    {
        if (!player || !player->GetGroup())
        {
            DETAIL_LOG("Bot %d expired", bot);
        }
        return true;
    }

    if (!player)
    {
        DETAIL_LOG("Bot %d is not online. Logging in...", bot);
        AddPlayerBot(bot, 0);
        if (!GetEventValue(bot, "online"))
        {
            SetEventValue(bot, "online", 1, sPlayerbotAIConfig.minRandomBotInWorldTime);
            ScheduleTeleport(bot, 30);
        }
        return true;
    }

    PlayerbotAI* ai = player->GetPlayerbotAI();
    if (!ai)
    {
        return false;
    }

    if (player->GetGroup())
    {
        DEBUG_LOG("Skipping bot %d as it is in group", bot);
        return false;
    }

    ai->GetAiObjectContext()->GetValue<bool>("random bot update")->Set(true);
    return true;
}

bool RandomPlayerbotMgr::ProcessBot(Player* player)
{
    player->GetPlayerbotAI()->GetAiObjectContext()->GetValue<bool>("random bot update")->Set(false);

    uint32 bot = player->GetGUIDLow();
    DEBUG_LOG("Processing random bot %d", bot);
    if (player->IsDead())
    {
        if (!GetEventValue(bot, "dead"))
        {
            DEBUG_LOG("Setting dead flag for bot %d", bot);
            uint32 randomTime = urand(sPlayerbotAIConfig.minRandomBotReviveTime, sPlayerbotAIConfig.maxRandomBotReviveTime);
            SetEventValue(bot, "dead", 1, randomTime);
            SetEventValue(bot, "revive", 1, randomTime - 60);
            return false;
        }

        if (!GetEventValue(bot, "revive"))
        {
            Revive(player);
            return true;
        }

        return false;
    }

    if (player->GetGuildId())
    {
        Guild* guild = sGuildMgr.GetGuildById(player->GetGuildId());
        if (guild->GetLeaderGuid().GetRawValue() == player->GetObjectGuid().GetRawValue()) {
            for (auto & i : players)
            {
                sGuildTaskMgr.Update(i, player);
            }
        }
    }

    if (!GetEventValue(bot, "randomize"))
    {
        DEBUG_LOG("Randomizing bot %d", bot);
        Randomize(player);
        ScheduleRandomize(bot, urand(sPlayerbotAIConfig.minRandomBotRandomizeTime, sPlayerbotAIConfig.maxRandomBotRandomizeTime));
        return true;
    }

    if (!GetEventValue(bot, "logout"))
    {
        DEBUG_LOG("Logging out bot %d", bot);
        LogoutPlayerBot(bot);
        SetEventValue(bot, "logout", 1, sPlayerbotAIConfig.maxRandomBotInWorldTime);
        return true;
    }

    if (!GetEventValue(bot, "teleport"))
    {
        DEBUG_LOG("Random teleporting bot %d", bot);
        RandomTeleportForLevel(player);
        SetEventValue(bot, "teleport", 1, sPlayerbotAIConfig.maxRandomBotInWorldTime);
        return true;
    }

    return false;
}

void RandomPlayerbotMgr::Revive(Player* player)
{
    uint32 bot = player->GetGUIDLow();
    DETAIL_LOG("Reviving dead bot %d", bot);
    SetEventValue(bot, "dead", 0, 0);
    SetEventValue(bot, "revive", 0, 0);
    RandomTeleportForLevel(player);
}


void RandomPlayerbotMgr::RandomTeleport(Player* bot, vector<WorldLocation> &locs)
{
    if (bot->IsBeingTeleported())
    {
        return;
    }

    if (locs.empty())
    {
        sLog.outError("Cannot teleport bot %s - no locations available", bot->GetName());
        return;
    }

    for (auto attempts = 0; attempts < 10; ++attempts)
    {
        WorldLocation loc = locs[urand(0, locs.size() - 1)];
        float x = loc.coord_x + frand(0, sPlayerbotAIConfig.grindDistance) - sPlayerbotAIConfig.grindDistance / 2;
        float y = loc.coord_y + frand(0, sPlayerbotAIConfig.grindDistance) - sPlayerbotAIConfig.grindDistance / 2;
        float z = loc.coord_z;

        Map * map = sMapMgr.FindMap(loc.mapid);
        if (!map)
        {
            continue;
        }

        const TerrainInfo * terrain = map->GetTerrain();
        if (!terrain->IsOutdoors(x, y, z) ||
            +terrain->IsUnderWater(x, y, z) ||
            +terrain->IsInWater(x, y, z))
            continue;

        DETAIL_LOG("Random teleporting bot %s to %s %f,%f,%f", bot->GetName(), map->GetMapName(), x, y, z);
        z = 0.05f + map->GetTerrain()->GetHeightStatic(x, y, 0.05f + z, true, MAX_HEIGHT);
        bot->TeleportTo(loc.mapid, x, y, z, 0);
        return;
    }

    sLog.outError("Cannot teleport bot %s - no locations available", bot->GetName());
}

void RandomPlayerbotMgr::RandomTeleportForLevel(Player* bot)
{
    if (locsPerLevelCache[bot->getLevel()].empty()) {
        QueryResult *results = WorldDatabase.PQuery("select map, position_x, position_y, position_z "
                                                    "from (select map, position_x, position_y, position_z, avg(t.maxlevel), avg(t.minlevel), "
                                                    "(avg(t.maxlevel) + avg(t.minlevel)) / 2 - %u delta "
                                                    "from creature c inner join creature_template t on c.id = t.entry group by t.entry) q "
                                                    "where delta >= 0 and delta <= 1 and map in (%s)",
                                                    bot->getLevel(), sPlayerbotAIConfig.randomBotMapsAsString.c_str());
        if (results) {
            do {
                Field *fields = results->Fetch();
                uint32 mapId = fields[0].GetUInt32();
                float x = fields[1].GetFloat();
                float y = fields[2].GetFloat();
                float z = fields[3].GetFloat();
                WorldLocation loc(mapId, x, y, z, 0);
                locsPerLevelCache[bot->getLevel()].push_back(loc);
            } while (results->NextRow());
            delete results;
        }
    }

    RandomTeleport(bot, locsPerLevelCache[bot->getLevel()]);
}

/*void RandomPlayerbotMgr::RandomTeleport(Player* bot, uint32 mapId, float teleX, float teleY, float teleZ)
{
    Refresh(bot);

    vector<WorldLocation> locs;
    QueryResult * results = WorldDatabase.PQuery("select position_x, position_y, position_z from creature where map = '%u' and abs(position_x - '%f') < '%u' and abs(position_y - '%f') < '%u'",
        +mapId, teleX, sPlayerbotAIConfig.randomBotTeleportDistance / 2, teleY, sPlayerbotAIConfig.randomBotTeleportDistance / 2);
    if (results)
    {
        do
             {
            Field * fields = results->Fetch();
            float x = fields[0].GetFloat();
            float y = fields[1].GetFloat();
            float z = fields[2].GetFloat();
            WorldLocation loc(mapId, x, y, z, 0);
            locs.push_back(loc);
            } while (results->NextRow());
            delete results;
    }

    RandomTeleport(bot, locs);
    Refresh(bot);
}*/

void RandomPlayerbotMgr::Randomize(Player* bot)
{
    if (bot->getLevel() == 1)
    {
        RandomizeFirst(bot);
    }
    else
    {
        IncreaseLevel(bot);
    }
}

void RandomPlayerbotMgr::IncreaseLevel(Player* bot)
{
    DETAIL_LOG("Increasing level for bot %s", bot->GetName());
    uint32 maxLevel = sPlayerbotAIConfig.randomBotMaxLevel;
    if (maxLevel > sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL))
    {
        maxLevel = sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL);
    }

    uint32 level = min((uint32)(bot->getLevel() + 1), maxLevel);
    PlayerbotFactory factory(bot, level);
    if (bot->GetGuildId())
    {
        factory.Refresh();
    }
    else
    {
        factory.Randomize();
    }
    RandomTeleportForLevel(bot);
}

void RandomPlayerbotMgr::RandomizeFirst(Player* bot)
{
    uint32 maxLevel = sPlayerbotAIConfig.randomBotMaxLevel;
    if (maxLevel > sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL))
    {
        maxLevel = sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL);
    }

    for (int attempt = 0; attempt < 100; ++attempt)
    {
        uint32 index = urand(0, sPlayerbotAIConfig.randomBotMaps.size() - 1);
        uint32 mapId = sPlayerbotAIConfig.randomBotMaps[index];

        vector<GameTele const*> locs;
        GameTeleMap const & teleMap = sObjectMgr.GetGameTeleMap();
        for (const auto & itr : teleMap)
        {
            GameTele const* tele = &itr.second;
            if (tele->mapId == mapId)
            {
                locs.push_back(tele);
            }
        }

        index = urand(0, locs.size() - 1);
        if (index >= locs.size())
        {
            return;
        }
        GameTele const* tele = locs[index];
        uint32 level = GetZoneLevel(tele->mapId, tele->position_x, tele->position_y);
        if (level > maxLevel + 5)
        {
            continue;
        }

        level = min(level, maxLevel);
        if (!level) level = 1;

        if (frand(0, 100) < 100 * sPlayerbotAIConfig.randomBotMaxLevelChance)
        {
            level = maxLevel;
        }

        if (level < sPlayerbotAIConfig.randomBotMinLevel)
        {
            continue;
        }

        PlayerbotFactory factory(bot, level);
        factory.CleanRandomize();
        RandomTeleportForLevel(bot);
        break;
    }
}

uint32 RandomPlayerbotMgr::GetZoneLevel(uint16 mapId, float teleX, float teleY)
{
    QueryResult* results = WorldDatabase.PQuery("select avg(t.minlevel) minlevel, avg(t.maxlevel) maxlevel from creature c "
        "inner join creature_template t on c.id = t.entry "
        "where map = '%u' and minlevel > 1 and abs(position_x - '%f') < '%u' and abs(position_y - '%f') < '%u'",
        mapId, teleX, sPlayerbotAIConfig.randomBotTeleportDistance / 2, teleY, sPlayerbotAIConfig.randomBotTeleportDistance / 2);

    if (results)
    {
        Field* fields = results->Fetch();
        delete results;

        uint8 minLevel = fields[0].GetUInt8();
        uint8 maxLevel = min(static_cast<uint32>(fields[1].GetUInt8()), sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL));

        return urand(minLevel, maxLevel);

    }
    else
    {
        return urand(1, sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL));
    }
}

/*void RandomPlayerbotMgr::Refresh(Player* bot)
{
    DETAIL_LOG("Refreshing bot %s", bot->GetName());
    if (bot->IsDead())
    {
        bot->ResurrectPlayer(1.0f);
        bot->SpawnCorpseBones();
        bot->GetPlayerbotAI()->ResetStrategies();
    }

    bot->GetPlayerbotAI()->Reset();

    HostileReference *ref = bot->GetHostileRefManager().getFirst();
    while (ref)
    {
        ThreatManager *threatManager = ref->getSource();
        Unit *unit = threatManager->getOwner();

        unit->RemoveAllAttackers();
        unit->ClearInCombat();

        ref = ref->next();
    }

    bot->RemoveAllAttackers();
    bot->ClearInCombat();

    bot->DurabilityRepairAll(false, 1.0f, false);
    bot->SetHealthPercent(100);
    bot->SetPvP(true);

    if (bot->GetMaxPower(POWER_MANA) > 0)
    {
        bot->SetPower(POWER_MANA, bot->GetMaxPower(POWER_MANA));
    }

    if (bot->GetMaxPower(POWER_ENERGY) > 0)
    {
        bot->SetPower(POWER_ENERGY, bot->GetMaxPower(POWER_ENERGY));
    }
}*/


bool RandomPlayerbotMgr::IsRandomBot(Player* bot)
{
    return IsRandomBot(bot->GetObjectGuid());
}

bool RandomPlayerbotMgr::IsRandomBot(uint32 bot)
{
    return GetEventValue(bot, "add");
}

void RandomPlayerbotMgr::GetBots(set<uint32> &bots)
{
    QueryResult* results = CharacterDatabase.Query(
        "select bot from ai_playerbot_random_bots where owner = 0 and event = 'add'");

    if (results)
    {
        do
        {
            Field* fields = results->Fetch();
            uint32 bot = fields[0].GetUInt32();
            bots.insert(bot);
        } while (results->NextRow());
        delete results;
    }
}

uint32 RandomPlayerbotMgr::GetEventValue(uint32 bot, const string& event)
{
    uint32 value = 0;

    QueryResult* results = CharacterDatabase.PQuery(
        "SELECT `value`, `time`, `validIn` FROM `ai_playerbot_random_bots` WHERE `owner` = '0' AND `bot` = '%u' AND `event` = '%s'",
        bot, event.c_str());

    if (results)
    {
        Field* fields = results->Fetch();
        value = fields[0].GetUInt32();
        uint32 lastChangeTime = fields[1].GetUInt32();
        uint32 validIn = fields[2].GetUInt32();
        if ((time(nullptr) - lastChangeTime) >= validIn)
        {
            value = 0;
        }
        delete results;
    }

    return value;
}

uint32 RandomPlayerbotMgr::SetEventValue(uint32 bot, const string& event, uint32 value, uint32 validIn)
{
    CharacterDatabase.PExecute("DELETE FROM `ai_playerbot_random_bots` WHERE `owner` = '0' AND `bot` = '%u' AND `event` = '%s'",
        bot, event.c_str());
    if (value)
    {
        CharacterDatabase.PExecute(
            "INSERT INTO `ai_playerbot_random_bots` (`owner`, `bot`, `time`, `validIn`, `event`, `value`) VALUES ('%u', '%u', '%ld', '%u', '%s', '%u')",
            0, bot, time(nullptr), validIn, event.c_str(), value);
    }

    return value;
}

bool RandomPlayerbotMgr::HandlePlayerbotConsoleCommand(char const *args)
{
    DEBUG_LOG("Handling rnd bot command: %s", args);
    if (!sPlayerbotAIConfig.enabled)
    {
        sLog.outError("Playerbot system is currently disabled!");
        return false;
    }

    if (!args || !*args)
    {
        sLog.outError("Usage: rndbot stats/update/reset/init/refresh/add/remove");
        return false;
    }

    string cmd = args;

    if (cmd == "reset")
    {
        CharacterDatabase.PExecute("delete from ai_playerbot_random_bots");
        sLog.outString("Random bots were reset for all players. Please restart the Server.");
        return true;
    }
    else if (cmd == "stats")
    {
        sRandomPlayerbotMgr.PrintStats();
        return true;
    }
    else if (cmd == "update")
    {
        sRandomPlayerbotMgr.UpdateAIInternal(0);
        return true;
    }
    else if (cmd == "init" || cmd == "refresh" || cmd == "teleport" || cmd == "revive")
    {
        sLog.outString("Randomizing bots for %zu accounts", sPlayerbotAIConfig.randomBotAccounts.size());
        list<uint32> botIds;
        for (auto account: sPlayerbotAIConfig.randomBotAccounts)
        {
            if (QueryResult* results = CharacterDatabase.PQuery("SELECT guid FROM characters where account = '%u'", account))
            {
                do
                {
                    Field* fields = results->Fetch();

                    uint32 botId = fields[0].GetUInt32();
                    ObjectGuid guid = ObjectGuid(HIGHGUID_PLAYER, botId);
                    Player* bot = sObjectMgr.GetPlayer(guid);
                    if (!bot)
                    {
                        continue;
                    }

                    botIds.push_back(botId);
                } while (results->NextRow());
                delete results;
            }
        }

        int processed = 0;
        for (auto botId : botIds)
        {
            ObjectGuid guid = ObjectGuid(HIGHGUID_PLAYER, botId);
            Player* bot = sObjectMgr.GetPlayer(guid);
            if (!bot)
            {
                continue;
            }

            sLog.outString("[%u/%zu] Processing command '%s' for bot '%s'",
                processed++, botIds.size(), cmd.c_str(), bot->GetName());

            if (cmd == "init")
            {
                sRandomPlayerbotMgr.RandomizeFirst(bot);
            }
            else if (cmd == "teleport")
            {
                sRandomPlayerbotMgr.RandomTeleportForLevel(bot);
            }
            else if (cmd == "revive")
            {
                sRandomPlayerbotMgr.Revive(bot);
            }
            else
            {
                bot->SetLevel(bot->getLevel() - 1);
                sRandomPlayerbotMgr.IncreaseLevel(bot);
            }
            uint32 randomTime = urand(sPlayerbotAIConfig.minRandomBotRandomizeTime, sPlayerbotAIConfig.maxRandomBotRandomizeTime);
            CharacterDatabase.PExecute("UPDATE ai_playerbot_random_bots SET validIn = '%u' WHERE event = 'randomize' AND bot = '%u'",
                randomTime, bot->GetGUIDLow());
            CharacterDatabase.PExecute("UPDATE ai_playerbot_random_bots SET validIn = '%u' WHERE event = 'logout' AND bot = '%u'",
                sPlayerbotAIConfig.maxRandomBotInWorldTime, bot->GetGUIDLow());
        }
        return true;
    }
    else
    {
        list<string> messages = sRandomPlayerbotMgr.HandlePlayerbotCommand(args, nullptr);
        for (auto & message : messages)
        {
            sLog.outError("%s", message.c_str());
        }
        return true;
    }
}

void RandomPlayerbotMgr::HandleCommand(uint32 type, const string& text, Player& fromPlayer)
{
    for (auto it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
    {
        Player* const bot = it->second;
        bot->GetPlayerbotAI()->HandleCommand(type, text, fromPlayer);
    }
}

void RandomPlayerbotMgr::OnPlayerLogout(Player* player)
{
    for (auto it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
    {
        Player* const bot = it->second;
        PlayerbotAI* ai = bot->GetPlayerbotAI();
        if (player == ai->GetMaster())
        {
            ai->SetMaster(nullptr);
            ai->ResetStrategies();
        }
    }

    auto i = find(players.begin(), players.end(), player);
    if (i != players.end())
    {
        players.erase(i);
    }
}

void RandomPlayerbotMgr::OnPlayerLogin(Player* player)
{
    for (auto it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
    {
        Player* const bot = it->second;
        if (player == bot || player->GetPlayerbotAI())
        {
            continue;
        }

        Group* group = bot->GetGroup();
        if (!group)
        {
            continue;
        }

        for (GroupReference *gref = group->GetFirstMember(); gref; gref = gref->next())
        {
            Player* member = gref->getSource();
            PlayerbotAI* ai = bot->GetPlayerbotAI();
            if (member == player && (!ai->GetMaster() || ai->GetMaster()->GetPlayerbotAI()))
            {
                ai->SetMaster(player);
                ai->ResetStrategies();
                ai->TellMaster("Hello");
                break;
            }
        }
    }

    if (!IsRandomBot(player))
    {
        players.push_back(player);
        DEBUG_LOG("Including non-random bot player %s into random bot update", player->GetName());
    }
}

Player* RandomPlayerbotMgr::GetRandomPlayer()
{
    if (players.empty())
    {
        return nullptr;
    }

    uint32 index = urand(0, players.size() - 1);
    return players[index];
}

void RandomPlayerbotMgr::PrintStats()
{
    sLog.outString("%zu Random Bots online", playerBots.size());

    map<uint32, uint32> alliance, horde;
    for (uint32 i = 0; i < 10; ++i)
    {
        alliance[i] = 0;
        horde[i] = 0;
    }

    map<uint8, uint32> perRace, perClass;
    for (uint8 race = RACE_HUMAN; race < MAX_RACES; ++race)
    {
        perRace[race] = 0;
    }
    for (uint8 cls = CLASS_WARRIOR; cls < MAX_CLASSES; ++cls)
    {
        perClass[cls] = 0;
    }

    uint32 dps = 0, heal = 0, tank = 0, active = 0;
    for (auto & playerBot : playerBots)
    {
        Player* bot = playerBot.second;
        if (IsAlliance(bot->getRace()))
        {
            alliance[bot->getLevel() / 10]++;
        }
        else
        {
            horde[bot->getLevel() / 10]++;
        }

        perRace[bot->getRace()]++;
        perClass[bot->getClass()]++;

        if (bot->GetPlayerbotAI()->IsActive())
        {
            active++;
        }

        int spec = AiFactory::GetPlayerSpecTab(bot);
        switch (bot->getClass())
        {
        case CLASS_DRUID:
            if (spec == 2)
            {
                heal++;
            }
            else
            {
                dps++;
            }
            break;
        case CLASS_PALADIN:
            if (spec == 1)
            {
                tank++;
            }
            else if (spec == 0)
            {
                heal++;
            }
            else
            {
                dps++;
            }
            break;
        case CLASS_PRIEST:
            if (spec != 2)
            {
                heal++;
            }
            else
            {
                dps++;
            }
            break;
        case CLASS_SHAMAN:
            if (spec == 2)
            {
                heal++;
            }
            else
            {
                dps++;
            }
            break;
        case CLASS_WARRIOR:
            if (spec == 2)
            {
                tank++;
            }
            else
            {
                dps++;
            }
            break;
        default:
            dps++;
            break;
        }
    }

    sLog.outString("Per level:");
    uint32 maxLevel = sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL);
    for (uint32 i = 0; i < 10; ++i)
    {
        if (!alliance[i] && !horde[i])
        {
            continue;
        }

        uint32 from = i * 10;
        uint32 to = min(from + 9, maxLevel);
        if (!from) from = 1;
        {
            sLog.outString("    %d..%d: %d alliance, %d horde", from, to, alliance[i], horde[i]);
        }
    }
    sLog.outString("Per race:");
    for (uint8 race = RACE_HUMAN; race < MAX_RACES; ++race)
    {
        if (perRace[race])
        {
            sLog.outString("    %s: %d", ChatHelper::formatRace(race).c_str(), perRace[race]);
        }
    }
    sLog.outString("Per class:");
    for (uint8 cls = CLASS_WARRIOR; cls < MAX_CLASSES; ++cls)
    {
        if (perClass[cls])
        {
            sLog.outString("    %s: %d", ChatHelper::formatClass(cls).c_str(), perClass[cls]);
        }
    }
    sLog.outString("Per role:");
    sLog.outString("    tank: %d", tank);
    sLog.outString("    heal: %d", heal);
    sLog.outString("    dps: %d", dps);

    sLog.outString("Active bots: %d", active);
}

double RandomPlayerbotMgr::GetBuyMultiplier(Player* bot)
{
    uint32 id = bot->GetGUIDLow();
    uint32 value = GetEventValue(id, "buymultiplier");
    if (!value)
    {
        value = urand(1, 120);
        uint32 validIn = urand(sPlayerbotAIConfig.minRandomBotsPriceChangeInterval, sPlayerbotAIConfig.maxRandomBotsPriceChangeInterval);
        SetEventValue(id, "buymultiplier", value, validIn);
    }

    return static_cast<double>(value) / 100.0;
}

double RandomPlayerbotMgr::GetSellMultiplier(Player* bot)
{
    uint32 id = bot->GetGUIDLow();
    uint32 value = GetEventValue(id, "sellmultiplier");
    if (!value)
    {
        value = urand(80, 250);
        uint32 validIn = urand(sPlayerbotAIConfig.minRandomBotsPriceChangeInterval, sPlayerbotAIConfig.maxRandomBotsPriceChangeInterval);
        SetEventValue(id, "sellmultiplier", value, validIn);
    }

    return static_cast<double>(value) / 100.0;
}

uint32 RandomPlayerbotMgr::GetLootAmount(Player* bot)
{
    uint32 id = bot->GetGUIDLow();
    return GetEventValue(id, "lootamount");
}

void RandomPlayerbotMgr::SetLootAmount(Player* bot, uint32 value)
{
    uint32 id = bot->GetGUIDLow();
    SetEventValue(id, "lootamount", value, 24 * 3600);
}

uint32 RandomPlayerbotMgr::GetTradeDiscount(Player* bot)
{
    Group* group = bot->GetGroup();
    return GetLootAmount(bot) / (group ? group->GetMembersCount() : 10);
}

string RandomPlayerbotMgr::HandleRemoteCommand(string request)
{
    string::iterator pos = find(request.begin(), request.end(), ',');
    if (pos == request.end())
    {
        return "invalid request: " + request;
    }

    string command = string(request.begin(), pos);
    uint64 guid = atoi(string(pos + 1, request.end()).c_str());
    Player* bot = GetPlayerBot(guid);
    if (!bot)
    {
        return "invalid guid";
    }

    PlayerbotAI *ai = bot->GetPlayerbotAI();
    if (!ai)
    {
        return "invalid guid";
    }

    return ai->HandleRemoteCommand(command);
}

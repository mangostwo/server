#include "Config/Config.h"
#include "../botpch.h"
#include "playerbot.h"
#include "PlayerbotAIConfig.h"
#include "AccountMgr.h"
#include "ObjectMgr.h"
#include "DatabaseEnv.h"
#include "Player.h"
#include "RandomPlayerbotFactory.h"
#include "SystemConfig.h"

#include <openssl/rand.h>

map<const uint8, const vector<uint8> > availableRaces = {
        {CLASS_WARRIOR,{
                               RACE_HUMAN,
                               RACE_ORC,
                               RACE_DWARF,
                               RACE_NIGHTELF,
                               RACE_UNDEAD,
                               RACE_TAUREN,
                               RACE_GNOME,
                               RACE_TROLL,
                               RACE_DRAENEI}},

        {CLASS_PALADIN, {
                               RACE_HUMAN,
                               RACE_DWARF,
                               RACE_BLOODELF,
                               RACE_DRAENEI}},

        {CLASS_HUNTER, {
                               RACE_ORC,
                               RACE_DWARF,
                               RACE_NIGHTELF,
                               RACE_TAUREN,
                               RACE_TROLL,
                               RACE_BLOODELF,
                               RACE_DRAENEI}},

        {CLASS_ROGUE, {
                               RACE_HUMAN,
                               RACE_ORC,
                               RACE_DWARF,
                               RACE_NIGHTELF,
                               RACE_UNDEAD,
                               RACE_GNOME,
                               RACE_TROLL,
                               RACE_BLOODELF}},

        {CLASS_PRIEST, {
                               RACE_HUMAN,
                               RACE_DWARF,
                               RACE_NIGHTELF,
                               RACE_UNDEAD,
                               RACE_TROLL,
                               RACE_BLOODELF,
                               RACE_DRAENEI}},

        {CLASS_DEATH_KNIGHT, {
                               RACE_HUMAN,
                               RACE_ORC,
                               RACE_DWARF,
                               RACE_NIGHTELF,
                               RACE_UNDEAD,
                               RACE_TAUREN,
                               RACE_GNOME,
                               RACE_TROLL,
                               RACE_BLOODELF,
                               RACE_DRAENEI}},

        {CLASS_SHAMAN, {
                               RACE_ORC,
                               RACE_TAUREN,
                               RACE_TROLL,
                               RACE_DRAENEI}},

        {CLASS_MAGE, {
                               RACE_HUMAN,
                               RACE_UNDEAD,
                               RACE_GNOME,
                               RACE_TROLL,
                               RACE_BLOODELF,
                               RACE_DRAENEI}},

        {CLASS_WARLOCK, {
                               RACE_HUMAN,
                               RACE_ORC,
                               RACE_UNDEAD,
                               RACE_GNOME,
                               RACE_BLOODELF}},

        {CLASS_DRUID, {
                               RACE_NIGHTELF,
                               RACE_TAUREN}}
};

RandomPlayerbotFactory::RandomPlayerbotFactory(uint32 accountId) : accountId(accountId)
{
}

bool RandomPlayerbotFactory::CreateRandomBot(uint8 cls) const
{
    DETAIL_LOG("Creating new random bot for class %d", cls);

    uint8 gender = urand(GENDER_MALE, GENDER_FEMALE) ;

    uint8 race = availableRaces[cls][urand(0, availableRaces[cls].size() - 1)];
    string name;

    if (!CreateRandomBotName(name))
    {
        return false;
    }

    uint8 skin = urand(0, 7);
    uint8 face = urand(0, 7);
    uint8 hairStyle = urand(0, 7);
    uint8 hairColor = urand(0, 7);
    uint8 facialHair = urand(0, 7);
    uint8 outfitId = 0;

    auto* session = new WorldSession(accountId, nullptr, SEC_PLAYER, MAX_EXPANSION, 0, LOCALE_enUS);

    auto *player = new Player(session);
    if (!player->Create(sObjectMgr.GeneratePlayerLowGuid(), name, race, cls, gender, skin, face, hairStyle, hairColor, facialHair, outfitId))
    {
        Player::DeleteFromDB(player->GetObjectGuid(), accountId, true, true);
        delete session;
        delete player;
        sLog.outError("Unable to create random bot for account %d - name: \"%s\"; race: %u; class: %u; gender: %u; skin: %u; face: %u; hairStyle: %u; hairColor: %u; facialHair: %u; outfitId: %u",
            accountId, name.c_str(), race, cls, gender, skin, face, hairStyle, hairColor, facialHair, outfitId);
        return false;
    }

    player->setCinematic(2);
    player->SetAtLoginFlag(AT_LOGIN_NONE);
    player->SaveToDB();

    DETAIL_LOG("Random bot created for account %d - name: \"%s\"; race: %u; class: %u; gender: %u; skin: %u; face: %u; hairStyle: %u; hairColor: %u; facialHair: %u; outfitId: %u",
        accountId, name.c_str(), race, cls, gender, skin, face, hairStyle, hairColor, facialHair, outfitId);

    return true;
}


bool RandomPlayerbotFactory::CreateRandomBotName(string& name)
{
    QueryResult *result = CharacterDatabase.Query("SELECT MAX(`name_id`) FROM `ai_playerbot_names`");
    if (!result)
    {
        sLog.outError("table `ai_playerbot_names` is empty. Check database");
        return false;
    }

    Field *fields = result->Fetch();
    uint32 maxId = fields[0].GetUInt32();
    delete result;

    uint32 id = urand(0, maxId);
    result = CharacterDatabase.PQuery("SELECT `n`.`name` FROM `ai_playerbot_names` n "
            "LEFT OUTER JOIN `characters` e ON `e`.`name` = `n`.`name` "
            "WHERE `e`.`guid` IS NULL AND `n`.`name_id` >= '%u' LIMIT 1", id);
    if (!result)
    {
        sLog.outError("No more names left for random bots");
        return false;
    }

    Field *nfields = result->Fetch();
    name = nfields[0].GetCppString();
    DEBUG_LOG("Created name %s for random bot.", name.c_str());
    delete result;

    return true;
}


void RandomPlayerbotFactory::CreateRandomBots()
{
    for (uint32 accountNumber = 0; accountNumber < sPlayerbotAIConfig.randomBotAccountCount; ++accountNumber)
    {
        string accountName = sPlayerbotAIConfig.randomBotAccountPrefix + to_string(accountNumber);
        QueryResult* results = LoginDatabase.PQuery("SELECT id FROM account where username = '%s'", accountName.c_str());
        if (results)
        {
            delete results;
            continue;
        }

        uint8 password[MAX_PASSWORD_STR];
        switch (RAND_bytes(password, MAX_PASSWORD_STR))
        {
            case -1:
                sLog.outError("Can't create password for random bot account. Not supported by this system.");
                break;
            case 0:
                sLog.outError("Password creation for random bot failed.");
                break;
            case 1:
                sAccountMgr.CreateAccount(accountName, reinterpret_cast<char* const>(password), MAX_EXPANSION);
                DEBUG_LOG( "Account %s created for random bots", accountName.c_str());
                break;
        }
    }

    LoginDatabase.PExecute("UPDATE account SET expansion = '%u' WHERE expansion < '%u' AND username LIKE '%s%%'", MAX_EXPANSION, MAX_EXPANSION, sPlayerbotAIConfig.randomBotAccountPrefix.c_str());

    uint32 totalRandomBotChars = 0;
    for (uint32 accountNumber = 0; accountNumber < sPlayerbotAIConfig.randomBotAccountCount; ++accountNumber)
    {
        string accountName = sPlayerbotAIConfig.randomBotAccountPrefix + to_string(accountNumber);

        QueryResult* results = LoginDatabase.PQuery("SELECT id FROM account where username = '%s'", accountName.c_str());
        if (!results)
        {
            continue;
        }

        Field* fields = results->Fetch();
        uint32 accountId = fields[0].GetUInt32();
        delete results;

        sPlayerbotAIConfig.randomBotAccounts.push_back(accountId);

        int count = sAccountMgr.GetCharactersCount(accountId);
        if (count >= 10)
        {
            totalRandomBotChars += count;
            continue;
        }

        RandomPlayerbotFactory factory(accountId);
        for (uint8 cls = CLASS_WARRIOR; cls < MAX_CLASSES; ++cls)
        {
            if (cls != 10)
            {
                factory.CreateRandomBot(cls);
            }
        }

        totalRandomBotChars += sAccountMgr.GetCharactersCount(accountId);
        DEBUG_LOG("Created %d bots for account %s.", sAccountMgr.GetCharactersCount(accountId) - count, accountName.c_str());
    }

    BASIC_LOG("%zu random bot accounts with %d characters available", sPlayerbotAIConfig.randomBotAccounts.size(), totalRandomBotChars);
}


void RandomPlayerbotFactory::CreateRandomGuilds()
{
    BASIC_LOG("Creating random bot guilds...");
    vector<uint32> randomBots;
    QueryResult* results = LoginDatabase.PQuery("SELECT id FROM account where username like '%s%%'", sPlayerbotAIConfig.randomBotAccountPrefix.c_str());
    if (results)
    {
        do
        {
            Field* fields = results->Fetch();
            uint32 accountId = fields[0].GetUInt32();

            QueryResult* results2 = CharacterDatabase.PQuery("SELECT guid FROM characters where account  = '%u'", accountId);
            if (results2)
            {
                do
                {
                    Field* fields2 = results2->Fetch();
                    uint32 guid = fields2[0].GetUInt32();
                    randomBots.push_back(guid);
                } while (results2->NextRow());
                delete results2;
            }

        } while (results->NextRow());
        delete results;
    }

    uint32 guildNumber = 0;
    vector<ObjectGuid> availableLeaders;
    for (uint32& i : randomBots)
    {
        ObjectGuid leader(HIGHGUID_PLAYER, i);
        Guild* guild = sGuildMgr.GetGuildByLeader(leader);
        if (guild)
        {
            ++guildNumber;
            sPlayerbotAIConfig.randomBotGuilds.push_back(guild->GetId());
        }
        else
        {
            Player* player = sObjectMgr.GetPlayer(leader);
            if (player)
            {
                availableLeaders.push_back(leader);
            }
        }
    }

    for (; guildNumber < sPlayerbotAIConfig.randomBotGuildCount; ++guildNumber)
    {
        string guildName;
        if (!CreateRandomGuildName(guildName))
        {
            break;
        }

        if (availableLeaders.empty())
        {
            sLog.outError("No leaders for random guilds available");
            break;
        }

        uint32 index = urand(0, availableLeaders.size() - 1);
        ObjectGuid leader = availableLeaders[index];
        Player* player = sObjectMgr.GetPlayer(leader);
        if (!player)
        {
            sLog.outError("Cannot find player for guild leader %u", leader.GetEntry());
            break;
        }

        auto* guild = new Guild();
        if (!guild->Create(player, guildName))
        {
            sLog.outError("Error creating guild %s", guildName.c_str());
            delete guild;
            break;
        }

        sGuildMgr.AddGuild(guild);
        sPlayerbotAIConfig.randomBotGuilds.push_back(guild->GetId());
    }

    BASIC_LOG("%d random bot guilds available", guildNumber);
}

bool RandomPlayerbotFactory::CreateRandomGuildName(string& guildName)
{
    QueryResult* result = CharacterDatabase.Query("SELECT MAX(name_id) FROM ai_playerbot_guild_names");
    if (!result)
    {
        sLog.outError("No more names left for random guilds");
        return false;
    }

    Field *fields = result->Fetch();
    uint32 maxId = fields[0].GetUInt32();
    delete result;

    uint32 id = urand(0, maxId);
    result = CharacterDatabase.PQuery("SELECT n.name FROM ai_playerbot_guild_names n "
            "LEFT OUTER JOIN guild e ON e.name = n.name "
            "WHERE e.guildid IS NULL AND n.name_id >= '%u' LIMIT 1", id);
    if (!result)
    {
        sLog.outError("No more names left for random guilds");
        return false;
    }

    fields = result->Fetch();
    delete result;
    guildName = fields[0].GetString();
    return true;
}

void RandomPlayerbotFactory::DeleteRandomBots()
{
    BASIC_LOG("Deleting random bot accounts...");
    QueryResult* results = LoginDatabase.PQuery("SELECT id FROM account where username like '%s%%'", sPlayerbotAIConfig.randomBotAccountPrefix.c_str());
    if (results)
    {
        do
        {
            Field* fields = results->Fetch();
            sAccountMgr.DeleteAccount(fields[0].GetUInt32());
        } while (results->NextRow());
        delete results;
    }

    CharacterDatabase.Execute("DELETE FROM ai_playerbot_random_bots");
    BASIC_LOG("Random bot accounts deleted");
}

void RandomPlayerbotFactory::DeleteRandomGuilds()
{
    BASIC_LOG("Deleting random bot guilds...");

    vector<uint32> randomBots;
    QueryResult* results = LoginDatabase.PQuery("SELECT id FROM account where username like '%s%%'", sPlayerbotAIConfig.randomBotAccountPrefix.c_str());
    if (results)
    {
        do
        {
            Field* fields = results->Fetch();
            uint32 accountId = fields[0].GetUInt32();

            QueryResult* results2 = CharacterDatabase.PQuery("SELECT guid FROM characters where account  = '%u'", accountId);
            if (results2)
            {
                do
                {
                    Field* fields2 = results2->Fetch();
                    uint32 guid = fields2[0].GetUInt32();
                    randomBots.push_back(guid);
                } while (results2->NextRow());
                delete results2;
            }

        } while (results->NextRow());
        delete results;
    }

    for (uint32& randomBot : randomBots)
    {
        ObjectGuid leader(HIGHGUID_PLAYER, randomBot);
        Guild* guild = sGuildMgr.GetGuildByLeader(leader);
        if (guild) guild->Disband();
        delete guild;
    }
    BASIC_LOG("Random bot guilds deleted");
}
#include "../botpch.h"
#include "PlayerbotAIConfig.h"
#include "playerbot.h"
#include "RandomPlayerbotFactory.h"
#include "AccountMgr.h"
#include "SystemConfig.h"

using namespace std;

INSTANTIATE_SINGLETON_1(PlayerbotAIConfig);

PlayerbotAIConfig::PlayerbotAIConfig()
{
}

template <class T>
void LoadList(string value, T &list)
{
    vector<string> ids = split(value, ',');
    for (vector<string>::iterator i = ids.begin(); i != ids.end(); i++)
    {
        uint32 id = atoi((*i).c_str());
        if (!id)
        {
            continue;
        }

        list.push_back(id);
    }
}

bool PlayerbotAIConfig::Initialize()
{
    sLog.outString("Initializing AI Playerbot by ike3, based on the original Playerbot by blueboy");

    const char* cfg_file = SYSCONFDIR"aiplayerbot.conf";
    if (!sConfig.SetSource(cfg_file))
    {
        sLog.outError("AI Playerbot is Disabled.  Could not find configuration file %s.", cfg_file);
        Log::WaitBeforeContinueIfNeed();
        return false;
    }

    enabled = sConfig.GetBoolDefault("AiPlayerbot.Enabled", true);
    if (!enabled)
    {
        sLog.outString("AI Playerbot is Disabled in %s", cfg_file);
        return false;
    }

    globalCoolDown = (uint32) sConfig.GetIntDefault("AiPlayerbot.GlobalCooldown", 500);
    maxWaitForMove = sConfig.GetIntDefault("AiPlayerbot.MaxWaitForMove", 3000);
    reactDelay = (uint32) sConfig.GetIntDefault("AiPlayerbot.ReactDelay", 100);

    sightDistance = sConfig.GetFloatDefault("AiPlayerbot.SightDistance", 50.0f);
    spellDistance = sConfig.GetFloatDefault("AiPlayerbot.SpellDistance", 30.0f);
    reactDistance = sConfig.GetFloatDefault("AiPlayerbot.ReactDistance", 150.0f);
    grindDistance = sConfig.GetFloatDefault("AiPlayerbot.GrindDistance", 100.0f);
    lootDistance = sConfig.GetFloatDefault("AiPlayerbot.LootDistance", 20.0f);
    fleeDistance = sConfig.GetFloatDefault("AiPlayerbot.FleeDistance", 20.0f);
    tooCloseDistance = sConfig.GetFloatDefault("AiPlayerbot.TooCloseDistance", 7.0f);
    meleeDistance = sConfig.GetFloatDefault("AiPlayerbot.MeleeDistance", 1.5f);
    followDistance = sConfig.GetFloatDefault("AiPlayerbot.FollowDistance", 1.5f);
    whisperDistance = sConfig.GetFloatDefault("AiPlayerbot.WhisperDistance", 6000.0f);
    contactDistance = sConfig.GetFloatDefault("AiPlayerbot.ContactDistance", 0.5f);

    criticalHealth = sConfig.GetIntDefault("AiPlayerbot.CriticalHealth", 20);
    lowHealth = sConfig.GetIntDefault("AiPlayerbot.LowHealth", 50);
    mediumHealth = sConfig.GetIntDefault("AiPlayerbot.MediumHealth", 70);
    almostFullHealth = sConfig.GetIntDefault("AiPlayerbot.AlmostFullHealth", 85);
    lowMana = sConfig.GetIntDefault("AiPlayerbot.LowMana", 15);
    mediumMana = sConfig.GetIntDefault("AiPlayerbot.MediumMana", 40);

    randomGearLoweringChance = sConfig.GetFloatDefault("AiPlayerbot.RandomGearLoweringChance", 0.15f);
    randomBotMaxLevelChance = sConfig.GetFloatDefault("AiPlayerbot.RandomBotMaxLevelChance", 0.4f);

    iterationsPerTick = sConfig.GetIntDefault("AiPlayerbot.IterationsPerTick", 4);

    allowGuildBots = sConfig.GetBoolDefault("AiPlayerbot.AllowGuildBots", true);

    randomBotMapsAsString = sConfig.GetStringDefault("AiPlayerbot.RandomBotMaps", "0,1,530,571");
    LoadList<vector<uint32> >(randomBotMapsAsString, randomBotMaps);
    LoadList<list<uint32> >(sConfig.GetStringDefault("AiPlayerbot.RandomBotQuestItems", "6948,5175,5176,5177,5178"), randomBotQuestItems);
    LoadList<list<uint32> >(sConfig.GetStringDefault("AiPlayerbot.RandomBotSpellIds", "54197"), randomBotSpellIds);

    randomBotAutologin = sConfig.GetBoolDefault("AiPlayerbot.RandomBotAutologin", true);
    minRandomBots = sConfig.GetIntDefault("AiPlayerbot.MinRandomBots", 50);
    maxRandomBots = sConfig.GetIntDefault("AiPlayerbot.MaxRandomBots", 200);
    randomBotUpdateInterval = sConfig.GetIntDefault("AiPlayerbot.RandomBotUpdateInterval", 60);
    randomBotCountChangeMinInterval = sConfig.GetIntDefault("AiPlayerbot.RandomBotCountChangeMinInterval", 24 * 3600);
    randomBotCountChangeMaxInterval = sConfig.GetIntDefault("AiPlayerbot.RandomBotCountChangeMaxInterval", 3 * 24 * 3600);
    minRandomBotInWorldTime = sConfig.GetIntDefault("AiPlayerbot.MinRandomBotInWorldTime", 2 * 3600);
    maxRandomBotInWorldTime = sConfig.GetIntDefault("AiPlayerbot.MaxRandomBotInWorldTime", 14 * 24 * 3600);
    minRandomBotRandomizeTime = sConfig.GetIntDefault("AiPlayerbot.MinRandomBotRandomizeTime", 2 * 3600);
    maxRandomBotRandomizeTime = sConfig.GetIntDefault("AiPlayerbot.MaxRandomRandomizeTime", 14 * 24 * 3600);
    minRandomBotReviveTime = sConfig.GetIntDefault("AiPlayerbot.MinRandomBotReviveTime", 60);
    maxRandomBotReviveTime = sConfig.GetIntDefault("AiPlayerbot.MaxRandomReviveTime", 300);
    randomBotTeleportDistance = sConfig.GetIntDefault("AiPlayerbot.RandomBotTeleportDistance", 1000);
    minRandomBotsPerInterval = sConfig.GetIntDefault("AiPlayerbot.MinRandomBotsPerInterval", 50);
    maxRandomBotsPerInterval = sConfig.GetIntDefault("AiPlayerbot.MaxRandomBotsPerInterval", 100);
    minRandomBotsPriceChangeInterval = sConfig.GetIntDefault("AiPlayerbot.MinRandomBotsPriceChangeInterval", 2 * 3600);
    maxRandomBotsPriceChangeInterval = sConfig.GetIntDefault("AiPlayerbot.MaxRandomBotsPriceChangeInterval", 48 * 3600);
    randomBotJoinLfg = sConfig.GetBoolDefault("AiPlayerbot.RandomBotJoinLfg", true);
    logInGroupOnly = sConfig.GetBoolDefault("AiPlayerbot.LogInGroupOnly", true);
    logValuesPerTick = sConfig.GetBoolDefault("AiPlayerbot.LogValuesPerTick", false);
    fleeingEnabled = sConfig.GetBoolDefault("AiPlayerbot.FleeingEnabled", true);
    randomBotMinLevel = sConfig.GetIntDefault("AiPlayerbot.RandomBotMinLevel", 1);
    randomBotMaxLevel = sConfig.GetIntDefault("AiPlayerbot.RandomBotMaxLevel", 255);
    randomBotLoginAtStartup = sConfig.GetBoolDefault("AiPlayerbot.RandomBotLoginAtStartup", true);
    randomBotTeleLevel = sConfig.GetIntDefault("AiPlayerbot.RandomBotTeleLevel", 3);

    randomChangeMultiplier = sConfig.GetFloatDefault("AiPlayerbot.RandomChangeMultiplier", 1.0);

    randomBotCombatStrategies = sConfig.GetStringDefault("AiPlayerbot.RandomBotCombatStrategies", "+dps,+attack weak");
    randomBotNonCombatStrategies = sConfig.GetStringDefault("AiPlayerbot.RandomBotNonCombatStrategies", "+grind,+move random,+loot");

    commandPrefix = sConfig.GetStringDefault("AiPlayerbot.CommandPrefix", "");

    commandServerPort = sConfig.GetIntDefault("AiPlayerbot.CommandServerPort", 0);

    for (uint32 cls = 0; cls < MAX_CLASSES; ++cls)
    {
        for (uint32 spec = 0; spec < 3; ++spec)
        {
            ostringstream os; os << "AiPlayerbot.RandomClassSpecProbability." << cls << "." << spec;
            specProbability[cls][spec] = sConfig.GetIntDefault(os.str().c_str(), 33);
        }
    }

    CreateRandomBots();
    sLog.outString("AI Playerbot configuration loaded");

    return true;
}


bool PlayerbotAIConfig::IsInRandomAccountList(uint32 id)
{
    return find(randomBotAccounts.begin(), randomBotAccounts.end(), id) != randomBotAccounts.end();
}

bool PlayerbotAIConfig::IsInRandomQuestItemList(uint32 id)
{
    return find(randomBotQuestItems.begin(), randomBotQuestItems.end(), id) != randomBotQuestItems.end();
}

string PlayerbotAIConfig::GetValue(string name)
{
    ostringstream out;

    if (name == "GlobalCooldown")
    {
        out << globalCoolDown;
    }
    else if (name == "ReactDelay")
    {
        out << reactDelay;
    }

    else if (name == "SightDistance")
    {
        out << sightDistance;
    }
    else if (name == "SpellDistance")
    {
        out << spellDistance;
    }
    else if (name == "ReactDistance")
    {
        out << reactDistance;
    }
    else if (name == "GrindDistance")
    {
        out << grindDistance;
    }
    else if (name == "LootDistance")
    {
        out << lootDistance;
    }
    else if (name == "FleeDistance")
    {
        out << fleeDistance;
    }

    else if (name == "CriticalHealth")
    {
        out << criticalHealth;
    }
    else if (name == "LowHealth")
    {
        out << lowHealth;
    }
    else if (name == "MediumHealth")
    {
        out << mediumHealth;
    }
    else if (name == "AlmostFullHealth")
    {
        out << almostFullHealth;
    }
    else if (name == "LowMana")
    {
        out << lowMana;
    }

    else if (name == "IterationsPerTick")
    {
        out << iterationsPerTick;
    }

    return out.str();
}

void PlayerbotAIConfig::SetValue(string name, string value)
{
    istringstream out(value, istringstream::in);

    if (name == "GlobalCooldown")
    {
        out >> globalCoolDown;
    }
    else if (name == "ReactDelay")
    {
        out >> reactDelay;
    }

    else if (name == "SightDistance")
    {
        out >> sightDistance;
    }
    else if (name == "SpellDistance")
    {
        out >> spellDistance;
    }
    else if (name == "ReactDistance")
    {
        out >> reactDistance;
    }
    else if (name == "GrindDistance")
    {
        out >> grindDistance;
    }
    else if (name == "LootDistance")
    {
        out >> lootDistance;
    }
    else if (name == "FleeDistance")
    {
        out >> fleeDistance;
    }

    else if (name == "CriticalHealth")
    {
        out >> criticalHealth;
    }
    else if (name == "LowHealth")
    {
        out >> lowHealth;
    }
    else if (name == "MediumHealth")
    {
        out >> mediumHealth;
    }
    else if (name == "AlmostFullHealth")
    {
        out >> almostFullHealth;
    }
    else if (name == "LowMana")
    {
        out >> lowMana;
    }

    else if (name == "IterationsPerTick")
    {
        out >> iterationsPerTick;
    }
}


void PlayerbotAIConfig::CreateRandomBots()
{
    string randomBotAccountPrefix = sConfig.GetStringDefault("AiPlayerbot.RandomBotAccountPrefix", "rndbot");
    int32 randomBotAccountCount = sConfig.GetIntDefault("AiPlayerbot.RandomBotAccountCount", 50);

    if (sConfig.GetBoolDefault("AiPlayerbot.DeleteRandomBotAccounts", false))
    {
        sLog.outBasic("Deleting random bot accounts...");
        QueryResult *results = LoginDatabase.PQuery("SELECT `id` FROM `account` WHERE `username` LIKE '%s%%'", randomBotAccountPrefix.c_str());
        if (results)
        {
            do
            {
                Field* fields = results->Fetch();
                sAccountMgr.DeleteAccount(fields[0].GetUInt32());
            } while (results->NextRow());

            delete results;
        }

        CharacterDatabase.Execute("DELETE FROM `ai_playerbot_random_bots`");
        sLog.outBasic("Random bot accounts deleted");
    }

    for (int accountNumber = 0; accountNumber < randomBotAccountCount; ++accountNumber)
    {
        ostringstream out; out << randomBotAccountPrefix << accountNumber;
        string accountName = out.str();
        QueryResult *results = LoginDatabase.PQuery("SELECT `id` FROM `account` WHERE `username` = '%s'", accountName.c_str());
        if (results)
        {
            delete results;
            continue;
        }

        string password = "";
        for (int i = 0; i < 10; i++)
        {
            password += (char)urand('!', 'z');
        }
        sAccountMgr.CreateAccount(accountName, password);

        sLog.outDetail("Account %s created for random bots", accountName.c_str());
    }

    LoginDatabase.PExecute("UPDATE `account` SET `expansion` = '%u', `playerbot` = %u WHERE `username` LIKE '%s%%'", 2,true, randomBotAccountPrefix.c_str());

    int totalRandomBotChars = 0;
    for (int accountNumber = 0; accountNumber < randomBotAccountCount; ++accountNumber)
    {
        ostringstream out; out << randomBotAccountPrefix << accountNumber;
        string accountName = out.str();

        QueryResult *results = LoginDatabase.PQuery("SELECT `id` FROM `account` WHERE `username` = '%s'", accountName.c_str());
        if (!results)
        {
            continue;
        }

        Field* fields = results->Fetch();
        uint32 accountId = fields[0].GetUInt32();
        delete results;

        randomBotAccounts.push_back(accountId);

        int count = sAccountMgr.GetCharactersCount(accountId);
        if (count >= 10)
        {
            totalRandomBotChars += count;
            continue;
        }

        RandomPlayerbotFactory factory(accountId);
        for (uint8 cls = CLASS_WARRIOR; cls < MAX_CLASSES; ++cls)
        {
            if (cls != 10 && cls != 6)
            {
                factory.CreateRandomBot(cls);
            }
        }

        totalRandomBotChars += sAccountMgr.GetCharactersCount(accountId);
    }

    sLog.outBasic("%d random bot accounts with %d characters available", randomBotAccounts.size(), totalRandomBotChars);
}

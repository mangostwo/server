#include "../botpch.h"
#include "AhBotConfig.h"
#include "SystemConfig.h"
std::vector<std::string> split(const std::string &s, char delim);

using namespace std;

INSTANTIATE_SINGLETON_1(AhBotConfig);

AhBotConfig::AhBotConfig(): enabled(false) {};

template <class T>
void LoadSet(const string& value, T &res)
{
    vector<string> ids = split(value, ',');
    for (vector<string>::iterator i = ids.begin(); i != ids.end(); ++i)
    {
        uint32 id = atoi((*i).c_str());
        if (!id)
        {
            continue;
        }

        res.insert(id);
    }
}

bool AhBotConfig::Initialize()
{
    const char* cfg_file = SYSCONFDIR"ahbot.conf";

    if (!config.SetSource(cfg_file))
    {
        sLog.outString("Failed to load config file %s", cfg_file);
        return false;
    }

    enabled = config.GetBoolDefault("AhBot.Enabled", false);

    if (enabled)
    {
        guid = (uint64)config.GetIntDefault("AhBot.GUID", 0);
        updateInterval = config.GetIntDefault("AhBot.UpdateIntervalInSeconds", 300);
        historyDays = config.GetIntDefault("AhBot.History.Days", 30);
        itemBuyMinInterval = config.GetIntDefault("AhBot.ItemBuyMinInterval", 7200);
        itemBuyMaxInterval = config.GetIntDefault("AhBot.ItemBuyMaxInterval", 28800);
        itemSellMinInterval = config.GetIntDefault("AhBot.ItemSellMinInterval", 7200);
        itemSellMaxInterval = config.GetIntDefault("AhBot.ItemSellMaxInterval", 28800);
        maxSellInterval = config.GetIntDefault("AhBot.MaxSellInterval", 3600 * 8);
        alwaysAvailableMoney = config.GetIntDefault("AhBot.AlwaysAvailableMoney", 200000);
        priceMultiplier = config.GetFloatDefault("AhBot.PriceMultiplier", 1.0f);
        defaultMinPrice = config.GetIntDefault("AhBot.DefaultMinPrice", 20);
        maxItemLevel = config.GetIntDefault("AhBot.MaxItemLevel", 199);
        maxRequiredLevel = config.GetIntDefault("AhBot.MaxRequiredLevel", 80);
        priceQualityMultiplier = config.GetFloatDefault("AhBot.PriceQualityMultiplier", 1.0f);
        underPriceProbability = config.GetFloatDefault("AhBot.UnderPriceProbability", 0.05f);
        LoadSet<set<uint32> >(config.GetStringDefault("AhBot.IgnoreItemIds", "49283,52200,8494,6345,6891,2460,37164,34835"), ignoreItemIds);
        sLog.outString("AhBot module config loaded");
    }
    else
    {
        sLog.outString("AhBot module is disabled in ahbot.conf");
    }
    return enabled;
}

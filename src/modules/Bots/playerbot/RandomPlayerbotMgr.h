#ifndef RandomPlayerbotMgr_H
#define RandomPlayerbotMgr_H

#include "Common.h"
#include "PlayerbotAIBase.h"
#include "PlayerbotMgr.h"

class WorldPacket;
class Player;
class Unit;
class Object;
class Item;

using namespace std;

class RandomPlayerbotMgr : public PlayerbotHolder
{
    public:
        RandomPlayerbotMgr();
        ~RandomPlayerbotMgr() override;
        static RandomPlayerbotMgr& instance()
        {
            static RandomPlayerbotMgr instance;
            return instance;
        }

        void UpdateAIInternal(uint32 elapsed) override;

    public:
        static bool HandlePlayerbotConsoleCommand(char const *args);
        static bool IsRandomBot(Player* bot);
        static bool IsRandomBot(uint32 bot);
        static void Randomize(Player* bot);
        static void RandomizeFirst(Player* bot);
        static void IncreaseLevel(Player* bot);
        static void ScheduleTeleport(uint32 bot, uint32 time = 0);
        void HandleCommand(uint32 type, const string& text, Player& fromPlayer);
        string HandleRemoteCommand(string request);
        void OnPlayerLogout(Player* player);
        void OnPlayerLogin(Player* player);
        Player* GetRandomPlayer();
        void PrintStats();
        static double GetBuyMultiplier(Player* bot);
        static double GetSellMultiplier(Player* bot);
        static uint32 GetLootAmount(Player* bot);
        static void SetLootAmount(Player* bot, uint32 value);
        static uint32 GetTradeDiscount(Player* bot);
    //    static void Refresh(Player* bot);
        static void RandomTeleportForLevel(Player* bot);
    //    void RandomTeleport(Player * bot, uint32 mapId, float teleX, float teleY, float teleZ);
        static uint32 GetMaxAllowedBotCount();
        bool ProcessBot(Player* player);
        static void Revive(Player* player);

    protected:
        void OnBotLoginInternal(Player * const bot) override {}

    private:
        static uint32 GetEventValue(uint32 bot, const string& event);
        static uint32 SetEventValue(uint32 bot, const string& event, uint32 value, uint32 validIn);
        static void GetBots(set<uint32> &bots);
        static void AddRandomBots(set<uint32> &bots);
        bool ProcessBot(uint32 bot);
        static void ScheduleRandomize(uint32 bot, uint32 time);
        //void RandomTeleport(Player* bot);
        static void RandomTeleport(Player* bot, vector<WorldLocation> &locs);
        static uint32 GetZoneLevel(uint16 mapId, float teleX, float teleY);

    private:
        vector<Player*> players;
        static map<uint8, vector<WorldLocation> > locsPerLevelCache;
};

#define sRandomPlayerbotMgr RandomPlayerbotMgr::instance()

#endif

#ifndef PLAYERBOTMGR_H
#define PLAYERBOTMGR_H

#include "Common.h"
#include "PlayerbotAIBase.h"
#include "../botpch.h"

class WorldPacket;
class Player;
class Unit;
class Object;
class Item;

typedef map<uint64, Player*> PlayerBotMap;

class PlayerbotHolder : public PlayerbotAIBase
{
public:
    PlayerbotHolder();
    virtual ~PlayerbotHolder();

    void AddPlayerBot(uint64 guid, uint32 masterAccountId);
//    void HandlePlayerBotLoginCallback(QueryResult * dummy, SqlQueryHolder * holder);

    void LogoutPlayerBot(uint64 guid);
    Player* GetBotByGUID (uint64 playerGuid) const;
    PlayerBotMap::const_iterator GetPlayerBotsBegin() const { return playerBots.begin(); }
    PlayerBotMap::const_iterator GetPlayerBotsEnd()   const { return playerBots.end();   }

    void UpdateAIInternal(__attribute__((unused)) uint32 elapsed) override;
    void UpdateSessions(__attribute__((unused)) uint32 elapsed);

    void LogoutAllBots();
    void OnBotLogin(Player* bot);

    list<string> HandlePlayerbotCommand(char const* args, Player* master = nullptr);
    string ProcessBotCommand(const string& cmd, ObjectGuid guid, bool admin, uint32 masterAccountId, uint32 masterGuildId);
    static uint32 GetAccountId(const string& name);
    string ListBots(Player* master) const;

protected:
    __attribute__((unused)) virtual void OnBotLoginInternal(Player* bot) = 0;

protected:
    PlayerBotMap playerBots;
};

class PlayerbotMgr : public PlayerbotHolder
{
public:
    explicit PlayerbotMgr(Player* master);
    ~PlayerbotMgr() override;

    static bool HandlePlayerbotMgrCommand(ChatHandler* handler, char const* args);
    void HandleMasterIncomingPacket(const WorldPacket& packet);
    void HandleMasterOutgoingPacket(const WorldPacket& packet);
    void HandleCommand(uint32 type, const string& text);

    __attribute__((unused)) void OnPlayerLogin(Player* player);

    void UpdateAIInternal(uint32 elapsed) override;

    Player* GetMaster() const { return master; };

    void SaveToDB();

protected:
    void OnBotLoginInternal(Player* bot) override;

private:
    Player* const master;
};

#endif

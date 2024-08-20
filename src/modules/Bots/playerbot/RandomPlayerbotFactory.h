#ifndef RandomPlayerbotFactory_H
#define RandomPlayerbotFactory_H

#include "Common.h"
#include "PlayerbotAIBase.h"

class WorldPacket;
class Player;
class Unit;
class Object;
class Item;

using namespace std;

class RandomPlayerbotFactory
{
    public:
        explicit RandomPlayerbotFactory(uint32 accountId);
        virtual ~RandomPlayerbotFactory() = default;

    public:
        bool CreateRandomBot(uint8 cls) const;
        static void CreateRandomBots();
        static void CreateRandomGuilds();
        static void DeleteRandomBots();
        static void DeleteRandomGuilds();

    private:
        static bool CreateRandomBotName(string& name);
        static bool CreateRandomGuildName(string& guildName);

    private:
        uint32 accountId;
};

#endif

#include "botpch.h"
#include "../../playerbot.h"
#include "FlagAction.h"
#include "../values/LootStrategyValue.h"
#include "LootAction.h"

using namespace ai;

bool FlagAction::TellUsage()
{
    ai->TellError("Usage: flag cloak/helm/pvp on/set/off/clear/toggle/?");
    return false;
}

bool FlagAction::Execute(Event event)
{
    string cmd = event.getParam();
    vector<string> ss = split(cmd, ' ');
    if (ss.size() != 2)
        return TellUsage();

    bool setFlag = (ss[1] == "set" || ss[1] == "on");
    bool clearFlag = (ss[1] == "clear" || ss[1] == "off");
    bool toggleFlag = (ss[1] == "toggle");
    if (ss[0] == "pvp")
    {
        if (setFlag) bot->SetPvP(true);
        else if (clearFlag) bot->SetPvP(false);
        else if (toggleFlag) bot->SetPvP(!bot->IsPvP());
        ostringstream out; out << ss[0] << " flag is " << chat->formatBoolean(bot->IsPvP());
        ai->TellMaster(out.str());
        return true;
    }

    uint32 playerFlags;
    if (ss[0] == "cloak") playerFlags = PLAYER_FLAGS_HIDE_CLOAK;
    if (ss[0] == "helm") playerFlags = PLAYER_FLAGS_HIDE_HELM;

    if (clearFlag) bot->SetFlag(PLAYER_FLAGS, playerFlags);
    else if (setFlag) bot->RemoveFlag(PLAYER_FLAGS, playerFlags);
    else if (toggleFlag && bot->HasFlag(PLAYER_FLAGS, playerFlags)) bot->RemoveFlag(PLAYER_FLAGS, playerFlags);
    else if (toggleFlag && !bot->HasFlag(PLAYER_FLAGS, playerFlags)) bot->SetFlag(PLAYER_FLAGS, playerFlags);
    ostringstream out; out << ss[0] << " flag is " << chat->formatBoolean(!bot->HasFlag(PLAYER_FLAGS, playerFlags));
    ai->TellMaster(out.str());
    return true;
}


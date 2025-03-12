#include "botpch.h"
#include "../../playerbot.h"
#include "NewPlayerNearbyValue.h"

using namespace ai;

ObjectGuid NewPlayerNearbyValue::Calculate()
{
    list<ObjectGuid> players = ai->GetAiObjectContext()->GetValue<list<ObjectGuid> >("nearest friendly players")->Get();
    set<ObjectGuid>& alreadySeenPlayers = ai->GetAiObjectContext()->GetValue<set<ObjectGuid>& >("already seen players")->Get();
    for (list<ObjectGuid>::iterator i = players.begin(); i != players.end(); ++i)
    {
        ObjectGuid guid = *i;
        Player *pl = dynamic_cast<Player*>(ai->GetUnit(guid));
        if (pl && !ai->IsOpposing(pl) && alreadySeenPlayers.find(guid) == alreadySeenPlayers.end())
            return guid;
    }

    return ObjectGuid();
}

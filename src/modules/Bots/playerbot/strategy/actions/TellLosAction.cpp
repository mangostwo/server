#include "botpch.h"
#include "../../playerbot.h"
#include "TellLosAction.h"

#include "../../ServerFacade.h"

using namespace ai;

bool TellLosAction::Execute(Event event)
{
    string param = event.getParam();

    if (param.empty() || param == "targets")
    {
        ListUnits("--- Targets ---", *context->GetValue<list<ObjectGuid> >("possible targets"));
        ListUnits("--- Targets (All) ---", *context->GetValue<list<ObjectGuid> >("all targets"));
    }

    if (param.empty() || param == "npcs")
    {
        ListUnits("--- NPCs ---", *context->GetValue<list<ObjectGuid> >("nearest npcs"));
    }

    if (param.empty() || param == "corpses")
    {
        ListUnits("--- Corpses ---", *context->GetValue<list<ObjectGuid> >("nearest corpses"));
    }

    if (param.empty() || param == "gos" || param == "game objects")
    {
        ListGameObjects("--- Game objects ---", *context->GetValue<list<ObjectGuid> >("nearest game objects"));
    }

    if (param.empty() || param == "players")
    {
        ListUnits("--- Friendly players ---", *context->GetValue<list<ObjectGuid> >("nearest friendly players"));
    }

    return true;
}

void TellLosAction::ListGrouped(string title, map<string, int32> grouped)
{
    if (grouped.empty()) return;

    ai->TellMasterNoFacing(title);

    for (map<string, int32>::iterator i = grouped.begin(); i != grouped.end(); i++)
    {
        ostringstream out;
        out << i->first;
        if (i->second > 1) out << " (x" << i->second << ")";
        ai->TellMasterNoFacing(out.str());
    }
}

void TellLosAction::ListUnits(string title, list<ObjectGuid> units)
{
    map<string, int32> grouped;
    for (list<ObjectGuid>::iterator i = units.begin(); i != units.end(); i++)
    {
        Unit* unit = ai->GetUnit(*i);
        if (unit)
        {
            float dist = sServerFacade.GetDistance2d(bot, unit);
            if (dist <= sPlayerbotAIConfig.sightDistance && sServerFacade.IsWithinLOSInMap(bot, unit))
            {
                grouped[unit->GetName()]++;
            }
        }
    }

    ListGrouped(title, grouped);
}

void TellLosAction::ListGameObjects(string title, list<ObjectGuid> gos)
{
    map<string, int32> groupedByName;
    for (list<ObjectGuid>::iterator i = gos.begin(); i != gos.end(); i++)
    {
        GameObject* go = ai->GetGameObject(*i);
        if (go)
        {
            float dist = sServerFacade.GetDistance2d(bot, go);
            if (dist <= sPlayerbotAIConfig.sightDistance && sServerFacade.IsWithinLOSInMap(bot, go))
            {
                groupedByName[go->GetName()]++;
            }
        }
    }

    map<string, int32> groupedByLink;
    for (map<string, int32>::iterator i = groupedByName.begin(); i != groupedByName.end(); i++)
    {
        GameObject* closest = NULL;
        float minDist = sPlayerbotAIConfig.sightDistance;
        for (list<ObjectGuid>::iterator j = gos.begin(); j != gos.end(); j++)
        {
            GameObject* go = ai->GetGameObject(*j);
            if (go && i->first == go->GetName())
            {
                float dist = sServerFacade.GetDistance2d(bot, go);
                if (dist < minDist)
                {
                    closest = go;
                    minDist = dist;
                }
            }
        }
        if (closest)
        {
            groupedByLink[chat->formatGameobject(closest)] = groupedByName[closest->GetName()];
        }
    }

    ListGrouped(title, groupedByLink);
}

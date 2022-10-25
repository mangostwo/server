#include "botpch.h"
#include "../../playerbot.h"
#include "../values/PositionValue.h"
#include "PositionAction.h"

using namespace ai;

void TellPosition(PlayerbotAI* ai, string name, ai::Position pos)
{
    ostringstream out; out << "Position " << name;
    if (pos.isSet())
    {
        float x = pos.x, y = pos.y;
        Map2ZoneCoordinates(x, y, ai->GetBot()->GetZoneId());
        out << " is set to " << x << "," << y;
    }
    else
    {
        out << " is not set";
    }
    ai->TellMaster(out);
}

bool PositionAction::Execute(Event event)
{
    string param = event.getParam();
    if (param.empty())
 {
     return false;
 }

    Player* master = GetMaster();
    if (!master)
    {
        return false;
    }

    ai::PositionMap& posMap = context->GetValue<ai::PositionMap&>("position")->Get();
    if (param == "?")
    {
        for (ai::PositionMap::iterator i = posMap.begin(); i != posMap.end(); ++i)
        {
            if (i->second.isSet())
            {
                TellPosition(ai, i->first, i->second);
            }
        }
        return true;
    }

    vector<string> params = split(param, ' ');
    if (params.size() != 2)
    {
        ai->TellMaster("Whisper position <name> ?/set/reset");
        return false;
    }

    string name = params[0];
    string action = params[1];
    ai::Position pos = posMap[name];
    if (action == "?")
    {
        TellPosition(ai, name, pos);
        return true;
    }

    vector<string> coords = split(action, ',');
    if (coords.size() == 3)
    {
        pos.Set(atoi(coords[0].c_str()), atoi(coords[1].c_str()), atoi(coords[2].c_str()));
        posMap[name] = pos;

        ostringstream out; out << "Position " << name << " is set";
        ai->TellMaster(out);
        return true;
    }

    if (action == "set")
    {
        pos.Set( bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ());
        posMap[name] = pos;

        ostringstream out; out << "Position " << name << " is set";
        ai->TellMaster(out);
        return true;
    }

    if (action == "reset")
    {
        pos.Reset();
        posMap[name] = pos;

        ostringstream out; out << "Position " << name << " is reset";
        ai->TellMaster(out);
        return true;
    }

    return false;
}

bool MoveToPositionAction::Execute(Event event)
{
    ai::Position pos = context->GetValue<ai::PositionMap&>("position")->Get()[qualifier];
    if (!pos.isSet())
    {
        ostringstream out; out << "Position " << qualifier << " is not set";
        ai->TellMaster(out);
        return false;
    }

    return MoveTo(bot->GetMapId(), pos.x, pos.y, pos.z);
}


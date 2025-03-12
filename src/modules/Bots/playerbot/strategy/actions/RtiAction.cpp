#include "botpch.h"
#include "../../playerbot.h"
#include "RtiAction.h"
#include "../../PlayerbotAIConfig.h"
#include "../values/RtiTargetValue.h"

using namespace ai;

bool RtiAction::Execute(Event event)
{
    string text = event.getParam();
    string type = "rti";
    if (text.find("cc ") == 0)
    {
        type = "rti cc";
        text = text.substr(3);
    }
    if (text.empty() || text == "?")
    {
        ostringstream outRti; outRti << "rti" << ": ";
        AppendRti(outRti, "rti");
        ai->TellMaster(outRti);

        ostringstream outRtiCc; outRtiCc << "rti cc" << ": ";
        AppendRti(outRtiCc, "rti cc");
        ai->TellMaster(outRtiCc);
        return true;
    }

    context->GetValue<string>(type)->Set(text);
    ostringstream out; out << type << " set to: ";
    AppendRti(out, type);
    ai->TellMaster(out);
    return true;
}

void RtiAction::AppendRti(ostringstream & out, string type)
{
    out << AI_VALUE(string, type);

    ostringstream n; n << type << " target";
    Unit* target = AI_VALUE(Unit*, n.str());
    if(target)
        out << " (" << target->GetName() << ")";

}

bool MarkRtiAction::Execute(Event event)
{
    Group *group = bot->GetGroup();
    if (!group) return false;

    Unit* target = NULL;
    list<ObjectGuid> attackers = ai->GetAiObjectContext()->GetValue<list<ObjectGuid> >("attackers")->Get();
    for (list<ObjectGuid>::iterator i = attackers.begin(); i != attackers.end(); ++i)
    {
        Unit* unit = ai->GetUnit(*i);
        if (!unit)
            continue;

        bool marked = false;
        for (int i = 0; i < 8; i++)
        {
            ObjectGuid guid = group->GetTargetIcon(i);
            if (guid == unit->GetObjectGuid())
            {
                marked = true;
                break;
            }
        }

        if (marked) continue;

        if (!target || (int)target->GetHealth() > (int)unit->GetHealth()) target = unit;
    }

    if (!target) return false;

    string rti = AI_VALUE(string, "rti");
    int index = RtiTargetValue::GetRtiIndex(rti);
    group->SetTargetIcon(index, 
#ifdef MANGOSBOT_TWO
        bot->GetObjectGuid(),
#endif
        target->GetObjectGuid());
    return true;
}


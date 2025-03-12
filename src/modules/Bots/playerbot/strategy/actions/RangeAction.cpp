#include "botpch.h"
#include "../../playerbot.h"
#include "RangeAction.h"
#include "../../PlayerbotAIConfig.h"

using namespace ai;

bool RangeAction::Execute(Event event)
{
    string param = event.getParam();
    if (param == "?")
    {
        PrintRange("spell");
        PrintRange("heal");
        PrintRange("shoot");
        PrintRange("flee");
    }

    int pos = param.find(" ");
    if (pos == string::npos) return false;

    string qualifier = param.substr(0, pos);
    string value = param.substr(pos + 1);

    if (value == "?")
    {
        PrintRange(qualifier);
        return true;
    }

    float newVal = (float) atof(value.c_str());
    context->GetValue<float>("range", qualifier)->Set(newVal);
    ostringstream out;
    out << qualifier << " range set to: " << newVal;
    ai->TellMaster(out.str());
    return true;
}

void RangeAction::PrintRange(string type)
{
    float curVal = AI_VALUE2(float, "range", type);

    ostringstream out;
    out << type << " range: ";
    if (abs(curVal) >= 0.1f) out << curVal;
    else out << ai->GetRange(type) << " (default)";

    ai->TellMaster(out.str());
}
#include "botpch.h"
#include "../../playerbot.h"
#include "RangeValues.h"

using namespace ai;

RangeValue::RangeValue(PlayerbotAI* ai)
    : ManualSetValue<float>(ai, 0, "range"), Qualified()
{
}

string RangeValue::Save()
{
    ostringstream out; out << value; return out.str();
}

bool RangeValue::Load(string text)
{
    value = atof(text.c_str());
    return true;
}

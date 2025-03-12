#include "botpch.h"
#include "AlwaysLootListValue.h"
#include "../../playerbot.h"
#include "../../PlayerbotAIConfig.h"
#include "../../ServerFacade.h"

using namespace ai;

string AlwaysLootListValue::Save()
{
    ostringstream out;
    bool first = true;
    for (set<uint32>::iterator i = value.begin(); i != value.end(); ++i)
    {
        if (!first) out << ",";
        else first = false;
        out << *i;
    }
    return out.str();
}

bool AlwaysLootListValue::Load(string text)
{
    value.clear();

    vector<string> ss = split(text, ',');
    for (vector<string>::iterator i = ss.begin(); i != ss.end(); ++i)
    {
        value.insert(atoi(i->c_str()));
    }
    return true;
}

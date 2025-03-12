#include "botpch.h"
#include "../../playerbot.h"
#include "RtiValue.h"

using namespace ai;

RtiValue::RtiValue(PlayerbotAI* ai)
    : ManualSetValue<string>(ai, "skull")
{
}

RtiCcValue::RtiCcValue(PlayerbotAI* ai)
    : ManualSetValue<string>(ai, "moon")
{
}

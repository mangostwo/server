#include "botpch.h"
#include "../../playerbot.h"
#include "PriestTriggers.h"
#include "PriestActions.h"

using namespace ai;

bool InnerFireTrigger::IsActive()
{
    Unit* target = GetTarget();
    return SpellTrigger::IsActive() && !ai->HasAura(spell, target);
}
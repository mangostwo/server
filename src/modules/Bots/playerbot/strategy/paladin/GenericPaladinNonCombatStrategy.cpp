#include "botpch.h"
#include "../../playerbot.h"
#include "PaladinMultipliers.h"
#include "GenericPaladinNonCombatStrategy.h"
#include "GenericPaladinStrategyActionNodeFactory.h"

using namespace ai;

GenericPaladinNonCombatStrategy::GenericPaladinNonCombatStrategy(PlayerbotAI* ai) : NonCombatStrategy(ai)
{
    actionNodeFactories.Add(new GenericPaladinStrategyActionNodeFactory());
}

void GenericPaladinNonCombatStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    NonCombatStrategy::InitTriggers(triggers);

	triggers.push_back(new TriggerNode(
		"party member dead",
		NextAction::array(0, new NextAction("redemption", 30.0f), NULL)));
}

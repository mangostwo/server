#include "botpch.h"
#include "../../playerbot.h"
#include "WarlockTriggers.h"
#include "WarlockMultipliers.h"
#include "DpsWarlockStrategy.h"
#include "WarlockActions.h"

using namespace ai;

class DpsWarlockStrategyActionNodeFactory : public NamedObjectFactory<ActionNode>
{
public:
    DpsWarlockStrategyActionNodeFactory()
    {
        creators["shadow bolt"] = &shadow_bolt;
    }
private:
    static ActionNode* shadow_bolt(PlayerbotAI* ai)
    {
        return new ActionNode ("shadow bolt",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("shoot"), NULL),
            /*C*/ NULL);
    }
};

DpsWarlockStrategy::DpsWarlockStrategy(PlayerbotAI* ai) : GenericWarlockStrategy(ai)
{
    actionNodeFactories.Add(new DpsWarlockStrategyActionNodeFactory());
}


NextAction** DpsWarlockStrategy::getDefaultActions()
{
    return NextAction::array(0, new NextAction("incinirate", 10.0f), new NextAction("shadow bolt", 10.0f), NULL);
}

void DpsWarlockStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    GenericWarlockStrategy::InitTriggers(triggers);

	triggers.push_back(new TriggerNode(
		"backlash",
		NextAction::array(0, new NextAction("shadow bolt", 20.0f), NULL)));
}

void DpsAoeWarlockStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "medium aoe",
        NextAction::array(0, new NextAction("rain of fire", 37.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "corruption on attacker",
        NextAction::array(0, new NextAction("corruption on attacker", 27.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "curse of agony on attacker",
        NextAction::array(0, new NextAction("curse of agony on attacker", 28.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "siphon life on attacker",
        NextAction::array(0, new NextAction("siphon life on attacker", 29.0f), NULL)));
}

void DpsWarlockDebuffStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "corruption",
        NextAction::array(0, new NextAction("corruption", 22.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "curse of agony",
        NextAction::array(0, new NextAction("curse of agony", 21.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "siphon life",
        NextAction::array(0, new NextAction("siphon life", 23.0f), NULL)));

}

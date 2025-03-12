#include "botpch.h"
#include "../../playerbot.h"
#include "WarlockMultipliers.h"
#include "GenericWarlockStrategy.h"

using namespace ai;

class GenericWarlockStrategyActionNodeFactory : public NamedObjectFactory<ActionNode>
{
public:
    GenericWarlockStrategyActionNodeFactory()
    {
        creators["summon voidwalker"] = &summon_voidwalker;
        creators["banish"] = &banish;
    }
private:
    static ActionNode* summon_voidwalker(PlayerbotAI* ai)
    {
        return new ActionNode ("summon voidwalker",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("drain soul"), NULL),
            /*C*/ NULL);
    }
    static ActionNode* banish(PlayerbotAI* ai)
    {
        return new ActionNode ("banish",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("fear"), NULL),
            /*C*/ NULL);
    }
};

GenericWarlockStrategy::GenericWarlockStrategy(PlayerbotAI* ai) : CombatStrategy(ai)
{
    actionNodeFactories.Add(new GenericWarlockStrategyActionNodeFactory());
}

NextAction** GenericWarlockStrategy::getDefaultActions()
{
    return NextAction::array(0, new NextAction("shoot", 10.0f), NULL);
}

void GenericWarlockStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    CombatStrategy::InitTriggers(triggers);

    triggers.push_back(new TriggerNode(
        "shadow trance",
        NextAction::array(0, new NextAction("shadow bolt", 20.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "low health",
        NextAction::array(0, new NextAction("drain life", 40.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "medium mana",
        NextAction::array(0, new NextAction("life tap", ACTION_EMERGENCY + 5), NULL)));

	triggers.push_back(new TriggerNode(
		"target critical health",
		NextAction::array(0, new NextAction("drain soul", 30.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "immolate",
        NextAction::array(0, new NextAction("immolate", 13.0f), new NextAction("conflagrate", 13.0f), NULL)));
}

void WarlockBoostStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "amplify curse",
        NextAction::array(0, new NextAction("amplify curse", 41.0f), NULL)));
}

void WarlockCcStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "banish",
        NextAction::array(0, new NextAction("banish on cc", 32.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "fear",
        NextAction::array(0, new NextAction("fear on cc", 33.0f), NULL)));
}
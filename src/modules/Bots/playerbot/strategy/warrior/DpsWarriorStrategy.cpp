#include "botpch.h"
#include "../../playerbot.h"
#include "WarriorMultipliers.h"
#include "DpsWarriorStrategy.h"

using namespace ai;

class DpsWarriorStrategyActionNodeFactory : public NamedObjectFactory<ActionNode>
{
public:
    DpsWarriorStrategyActionNodeFactory()
    {
        creators["charge"] = &charge;
        creators["bloodthirst"] = &bloodthirst;
        creators["death wish"] = &death_wish;
    }
private:
    static ActionNode* charge(PlayerbotAI* ai)
    {
        return new ActionNode ("charge",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("reach melee"), NULL),
            /*C*/ NULL);
    }
    static ActionNode* bloodthirst(PlayerbotAI* ai)
    {
        return new ActionNode ("bloodthirst",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("heroic strike"), NULL),
            /*C*/ NULL);
    }
    static ActionNode* death_wish(PlayerbotAI* ai)
    {
        return new ActionNode ("death wish",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("berserker rage"), NULL),
            /*C*/ NULL);
    }
};

DpsWarriorStrategy::DpsWarriorStrategy(PlayerbotAI* ai) : GenericWarriorStrategy(ai)
{
    actionNodeFactories.Add(new DpsWarriorStrategyActionNodeFactory());
}

NextAction** DpsWarriorStrategy::getDefaultActions()
{
    return NextAction::array(0, new NextAction("bloodthirst", ACTION_NORMAL + 1), new NextAction("melee", ACTION_NORMAL), NULL);
}

void DpsWarriorStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    GenericWarriorStrategy::InitTriggers(triggers);

    triggers.push_back(new TriggerNode(
        "enemy out of melee",
        NextAction::array(0, new NextAction("charge", ACTION_NORMAL + 9), NULL)));

    triggers.push_back(new TriggerNode(
        "battle stance",
        NextAction::array(0, new NextAction("battle stance", ACTION_HIGH + 9), NULL)));

    triggers.push_back(new TriggerNode(
        "target critical health",
        NextAction::array(0, new NextAction("execute", ACTION_HIGH + 4), NULL)));

	triggers.push_back(new TriggerNode(
		"hamstring",
		NextAction::array(0, new NextAction("hamstring", ACTION_HIGH), NULL)));

	triggers.push_back(new TriggerNode(
		"victory rush",
		NextAction::array(0, new NextAction("victory rush", ACTION_HIGH + 3), NULL)));

    triggers.push_back(new TriggerNode(
        "death wish",
        NextAction::array(0, new NextAction("death wish", ACTION_HIGH + 2), NULL)));

    triggers.push_back(new TriggerNode(
        "rend",
        NextAction::array(0, new NextAction("rend", ACTION_NORMAL + 1), NULL)));
}


void DpsWarrirorAoeStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "rend on attacker",
        NextAction::array(0, new NextAction("rend on attacker", ACTION_HIGH + 1), NULL)));

    triggers.push_back(new TriggerNode(
        "medium aoe",
        NextAction::array(0, new NextAction("thunder clap", ACTION_HIGH + 2), new NextAction("demoralizing shout", ACTION_HIGH + 2), NULL)));

    triggers.push_back(new TriggerNode(
        "light aoe",
        NextAction::array(0, new NextAction("cleave", ACTION_HIGH + 3), NULL)));
}

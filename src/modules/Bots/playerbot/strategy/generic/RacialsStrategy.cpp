#include "botpch.h"
#include "../../playerbot.h"
#include "RacialsStrategy.h"

using namespace ai;


class RacialsStrategyActionNodeFactory : public NamedObjectFactory<ActionNode>
{
public:
    RacialsStrategyActionNodeFactory()
    {
        creators["lifeblood"] = &lifeblood;
    }
private:
    static ActionNode* lifeblood(PlayerbotAI* ai)
    {
        return new ActionNode ("lifeblood",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("gift of the naaru"), NULL),
            /*C*/ NULL);
    }
};

void RacialsStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
	triggers.push_back(new TriggerNode(
		"low health",
		NextAction::array(0, new NextAction("lifeblood", 71.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "medium mana",
        NextAction::array(0, new NextAction("mana tap", ACTION_MEDIUM_HEAL + 2), NULL)));

    triggers.push_back(new TriggerNode(
        "low mana",
        NextAction::array(0, new NextAction("arcane torrent", ACTION_MEDIUM_HEAL + 1), NULL)));
}

RacialsStrategy::RacialsStrategy(PlayerbotAI* ai) : Strategy(ai)
{
    actionNodeFactories.Add(new RacialsStrategyActionNodeFactory());
}

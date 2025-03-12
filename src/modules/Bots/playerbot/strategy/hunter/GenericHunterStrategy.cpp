#include "botpch.h"
#include "../../playerbot.h"
#include "GenericHunterStrategy.h"
#include "HunterAiObjectContext.h"

using namespace ai;

class GenericHunterStrategyActionNodeFactory : public NamedObjectFactory<ActionNode>
{
public:
    GenericHunterStrategyActionNodeFactory()
    {
        creators["rapid fire"] = &rapid_fire;
        creators["boost"] = &rapid_fire;
        creators["aspect of the pack"] = &aspect_of_the_pack;
        creators["feign death"] = &feign_death;
    }
private:
    static ActionNode* rapid_fire(PlayerbotAI* ai)
    {
        return new ActionNode ("rapid fire",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("readiness"), NULL),
            /*C*/ NULL);
    }
    static ActionNode* aspect_of_the_pack(PlayerbotAI* ai)
    {
        return new ActionNode ("aspect of the pack",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("aspect of the cheetah"), NULL),
            /*C*/ NULL);
    }
    static ActionNode* feign_death(PlayerbotAI* ai)
    {
        return new ActionNode ("feign death",
            /*P*/ NULL,
            /*A*/ NULL,
            /*C*/ NULL);
    }
};

GenericHunterStrategy::GenericHunterStrategy(PlayerbotAI* ai) : CombatStrategy(ai)
{
    actionNodeFactories.Add(new GenericHunterStrategyActionNodeFactory());
}

void GenericHunterStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    CombatStrategy::InitTriggers(triggers);

    triggers.push_back(new TriggerNode(
        "enemy is close",
        NextAction::array(0, new NextAction("wing clip", 50.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "medium threat",
        NextAction::array(0, new NextAction("feign death", 32.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "hunters pet low health",
        NextAction::array(0, new NextAction("mend pet", 20.0f), NULL)));
}

NextAction** HunterBoostStrategy::getDefaultActions()
{
    return NextAction::array(0, new NextAction("bestial wrath", 15.0f), NULL);
}

void HunterBoostStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "rapid fire",
        NextAction::array(0, new NextAction("rapid fire", 16.0f), NULL)));
}

void HunterCcStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "scare beast",
        NextAction::array(0, new NextAction("scare beast on cc", ACTION_HIGH + 3), NULL)));
}

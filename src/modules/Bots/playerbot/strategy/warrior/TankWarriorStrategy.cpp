#include "botpch.h"
#include "../../playerbot.h"
#include "WarriorMultipliers.h"
#include "TankWarriorStrategy.h"

using namespace ai;


TankWarriorStrategy::TankWarriorStrategy(PlayerbotAI* ai) : GenericWarriorStrategy(ai)
{
}

NextAction** TankWarriorStrategy::getDefaultActions()
{
    return NextAction::array(0, new NextAction("sunder armor", ACTION_NORMAL + 1), new NextAction("melee", ACTION_NORMAL), NULL);
}

void TankWarriorStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    GenericWarriorStrategy::InitTriggers(triggers);

    triggers.push_back(new TriggerNode(
        "defensive stance",
        NextAction::array(0, new NextAction("defensive stance", ACTION_HIGH + 9), NULL)));

    triggers.push_back(new TriggerNode(
        "medium rage available",
        NextAction::array(0, new NextAction("shield slam", ACTION_NORMAL + 2), new NextAction("heroic strike", ACTION_NORMAL + 2), NULL)));

    triggers.push_back(new TriggerNode(
        "shield block",
        NextAction::array(0, new NextAction("shield block", ACTION_NORMAL + 3), NULL)));

    triggers.push_back(new TriggerNode(
        "revenge",
        NextAction::array(0, new NextAction("revenge", ACTION_NORMAL + 4), NULL)));

    triggers.push_back(new TriggerNode(
        "lose aggro",
        NextAction::array(0, new NextAction("taunt", ACTION_HIGH + 8), NULL)));

    triggers.push_back(new TriggerNode(
        "low health",
        NextAction::array(0, new NextAction("shield wall", ACTION_MEDIUM_HEAL), NULL)));

	triggers.push_back(new TriggerNode(
		"critical health",
		NextAction::array(0, new NextAction("last stand", ACTION_EMERGENCY + 3), NULL)));

	triggers.push_back(new TriggerNode(
        "light aoe",
        NextAction::array(0, new NextAction("demoralizing shout", ACTION_HIGH + 2), new NextAction("cleave", ACTION_HIGH + 1), NULL)));

	triggers.push_back(new TriggerNode(
        "medium aoe",
        NextAction::array(0, new NextAction("battle shout taunt", ACTION_HIGH + 3), NULL)));

    triggers.push_back(new TriggerNode(
        "high aoe",
        NextAction::array(0, new NextAction("challenging shout", ACTION_HIGH + 3), NULL)));

	triggers.push_back(new TriggerNode(
		"concussion blow",
		NextAction::array(0, new NextAction("concussion blow", ACTION_INTERRUPT), NULL)));

    triggers.push_back(new TriggerNode(
        "sword and board",
        NextAction::array(0, new NextAction("shield slam", ACTION_HIGH + 3), NULL)));
}

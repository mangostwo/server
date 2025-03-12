#include "botpch.h"
#include "../../playerbot.h"
#include "PaladinMultipliers.h"
#include "PaladinBuffStrategies.h"

using namespace ai;

void PaladinBuffManaStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "blessing on party",
        NextAction::array(0, new NextAction("blessing of wisdom on party", 11.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "blessing",
        NextAction::array(0, new NextAction("blessing of wisdom", ACTION_HIGH + 8), NULL)));
}

void PaladinBuffHealthStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "blessing on party",
        NextAction::array(0, new NextAction("blessing of kings on party", 11.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "blessing",
        NextAction::array(0, new NextAction("blessing of kings", ACTION_HIGH + 8), NULL)));
}

void PaladinBuffDpsStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "blessing on party",
        NextAction::array(0, new NextAction("blessing of might on party", 11.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "blessing",
        NextAction::array(0, new NextAction("blessing of might", ACTION_HIGH + 8), NULL)));
}

void PaladinShadowResistanceStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
	triggers.push_back(new TriggerNode(
		"shadow resistance aura",
		NextAction::array(0, new NextAction("shadow resistance aura", 90.0f), NULL)));
}

void PaladinFrostResistanceStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
	triggers.push_back(new TriggerNode(
		"frost resistance aura",
		NextAction::array(0, new NextAction("frost resistance aura", 90.0f), NULL)));
}

void PaladinFireResistanceStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
	triggers.push_back(new TriggerNode(
		"fire resistance aura",
		NextAction::array(0, new NextAction("fire resistance aura", 90.0f), NULL)));
}


void PaladinBuffArmorStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
	triggers.push_back(new TriggerNode(
		"devotion aura",
		NextAction::array(0, new NextAction("devotion aura", 90.0f), NULL)));
}

void PaladinBuffAoeStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "retribution aura",
        NextAction::array(0, new NextAction("retribution aura", 90.0f), NULL)));
}

void PaladinBuffThreatStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "righteous fury",
        NextAction::array(0, new NextAction("righteous fury", ACTION_HIGH + 8), NULL)));

}

void PaladinBuffStatsStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "blessing on party",
        NextAction::array(0, new NextAction("blessing of kings on party", 11.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "blessing",
        NextAction::array(0, new NextAction("blessing of kings", ACTION_HIGH + 8), NULL)));
}

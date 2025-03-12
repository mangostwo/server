#include "botpch.h"
#include "../../playerbot.h"
#include "PaladinActions.h"

using namespace ai;

string GetActualBlessingOfMight(Unit* target)
{
    switch (target->getClass())
    {
    case CLASS_MAGE:
    case CLASS_PRIEST:
    case CLASS_WARLOCK:
        return "blessing of wisdom";
    }
    return "blessing of might";
}

string GetActualBlessingOfWisdom(Unit* target)
{
    switch (target->getClass())
    {
    case CLASS_WARRIOR:
    case CLASS_ROGUE:
        return "blessing of might";
    }
    return "blessing of wisdom";
}

Value<Unit*>* CastBlessingOnPartyAction::GetTargetValue()
{
    return context->GetValue<Unit*>("party member without aura",
            "blessing of kings,blessing of might,blessing of wisdom");
}

bool CastBlessingOfMightAction::Execute(Event event)
{
    Unit* target = GetTarget();
    if (!target) return false;

    return ai->CastSpell(GetActualBlessingOfMight(target), target);
}

bool CastBlessingOfMightOnPartyAction::Execute(Event event)
{
    Unit* target = GetTarget();
    if (!target) return false;

    return ai->CastSpell(GetActualBlessingOfMight(target), target);
}

bool CastBlessingOfWisdomAction::Execute(Event event)
{
    Unit* target = GetTarget();
    if (!target) return false;

    return ai->CastSpell(GetActualBlessingOfWisdom(target), target);
}

bool CastBlessingOfWisdomOnPartyAction::Execute(Event event)
{
    Unit* target = GetTarget();
    if (!target) return false;

    return ai->CastSpell(GetActualBlessingOfWisdom(target), target);
}
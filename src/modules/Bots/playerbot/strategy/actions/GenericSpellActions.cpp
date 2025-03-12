#include "botpch.h"
#include "../../playerbot.h"
#include "GenericActions.h"

using namespace ai;

bool CastSpellAction::Execute(Event event)
{
	return ai->CastSpell(spell, GetTarget());
}

bool CastSpellAction::isPossible()
{
	return ai->CanCastSpell(spell, GetTarget());
}

bool CastSpellAction::isUseful()
{
    return GetTarget() && AI_VALUE2(bool, "spell cast useful", spell) && AI_VALUE2(float, "distance", GetTargetName()) <= range;
}

bool CastAuraSpellAction::isUseful()
{
	return CastSpellAction::isUseful() && !ai->HasAura(spell, GetTarget(), true);
}

bool CastEnchantItemAction::isPossible()
{
    if (!CastSpellAction::isPossible())
        return false;

    uint32 spellId = AI_VALUE2(uint32, "spell id", spell);
    return spellId && AI_VALUE2(Item*, "item for spell", spellId);
}

bool CastHealingSpellAction::isUseful()
{
	return CastAuraSpellAction::isUseful();
}

bool CastAoeHealSpellAction::isUseful()
{
	return CastSpellAction::isUseful();
}


Value<Unit*>* CurePartyMemberAction::GetTargetValue()
{
    return context->GetValue<Unit*>("party member to dispel", dispelType);
}

Value<Unit*>* BuffOnPartyAction::GetTargetValue()
{
    return context->GetValue<Unit*>("party member without aura", spell);
}

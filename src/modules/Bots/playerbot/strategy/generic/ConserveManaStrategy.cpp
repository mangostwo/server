#include "botpch.h"
#include "../../playerbot.h"
#include "ConserveManaStrategy.h"
#include "../../PlayerbotAIConfig.h"
#include "../actions/GenericSpellActions.h"
#include "../values/LastSpellCastValue.h"
#include "../../ServerFacade.h"

using namespace ai;

float ConserveManaMultiplier::GetValue(Action* action)
{
    if (action == NULL) return 1.0f;

    uint8 health = AI_VALUE2(uint8, "health", "self target");
    uint8 targetHealth = AI_VALUE2(uint8, "health", "current target");
    uint8 mana = AI_VALUE2(uint8, "mana", "self target");
    bool hasMana = AI_VALUE2(bool, "has mana", "self target");
    bool mediumMana = hasMana && mana < sPlayerbotAIConfig.mediumMana;

    string name = action->getName();

    if (health < sPlayerbotAIConfig.lowHealth)
        return 1.0f;

    CastSpellAction* spellAction = dynamic_cast<CastSpellAction*>(action);
    if (!spellAction)
        return 1.0f;

    string spell = spellAction->getName();
    if (spell.find(" on party") != string::npos) spell = spell.substr(0, spell.size() - 9);
    uint32 spellId = AI_VALUE2(uint32, "spell id", spell);
    const SpellEntry* const spellInfo = sServerFacade.LookupSpellInfo(spellId);
    if (!spellInfo || spellInfo->powerType != POWER_MANA)
        return 1.0f;

    if (mediumMana && dynamic_cast<CastBuffSpellAction*>(action))
        return 0.0f;

    if (dynamic_cast<HealPartyMemberAction*>(action) && bot->GetGroup() && !ai->IsHeal(bot))
    {
        Group::MemberSlotList const& groupSlot = bot->GetGroup()->GetMemberSlots();
        bool foundHealers = false;
        bool foundLowManaHealers = false;
        for (Group::member_citerator itr = groupSlot.begin(); itr != groupSlot.end(); itr++)
        {
            Player *member = sObjectMgr.GetPlayer(itr->guid);
            if (!member || !sServerFacade.IsAlive(member) || member == bot)
                continue;

            if (ai->IsHeal(member) && bot->GetMapId() == member->GetMapId() &&
                    sServerFacade.IsDistanceLessOrEqualThan(sServerFacade.GetDistance2d(bot, member), sPlayerbotAIConfig.spellDistance))
            {
                float manaLevel = (static_cast<float> (member->GetPower(POWER_MANA)) / member->GetMaxPower(POWER_MANA)) * 100;
                foundHealers = true;
                if (manaLevel < sPlayerbotAIConfig.lowMana) foundLowManaHealers = true;
            }
        }

        if (foundHealers && !foundLowManaHealers) return 0.0f;
    }

    return 1.0f;
}

float SaveManaMultiplier::GetValue(Action* action)
{
    if (action == NULL)
        return 1.0f;

    if (action->GetTarget() != AI_VALUE(Unit*, "current target"))
        return 1.0f;

    double saveLevel = AI_VALUE(double, "mana save level");
    if (saveLevel <= 1.0)
        return 1.0f;

    CastSpellAction* spellAction = dynamic_cast<CastSpellAction*>(action);
    if (!spellAction)
        return 1.0f;

    string spell = spellAction->getName();
    uint32 spellId = AI_VALUE2(uint32, "spell id", spell);
    const SpellEntry* const spellInfo = sServerFacade.LookupSpellInfo(spellId);
    if (!spellInfo || spellInfo->powerType != POWER_MANA)
        return 1.0f;

    int32 cost = spellInfo->manaCost;
    if (!cost)
        return 1.0f;

    time_t lastCastTime = AI_VALUE2(time_t, "last spell cast time", spell);
    if (!lastCastTime)
        return 1.0f;

    time_t elapsed = time(0) - lastCastTime;
    if ((double)elapsed < 10 * saveLevel)
        return 0.0f;

    return 1.0f;
}


void ConserveManaStrategy::InitMultipliers(std::list<Multiplier*> &multipliers)
{
    multipliers.push_back(new ConserveManaMultiplier(ai));
}

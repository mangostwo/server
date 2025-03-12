#include "botpch.h"
#include "../../playerbot.h"
#include "PartyMemberToHeal.h"
#include "../../PlayerbotAIConfig.h"
#include "../../ServerFacade.h"

using namespace ai;

class IsTargetOfHealingSpell : public SpellEntryPredicate
{
public:
    virtual bool Check(SpellEntry const* spell) {
        for (int i=0; i<3; i++) {
            if (spell->Effect[i] == SPELL_EFFECT_HEAL ||
                spell->Effect[i] == SPELL_EFFECT_HEAL_MAX_HEALTH ||
                spell->Effect[i] == SPELL_EFFECT_HEAL_MECHANICAL)
                return true;
        }
        return false;
    }

};

bool compareByHealth(const Unit *u1, const Unit *u2)
{
    return u1->GetHealthPercent() < u2->GetHealthPercent();
}

Unit* PartyMemberToHeal::Calculate()
{

    IsTargetOfHealingSpell predicate;

    Group* group = bot->GetGroup();
    if (!group)
        return NULL;

    vector<Unit*> needHeals;
    for (GroupReference *gref = group->GetFirstMember(); gref; gref = gref->next())
    {
        Player* player = gref->getSource();
        if (!Check(player) || !sServerFacade.IsAlive(player))
            continue;

        uint8 health = player->GetHealthPercent();
        if (health < sPlayerbotAIConfig.almostFullHealth || !IsTargetOfSpellCast(player, predicate))
            needHeals.push_back(player);

        Pet* pet = player->GetPet();
        if (pet && CanHealPet(pet))
        {
            health = pet->GetHealthPercent();
            if (health < sPlayerbotAIConfig.almostFullHealth || !IsTargetOfSpellCast(player, predicate))
                needHeals.push_back(pet);
        }
    }
    if (needHeals.empty())
        return NULL;

    sort(needHeals.begin(), needHeals.end(), compareByHealth);

    int healerIndex = 0;
    for (GroupReference *gref = group->GetFirstMember(); gref; gref = gref->next())
    {
        Player* player = gref->getSource();
        if (!player) continue;
        if (player == bot) break;
        if (ai->IsHeal(player))
        {
            float percent = (float)player->GetPower(POWER_MANA) / (float)player->GetMaxPower(POWER_MANA) * 100.0;
            if (percent > sPlayerbotAIConfig.lowMana)
                healerIndex++;
        }
    }
    healerIndex = healerIndex % needHeals.size();
    return needHeals[healerIndex];
}

bool PartyMemberToHeal::CanHealPet(Pet* pet)
{
    return MINI_PET != pet->getPetType();
}

bool PartyMemberToHeal::Check(Unit* player)
{
    return player && player != bot && player->GetMapId() == bot->GetMapId() &&
        bot->GetDistance(player) < sPlayerbotAIConfig.spellDistance;
}

#pragma once
#include "../Value.h"
#include "PartyMemberValue.h"
#include "../../PlayerbotAIConfig.h"

namespace ai
{
    class PartyMemberWithoutItemValue : public PartyMemberValue, public Qualified
    {
    public:
        PartyMemberWithoutItemValue(PlayerbotAI* ai, float range = sPlayerbotAIConfig.sightDistance) :
          PartyMemberValue(ai) {}

    protected:
        virtual Unit* Calculate();
        virtual FindPlayerPredicate* CreatePredicate();
    };

    class PartyMemberWithoutFoodValue : public PartyMemberWithoutItemValue
    {
    public:
        PartyMemberWithoutFoodValue(PlayerbotAI* ai) : PartyMemberWithoutItemValue(ai) {}

    protected:
        virtual FindPlayerPredicate* CreatePredicate();
    };

    class PartyMemberWithoutWaterValue : public PartyMemberWithoutItemValue
    {
    public:
        PartyMemberWithoutWaterValue(PlayerbotAI* ai) : PartyMemberWithoutItemValue(ai) {}

    protected:
        virtual FindPlayerPredicate* CreatePredicate();
    };
}

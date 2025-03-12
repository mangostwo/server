#pragma once
#include "../Value.h"
#include "TargetValue.h"
#include "NearestUnitsValue.h"

namespace ai
{
    class AttackersValue : public ObjectGuidListCalculatedValue
	{
	public:
        AttackersValue(PlayerbotAI* ai) : ObjectGuidListCalculatedValue(ai, "attackers", 2) {}
        list<ObjectGuid> Calculate();

	private:
        void AddAttackersOf(Group* group, set<Unit*>& targets);
        void AddAttackersOf(Player* player, set<Unit*>& targets);
		void RemoveNonThreating(set<Unit*>& targets);

	public:
		static bool IsPossibleTarget(Unit* attacker, Player *bot);
		static bool IsValidTarget(Unit* attacker, Player *bot);
		static bool IsCCed(Unit* attacker);
    };

    class PossibleAdsValue : public BoolCalculatedValue
    {
    public:
        PossibleAdsValue(PlayerbotAI* const ai) : BoolCalculatedValue(ai) {}
        virtual bool Calculate();
    };
}

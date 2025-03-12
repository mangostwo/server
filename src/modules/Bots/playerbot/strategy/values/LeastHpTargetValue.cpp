#include "botpch.h"
#include "../../playerbot.h"
#include "LeastHpTargetValue.h"

#include "AttackersValue.h"
#include "TargetValue.h"

using namespace ai;
using namespace std;

class FindLeastHpTargetStrategy : public FindNonCcTargetStrategy
{
public:
    FindLeastHpTargetStrategy(PlayerbotAI* ai) : FindNonCcTargetStrategy(ai)
    {
        minHealth = 0;
    }

public:
    virtual void CheckAttacker(Unit* attacker, ThreatManager* threatManager)
    {
        Player* bot = ai->GetBot();
        if (IsCcTarget(attacker)) return;

        if (!result || result->GetHealth() > attacker->GetHealth())
            result = attacker;
    }

protected:
    float minHealth;
};


Unit* LeastHpTargetValue::Calculate()
{
    FindLeastHpTargetStrategy strategy(ai);
    return FindTarget(&strategy);
}

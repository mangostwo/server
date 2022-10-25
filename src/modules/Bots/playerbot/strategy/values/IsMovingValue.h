#pragma once
#include "../Value.h"
#include "MotionGenerators/TargetedMovementGenerator.h"

namespace ai
{
    class IsMovingValue : public BoolCalculatedValue, public Qualified
    {
    public:
        IsMovingValue(PlayerbotAI* ai) : BoolCalculatedValue(ai) {}

        virtual bool Calculate()
        {
            Unit* target = AI_VALUE(Unit*, qualifier);
            Unit* chaseTarget;

            if (!target)
            {
                return false;
            }

            switch (target->GetMotionMaster()->GetCurrentMovementGeneratorType())
            {
            case FLEEING_MOTION_TYPE:
                return true;
            case CHASE_MOTION_TYPE:
                if (target->GetTypeId() == TYPEID_PLAYER)
                {
                    chaseTarget = static_cast<ChaseMovementGenerator<Player> const*>(target->GetMotionMaster()->GetCurrent())->GetTarget();
                }
                else
                {
                    chaseTarget = static_cast<ChaseMovementGenerator<Creature> const*>(target->GetMotionMaster()->GetCurrent())->GetTarget();
                }

                if (!chaseTarget)
                {
                    return false;
                }
                Player* chaseTargetPlayer = sObjectMgr.GetPlayer(chaseTarget->GetObjectGuid());
                return chaseTargetPlayer && !ai->IsTank(chaseTargetPlayer);
            }
            return false;
        }
    };

    class IsSwimmingValue : public BoolCalculatedValue, public Qualified
    {
    public:
        IsSwimmingValue(PlayerbotAI* ai) : BoolCalculatedValue(ai) {}

        virtual bool Calculate()
        {
            Unit* target = AI_VALUE(Unit*, qualifier);

            if (!target)
            {
                return false;
            }

            return target->IsUnderWater() || target->IsInWater();
        }
    };
}

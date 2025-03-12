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
                return false;

            return sServerFacade.isMoving(target);
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
                return false;

            return sServerFacade.IsUnderwater(target) || sServerFacade.IsInWater(target);
        }
    };
}

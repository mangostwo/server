#pragma once
#include "../Trigger.h"
#include "../../PlayerbotAIConfig.h"
#include "../../ServerFacade.h"

namespace ai
{
    class NoRtiTrigger : public Trigger
    {
    public:
        NoRtiTrigger(PlayerbotAI* ai) : Trigger(ai, "no rti target") {}

        virtual bool IsActive()
		{
			return !AI_VALUE(Unit*, "rti target");
        }
    };
}

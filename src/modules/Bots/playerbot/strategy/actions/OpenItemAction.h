#pragma once

#include "../Action.h"

namespace ai
{
	class OpenItemAction : public Action {
	public:
		OpenItemAction(PlayerbotAI* ai) : Action(ai, "open") {}

    public:
        virtual bool Execute(Event event);

    };
}

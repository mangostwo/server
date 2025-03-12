#pragma once

#include "../Action.h"
#include "MovementActions.h"
#include "../values/LastMovementValue.h"
#include "UseItemAction.h"

namespace ai
{
    class CheckMountStateAction : public UseItemAction {
    public:
        CheckMountStateAction(PlayerbotAI* ai) : UseItemAction(ai, "check mount state") {}

        virtual bool Execute(Event event);
        virtual bool isPossible() { return true; }

    private:
        bool Mount();
    };

}

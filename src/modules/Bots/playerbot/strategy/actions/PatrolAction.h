#pragma once

#include "../Action.h"
#include "MovementActions.h"
#include "../values/LastMovementValue.h"

namespace ai
{
    class PatrolAction : public MovementAction {
    public:
        PatrolAction(PlayerbotAI* ai) : MovementAction(ai, "patrol") {}

        virtual bool Execute(Event event);
        virtual bool isUseful();

    private:
        void UpdateDestination();
    };

}

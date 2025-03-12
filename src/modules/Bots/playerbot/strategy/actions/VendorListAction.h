#pragma once

#include "../Action.h"
#include "../../LootObjectStack.h"

namespace ai
{
    class VendorListAction : public Action {
    public:
        VendorListAction(PlayerbotAI* ai) : Action(ai, "vl") {}
        virtual bool Execute(Event event);

    private:
        void TellVendorList(string name);
    };

}

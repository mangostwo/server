#pragma once

#include "../Action.h"
#include "InventoryAction.h"

namespace ai
{
    class GiveItemAction : public InventoryAction
    {
    public:
        GiveItemAction(PlayerbotAI* ai, string name, string item) : InventoryAction(ai, name), item(item) {}
        virtual bool Execute(Event event);
        virtual bool isUseful() { return GetTarget() && AI_VALUE2(uint8, "mana", "self target") > sPlayerbotAIConfig.lowMana; }
        virtual Unit* GetTarget();

    protected:
        string item;
    };

    class GiveFoodAction : public GiveItemAction
    {
    public:
        GiveFoodAction(PlayerbotAI* ai) : GiveItemAction(ai, "give food", "conjured food") {}
        virtual Unit* GetTarget();
    };

    class GiveWaterAction : public GiveItemAction
    {
    public:
        GiveWaterAction(PlayerbotAI* ai) : GiveItemAction(ai, "give water", "conjured water") {}
        virtual Unit* GetTarget();
    };

}
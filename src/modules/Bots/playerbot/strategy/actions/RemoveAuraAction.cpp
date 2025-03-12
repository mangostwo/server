#include "botpch.h"
#include "../../playerbot.h"
#include "RemoveAuraAction.h"

#include "../../PlayerbotAIConfig.h"
#include "../../ServerFacade.h"
using namespace ai;

RemoveAuraAction::RemoveAuraAction(PlayerbotAI* ai) : Action(ai, "ra")
{
}

bool RemoveAuraAction::Execute(Event event)
{
    string text = event.getParam();
    ai->RemoveAura(text);
    return true;
}

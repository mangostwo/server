#include "botpch.h"
#include "../../playerbot.h"
#include "ShamanActions.h"

using namespace ai;

bool CastShamanCasterFormAction::Execute(Event event)
{
    ai->RemoveShapeshift();
    return true;
}

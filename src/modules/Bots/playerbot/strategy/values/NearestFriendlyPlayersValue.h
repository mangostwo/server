#pragma once
#include "../Value.h"
#include "NearestUnitsValue.h"
#include "../../PlayerbotAIConfig.h"

namespace ai
{
    class NearestFriendlyPlayersValue : public NearestUnitsValue
	{
	public:
        NearestFriendlyPlayersValue(PlayerbotAI* ai) :
          NearestUnitsValue(ai, "nearest friendly players", sPlayerbotAIConfig.spellDistance) {}

    protected:
        void FindUnits(list<Unit*> &targets);
        virtual bool AcceptUnit(Unit* unit);
	};
}

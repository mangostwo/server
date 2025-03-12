#pragma once
#include "../Value.h"

namespace ai
{
    class ActiveSpellValue : public CalculatedValue<uint32>
	{
	public:
        ActiveSpellValue(PlayerbotAI* ai) : CalculatedValue<uint32>(ai) {}

        virtual uint32 Calculate();
    };
}

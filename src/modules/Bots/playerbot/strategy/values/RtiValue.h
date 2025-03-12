#pragma once
#include "../Value.h"

namespace ai
{
    class RtiValue : public ManualSetValue<string>
	{
	public:
        RtiValue(PlayerbotAI* ai);

        virtual string Save() { return value; }
        virtual bool Load(string text) { value = text; return true; }
	};

    class RtiCcValue : public ManualSetValue<string>
	{
	public:
        RtiCcValue(PlayerbotAI* ai);

        virtual string Save() { return value; }
        virtual bool Load(string text) { value = text; return true; }
	};
}

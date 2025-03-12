#pragma once
#include "../Value.h"

namespace ai
{
    typedef list<string> Outfit;
    class OutfitListValue : public ManualSetValue<Outfit&>
	{
	public:
        OutfitListValue(PlayerbotAI* ai) : ManualSetValue<Outfit&>(ai, list) {}

        virtual string Save();
        virtual bool Load(string value);

    private:
        Outfit list;
    };
}

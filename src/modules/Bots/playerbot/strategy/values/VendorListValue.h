#pragma once
#include "../Value.h"

namespace ai
{
    class VendorListValue : public ManualSetValue<set<uint32>&>
	{
	public:
        VendorListValue(PlayerbotAI* ai, string name) : ManualSetValue<set<uint32>&>(ai, list, name) {}

        virtual string Save();
        virtual bool Load(string value);

    private:
        set<uint32> list;
    };
}

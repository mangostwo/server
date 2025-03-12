#pragma once
#include "Category.h"

using namespace std;

namespace ahbot
{
    class TradeSkill : public Trade
    {
    public:
        TradeSkill(uint32 skill, bool reagent) : Trade(), skill(skill), reagent(reagent), rebuildRequired(false) {}

    public:
        virtual bool Contains(ItemPrototype const* proto);
        virtual string GetName();
        virtual string GetLabel();
        virtual uint32 GetSkillId() { return skill; }
        virtual void LoadCache();
        virtual void SaveCache();

    private:
        bool ContainsInternal(ItemPrototype const* proto);
        bool IsCraftedBySpell(ItemPrototype const* proto, uint32 spellId);
        bool IsCraftedBy(ItemPrototype const* proto, uint32 craftId);
        bool IsCraftedBySpell(ItemPrototype const* proto, SpellEntry const *entry);
        uint32 skill;
        map<uint32, bool> itemCache;
        bool reagent;
        bool rebuildRequired;
    };

};

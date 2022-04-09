#pragma once
#include "../Trigger.h"

namespace ai
{
    class SpellTrigger;

    class NeedCureTrigger : public SpellTrigger {
    public:
        NeedCureTrigger(PlayerbotAI* ai, const string &spell, uint32 dispelType) : SpellTrigger(ai, spell)
          {
            this->dispelType = dispelType;
        }
        virtual string GetTargetName() { return "self target"; }
        virtual bool IsActive();

    protected:
        uint32 dispelType;
    };

    class TargetAuraDispelTrigger : public NeedCureTrigger {
    public:
        TargetAuraDispelTrigger(PlayerbotAI* ai, const string &spell, uint32 dispelType) :
            NeedCureTrigger(ai, spell, dispelType) {}
        virtual string GetTargetName() { return "current target"; }
    };

    class PartyMemberNeedCureTrigger : public NeedCureTrigger {
    public:
        PartyMemberNeedCureTrigger(PlayerbotAI* ai, const string &spell, uint32 dispelType) :
            NeedCureTrigger(ai, spell, dispelType) {}

        virtual Value<Unit*>* GetTargetValue();
    };
}

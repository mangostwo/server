#pragma once
#include "../Value.h"
#include "../../PlayerbotAIConfig.h"
#include "Formations.h"

namespace ai
{
    class Stance : public Formation
    {
    public:
        Stance(PlayerbotAI* ai, string name) : Formation (ai, name) {}

    protected:
        virtual Unit* GetTarget();
        virtual WorldLocation GetLocationInternal() = 0;
        virtual WorldLocation GetNearLocation(float angle, float distance);

    public:
        virtual WorldLocation GetLocation();
        virtual string GetTargetName() { return "current target"; }
        virtual float GetMaxDistance() { return sPlayerbotAIConfig.contactDistance; }
    };

    class MoveStance : public Stance
    {
    public:
        MoveStance(PlayerbotAI* ai, string name) : Stance (ai, name) {}

    protected:
        virtual WorldLocation GetLocationInternal();
        virtual float GetAngle() = 0;

    public:
    };


    class StanceValue : public ManualSetValue<Stance*>
	{
	public:
        StanceValue(PlayerbotAI* ai);
        ~StanceValue() { if (value) { delete value; value = NULL; } }
        virtual string Save();
        virtual bool Load(string value);
    };

    class SetStanceAction : public Action
    {
    public:
        SetStanceAction(PlayerbotAI* ai) : Action(ai, "set Stance") {}
        virtual bool Execute(Event event);
    };
};


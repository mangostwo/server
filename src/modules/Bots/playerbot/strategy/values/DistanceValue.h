#pragma once
#include "../Value.h"
#include "TargetValue.h"
#include "../../LootObjectStack.h"
#include "../../ServerFacade.h"
#include "PositionValue.h"

namespace ai
{
    class DistanceValue : public FloatCalculatedValue, public Qualified
	{
	public:
        DistanceValue(PlayerbotAI* ai) : FloatCalculatedValue(ai) {}

    public:
        float Calculate()
        {
            if (qualifier == "loot target")
            {
                LootObject loot = AI_VALUE(LootObject, qualifier);
                if (loot.IsEmpty())
                    return 0.0f;

                WorldObject* obj = loot.GetWorldObject(bot);
                if (!obj)
                    return 0.0f;

                return sServerFacade.GetDistance2d(ai->GetBot(), obj);
            }

            if (qualifier.find("position_") == 0)
            {
                string position = qualifier.substr(9);
                ai::Position pos = context->GetValue<ai::PositionMap&>("position")->Get()[position];
                if (!pos.isSet()) return 0.0f;
                if (ai->GetBot()->GetMapId() != pos.mapId) return 0.0f;
                return sServerFacade.GetDistance2d(ai->GetBot(), pos.x, pos.y);
            }

            Unit* target = NULL;
            if (qualifier == "rpg target")
            {
                ObjectGuid rpgTarget = AI_VALUE(ObjectGuid, qualifier);
                target = ai->GetUnit(rpgTarget);
            }
            else if (qualifier == "current target")
            {
                Stance* stance = AI_VALUE(Stance*, "stance");
                WorldLocation loc = stance->GetLocation();
                return sServerFacade.GetDistance2d(ai->GetBot(), loc.coord_x, loc.coord_y);
            }
            else
            {
                target = AI_VALUE(Unit*, qualifier);
                if (target && target == GetMaster())
                {
                    Formation* formation = AI_VALUE(Formation*, "formation");
                    WorldLocation loc = formation->GetLocation();
                    return sServerFacade.GetDistance2d(ai->GetBot(), loc.coord_x, loc.coord_y);
                }
            }

            if (!target || !target->IsInWorld())
                return 0.0f;

            if (target == ai->GetBot())
                return 0.0f;

            return sServerFacade.GetDistance2d(ai->GetBot(), target);
        }
    };

    class InsideTargetValue : public BoolCalculatedValue, public Qualified
	{
	public:
        InsideTargetValue(PlayerbotAI* ai) : BoolCalculatedValue(ai) {}

    public:
        bool Calculate()
        {
            Unit* target = AI_VALUE(Unit*, qualifier);

            if (!target || !target->IsInWorld() || target == ai->GetBot())
                return false;

            float dist = sServerFacade.GetDistance2d(ai->GetBot(), target->GetPositionX(), target->GetPositionY());
            return sServerFacade.IsDistanceLessThan(dist, target->GetObjectBoundingRadius());
        }
    };
}

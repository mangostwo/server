#pragma once

#include "../Action.h"
#include "ChooseTargetActions.h"

#include "../../ServerFacade.h"
#include "MovementGenerator.h"
#include "CreatureAI.h"

bool DropTargetAction::Execute(Event event)
{
    Unit* target = context->GetValue<Unit*>("current target")->Get();
    ObjectGuid pullTarget = context->GetValue<ObjectGuid>("pull target")->Get();
    list<ObjectGuid> possible = ai->GetAiObjectContext()->GetValue<list<ObjectGuid> >("possible targets")->Get();
    if (pullTarget && find(possible.begin(), possible.end(), pullTarget) == possible.end())
    {
        context->GetValue<ObjectGuid>("pull target")->Set(ObjectGuid());
    }
    context->GetValue<Unit*>("current target")->Set(NULL);
    bot->SetSelectionGuid(ObjectGuid());
    ai->ChangeEngine(BOT_STATE_NON_COMBAT);
    ai->InterruptSpell();
    bot->AttackStop();
    Pet* pet = bot->GetPet();
    if (pet)
    {
#ifdef MANGOS
        CreatureAI*
#endif
#ifdef CMANGOS
        UnitAI*
#endif
        creatureAI = ((Creature*)pet)->AI();
        if (creatureAI)
        {
#ifdef CMANGOS
            creatureAI->SetReactState(REACT_PASSIVE);
#endif
#ifdef MANGOS
            pet->GetCharmInfo()->SetReactState(REACT_PASSIVE);
            pet->GetCharmInfo()->SetCommandState(COMMAND_FOLLOW);
#endif
            pet->AttackStop();
        }
    }
    if (!urand(0, 25))
    {
        vector<uint32> sounds;
        if (target && sServerFacade.UnitIsDead(target))
        {
            sounds.push_back(TEXTEMOTE_CHEER);
            sounds.push_back(TEXTEMOTE_CONGRATULATE);
        }
        else
        {
            sounds.push_back(304); // guard
            sounds.push_back(325); // stay
        }
        if (!sounds.empty()) ai->PlaySound(sounds[urand(0, sounds.size() - 1)]);
    }
    return true;
}


bool AttackAnythingAction::Execute(Event event)
{
    bool result = AttackAction::Execute(event);
    if (result && GetTarget()) context->GetValue<ObjectGuid>("pull target")->Set(GetTarget()->GetObjectGuid());
    return result;
}

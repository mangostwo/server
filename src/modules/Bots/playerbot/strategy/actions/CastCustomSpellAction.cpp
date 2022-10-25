#include "botpch.h"
#include "../../playerbot.h"
#include "CastCustomSpellAction.h"

#include "../../PlayerbotAIConfig.h"
using namespace ai;

bool CastCustomSpellAction::Execute(Event event)
{
    Unit* target = NULL;

    Player* master = GetMaster();
    if (master && master->GetSelectionGuid())
    {
        target = ai->GetUnit(master->GetSelectionGuid());
    }

    if (!target)
    {
        target = bot;
    }

    string text = event.getParam();
    int pos = text.find_last_of(" ");
    int castCount = 1;
    if (pos != string::npos)
    {
        castCount = atoi(text.substr(pos + 1).c_str());
        if (castCount > 0)
        {
            text = text.substr(0, pos);
        }
    }

    uint32 spell = AI_VALUE2(uint32, "spell id", text);

    ostringstream msg;
    if (!spell)
    {
        msg << "Unknown spell " << text;
        ai->TellMaster(msg.str());
        return false;
    }

    SpellEntry const *pSpellInfo = sSpellStore.LookupEntry(spell);
    if (!pSpellInfo)
    {
        msg << "Unknown spell " << text;
        ai->TellMaster(msg.str());
        return false;
    }

    if (target != bot && !bot->IsInFront(target, sPlayerbotAIConfig.sightDistance, CAST_ANGLE_IN_FRONT))
    {
        bot->SetFacingTo(bot->GetAngle(target));
        ai->SetNextCheckDelay(sPlayerbotAIConfig.globalCoolDown);
        msg << "cast " << text;
        ai->HandleCommand(CHAT_MSG_WHISPER, msg.str(), *master);
        return true;
    }


    if (!ai->CanCastSpell(spell, target))
    {
        msg << "Cannot cast " << ChatHelper::formatSpell(pSpellInfo) << " on " << target->GetName();
        ai->TellMaster(msg.str());
        return false;
    }

    bool result = spell ? ai->CastSpell(spell, target) : ai->CastSpell(text, target);
    if (result)
    {
        msg << "Casting " << ChatHelper::formatSpell(pSpellInfo);
        if (target != bot)
        {
            msg << " on " << target->GetName();
        }

        if (castCount > 1)
        {
            ostringstream cmd;
            cmd << "cast " << text << " " << (castCount - 1);
            ai->HandleCommand(CHAT_MSG_WHISPER, cmd.str(), *master);
            msg << "|cffffff00(x" << (castCount-1) << " left)|r";
        }
        ai->TellMasterNoFacing(msg.str());
    }
    else
    {
        msg << "Cast " << ChatHelper::formatSpell(pSpellInfo) << " on " << target->GetName() << " is failed";
        ai->TellMaster(msg.str());
    }

    return result;
}

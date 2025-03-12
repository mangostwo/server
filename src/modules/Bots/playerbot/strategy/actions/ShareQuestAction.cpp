#include "botpch.h"
#include "../../playerbot.h"
#include "ShareQuestAction.h"


using namespace ai;

bool ShareQuestAction::Execute(Event event)
{
    string link = event.getParam();
    if (!GetMaster())
        return false;

    PlayerbotChatHandler handler(GetMaster());
    uint32 entry = handler.extractQuestId(link);

    // remove all quest entries for 'entry' from quest log
    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 logQuest = bot->GetQuestSlotQuestId(slot);
        Quest const* quest = sObjectMgr.GetQuestTemplate(logQuest);
        if (!quest)
            continue;

        if (logQuest == entry || link.find(quest->GetTitle()) != string::npos)
        {
            WorldPacket p;
            p << logQuest;
            bot->GetSession()->HandlePushQuestToParty(p);
            ai->TellMaster("Quest shared");
            return true;
        }
    }

    return false;
}

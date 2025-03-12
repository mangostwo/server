#include "botpch.h"
#include "../../playerbot.h"
#include "SayAction.h"

using namespace ai;

map<string, vector<string> > SayAction::stringTable;
map<string, uint32 > SayAction::probabilityTable;

SayAction::SayAction(PlayerbotAI* ai) : Action(ai, "say"), Qualified()
{
}

void replaceAll(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

bool SayAction::Execute(Event event)
{
	if (stringTable.empty())
	{
		QueryResult* results = CharacterDatabase.PQuery("SELECT name, text, type FROM ai_playerbot_speech");
		if (results)
		{
            do
            {
                Field* fields = results->Fetch();
                string name = fields[0].GetString();
                string text = fields[1].GetString();
                string type = fields[2].GetString();

                if (type == "yell") text = "/y " + text;
                stringTable[name].push_back(text);
            } while (results->NextRow());
			delete results;
		}
	}
	if (probabilityTable.empty())
	{
        QueryResult* results = CharacterDatabase.PQuery("SELECT name, probability FROM ai_playerbot_speech_probability");
        if (results)
        {
            do
            {
                Field* fields = results->Fetch();
                string name = fields[0].GetString();
                uint32 probability = fields[1].GetUInt32();

                probabilityTable[name] = probability;
            } while (results->NextRow());
			delete results;
		}
	}

    vector<string> &strings = stringTable[qualifier];
    if (strings.empty()) return false;

    time_t lastSaid = AI_VALUE2(time_t, "last said", qualifier);
    uint32 nextTime = time(0) + urand(1, 30);
    ai->GetAiObjectContext()->GetValue<time_t>("last said", qualifier)->Set(nextTime);

    Group* group = bot->GetGroup();
    if (group)
    {
        vector<Player*> members;
        for (GroupReference *ref = group->GetFirstMember(); ref; ref = ref->next())
        {
            Player *member = ref->getSource();
            PlayerbotAI* memberAi = member->GetPlayerbotAI();
            if (memberAi) members.push_back(member);
        }

        uint32 count = members.size();
        if (count > 1)
        {
            for (uint32 i = 0; i < count * 5; i++)
            {
                int i1 = urand(0, count - 1);
                int i2 = urand(0, count - 1);

                Player* item = members[i1];
                members[i1] = members[i2];
                members[i2] = item;
            }
        }

        int index = 0;
        for (vector<Player*>::iterator i = members.begin(); i != members.end(); ++i)
        {
            PlayerbotAI* memberAi = (*i)->GetPlayerbotAI();
            memberAi->GetAiObjectContext()->GetValue<time_t>("last said", qualifier)->Set(nextTime + (20 * ++index) + urand(1, 15));
        }
    }

    uint32 probability = probabilityTable[qualifier];
    if (!probability) probability = 100;
    if (urand(0, 100) >= probability) return false;

    uint32 idx = urand(0, strings.size() - 1);
    string text = strings[idx];

    Unit* target = AI_VALUE(Unit*, "tank target");
    if (!target) target = AI_VALUE(Unit*, "current target");
    if (target) replaceAll(text, "<target>", target->GetName());

    replaceAll(text, "<randomfaction>", IsAlliance(bot->getRace()) ? "Alliance" : "Horde");

    if (bot->GetMap())
    {
        if (AreaTableEntry const* area = GetAreaEntryByAreaID(bot->GetAreaId()))
            replaceAll(text, "<subzone>", area->area_name[0]);
    }

    if (text.find("/y ") == 0)
        bot->Yell(text.substr(3), LANG_UNIVERSAL);
    else
        bot->Say(text, LANG_UNIVERSAL);

    return true;
}


bool SayAction::isUseful()
{
    time_t lastSaid = AI_VALUE2(time_t, "last said", qualifier);
    return (time(0) - lastSaid) > 30;
}

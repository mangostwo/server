#include "botpch.h"
#include "../../playerbot.h"
#include "EmoteAction.h"

#include "../../PlayerbotAIConfig.h"
using namespace ai;

map<string, uint32> EmoteAction::emotes;
map<string, uint32> EmoteAction::textEmotes;

bool EmoteAction::Execute(Event event)
{
    if (emotes.empty())
    {
        InitEmotes();
    }

    uint32 emote = 0;

    time_t lastEmote = AI_VALUE2(time_t, "last emote", qualifier);
    ai->GetAiObjectContext()->GetValue<time_t>("last emote", qualifier)->Set(time(0) + urand(1, 60));

    string param = event.getParam();
    if (param.empty()) param = qualifier;

    if (!param.empty() && textEmotes.find(param) != emotes.end())
    {
        return ai->PlaySound(textEmotes[param]);
    }
    else if (param.empty() || emotes.find(param) == emotes.end())
    {
        int index = rand() % emotes.size();
        for (map<string, uint32>::iterator i = emotes.begin(); i != emotes.end() && index; ++i, --index)
  {
      emote = i->second;
  }
    }
    else
    {
        emote = emotes[param];
    }

    Player* master = GetMaster();
    if (master)
    {
        ObjectGuid masterSelection = master->GetSelectionGuid();
        if (masterSelection)
        {
            ObjectGuid oldSelection = bot->GetSelectionGuid();
            bot->SetSelectionGuid(masterSelection);
            bot->HandleEmoteCommand(emote);
            if (oldSelection)
            {
                bot->SetSelectionGuid(oldSelection);
            }
            return true;
        }
    }

    bot->HandleEmoteCommand(emote);
    return true;
}

void EmoteAction::InitEmotes()
{
    emotes["dance"] = EMOTE_ONESHOT_DANCE;
    emotes["drown"] = EMOTE_ONESHOT_DROWN;
    emotes["land"] = EMOTE_ONESHOT_LAND;
    emotes["liftoff"] = EMOTE_ONESHOT_LIFTOFF;
    emotes["loot"] = EMOTE_ONESHOT_LOOT;
    emotes["no"] = EMOTE_ONESHOT_NO;
    emotes["roar"] = EMOTE_STATE_ROAR;
    emotes["salute"] = EMOTE_ONESHOT_SALUTE;
    emotes["stomp"] = EMOTE_ONESHOT_STOMP;
    emotes["train"] = EMOTE_ONESHOT_TRAIN;
    emotes["yes"] = EMOTE_ONESHOT_YES;
    emotes["applaud"] = EMOTE_ONESHOT_APPLAUD;
    emotes["beg"] = EMOTE_ONESHOT_BEG;
    emotes["bow"] = EMOTE_ONESHOT_BOW;
    emotes["cheer"] = EMOTE_ONESHOT_CHEER;
    emotes["chicken"] = EMOTE_ONESHOT_CHICKEN;
    emotes["cry"] = EMOTE_ONESHOT_CRY;
    emotes["dance"] = EMOTE_STATE_DANCE;
    emotes["eat"] = EMOTE_ONESHOT_EAT;
    emotes["exclamation"] = EMOTE_ONESHOT_EXCLAMATION;
    emotes["flex"] = EMOTE_ONESHOT_FLEX;
    emotes["kick"] = EMOTE_ONESHOT_KICK;
    emotes["kiss"] = EMOTE_ONESHOT_KISS;
    emotes["kneel"] = EMOTE_ONESHOT_KNEEL;
    emotes["laugh"] = EMOTE_ONESHOT_LAUGH;
    emotes["point"] = EMOTE_ONESHOT_POINT;
    emotes["question"] = EMOTE_ONESHOT_QUESTION;
    emotes["ready1h"] = EMOTE_ONESHOT_READY1H;
    emotes["roar"] = EMOTE_ONESHOT_ROAR;
    emotes["rude"] = EMOTE_ONESHOT_RUDE;
    emotes["shout"] = EMOTE_ONESHOT_SHOUT;
    emotes["shy"] = EMOTE_ONESHOT_SHY;
    emotes["sleep"] = EMOTE_STATE_SLEEP;
    emotes["talk"] = EMOTE_ONESHOT_TALK;
    emotes["wave"] = EMOTE_ONESHOT_WAVE;
    emotes["wound"] = EMOTE_ONESHOT_WOUND;

    textEmotes["agree"] = TEXTEMOTE_AGREE;
    textEmotes["amaze"] = TEXTEMOTE_AMAZE;
    textEmotes["angry"] = TEXTEMOTE_ANGRY;
    textEmotes["apologize"] = TEXTEMOTE_APOLOGIZE;
    textEmotes["applaud"] = TEXTEMOTE_APPLAUD;
    textEmotes["bashful"] = TEXTEMOTE_BASHFUL;
    textEmotes["beckon"] = TEXTEMOTE_BECKON;
    textEmotes["beg"] = TEXTEMOTE_BEG;
    textEmotes["bite"] = TEXTEMOTE_BITE;
    textEmotes["bleed"] = TEXTEMOTE_BLEED;
    textEmotes["blink"] = TEXTEMOTE_BLINK;
    textEmotes["blush"] = TEXTEMOTE_BLUSH;
    textEmotes["bonk"] = TEXTEMOTE_BONK;
    textEmotes["bored"] = TEXTEMOTE_BORED;
    textEmotes["bounce"] = TEXTEMOTE_BOUNCE;
    textEmotes["brb"] = TEXTEMOTE_BRB;
    textEmotes["bow"] = TEXTEMOTE_BOW;
    textEmotes["burp"] = TEXTEMOTE_BURP;
    textEmotes["bye"] = TEXTEMOTE_BYE;
    textEmotes["cackle"] = TEXTEMOTE_CACKLE;
    textEmotes["cheer"] = TEXTEMOTE_CHEER;
    textEmotes["chicken"] = TEXTEMOTE_CHICKEN;
    textEmotes["chuckle"] = TEXTEMOTE_CHUCKLE;
    textEmotes["clap"] = TEXTEMOTE_CLAP;
    textEmotes["confused"] = TEXTEMOTE_CONFUSED;
    textEmotes["congratulate"] = TEXTEMOTE_CONGRATULATE;
    textEmotes["cough"] = TEXTEMOTE_COUGH;
    textEmotes["cower"] = TEXTEMOTE_COWER;
    textEmotes["crack"] = TEXTEMOTE_CRACK;
    textEmotes["cringe"] = TEXTEMOTE_CRINGE;
    textEmotes["cry"] = TEXTEMOTE_CRY;
    textEmotes["curious"] = TEXTEMOTE_CURIOUS;
    textEmotes["curtsey"] = TEXTEMOTE_CURTSEY;
    textEmotes["dance"] = TEXTEMOTE_DANCE;
    textEmotes["drink"] = TEXTEMOTE_DRINK;
    textEmotes["drool"] = TEXTEMOTE_DROOL;
    textEmotes["eat"] = TEXTEMOTE_EAT;
    textEmotes["eye"] = TEXTEMOTE_EYE;
    textEmotes["fart"] = TEXTEMOTE_FART;
    textEmotes["fidget"] = TEXTEMOTE_FIDGET;
    textEmotes["flex"] = TEXTEMOTE_FLEX;
    textEmotes["frown"] = TEXTEMOTE_FROWN;
    textEmotes["gasp"] = TEXTEMOTE_GASP;
    textEmotes["gaze"] = TEXTEMOTE_GAZE;
    textEmotes["giggle"] = TEXTEMOTE_GIGGLE;
    textEmotes["glare"] = TEXTEMOTE_GLARE;
    textEmotes["gloat"] = TEXTEMOTE_GLOAT;
    textEmotes["greet"] = TEXTEMOTE_GREET;
    textEmotes["grin"] = TEXTEMOTE_GRIN;
    textEmotes["groan"] = TEXTEMOTE_GROAN;
    textEmotes["grovel"] = TEXTEMOTE_GROVEL;
    textEmotes["guffaw"] = TEXTEMOTE_GUFFAW;
    textEmotes["hail"] = TEXTEMOTE_HAIL;
    textEmotes["happy"] = TEXTEMOTE_HAPPY;
    textEmotes["hello"] = TEXTEMOTE_HELLO;
    textEmotes["hug"] = TEXTEMOTE_HUG;
    textEmotes["hungry"] = TEXTEMOTE_HUNGRY;
    textEmotes["kiss"] = TEXTEMOTE_KISS;
    textEmotes["kneel"] = TEXTEMOTE_KNEEL;
    textEmotes["laugh"] = TEXTEMOTE_LAUGH;
    textEmotes["laydown"] = TEXTEMOTE_LAYDOWN;
    textEmotes["message"] = TEXTEMOTE_MESSAGE;
    textEmotes["moan"] = TEXTEMOTE_MOAN;
    textEmotes["moon"] = TEXTEMOTE_MOON;
    textEmotes["mourn"] = TEXTEMOTE_MOURN;
    textEmotes["no"] = TEXTEMOTE_NO;
    textEmotes["nod"] = TEXTEMOTE_NOD;
    textEmotes["nosepick"] = TEXTEMOTE_NOSEPICK;
    textEmotes["panic"] = TEXTEMOTE_PANIC;
    textEmotes["peer"] = TEXTEMOTE_PEER;
    textEmotes["plead"] = TEXTEMOTE_PLEAD;
    textEmotes["point"] = TEXTEMOTE_POINT;
    textEmotes["poke"] = TEXTEMOTE_POKE;
    textEmotes["pray"] = TEXTEMOTE_PRAY;
    textEmotes["roar"] = TEXTEMOTE_ROAR;
    textEmotes["rofl"] = TEXTEMOTE_ROFL;
    textEmotes["rude"] = TEXTEMOTE_RUDE;
    textEmotes["salute"] = TEXTEMOTE_SALUTE;
    textEmotes["scratch"] = TEXTEMOTE_SCRATCH;
    textEmotes["sexy"] = TEXTEMOTE_SEXY;
    textEmotes["shake"] = TEXTEMOTE_SHAKE;
    textEmotes["shout"] = TEXTEMOTE_SHOUT;
    textEmotes["shrug"] = TEXTEMOTE_SHRUG;
    textEmotes["shy"] = TEXTEMOTE_SHY;
    textEmotes["sigh"] = TEXTEMOTE_SIGH;
    textEmotes["sit"] = TEXTEMOTE_SIT;
    textEmotes["sleep"] = TEXTEMOTE_SLEEP;
    textEmotes["snarl"] = TEXTEMOTE_SNARL;
    textEmotes["spit"] = TEXTEMOTE_SPIT;
    textEmotes["stare"] = TEXTEMOTE_STARE;
    textEmotes["surprised"] = TEXTEMOTE_SURPRISED;
    textEmotes["surrender"] = TEXTEMOTE_SURRENDER;
    textEmotes["talk"] = TEXTEMOTE_TALK;
    textEmotes["talkex"] = TEXTEMOTE_TALKEX;
    textEmotes["talkq"] = TEXTEMOTE_TALKQ;
    textEmotes["tap"] = TEXTEMOTE_TAP;
    textEmotes["thank"] = TEXTEMOTE_THANK;
    textEmotes["threaten"] = TEXTEMOTE_THREATEN;
    textEmotes["tired"] = TEXTEMOTE_TIRED;
    textEmotes["victory"] = TEXTEMOTE_VICTORY;
    textEmotes["wave"] = TEXTEMOTE_WAVE;
    textEmotes["welcome"] = TEXTEMOTE_WELCOME;
    textEmotes["whine"] = TEXTEMOTE_WHINE;
    textEmotes["whistle"] = TEXTEMOTE_WHISTLE;
    textEmotes["work"] = TEXTEMOTE_WORK;
    textEmotes["yawn"] = TEXTEMOTE_YAWN;
    textEmotes["boggle"] = TEXTEMOTE_BOGGLE;
    textEmotes["calm"] = TEXTEMOTE_CALM;
    textEmotes["cold"] = TEXTEMOTE_COLD;
    textEmotes["comfort"] = TEXTEMOTE_COMFORT;
    textEmotes["cuddle"] = TEXTEMOTE_CUDDLE;
    textEmotes["duck"] = TEXTEMOTE_DUCK;
    textEmotes["insult"] = TEXTEMOTE_INSULT;
    textEmotes["introduce"] = TEXTEMOTE_INTRODUCE;
    textEmotes["jk"] = TEXTEMOTE_JK;
    textEmotes["lick"] = TEXTEMOTE_LICK;
    textEmotes["listen"] = TEXTEMOTE_LISTEN;
    textEmotes["lost"] = TEXTEMOTE_LOST;
    textEmotes["mock"] = TEXTEMOTE_MOCK;
    textEmotes["ponder"] = TEXTEMOTE_PONDER;
    textEmotes["pounce"] = TEXTEMOTE_POUNCE;
    textEmotes["praise"] = TEXTEMOTE_PRAISE;
    textEmotes["purr"] = TEXTEMOTE_PURR;
    textEmotes["puzzle"] = TEXTEMOTE_PUZZLE;
    textEmotes["raise"] = TEXTEMOTE_RAISE;
    textEmotes["ready"] = TEXTEMOTE_READY;
    textEmotes["shimmy"] = TEXTEMOTE_SHIMMY;
    textEmotes["shiver"] = TEXTEMOTE_SHIVER;
    textEmotes["shoo"] = TEXTEMOTE_SHOO;
    textEmotes["slap"] = TEXTEMOTE_SLAP;
    textEmotes["smirk"] = TEXTEMOTE_SMIRK;
    textEmotes["sniff"] = TEXTEMOTE_SNIFF;
    textEmotes["snub"] = TEXTEMOTE_SNUB;
    textEmotes["soothe"] = TEXTEMOTE_SOOTHE;
    textEmotes["stink"] = TEXTEMOTE_STINK;
    textEmotes["taunt"] = TEXTEMOTE_TAUNT;
    textEmotes["tease"] = TEXTEMOTE_TEASE;
    textEmotes["thirsty"] = TEXTEMOTE_THIRSTY;
    textEmotes["veto"] = TEXTEMOTE_VETO;
    textEmotes["snicker"] = TEXTEMOTE_SNICKER;
    textEmotes["stand"] = TEXTEMOTE_STAND;
    textEmotes["tickle"] = TEXTEMOTE_TICKLE;
    textEmotes["violin"] = TEXTEMOTE_VIOLIN;
    textEmotes["smile"] = TEXTEMOTE_SMILE;
    textEmotes["rasp"] = TEXTEMOTE_RASP;
    textEmotes["pity"] = TEXTEMOTE_PITY;
    textEmotes["growl"] = TEXTEMOTE_GROWL;
    textEmotes["bark"] = TEXTEMOTE_BARK;
    textEmotes["scared"] = TEXTEMOTE_SCARED;
    textEmotes["flop"] = TEXTEMOTE_FLOP;
    textEmotes["love"] = TEXTEMOTE_LOVE;
    textEmotes["moo"] = TEXTEMOTE_MOO;
    textEmotes["commend"] = TEXTEMOTE_COMMEND;
    textEmotes["openfire"] = TEXTEMOTE_OPENFIRE;
    textEmotes["flirt"] = TEXTEMOTE_FLIRT;
    textEmotes["joke"] = TEXTEMOTE_JOKE;
    textEmotes["wink"] = TEXTEMOTE_WINK;
    textEmotes["pat"] = TEXTEMOTE_PAT;
    textEmotes["serious"] = TEXTEMOTE_SERIOUS;
    textEmotes["goodluck"] = TEXTEMOTE_GOODLUCK;
    textEmotes["blame"] = TEXTEMOTE_BLAME;
    textEmotes["blank"] = TEXTEMOTE_BLANK;
    textEmotes["brandish"] = TEXTEMOTE_BRANDISH;
    textEmotes["breath"] = TEXTEMOTE_BREATH;
    textEmotes["disagree"] = TEXTEMOTE_DISAGREE;
    textEmotes["doubt"] = TEXTEMOTE_DOUBT;
    textEmotes["embarrass"] = TEXTEMOTE_EMBARRASS;
    textEmotes["encourage"] = TEXTEMOTE_ENCOURAGE;
    textEmotes["enemy"] = TEXTEMOTE_ENEMY;
    textEmotes["eyebrow"] = TEXTEMOTE_EYEBROW;
    textEmotes["toast"] = TEXTEMOTE_TOAST;
    textEmotes["oom"] = 323;
    textEmotes["follow"] = 324;
    textEmotes["wait"] = 325;
    textEmotes["healme"] = 326;
    textEmotes["openfire"] = 327;
    textEmotes["helpme"] = 303;
}


bool EmoteAction::isUseful()
{
    time_t lastEmote = AI_VALUE2(time_t, "last emote", qualifier);
    return (time(0) - lastEmote) >= sPlayerbotAIConfig.repeatDelay / 1000;
}

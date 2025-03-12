#include "botpch.h"
#include "../../playerbot.h"
#include "PartyMemberWithoutItemValue.h"

#include "../../ServerFacade.h"
using namespace ai;

class PlayerWithoutItemPredicate : public FindPlayerPredicate, public PlayerbotAIAware
{
public:
    PlayerWithoutItemPredicate(PlayerbotAI* ai, string item) :
        PlayerbotAIAware(ai), FindPlayerPredicate(), item(item) {}

public:
    virtual bool Check(Unit* unit)
    {
        Pet* pet = dynamic_cast<Pet*>(unit);
        if (pet && (pet->getPetType() == MINI_PET || pet->getPetType() == SUMMON_PET))
            return false;

        if (!sServerFacade.IsAlive(unit))
            return false;

        Player* member = dynamic_cast<Player*>(unit);
        if (!member)
            return false;

        PlayerbotAI *memberAi = member->GetPlayerbotAI();
        if (!memberAi)
            return false;

        Player* bot = ai->GetBot();
        bool sameGroup = member->GetGroup() && bot->GetGroup() && member->GetGroup() == bot->GetGroup();
        if (abs((int32)member->getLevel() - (int32)bot->getLevel()) >= 3 && !sameGroup)
            return false;

        return !memberAi->GetAiObjectContext()->GetValue<uint8>("item count", item)->Get();
    }

private:
    string item;
};

Unit* PartyMemberWithoutItemValue::Calculate()
{
    FindPlayerPredicate *predicate = CreatePredicate();
    Unit *unit = FindPartyMember(*predicate);
    delete predicate;
    return unit;
}

FindPlayerPredicate* PartyMemberWithoutItemValue::CreatePredicate()
{
    return new PlayerWithoutItemPredicate(ai, qualifier);
}

class PlayerWithoutFoodPredicate : public PlayerWithoutItemPredicate
{
public:
    PlayerWithoutFoodPredicate(PlayerbotAI* ai) : PlayerWithoutItemPredicate(ai, "conjured food") {}

public:
    virtual bool Check(Unit* unit)
    {
        if (!PlayerWithoutItemPredicate::Check(unit))
            return false;

        Player* member = dynamic_cast<Player*>(unit);
        if (!member)
            return false;

        return member->getClass() != CLASS_MAGE;
    }

};

class PlayerWithoutWaterPredicate : public PlayerWithoutItemPredicate
{
public:
    PlayerWithoutWaterPredicate(PlayerbotAI* ai) : PlayerWithoutItemPredicate(ai, "conjured water") {}

public:
    virtual bool Check(Unit* unit)
    {
        if (!PlayerWithoutItemPredicate::Check(unit))
            return false;

        Player* member = dynamic_cast<Player*>(unit);
        if (!member)
            return false;

        uint8 cls = member->getClass();
        return cls == CLASS_DRUID ||
                cls == CLASS_HUNTER ||
                cls == CLASS_PALADIN ||
                cls == CLASS_PRIEST ||
                cls == CLASS_SHAMAN ||
                cls == CLASS_WARLOCK;
    }

};

FindPlayerPredicate* PartyMemberWithoutFoodValue::CreatePredicate()
{
    return new PlayerWithoutFoodPredicate(ai);
}

FindPlayerPredicate* PartyMemberWithoutWaterValue::CreatePredicate()
{
    return new PlayerWithoutWaterPredicate(ai);
}
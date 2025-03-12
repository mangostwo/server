#include "botpch.h"
#include "../../playerbot.h"
#include "UseItemAction.h"

#include "../../PlayerbotAIConfig.h"
#include "../../ServerFacade.h"
using namespace ai;

bool UseItemAction::Execute(Event event)
{
    string name = event.getParam();
    if (name.empty())
        name = getName();

    list<Item*> items = AI_VALUE2(list<Item*>, "inventory items", name);
    list<ObjectGuid> gos = chat->parseGameobjects(name);

    if (gos.empty())
    {
        if (items.size() > 1)
        {
            list<Item*>::iterator i = items.begin();
            Item* item = *i++;
            Item* itemTarget = *i;
            return UseItemOnItem(item, itemTarget);
        }
        else if (!items.empty())
            return UseItemAuto(*items.begin());
    }
    else
    {
        if (items.empty())
            return UseGameObject(*gos.begin());
        else
            return UseItemOnGameObject(*items.begin(), *gos.begin());
    }

    ai->TellError("No items (or game objects) available");
    return false;
}

bool UseItemAction::UseGameObject(ObjectGuid guid)
{
    GameObject* go = ai->GetGameObject(guid);
    if (!go || !sServerFacade.isSpawned(go)
#ifdef CMANGOS
        || go->IsInUse()
#endif
        || go->GetGoState() != GO_STATE_READY)
        return false;

    go->Use(bot);
    ostringstream out; out << "Using " << chat->formatGameobject(go);
    ai->TellMasterNoFacing(out.str());
    return true;
}

bool UseItemAction::UseItemAuto(Item* item)
{
    return UseItem(item, ObjectGuid(), NULL);
}

bool UseItemAction::UseItemOnGameObject(Item* item, ObjectGuid go)
{
    return UseItem(item, go, NULL);
}

bool UseItemAction::UseItemOnItem(Item* item, Item* itemTarget)
{
    return UseItem(item, ObjectGuid(), itemTarget);
}

bool UseItemAction::UseItem(Item* item, ObjectGuid goGuid, Item* itemTarget)
{
    if (bot->CanUseItem(item) != EQUIP_ERR_OK)
        return false;

    if (bot->IsNonMeleeSpellCasted(true))
        return false;

    uint8 bagIndex = item->GetBagSlot();
    uint8 slot = item->GetSlot();
    uint8 spell_count = 0;
    uint8 cast_count = 1;
    uint64 item_guid = item->GetObjectGuid().GetRawValue();
    uint32 glyphIndex = 0;
    uint8 unk_flags = 0;
#ifdef MANGOSBOT_ZERO
    uint16 targetFlag = TARGET_FLAG_SELF;
#endif
#ifdef MANGOSBOT_ONE
    uint32 targetFlag = TARGET_FLAG_SELF;
#endif
#ifdef MANGOSBOT_TWO
    uint32 targetFlag = TARGET_FLAG_SELF;
#endif

    WorldPacket packet(CMSG_USE_ITEM);
#ifdef MANGOSBOT_ZERO
    packet << bagIndex << slot << spell_count;
#endif
#ifdef MANGOSBOT_ONE
    packet bagIndex << slot << spell_count << cast_count << item_guid;
#endif
#ifdef MANGOSBOT_TWO
    uint32 spell_id = 0;
    packet << bagIndex << slot << cast_count << spell_id << item_guid << glyphIndex << unk_flags;
#endif

    bool targetSelected = false;
    ostringstream out; out << "Using " << chat->formatItem(item->GetProto());
    if ((int)item->GetProto()->Stackable > 1)
    {
        uint32 count = item->GetCount();
        if (count > 1)
            out << " (" << count << " available) ";
        else
            out << " (the last one!)";
    }

    if (goGuid)
    {
        GameObject* go = ai->GetGameObject(goGuid);
        if (!go || !sServerFacade.isSpawned(go))
            return false;

#ifdef MANGOS
        targetFlag = TARGET_FLAG_OBJECT;
#endif
#ifdef CMANGOS
        targetFlag = TARGET_FLAG_GAMEOBJECT;
#endif
        packet << targetFlag;
        packet.appendPackGUID(goGuid.GetRawValue());
        out << " on " << chat->formatGameobject(go);
        targetSelected = true;
    }

    if (itemTarget)
    {
        targetFlag = TARGET_FLAG_ITEM;
        packet << targetFlag;
        packet.appendPackGUID(itemTarget->GetObjectGuid());
        out << " on " << chat->formatItem(itemTarget->GetProto());
        targetSelected = true;
    }

	Player* master = GetMaster();
	if (!targetSelected && item->GetProto()->Class != ITEM_CLASS_CONSUMABLE && master)
	{
		ObjectGuid masterSelection = master->GetSelectionGuid();
		if (masterSelection)
		{
			Unit* unit = ai->GetUnit(masterSelection);
			if (unit)
			{
			    targetFlag = TARGET_FLAG_UNIT;
				packet << targetFlag << masterSelection.WriteAsPacked();
				out << " on " << unit->GetName();
				targetSelected = true;
			}
		}
	}

    if(uint32 questid = item->GetProto()->StartQuest)
    {
        Quest const* qInfo = sObjectMgr.GetQuestTemplate(questid);
        if (qInfo)
        {
            WorldPacket packet(CMSG_QUESTGIVER_ACCEPT_QUEST, 8+4+4);
            packet << item_guid;
            packet << questid;
            packet << uint32(0);
            bot->GetSession()->HandleQuestgiverAcceptQuestOpcode(packet);
            ostringstream out; out << "Got quest " << chat->formatQuest(qInfo);
            ai->TellMasterNoFacing(out.str());
            return true;
        }
    }

    bot->clearUnitState( UNIT_STAT_CHASE );
    bot->clearUnitState( UNIT_STAT_FOLLOW );

    if (sServerFacade.isMoving(bot))
    {
        bot->StopMoving();
        ai->SetNextCheckDelay(sPlayerbotAIConfig.globalCoolDown);
        return false;
    }

    bool spellCast = false;
    for (int i=0; i<MAX_ITEM_PROTO_SPELLS; i++)
    {
        uint32 spellId = item->GetProto()->Spells[i].SpellId;
        if (!spellId)
            continue;

        if (!ai->CanCastSpell(spellId, bot, false))
            continue;

		const SpellEntry* const pSpellInfo = sServerFacade.LookupSpellInfo(spellId);
		if (pSpellInfo->Targets & TARGET_FLAG_ITEM)
        {
            Item* itemForSpell = AI_VALUE2(Item*, "item for spell", spellId);
            if (!itemForSpell)
                continue;

            if (itemForSpell->GetEnchantmentId(TEMP_ENCHANTMENT_SLOT))
                continue;

            if (bot->GetTrader())
            {
                if (selfOnly)
                    return false;

                targetFlag = TARGET_FLAG_TRADE_ITEM;
                packet << targetFlag << (uint8)1 << (uint64)TRADE_SLOT_NONTRADED;
                targetSelected = true;
                out << " on traded item";
            }
            else
            {
                targetFlag = TARGET_FLAG_ITEM;
                packet << targetFlag;
                packet.appendPackGUID(itemForSpell->GetObjectGuid());
                targetSelected = true;
                out << " on "<< chat->formatItem(itemForSpell->GetProto());
            }

            Spell *spell = new Spell(bot, pSpellInfo, false);
            ai->WaitForSpellCast(spell);
            spellCast = true;
            delete spell;
        }
        break;
    }

    if (!targetSelected)
    {
        targetFlag = TARGET_FLAG_SELF;
        packet << targetFlag;
        packet.appendPackGUID(bot->GetObjectGuid());
        targetSelected = true;
        out << " on self";
    }

    ItemPrototype const* proto = item->GetProto();
    bool isDrink = proto->Spells[0].SpellCategory == 59;
    bool isFood = proto->Spells[0].SpellCategory == 11;
    if (proto->Class == ITEM_CLASS_CONSUMABLE && (proto->SubClass == ITEM_SUBCLASS_FOOD || proto->SubClass == ITEM_SUBCLASS_CONSUMABLE) &&
		(isFood || isDrink))
    {
        if (sServerFacade.IsInCombat(bot))
            return false;

        bot->addUnitState(UNIT_STAND_STATE_SIT);
        ai->InterruptSpell();

        float hp = bot->GetHealthPercent();
        float mp = bot->GetPower(POWER_MANA) * 100.0f / bot->GetMaxPower(POWER_MANA);
        float p;
        if (isDrink && isFood)
        {
            p = min(hp, mp);
            TellConsumableUse(item, "Feasting", p);
        }
        else if (isDrink)
        {
            p = mp;
            TellConsumableUse(item, "Drinking", p);
        }
        else if (isFood)
        {
            p = hp;
            TellConsumableUse(item, "Eating", p);
        }
        ai->SetNextCheckDelay(27000.0f * (100 - p) / 100.0f);
        bot->GetSession()->HandleUseItemOpcode(packet);
        return true;
    }

    ai->SetNextCheckDelay(sPlayerbotAIConfig.globalCoolDown);
    ai->TellMasterNoFacing(out.str());
    bot->GetSession()->HandleUseItemOpcode(packet);
    return true;
}

void UseItemAction::TellConsumableUse(Item* item, string action, float percent)
{
    ostringstream out;
    out << action << " " << chat->formatItem(item->GetProto());
    if ((int)item->GetProto()->Stackable > 1) out << "/x" << item->GetCount();
    out  << " (" << round(percent) << "%)";
    ai->TellMasterNoFacing(out.str());
}

bool UseItemAction::isPossible()
{
    return getName() == "use" || AI_VALUE2(uint8, "item count", getName()) > 0;
}

bool UseSpellItemAction::isUseful()
{
    return AI_VALUE2(bool, "spell cast useful", getName());
}

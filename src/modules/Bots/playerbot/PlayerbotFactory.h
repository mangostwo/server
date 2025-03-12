#pragma once

#include "strategy/actions/InventoryAction.h"

class Player;
class PlayerbotMgr;
class ChatHandler;

using namespace std;
using ai::InventoryAction;

class PlayerbotFactory : public InventoryAction
{
public:
    PlayerbotFactory(Player* bot, uint32 level, uint32 itemQuality = 0) :
        level(level), itemQuality(itemQuality), InventoryAction(bot->GetPlayerbotAI(), "factory") {}

    static ObjectGuid GetRandomBot();
    static void Init();
    void Refresh();
    void Randomize(bool incremental);

private:
    void Prepare();
    void InitSecondEquipmentSet();
    void InitEquipment(bool incremental);
    bool CanEquipItem(ItemPrototype const* proto, uint32 desiredQuality);
    bool CanEquipUnseenItem(uint8 slot, uint16 &dest, uint32 item);
    void InitSkills();
    void InitTradeSkills();
    void UpdateTradeSkills();
    void SetRandomSkill(uint16 id);
    void InitSpells();
    void ClearSpells();
    void ClearSkills();
    void InitAvailableSpells();
    void InitSpecialSpells();
    void InitTalents();
    void InitTalents(uint32 specNo);
    void InitQuests(list<uint32>& questMap);
    void InitPet();
    void ClearInventory();
    void InitAmmo();
    void InitMounts();
    void InitPotions();
    void InitFood();
    bool CanEquipArmor(ItemPrototype const* proto);
    bool CanEquipWeapon(ItemPrototype const* proto);
    void EnchantItem(Item* item);
    void AddItemStats(uint32 mod, uint8 &sp, uint8 &ap, uint8 &tank);
    bool CheckItemStats(uint8 sp, uint8 ap, uint8 tank);
    void CancelAuras();
    bool IsDesiredReplacement(Item* item);
    void InitBags();
    void InitInventory();
    void InitInventoryTrade();
    void InitInventoryEquip();
    void InitInventorySkill();
    Item* StoreItem(uint32 itemId, uint32 count);
    void InitGuild();
    void InitImmersive();
    void InitStats();
    static void AddPrevQuests(uint32 questId, list<uint32>& questIds);

private:
    uint32 level;
    uint32 itemQuality;
    static uint32 tradeSkills[];
    static list<uint32> classQuestIds;
    static list<uint32> specialQuestIds;
};

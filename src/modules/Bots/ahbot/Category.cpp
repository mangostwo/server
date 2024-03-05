#include "../botpch.h"
#include "Category.h"
#include "ItemBag.h"
#include "AhBotConfig.h"
#include "PricingStrategy.h"

using namespace ahbot;

uint32 Category::GetStackCount(ItemPrototype const* proto)
{
    if (proto->Quality > ITEM_QUALITY_UNCOMMON)
    {
        return 1;
    }

    return urand(1, proto->GetMaxStackSize());
}

uint32 Category::GetMaxAllowedItemAuctionCount(ItemPrototype const* proto)
{
    return 0;
}

uint32 Category::GetMaxAllowedAuctionCount()
{
    return sAhBotConfig.GetMaxAllowedAuctionCount(GetName());
}

PricingStrategy* Category::GetPricingStrategy()
{
    if (pricingStrategy)
    {
        return pricingStrategy;
    }

    ostringstream out; out << "AhBot.PricingStrategy." << GetName();
    string name = sAhBotConfig.GetStringDefault(out.str().c_str(), "default");
    return pricingStrategy = PricingStrategyFactory::Create(name, this);
}

QualityCategoryWrapper::QualityCategoryWrapper(Category* category, uint32 quality) : Category(), quality(quality), category(category)
{
    ostringstream out; out << category->GetName() << ".";
    switch (quality)
    {
    case ITEM_QUALITY_POOR:
        out << "gray";
        break;
    case ITEM_QUALITY_NORMAL:
        out << "white";
        break;
    case ITEM_QUALITY_UNCOMMON:
        out << "green";
        break;
    case ITEM_QUALITY_RARE:
        out << "blue";
        break;
    default:
        out << "epic";
        break;
    }

    combinedName = out.str();
}

bool QualityCategoryWrapper::Contains(ItemPrototype const* proto)
{
    return proto->Quality == quality && category->Contains(proto);
}

uint32 QualityCategoryWrapper::GetMaxAllowedAuctionCount()
{
    uint32 count = sAhBotConfig.GetMaxAllowedAuctionCount(combinedName);
    return count > 0 ? count : category->GetMaxAllowedAuctionCount();
}

uint32 QualityCategoryWrapper::GetMaxAllowedItemAuctionCount(ItemPrototype const* proto)
{
    return category->GetMaxAllowedItemAuctionCount(proto);
}

bool TradeSkill::Contains(ItemPrototype const* proto)
{
    if (!Trade::Contains(proto))
    {
        return false;
    }

    for (uint32 j = 0; j < sSkillLineAbilityStore.GetNumRows(); ++j)
    {
        SkillLineAbilityEntry const* skillLine = sSkillLineAbilityStore.LookupEntry(j);
        if (!skillLine || skillLine->skillId != skill)
        {
            continue;
        }

        if (IsCraftedBy(proto, skillLine->spellId))
        {
            return true;
        }
    }

    for (uint32 id = 0; id < sCreatureStorage.GetMaxEntry(); ++id)
    {
        CreatureInfo const* co = sCreatureStorage.LookupEntry<CreatureInfo>(id);
        if (!co || co->TrainerType != TRAINER_TYPE_TRADESKILLS)
        {
            continue;
        }

        uint32 trainerId = co->TrainerTemplateId;
        if (!trainerId)
        {
            trainerId = co->Entry;
        }

        TrainerSpellData const* trainer_spells = sObjectMgr.GetNpcTrainerTemplateSpells(trainerId);
        if (!trainer_spells)
        {
            trainer_spells = sObjectMgr.GetNpcTrainerSpells(trainerId);
        }

        if (!trainer_spells)
        {
            continue;
        }

        for (TrainerSpellMap::const_iterator itr = trainer_spells->spellList.begin(); itr != trainer_spells->spellList.end(); ++itr)
        {
            TrainerSpell const* tSpell = &itr->second;

            if (!tSpell || tSpell->reqSkill != skill)
            {
                continue;
            }

            if (IsCraftedBy(proto, tSpell->spell))
            {
                return true;
            }
        }
    }

    for (uint32 itemId = 0; itemId < sItemStorage.GetMaxEntry(); ++itemId)
    {
        ItemPrototype const* recipe = sItemStorage.LookupEntry<ItemPrototype>(itemId);
        if (!recipe)
        {
            continue;
        }

        if (recipe->Class == ITEM_CLASS_RECIPE && (
            (recipe->SubClass == ITEM_SUBCLASS_LEATHERWORKING_PATTERN && skill == SKILL_LEATHERWORKING) ||
            (recipe->SubClass == ITEM_SUBCLASS_TAILORING_PATTERN && skill == SKILL_TAILORING) ||
            (recipe->SubClass == ITEM_SUBCLASS_ENGINEERING_SCHEMATIC && skill == SKILL_ENGINEERING) ||
            (recipe->SubClass == ITEM_SUBCLASS_BLACKSMITHING && skill == SKILL_BLACKSMITHING) ||
            (recipe->SubClass == ITEM_SUBCLASS_COOKING_RECIPE && skill == SKILL_COOKING) ||
            (recipe->SubClass == ITEM_SUBCLASS_ALCHEMY_RECIPE && skill == SKILL_ALCHEMY) ||
            (recipe->SubClass == ITEM_SUBCLASS_FIRST_AID_MANUAL && skill == SKILL_FIRST_AID) ||
            (recipe->SubClass == ITEM_SUBCLASS_ENCHANTING_FORMULA && skill == SKILL_ENCHANTING) ||
            (recipe->SubClass == ITEM_SUBCLASS_FISHING_MANUAL && skill == SKILL_FISHING)
            ))
        {
            for (uint32 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
            {
                if (IsCraftedBy(proto, recipe->Spells[i].SpellId))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

bool TradeSkill::IsCraftedBySpell(ItemPrototype const* proto, uint32 spellId)
{
    SpellEntry const *entry = sSpellStore.LookupEntry(spellId);
    if (!entry)
    {
        return false;
    }

    for (uint32 x = 0; x < MAX_SPELL_REAGENTS; ++x)
    {
        if (entry->Reagent[x] <= 0)
        {
            continue;
        }

        if (proto->ItemId == entry->Reagent[x])
        {
            sLog.outDetail("%s is crafted by %s", proto->Name1, entry->SpellName[0]);
            return true;
        }
    }

    return false;
}

bool TradeSkill::IsCraftedBy(ItemPrototype const* proto, uint32 spellId)
{
    if (IsCraftedBySpell(proto, spellId))
    {
        return true;
    }

    SpellEntry const *entry = sSpellStore.LookupEntry(spellId);
    if (!entry)
    {
        return false;
    }

    for (uint32 effect = EFFECT_INDEX_0; effect < MAX_EFFECT_INDEX; ++effect)
    {
        uint32 craftId = entry->EffectTriggerSpell[effect];
        SpellEntry const *craft = sSpellStore.LookupEntry(craftId);
        if (!craft)
        {
            continue;
        }

        for (uint32 i = 0; i < MAX_SPELL_REAGENTS; ++i)
        {
            uint32 itemId = craft->Reagent[i];
            if (itemId == proto->ItemId)
            {
                sLog.outDetail("%s is crafted by %s", proto->Name1, craft->SpellName[0]);
                return true;
            }
        }
    }

    return false;
}

string TradeSkill::GetName()
{
    switch (skill)
    {
    case SKILL_TAILORING:
        return "tailoring";
    case SKILL_LEATHERWORKING:
        return "leatherworking";
    case SKILL_ENGINEERING:
        return "engineering";
    case SKILL_BLACKSMITHING:
        return "blacksmithing";
    case SKILL_ALCHEMY:
        return "alchemy";
    case SKILL_COOKING:
        return "cooking";
    case SKILL_FISHING:
        return "fishing";
    case SKILL_ENCHANTING:
        return "enchanting";
    case SKILL_MINING:
        return "mining";
    case SKILL_SKINNING:
        return "skinning";
    case SKILL_HERBALISM:
        return "herbalism";
    case SKILL_FIRST_AID:
        return "firstaid";
    }
}

string TradeSkill::GetLabel()
{
    switch (skill)
    {
    case SKILL_TAILORING:
        return "tailoring materials";
    case SKILL_LEATHERWORKING:
    case SKILL_SKINNING:
        return "leather and hides";
    case SKILL_ENGINEERING:
        return "engineering materials";
    case SKILL_BLACKSMITHING:
        return "blacksmithing materials";
    case SKILL_ALCHEMY:
    case SKILL_HERBALISM:
        return "herbs";
    case SKILL_COOKING:
        return "fish and meat";
    case SKILL_FISHING:
        return "fish";
    case SKILL_ENCHANTING:
        return "enchants";
    case SKILL_MINING:
        return "ore and stone";
    case SKILL_FIRST_AID:
        return "first aid reagents";
    }
}

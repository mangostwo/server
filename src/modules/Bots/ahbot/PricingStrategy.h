#pragma once
#include "Config.h"
#include "ItemPrototype.h"

using namespace std;

namespace ahbot
{
    class Category;

    class PricingStrategy
    {
    public:
        PricingStrategy(Category* category) : category(category) {}

    public:
        virtual uint32 GetSellPrice(ItemPrototype const* proto, uint32 auctionHouse, bool ignoreMarket = false);
        virtual uint32 GetBuyPrice(ItemPrototype const* proto, uint32 auctionHouse);
        double GetMarketPrice(uint32 itemId, uint32 auctionHouse);
        string ExplainSellPrice(ItemPrototype const* proto, uint32 auctionHouse);
        string ExplainBuyPrice(ItemPrototype const* proto, uint32 auctionHouse);
        virtual double GetRarityPriceMultiplier(uint32 itemId);
        static uint32 RoundPrice(double price);

    protected:
        virtual uint32 GetDefaultBuyPrice(ItemPrototype const* proto);
        virtual uint32 GetDefaultSellPrice(ItemPrototype const* proto);
        virtual double GetQualityMultiplier(ItemPrototype const* proto);
        virtual double GetCategoryPriceMultiplier(uint32 untilTime, uint32 auctionHouse);
        virtual double GetItemPriceMultiplier(ItemPrototype const* proto, uint32 untilTime, uint32 auctionHouse);
        double GetMultiplier(double count, double firstBuyTime, double lastBuyTime);

    protected:
        Category* category;
    };

    class BuyOnlyRarePricingStrategy : public PricingStrategy
    {
    public:
        BuyOnlyRarePricingStrategy(Category* category) : PricingStrategy(category) {}

    public:
        virtual uint32 GetBuyPrice(ItemPrototype const* proto, uint32 auctionHouse);
        virtual uint32 GetSellPrice(ItemPrototype const* proto, uint32 auctionHouse);
    };

    class PricingStrategyFactory
    {
    public:
        static PricingStrategy* Create(string name, Category* category)
        {
            if (name == "buyOnlyRare")
            {
                return new BuyOnlyRarePricingStrategy(category);
            }

            return new PricingStrategy(category);
        }
    };
};

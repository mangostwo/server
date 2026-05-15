/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2025 MaNGOS <https://www.getmangos.eu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

#ifndef AUCTION_HOUSE_BOT_H
#define AUCTION_HOUSE_BOT_H

#include "Config/Config.h"
#include "AuctionHouseMgr.h"
#include "SharedDefines.h"
#include "Item.h"

/**
 * @file AuctionHouseBot.h
 * @brief Auction House Bot implementation for World of Warcraft
 *
 * The AuctionHouseBot makes less populated servers appear more populated by
 * creating auctions automatically. It can both create auctions and bid/buyout
 * on existing auctions to simulate an active auction house economy.
 *
 * Key components:
 * - AuctionBotConfig: Configuration management
 * - AuctionBotAgent: Base interface for bot agents
 * - AuctionBotBuyer: Handles buying items from auction houses
 * - AuctionBotSeller: Handles selling items to auction houses
 * - AuctionHouseBot: Main controller managing buyer and seller
 *
 */

   /**
    * @brief Auction item quality enumeration
    *
    * Shadow of ItemQualities with skipped ITEM_QUALITY_HEIRLOOM and
    * anything after ITEM_QUALITY_ARTIFACT(6). Used to categorize
    * auction items by quality level.
    */
enum AuctionQuality
{
    AUCTION_QUALITY_GREY   = ITEM_QUALITY_POOR,
    AUCTION_QUALITY_WHITE  = ITEM_QUALITY_NORMAL,
    AUCTION_QUALITY_GREEN  = ITEM_QUALITY_UNCOMMON,
    AUCTION_QUALITY_BLUE   = ITEM_QUALITY_RARE,
    AUCTION_QUALITY_PURPLE = ITEM_QUALITY_EPIC,
    AUCTION_QUALITY_ORANGE = ITEM_QUALITY_LEGENDARY,
    AUCTION_QUALITY_YELLOW = ITEM_QUALITY_ARTIFACT
};

#define MAX_AUCTION_QUALITY 7

/**
 * @brief Configuration values of type uint32 for AuctionBot
 *
 * This enumeration defines all uint32 configuration options for
 * the auction house bot, including timing, item amounts, price ratios,
 * and various item-specific settings.
 */
enum AuctionBotConfigUInt32Values
{
    CONFIG_UINT32_AHBOT_MAXTIME,
    CONFIG_UINT32_AHBOT_MINTIME,
    CONFIG_UINT32_AHBOT_ITEMS_PER_CYCLE_BOOST,
    CONFIG_UINT32_AHBOT_ITEMS_PER_CYCLE_NORMAL,
    CONFIG_UINT32_AHBOT_ALLIANCE_ITEM_AMOUNT_RATIO,
    CONFIG_UINT32_AHBOT_HORDE_ITEM_AMOUNT_RATIO,
    CONFIG_UINT32_AHBOT_NEUTRAL_ITEM_AMOUNT_RATIO,
    CONFIG_UINT32_AHBOT_ITEM_MIN_ITEM_LEVEL,
    CONFIG_UINT32_AHBOT_ITEM_MAX_ITEM_LEVEL,
    CONFIG_UINT32_AHBOT_ITEM_MIN_REQ_LEVEL,
    CONFIG_UINT32_AHBOT_ITEM_MAX_REQ_LEVEL,
    CONFIG_UINT32_AHBOT_ITEM_MIN_SKILL_RANK,
    CONFIG_UINT32_AHBOT_ITEM_MAX_SKILL_RANK,
    CONFIG_UINT32_AHBOT_ITEM_GREY_AMOUNT,
    CONFIG_UINT32_AHBOT_ITEM_WHITE_AMOUNT,
    CONFIG_UINT32_AHBOT_ITEM_GREEN_AMOUNT,
    CONFIG_UINT32_AHBOT_ITEM_BLUE_AMOUNT,
    CONFIG_UINT32_AHBOT_ITEM_PURPLE_AMOUNT,
    CONFIG_UINT32_AHBOT_ITEM_ORANGE_AMOUNT,
    CONFIG_UINT32_AHBOT_ITEM_YELLOW_AMOUNT,
    CONFIG_UINT32_AHBOT_CLASS_CONSUMABLE_AMOUNT,
    CONFIG_UINT32_AHBOT_CLASS_CONTAINER_AMOUNT,
    CONFIG_UINT32_AHBOT_CLASS_WEAPON_AMOUNT,
    CONFIG_UINT32_AHBOT_CLASS_GEM_AMOUNT,
    CONFIG_UINT32_AHBOT_CLASS_ARMOR_AMOUNT,
    CONFIG_UINT32_AHBOT_CLASS_REAGENT_AMOUNT,
    CONFIG_UINT32_AHBOT_CLASS_PROJECTILE_AMOUNT,
    CONFIG_UINT32_AHBOT_CLASS_TRADEGOOD_AMOUNT,
    CONFIG_UINT32_AHBOT_CLASS_GENERIC_AMOUNT,
    CONFIG_UINT32_AHBOT_CLASS_RECIPE_AMOUNT,
    CONFIG_UINT32_AHBOT_CLASS_QUIVER_AMOUNT,
    CONFIG_UINT32_AHBOT_CLASS_QUEST_AMOUNT,
    CONFIG_UINT32_AHBOT_CLASS_KEY_AMOUNT,
    CONFIG_UINT32_AHBOT_CLASS_MISC_AMOUNT,
    CONFIG_UINT32_AHBOT_CLASS_GLYPH_AMOUNT,
    CONFIG_UINT32_AHBOT_ALLIANCE_PRICE_RATIO,
    CONFIG_UINT32_AHBOT_HORDE_PRICE_RATIO,
    CONFIG_UINT32_AHBOT_NEUTRAL_PRICE_RATIO,
    CONFIG_UINT32_AHBOT_BUYER_CHANCE_RATIO_ALLIANCE,
    CONFIG_UINT32_AHBOT_BUYER_CHANCE_RATIO_HORDE,
    CONFIG_UINT32_AHBOT_BUYER_CHANCE_RATIO_NEUTRAL,
    CONFIG_UINT32_AHBOT_BUYER_RECHECK_INTERVAL,
    CONFIG_UINT32_AHBOT_CLASS_MISC_MOUNT_MIN_REQ_LEVEL,
    CONFIG_UINT32_AHBOT_CLASS_MISC_MOUNT_MAX_REQ_LEVEL,
    CONFIG_UINT32_AHBOT_CLASS_MISC_MOUNT_MIN_SKILL_RANK,
    CONFIG_UINT32_AHBOT_CLASS_MISC_MOUNT_MAX_SKILL_RANK,
    CONFIG_UINT32_AHBOT_CLASS_GLYPH_MIN_REQ_LEVEL,
    CONFIG_UINT32_AHBOT_CLASS_GLYPH_MAX_REQ_LEVEL,
    CONFIG_UINT32_AHBOT_CLASS_GLYPH_MIN_ITEM_LEVEL,
    CONFIG_UINT32_AHBOT_CLASS_GLYPH_MAX_ITEM_LEVEL,
    CONFIG_UINT32_AHBOT_CLASS_TRADEGOOD_MIN_ITEM_LEVEL,
    CONFIG_UINT32_AHBOT_CLASS_TRADEGOOD_MAX_ITEM_LEVEL,
    CONFIG_UINT32_AHBOT_CLASS_CONTAINER_MIN_ITEM_LEVEL,
    CONFIG_UINT32_AHBOT_CLASS_CONTAINER_MAX_ITEM_LEVEL,
    CONFIG_UINT32_AHBOT_UINT32_COUNT
};

/**
 * @brief Configuration values of type bool for AuctionBot
 *
 * This enumeration defines all boolean configuration options for
 * the auction house bot, including feature toggles and debug flags.
 */
enum AuctionBotConfigBoolValues
{
    CONFIG_BOOL_AHBOT_BUYER_ALLIANCE_ENABLED,
    CONFIG_BOOL_AHBOT_BUYER_HORDE_ENABLED,
    CONFIG_BOOL_AHBOT_BUYER_NEUTRAL_ENABLED,
    CONFIG_BOOL_AHBOT_ITEMS_VENDOR,
    CONFIG_BOOL_AHBOT_ITEMS_LOOT,
    CONFIG_BOOL_AHBOT_ITEMS_MISC,
    CONFIG_BOOL_AHBOT_BIND_NO,
    CONFIG_BOOL_AHBOT_BIND_PICKUP,
    CONFIG_BOOL_AHBOT_BIND_EQUIP,
    CONFIG_BOOL_AHBOT_BIND_USE,
    CONFIG_BOOL_AHBOT_BIND_QUEST,
    CONFIG_BOOL_AHBOT_BUYPRICE_SELLER,
    CONFIG_BOOL_AHBOT_BUYPRICE_BUYER,
    CONFIG_BOOL_AHBOT_DEBUG_SELLER,
    CONFIG_BOOL_AHBOT_DEBUG_BUYER,
    CONFIG_BOOL_AHBOT_SELLER_ENABLED,
    CONFIG_BOOL_AHBOT_BUYER_ENABLED,
    CONFIG_BOOL_AHBOT_LOCKBOX_ENABLED,
    CONFIG_UINT32_AHBOT_BOOL_COUNT
};

/**
 * @brief Configuration manager for Auction House Bot
 *
 * AuctionBotConfig loads and manages all configuration data used by
 * other AHBot classes. It handles loading from the configuration file
 * and provides access to various settings for item amounts, prices,
 * buyer/seller behavior, and debug options.
 */
class AuctionBotConfig
{
public:
    /**
     * @brief Constructor - initializes configuration with default values
     */
    AuctionBotConfig();

    /**
     * @brief Set the configuration file name
     * @param filename Path to the configuration file
     */
    void SetConfigFileName(char const* filename) { m_configFileName = filename; }

    /**
     * @brief Initialize the configuration by loading from file
     * @return True if initialization successful, false otherwise
     */
    bool Initialize();

    /**
     * @brief Get the AHBot includes string
     * @return Comma-separated list of included item IDs
     */
    const char* GetAHBotIncludes() const { return m_AHBotIncludes.c_str(); }

    /**
     * @brief Get the AHBot excludes string
     * @return Comma-separated list of excluded item IDs
     */
    const char* GetAHBotExcludes() const { return m_AHBotExcludes.c_str(); }

    /**
     * @brief Get the AH Bot character ID
     * @return Bot character ID
     */
    uint32 GetAHBotId() const { return m_BotId; }

    /**
     * @brief Get a uint32 configuration value
     * @param index Configuration index
     * @return Configuration value
     */
    uint32 getConfig(AuctionBotConfigUInt32Values index) const { return m_configUint32Values[index]; }

    /**
     * @brief Get a bool configuration value
     * @param index Configuration index
     * @return Configuration value
     */
    bool getConfig(AuctionBotConfigBoolValues index) const { return m_configBoolValues[index]; }

    /**
     * @brief Set a bool configuration value
     * @param index Configuration index
     * @param value Value to set
     */
    void setConfig(AuctionBotConfigBoolValues index, bool value) { m_configBoolValues[index] = value; }

    /**
     * @brief Set a uint32 configuration value
     * @param index Configuration index
     * @param value Value to set
     */
    void setConfig(AuctionBotConfigUInt32Values index, uint32 value) { m_configUint32Values[index] = value; }

    /**
     * @brief Get the ratio of items to sell for a given auction house type
     * @param houseType Auction house type (Alliance/Horde/Neutral)
     * @return Item amount ratio (0-10000 representing 0%-100%)
     */
    uint32 getConfigItemAmountRatio(AuctionHouseType houseType) const;

    /**
     * @brief Check if buyer is enabled for a given auction house type
     * @param houseType Auction house type
     * @return True if buyer enabled, false otherwise
     */
    bool getConfigBuyerEnabled(AuctionHouseType houseType) const;

    /**
     * @brief Get the ratio for items of a certain quality to be sold
     * @param quality Item quality
     * @return Quality amount ratio (0-10000 representing 0%-100%)
     */
    uint32 getConfigItemQualityAmount(AuctionQuality quality) const;

    /**
     * @brief Get the number of items to add per cycle during boost mode
     * @return Items per cycle boost count
     */
    uint32 GetItemPerCycleBoost() const { return m_ItemsPerCycleBoost; }

    /**
     * @brief Get the number of items to add per cycle during normal mode
     * @return Items per cycle normal count
     */
    uint32 GetItemPerCycleNormal() const { return m_ItemsPerCycleNormal; }

    /**
     * @brief Reload the AHBot configuration from file
     * @return True if reload successful, false otherwise
     */
    bool Reload();

    /**
     * @brief Get the name of an item class
     * @param itemclass Item class enum value
     * @return String representation of the item class
     * \see ItemClass
     */
    static char const* GetItemClassName(ItemClass itemclass);

    /**
     * @brief Get the name of an auction house type
     * @param houseType Auction house type enum value
     * @return String representation of the house type
     */
    static char const* GetHouseTypeName(AuctionHouseType houseType);

private:
    std::string m_configFileName; /**< Path to the configuration file */
    std::string m_AHBotIncludes; /**< Comma-separated list of included item IDs */
    std::string m_AHBotExcludes; /**< Comma-separated list of excluded item IDs */
    Config m_AhBotCfg; /**< Configuration object */
    uint32 m_ItemsPerCycleBoost; /**< Items to add per cycle during boost mode */
    uint32 m_ItemsPerCycleNormal; /**< Items to add per cycle during normal mode */
    uint32 m_BotId; /**< Bot character ID */
    uint32 m_configUint32Values[CONFIG_UINT32_AHBOT_UINT32_COUNT]; /**< Array of uint32 configuration values */
    bool m_configBoolValues[CONFIG_UINT32_AHBOT_BOOL_COUNT]; /**< Array of bool configuration values */

    /**
     * @brief Set the AHBot includes string
     * @param AHBotIncludes Comma-separated list of included item IDs
     */
    void SetAHBotIncludes(const std::string& AHBotIncludes) { m_AHBotIncludes = AHBotIncludes; }

    /**
     * @brief Set the AHBot excludes string
     * @param AHBotExcludes Comma-separated list of excluded item IDs
     */
    void SetAHBotExcludes(const std::string& AHBotExcludes) { m_AHBotExcludes = AHBotExcludes; }

    /**
     * @brief Set the AH Bot character ID from character name
     * @param BotCharName Bot character name
     */
    void SetAHBotId(const std::string& BotCharName);

    /**
     * @brief Set a uint32 config value with default
     * @param index Configuration index
     * @param fieldname Field name in config file
     * @param defvalue Default value
     */
    void setConfig(AuctionBotConfigUInt32Values index, char const* fieldname, uint32 defvalue);

    /**
     * @brief Set a uint32 config value with default and maximum cap
     * @param index Configuration index
     * @param fieldname Field name in config file
     * @param defvalue Default value
     * @param maxvalue Maximum allowed value
     */
    void setConfigMax(AuctionBotConfigUInt32Values index, char const* fieldname, uint32 defvalue, uint32 maxvalue);

    /**
     * @brief Set a uint32 config value with default, minimum, and maximum
     * @param index Configuration index
     * @param fieldname Field name in config file
     * @param defvalue Default value
     * @param minvalue Minimum allowed value
     * @param maxvalue Maximum allowed value
     */
    void setConfigMinMax(AuctionBotConfigUInt32Values index, char const* fieldname, uint32 defvalue, uint32 minvalue, uint32 maxvalue);

    /**
     * @brief Set a bool config value with default
     * @param index Configuration index
     * @param fieldname Field name in config file
     * @param defvalue Default value
     */
    void setConfig(AuctionBotConfigBoolValues index, char const* fieldname, bool defvalue);

    /**
     * @brief Load configuration from the configuration file
     */
    void GetConfigFromFile();
};

#define sAuctionBotConfig MaNGOS::Singleton<AuctionBotConfig>::Instance()

/**
 * @brief Base interface for auction bot agents
 *
 * AuctionBotAgent is the base interface for AuctionBotSeller and AuctionBotBuyer.
 * It provides the ability to use dynamic_cast in AuctionHouseBot methods,
 * such as SetItemsRatio which casts m_Seller to AuctionBotSeller.
 */
class AuctionBotAgent
{
public:
    /**
     * @brief Constructor
     */
    AuctionBotAgent() {}

    /**
     * @brief Virtual destructor
     */
    virtual ~AuctionBotAgent() {}

public:
    /**
     * @brief Initialize the agent and check if there's work to do
     * @return True if initialized with at least one active auction house, false otherwise
     */
    virtual bool Initialize() = 0;

    /**
     * @brief Update the agent's operations for the specified auction house
     *
     * For AuctionBotBuyer: places bids on items matching criteria
     * For AuctionBotSeller: lists new items for auction
     *
     * @param houseType Auction house type to update
     * @return True if any action was taken, false otherwise
     */
    virtual bool Update(AuctionHouseType houseType) = 0;
};

/**
 * @brief Status information for auction house bot per house type
 *
 * Used in AuctionHouseBot::PrepareStatusInfos to show item counts
 * per auction house and per quality level for items created by
 * AuctionBotBuyer and AuctionBotSeller.
 * \see AuctionQuality
 */
struct AuctionHouseBotStatusInfoPerType
{
    uint32 ItemsCount;                          /**< Total items in this auction house */
    uint32 QualityInfo[MAX_AUCTION_QUALITY];    /**< Items count per quality level */
};

/**
 * @brief Array of status info for all auction house types
 *
 * Provides status information for all possible auction houses:
 * Alliance, Horde, and Neutral.
 */
typedef AuctionHouseBotStatusInfoPerType AuctionHouseBotStatusInfo[MAX_AUCTION_HOUSE_TYPE];

/**
<<<<<<< HEAD
 * @brief This class handle both Selling and Buying method
 * (holder of AuctionBotBuyer and AuctionBotSeller objects)
 * (Taken from comments in source)
 *
 * \todo Better description here perhaps
 */
class AuctionHouseBot
{
    public:
        /**
         * @brief Initializes a new instance of the \ref AuctionHouseBot class.
         *
         */
        AuctionHouseBot();
        /**
         * @brief Finalizes an instance of the \ref AuctionHouseBot class.
         *
         */
        ~AuctionHouseBot();

        /**
         * @brief Updates the \ref AuctionHouseBot by checking if either the \ref AuctionBotSeller or
         * \ref AuctionBotBuyer wants to sell/buy anything and in that case lets one of them do
         * that and the other one will have to wait until the next call to \ref AuctionHouseBot::Update
         *
         */
        void Update();
        /**
         * @brief Initializes this instance.
         *
         */
        void Initialize();

        // Followed method is mainly used by level3.cpp for ingame/console command
        /**
         * @brief Sets the items ratio which probably decides how many items should
         * appear in each of the auction houses
         *
         * @param al The alliance house ratio
         * @param ho The horde house ratio
         * @param ne The neutral house ratio
         */
        void SetItemsRatio(uint32 al, uint32 ho, uint32 ne);
        /**
         * @brief Sets the items ratio for a specific house, like \ref AuctionHouseBot::SetItemsRatio but
         * only for one house.
         *
         * @param house The house
         * @param val The new ratio
         */
        void SetItemsRatioForHouse(AuctionHouseType house, uint32 val);
        /**
         * @brief Sets the items amount.
         *
         * @param (vals)[] The vals.
         */
        void SetItemsAmount(uint32(&vals) [MAX_AUCTION_QUALITY]);
        /**
         * @brief Changes the ratio for how often a certain quality of items should show up at the
         * AH. A specialised version of \ref AuctionHouseBot::SetItemsAmount
         *
         * @param quality quality of the items you want to change the ratio for
         * @param val the new ratio you want as a value between 0-10000 probably representing 0%-100%
         * \see AuctionQuality
         */
        void SetItemsAmountForQuality(AuctionQuality quality, uint32 val);
        /**
         * @brief Reloads all the configurations, for the AH bot and for both \ref AuctionBotBuyer and
         * \ref AuctionBotSeller and ourselves
         *
         * @return bool true if it went well, false otherwise
         */
        bool ReloadAllConfig();
        /**
         * @brief Expires all the items currently created by the AH bot and they'll be replaced later on
         * again. If parameter all is false only auctions without a bid are removed.
         *
         * @param all Whether to expire all auctions or only those without a bid
         */
        void Rebuild(bool all);

        /**
         * @brief Fills a status info structure with data about how many items of each there
         * currently are in the auction house that the auction bot has created
         *
         * @param statusInfo the structure to fill with data
         */
        void PrepareStatusInfos(AuctionHouseBotStatusInfo& statusInfo);
    private:
        /**
         * @brief Initializes the agents, ie: the \ref AuctionBotBuyer and \ref AuctionBotSeller
         *
         */
        void InitializeAgents();
        AuctionBotAgent* m_Buyer; /**< The buyer (\ref AuctionBotBuyer) for this \ref AuctionHouseBot */
        AuctionBotAgent* m_Seller; /**< The seller (\ref AuctionBotSeller) for this \ref AuctionHouseBot */

        uint32 m_OperationSelector; /**< 0..2*MAX_AUCTION_HOUSE_TYPE-1 */
};

///Convenience to easily access the singleton for the \ref AuctionHouseBot
#define sAuctionBot MaNGOS::Singleton<AuctionHouseBot>::Instance()

#endif

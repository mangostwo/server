# Auction House bot

This Auction House bot module was rebuilt from the blueboy Playerbot by ike3 and further adapted into Mangos by several contributors.  It provides an alternative for the built-in AuctionHouseBot (which should be disabled by ahbot.conf).

## Commands

So-as not to conflict with the built-in auction house bot this module uses the keyword `auctionbot`.

* auctionbot stats
* auctionbot expire
* auctionbot update
* auctionbot <itemId>

## Config
### Enabling the bot

At the top of the ahbot.conf should be the two entries that disable the built-in auction house bot's agents:
```
AuctionHouseBot.Seller.Enabled = 0
AuctionHouseBot.Buyer.Enabled = 0
```

Add the ahbot module's enable flag
```
AhBot.Enabled = 1
```

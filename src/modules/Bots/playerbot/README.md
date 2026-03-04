# Player Bot Module

The Playerbot module integrates with Mangos to provide user-controll AI characters for groups and raids.  This is especially helpful on low-activity servers.

## Player Bots

Alternative characters can join the game as a playerbot, controlled by the human player.

## Random Bots

Based on the configuration found in aiplayerbot.conf a number of bots will be randomly generated and added to the server.  These bots will perform random grinding actions until invited to a group by a human player.

## Commands

### Chat

To add, remove, or initialize a playerbot a player can use the `.bot` command.  Players can further interact with Playerbots or Randombots by whispering or chatting in group, guild, or raid chats.

See the reference below for bot commands.

### Console

A console user can interact with the Random Bots module using the `rndbot` command

## Reference

* http://ike3.github.io/mangosbot-docs/

### Command Reference from IKE3 documentation

| Command | Description |
| --- | --- |
| Movement |     |
| follow | Follow master |
| stay | Stay in place |
| flee | Flee with master (ignore everything else) |
| d attack my target | Attack my target |
| d add all loot | Check every corpse and game object for loot |
| grind | Attack anything |
| Strategies |     |
| co +s1,-s2,~s3,? | Add, remove, toggle and show combat strategies |
| nc +s1,-s2,~s3,? | Add, remove, toggle and show non-combat strategies |
| ds +s1,-s2,~s3,? | Add, remove, toggle and show dead strategies |
| Items |     |
| e \[item\] | Equip item |
| ue \[item\] | Unequip item |
| u \[item\] | Use item |
| u \[item\] \[target\] | Use item on target (e.g. use gem on item) |
| destroy \[item\] | Destroy item |
| \[item\] | Add to trade window if trading, show if it is useful |
| Quests |     |
| accept \[quest\] | Accept quest at the selected quest giver |
| accept * | Accept all quests at the selected quest giver |
| drop \[quest\] | Abandon quest |
| r \[item\] | Choose quest reward |
| quests | Show quest summary |
| \[quest\] | Show quest and objectives status |
| talk | Talk to the selected NPC (to complete a quest) |
| u \[game object\] | Use game object (use los command to obtain the game object link) |
| Misc |     |
| los | Enlist game objects, items, creatures and NPCs bot can see |
| stats | Show stat summary (inventory, gold, xp, etc.) |
| leave | Leave party |
| trainer | Show what bot can learn from the selected trainer |
| trainer learn | Learn from the selected trainer |
| spells | Show bot's spells |
| cast \[spell\] | Cast the spell |
| home | Set home at the selected innkeeper |
| summon | Summon bot at the inn |
| release | Release spirit when dead |
| revive | Revive when near a spirit healer |

There are some of master's actions bot can react. For example:

| Action | Bot reaction |
| --- | --- |
| accept a quest | Bot will accept it as well |
| talk to a quest giver | Bot will turn in his completed quests |
| use meeting stone | Teleport using the stone |
| start using game object and interrupt | Use the game object |
| open trade window | Show inventory and start trading |
| invite to a party/raid | Accept the invitation |
| start raid ready check | Tell his ready status |
| mount/unmount | Mount/unmount as well |
| go through a dungeon portal | Follow into the dungeon |

**HINT**: you can create key-binded macros for any of this command for quick usage, e.g.

| Key | Macro | Decription |
| --- | --- | --- |
| F   | /p follow | Follow me |
| G   | /p stay | Stay |
| H   | /p flee | Pull back |
| Shift+T | /p d attack my target | Attack my target |
| T   | /p @tank d attack my target | Attack my target (for tanks only) |
| P   | /p co ~passive,? | Toggle active/passive mode (assist or ignore) |
| J   | /p d add all loot | Loot everything |

*/p* can be replaced with */r* if you are in raid group.

To learn about other commands tell bot someting invalid and it will enlist all the commands and stategies available.
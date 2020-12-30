MaNGOS Two Changelog
====================
This change log references the relevant changes (bug and security fixes) done
in recent versions.

0.22 (2021-01-01 to now) - "Echo of Nostalgia"
----------------------------------------------
* Initial Release 22 Commit


0.21 (2016-04-01 to 2021-01-01) - "The Battle for Northrend"
------------------------------------------------------------
Many Thanks to all the groups and individuals who contributed to this release.
- 625+ Commits since the previous release.

Code Changes:
=============
* Removed the old SD2 scripts and Added the new unified SD3 Submodule
* Removed the individual extractor projects and added a unified Extractors Submodule

* (from cab's repo) added UpdateSpeed function
* [Appveyor] Remove no-longer needed file
* [Build] Add OpemSSL1.0.2j installers
* [Build] Added MySQL 5.7 support
* [Build] Enhanced Build System
* [Build] Force building SOAP and PlayerBots in testing
* [Build] move core definition into cmake
* [Build] Updated build system, based on the work of H0zen (#161)
* [Cleanup] Remove tabs which have crept into the source
* [Core] Add a debug command to show all possible random point for selected creature. [c12813]
* [Core] Allow GAMEOBJECT_TYPE_TRAP/BUTTON respawn using SCRIPT_COMMAND_RESPAWN_GAMEOBJECT [c12815] [c12837]
* [Core] Blink improved. [ct12819]
* [Core] Fix charged items auctioneering.
* [Core] Fix GO 190769 use animation. [c12829]
* [Core] Fix hunter traps.
* [Core] Fix spell 19714 is buff instead of debuff. May fix also other spell that use same aura. [ct12831]
* [Core] Implement spells effects required for first encounter in Toc5 Spells 64101, 62552, 62575, 68282, 63010, 68307, 68504, 63119 and 64192 [ct12830]
* [Core] Improve random movement generator. [c12811]
* [Core] Minor changes in prep for release
* [Core] Restore build on Mac OS X. [ct12824]
* [CORE] Restrict debug logs to debug builds only
* [Core] Traditional include fix for gcc et al. [c12820]
* [Core] Waypoint and movement generator system rewrite
* [Core][SD2] ..and binding of InstanceScript
* [Core][SD2] Correct binding of ZoneScript
* [Core][SD2] The first start with a proper DB (not included for now)
* [Core][SD2]Refactoring of Northrend scripts
* [CRITICAL] Fixed dumb logic in ObjectAccessor
* [DB] Set min db levels
* [DbDocs] The Big DB documentation update
* [DEP] Fix simultaneous connection contention issue
* [DEP] Update Stormlib v9.21
* [DEP] Updated ACE to 6.5.9
* [Dep] Updated Dep submodule
* [Dep] Updated submodule
* [Deps] Dep library updated. Thanks H0zen/xfury
* [DEPS] Update zlib version to 1.2.8
* [Docs] Fix some broken links
* [Docs] Updated extraction readme
* [EasyBuild] Add support for newer MySQL/MariaDB versions
* [EasyBuild] Fix cmake crash on French OS
* [EasyBuild] Fix some more French OS crashes
* [Easybuild] Fixed a crash. Thanks MadMaxMangos for finding.
* [EasyBuild] Hotfix Revert OpenSSL binaries.
* [EasyBuild] ignore easybuild created debug files
* [EasyBuild] Minor update to include some additional checks
* [EASYBUILD] Move the source of the downloads from external sites to internal
* [Easybuild] Reactivate VS2019 support
* [EasyBuild] Remove static files
* [EasyBuild] Updated base versions of libraries
* [EasyBuild] Updated MySQL and Cmake library locations
* [Easybuild] Updated to include VS2015 support
* [EasyBuild] Updated to remove some build options
* [EasyBuild] Updated to Support modified build system and enhancements
* [Eluna] Add conditionals around code
* [Eluna] Fix crash when accessing players not on any map
* [Eluna] Fix uint32_t errors (VS 2013)
* [Eluna] Remove Eluna Submodule URL
* [ELUNA] SpellAttr fixes and more #120
* [Extractors] Fix file locations
* [EXTRACTORS] Improvement made to getBuildNumber()
* [Extractors] Minor cleanup to fix some warning messages
* [Extractors] removed useless code
* [Extractors] Updated extractors to fix movement bug. Will need to reextract
* [FIX] Fix server crash
* [FIX] Fixed a logic error in EasyBuild
* [Fix] non initialised variable. Thanks xfury
* [Fix] Updated Realmdb
* [Install] Port changes from Zero
* [Linux] Fix playerbots in getmangos.sh. Thanks Tom Peters
* [OSX] activate MAC OS build testing
* [Phase1] Prep part 1
* [Realm] fix account table errors
* [REALM] Updated submodule
* [Realmd] Fixed Broken Patching system
* [Realmd] Resolve SRP6a authentication bypass issue. Thanks 
* [realmd] Resolved authentication bypass. Thanks namreeb
* [Scripts] Allow SD3 scripted dummy and script spelleffects upon players
* [Scripts] Fix typo in boss_thermaplugg script.
* [SD2] minor cleaning
* [SD2] Refactoring complete
* [SD2] Restoring several overlooked creature AIs
* [SD2] The first build since refactoring
* [SD3] Correct typo
* [SD3] Fix BRD issues and server crash. Thanks H0zen
* [SD3] Fix deeprun rat roundout crash
* [SD3] Fix error in submodule
* [SD3] Fix item_petrov_cluster_bombs
* [SD3] fix Quest 7603 - Kroshius
* [SD3] Fix quest Kodo Roundup (#69)
* [SD3] Fix SD3::CreateInstanceData to work with non-instantiable maps
* [SD3] Fix server crash on quest 4021
* [SD3] Fix sleeping peon in durotar
* [SD3] Fix Stratholme Unforgiven spawn location
* [SD3] Initial SD3 implementation and removal of SD2
* [SD3] MC: correct spawning of Majordomo
* [SD3] Missing pointers added back
* [SD3] Naxx: Gothik - redesign
* [SD3] ScriptDev3 Updates
* [SD3] SpellAttr fixes and more #120
* [SD3] SpellAttr fixes and more for mangosOne #120
* [SD3] Step back SD3 until eluna is ready
* [SD3] The Endless Hunger script update
* [SD3] Update UBRS door logic
* [SD3] Updated for BRD arena fix
* [SD3] Updated TAQ
* [Submodules] Updated submodules
* [Sync] Added another chunk of project sync
* [Sync] Added more project sync
* [Sync] Minor project sync
* [Sync] Remove unused MANGOS_DLL_DECL statement
* [Sync] Some adjustments from Zero
* [Sync] Some project sync up
* [Sync] Some sync from Zero
* [Sync] Update build system to match Zero
* [Tool] Merged vmaps extract/assembler. Updated scripts
* [TOOLS] Added unified extractor submodule
* [Tools] Extractor updates
* [Tools] Fix vmap-extractor crash.
* [TOOLS] Fixed mmap extractor binary name used in MoveMapGen.sh
* [TOOLS] Fixed mmap extractor binary name used in various scripts
* [Tools] Updated Extractor
* [TOOLS] Updated extractor submodule
* [Tools] Updated extractors
* [Tools] Updated Unified Extractor subModule
* [Warden] Fix OS check on Login
* [Warden] Refactor to match other cores
* Add Codacy badge and status
* Add Core support for Franklin the Amiable / Klinfran the crazed (#118)
* Add Disables table based on the work of olion on Zero
* Add game event hooks and update eluna version
* Add mangosd full versioning information on windows
* add new mangos 'family' icons. Thanks UnkleNuke for the original design
* Add possibility to write cmangos command via a whisp.
* Add realmd full versioning information on windows
* Add spell school prohibition to interrupts Creatures now honor spell cooldowns for both behavior and event AI Fix setting both shadow and arcane resistance from arcane resistance template value
* Add state for GM command completed quests. Thanks H0zen for assistance
* Add support for new comment column
* Add support for spell 28352
* Add Ubuntu 19.04 case for Prerequisites install (#77)
* Added AppVeyor Build Status
* Added MySql 8.0 support, see notes
* Added support for openssl on OSX systems running OpenSSL 1.0.2g (installed using homebrew). (#115)
* Adding new distribution support (Fedora) (#16)
* Adding support for Player Bots submodule in installer. (#20)
* Adding support for Ubuntu: Curl dependencies added - Adding support when several WoW clients path are detected. Only the first one is selected - Adding support for database updates. Only last folder (alphabetically sorted) will be takenxw
* Adding support of several known Linux distribution for dependancies setup (#175)
* Adds custom emote to wyrmthalak script
* Adjust the source code and build enviornment so that Mangos Zero will build on ARM32. (#79)
* Ammend Core name
* Another rogue stealth tweak.
* Apply style fix
* Apply VMAP updates and other fixes from MangosOne
* Appveyor supplied fix for openSSL 1.0
* Attempt to correct tabing
* Attempt to fix Mutanus the Devourer event.
* Auction House Seller Option, ported from Zero
* AuctionHouse Bot fixes (#170)
* Autobroadcast should be disabled by default
* AutoBroadcast system.
* Base SD2 files updated
* Battlegrounds updated
* BRD Grimm Guzzler related updates
* Change world DB structure for refactored scripts
* Changed email return for item that can't be equiped anymore. Before the email was sent with an empty body and the subject was to long to be displayed in the player email. Now the Email is sent with the subject 'Item could not be loaded to inventory.' and the body as the subject message before. (#71)
* Chest with quest loot deactivation (2622b33)
* Clean up readme a little
* Correct .EXE release number back to Rel21
* Correct a self only spell error
* Correct invalid parameter
* Correct Typo for default status
* Corrected a typo
* Corrected website URL
* Corrections to the build system.
* Cosmetic fixes of previous commit (#158)
* Create a docker container image and runing it with docker-compose (#164)
* Database revision refactor
* DB version tracking: less includes to minimize rebuild at revision change
* debug recv Command added
* Description of the meaning of the format strings added
* Disable OSX build checking until we have an OSX dev to get them fixed
* Eluna update version - Fix duplicate timer update
* ensure bins are marked as executable (#108)
* Expected Base DB updated to Rel21_12_055
* Extend pvp team indexes to global use
* Final cleanup
* Finkle Einhorn is now spawned after skinning The Beast in UBRS.
* Fix .gobject add #guid
* Fix a typo in Level2.cpp
* Fix ACHIEVEMENT_CRITERIA_TYPE_OWN_ITEM
* Fix add event gameobjects appearing at once
* Fix AH notification before Auction sold
* Fix AHBot SetPricesOfItem (#87)
* Fix appveyor link
* Fix bag swapping
* Fix Blood Draining enchant aura
* Fix build
* Fix build and tidy up file
* Fix build error due to 226f27f.
* Fix build system - part 2.
* Fix build system - part 3
* Fix build system 3.
* Fix build system 5.
* Fix character login issue
* Fix cmake config dir variable
* Fix Compile with Latest SD3 : GameObjectAI methods - and some Dire Maul things #95
* Fix crash at startUp due to command localization loading
* Fix crash in BIH module due to uninitialized member variable. (#172)
* Fix crash on taming rare creatures.
* Fix crash when using command helps (#93)
* Fix Deeprun spell used on player
* Fix Eluna and OS X build
* Fix Eluna build
* Fix encoding of mangos.conf file. Thanks Wolverine for pointing
* Fix Feral Swiftness talent
* Fix Fishing
* Fix floating point model for VS 2015 (#52)
* Fix Gameobject spawns. Hopefully for real this time
* Fix Go Rotations. Thanks H0zen
* Fix instance cleanup at startup (#99)
* fix linux shell script error. (#82)
* Fix logo Url
* Fix mac build
* Fix non PCH build and update Eluna
* Fix OpenSSL travis for mac
* Fix part of NPC localized text cannot be displayed.
* Fix pdump write command and add check to pdump load (#106)
* Fix possible problem with 'allow two side interaction' and loot.
* Fix potential NullPointerException on C'Thun (#107)
* Fix proc system.
* Fix quest rewards appearing twice in chat
* Fix quests 4512 & 4513
* fix reference to dockerFiles to match with real files name (#92)
* Fix rpath for world server on Linux
* Fix send mail and send item commands
* Fix server crash. Thank H0zen/mpfans
* Fix Simone the seductress (#121)
* Fix SOAP build
* Fix some codacy detected issues
* Fix some compiler warnings and project sync
* Fix some conflicts.
* Fix some startup (fake) errors
* Fix stock unit frames displaying group members as offline after teleport
* Fix tabs
* Fix typo in VMap BIH generation
* Fix up for Phase1 changes
* Fix VS debug build
* Fix VS2017 build. Needs cmake 3.8.0 minimum
* Fix whisper blocking (#160)
* Fix wrong use of uninitialized locks. Whenever ACE_XXX_Thread_Mutexes are used, there are 3 fundamental rules to obey: 1. Always make sure the lock is initialized before use; 2. Never put 2 locks each other in memory (false sharing effect); 3. Always verify that the lock is really acquired - use ACE_XXX_GUARD macros;
* FIX-CENTOS-BUILD Added epel repo
* FIX-CENTOS-BUILD Fixed centos 7 build
* Fixed a bug where destroying a channeling object would crash the server. (#165)
* Fixed build
* Fixed build for previous commit
* Fixed instant 'Failed attempt' on gathering
* Fixed memory issue with msbuild build
* Fixed OpenSSL location (#118)
* Fixed server Crash
* Fixed spell 56626 and ranks
* Fixed to allow some auras to stack
* Fixes Error "There is no game at this location" (#172)
* Format specifiers was not correct in lootmgr
* g++ was not installed without build-essential (#80)
* Gossip Item Script support (#124)
* GroupHandler: prevent cheater self-invite
* Hai'shulud script updated.
* Hunter Pet speed
* Implement command localization
* Implement DBscript  SCRIPT_COMMAND_RESET_GO
* Implement OpenSSL 1.1.x support
* Implement quest_relations table. Based on work by 
* Implement spell effects 34653 and 36920
* Implement spell effects required for the Twin Val'kyrs encounter in ToC25
* Improving Build system and removing Common.h clutter
* InstantFlightPaths, ported from Zero
* Lazt Peons will now call players by name.
* Lazy Peon SpellId Information
* Lich king added another profession bringing the total to 11 (#167)
* linux/getmangos.sh: default to build client tools (#19)
* Major battleground update
* Make GM max speed customisable through mangosd.conf (#89)
* Make Mangos compatible with newer MySQL.
* Master of Subtlety
* Minor  pet/Spell update
* Minor changes to previous commit.
* Minor styling tidy up
* Minor typo corrected (#184)
* Minor typo tidy up
* Missed spot
* Missing delimiter (#168)
* More fixes
* More lock fixes. Also fix the .character level command
* More minor corrections
* More robust checks on mutex acquire.     - When using ACE_xxx_Guard, the caller must ensure the internal lock is really acquired before entering the critical section (see http://www.dre.vanderbilt.edu/Doxygen/6.0.1/html/libace-doc/a00186.html#_details - Warnings paragraph)
* More SQL delimiting for modern servers (#166)
* Move DB revision struct to cpp
* Move the license file
* Moved mail expiry delay into configuration file
* moved SendShowMailBox to MailHandler.cpp, related to mangosthree/server
* New SD3 script file added: scholomance.cpp
* New thread pool reactor implementation and refactoring world daemon. (#8)
* Not so bright warning if DB content newer than core awaits
* Now we can inspect player when GM mode is ON (#98)
* Ogre Brew scripts replaced
* Outland - Auchindoun and Black Temple
* Perform DB Rollup to Rel21_11_075
* PLAYER_EVENT_ON_LOOT_ITEM fix for eluna. Thanks mostlikey
* Ported multithreaded map updater from ZERO
* Prevent duplicate Auction Expired mails
* Project tidy up and sync
* Quality of life code update
* Refactored db_scripts The unity! - Based on the original work of H0zen for Zero.
* Regex requires gcc 4.9 or higher
* Remove last reminents of obsolete npc_gossip table
* remove obsolete code. Thanks H0zen
* Remove obsolete project and files
* Remove Remnants of Two obsolete tables: npc_trainer_template & npc_vendor_template
* Removed OpenSSL1.1.x blocker
* Removed SD2 database binding
* Renamed goname/namego commands to appear/summon.
* Replacing aptitude by apt-get on Ubuntu by default. Added support for Red Har 'Experimental'
* Revert "[SD3] Updated for BRD arena fix"
* Revert "Changed email return for item that can't be equiped anymore. Before the email was sent with an empty body and the subject was to long to be displayed in the player email. Now the Email is sent with the subject 'Item could not be loaded to inventory.' and the body as the subject message before. (#71)"
* Revert "Final cleanup"
* Revert "Fix VS debug build"
* Revert "Remove Remnants of Two obsolete tables: npc_trainer_template & npc_vendor_template"
* Revert [0935b66]. Breaks Eluna.
* Reverted 'Updated ACE to 6.5.9'
* Reverting back to the previous version of the code.
* Revision updated (World DB)
* Rogue Stealth corrections
* SD3 fix linux compile and reference latest SD3 commit
* Server Banner and Status redone
* Server-owned world channel
* Several major improvements to Linux installer. (#15)
* Some minor code / text tidying up
* Some styling cleanup
* Spell 1961 support
* Spell class - Fix for inaccessible class data
* Spell ID 51173 support.
* Spell target update.
* Spell updates
* SpellAttr fixes and more for MangosTwo #120
* Style cleanup from the Mangos Futures Team
* Summon Spell corrections
* Supress Travis build on OS X until a fix is found.
* Swapped 'dbscripts_on_creature_movement' warning with 'dbscripts' â€¦ (#97)
* Switch off non-existing playerbots
* Synchronized Conf files for easier comparison
* Syncing with the lower cores.
* Tab cleanup
* The Big Command Files Reorganization. Based on the work of Elmsroth (#170)
* Trimming Ubuntu dependencies (#17)
* Type conversion mismatch correction
* Undying Resolve buff wont see opposite faction
* Update Appveyor and Travis build files
* Update cmake macros
* update deeprun rat roundup script
* Update deprecated row_format_fixed
* Update getmangos.sh
* Update mangosd.conf.dist.in
* Update maps expected version number
* Update missed year changes
* Update notes on DK talent Butchery
* Update revision.h
* Update SD3 to fix gossip scripts
* Update SpellMgr.cpp
* Updated ACE to latest version and fixed appveyor
* Updated databases version.
* Updated Readme.md and icons
* Updated submodules
* Updated submodules: SD3 and Realmd
* Updated to latest version of SD3's master branch
* Updating Debian Sources (#169)
* Updating to latest version of SD3
* Upgrading checks for Database::CheckDatabaseVersion (#86)
* URL update
* use canonical target names for zlib and bzip2
* Use pet level modifiers for SPELL_EFFECT_SUMMON_PET (56)
* Various Fix
* Various Spell Implementation.
* well fed buff
* World Scripts
 
DB Changes:
===========
* [French] Updated Translations
* [Locale] Fix 'replace_BaseEnglish_with_xxx' file
* [Locale] Fix up installation script
* [Locales] Add multi translations. Thanks Elmsroth and Gromchek
* [Locales] Added Achievement Locale loading
* [Localisation] Added Achievement Locale loading
* [Localisation] Format updates and minor changes. Thanks all authors
* [Localisation] Minor update from magnet
* [Localisation] Multiple updates. Thanks Gromchek and everyone else
* [Localisation] Updated Quest texts
* [Localisation] Updated various texts
* [Localisation] various Text translations. Thanks Gromcheck/Elmsroth
* [Localisation] Various updates. Thanks galathil and other contributors
* [Realm] fix account table errors
* [Realm] fix missing comma
* [Russian] Added some new translations
* Add missing creature script text
* Add missing Fel Fire aura to Warbringer Arix'Amal.
* Add missing spawns of NPC 12125
* Add missing Spirit Healer
* Add missing table to backup scripts
* Add script for NPC 14488 EventAI.
* Add script for Plagued Dragonflayers.
* Add Velendris Whitemorn gossip.
* Add XT:9 pathing
* Added missing item 13325
* Added Tracker Val'zij spawn.
* Area 52 Big Bruiser 20484 EventAI.
* Area 52 Bruiser 20485 EventAI.
* Asghar 22025 EventAI
* Attempt to fix 21_20_001 not applying
* Base DB Rollup to Rel21_12_055
* Bishop Street 27246
* Brainwashed Noble 596
* carriage returns - quest
* carriage returns gossip_menu_option
* Clefthoof Calf 19183 EventAI added.
* Dangerous! - object
* DB script for command localization
* Durkot Wolfbrother & Armorer Orkuruk
* Felfire Diemetradon 21408 AI.
* Fix a few spawndist errors caused in previous updates.
* Fix another start up error with movement / spawndist
* Fix broken update file
* Fix for the quest 7636 (#110)
* Fix position of a Menethil Sentry.
* Fix quest 3861 - CLUCK
* fix some startup errors
* Fix target for  Empty cursed jar, Empty tainted jar and Empty pure sample jar.
* Fix Terrorclaw Respawn Time.
* Fix texts for quest 6461
* Fix type in creature_ai_text
* Fix typo 'Skuller' -> 'Skulker'
* Fix upper case in OfferRewardText for quest 8288
* Fixed a creature_ai_texts typo
* Fixed an error from last commit
* Fixed gossip of NPC 14741
* Fixed text for item 10022
* Frostmane Troll Whelp.
* Illidari Jailor AI script
* Implement script for Warbringer Arix'Amal.
* Improve Broken Skeleton 16805.
* Laris Geardawdl complete rework.
* Merge branch 'master' of https://github.com/mangostwo/database
* Missing spawn, dbscript and WP for  En'kilah Necromancers.
* Moaki Bottom Thresher 26511
* Overlord Gorefist 18160
* q.11593 'The Honored Dead'
* q.12182 'To Venomspite!'
* q.354 'Deaths in the Family'
* q.770 Should only be available to horde.
* Quick fix to Velendris Whitemorn gossip update.
* Remove comment from previous commit to avoid confusion.
* Remove obsolete file
* Remove unnecessary spacing from mangos_string
* River Thresher 27617
* Skyguard Rations and Enriched Terocone Juice.
* Slaag 22199 - Frenzy should be used at any HP%
* Support for quest 7636.
* Target type for Empty pure sample jar - Part 2.
* The Big Command Help Sync Pt2
* The big Command help syncup
* This fixes some issues a pervious update attempted to fix
* Update barrel locations
* Update deprecated row_format_fixed
* Updates to InstallDatabases.sh (#112)
* Venomspite quest relations.
* WIP: Setup database with docker (#166)
* Wrath Lord 20929


0.20 (2015-02-31) - Points of departure
---------------------------------------
* Internal version, only used for restructuring to new system

0.19 (2014-10-31) - Untitled
----------------------------------
Many Thanks to all the groups and individuals who contributed to this release.

* Some of the dependant file groups have been made into submodules
* i.e. all the dependant libraries (dep folder) and realmd

* Merged scripts directly to core repository.
* Cleaned up much gcc warnings.
* Implemented "Random battleground".
* Fixed arena scoreboard end screen.
* Fixed showing skirmish or rated arena queue icon.
* Fix LANG_ADDON use on Guild Channels.
* Fix aura not removed in some case.

0.18.0 (2014-04-01)
--------------------------------------------------------------------------------
 * Remove a duplicate comment start.
 * Renamed SPELLFAMILY_UNK1 to SPELLFAMILY_ENVIRONMENT.
 * [m12535] Renamed table scripted_event_id to scripted_event.
 * [m12534] SPELL_ATTR_UNK7 changed to SPELL_ATTR_HIDE_SPELL.
 * Added ChangeLog template.
 * Updated the list of ignored files and directories.
 * Fixed a NULL pointer dereference.
 * [m12533] Added unique summon action to EventAI.
 * Merge branch 'master' of http://github.com/mangostwo/server
 * [m] Updated MMaps extraction exceptions
 * Updated Required Version
 * Merge pull request #4 from lfxgroove/fix-g3d-freebsd
 * Merge pull request #3 from lfxgroove/master
 * [m] Add fix by bels to compile g3d on FreeBSD, thanks!
 * [m] Add fix by danielsreichenbach to be able to build mmap and vmap/extractor/assembler, see 3080b49 in mangozero server repo
 * spelling corrections and explained options a little more
 * [m12532] sync'd realmd version with other cores
 * [m12531] Sync'd vs project files with zero
 * [m] Ammended some url locations
 * Merge pull request #2 from sikevux/master
 * Fixed issue with db version being set to wrong value
 * Merge pull request #1 from sikevux/master
 * Fixed error in SQL syntax
 * [m] Added lots of Doxygen docs and fixed many more references
 * Merge branch 'Rel18'
 * [m12530] Updated console URL
 * [m] more project cleanup
 * [c12529] Hotfix to recent text loading functions and format strings
 * [c12528] Use poosible changed model names with vmap extraction & new exes
 * [c] Fix some warnings
 * [c12527] Store how many texts are loaded for validity checks. Use this with EventAI
 * [c12526] EventAI: Use generic DoDisplayText and loading of additional text display
 * [c12525] Add generic DoDisplayText function and use additional data of dbscripts table
 * [c12524] Add const-correctness to Text related functions
 * [c12523] Add database changes to support more data for DB Script texts
 * [m] minor formatting cleanup
 * [m12522] ACE Code Style cleanup
 * [c12521] Add stacking exception for spells 39993 and 40041
 * [c12520] Allow spell effect 86 - Activate Object to use the misc value
 * [c12519] Allow player pets to swim
 * [c12518] Enable resummoning of warlock pets
 * [c12517] Do not remove FLY auras on Evade
 * [c12516] EventAI: Improve code
 * [c12515] EventAI: Implement ACTION_T_SET_THROW_MASK (46)
 * [c12514] Forward original caster GUID to script library
 * [c12513] Fixup commit 12511 Thanks to Zakamurite for pointing
 * [c12512] Implement some spells for Felmyst encounter
 * [12512] Fix invisible spirit healers & such on death near them
 * [c12510] Fix take ammo for most ranged spells
 * [c12509] Check cast spell 51690
 * [c12508] Fix SpellDamage modifier of SPELL_AURA_MOD_DAMAGE_DONE_CREATURE
 * [c12507] Implement proc effect of spells 67712, 67758
 * [c12506] Improve proc of spell 50421
 * [c12505] Add and implement server-side spell 23770
 * [c12504] EventAI: Improve TargetSelection related ErrorLog output
 * [c12503] DBScripts Engine: Change behaviour to search for a different npc when using buddy-search
 * [c12502] DBScripts Engine: Allow pets as buddy
 * [c12501] DBScripts Engine: Support buddy search by guid
 * [12500] Remove a few empty lines
 * [c12498] Fix a typo for spell 37096
 * [12497] Zipped up very old SQL updates
 * [c12496] Add some doxygen documentation
 * [c12495] Implement some spells for Ahune encounter
 * [c12494] Fix SMSG_BINDPOINTUPDATE and SMSG_PLAYERBOUND sent data
 * [c12493] Fix (at least) two false positive startup errors
 * [c12492] Fix Spell 61254
 * [c12491] Add additional miscValue to AIEvent throwing
 * [c12490] Keep CombatMovement, Running and Waypoint-Paused states after evade
 * Stop movement of dead npcs
 * Change Debug Output to EventAIError output if no target is found
 * [c12489] Add visual vomiting to Spell::EffectInebriate
 * Merge branch 'Rel18' of http://github.com/mangostwo/server into Rel18
 * minor text cleanup
 * Rolled version from 0.17 to 0.18 for Release 18.
 * [12488] Revert previous commit (see Notes)
 * [c12487] Improve alcohol handling
 * [c12486] Fix guid sent in SMSG_PLAYERBOUND, it should be caster's guid, not player's
 * [c12485] Improve WaypointMMGen
 * [c12484] Fix a stupid mistake from me introduced in [c12472]
 * [c12483] Improvements for NPC summoning code
 * [c12482] Implement target limitation for spell 72254
 * [c12481] Fix a bug with vmap extraction
 * [c12480] Improve unsummoning of wild-summoned GOs
 * [c12479] Enable using table creature_linking with empty table creature_linking_template
 * [c12478] Add some safety to rare case of target-selection for unreachable
 * [c12477] Fix a VC90 compile problem. Also add some const correctness
 * [12476] Add support for client 1.12.3 (build 6141) to realmd
 * [c12475] Add ACTION_T_THROW_AI_EVENT and EVENT_T_RECEIVE_AI_EVENT
 * [c12474] AI-Event throwing and receiving implementation
 * [c12473] Fix MoveGen's interrupting after last commit
 * [c12472] Rework StopMove to not start movement if already stopped
 * [c12471] Add an additional wrapper to SetRespawnCoord
 * [12470] Let Taunt behave more expectedly in case we have non-generic
 * [c12469] Improve Waypoint Management
 * [c12468] Use normal cast time for triggered spells by default
 * [c12467] Add GO-Casting to SCRIPT_COMMAND_CAST_SPELL
 * Add commented out helper to assign zone/area to gameobject by guid
 * [c12466] Implement spells 48129, 60320, 60321
 * [c12465] Add serverside spell 50574
 * Cleanup and text corrections
 * [12464] Implement spell 57578
 * [12463] Handle GO Trap triggering like GO handling is expected
 * [12462] Fix order of gossip menu options. Thanks to X-Savior for testing
 * [12461] Add stacking exception for spells 57494 and 57492
 * add new mangos icons created by zlxppp thank you. NOTE:     http://getmangos.eu/bb/post/8015
 * [12460] GO_FLAG_INTERACT_COND should not reset in instances
 * [12459] Add positive exception for spell 38449
 * [12458] Implement some spells used in Sunwell Plateau
 * [12457] Change a default config setting to more fitting
 * [12456] Implement new FlyOrLandMovementGenerator
 * [12455] Simplify use of CombatReach
 * Add some Debug log output to MapPersistantStateMgr deconstructor
 * [12454] Implement spell 33676
 * [12453] Fix Claw Rage spells
 * [12452] Implement some spells for Black Temple
 * [12451] Implement virtual void HealedBy
 * [12450] Implement new TempSummon types
 * [12449] Fix send-speed opcode sending
 * [12448] Fix typo. Thanks also to vladimir for pointing
 * [12447] Implement SCRIPT_COMMAND_TERMINATE_COND
 * [12446] Implement CONDITION_DEAD_OR_AWAY
 * [12445] Add const Group member iteration
 * [12444] Remove dropped SingletonImpl.h file from VC project files
 * [12443] Prevent annoying client freezes
 * [12442] Reorder handling in Creature::SetDeathState(JUST_ALIVE) case
 * [12441] Add condition_id support to npc_spellclick_spells
 * [12440] Add support for condition_id to spell_area table
 * [12439] Implement spells 19869 and 20037
 * [12438] Limit targets of spell 10258
 * Change default reaction on aggro for non-combat movement mobs
 * [12437] Implement spell 24324
 * [12436] Correct file version no. in VC110 sln files
 * [12435] Fix a mistake from [12432]
 * [12434] Add support for server-side spells attr, attr_ex, attr_ex2 and effect0_target_b columns
 * [12433] Add support for the Rabbit Day
 * [12432] Do more checks if a Player can enter a map On Login
 * [12431] Use proper error files for missing string errors
 * [12430] Implement spells 50630, 70623 and 70638
 * [12429] Fix possible memory corruption related to loading pet-spells
 * [12428] Add delayed starting for mmap extraction to ExtractResources script
 * [12427] Implement spell 17009
 * [12425] Implement spells 68987 and 69012
 * [12425] Implement CONDITION_GENDER Thanks to crackm for testing
 * [12424] Add support for XP-Gain disabling
 * [12423] Implement spell 56072
 * [12422] Fix possible crash due to [12409]
 * [12421] Add negative exception for spells 57508 - 57512
 * [12420] Implement spells 53035, 53036 and 53037
 * [12419] Avoid creature not respawning when not looted.
 * [12418] CMake: strip the trailing newline from ${GIT_RESULT}
 * [12417] CMake cleanup
 * Fix typos, Thanks to Vladimir for pointing
 * [12416] Improve debug log output for learning/unlearning spells/skills. Thanks to NeatElves for pointing
 * [12415] Improve target selection for TARGET_BEHIND_VICTIM
 * [12414] Add stacking exception for spells 53456 and 53421
 * [12413] Allow spell effect 77 (ScriptEffect) and aura 226 (PeriodicDummy) to be handled by script library
 * [12412] Add stacking exception for spells 50442 and 47941
 * [12411] minor CMake cleanup
 * [12410] Fix TARGET_TOTEM_* positions
 * [12409] Implement spells 47958, 57082 and 57083
 * [12408] Correct a little typo in mangos.conf about MapsLoading log filter.
 * [12407] Add stacking exception for spells 51019 and 51024
 * [12406] Fix a Typo
 * [12401] Add server-side spells 37264, 37278 and 37365
 * [12404] Cleanup Style
 * Escape some file names in the cleanupStyle helper script
 * [12403] Add support for spell 62941
 * [12402] DBScripts - Add Map/Zone wide sound support to COMMAND_PLAY_SOUND
 * [12401] Allow stacking for PERIODIC_DUMMY auras from different casters
 * [12400] Fix some summon spells which have only coordinate targeting
 * [12399] Refactor custom maxTargets and Radius into own function
 * [12398] Remove the PREFIX parameter from the CMake files and use the standard CMAKE_INSTALL_PREFIX parameter instead
 * [12397] Do not use GLOB in CMake for source file lists
 * [12396] Fix Typo from 10727. Special thanks to Morenn for spotting
 * [12395] Implement spell effects 50742, 50810 and 61546
 * [12394] Note ingame if gossip conditions are ignored because of GM mode
 * [12393] Use Filter log for grid unloading messages
 * [12392] Fix recently un-outcommented code to be actually working
 * [12391] Looks for passive auras that need to be recast. Like talent spell Feral Swiftness (id: 17002 and ranks), once you go inside a building and then outside again.
 * [12390] Increase healing factor with each critical strike. Patch 3.0.2
 * [12389] Fix behavior of spell 42631
 * [12388] Improve [12378]. Thanks to rsa for pointing
 * [12387] Fix CMake ExternalProject_Add command for ACE on UNIX systems
 * [12386] EventAI - Define way mobs with difficulty flags behave outside of dungeons
 * [12385] Lost git_id_vc110.vcxproj.filters
 * [12384] Fixed build in release mode for vs2012.
 * [12383] Revert "[12376] Add continued integration support using Travis CI"
 * [12382] Fix some signed/unsigned integer comparison warnings
 * [12381] Fix compiler warning about initialization order
 * [12380] Don't send Spell::CheckCast fail result for passive auras
 * [12379] Use recently introduced performance timer for additional state updates
 * [12378] Implement spell effects for spells 74452 and 74455
 * [12377] Add support for VS2012 building.
 * [12376] Add continued integration support using Travis CI
 * [12375] Hack TBB code for support VS2012 (and new C++ standard in fact)
 * Remove shapeshift forced object update hack
 * Cleanup battleground code
 * [12374] fix clang compile error from call of templated member function
 * [12373] fix clang compile error from implicit type conversion
 * [12372] fix Singleton compile error on clang
 * Remove another redundant InBattleGround() check
 * [12371] Some small changes
 * [12370] Add condition support to vendors
 * Fix a comment of [12358]. Thx to Reamer for pointing
 * Fix a small issue for [12361]
 * [12369] Implement water script support for SSC water
 * [12368] Further fixes for extract tools. Thanks danielsreichenbach The patch will allow map files to be extracted with binaries built on Linux, since they currently do fail when extraction maps, since Linux is case-sensitive when it comes to file names.
 * [12367] Fix build on linux systems.
 * [12366] Fix spell 24834
 * [12365] Rename to new and clear defined tempsummon types
 * Introduce and clarify new TEMPSUMMON types
 * [12364] Add support to pass spell_script_target depending on effect index
 * [12363] Use default SQL storage for spell_script_target
 * [12362] Some code cleanup
 * [12361] Fix logic mistake in loading respawn times. Thx to xfurry for pointing
 * [12360] use the absolut path for CMAKE_INSTALL_PREFIX
 * [12359] CMake build fixes Thanks danielsreichenbach
 * [12358] Add some creature_template level checks to avoid client crashes with invalid values
 * [12357] Tyo Thanks linflaas close #1
 * [12356] Forgot to Update
 * [12355] Update *Orginal* MaNGOS Copyright to 2013 Thanks Antz
 * [12354] Revert "Initial Commit for C(ontinued)-MaNGOS"
 * [12353] Revert "[12114] Improve EventAI documentation file"
 * [12352] Revert "Improve readme"
 * [12351] Revert "[12325] Happy New Year 2013"
 * [12350] Calendar, correct a server freeze when gquit. Olso correct a memleak and optimise a little bit CalendarEventInvite handler by not send sql request when player is online to get his ignored char list.
 * [12349] Implement DBScript support for 'on creature death' events
 * [12348] Add usage info directly in movemapgen
 * [12347] Introduce new EventAI Error Log File. Update your config
 * [12346] Improve MMap File loading related log output
 * [12345] Add forgeted quote to sql request.
 * [12344] More correct help string for .wp show command.
 * [12343] Fix NoPCH build.
 * Improve code a little bit. Thanks again to Vladimir
 * [12342] Fix format-strings. Thanks to Vladimir for pointing
 * [12341] Fix compile for vmap_assembler and mmap generator.
 * [12340] Fix build
 * [12339] Calendar, some hotfixes. Correct player guid usage for sql request. (use GetCounter() instead of GetString()) Correct CMSG_CALENDAR_REMOVE_EVENT handler. Correct some possible crash. Use a better way to add element in EventsStore map. Correct sql update and revision.
 * [12338] Calendar implementation.
 * [12337] More check on raid reset loading. If db reset time is corrupted and/or set in far future it was never been updated.
 * [12336] Add debug log filter for MAP, VMAP, MMAP loading. Olso prepared log filter for EVENT_AI.
 * [12335] Fix loading Respawntimes on server start for continents.
 * [12334] Send changed fields even if they were changed back
 * [12333] Add some taxi flying spell cast exceptions
 * [12332] Prevent exploit for quest 11524
 * [12331] EventAI: Evaluate TargetTypes for ACTION_T_CAST
 * [12330] Drop unused file. Caught by Vladimir
 * [12329] EventAI: Add new TargetTypes
 * [12328] Add Unit::FixateTarget
 * [12327] Implement Target Limitation of Bone Spike Spells of ICC
 * [12326] Keep comitter information within the cleanupHistory.sh script
 * [12325] Happy New Year 2013
 * [12324] Add moving possibility check before start spline movement in PointMovementGenerator.
 * [12323] Remove creature from creature_linking table when deleting it.
 * Move all PCH-related settings to property sheet
 * [12322] Creature respawn position, use m_respawnPos as general respawn Position also for Creatures from DB. Now we can change Respawn Points in Script. Good for EscortAI and some Boss Scripts.
 * Merge pull request #43 from evil-at-wow/quick_fixes
 * [12321] Fix check for spell incoming interval calculation. Correct a comment too.
 * [12320] Fix build: nested classes shall obey the usual access rules, so SQLMSIteratorBounds could not access SQLMultiSIterator's private constructor, which is needed to construct the 'first' and 'second' members in its own constructor.
 * [12319] Fix Windows compile for Ad.exe
 * [12318] Improve processing of killed victim from Unit::DealDamage
 * Revert changes that should not have been in recent commit
 * [12317] Fix GCC compile. Thanks to LordJZ for input, and NEatElves for pointing
 * [12316] Add Unit::RemoveAllAurasOnEvade
 * [12315] Disable combat movement for npcs on vehicles
 * [12314] Move CombatMovement on/off setting to CreatureAI
 * [12313] Fix some issues shown by Cppcheck
 * Fix compile
 * [12312] Implement CONDITION_LAST_WAYPOINT - Thanks to Grz3s for testing
 * [12311] Fix No-PCH compile
 * [12310] Hopefully really fix No-PCH compile. Thanks to rsa for pointing
 * [12309] Fix No-PCH compile. Also make some InstanceData Getter const
 * [12308] Use non-player conditions within the loot system
 * [12307] Add support for 'conditioned references'
 * [12306] Implement CONDITION_SOURCE_AURA to check if the source of a condition has an aura
 * [12305] Change CONDITION_INSTANCE_SCRIPT and related InstanceData hook
 * [12304] Support non-player conditions
 * [12303] Fix client side InCombat state bug
 * [12302] Unify Received opcode messages
 * [12301] Let traps trigger when any unfriendly is nearby
 * [12300] Remove a few empty lines
 * [12299] Add support to pause Waypoint Movement
 * Fix possible glitch in recent commit. Thx to Grz3s for pointing
 * [12298] Add possibility to toggle WaypointMovement WaitTimer
 * [12297] DBScriptsEngine - Prevent multi execution for same scripts for most cases
 * [12296] Add new SCRIPT_COMMAND_TERMINATE_SCRIPT (31) command to DBScriptsEngine
 * [12295] Fixup DBScriptEngine Doc file. Thx to NeatElves for pointing
 * [12294] Implement target limitations for spells 62166, 63981
 * Update script_commands.txt documentation file. Thx to Grz3s for reminding
 * [12293] Ensure that IsAlive and IsDead return expected results
 * [12292] Add support to toggle UNIT_FLAGS with TemporaryFaction changes
 * [12291] Fix max targets for spell 64620
 * Improve recent crash fix with easier code
 * [12290] Implement Aura 48 as SPELL_AURA_PERIODIC_TRIGGER_BY_CLIENT
 * Improve readme
 * [12289] Fix a possible crash for reputation rewarding while rewarded player is teleported out of the map
 * [12288] Implement target 108 as TARGET_GO_IN_FRONT_OF_CASTER_90
 * [12287] Implement target limitation for Soulstorm (FoS) Spells
 * [12286] Let summoned traps trigger when there is any unfriendly unit nearby
 * [12285] Fix and improve last commit
 * [12284] Fix initialisation for far away GOs
 * [12283] Don't calculate fall damage while a player is on an elevator
 * [12282] Implement spells 63820, 64425 64620
 * [12281] Use SpellFocus check for all Spells
 * [12280] Add support for an own error file for scripting library
 * [12279] Drop temp CREATURE_FLAG_EXTRA_NO_TALKTO_CREDIT
 * [12278] Drop no more used creature_template.spellX fields
 * [12277] Implement SPELL_ATTR_EX2_IGNORE_LOS and SPELL_ATTR_EX_CANT_REFLECTED
 * [12276] Implement SPELL_ATTR_EX3_TARGET_ONLY_PLAYER (0x100)
 * Improve recent commit, Thanks to Zergtmn for the idea
 * [12275] Implement spells 52479, 52555
 * [12274] Implement spells 51904, 52694 for eye of acherus
 * Fix a typo
 * [12273] Rewrite GetHeight functions to return a reasonable height if possible
 * [12272] Fix crash with Destructible GO of MaxHealth
 * [12271] Fix creature spline relocation with ForceDespawn command

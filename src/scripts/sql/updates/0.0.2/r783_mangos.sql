UPDATE `creature_template` SET `ScriptName`='npc_twiggy_flathead' WHERE `entry`=6248;
DELETE FROM `areatrigger_scripts` WHERE `entry`=522;
INSERT INTO `areatrigger_scripts` VALUES (522,'at_twiggy_flathead');

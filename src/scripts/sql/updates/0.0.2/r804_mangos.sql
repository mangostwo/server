UPDATE `creature_template` SET `ScriptName`='npc_commander_dawnforge' WHERE `entry`=19831;

DELETE FROM `areatrigger_scripts` WHERE `entry`=4497;
INSERT INTO `areatrigger_scripts` VALUES (4497,'at_commander_dawnforge');

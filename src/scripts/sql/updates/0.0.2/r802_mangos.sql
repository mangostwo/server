UPDATE `creature_template` SET `ScriptName`='' WHERE `entry`=21807;

DELETE FROM `areatrigger_scripts` WHERE `entry`=4560;
INSERT INTO `areatrigger_scripts` VALUES (4560,'at_legion_teleporter');

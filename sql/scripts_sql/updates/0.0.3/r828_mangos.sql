UPDATE `creature_template` SET `ScriptName`='npc_grimstone' WHERE `entry`=10096;
UPDATE `creature_template` SET `ScriptName`='mob_phalanx' WHERE `entry`=9502;

DELETE FROM `areatrigger_scripts` WHERE `entry`=1526;
INSERT INTO `areatrigger_scripts` VALUES (1526,'at_ring_of_law');

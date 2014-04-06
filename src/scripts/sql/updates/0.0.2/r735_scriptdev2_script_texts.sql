DELETE FROM `script_texts` WHERE `entry` BETWEEN -1000105 AND -1000101;
INSERT INTO `script_texts` (`entry`,`content_default`,`sound`,`type`,`language`,`comment`) VALUES
(-1000101,'Follow me, $N. I\'ll take you to the Defias hideout. But you better protect me or I am as good as dead',0,0,7,'defias traitor SAY_START'),
(-1000102,'The entrance is hidden here in Moonbrook. Keep your eyes peeled for thieves. They want me dead.',0,0,7,'defias traitor SAY_PROGRESS'),
(-1000103,'You can go tell Stoutmantle this is where the Defias Gang is holed up, $N.',0,0,7,'defias traitor SAY_END'),
(-1000104,'%s coming in fast! Prepare to fight!',0,0,7,'defias traitor SAY_AGGRO_1'),
(-1000105,'Help!',0,0,7,'defias traitor SAY_AGGRO_2');

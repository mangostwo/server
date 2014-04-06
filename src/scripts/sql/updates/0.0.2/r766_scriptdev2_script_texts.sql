DELETE FROM `script_texts` WHERE `entry` BETWEEN -1189004 AND -1189000;
INSERT INTO `script_texts` (`entry`,`content_default`,`sound`,`type`,`language`,`comment`) VALUES
(-1189000,'Ah, I have been waiting for a real challenge!',5830,1,0,'herod SAY_AGGRO'),
(-1189001,'Blades of Light!',5832,1,0,'herod SAY_WHIRLWIND'),
(-1189002,'Light, give me strength!',5833,1,0,'herod SAY_ENRAGE'),
(-1189003,'Hah, is that all?',5831,1,0,'herod SAY_KILL'),
(-1189004,'becomes enraged!',0,2,0,'herod EMOTE_ENRAGE');

DELETE FROM `script_texts` WHERE `entry` IN (-1036000,-1036001);
INSERT INTO `script_texts` (`entry`,`content_default`,`sound`,`type`,`language`,`comment`) VALUES
(-1036000,'You there, check out that noise!',5775,1,7,'smite INST_SAY_ALARM1'),
(-1036001,'We\'re under attack! A vast, ye swabs! Repel the invaders!',5777,1,7,'smite INST_SAY_ALARM2');

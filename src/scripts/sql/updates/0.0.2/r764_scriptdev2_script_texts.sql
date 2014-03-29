DELETE FROM `script_texts` WHERE `entry`=-1544015;
INSERT INTO `script_texts` (`entry`,`content_default`,`sound`,`type`,`language`,`comment`) VALUES
(-1544015,'breaks free!',0,2,0,'magtheridon EMOTE_FREED');

UPDATE `script_texts` SET `type`=3 WHERE `entry`=-1544013;

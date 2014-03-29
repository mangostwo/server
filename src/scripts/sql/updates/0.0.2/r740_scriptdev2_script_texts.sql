DELETE FROM `script_texts` WHERE `entry` BETWEEN -1000110 AND -1000106;
INSERT INTO `script_texts` (`entry`,`content_default`,`sound`,`type`,`language`,`comment`) VALUES
(-1000106,'Everyone ready?',0,0,1,'torek SAY_READY'),
(-1000107,'Ok, Lets move out!',0,0,1,'torek SAY_MOVE'),
(-1000108,'Prepare yourselves. Silverwing is just around the bend.',0,0,1,'torek SAY_PREPARE'),
(-1000109,'Silverwing is ours!',0,0,1,'torek SAY_WIN'),
(-1000110,'Go report that the outpost is taken. We will remain here.',0,0,1,'torek SAY_END');

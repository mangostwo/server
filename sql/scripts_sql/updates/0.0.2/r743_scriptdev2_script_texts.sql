DELETE FROM `script_texts` WHERE `entry` BETWEEN -1000116 AND -1000111;
INSERT INTO `script_texts` (`entry`,`content_default`,`sound`,`type`,`language`,`comment`) VALUES
(-1000111, 'Our house is this way, through the thicket.', 0, 0, 7, 'magwin SAY_START'),
(-1000112, 'Help me!', 0, 0, 7, 'magwin SAY_AGGRO'),
(-1000113, 'My poor family. Everything has been destroyed.', 0, 0, 7, 'magwin SAY_PROGRESS'),
(-1000114, 'Father! Father! You\'re alive!', 0, 0, 7, 'magwin SAY_END1'),
(-1000115, 'You can thank $N for getting me back here safely, father.', 0, 0, 7, 'magwin SAY_END2'),
(-1000116, 'hugs her father.', 0, 2, 7, 'magwin EMOTE_HUG');

DELETE FROM `script_texts` WHERE `entry` BETWEEN -1000127 AND -1000123;
INSERT INTO `script_texts` (`entry`,`content_default`,`sound`,`type`,`language`,`comment`) VALUES
(-1000123, 'Ready when you are, $c.', 0, 0, 0, 'big will SAY_BIG_WILL_READY'),
(-1000124, 'The Affray has begun. $n, get ready to fight!', 0, 0, 0, 'twiggy SAY_TWIGGY_BEGIN'),
(-1000125, 'You! Enter the fray!', 0, 0, 0, 'twiggy SAY_TWIGGY_FRAY'),
(-1000126, 'Challenger is down!', 0, 0, 0, 'twiggy SAY_TWIGGY_DOWN'),
(-1000127, 'The Affray is over.', 0, 0, 0, 'twiggy SAY_TWIGGY_OVER');

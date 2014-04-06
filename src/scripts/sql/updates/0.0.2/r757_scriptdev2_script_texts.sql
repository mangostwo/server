DELETE FROM `script_texts` WHERE `entry` BETWEEN -1000117 AND -1000122;
INSERT INTO `script_texts` (`entry`,`content_default`,`sound`,`type`,`language`,`comment`) VALUES
(-1000117, 'Thank you for agreeing to help. Now, let\'s get out of here $N.', 0, 0, 1, 'wounded elf SAY_ELF_START'),
(-1000118, 'Over there! They\'re following us!', 0, 0, 1, 'wounded elf SAY_ELF_SUMMON1'),
(-1000119, 'Allow me a moment to rest. The journey taxes what little strength I have.', 0, 0, 1, 'wounded elf SAY_ELF_RESTING'),
(-1000120, 'Did you hear something?', 0, 0, 1, 'wounded elf SAY_ELF_SUMMON2'),
(-1000121, 'Falcon Watch, at last! Now, where\'s my... Oh no! My pack, it\'s missing! Where has -', 0, 0, 1, 'wounded elf SAY_ELF_COMPLETE'),
(-1000122, 'You won\'t keep me from getting to Falcon Watch!', 0, 0, 1, 'wounded elf SAY_ELF_AGGRO');

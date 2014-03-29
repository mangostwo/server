DELETE FROM script_texts WHERE entry BETWEEN -1632025 AND -1632000;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1632000,'More souls to power the engine!',0,1,0,0,'boss_bronjahm SAY_AGGRO_1'),
(-1632001,'Finally...a captive audience!',16595,1,0,0,'boss_bronjahm SAY_AGGRO_2'),
(-1632002,'Fodder for the engine!',16596,1,0,0,'boss_bronjahm SAY_SLAY_1'),
(-1632003,'Another soul to strengthen the host!',16597,1,0,0,'boss_bronjahm SAY_SLAY_2'),
(-1632004,'My soul for you, master.',16598,1,0,0,'boss_bronjahm SAY_DEATH'),
(-1632005,'The vortex of the harvested calls to you!',16599,1,0,0,'boss_bronjahm SAY_SOULSTORM'),
(-1632006,'I will sever the soul from your body!',16600,1,0,0,'boss_bronjahm SAY_CORRUPT_SOUL'),

(-1632007,'You dare look upon the host of souls?! I SHALL DEVOUR YOU WHOLE!',16884,1,0,0,'boss_devourer SAY_MALE_1_AGGRO'),
(-1632008,'You dare look upon the host of souls?! I SHALL DEVOUR YOU WHOLE!',16890,1,0,0,'boss_devourer SAY_FEMALE_AGGRO'),
(-1632009,'Damnation!',16885,1,0,0,'boss_devourer SAY_MALE_1_SLAY_1'),
(-1632010,'Damnation!',16891,1,0,0,'boss_devourer SAY_FEMALE_SLAY_1'),
(-1632011,'Damnation!',16896,1,0,0,'boss_devourer SAY_MALE_2_SLAY_1'),
(-1632012,'Doomed for eternity!',16886,1,0,0,'boss_devourer SAY_MALE_1_SLAY_2'),
(-1632013,'Doomed for eternity!',16892,1,0,0,'boss_devourer SAY_FEMALE_SLAY_2'),
(-1632014,'Doomed for eternity!',16897,1,0,0,'boss_devourer SAY_MALE_2_SLAY_2'),
(-1632015,'The swell of souls will not be abated! You only delay the inevitable!',16887,1,0,0,'boss_devourer SAY_MALE_1_DEATH'),
(-1632016,'The swell of souls will not be abated! You only delay the inevitable!',16893,1,0,0,'boss_devourer SAY_FEMALE_DEATH'),
(-1632017,'The swell of souls will not be abated! You only delay the inevitable!',16898,1,0,0,'boss_devourer SAY_MALE_2_DEATH'),
(-1632018,'SUFFERING! ANGUISH! CHAOS! RISE AND FEED!',16888,1,0,0,'boss_devourer SAY_MALE_1_SOUL_ATTACK'),
(-1632019,'SUFFERING! ANGUISH! CHAOS! RISE AND FEED!',16894,1,0,0,'boss_devourer SAY_FEMALE_SOUL_ATTACK'),
(-1632020,'SUFFERING! ANGUISH! CHAOS! RISE AND FEED!',16899,1,0,0,'boss_devourer SAY_MALE_2_SOUL_ATTACK'),
(-1632021,'Stare into the abyss, and see your end!',16889,1,0,0,'boss_devourer SAY_MALE_1_DARK_GLARE'),
(-1632022,'Stare into the abyss, and see your end!',16895,1,0,0,'boss_devourer SAY_FEMALE_DARK_GLARE'),
(-1632023,'%s begins to cast Mirrored Soul!',0,3,0,0,'boss_devourer EMOTE_MIRRORED_SOUL'),
(-1632024,'%s begins to Unleash Souls!',0,3,0,0,'boss_devourer EMOTE_UNLEASH_SOULS'),
(-1632025,'%s begins to cast Wailing Souls!',0,3,0,0,'boss_devourer EMOTE_WAILING_SOULS');

DELETE FROM script_texts WHERE entry IN (-1090026, -1090025);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1090025,'My machines are the future! They\'ll destroy you all!',5808,1,0,0,'thermaplugg  SAY_PHASE'),
(-1090026,'Explosions! MORE explosions! I\'ve got to have more explosions!',5809,1,0,0,'thermaplugg  SAY_BOMB');

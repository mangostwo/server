DELETE FROM script_texts WHERE entry BETWEEN -1552055 AND -1552031;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1552031,'It is unwise to anger me.',11086,1,0,0,'dalliah SAY_AGGRO'),
(-1552032,'Ahh... That is much better.',11091,1,0,0,'dalliah SAY_HEAL_1'),
(-1552033,'Ahh... Just what I needed.',11092,1,0,0,'dalliah SAY_HEAL_2'),
(-1552034,'Completely ineffective. Just like someone else I know.',11087,1,0,0,'dalliah SAY_KILL_1'),
(-1552035,'You chose the wrong opponent.',11088,1,0,0,'dalliah SAY_KILL_2'),
(-1552036,'I\'ll cut you to pieces!',11090,1,0,0,'dalliah SAY_WHIRLWIND_1'),
(-1552037,'Reap the Whirlwind!',11089,1,0,0,'dalliah SAY_WHIRLWIND_2'),
(-1552038,'Now I\'m really... angry...',11093,1,0,0,'dalliah SAY_DEATH'),

(-1552039,'Have you come to kill Dalliah? Can I watch?',11237,1,0,1,'soccothrates SAY_DALLIAH_AGGRO_1'),
(-1552040,'This may be the end for you, Dalliah. What a shame that would be.',11245,1,0,1,'soccothrates SAY_DALLIAH_TAUNT_1'),
(-1552041,'Facing difficulties, Dalliah? How nice.',11244,1,0,1,'soccothrates SAY_DALLIAH_TAUNT_2'),
(-1552042,'I suggest a new strategy, you draw the attackers while I gather reinforcements. Hahaha!',11246,1,0,1,'soccothrates SAY_DALLIAH_TAUNT_3'),
(-1552043,'Finally! Well done!',11247,1,0,66,'soccothrates SAY_DALLIAH_DEAD'),
(-1552044,'On guard!',11241,1,0,0,'soccothrates SAY_CHARGE_1'),
(-1552045,'Defend yourself, for all the good it will do...',11242,1,0,0,'soccothrates SAY_CHARGE_2'),
(-1552046,'Knew this was... the only way out',11243,1,0,0,'soccothrates SAY_DEATH'),
(-1552047,'Yes, that was quite satisfying',11239,1,0,0,'soccothrates SAY_KILL'),
(-1552048,'At last, a target for my frustrations!',11238,1,0,0,'soccothrates SAY_AGGRO'),

(-1552049,'Did you call on me?',11236,1,0,397,'soccothrates SAY_INTRO_1'),
(-1552050,'Why would I call on you?',0,1,0,396,'dalliah SAY_INTRO_2'),
(-1552051,'To do your heavy lifting, most likely.',0,1,0,396,'soccothrates SAY_INTRO_3'),
(-1552052,'When I need someone to prance around like an overstuffed peacock, I''ll call on you.',0,1,0,396,'dalliah SAY_INTRO_4'),
(-1552053,'Then I\'ll commit myself to ignoring you.',0,1,0,396,'soccothrates SAY_INTRO_5'),
(-1552054,'What would you know about commitment, sheet-sah?',0,1,0,396,'dalliah SAY_INTRO_6'),
(-1552055,'You\'re the one who should be-- Wait, we have company...',0,1,0,396,'soccothrates SAY_INTRO_7');

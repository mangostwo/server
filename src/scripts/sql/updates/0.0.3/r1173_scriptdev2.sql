DELETE FROM script_texts WHERE entry BETWEEN -1609024 AND -1609022;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1609022,'The Lich King will see his true champion on this day!',0,0,0,0,'dk_initiate SAY_DUEL_G'),
(-1609023,'You\'re going down!',0,0,0,0,'dk_initiate SAY_DUEL_H'),
(-1609024,'You don\'t stand a chance, $n',0,0,0,0,'dk_initiate SAY_DUEL_I');

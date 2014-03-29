DELETE FROM script_texts  WHERE entry IN (-1000590,-1000591,-1000592,-1000593);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000590,'Woot!',0,0,0,0,'Captive Child SAY_THANKS_1'),
(-1000591,'I think those weird bird guys were going to eat us. Gross!',0,0,0,0,'Captive Child SAY_THANKS_2'),
(-1000592,'Yay! We\'re free!',0,0,0,0,'Captive Child SAY_THANKS_3'),
(-1000593,'Gross!',0,0,0,0,'Captive Child SAY_THANKS_4');

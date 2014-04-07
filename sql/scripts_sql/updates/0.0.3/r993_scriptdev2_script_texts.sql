DELETE FROM script_texts WHERE entry IN (-1000325, -1000326);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000325, '"Threshwackonator First Mate unit prepared to follow"',0,2,0,0,'threshwackonator EMOTE_START'),
(-1000326, 'YARRR! Swabie, what have ye done?! He\'s gone mad! Baton him down the hatches! Hoist the mast! ARRRR! Every man for hi\'self!',0,0,7,0,'threshwackonator SAY_AT_CLOSE');

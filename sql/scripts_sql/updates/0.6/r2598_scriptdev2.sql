DELETE FROM script_texts WHERE entry BETWEEN -1556020 AND -1556016;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1556016,'No! How can this be?',0,1,0,0,'anzu SAY_INTRO_1'),
(-1556017,'Pain will be the price for your insolence! You cannot stop me from claiming the Emerald Dream as my own!',0,1,0,0,'anzu SAY_INTRO_2'),
(-1556018,'Awaken, my children and assist your master!',0,1,0,0,'anzu SAY_BANISH'),
(-1556019,'Your magics shall be your undoing... ak-a-ak...',0,4,0,0,'anzu SAY_WHISPER_MAGIC'),
(-1556020,'%s returns to stone.',0,2,0,0,'anzu EMOTE_BIRD_STONE');

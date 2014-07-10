DELETE FROM script_texts WHERE entry IN (-1209000, -1209001, -1209002, -1209003);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1209000,'How dare you enter my sanctum!',0,0,0,0,'zumrah SAY_INTRO'),
(-1209001,'Sands consume you!',5872,1,14,0,'zumrah SAY_AGGRO'),
(-1209002,'Fall!',5873,1,14,0,'zumrah SAY_KILL'),
(-1209003,'Come to me, my children!',0,0,8,0,'zumrah SAY_SUMMON');

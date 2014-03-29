DELETE FROM script_texts WHERE entry IN (-1309025,-1309026,-1309027);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1309025,'The brood shall not fall!',0,1,0,0,'marli SAY_TRANSFORM_BACK'),
(-1309026,'%s emits a deafening shriek',0,2,0,0,'jeklik SAY_SHRIEK'),
(-1309027,'%s begins to cast a Great Heal!',0,2,0,0,'jeklik SAY_HEAL');

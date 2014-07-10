DELETE FROM script_texts WHERE entry IN (-1000907, -1000908, -1000909);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000907,'%s sniffs at the air. A tuber is near!',0,2,0,0,'domesticated felboar EMOTE_SNIFF_AIR'),
(-1000908,'%s starts to dig.',0,2,0,0,'domesticated felboar EMOTE_START_DIG'),
(-1000909,'%s squeals with glee at its discovery.',0,2,0,0,'domesticated felboar EMOTE_SQUEAL');

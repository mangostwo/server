DELETE FROM script_texts WHERE entry BETWEEN -1000542 AND -1000541;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000541,'%s jumps into the moonwell and goes underwater...',0,2,0,0,'kitten EMOTE_SAB_JUMP'),
(-1000542,'%s follows $n obediertly.',0,2,0,0,'kitten EMOTE_SAB_FOLLOW');

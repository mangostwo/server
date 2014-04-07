DELETE FROM script_texts WHERE entry=-1000193;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000193,'%s looks down at the discarded necklace. In her sadness, the lady incants a glamour, which beckons forth Highborne spirits. The chamber resonates with their ancient song about the Sin\'dorei...',10896,2,1,0,'lady_sylvanas EMOTE_LAMENT_START');
UPDATE script_texts SET emote=16 WHERE entry=-1000197;

DELETE FROM script_texts WHERE entry BETWEEN -1576022 AND -1576021;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1576021,'%s opens a Chaotic Rift!',0,3,0,0,'anomalus EMOTE_OPEN_RIFT'),
(-1576022,'%s shields himself and divert his power to the rifts!',0,3,0,0,'anomalus EMOTE_SHIELD');

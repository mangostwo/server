DELETE FROM script_texts WHERE entry BETWEEN -1533133 AND -1533130;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1533130,'%s summons forth Skeletal Warriors!',0,3,0,0,'noth EMOTE_WARRIOR'),
(-1533131,'%s teleports to the balcony above!',0,3,0,0,'noth EMOTE_SKELETON'),
(-1533132,'%s raises more skeletons!',0,3,0,0,'noth EMOTE_TELEPORT'),
(-1533133,'%s teleports back into the battle!',0,3,0,0,'noth EMOTE_TELEPORT_RETURN');

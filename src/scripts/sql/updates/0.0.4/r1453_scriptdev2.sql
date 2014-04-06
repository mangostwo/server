UPDATE script_texts SET content_default='$N! I\'m watching you!' WHERE entry=-1309018;
UPDATE script_texts SET type=4 WHERE entry=-1309019;

DELETE FROM script_texts WHERE entry=-1309024;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1309024,'%s goes into a rage after seeing his raptor fall in battle!',0,2,0,0,'mandokir EMOTE_RAGE');

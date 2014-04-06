DELETE FROM script_texts WHERE entry=-1544016;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1544016,'%s is nearly free of his bonds!',0,2,0,0,'magtheridon EMOTE_NEARLY_FREE');
UPDATE script_texts SET type=6 WHERE entry IN (-1544000, -1544001, -1544002, -1544003, -1544004, -1544005);

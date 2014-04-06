DELETE FROM script_texts WHERE entry=-1000005;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000005,'%s goes into a frenzy!',0,3,0,0,'EMOTE_BOSS_GENERIC_FRENZY');

UPDATE script_texts SET content_default='REUSE ME', type=0, comment='REUSE ME' WHERE entry IN (-1000156, -1269018, -1469031, -1601013, -1409001, -1469002);

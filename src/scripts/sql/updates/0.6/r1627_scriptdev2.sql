UPDATE script_texts SET comment='heigan SAY_CHANNELING' WHERE entry=-1533116;
UPDATE script_texts SET comment='heigan SAY_TAUNT4' WHERE entry=-1533117;

DELETE FROM script_texts WHERE entry IN (-1533136,-1533137);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1533136,'%s teleports and begins to channel a spell!',0,3,0,0,'heigan EMOTE_TELEPORT'),
(-1533137,'%s rushes to attack once more!',0,3,0,0,'heigan EMOTE_RETURN');

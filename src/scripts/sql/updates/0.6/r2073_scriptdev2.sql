DELETE FROM script_texts WHERE entry IN (-1000664, -1000665);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000664,'The Destructive Ward gains in power.',0,5,0,0,'destructive ward SAY_WARD_POWERUP'),
(-1000665,'The Destructive Ward is fully charged!',0,5,0,0,'destructive ward SAY_WARD_CHARGED');

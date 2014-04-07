DELETE FROM script_texts WHERE entry IN (-1000551,-1000552);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000551,'The %s is angered by your request and attacks!',0,2,0,0,'woodlands EMOTE_AGGRO'),
(-1000552,'Breaking off a piece of its bark, the %s hands it to you before departing.',0,2,0,0,'woodlands EMOTE_CREATE');

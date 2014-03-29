DELETE FROM script_texts WHERE entry IN (-1601013, -1601025, -1601026);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1601013,'%s moves up the tunnel!',0,3,0,0,'hadronox EMOTE_MOVE_TUNNEL'),
(-1601025,'The gate has been breached! Quickly, divert forces to deal with these invaders!',0,1,0,0,'anub\'ar crusher SAY_AGGRO'),
(-1601026,'There\'s no time left! All remaining forces, attack the invaders!',0,1,0,0,'anub\'ar crusher SAY_SPECIAL');

DELETE FROM script_texts WHERE entry IN (-1000963,-1000964,-1000965);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000963,'%s looks at you for a moment, then motions for you to follow.',0,2,0,0,'cenarion sparrowhawk EMOTE_FOLLOW'),
(-1000964,'%s surveys the ground for the buried raven stones.',0,2,0,0,'cenarion sparrowhawk EMOTE_SURVEY'),
(-1000965,'%s locates a buried raven stone.',0,2,0,0,'cenarion sparrowhawk EMOTE_LOCATE');

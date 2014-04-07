DELETE FROM script_texts WHERE entry IN (-1000889, -1000890);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000889,'%s submits.',0,2,0,0,'grark EMOTE_SUBMIT'),
(-1000890,'You have come to play? Then let us play!',0,0,0,0,'grark SAY_AGGRO');

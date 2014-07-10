DELETE FROM script_texts where entry IN (-1000666, -1000667);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000666,'I can sense the SHADOW on your hearts. There can be no rest for the wicked!',0,1,0,0,'lethon SAY_LETHON_AGGRO'),
(-1000667,'Your wicked souls shall feed my power!',0,1,0,0,'lethon SAY_LETHON_SHADE');

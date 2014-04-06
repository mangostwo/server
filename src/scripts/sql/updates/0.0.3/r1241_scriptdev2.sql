DELETE FROM script_texts WHERE entry IN (-1000360, -1000361);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000360,'The strands of LIFE have been severed! The Dreamers must be avenged!',0,1,0,0,' ysondre SAY_AGGRO'),
(-1000361,'Come forth, ye Dreamers – and claim your vengeance!',0,1,0,0,' ysondre SAY_SUMMONDRUIDS');

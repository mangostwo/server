DELETE FROM script_texts WHERE entry IN (-1631197,-1631198,-1631199);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1631197,'Invocation of Blood jumps to %s!',0,3,0,0,'blood_princes EMOTE_INVOCATION'),
(-1631198,'%s begins casting Empowered Shock Vortex!',0,3,0,0,'valanar EMOTE_SHOCK_VORTEX'),
(-1631199,'%s speed toward $N!',0,3,0,0,'taldaram EMOTE_FLAMES');

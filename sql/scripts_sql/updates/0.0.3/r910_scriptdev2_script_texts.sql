UPDATE script_texts SET language=0 WHERE entry=-1000231;
UPDATE script_texts SET comment='garments SAY_COMMON_HEALED' WHERE entry=-1000231;
UPDATE script_texts SET content_default='Thank you! Thank you, $GPriest:Priestess;. Now I can take on those gnolls with your power to back me!' WHERE entry=-1000232;
UPDATE script_texts SET comment='garments SAY_DG_KEL_THANKS' WHERE entry=-1000232;
UPDATE script_texts SET comment='garments SAY_DG_KEL_GOODBYE' WHERE entry=-1000233;

DELETE FROM script_texts WHERE entry BETWEEN -1000263 AND -1000256;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000256,'Thank you! Thank you, $GPriest:Priestess;. Now I can take on those murlocs with the Light on my side!',0,0,7,1,'garments SAY_ROBERTS_THANKS'),
(-1000257,'Farewell to you, and may the Light be with you always.',0,0,7,3,'garments SAY_ROBERTS_GOODBYE'),
(-1000258,'Thank you! Thank you, $GPriest:Priestess;. Now I can take on those humans with your power to back me!',0,0,1,1,'garments SAY_KORJA_THANKS'),
(-1000259,'Farewell to you, and may our ancestors be with you always!',0,0,1,3,'garments SAY_KORJA_GOODBYE'),
(-1000260,'Thank you! Thank you, $GPriest:Priestess;. Now I can take on those wendigo with the Light on my side!',0,0,7,1,'garments SAY_DOLF_THANKS'),
(-1000261,'Farewell to you, and may the Light be with you always.',0,0,7,3,'garments SAY_DOLF_GOODBYE'),
(-1000262,'Thank you! Thank you, $GPriest:Priestess;. Now I can take on those corrupt timberlings with Elune\'s power behind me!',0,0,2,1,'garments SAY_SHAYA_THANKS'),
(-1000263,'Farewell to you, and may Elune be with you always.',0,0,2,3,'garments SAY_SHAYA_GOODBYE');

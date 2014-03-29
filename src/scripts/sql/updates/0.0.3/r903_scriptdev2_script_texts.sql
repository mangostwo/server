DELETE FROM script_texts WHERE entry BETWEEN -1000233 AND -1000231;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000231,'Ah, $GPriest:Priestess; you came along just in time. I appreciate it.',0,0,1,2,'SAY_DG_KEL_HEALED'),
(-1000232,'Thank you! Thank you, $GPriest:Priestess;. Now i can take on those gnolls with your power back to me!',0,0,1,1,'SAY_DG_KEL_THANKS'),
(-1000233,'Farewell to you, and may shadow always protect you!',0,0,1,3,'SAY_DG_KEL_GOODBYE');

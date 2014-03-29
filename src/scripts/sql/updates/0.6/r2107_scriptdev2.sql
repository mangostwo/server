DELETE FROM script_texts WHERE entry BETWEEN -1229003 AND -1229000;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1229000,'%s begins to regain its strength!',0,2,0,0,'pyroguard EMOTE_BEGIN'),
(-1229001,'%s is nearly at full strength!',0,2,0,0,'pyroguard EMOTE_NEAR'),
(-1229002,'%s regains its power and breaks free of its bonds!',0,2,0,0,'pyroguard EMOTE_FULL'),
(-1229003,'Ha! Ha! Ha! Thank you for freeing me, fools. Now let me repay you by charring the flesh from your bones.',0,1,0,0,'pyroguard SAY_FREE');

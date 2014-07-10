DELETE FROM script_texts WHERE entry IN (-1036002, -1036003);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1036002,'You land lubbers are tougher than I thought! I\'ll have to improvise!',5778,0,0,21,'smite SAY_PHASE_2'),
(-1036003,'D\'ah! Now you\'re making me angry!',5779,0,0,15,'smite SAY_PHASE_3');

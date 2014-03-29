DELETE FROM script_texts WHERE entry IN (-1000626,-1000627);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000626,'Intiating energy collection.',0,0,0,0,'depleted golem SAY_GOLEM_CHARGE'),
(-1000627,'Energy collection complete.',0,0,0,0,'depleted golem SAY_GOLEM_COMPLETE');

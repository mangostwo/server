DELETE FROM script_texts WHERE entry BETWEEN -1543025 AND -1543017;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1543017,'You have faced many challenges, pity they were all in vain. Soon your people will kneel to my lord!',10292,1,0,0,'vazruden SAY_INTRO'),
(-1543018,'Your time is running out!',10294,1,0,0,'vazruden SAY_AGGRO1'),
(-1543019,'You are nothing, I answer a higher call!',10295,1,0,0,'vazruden SAY_AGGRO2'),
(-1543020,'The Dark Lord laughs at you!',10296,1,0,0,'vazruden SAY_AGGRO3'),
(-1543021,'Is there no one left to test me?',10293,1,0,0,'vazruden SAY_TAUNT'),
(-1543022,'It is over. Finished!',10297,1,0,0,'vazruden SAY_KILL1'),
(-1543023,'Your days are done!',10298,1,0,0,'vazruden SAY_KILL2'),
(-1543024,'My lord will be the end you all...',10299,1,0,0,'vazruden SAY_DEATH'),
(-1543025,'%s descends from the sky.',0,3,0,0,'vazruden EMOTE_DESCEND');

UPDATE script_waypoint SET waittime=10000 WHERE entry=24358 AND pointid=3;
DELETE FROM script_waypoint WHERE entry=24358 AND pointid=4;
INSERT INTO script_waypoint VALUES (24358, 4, 120.536003, 1611.654663, 43.473, 0, '');

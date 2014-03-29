UPDATE sd2_db_version SET version='ScriptDev2 (for MaNGOS 10390+) ';

DELETE FROM script_texts WHERE entry BETWEEN -1560053 AND -1560050;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1560050,'That\'s enough out of him.',0,0,0,0,'thrall hillsbrad SAY_TH_KILL_ARMORER'),
(-1560051,'That spell should wipe their memories of us and what just happened. All they should remember now is what reality would be like without the attempted temporal interference. Well done. Thrall will journey on to find his destiny, and Taretha...',0,0,0,0,'erozion SAY_WIPE_MEMORY'),
(-1560052,'Her fate is regrettably unavoidable.',0,0,0,0,'erozion SAY_ABOUT_TARETHA'),
(-1560053,'They call you a monster. But they\'re the monsters, not you. Farewell Thrall.',0,0,0,0,'taretha SAY_TA_FAREWELL');

UPDATE script_texts SET type=0 WHERE entry = -1560023;
UPDATE script_texts SET type=0 WHERE entry BETWEEN -1560033 AND -1560031;
UPDATE script_texts SET type=0 WHERE entry BETWEEN -1560041 AND -1560038;
UPDATE script_texts SET type=0 WHERE entry BETWEEN -1560044 AND -1560042;
UPDATE script_texts SET type=0 WHERE entry BETWEEN -1560047 AND -1560045;

UPDATE script_waypoint SET waittime=15000 WHERE entry=17876 AND pointid=96;
UPDATE script_waypoint SET waittime=10000 WHERE entry=17876 AND pointid=97;

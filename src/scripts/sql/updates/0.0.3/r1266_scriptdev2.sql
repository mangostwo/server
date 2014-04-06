DELETE FROM script_texts WHERE entry=-1000410;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000410,'Here they come! Defend yourself!',0,0,1,5,'kanati SAY_KAN_START');

DELETE FROM script_waypoint WHERE entry=10638;
INSERT INTO script_waypoint VALUES
(10638, 0, -4903.521973, -1368.339844, -52.611, 5000, 'SAY_KAN_START'),
(10638, 1, -4906.004395, -1367.048096, -52.611, 0, '');

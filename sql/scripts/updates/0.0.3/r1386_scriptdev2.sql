DELETE FROM script_texts WHERE entry BETWEEN -1000500 AND -1000499;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000499,'Puny $r wanna fight %s? Me beat you! Me boss here!',0,0,1,0,'morokk SAY_MOR_CHALLENGE'),
(-1000500,'Me scared! Me run now!',0,1,0,0,'morokk SAY_MOR_SCARED');

DELETE FROM script_waypoint WHERE entry=4500;
INSERT INTO script_waypoint VALUES
(4500, 0, -3125.597168, -2885.673828, 34.731, 2500, ''),
(4500, 1, -3120.257080, -2877.830322, 34.917, 0, ''),
(4500, 2, -3116.487305, -2850.670410, 34.869, 0, ''),
(4500, 3, -3093.474854, -2819.189697, 34.432, 0, ''),
(4500, 4, -3104.726318, -2802.020996, 33.954, 0, ''),
(4500, 5, -3105.906006, -2780.234375, 34.469, 0, ''),
(4500, 6, -3116.080811, -2757.902588, 34.734, 0, ''),
(4500, 7, -3125.234375, -2733.960205, 33.189, 0, '');

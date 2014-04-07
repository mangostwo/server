DELETE FROM script_texts WHERE entry BETWEEN -1000962 AND -1000958;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000958,'You must protect me from monsters, who are living in this forest!',0,0,0,0,'stinky ignatz SAY_STINKY_BEGIN'),
(-1000959,'This part of forest are very danger for us. We must be a careful!',0,0,0,0,'stinky ignatz SAY_STINKY_FIRST_STOP'),
(-1000960,'Kill two monsters, who stay near Bogbean plant and then I gather a bogbean.',0,0,0,0,'stinky ignatz SAY_STINKY_2_MONSTERS'),
(-1000961,'I am gathering a bogbean. It takes some time.',0,0,0,69,'stinky ignatz SAY_STINKY_GATHERING'),
(-1000962,'Thanks you for help.',0,0,0,0,'stinky ignatz SAY_STINKY_END');

DELETE FROM script_waypoint WHERE entry=4880;
INSERT INTO script_waypoint VALUES
(4880, 0,  -2674.53, -3440.48, 33.686,  0, ''),
(4880, 1,  -2711.17, -3435.06, 33.1926, 0, ''),
(4880, 2,  -2734.04, -3456.12, 33.2254, 0, ''),
(4880, 3,  -2749.64, -3457.26, 32.8249, 0, ''),
(4880, 4,  -2762.25, -3457.77, 30.6813, 0, ''),
(4880, 5,  -2777.0,  -3456.12, 30.2484, 0, ''),
(4880, 6,  -2805.49, -3450.27, 29.0624, 0, ''),
(4880, 7,  -2809.77, -3447.14, 30.0948, 0, ''),
(4880, 8,  -2824,    -3440.62, 33.405,  0, ''),
(4880, 9,  -2840.2,  -3439.02, 34.1008, 0, ''),
(4880, 10, -2878.49, -3482.81, 34.362,  0, ''),
(4880, 11, -2878.35, -3511.51, 34.4826, 0, 'SAY_STINKY_FIRST_STOP'),
(4880, 12, -2873.99, -3514.84, 34.5298, 0, ''),
(4880, 13, -2866.71, -3519.06, 36.3674, 0, ''),
(4880, 14, -2850.75, -3539.38, 36.4573, 0, ''),
(4880, 15, -2844.49, -3557.7,  35.5588, 0, ''),
(4880, 16, -2841.36, -3574.59, 35.5056, 0, ''),
(4880, 17, -2841.13, -3596.95, 36.7699, 30000, 'SAY_STINKY_2_MONSTERS'),
(4880, 18, -2828.83, -3597.3,  31.2891, 0, ''),
(4880, 19, -2822.13, -3596.33, 31.2684, 5000, 'SAY_STINKY_GATHERING'),
(4880, 20, -2829.08, -3597.82, 31.307,  0, ''),
(4880, 21, -2859.28, -3602.33, 42.298,  0, ''),
(4880, 22, -2881.64, -3601.28, 42.2111, 0, ''),
(4880, 23, -2904.04, -3601.35, 34.969,  0, ''),
(4880, 24, -2907.6,  -3612.73, 34.2434, 10000, 'SAY_STINKY_END');

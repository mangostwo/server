DELETE FROM script_texts WHERE entry BETWEEN -1000788 AND -1000784;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000784,'Thanks $N. Now let\'s get out of here!',0,0,0,0,'melizza SAY_MELIZZA_START'),
(-1000785,'We made it! Thanks again! I\'m going to run ahead!',0,0,0,0,'melizza SAY_MELIZZA_FINISH'),
(-1000786,'Hey Hornizz! I\'m back! And there are some people behind me who helped me out of a jam.',0,0,0,1,'melizza SAY_MELIZZA_1'),
(-1000787,'We\'re going to have to scratch the Maraudines off our list. Too hard to work with...',0,0,0,1,'melizza SAY_MELIZZA_2'),
(-1000788,'Well, I\'m off to the Gelkis. They\'re not as dumb as the Maraudines, but they\'re more reasonable.',0,0,0,3,'melizza SAY_MELIZZA_3');

DELETE FROM script_waypoint WHERE entry=12277;
INSERT INTO script_waypoint VALUES
(12277, 1, -1154.87, 2708.16, 111.123, 1000, 'SAY_MELIZZA_START'),
(12277, 2, -1162.62, 2712.86, 111.549, 0, ''),
(12277, 3, -1183.37, 2709.45, 111.601, 0, ''),
(12277, 4, -1245.09, 2676.43, 111.572, 0, ''),
(12277, 5, -1260.54, 2672.48, 111.55, 0, ''),
(12277, 6, -1272.71, 2666.38, 111.555, 0, ''),
(12277, 7, -1342.95, 2580.82, 111.557, 0, ''),
(12277, 8, -1362.24, 2561.74, 110.848, 0, ''),
(12277, 9, -1376.56, 2514.06, 95.6146, 0, ''),
(12277, 10, -1379.06, 2510.88, 93.3256, 0, ''),
(12277, 11, -1383.14, 2489.17, 89.009, 0, ''),
(12277, 12, -1395.34, 2426.15, 88.6607, 0, 'SAY_MELIZZA_FINISH'),
(12277, 13, -1366.23, 2317.17, 91.8086, 0, ''),
(12277, 14, -1353.81, 2213.52, 90.726, 0, ''),
(12277, 15, -1354.19, 2208.28, 88.7386, 0, ''),
(12277, 16, -1354.59, 2193.77, 77.6702, 0, ''),
(12277, 17, -1367.62, 2160.64, 67.1482, 0, ''),
(12277, 18, -1379.44, 2132.77, 64.1326, 0, ''),
(12277, 19, -1404.81, 2088.68, 61.8162, 0, 'SAY_MELIZZA_1'),
(12277, 20, -1417.15, 2082.65, 62.4112, 0, ''),
(12277, 21, -1423.28, 2074.19, 62.2046, 0, ''),
(12277, 22, -1432.99, 2070.56, 61.7811, 0, ''),
(12277, 23, -1469.27, 2078.68, 63.1141, 0, ''),
(12277, 24, -1507.21, 2115.12, 62.3578, 0, '');

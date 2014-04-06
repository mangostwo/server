DELETE FROM script_texts WHERE entry BETWEEN -1000481 AND -1000477;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000477,'Let us leave this place. I\'ve had enough of these madmen!',0,0,0,0,'akuno SAY_AKU_START'),
(-1000478,'You\'ll go nowhere, fool!',0,0,0,0,'akuno SAY_AKU_AMBUSH_A'),
(-1000479,'Beware! More cultists come!',0,0,0,0,'akuno SAY_AKU_AMBUSH_B'),
(-1000480,'You will not escape us so easily!',0,0,0,0,'akuno SAY_AKU_AMBUSH_B_REPLY'),
(-1000481,'I can find my way from here. My friend Mekeda will reward you for your kind actions.',0,0,0,0,'akuno SAY_AKU_COMPLETE');

DELETE FROM script_waypoint WHERE entry=22377;
INSERT INTO script_waypoint VALUES
(22377, 0, -2770.457520, 5418.410645, -34.538, 0, ''),
(22377, 1, -2778.180420, 5416.253906, -34.538, 0, ''),
(22377, 2, -2816.960449, 5414.944336, -34.529, 0, ''),
(22377, 3, -2827.533203, 5414.737305, -28.265, 0, ''),
(22377, 4, -2841.610596, 5413.021973, -28.261, 0, ''),
(22377, 5, -2863.605957, 5411.964355, -28.262, 1000, 'SAY_AKU_AMBUSH_A'),
(22377, 6, -2874.559570, 5413.799316, -28.260, 0, ''),
(22377, 7, -2878.775879, 5413.812012, -28.261, 0, ''),
(22377, 8, -2892.586914, 5413.478516, -18.784, 0, ''),
(22377, 9, -2896.040527, 5413.137207, -18.589, 0, ''),
(22377, 10, -2896.318848, 5409.431641, -18.450, 0, ''),
(22377, 11, -2895.997803, 5396.909668, -8.855, 0, ''),
(22377, 12, -2895.734131, 5386.623535, -9.260, 0, ''),
(22377, 13, -2895.318359, 5367.613281, -9.456, 0, ''),
(22377, 14, -2890.306641, 5353.883301, -11.280, 1000, 'SAY_AKU_AMBUSH_B'),
(22377, 15, -2880.419189, 5334.625977, -10.629, 0, ''),
(22377, 16, -2866.394043, 5314.253906, -9.678, 0, ''),
(22377, 17, -2864.753174, 5277.734375, -11.087, 0, ''),
(22377, 18, -2856.330322, 5255.902344, -11.496, 5000, 'SAY_AKU_COMPLETE');

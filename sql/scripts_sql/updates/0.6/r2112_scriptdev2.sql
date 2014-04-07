DELETE FROM script_texts WHERE entry BETWEEN -1129012 AND -1129005;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1129005,'All right, stay close. These fiends will jump right out of the shadows at you if you let your guard down.',0,0,0,0,'belnistrasz SAY_READY'),
(-1129006,'Okay, here we go. It\'s going to take about five minutes to shut this thing down through the ritual. Once I start, keep the vermin off of me or it will be the end of us all!',0,0,0,0,'belnistrasz SAY_START_RIT'),
(-1129007,'You\'ll rue the day you crossed me, $N',0,0,0,0,'belnistrasz SAY_AGGRO_1'),
(-1129008,'Incoming $N - look sharp, friends!',0,0,0,0,'belnistrasz SAY_AGGRO_2'),
(-1129009,'Three minutes left -- I can feel the energy starting to build! Keep up the solid defense!',0,1,0,0,'belnistrasz SAY_3_MIN'),
(-1129010,'Just two minutes to go! We\'re half way there, but don\'t let your guard down!',0,1,0,0,'belnistrasz SAY_2_MIN'),
(-1129011,'One more minute! Hold on now, the ritual is about to take hold!',0,1,0,0,'belnistrasz SAY_1_MIN'),
(-1129012,'That\'s it -- we made it! The ritual is set in motion, and idol fires are about to go out for good! You truly are the heroes I thought you would be!',0,1,0,4,'belnistrasz SAY_FINISH');

DELETE FROM script_waypoint WHERE entry = 8516;
INSERT INTO script_waypoint VALUES
(8516, 1,2603.18, 725.259, 54.6927, 0, ''),
(8516, 2,2587.13, 734.392, 55.231, 0, ''),
(8516, 3,2570.69, 753.572, 54.5855, 0, ''),
(8516, 4,2558.51, 747.66, 54.4482, 0, ''),
(8516, 5,2544.23, 772.924, 47.9255, 0, ''),
(8516, 6,2530.08, 797.475, 45.97, 0, ''),
(8516, 7,2521.83, 799.127, 44.3061, 0, ''),
(8516, 8,2502.61, 789.222, 39.5074, 0, ''),
(8516, 9,2495.25, 789.406, 39.499, 0, ''),
(8516, 10,2488.07, 802.455, 42.9834, 0, ''),
(8516, 11,2486.64, 826.649, 43.6363, 0, ''),
(8516, 12,2492.64, 835.166, 45.1427, 0, ''),
(8516, 13,2505.02, 847.564, 47.6487, 0, ''),
(8516, 14,2538.96, 877.362, 47.6781, 0, ''),
(8516, 15,2546.07, 885.672, 47.6789, 0, ''),
(8516, 16,2548.02, 897.584, 47.7277, 0, ''),
(8516, 17,2544.29, 909.116, 46.2506, 0, ''),
(8516, 18,2523.60, 920.306, 45.8717, 0, ''),
(8516, 19,2522.69, 933.546, 47.5769, 0, ''),
(8516, 20,2531.63, 959.893, 49.4111, 0, ''),
(8516, 21,2540.23, 973.338, 50.1241, 0, ''),
(8516, 22,2547.21, 977.489, 49.9759, 0, ''),
(8516, 23,2558.75, 969.243, 50.7353, 0, ''),
(8516, 24,2575.60, 950.138, 52.8460, 0, ''),
(8516, 25,2575.60, 950.138, 52.8460, 0, '');

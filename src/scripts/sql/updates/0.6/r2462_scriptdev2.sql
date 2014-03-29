DELETE FROM script_texts WHERE entry BETWEEN -1000794 AND -1000789;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000789,'Well, now or never I suppose. Remember, once we get to the road safety, return to Terenthis to let him know we escaped.',0,0,0,0,'volcor SAY_START'),
(-1000790,'We made it, My friend. Remember to find Terenthis and let him know we\'re safe. Thank you again.',0,0,0,0,'volcor SAY_END'),
(-1000791,'Here they come.',0,0,0,0,'volcor SAY_FIRST_AMBUSH'),
(-1000792,'We can overcome these foul creatures.',0,0,0,0,'volcor SAY_AGGRO_1'),
(-1000793,'We shall earn our deaths at the very least!',0,0,0,0,'volcor SAY_AGGRO_2'),
(-1000794,'Don\'t give up! Fight, to the death!',0,0,0,0,'volcor SAY_AGGRO_3');

DELETE FROM script_waypoint WHERE entry=3692;
INSERT INTO script_waypoint VALUES
(3692, 1, 4608.54, -6.47, 69.69, 4000, 'SAY_START'),
(3692, 2, 4604.54, -5.17, 69.51, 0, ''),
(3692, 3, 4604.26, -2.02, 69.42, 0, ''),
(3692, 4, 4607.75, 3.79, 70.13, 1000, 'first ambush'),
(3692, 5, 4607.75, 3.79, 70.13, 0, 'SAY_FIRST_AMBUSH'),
(3692, 6, 4619.77, 27.47, 70.40, 0, ''),
(3692, 7, 4626.28, 42.46, 68.75, 0, ''),
(3692, 8, 4633.13, 51.17, 67.40, 0, ''),
(3692, 9, 4639.67, 79.03, 61.74, 0, ''),
(3692, 10, 4647.54, 94.25, 59.92, 0, 'second ambush'),
(3692, 11, 4682.08, 113.47, 54.83, 0, ''),
(3692, 12, 4705.28, 137.81, 53.36, 0, 'last ambush'),
(3692, 13, 4730.30, 158.76, 52.33, 0, ''),
(3692, 14, 4756.47, 195.65, 53.61, 10000, 'SAY_END');

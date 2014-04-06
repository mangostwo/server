DELETE FROM script_texts WHERE entry BETWEEN -1000904 AND -1000901;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000901,'Thanks, friend. Will you help me get out of here?',0,0,0,1,'vanguard SAY_VANGUARD_INTRO'),
(-1000902,'We\'re not too far from the Protectorate Watch Post, $N. This way!',0,0,0,1,'vanguard SAY_VANGUARD_START'),
(-1000903,'Commander! This fleshling rescued me!',0,0,0,0,'vanguard SAY_VANGUARD_FINISH'),
(-1000904,'%s salutes $N.',0,2,0,0,'vanguard EMOTE_VANGUARD_FINISH');

DELETE FROM script_waypoint WHERE entry=20763;
INSERT INTO script_waypoint VALUES
(20763, 0, 4084.092, 2297.254, 110.277, 0, ''),
(20763, 1, 4107.174, 2294.916, 106.625, 0, ''),
(20763, 2, 4154.129, 2296.789, 102.331, 0, ''),
(20763, 3, 4166.021, 2302.819, 103.422, 0, ''),
(20763, 4, 4195.039, 2301.094, 113.786, 0, ''),
(20763, 5, 4205.246, 2297.116, 117.992, 0, ''),
(20763, 6, 4230.429, 2294.642, 127.307, 0, ''),
(20763, 7, 4238.981, 2293.579, 129.332, 0, ''),
(20763, 8, 4250.184, 2293.272, 129.009, 0, ''),
(20763, 9, 4262.810, 2290.768, 126.485, 0, ''),
(20763, 10, 4265.845, 2278.562, 128.235, 0, ''),
(20763, 11, 4265.609, 2265.734, 128.452, 0, ''),
(20763, 12, 4258.838, 2245.354, 132.804, 0, ''),
(20763, 13, 4247.976, 2221.211, 137.668, 0, ''),
(20763, 14, 4247.973, 2213.876, 137.721, 0, ''),
(20763, 15, 4249.876, 2204.265, 137.121, 4000, ''),
(20763, 16, 4249.876, 2204.265, 137.121, 0, 'SAY_VANGUARD_FINISH'),
(20763, 17, 4252.455, 2170.885, 137.677, 3000, 'EMOTE_VANGUARD_FINISH'),
(20763, 18, 4252.455, 2170.885, 137.677, 5000, '');

DELETE FROM script_texts WHERE entry BETWEEN -1090023 AND -1090000 OR entry=-1090028;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1090000,'With your help, I can evaluate these tunnels.',0,0,0,1,'emi shortfuse SAY_START'),
(-1090001,'Let\'s see if we can find out where these Troggs are coming from.... and put a stop to the invasion!',0,0,0,1,'emi shortfuse SAY_INTRO_1'),
(-1090002,'Such devastation... what a horrible mess...',0,0,0,5,'emi shortfuse SAY_INTRO_2'),
(-1090003,'It\'s quiet here....',0,0,0,1,'emi shortfuse SAY_INTRO_3'),
(-1090004,'...too quiet.',0,0,0,1,'emi shortfuse SAY_INTRO_4'),
(-1090005,'Look! Over there at the tunnel wall!',0,0,0,25,'emi shortfuse SAY_LOOK_1'),
(-1090006,'Trogg incursion! Defend me while I blast the hole closed!',0,0,0,5,'emi shortfuse SAY_HEAR_1'),
(-1090007,'Get this, $n off of me!',0,0,0,0,'emi shortfuse SAY_AGGRO_1'),
(-1090008,'I don\'t think one charge is going to cut it. Keep fending them off!',0,0,0,0,'emi shortfuse SAY_CHARGE_1'),
(-1090009,'The charges are set. Get back before they blow!',0,0,0,5,'emi shortfuse SAY_CHARGE_2'),
(-1090010,'Incoming blast in 10 seconds!',0,1,0,5,'emi shortfuse SAY_BLOW_1_10'),
(-1090011,'Incoming blast in 5 seconds. Clear the tunnel! Stay back!',0,1,0,5,'emi shortfuse SAY_BLOW_1_5'),
(-1090012,'FIRE IN THE HOLE!',0,1,0,25,'emi shortfuse SAY_BLOW_1'),
(-1090013,'Well done! Without your help I would have never been able to thwart that wave of troggs.',0,0,0,4,'emi shortfuse SAY_FINISH_1'),
(-1090014,'Did you hear something?',0,0,0,6,'emi shortfuse SAY_LOOK_2'),
(-1090015,'I heard something over there.',0,0,0,25,'emi shortfuse SAY_HEAR_2'),
(-1090016,'More troggs! Ward them off as I prepare the explosives!',0,0,0,0,'emi shortfuse SAY_CHARGE_3'),
(-1090017,'The final charge is set. Stand back!',0,0,0,1,'emi shortfuse SAY_CHARGE_4'),
(-1090018,'10 seconds to blast! Stand back!!!',0,1,0,5,'emi shortfuse SAY_BLOW_2_10'),
(-1090019,'5 seconds until detonation!!!!!',0,1,0,5,'emi shortfuse SAY_BLOW_2_5'),
(-1090020,'Nice work! I\'ll set off the charges to prevent any more troggs from making it to the surface.',0,0,0,1,'emi shortfuse SAY_BLOW_SOON'),
(-1090021,'FIRE IN THE HOLE!',0,1,0,0,'emi shortfuse SAY_BLOW_2'),
(-1090022,'Superb! Because of your help, my people stand a chance of re-taking our beloved city. Three cheers to you!',0,0,0,0,'emi shortfuse SAY_FINISH_2'),

(-1090023,'We come from below! You can never stop us!',0,1,0,1,'grubbis SAY_GRUBBIS_SPAWN'),

(-1090028,'$n attacking! Help!',0,0,0,0,'emi shortfuse SAY_AGGRO_2');


DELETE FROM script_waypoint WHERE entry=7998;
INSERT INTO script_waypoint VALUES
(7998, 01, -510.1305, -132.6899, -152.5,    0, ''),
(7998, 02, -511.0994, -129.74,   -153.8453, 0, ''),
(7998, 03, -511.7897, -127.4764, -155.5505, 0, ''),
(7998, 04, -512.9688, -124.926,  -156.1146, 5000, ''),
(7998, 05, -513.9719, -120.2358, -156.1161, 0, ''),
(7998, 06, -514.3875, -115.1896, -156.1165, 0, ''),
(7998, 07, -514.3039, -111.4777, -155.5205, 0, ''),
(7998, 08, -514.8401, -107.6633, -154.8925, 0, ''),
(7998, 09, -518.9943, -101.4161, -154.6482, 27000, ''),
(7998, 10, -526.9984, -98.14884, -155.6253, 0, ''),
(7998, 11, -534.5686, -105.4101, -155.989,  30000, ''),
(7998, 12, -535.5336, -104.6947, -155.9713, 0, ''),
(7998, 13, -541.6304, -98.6583,  -155.8584, 25000, ''),
(7998, 14, -535.0923, -99.91748, -155.9742, 0, ''),
(7998, 15, -519.0099, -101.5097, -154.6766, 3000, ''),
(7998, 16, -504.4659, -97.84802, -150.9554, 30000, ''),
(7998, 17, -506.9069, -89.14736, -151.083,  23000, ''),
(7998, 18, -512.7576, -101.9025, -153.198, 0, ''),
(7998, 19, -519.9883, -124.8479, -156.128, 86400000, 'this npc should not reset on wp end');

DELETE FROM script_texts WHERE entry BETWEEN -1560058 AND -1560054;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1560054,'I\'m glad you\'re safe, Taretha. None of this would have been possible without your friends. They made all of this happen.',0,0,0,0,'thrall hillsbrad SAY_TR_GLAD_SAFE'),
(-1560055,'Thrall, I\'ve never met these people before in my life.',0,0,0,0,'taretha SAY_TA_NEVER_MET'),
(-1560056,'Then who are these people?',0,0,0,0,'thrall hillsbrad SAY_TR_THEN_WHO'),
(-1560057,'I believe I can explain everything to you two if you give me a moment of your time.',0,0,0,0,'erozion SAY_PRE_WIPE'),
(-1560058,'You have done a great thing. Alas, the young warchief\'s memory of these events must be as they originally were ... Andormu awaits you in the master\'s lair.',0,0,0,0,'erozion SAY_AFTER_WIPE');

DELETE FROM script_waypoint WHERE entry=17876 AND pointid IN (106,107,108);
INSERT INTO script_waypoint VALUES
(17876, 106, 2630.45, 674.420, 54.4943, 5000, 'when all dead and meet Taretha'),
(17876, 107, 2634.30, 661.698, 54.4147, 0, 'run off'),
(17876, 108, 2652.21, 644.396, 56.1906, 0, '');

DELETE FROM script_waypoint WHERE entry=18887 AND pointid IN (7,8,9,10,11);
INSERT INTO script_waypoint VALUES
(18887, 7, 2638.57, 671.231, 54.5200, 60000, ''),
(18887, 8, 2636.56, 679.894, 54.6595, 0, ''),
(18887, 9, 2640.79, 689.647, 55.3215, 0, ''),
(18887, 10, 2639.35, 706.777, 56.0667, 0, ''),
(18887, 11, 2617.70, 731.884, 55.5571, 0, '');

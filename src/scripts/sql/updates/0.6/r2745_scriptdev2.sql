DELETE FROM script_texts WHERE entry BETWEEN -1564134 AND -1564131;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1564131,'This door is all that stands between us and the Betrayer. Stand aside, friends.',0,0,0,1,'akama illidan SAY_OPEN_DOOR_1'),
(-1564132,'I cannot do this alone...',0,0,0,0,'akama illidan SAY_OPEN_DOOR_2'),
(-1564133,'You are not alone, Akama.',0,0,0,0,'spirit Udalo SAY_OPEN_DOOR_3'),
(-1564134,'Your people will always be with you!',0,0,0,0,'spirit Olum SAY_OPEN_DOOR_4');

DELETE FROM script_texts WHERE entry BETWEEN -1564122 AND -1564097;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1564097,'Akama. Your duplicity is hardly surprising. I should have slaughtered you and your malformed brethren long ago.',11463,1,0,0,'illidan SAY_ILLIDAN_SPEECH_1'),
(-1564098,'We\'ve come to end your reign, Illidan. My people and all of Outland shall be free!',11389,1,0,25,'akama(illidan) SAY_AKAMA_SPEECH_2'),
(-1564099,'Boldly said. But I remain unconvinced.',11464,1,0,6,'illidan SAY_ILLIDAN_SPEECH_3'),
(-1564100,'The time has come! The moment is at hand!',11380,1,0,22,'akama(illidan) SAY_AKAMA_SPEECH_4'),
(-1564101,'You are not prepared!',11466,1,0,406,'illidan SAY_ILLIDAN_SPEECH_5'),
(-1564102,'Is this it, mortals? Is this all the fury you can muster?',11476,1,0,0,'illidan SAY_ILLIDAN_SPEECH_6'),
(-1564103,'Their fury pales before mine, Illidan. We have some unsettled business between us.',11491,1,0,6,'maiev SAY_MAIEV_SPEECH_7'),
(-1564104,'Maiev... How is this even possible?',11477,1,0,1,'illidan SAY_ILLIDAN_SPEECH_8'),
(-1564105,'My long hunt is finally over. Today, Justice will be done!',11492,1,0,5,'maiev SAY_MAIEV_SPEECH_9'),
(-1564106,'Feel the hatred of ten thousand years!',11470,1,0,0,'illidan SAY_FRENZY'),
(-1564107,'It is finished. You are beaten.',11496,1,0,0,'maiev SAY_MAIEV_EPILOGUE_1'),
(-1564108,'You have won... Maiev. But the huntress... is nothing without the hunt. You... are nothing... without me.',11478,1,0,0,'illidan SAY_ILLIDAN_EPILOGUE_2'),
(-1564109,'He\'s right. I feel nothing... I am... nothing.',11497,1,0,0,'maiev SAY_MAIEV_EPILOGUE_3'),
(-1564110,'Farewell, champions.',11498,1,0,0,'maiev SAY_MAIEV_EPILOGUE_4'),
(-1564111,'The Light will fill these dismal halls once again. I swear it.',11387,1,0,0,'akama(illidan) SAY_AKAMA_EPILOGUE_5'),
(-1564112,'I can feel your hatred.',11467,1,0,0,'illidan SAY_TAUNT_1'),
(-1564113,'Give in to your fear!',11468,1,0,0,'illidan SAY_TAUNT_2'),
(-1564114,'You know nothing of power!',11469,1,0,0,'illidan SAY_TAUNT_3'),
(-1564115,'Such... arrogance!',11471,1,0,0,'illidan SAY_TAUNT_4'),
(-1564116,'That is for Naisha!',11493,1,0,0,'maiev SAY_MAIEV_TAUNT_1'),
(-1564117,'Bleed as I have bled!',11494,1,0,0,'maiev SAY_MAIEV_TAUNT_2'),
(-1564118,'There shall be no prison for you this time!',11495,1,0,0,'maiev SAY_MAIEV_TRAP'),
(-1564119,'Meet your end, demon!',11500,1,0,0,'maiev SAY_MAIEV_TAUNT_4'),
(-1564120,'Be wary friends, The Betrayer meditates in the court just beyond.',11388,1,0,0,'akama(illidan) SAY_AKAMA_BEWARE'),
(-1564121,'Come, my minions. Deal with this traitor as he deserves!',11465,1,0,0,'illidan SAY_AKAMA_MINION'),
(-1564122,'I\'ll deal with these mongrels. Strike now, friends! Strike at the betrayer!',11390,1,0,22,'akama(illidan) SAY_AKAMA_LEAVE');

DELETE FROM gossip_texts WHERE entry IN (-3564001,-3564002);
INSERT INTO gossip_texts (entry,content_default,comment) VALUES
(-3564001,'I\'m ready, Akama.','akama(illidan) GOSSIP_ITEM_PREPARE'),
(-3564002,'We\'re ready to face Illidan.','akama(illidan) GOSSIP_ITEM_START_EVENT');

DELETE FROM script_waypoint WHERE entry=23089;
INSERT INTO script_waypoint VALUES
(23089, 0, 660.22, 305.74, 271.688, 0, 'escort paused - GOSSIP_ITEM_PREPARE'),
(23089, 1, 675.10, 343.30, 271.688, 0, ''),
(23089, 2, 694.01, 374.84, 271.687, 0, ''),
(23089, 3, 706.22, 375.75, 274.888, 0, ''),
(23089, 4, 720.48, 370.38, 281.300, 0, ''),
(23089, 5, 733.30, 357.66, 292.477, 0, ''),
(23089, 6, 740.40, 344.39, 300.920, 0, ''),
(23089, 7, 747.54, 329.03, 308.509, 0, ''),
(23089, 8, 748.24, 318.78, 311.781, 0, ''),
(23089, 9, 752.41, 304.31, 312.077, 0, 'escort paused - SAY_AKAMA_OPEN_DOOR_1'),
(23089, 10, 770.27, 304.89, 312.35, 0, ''),
(23089, 11, 780.18, 305.26, 319.71 , 0, ''),
(23089, 12, 791.45, 289.27, 319.80, 0, ''),
(23089, 13, 790.41, 262.70, 341.42, 0, ''),
(23089, 14, 782.88, 250.20, 341.60, 0, ''),
(23089, 15, 765.35, 241.40, 353.62, 0, ''),
(23089, 16, 750.61, 235.63, 353.02, 0, 'escort paused - GOSSIP_ITEM_START_EVENT'),
(23089, 17, 748.87, 304.93, 352.99, 0, 'escort paused - SAY_ILLIDAN_SPEECH_1'),
(23089, 18, 737.92, 368.15, 352.99, 0, ''),
(23089, 19, 749.64, 378.69, 352.99, 0, ''),
(23089, 20, 766.49, 371.79, 353.63, 0, ''),
(23089, 21, 784.98, 361.89, 341.41, 0, ''),
(23089, 22, 791.44, 347.10, 341.41, 0, ''),
(23089, 23, 794.80, 319.47, 319.75, 0, ''),
(23089, 24, 794.34, 304.34, 319.75, 0, 'escort paused - fight illidari elites'),
(23089, 25, 794.80, 319.47, 319.75, 0, ''),
(23089, 26, 791.44, 347.10, 341.41, 0, ''),
(23089, 27, 784.98, 361.89, 341.41, 0, ''),
(23089, 28, 766.49, 371.79, 353.63, 0, ''),
(23089, 29, 749.64, 378.69, 352.99, 0, ''),
(23089, 30, 737.92, 368.15, 352.99, 0, 'escort paused');

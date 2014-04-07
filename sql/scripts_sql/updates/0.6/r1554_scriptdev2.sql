DELETE FROM script_waypoint WHERE entry=30658;
INSERT INTO script_waypoint VALUES
(30658, 0, 1830.504517, 799.356506, 44.341801, 5000, 'use activation'),
(30658, 1, 1832.461792, 800.431396, 44.311745, 10000, 'SAY_BEGIN call back guards'),
(30658, 2, 1824.786987, 803.828369, 44.363434, 0, 'SAY_LOCK_DOOR close door'),
(30658, 3, 1807.245483, 803.904114, 44.363434, 0, ''),
(30658, 4, 1785.160400, 803.856873, 44.364830, 30000, '');

DELETE FROM script_texts WHERE entry BETWEEN -1608007 AND -1608000;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1608000,'Prison guards, we are leaving! These adventurers are taking over! Go, go, go!',0,1,0,0,'sinclair SAY_BEGIN'),
(-1608001,'I\'m locking the door. Good luck, and thank you for doing this.',0,0,0,0,'sinclair SAY_LOCK_DOOR'),
(-1608002,'Adventurers, the door is beinning to weaken!',0,1,0,0,'sinclair SAY_SEAL_75'),
(-1608003,'Only half of the door seal\'s strength remains! You must fight on!',0,1,0,0,'sinclair SAY_SEAL_50'),
(-1608004,'The door seal is about to collapse! All is lost if the Blue Dragonflight breaks through the door!',0,1,0,0,'sinclair SAY_SEAL_5'),
(-1608005,'A Portal Guardian defends the new portal!',0,3,0,0,'EMOTE_GUARDIAN_PORTAL'),
(-1608006,'An elite Blue Dragonflight squad appears from the portal!',0,3,0,0,'EMOTE_DRAGONFLIGHT_PORTAL'),
(-1608007,'A Guardian Keeper emerges from the portal!',0,3,0,0,'EMOTE_KEEPER_PORTAL');

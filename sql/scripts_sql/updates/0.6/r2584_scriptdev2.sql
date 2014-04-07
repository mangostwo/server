DELETE FROM script_texts WHERE entry IN (-1608000,-1608001,-1608027);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1608000,'Prison guards, we are leaving! These adventurers are taking over! Go, go, go!',0,1,0,0,'sinclari SAY_BEGIN'),
(-1608001,'I\'m locking the door. Good luck, and thank you for doing this.',0,0,0,1,'sinclari SAY_LOCK_DOOR'),
(-1608027,'You did it! You held the Blue Dragonflight back and defeated their commander. Amazing work!',0,0,0,1,'sinclari SAY_VICTORY');
DELETE FROM gossip_texts WHERE entry IN (-3608002);
INSERT INTO gossip_texts (entry,content_default,comment) VALUES
(-3608002,'I\'m not fighting, so send me in now!','sinclari GOSSIP_ITEM_TELEPORT');
DELETE FROM script_waypoint WHERE entry=30658;
INSERT INTO script_waypoint VALUES
(30658, 0, 1830.504517, 799.356506, 44.341801, 5000, 'use activation'),
(30658, 1, 1832.461792, 800.431396, 44.311745, 10000, 'SAY_BEGIN call back guards'),
(30658, 2, 1824.786987, 803.828369, 44.363434, 3000, 'SAY_LOCK_DOOR'),
(30658, 3, 1824.786987, 803.828369, 44.363434, 0, 'close door'),
(30658, 4, 1817.315674, 804.060608, 44.363998, 0, 'escort paused - allow teleport inside'),
(30658, 5, 1826.889648, 803.929993, 44.363239, 30000, 'SAY_VICTORY');

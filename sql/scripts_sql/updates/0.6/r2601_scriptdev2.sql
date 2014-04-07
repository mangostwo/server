DELETE FROM script_texts WHERE entry BETWEEN -1000824 AND -1000822;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000822,'The signal has been sent. He should be arriving shortly.',0,0,0,1,'squire rowe SAY_SIGNAL_SENT'),
(-1000823,'Yawww!',0,0,0,35,'reginald windsor SAY_DISMOUNT'),
(-1000824,'I knew you would come, $N. It is good to see you again, friend.',0,0,0,1,'reginald windsor SAY_WELCOME');
DELETE FROM gossip_texts WHERE entry=-3000106;
INSERT INTO gossip_texts (entry,content_default,comment) VALUES
(-3000106,'Let Marshal Windsor know that I am ready.','squire rowe GOSSIP_ITEM_WINDSOR');
DELETE FROM script_waypoint WHERE entry=17804;
INSERT INTO script_waypoint VALUES
(17804, 0, -9054.86, 443.58, 93.05, 0, ''),
(17804, 1, -9079.33, 424.49, 92.52, 0, ''),
(17804, 2, -9086.21, 419.02, 92.32, 3000, ''),
(17804, 3, -9086.21, 419.02, 92.32, 1000, ''),
(17804, 4, -9079.33, 424.49, 92.52, 0, ''),
(17804, 5, -9054.38, 436.30, 93.05, 0, ''),
(17804, 6, -9042.23, 434.24, 93.37, 5000, 'SAY_SIGNAL_SENT');

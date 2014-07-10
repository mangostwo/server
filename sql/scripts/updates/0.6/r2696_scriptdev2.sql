DELETE FROM script_texts WHERE entry=-1532115;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1532115,'Splendid, I\'m going to get the audience ready. Break a leg!',0,0,0,0,'barnes SAY_EVENT_START');
DELETE FROM gossip_texts WHERE entry IN (-3532000,-3532001,-3532002);
INSERT INTO gossip_texts (entry,content_default,comment) VALUES
(-3532000,'Teleport me to the Guardian\'s Library','berthold GOSSIP_ITEM_TELEPORT'),
(-3532001,'I\'m not an actor.','barnes GOSSIP_ITEM_OPERA_1'),
(-3532002,'Ok, I\'ll give it a try, then.','barnes GOSSIP_ITEM_OPERA_2');

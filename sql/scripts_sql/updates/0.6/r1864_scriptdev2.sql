DELETE FROM script_texts WHERE entry IN (-1000603, -1000604);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000603,'Do not test me, scurvy dog! I\'m trained in the way of the Blood Knights!',0,0,0,0,'silvermoon harry SAY_AGGRO'),
(-1000604,'I\'ll pay! I\'ll pay! Eeeek! Please don\'t hurt me!',0,0,0,0,'silvermoon harry SAY_BEATEN');

DELETE FROM gossip_texts WHERE entry IN (-3000101, -3000102);
INSERT INTO gossip_texts (entry,content_default,comment) VALUES
(-3000101,'Taruk send me to collect what you owe.','silvermoon harry GOSSIP_ITEM_GAMBLING_DEBT'),
(-3000102,'Here\'s the money.','silvermoon harry GOSSIP_ITEM_PAYING');

DELETE FROM script_texts WHERE entry=-1564130;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1564130,'Broken of the Ashtongue tribe, your leader speaks!',0,1,0,0,'akama shade SAY_FREE_1');
UPDATE script_texts SET sound=11386 WHERE entry=-1564013;
UPDATE script_texts SET sound=11385 WHERE entry=-1564014;
DELETE FROM gossip_texts WHERE entry=-3564000;
INSERT INTO gossip_texts (entry,content_default,comment) VALUES
(-3564000,'I\'m with you, Akama.','akama(shade) GOSSIP_ITEM_START_ENCOUNTER');

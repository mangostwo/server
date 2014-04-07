DELETE FROM script_texts WHERE entry BETWEEN -1554005 AND -1554000;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1554000,'REUSE_ME',0,0,0,0,'REUSE_ME'),
(-1554001,'REUSE_ME',0,0,0,0,'REUSE_ME'),
(-1554002,'REUSE_ME',0,0,0,0,'REUSE_ME'),
(-1554003,'REUSE_ME',0,0,0,0,'REUSE_ME'),
(-1554004,'REUSE_ME',0,0,0,0,'REUSE_ME'),
(-1554005,'REUSE_ME',0,0,0,0,'REUSE_ME');

DELETE FROM gossip_texts WHERE entry=-3000102;
INSERT INTO gossip_texts (entry,content_default,comment) VALUES
(-3000102,'Pay up, Harry!','silvermoon harry GOSSIP_ITEM_PAYING');

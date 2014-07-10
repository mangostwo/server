DELETE FROM script_texts WHERE entry IN (-1595000,-1595001);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1595000,'All soldiers of Lordaeron should immediately report to the entrance of Stratholme, and await further orders from Prince Arthas.',0,6,0,0,'lordaeron crier SAY_SOLDIERS_REPORT'),
(-1595001,'Good work with the crates! Come talk to me in front of Stratholme for your next assignment!',0,4,0,0,'chromie WHISPER_CHROMIE_CRATES');

DELETE FROM gossip_texts WHERE entry IN (-3595006,-3595007);
INSERT INTO gossip_texts (entry,content_default,comment) VALUES
(-3595006,'Chromie, you and I both know what''s going to happen in this time stream. We''ve seen this all before.$B$BCan you just skip us ahead to all the real action?','chromie GOSSIP_ITEM_INN_SKIP'),
(-3595007,'Yes, please!','chromie GOSSIP_ITEM_INN_TELEPORT');

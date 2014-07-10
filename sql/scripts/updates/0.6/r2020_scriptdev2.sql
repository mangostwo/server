DELETE FROM script_texts WHERE entry IN (-1409023, -1409024);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1409023,'Very well, $N.',0,0,0,0,'majordomo SAY_SUMMON_0'),
(-1409024,'Impudent whelps! You''ve rushed headlong to your own deaths! See now, the master stirs!',0,1,0,0,'majordomo SAY_SUMMON_1');

DELETE FROM gossip_texts WHERE entry IN (-3409000, -3409001, -3409002);
INSERT INTO gossip_texts (entry,content_default,comment) VALUES
(-3409000,'Tell me more.','majordomo_executus GOSSIP_ITEM_SUMMON_1'),
(-3409001,'What else do you have to say?','majordomo_executus GOSSIP_ITEM_SUMMON_2'),
(-3409002,'You challenged us and we have come. Where is this master you speak of?','majordomo_executus GOSSIP_ITEM_SUMMON_3');

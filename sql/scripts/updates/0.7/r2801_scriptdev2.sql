DELETE FROM gossip_texts WHERE entry IN (-3599000, -3599001);
INSERT INTO gossip_texts (entry,content_default,comment) VALUES
(-3599000,'Brann, it would be our honor!','brann GOSSIP_ITEM_ID_START'),
(-3599001,'Let\'s move Brann, enough of the history lessons!','brann GOSSIP_ITEM_ID_PROGRESS');

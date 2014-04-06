DELETE FROM gossip_texts WHERE entry IN (-3609000, -3609001);
INSERT INTO gossip_texts (entry,content_default,comment) VALUES
(-3609000,'I challenge you, death knight!','Death Knight Initiate GOSSIP_ITEM_ACCEPT_DUEL'),
(-3609001,'I am ready, Highlord. Let the siege of Light\'s Hope begin!','Highlord Darion Mograine GOSSIP_ITEM_READY');

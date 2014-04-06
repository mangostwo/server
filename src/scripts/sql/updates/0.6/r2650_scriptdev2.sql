DELETE FROM gossip_texts WHERE entry IN (-3000108,-3000110);
INSERT INTO gossip_texts (entry,content_default,comment) VALUES
(-3000108,'I need a moment of your time, sir.','prospector anvilward GOSSIP_ITEM_MOMENT'),
(-3000110,'Why... yes, of course. I\'ve something to show you right inside this building, Mr. Anvilward.','prospector anvilward GOSSIP_ITEM_SHOW');

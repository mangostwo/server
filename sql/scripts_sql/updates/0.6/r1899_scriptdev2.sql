DELETE FROM gossip_texts WHERE entry BETWEEN -3000108 AND -3000106;
INSERT INTO gossip_texts (entry,content_default,comment) VALUES
(-3000106,'Show me where I can fly.','greer orehammer GOSSIP_ITEM_TAXI'),
(-3000107,'[PH] Get Presicion Bombs','greer orehammer GOSSIP_ITEM_GET_BOMBS'),
(-3000108,'[PH] Start bombing mission','greer orehammer GOSSIP_ITEM_FLIGHT');

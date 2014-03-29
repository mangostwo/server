DELETE FROM gossip_texts WHERE entry BETWEEN -3603008 AND -3603000;
INSERT INTO gossip_texts (entry,content_default,comment) VALUES
(-3603000,'Teleport to the Expedition Base Camp.','GOSSIP_ITEM_TELE_BASE_CAMP'),
(-3603001,'Teleport to the Formation Grounds.','GOSSIP_ITEM_TELE_FORMATION_GROUNDS'),
(-3603002,'Teleport to the Colossal Forge.','GOSSIP_ITEM_TELE_COLOSSAR_FORGE'),
(-3603003,'Teleport to the Scrapyard.','GOSSIP_ITEM_TELE_SCRAPYARD'),
(-3603004,'Teleport to the Antechamber of Ulduar.','GOSSIP_ITEM_TELE_ANTECHAMBER'),
(-3603005,'Teleport to the Shattered Walkway.','GOSSIP_ITEM_TELE_WALKWAY'),
(-3603006,'Teleport to the Conservatory of Life.','GOSSIP_ITEM_TELE_CONSERVATORY'),
(-3603007,'Teleport to the Spark of Imagination.','GOSSIP_ITEM_TELE_SPARK_IMAGINATION'),
(-3603008,'Teleport to the Prison of Yogg-Saron.','GOSSIP_ITEM_TELE_YOGG_SARON');

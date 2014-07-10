DELETE FROM gossip_texts WHERE entry IN (-3603010,-3603011,-3603012);
INSERT INTO gossip_texts (entry,content_default,comment) VALUES
(-3603010,'Activate secondary defensive systems.','Lore Keeper of Norgannon GOSSIP_ITEM_ACTIVATE_SYSTEMS'),
(-3603011,'Confirmed.','Lore Keeper of Norgannon GOSSIP_ITEM_CONFIRMED'),
(-3603012,'We\'re ready. Begin the assault!','Brann Bronzebeard GOSSIP_ITEM_BEGIN_ASSAULT');

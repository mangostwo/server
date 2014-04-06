DELETE FROM gossip_texts WHERE entry=-3000000;
INSERT INTO gossip_texts (entry,content_default,comment) VALUES
(-3000000,'[PH] SD2 unknown text','GOSSIP_ID_UNKNOWN_TEXT');

DELETE FROM gossip_texts WHERE entry BETWEEN -3560006 AND -3560000;
INSERT INTO gossip_texts (entry,content_default,comment) VALUES
(-3560000,'I am ready to go to Durnholde Keep.','brazen GOSSIP_ITEM_READY'),
(-3560001,'I need a pack of Incendiary Bombs.','erozion GOSSIP_ITEM_NEED_BOMBS'),
(-3560002,'Taretha cannot see you, Thrall.','thrall GOSSIP_ITEM_SKARLOC1'),
(-3560003,'The situation is rather complicated, Thrall. It would be best for you to head into the mountains now, before more of Blackmoore\'s men show up. We\'ll make sure Taretha is safe.','thrall GOSSIP_ITEM_SKARLOC2'),
(-3560004,'We\'re ready, Thrall.','thrall GOSSIP_ITEM_TARREN'),
(-3560005,'Strange wizard?','taretha GOSSIP_ITEM_EPOCH1'),
(-3560006,'We\'ll get you out. Taretha. Don\'t worry. I doubt the wizard would wander too far away.','taretha GOSSIP_ITEM_EPOCH2');

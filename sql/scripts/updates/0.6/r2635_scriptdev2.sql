UPDATE script_texts SET type=6 WHERE entry=-1534018;
DELETE FROM gossip_texts WHERE entry IN (-3534000,-3534001,-3534002,-3534003,-3534004,-3534005,-3534006);
INSERT INTO gossip_texts (entry,content_default,comment) VALUES
(-3534000,'My companions and I are with you, Lady Proudmoore.','jaina GOSSIP_ITEM_JAIN_START'),
(-3534001,'We are ready for whatever Archimonde might send our way, Lady Proudmoore.','jaina GOSSIP_ITEM_ANATHERON'),
(-3534002,'Until we meet again, Lady Proudmoore.','jaina GOSSIP_ITEM_SUCCESS'),
(-3534003,'I am with you, Thrall.','thrall GOSSIP_ITEM_THRALL_START'),
(-3534004,'We have nothing to fear.','thrall GOSSIP_ITEM_AZGALOR'),
(-3534005,'Until we meet again, Thrall.','thrall GOSSIP_ITEM_SUCCESS'),
(-3534006,'I would be grateful for any aid you can provide, Priestess.','tyrande GOSSIP_ITEM_AID');

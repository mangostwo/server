UPDATE script_texts SET content_default='Run! They are coming!', type=1, comment='vaelastrasz blackwing tech SAY_INTRO_TECH' WHERE entry = -1469002;
UPDATE script_texts SET content_default='Fools! These eggs are more precious than you know!' WHERE entry = -1469023;
UPDATE script_texts SET content_default='No - not another one! I\'ll have your heads for this atrocity!' WHERE entry = -1469024;

DELETE FROM script_texts WHERE entry=-1469033;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1469033,'%s flee as the controlling power of the orb is drained.',0,2,0,0,'razorgore EMOTE_TROOPS_FLEE');

DELETE FROM gossip_texts WHERE entry IN (-3469003,-3469004);
INSERT INTO gossip_texts (entry,content_default,comment) VALUES
(-3469003,'I cannot, Vaelastrasz! Surely something can be done to heal you!','vaelastrasz GOSSIP_ITEM_VAEL_1'),
(-3469004,'Vaelastrasz, no!!!','vaelastrasz GOSSIP_ITEM_VAEL_2');

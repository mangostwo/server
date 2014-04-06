UPDATE script_texts SET comment='varian SAY_VARIAN_PVP_H_INTRO_2' WHERE entry=-1649024;
UPDATE script_texts SET comment='garrosh SAY_GARROSH_PVP_A_INTRO_2' WHERE entry=-1649025;
UPDATE script_texts SET content_default='GLORY TO THE ALLIANCE!' WHERE entry=-1649026;
UPDATE script_texts SET content_default='Ahhh, our guests have arrived, just as the master promised.' WHERE entry=-1649038;

UPDATE gossip_texts SET comment='barrett GOSSIP_ITEM_BEAST_INIT' WHERE entry=-3649000;
DELETE FROM gossip_texts WHERE entry BETWEEN -3649010 AND -3649001;
INSERT INTO gossip_texts (entry,content_default,comment) VALUES
(-3649001,'Bring forth the first challenge!','barrett GOSSIP_ITEM_BEAST_START'),
(-3649002,'We want another shot at those beasts!','barrett GOSSIP_ITEM_BEAST_WIPE_INIT'),
(-3649003,'What new challenge awaits us?','barrett GOSSIP_ITEM_JARAXXUS_INIT'),
(-3649004,'We\'re ready to fight the sorceror again.','barrett GOSSIP_ITEM_JARAXXUS_WIPE_INIT'),
(-3649005,'Of course!','barrett GOSSIP_ITEM_PVP_INIT'),
(-3649006,'Give the signal! We\'re ready to go!','barrett GOSSIP_ITEM_PVP_START'),
(-3649007,'That tough, huh?','barrett GOSSIP_ITEM_TWINS_INIT'),
(-3649008,'Val\'kyr? We\'re ready for them','barrett GOSSIP_ITEM_TWINS_START'),
(-3649009,'Your words of praise are appreciated, Coliseum Master.','barrett GOSSIP_ITEM_ANUB_INIT'),
(-3649010,'That is strange...','barrett GOSSIP_ITEM_ANUB_START');

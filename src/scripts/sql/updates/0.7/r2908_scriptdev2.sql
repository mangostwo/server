DELETE FROM script_texts WHERE entry BETWEEN -1532132 AND -1532131;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1532131,'The halls of Karazhan shake, as the curse binding the doors of the Gamemaster\'s Hall is lifted.',0,2,0,0,'echo_of_medivh EMOTE_LIFT_CURSE'),
(-1532132,'%s cheats!',0,3,0,0,'echo_of_medivh EMOTE_CHEAT');

DELETE FROM gossip_texts WHERE entry BETWEEN -3532017 AND -3532006;
INSERT INTO gossip_texts (entry,content_default,comment) VALUES
(-3532006,'Control Orc Grunt','orc grunt GOSSIP_ITEM_ORC_GRUNT'),
(-3532007,'Control Orc Wolf','orc wolf GOSSIP_ITEM_ORC_WOLF'),
(-3532008,'Control Summoned Daemon','summoned deamon GOSSIP_ITEM_SUMMONED_DEAMON'),
(-3532009,'Control Orc Warlock','orc warlock GOSSIP_ITEM_ORC_WARLOCK'),
(-3532010,'Control Orc Necrolyte','orc necrolyte GOSSIP_ITEM_ORC_NECROLYTE'),
(-3532011,'Control Warchief Blackhand','warchief blackhand GOSSIP_ITEM_WARCHIEF_BLACKHAND'),
(-3532012,'Control Human Footman','human footman GOSSIP_ITEM_HUMAN_FOOTMAN'),
(-3532013,'Control Human Charger','human charger GOSSIP_ITEM_HUMAN_CHARGER'),
(-3532014,'Control Conjured Water Elemental','conjured water elemental GOSSIP_ITEM_WATER_ELEMENTAL'),
(-3532015,'Control Human Conjurer','human conjurer GOSSIP_ITEM_HUMAN_CONJURER'),
(-3532016,'Control Human Cleric','human cleric GOSSIP_ITEM_HUMAN_CLERIC'),
(-3532017,'Control King Llane','king llane GOSSIP_ITEM_KING_LLANE');

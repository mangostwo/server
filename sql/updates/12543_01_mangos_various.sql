ALTER TABLE db_version CHANGE COLUMN required_12539_01_mangos_playercreateinfo_spell required_12543_01_mangos_various bit;

-- ### Wind Stones & Ice Stones (data from YTDB) ###
-- TEMPLARS
DELETE FROM spell_script_target WHERE entry IN (24734,24744,24756,24758,24760);
INSERT INTO spell_script_target VALUES
(24734, 0, 180456, 0),
(24734, 0, 180518, 0),
(24734, 0, 180529, 0),
(24734, 0, 180544, 0),
(24734, 0, 180549, 0),
(24734, 0, 180564, 0),
(24744, 0, 180456, 0),
(24744, 0, 180518, 0),
(24744, 0, 180529, 0),
(24744, 0, 180544, 0),
(24744, 0, 180549, 0),
(24744, 0, 180564, 0),
(24756, 0, 180456, 0),
(24756, 0, 180518, 0),
(24756, 0, 180529, 0),
(24756, 0, 180544, 0),
(24756, 0, 180549, 0),
(24756, 0, 180564, 0),
(24758, 0, 180456, 0),
(24758, 0, 180518, 0),
(24758, 0, 180529, 0),
(24758, 0, 180544, 0),
(24758, 0, 180549, 0),
(24758, 0, 180564, 0),
(24760, 0, 180456, 0),
(24760, 0, 180518, 0),
(24760, 0, 180529, 0),
(24760, 0, 180544, 0),
(24760, 0, 180549, 0),
(24760, 0, 180564, 0);
-- DUKES
DELETE FROM spell_script_target WHERE entry IN (24763,24765,24768,24770,24772);
INSERT INTO spell_script_target VALUES
(24763, 0, 180461, 0),
(24763, 0, 180534, 0),
(24763, 0, 180554, 0),
(24765, 0, 180461, 0),
(24765, 0, 180534, 0),
(24765, 0, 180554, 0),
(24768, 0, 180461, 0),
(24768, 0, 180534, 0),
(24768, 0, 180554, 0),
(24770, 0, 180461, 0),
(24770, 0, 180534, 0),
(24770, 0, 180554, 0),
(24772, 0, 180461, 0),
(24772, 0, 180534, 0),
(24772, 0, 180554, 0);
-- ROYALS
DELETE FROM spell_script_target WHERE entry IN (24784,24786,24788,24789,24790);
INSERT INTO spell_script_target VALUES
(24784, 0, 180466, 0),
(24784, 0, 180539, 0),
(24784, 0, 180559, 0),
(24786, 0, 180466, 0),
(24786, 0, 180539, 0),
(24786, 0, 180559, 0),
(24788, 0, 180466, 0),
(24788, 0, 180539, 0),
(24788, 0, 180559, 0),
(24789, 0, 180466, 0),
(24789, 0, 180539, 0),
(24789, 0, 180559, 0),
(24790, 0, 180466, 0),
(24790, 0, 180539, 0),
(24790, 0, 180559, 0);
-- conditions
DELETE FROM conditions WHERE condition_entry BETWEEN 887 AND 918;
INSERT INTO conditions VALUES
(887, 11, 24746, 0),    -- no cultist disguse
(888, 1, 24746, 0),     -- basic disguise
(889, 11, 24748, 0),    -- no cultist medalion
(890, -1, 888, 889),    -- basic disguise but no medalion
(891, 1, 24748, 0),     -- cultist medalion
(892, -1, 888, 891),    -- basic disguise & medalion
(893, 1, 24782, 0),     -- cultist ring
(894, -1, 892, 893),    -- disguise & medalion & ring
(895, 2, 20416, 1),     -- Crest of Beckoning: Fire
(896, 2, 20419, 1),     -- Crest of Beckoning: Earth
(897, 2, 20418, 1),     -- Crest of Beckoning: Thunder
(898, 2, 20420, 1),     -- Crest of Beckoning: Water
(899, -1, 888, 895),    -- disguise and fire
(900, -1, 888, 896),    -- disguise and earth
(901, -1, 888, 897),    -- disguise and air
(902, -1, 888, 898),    -- disguise and water
(903, 2, 20432, 1),     -- Signet of Beckoning: Fire
(904, 2, 20435, 1),     -- Signet of Beckoning: Stone
(905, 2, 20433, 1),     -- Signet of Beckoning: Thunder
(906, 2, 20436, 1),     -- Signet of Beckoning: Water
(907, -1, 892, 903),    -- disguise & medalion and fire
(908, -1, 892, 904),    -- disguise & medalion and earth
(909, -1, 892, 905),    -- disguise & medalion and air
(910, -1, 892, 906),    -- disguise & medalion and water
(911, 2, 20447, 1),     -- Scepter of Beckoning: Fire
(912, 2, 20449, 1),     -- Scepter of Beckoning: Stone
(913, 2, 20448, 1),     -- Scepter of Beckoning: Thunder
(914, 2, 20450, 1),     -- Scepter of Beckoning: Water
(915, -1, 894, 911),    -- disguise & medalion & ring and fire
(916, -1, 894, 912),    -- disguise & medalion & ring and earth
(917, -1, 894, 913),    -- disguise & medalion & ring and air
(918, -1, 894, 914);    -- disguise & medalion & ring and water
-- gossips
DELETE FROM gossip_menu where entry IN (6542,6543,6540);
INSERT INTO gossip_menu VALUES
(6540, 7744, 6540, 887),    -- punishment, no disguise
(6542, 7749, 6540, 887),    -- punishment, no disguise
(6543, 7754, 6540, 887),    -- punishment, no disguise
(6540, 7771, 0, 888),       -- basic disguise
(6542, 7772, 0, 890),       -- basic disguise & no medalion
(6542, 7773, 0, 892),       -- basic disguise & medalion
(6543, 7774, 0, 894),       -- disguise, medalion, ring (text is confirmed, but id is guesswork)
(6543, 7775, 0, 892),       -- basic disguise & medalion (text missing)
(6543, 7776, 0, 888);       -- basic disguise
DELETE FROM npc_text WHERE id=7774;
INSERT INTO npc_text (id, text0_0, text0_1, prob0) VALUES
(7774,'A thunderous voice bellows from the stone...$B$BGreetings, commander. What news of Silithus do you bring the Lords of the Council?', 'A thunderous voice bellows from the stone...$B$BGreetings, commander. What news of Silithus do you bring the Lords of the Council?', 1);
DELETE FROM gossip_menu_option where menu_id IN (6542,6543,6540);
INSERT INTO gossip_menu_option VALUES
(6540, 0, 0, 'I am no cultist, you monster! Come to me and face your destruction!', 1, 1, -1, 0, 654001, 0, 0, NULL, 888),
(6540, 1, 0, 'Crimson Templar! I hold your signet! Heed my call!', 1, 1, -1, 0, 654002, 0, 0, NULL, 899),
(6540, 2, 0, 'Earthen Templar! I hold your signet! Heed my call!', 1, 1, -1, 0, 654003, 0, 0, NULL, 900),
(6540, 3, 0, 'Hoary Templar! I hold your signet! Heed my call!', 1, 1, -1, 0, 654004, 0, 0, NULL, 901),
(6540, 4, 0, 'Azure Templar! I hold your signet! Heed my call!', 1, 1, -1, 0, 654005, 0, 0, NULL, 902),
(6542, 0, 0, 'You will listen to this, vile duke! I am not your Twilight\'s Hammer lapdog! I am here to challenge you! Come! Come, and meet your death...', 1, 1, -1, 0, 654201, 0, 0, NULL, 892),
(6542, 1, 0, 'Duke of Cynders! I hold your signet! Heed my call', 1, 1, -1, 0, 654202, 0, 0, NULL, 907),
(6542, 2, 0, 'The Duke of Shards! I hold your signet! Heed my call!', 1, 1, -1, 0, 654203, 0, 0, NULL, 908),
(6542, 3, 0, 'The Duke of Zephyrs! I hold your signet! Heed my call!', 1, 1, -1, 0, 654204, 0, 0, NULL, 909),
(6542, 4, 0, 'The Duke of Fathoms! I hold your signet! Heed my call!', 1, 1, -1, 0, 654205, 0, 0, NULL, 910),
(6543, 0, 0, 'The day of the judgement has come, fiend! I challenge you to battle!', 1, 1, -1, 0, 654301, 0, 0, NULL, 894),
(6543, 1, 0, 'Prince Skaldrenox! I hold your signet! Heed my call!', 1, 1, -1, 0, 654302, 0, 0, NULL, 915),
(6543, 2, 0, 'Baron Kazum! I hold your signet! Heed my call!', 1, 1, -1, 0, 654303, 0, 0, NULL, 916),
(6543, 3, 0, 'High Marshal Whirlaxis! I hold your signet! Heed my call!', 1, 1, -1, 0, 654304, 0, 0, NULL, 917),
(6543, 4, 0, 'Lord Skwol! I hold your signet! Heed my call!', 1, 1, -1, 0, 654305, 0, 0, NULL, 918);
DELETE FROM dbscripts_on_gossip WHERE id IN (6540,654001,654002,654003,654004,654005,654201,654202,654203,654204,654205,654301,654302,654303,654304,654305);
INSERT INTO dbscripts_on_gossip (id, delay, command, datalong, buddy_entry, search_radius, data_flags, comments) VALUES
(6540, 1, 13, 0, 180502, 10, 1, 'use Wind Stone trap'),
(654001, 0, 15, 24745, 0, 0, 4, 'lesser wind stone - random'),
(654002, 0, 15, 24747, 0, 0, 4, 'lesser wind stone - fire'),
(654003, 0, 15, 24759, 0, 0, 4, 'lesser wind stone - earth'),
(654004, 0, 15, 24757, 0, 0, 4, 'lesser wind stone - air'),
(654005, 0, 15, 24761, 0, 0, 4, 'lesser wind stone - water'),
(654201, 0, 15, 24762, 0, 0, 4, 'wind stone - random'),
(654202, 0, 15, 24766, 0, 0, 4, 'wind stone - fire'),
(654203, 0, 15, 24771, 0, 0, 4, 'wind stone - earth'),
(654204, 0, 15, 24769, 0, 0, 4, 'wind stone - air'),
(654205, 0, 15, 24773, 0, 0, 4, 'wind stone - water'),
(654301, 0, 15, 24785, 0, 0, 4, 'greater wind stone - random'),
(654302, 0, 15, 24787, 0, 0, 4, 'greater wind stone - fire'),
(654303, 0, 15, 24792, 0, 0, 4, 'greater wind stone - earth'),
(654304, 0, 15, 24791, 0, 0, 4, 'greater wind stone - air'),
(654305, 0, 15, 24793, 0, 0, 4, 'greater wind stone - water');


-- SUMMON SPELLS (YTDB)
DELETE FROM spell_script_target WHERE entry IN (40632,40640,40642,40644,41004);
INSERT INTO spell_script_target VALUES
(40632, 0, 185913, 0),
(40640, 0, 185913, 0),
(40642, 0, 185913, 0),
(40644, 0, 185913, 0),
(41004, 0, 185928, 0);
DELETE FROM conditions WHERE condition_entry=919;
INSERT INTO conditions VALUES
(919, 2, 32720, 1);
DELETE FROM gossip_menu WHERE entry IN (8687);
INSERT INTO gossip_menu VALUES
(8687, 11058, 0, 0);
DELETE FROM gossip_menu_option WHERE menu_id=8687;
INSERT INTO gossip_menu_option VALUES
(8687, 0, 0, '<Call forth Terrok.>', 1, 2, -1, 0, 8687, 0, 0, NULL, 919);
UPDATE gossip_menu_option SET action_menu_id=-1 WHERE menu_id=8660;
DELETE FROM dbscripts_on_gossip WHERE id IN (8687);
INSERT INTO dbscripts_on_gossip (id, delay, command, datalong, buddy_entry, search_radius, data_flags, comments) VALUES
(8687, 0, 15, 41003, 0, 0, 4, 'Terokk Trigger');
-- ToDo: this may need additional research
DELETE FROM dbscripts_on_event WHERE id IN (15014);
INSERT INTO dbscripts_on_event VALUES (15014, 2, 10, 21838, 180000, 0, 0, 0, 0, 0, 0, 0, -3789.4, 3507.63, 286.982, 0, 'spawn Terokk');



-- ICE STONES
DELETE FROM spell_script_target WHERE entry IN (46592);
INSERT INTO spell_script_target VALUES
(46592, 0, 188049, 0),
(46592, 0, 188137, 0),
(46592, 0, 188138, 0),
(46592, 0, 188148, 0),
(46592, 0, 188149, 0),
(46592, 0, 188150, 0);
delete from gossip_menu where entry IN (9213,9256,9257,9269,9271,9272,9273,9274);
INSERT INTO gossip_menu VALUES
(9213, 12524, 0, 0),
(9256, 12524, 0, 0),
(9257, 12524, 0, 0),
(9269, 12524, 0, 0),
(9271, 12524, 0, 0),
(9272, 12524, 0, 0),
(9273, 12524, 0, 0),
(9274, 12524, 0, 0);
DELETE FROM gossip_menu_option WHERE menu_id IN (9213,9256,9257,9269,9271,9272,9273,9274);
INSERT INTO gossip_menu_option (menu_id, id, option_icon, option_text, option_id, npc_option_npcflag, action_menu_id, action_poi_id, action_script_id, box_coded, box_money, box_text, condition_id) VALUES
(9213, 0, 0, 'Lay your hand on the stone.', 1, 1, -1, 0, 9213, 0, 0, NULL, 888),
(9256, 0, 0, 'Lay your hand on the stone.', 1, 1, -1, 0, 9213, 0, 0, NULL, 888),
(9257, 0, 0, 'Lay your hand on the stone.', 1, 1, -1, 0, 9213, 0, 0, NULL, 888),
(9269, 0, 0, 'Lay your hand on the stone.', 1, 1, -1, 0, 9213, 0, 0, NULL, 888),
(9271, 0, 0, 'Lay your hand on the stone.', 1, 1, -1, 0, 9213, 0, 0, NULL, 888),
(9272, 0, 0, 'Lay your hand on the stone.', 1, 1, -1, 0, 9213, 0, 0, NULL, 888),
(9273, 0, 0, 'Lay your hand on the stone.', 1, 1, -1, 0, 9213, 0, 0, NULL, 888),
(9274, 0, 0, 'Lay your hand on the stone.', 1, 1, -1, 0, 9213, 0, 0, NULL, 888);
DELETE FROM dbscripts_on_gossip WHERE id IN (9213);
INSERT INTO dbscripts_on_gossip (id, delay, command, datalong, data_flags, comments) VALUES
(9213, 0, 15, 46595, 4, 'cast Summon Ice Stone Lieutenant, Trigger');

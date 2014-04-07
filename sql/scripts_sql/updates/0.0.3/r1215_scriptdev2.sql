DELETE FROM script_waypoint WHERE entry=28912;
INSERT INTO script_waypoint VALUES
(28912, 0, 1653.518, -6038.374, 127.585, 0, 'Jump off'),
(28912, 1, 1653.978, -6034.614, 127.585, 5000, 'To Box'),
(28912, 2, 1653.854, -6034.726, 127.585, 500, 'Equip'),
(28912, 3, 1652.297, -6035.671, 127.585, 3000, 'Recover'),
(28912, 4, 1639.762, -6046.343, 127.948, 0, 'Escape'),
(28912, 5, 1640.963, -6028.119, 134.740, 0, ''),
(28912, 6, 1625.805, -6029.197, 134.740, 0, ''),
(28912, 7, 1626.845, -6015.085, 134.740, 0, ''),
(28912, 8, 1649.150, -6016.975, 133.240, 0, ''),
(28912, 9, 1653.063, -5974.844, 132.652, 5000, 'Mount'),
(28912, 10, 1654.747, -5926.424, 121.191, 0, 'Disappear');

DELETE FROM script_texts WHERE entry BETWEEN -1609088 AND -1609079;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1609079, 'I\'ll need to get my runeblade and armor... Just need a little more time.',0,0,0,399,'koltira SAY_BREAKOUT1'),
(-1609080, 'I\'m still weak, but I think I can get an anti-magic barrier up. Stay inside it or you\'ll be destroyed by their spells.',0,0,0,0,'koltira SAY_BREAKOUT2'),
(-1609081, 'Maintaining this barrier will require all of my concentration. Kill them all!',0,0,0,16,'koltira SAY_BREAKOUT3'),
(-1609082, 'There are more coming. Defend yourself! Don\'t fall out of the anti-magic field! They\'ll tear you apart without its protection!',0,0,0,0,'koltira SAY_BREAKOUT4'),
(-1609083, 'I can\'t keep barrier up much longer... Where is that coward?',0,0,0,0,'koltira SAY_BREAKOUT5'),
(-1609084, 'The High Inquisitor comes! Be ready, death knight! Do not let him draw you out of the protective bounds of my anti-magic field! Kill him and take his head!',0,0,0,0,'koltira SAY_BREAKOUT6'),
(-1609085, 'Stay in the anti-magic field! Make them come to you!',0,0,0,0,'koltira SAY_BREAKOUT7'),
(-1609086, 'The death of the High Inquisitor of New Avalon will not go unnoticed. You need to get out of here at once! Go, before more of them show up. I\'ll be fine on my own.',0,0,0,0,'koltira SAY_BREAKOUT8'),
(-1609087, 'I\'ll draw their fire, you make your escape behind me.',0,0,0,0,'koltira SAY_BREAKOUT9'),
(-1609088, 'Your High Inquisitor is nothing more than a pile of meat, Crusaders! There are none beyond the grasp of the Scourge!',0,1,0,0,'koltira SAY_BREAKOUT10');

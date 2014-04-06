DELETE FROM script_texts WHERE entry BETWEEN -1000663 AND -1000657;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000657,'Let\'s get the hell out of here.',0,0,0,5,'helice SAY_HELICE_ACCEPT'),
(-1000658,'Listen up, Venture Company goons! Rule #1: Never keep the prisoner near the explosives.',0,0,0,25,'helice SAY_HELICE_EXPLOSIVES_1'),
(-1000659,'Or THIS is what you get.',0,0,0,0,'helice SAY_HELICE_EXPLODE_1'),
(-1000660,'It\'s getting a little hot over here. Shall we move on?',0,0,0,11,'helice SAY_HELICE_MOVE_ON'),
(-1000661,'Oh, look, it\'s another cartload of explosives! Let\'s help them dispose of it.',0,0,0,25,'helice SAY_HELICE_EXPLOSIVES_2'),
(-1000662,'You really shouldn\'t play with this stuff. Someone could get hurt.',0,0,0,5,'helice SAY_HELICE_EXPLODE_2'),
(-1000663,'We made it! Thank you for getting me out of that hell hole. Tell Hemet to expect me!',0,0,0,4,'helice SAY_HELICE_COMPLETE');

DELETE FROM script_waypoint WHERE entry=28787;
INSERT INTO script_waypoint (entry, pointid, location_x, location_y, location_z, waittime, point_comment) VALUES
(28787, 1, 5913.516113, 5379.034668, -98.896118, 0, ''),
(28787, 2, 5917.750977, 5374.519043, -98.869781, 0, 'SAY_HELICE_EXPLOSIVES_1'),
(28787, 3, 5926.428711, 5372.145020, -98.884453, 0, ''),
(28787, 4, 5929.214844, 5377.803223, -99.020065, 0, ''),
(28787, 5, 5927.621582, 5378.564941, -99.047890, 0, ''),
(28787, 6, 5917.622070, 5383.494629, -106.310204, 0, ''),
(28787, 7, 5908.991211, 5387.655762, -106.310204, 0, ''),
(28787, 8, 5906.287109, 5390.496582, -106.041801, 0, ''),
(28787, 9, 5902.415039, 5399.741211, -99.306595, 0, ''),
(28787, 10, 5901.444336, 5404.593262, -96.759636, 0, ''),
(28787, 11, 5897.860352, 5406.656250, -96.029709, 0, ''),
(28787, 12, 5892.254395, 5401.291504, -95.848808, 0, ''),
(28787, 13, 5887.421875, 5386.701172, -95.400146, 0, 'SAY_HELICE_EXPLOSIVES_2'),
(28787, 14, 5883.308105, 5385.057617, -94.423698, 0, ''),
(28787, 15, 5879.180664, 5375.897461, -95.088066, 0, ''),
(28787, 16, 5872.613281, 5363.473633, -97.703575, 0, ''),
(28787, 17, 5857.971191, 5354.929688, -98.586090, 0, ''),
(28787, 18, 5848.729004, 5345.326660, -99.428978, 0, ''),
(28787, 19, 5842.330566, 5335.018555, -100.421455, 0, ''),
(28787, 20, 5832.164551, 5323.145020, -98.703285, 0, ''),
(28787, 21, 5824.738770, 5315.712891, -97.758018, 0, ''),
(28787, 22, 5819.650879, 5305.409668, -97.481796, 10000, 'SAY_HELICE_COMPLETE');

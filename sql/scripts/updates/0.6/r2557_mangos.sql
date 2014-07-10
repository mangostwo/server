DELETE FROM scripted_areatrigger WHERE entry in (4288,4485);
INSERT INTO scripted_areatrigger VALUES
(4288,'at_dark_portal'),
(4485,'at_dark_portal');
UPDATE creature_template SET ScriptName='npc_medivh_black_morass' WHERE entry=15608;
UPDATE creature_template SET ScriptName='npc_time_rift_channeler' WHERE entry IN (21104,17839,21697,21698);

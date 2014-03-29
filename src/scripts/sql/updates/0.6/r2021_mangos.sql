UPDATE creature_template SET ScriptName='boss_thaddius' WHERE entry=15928;
UPDATE creature_template SET ScriptName='boss_stalagg' WHERE entry=15929;
UPDATE creature_template SET ScriptName='boss_feugen' WHERE entry=15930;
UPDATE creature_template SET ScriptName='npc_tesla_coil' WHERE entry=16218;

DELETE FROM scripted_areatrigger WHERE entry=4113;
INSERT INTO scripted_areatrigger VALUES
(4113,'at_naxxramas');

DELETE FROM scripted_areatrigger WHERE entry=4937;
INSERT INTO scripted_areatrigger VALUES (4937, 'at_sunwell_plateau');

UPDATE creature_template SET ScriptName='boss_alythess' WHERE entry=25166;
UPDATE creature_template SET ScriptName='boss_sacrolash' WHERE entry=25165;
UPDATE creature_template SET ScriptName='npc_shadow_image' WHERE entry=25214;

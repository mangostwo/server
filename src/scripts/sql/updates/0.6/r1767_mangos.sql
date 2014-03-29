UPDATE gameobject_template SET ScriptName='' WHERE entry=130511;
UPDATE gameobject_template SET ScriptName='' WHERE entry=133234;

DELETE FROM scripted_event_id WHERE id IN (2228,2268);
INSERT INTO scripted_event_id VALUES
(2228,'event_spell_altar_boss_aggro'),
(2268,'event_spell_altar_boss_aggro');

UPDATE creature_template SET ScriptName='boss_general_vezax' WHERE entry=33271;
DELETE FROM scripted_event_id WHERE id=9735;
INSERT INTO scripted_event_id VALUES
(9735,'event_spell_saronite_barrier');

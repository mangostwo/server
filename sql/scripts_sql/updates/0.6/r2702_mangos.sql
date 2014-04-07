UPDATE creature_template SET ScriptName='npc_image_of_medivh' WHERE entry=17651;
DELETE FROM scripted_event_id WHERE id=10951;
INSERT INTO scripted_event_id VALUES
(10951,'event_spell_medivh_journal');

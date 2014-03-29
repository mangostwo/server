UPDATE creature_template SET ScriptName='boss_nightbane' WHERE entry=17225;
DELETE FROM scripted_event_id WHERE id in (10591);
INSERT INTO scripted_event_id VALUES
(10591,'event_spell_summon_nightbane');

UPDATE creature_template SET ScriptName='boss_anzu' WHERE entry=23035;
DELETE FROM scripted_event_id WHERE id=14797;
INSERT INTO scripted_event_id VALUES
(14797,'event_spell_summon_raven_god');

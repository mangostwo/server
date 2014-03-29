UPDATE creature_template SET ScriptName='npc_gortok_subboss' WHERE entry IN (26683,26684,26685,26686);
DELETE FROM scripted_event_id WHERE id IN (17728);
INSERT INTO scripted_event_id VALUES
(17728,'event_spell_gortok_event');

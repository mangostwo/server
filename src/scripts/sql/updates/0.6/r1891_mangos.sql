UPDATE creature_template SET ScriptName='npc_totem_of_spirits' WHERE entry=21071;

DELETE FROM scripted_event_id WHERE id IN (13513,13514,13515,13516);
INSERT INTO scripted_event_id VALUES
(13513,'event_spell_soul_captured_credit'),
(13514,'event_spell_soul_captured_credit'),
(13515,'event_spell_soul_captured_credit'),
(13516,'event_spell_soul_captured_credit');

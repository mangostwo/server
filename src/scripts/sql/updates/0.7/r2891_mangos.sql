DELETE FROM scripted_event_id WHERE id IN (2609);
INSERT INTO scripted_event_id VALUES
(2609,'event_spell_unlocking');
UPDATE creature_template SET ScriptName='' WHERE entry=7604;
UPDATE creature_template SET ScriptName='' WHERE entry=7607;

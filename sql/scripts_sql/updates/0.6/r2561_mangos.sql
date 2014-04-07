UPDATE creature_template SET ScriptName='' WHERE entry IN (24848);
DELETE FROM scripted_event_id WHERE id=16547;
INSERT INTO scripted_event_id VALUES
(16547,'event_go_scrying_orb');

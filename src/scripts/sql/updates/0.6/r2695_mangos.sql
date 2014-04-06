DELETE FROM scripted_event_id WHERE id=11111;
INSERT INTO scripted_event_id VALUES
(11111,'event_go_barrel_old_hillsbrad');
UPDATE gameobject_template SET ScriptName='' WHERE entry=182589;

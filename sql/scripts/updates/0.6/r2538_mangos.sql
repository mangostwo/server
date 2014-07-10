UPDATE instance_template SET ScriptName='instance_icecrown_citadel' WHERE map=631;
DELETE FROM scripted_event_id WHERE id IN (23426,23438);
INSERT INTO scripted_event_id VALUES
(23426,'event_gameobject_citadel_valve'),
(23438,'event_gameobject_citadel_valve');

UPDATE instance_template SET ScriptName='instance_eye_of_eternity' WHERE map=616;
UPDATE creature_template SET ScriptName='boss_malygos' WHERE entry=28859;
DELETE FROM scripted_event_id WHERE id IN (20711);
INSERT INTO scripted_event_id VALUES
(20711,'event_go_focusing_iris');

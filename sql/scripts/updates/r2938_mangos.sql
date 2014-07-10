UPDATE creature_template SET ScriptName='npc_scrapbot' WHERE entry=33343;
DELETE FROM scripted_event_id WHERE id IN (21606);
INSERT INTO scripted_event_id VALUES
(21606,'event_ulduar');

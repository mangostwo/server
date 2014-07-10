UPDATE creature_template SET ScriptName='boss_ignis' WHERE entry=33118;
UPDATE creature_template SET ScriptName='npc_iron_construct' WHERE entry=33121;
UPDATE creature_template SET ScriptName='npc_scorch' WHERE entry=33221;
DELETE FROM scripted_event_id WHERE id IN (21620);
INSERT INTO scripted_event_id VALUES
(21620,'event_ulduar');

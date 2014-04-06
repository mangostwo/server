UPDATE creature_template SET ScriptName='npc_flash_freeze' WHERE entry IN (32926,32938);
UPDATE creature_template SET ScriptName='npc_icicle_target' WHERE entry=33174;
DELETE FROM scripted_event_id WHERE id IN (20907,21045);
INSERT INTO scripted_event_id VALUES
(20907,'event_boss_hodir'),
(21045,'event_boss_hodir');

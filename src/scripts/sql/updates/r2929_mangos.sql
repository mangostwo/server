UPDATE creature_template SET ScriptName='npc_razorscale_spawner' WHERE entry=33245;
UPDATE creature_template SET ScriptName='npc_harpoon_fire_state' WHERE entry=33282;
DELETE FROM scripted_event_id WHERE id IN (20964);
INSERT INTO scripted_event_id VALUES
(20964,'event_spell_harpoon_shot');

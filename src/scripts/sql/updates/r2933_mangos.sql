UPDATE creature_template SET ScriptName='npc_keeper_norgannon' WHERE entry=33686;
UPDATE creature_template SET ScriptName='npc_brann_ulduar' WHERE entry=33579;
DELETE FROM scripted_event_id WHERE id IN (21030,21031,21032,21033);
INSERT INTO scripted_event_id VALUES
(21030,'event_go_ulduar_tower'),        -- Tower of Life destroyed event
(21031,'event_go_ulduar_tower'),        -- Tower of Storms destroyed event
(21032,'event_go_ulduar_tower'),        -- Tower of Frost destroyed event
(21033,'event_go_ulduar_tower');        -- Tower of Flame destroyed event

UPDATE creature_template SET ScriptName='spell_dummy_npc_crates_bunny' WHERE entry=27827;
DELETE FROM scripted_areatrigger WHERE entry=5291;
INSERT INTO scripted_areatrigger VALUES
(5291,'at_culling_of_stratholme');

DELETE FROM scripted_areatrigger WHERE entry=4778;
INSERT INTO scripted_areatrigger VALUES (4778,'at_ancient_male_vrykul');

UPDATE creature_template SET ScriptName='npc_ancient_male_vrykul' WHERE entry=24314;

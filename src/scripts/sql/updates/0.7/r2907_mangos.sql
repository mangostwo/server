UPDATE creature_template SET ScriptName='npc_king_ymiron' WHERE entry=24321;
DELETE FROM scripted_areatrigger WHERE entry IN (4779);
INSERT INTO scripted_areatrigger VALUES (4779,'at_nifflevar');

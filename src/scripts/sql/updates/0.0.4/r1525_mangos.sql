DELETE FROM areatrigger_scripts WHERE entry=4016;
INSERT INTO areatrigger_scripts VALUES (4016,'at_shade_of_eranikus');
UPDATE creature_template SET ScriptName='npc_malfurion_stormrage' WHERE entry=15362;

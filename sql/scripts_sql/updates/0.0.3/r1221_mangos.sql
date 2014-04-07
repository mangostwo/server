UPDATE creature_template SET ScriptName='' WHERE entry=13936;

DELETE FROM areatrigger_scripts WHERE entry=3066;
INSERT INTO areatrigger_scripts VALUES (3066,'at_ravenholdt');

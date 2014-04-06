DELETE FROM areatrigger_scripts WHERE entry=5140;
INSERT INTO areatrigger_scripts VALUES (5140,'at_svala_intro');

UPDATE instance_template SET script='instance_pinnacle' WHERE map=575;

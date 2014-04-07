UPDATE instance_template SET script='instance_blackrock_spire' WHERE map=229;

DELETE FROM areatrigger_scripts WHERE entry IN (2026, 2046);
INSERT INTO areatrigger_scripts VALUES
(2026, 'at_blackrock_spire'),
(2046, 'at_blackrock_spire');


DELETE FROM areatrigger_scripts WHERE entry=5108;
INSERT INTO areatrigger_scripts VALUES (5108,'at_stormwright_shelf');

DELETE FROM areatrigger_scripts WHERE entry IN (4871, 4872, 4873);
INSERT INTO areatrigger_scripts VALUES
(4871,'at_warsong_farms'),
(4872,'at_warsong_farms'),
(4873,'at_warsong_farms');


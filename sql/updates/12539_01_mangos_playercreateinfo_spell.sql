ALTER TABLE db_version CHANGE COLUMN required_12535_01_mangos_scripted_event required_12539_01_mangos_playercreateinfo_spell bit;

DELETE FROM playercreateinfo_spell WHERE race=10 AND Spell=61437;
INSERT INTO playercreateinfo_spell (race, class, Spell, Note) VALUES
(10,2,61437,'Opening'),
(10,3,61437,'Opening'),
(10,4,61437,'Opening'),
(10,5,61437,'Opening'),
(10,6,61437,'Opening'),
(10,8,61437,'Opening'),
(10,9,61437,'Opening');

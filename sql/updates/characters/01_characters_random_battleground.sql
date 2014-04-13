ALTER TABLE character_db_version CHANGE COLUMN required_12487_01_characters_characters required_12546_01_characters_random_battleground bit;

ALTER TABLE `saved_variables` ADD COLUMN `NextRandomBGResetTime` BIGINT(40) UNSIGNED NOT NULL DEFAULT '0' AFTER `NextWeeklyQuestResetTime`;

DROP TABLE IF EXISTS `character_battleground_random`;
CREATE TABLE `character_battleground_random` (
  `guid` INT(11) UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY  (`guid`)
) ENGINE=MYISAM DEFAULT CHARSET=utf8;
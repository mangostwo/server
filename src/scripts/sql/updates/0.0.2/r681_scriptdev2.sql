--
-- NOTE: if you have temporary stored data in table `script_localized_texts` make sure to make backup of this before running this update!
-- Also note if you have any texts in current script_texts and they are not using entries valid for *_texts table, you _will_ get error messages on startup.
--

-- drop obsolete table
DROP TABLE script_localized_texts;

-- alter and add fields in table `script_texts`
ALTER TABLE script_texts CHANGE COLUMN `id` `entry` mediumint(8) NOT NULL;
ALTER TABLE script_texts CHANGE COLUMN `text` `content_default` text NOT NULL AFTER `entry`;
ALTER TABLE script_texts ADD COLUMN `content_loc1` text AFTER `content_default`;
ALTER TABLE script_texts ADD COLUMN `content_loc2` text AFTER `content_loc1`;
ALTER TABLE script_texts ADD COLUMN `content_loc3` text AFTER `content_loc2`;
ALTER TABLE script_texts ADD COLUMN `content_loc4` text AFTER `content_loc3`;
ALTER TABLE script_texts ADD COLUMN `content_loc5` text AFTER `content_loc4`;
ALTER TABLE script_texts ADD COLUMN `content_loc6` text AFTER `content_loc5`;
ALTER TABLE script_texts ADD COLUMN `content_loc7` text AFTER `content_loc6`;
ALTER TABLE script_texts ADD COLUMN `content_loc8` text AFTER `content_loc7`;
ALTER TABLE script_texts MODIFY COLUMN `sound` tinyint unsigned NOT NULL default '0';
ALTER TABLE script_texts MODIFY COLUMN `type` tinyint unsigned NOT NULL default '0';
ALTER TABLE script_texts MODIFY COLUMN `language` tinyint unsigned NOT NULL default '0';
ALTER TABLE script_texts MODIFY COLUMN `comment` text;

-- new table for custom texts
DROP TABLE IF EXISTS `custom_texts`;
CREATE TABLE `custom_texts` (
  `entry` mediumint(8) NOT NULL,
  `content_default` text NOT NULL,
  `content_loc1` text,
  `content_loc2` text,
  `content_loc3` text,
  `content_loc4` text,
  `content_loc5` text,
  `content_loc6` text,
  `content_loc7` text,
  `content_loc8` text,
  `sound` mediumint(8) unsigned NOT NULL default '0',
  `type` tinyint unsigned NOT NULL default '0',
  `language` tinyint unsigned NOT NULL default '0',
  `comment` text,
  PRIMARY KEY  (`entry`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=FIXED COMMENT='Custom Texts';

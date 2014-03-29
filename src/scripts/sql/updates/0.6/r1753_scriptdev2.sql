UPDATE sd2_db_version SET version='ScriptDev2 (for MaNGOS 10305+) ';

DROP TABLE IF EXISTS `gossip_texts`;
CREATE TABLE `gossip_texts` (
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
  `comment` text,
  PRIMARY KEY (`entry`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=FIXED COMMENT='Gossip Texts';

DELETE FROM gossip_texts WHERE entry IN (-3608000, -3608001);
INSERT INTO gossip_texts (entry,content_default,comment) VALUES
(-3608000,'Activate the crystals when we get in trouble, right?','sinclari GOSSIP_ITEM_INTRO'),
(-3608001,'Get your people to safety, we\'ll keep the Blue Dragonflight\'s forces at bay.','sinclari GOSSIP_ITEM_START');

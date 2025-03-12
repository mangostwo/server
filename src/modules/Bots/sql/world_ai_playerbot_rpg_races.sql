DROP TABLE IF EXISTS `ai_playerbot_rpg_races`;

CREATE TABLE `ai_playerbot_rpg_races` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `entry` bigint(20),
  `race` bigint(20),
  PRIMARY KEY (`id`),
  KEY `entry` (`entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

DELETE FROM `ai_playerbot_rpg_races`;

-- say

INSERT INTO `ai_playerbot_rpg_races` VALUES
-- Milli Featherwhistle, Gnome
(NULL, 7955, 3),
(NULL, 7955, 7),
-- Innkeeper Belm, Dward
(NULL, 1247, 3),
(NULL, 1247, 7),
-- Innkeeper Skindle, Booty Bay 6807	
-- Innkeeper Thulbek, Orc, StVale
(NULL, 5814, 2),
(NULL, 5814, 6),
(NULL, 5814, 8),
-- Innkeeper Firebrew, Ironforge
(NULL, 5111, 3),
(NULL, 5111, 7),
-- Innkeeper Trelayne, Duskwood
(NULL, 6790, 1),
-- Innkeeper Gryshka, Orgrimmar
(NULL, 6929, 2),
(NULL, 6929, 8),
-- Innkeeper Shul'kar, Searing Gorge, Orc
(NULL, 9356, 2),
(NULL, 9356, 5),
(NULL, 9356, 6),
(NULL, 9356, 8),
-- Innkeeper Grosk, Durotar
(NULL, 6928, 2),
(NULL, 6928, 8),
-- Innkeeper Hearthstove, Loch Modan
(NULL, 6734, 3),
(NULL, 6734, 7),
-- Innkeeper Helbrek, Wetlands
(NULL, 1464, 3),
(NULL, 1464, 7),
-- Innkeeper Brianna, Redridge
(NULL, 6727, 1),
-- Innkeeper Adegwa, Arathi, Hammerfall
(NULL, 9501, 5),
-- Innkeeper Byula, Barrens, Camb Taraugho
(NULL, 7714, 2),
(NULL, 7714, 6),
(NULL, 7714, 8),
-- Innkeeper Boorand Plainswind, Barrens
(NULL, 3934, 2),
(NULL, 3934, 6),
(NULL, 3934, 8),
-- Innkeeper Wiley, Ratchet
(NULL, 6791, 2),
(NULL, 6791, 8),
-- Innkeeper Shay, Tarren Mill
(NULL, 2388, 5),
-- Innkeeper Anderson, Southshore
(NULL, 2352, 1),
(NULL, 2352, 3),
(NULL, 2352, 7),
-- Innkeeper Bates, Silverpine
(NULL, 6739, 5),
-- Innkeeper Abeqwa, Thousand Needles
(NULL, 11116, 2),
(NULL, 11116, 6),
(NULL, 11116, 8),
-- Innkeeper Fizzgrimble, Tanaris, 7733	
-- Marin Noggenfogger, 7564
-- Innkeeper Pala, Mulgore
(NULL, 6746, 6),
-- Innkeeper Kauth, Mulgore
(NULL, 6747, 6),
-- Bashana Runetotem, Thunder Bluff
(NULL, 9087, 6),
-- Innkeeper Lyshaerya, Desolase
(NULL, 11103, 4),
-- Innkeeper Sikewa, Desolace
(NULL, 11106, 2),
(NULL, 11106, 6),
(NULL, 11106, 8),
-- Innkeeper Renee, Brill
(NULL, 5688, 5),
-- Innkeeper Jayka, Stonetalon, Red Rock Retreat
(NULL, 7731, 2),
(NULL, 7731, 6),
(NULL, 7731, 8),
-- Innkeeper Faralia, Stonetalon Peak
(NULL, 16458, 4),	
-- Innkeeper Janene, Theramore
(NULL, 6272, 1),	
(NULL, 6272, 3),	
(NULL, 6272, 7),	
-- Innkeeper Karakul, Swamp of Sorrows
(NULL, 6930, 2),	
(NULL, 6930, 8),	
-- Innkeeper Kimlya, Astranaar
(NULL, 6738, 4),
-- Innkeeper Kaylisk, Splitertree
(NULL, 12196, 2),
(NULL, 12196, 8),
-- Innkeeper Shaussiy, Darkshore
(NULL, 6737, 4),
-- Innkeeper Norman, Undercity
(NULL, 6741, 5),
-- Innkeeper Vizzie, Everlook, 11118	
-- Calandrath, Silithus, 15174
-- Alchemist Arbington, West Plaguelands, Human
(NULL, 11056, 1),
(NULL, 11056, 3),
(NULL, 11056, 4),
(NULL, 11056, 7),
-- Innkeeper Saelienne, Darnassus
(NULL, 6735, 4),
-- Innkeeper Keldamyr, Dolanaar
(NULL, 6736, 4),
-- Lelanai, Darnassus
(NULL, 4730, 4),
-- Lokhtos Darkbargainer, Blackrock, 12944
-- Innkeeper Shyria, Feralas, Feathermoon
(NULL, 7736, 4),
-- Innkeeper Greul, Feralas, Horde
(NULL, 7737, 6),
-- Gregan Brewspewer, Feralas, Dwarf
(NULL, 7775, 3),	
-- Augustus the Touched, East Plaguelands, Undead
(NULL, 12384, 5),	
-- Jessica Chambers, East Plaguelands, 16256
-- Innkeeper Heather, Westfall
(NULL, 8931, 1),	
-- Innkeeper Allison, Stormwind
(NULL, 6740, 1),	
-- Innkeeper Farley, Goldshire
(NULL, 295, 1),	
-- Lard, Hinterlands, 14731
-- Innkeeper Thulfram, Hillsbrad, Dward
(NULL, 7744, 3)
;

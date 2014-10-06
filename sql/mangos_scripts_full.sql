/*  */

/* AREATRIGGER */
DELETE FROM scripted_areatrigger WHERE entry=4591;
INSERT INTO scripted_areatrigger VALUES (4591,'at_coilfang_waterfall');
DELETE FROM scripted_areatrigger WHERE entry=4560;
INSERT INTO scripted_areatrigger VALUES (4560,'at_legion_teleporter');
DELETE FROM scripted_areatrigger WHERE entry=3066;
INSERT INTO scripted_areatrigger VALUES (3066,'at_ravenholdt');
DELETE FROM scripted_areatrigger WHERE entry IN (4871,4872,4873);
INSERT INTO scripted_areatrigger VALUES
(4871,'at_warsong_farms'),
(4872,'at_warsong_farms'),
(4873,'at_warsong_farms');
DELETE FROM scripted_areatrigger WHERE entry IN (5046,5047);
INSERT INTO scripted_areatrigger VALUES
(5046,'at_waygate'),
(5047,'at_waygate');
DELETE FROM scripted_areatrigger WHERE entry BETWEEN 5284 AND 5287;
INSERT INTO scripted_areatrigger VALUES
(5284,'at_aldurthar_gate'),
(5285,'at_aldurthar_gate'),
(5286,'at_aldurthar_gate'),
(5287,'at_aldurthar_gate');
DELETE FROM scripted_areatrigger WHERE entry IN (4112,4113);
INSERT INTO scripted_areatrigger VALUES
(4112,'at_naxxramas'),
(4113,'at_naxxramas');
DELETE FROM scripted_areatrigger WHERE entry=5108;
INSERT INTO scripted_areatrigger VALUES (5108,'at_stormwright_shelf');
DELETE FROM scripted_areatrigger WHERE entry IN (3546,3547,3548,3549,3550,3552);
INSERT INTO scripted_areatrigger VALUES
(3546,'at_childrens_week_spot'), -- Darnassian bank
(3547,'at_childrens_week_spot'), -- Undercity - thone room
(3548,'at_childrens_week_spot'), -- Stonewrought Dam
(3549,'at_childrens_week_spot'), -- The Mor'shan Rampart
(3550,'at_childrens_week_spot'), -- Ratchet Docks
(3552,'at_childrens_week_spot'); -- Westfall Lighthouse
DELETE FROM scripted_areatrigger WHERE entry IN (2026,2046);
INSERT INTO scripted_areatrigger VALUES
(2026,'at_blackrock_spire'),
(2046,'at_blackrock_spire');
DELETE FROM scripted_areatrigger WHERE entry=5030;
INSERT INTO scripted_areatrigger VALUES (5030,'at_spearborn_encampment');
DELETE FROM scripted_areatrigger WHERE entry IN (3958,3960);
INSERT INTO scripted_areatrigger VALUES
(3958,'at_zulgurub'),
(3960,'at_zulgurub');
DELETE FROM scripted_areatrigger WHERE entry=3626;
INSERT INTO scripted_areatrigger VALUES (3626,'at_vaelastrasz');
DELETE FROM scripted_areatrigger WHERE entry=4937;
INSERT INTO scripted_areatrigger VALUES (4937,'at_sunwell_plateau');
DELETE FROM scripted_areatrigger WHERE entry=4524;
INSERT INTO scripted_areatrigger VALUES (4524,'at_shattered_halls');
DELETE FROM scripted_areatrigger WHERE entry BETWEEN 1726 AND 1740;
INSERT INTO scripted_areatrigger VALUES
(1726,'at_scent_larkorwi'),
(1727,'at_scent_larkorwi'),
(1728,'at_scent_larkorwi'),
(1729,'at_scent_larkorwi'),
(1730,'at_scent_larkorwi'),
(1731,'at_scent_larkorwi'),
(1732,'at_scent_larkorwi'),
(1733,'at_scent_larkorwi'),
(1734,'at_scent_larkorwi'),
(1735,'at_scent_larkorwi'),
(1736,'at_scent_larkorwi'),
(1737,'at_scent_larkorwi'),
(1738,'at_scent_larkorwi'),
(1739,'at_scent_larkorwi'),
(1740,'at_scent_larkorwi');
DELETE FROM scripted_areatrigger WHERE entry IN (5604,5709,5732);
INSERT INTO scripted_areatrigger VALUES
(5604,'at_icecrown_citadel'),
(5709,'at_icecrown_citadel'),
(5732,'at_icecrown_citadel');
DELETE FROM scripted_areatrigger WHERE entry in (4288,4485);
INSERT INTO scripted_areatrigger VALUES
(4288,'at_dark_portal'),
(4485,'at_dark_portal');
DELETE FROM scripted_areatrigger WHERE entry=1966;
INSERT INTO scripted_areatrigger VALUES (1966,'at_murkdeep');
DELETE FROM scripted_areatrigger WHERE entry IN (4047,4052);
INSERT INTO scripted_areatrigger VALUES
(4047,'at_temple_ahnqiraj'),
(4052,'at_temple_ahnqiraj');
DELETE FROM scripted_areatrigger WHERE entry IN (5710,5711,5712,5714,5715,5716);
INSERT INTO scripted_areatrigger VALUES
(5710, 'at_hot_on_the_trail'),
(5711, 'at_hot_on_the_trail'),
(5712, 'at_hot_on_the_trail'),
(5714, 'at_hot_on_the_trail'),
(5715, 'at_hot_on_the_trail'),
(5716, 'at_hot_on_the_trail');


/* BATTLEGROUNDS */
UPDATE creature_template SET ScriptName='npc_spirit_guide' WHERE entry IN (13116, 13117);

/* WORLD BOSS */
UPDATE creature_template SET ScriptName='boss_ysondre' WHERE entry=14887;
UPDATE creature_template SET ScriptName='boss_emeriss' WHERE entry=14889;
UPDATE creature_template SET ScriptName='boss_taerar' WHERE entry=14890;
UPDATE creature_template SET ScriptName='boss_azuregos' WHERE entry=6109;
UPDATE creature_template SET ScriptName='boss_lethon' WHERE entry=14888;
UPDATE creature_template SET ScriptName='npc_spirit_shade' WHERE entry=15261;

/* GO */
UPDATE gameobject_template SET ScriptName='go_barov_journal' WHERE entry=180794;
UPDATE gameobject_template SET ScriptName='go_ethereum_prison' WHERE entry BETWEEN 184418 AND 184431;
UPDATE gameobject_template SET ScriptName='go_ethereum_stasis' WHERE entry BETWEEN 185465 AND 185467;
UPDATE gameobject_template SET ScriptName='go_ethereum_stasis' WHERE entry=184595;
UPDATE gameobject_template SET ScriptName='go_ethereum_stasis' WHERE entry BETWEEN 185461 AND 185464;
UPDATE gameobject_template SET ScriptName='go_jump_a_tron' WHERE entry=183146;
UPDATE gameobject_template SET ScriptName='go_mysterious_snow_mound' WHERE entry=195308;
UPDATE gameobject_template SET ScriptName='go_tele_to_dalaran_crystal' WHERE entry=191230;
UPDATE gameobject_template SET ScriptName='go_tele_to_violet_stand' WHERE entry=191229;
UPDATE gameobject_template SET ScriptName='go_andorhal_tower' WHERE entry IN (176094,176095,176096,176097);
UPDATE gameobject_template SET ScriptName='go_scourge_enclosure' WHERE entry=191548;
UPDATE gameobject_template SET ScriptName='go_veil_skith_cage' WHERE entry IN (185202,185203,185204,185205);
UPDATE gameobject_template SET ScriptName='go_lab_work_reagents' WHERE entry IN (190462, 190473, 190478, 190459);

/* GUARD */
UPDATE creature_template SET ScriptName='guard_azuremyst' WHERE entry=18038;
UPDATE creature_template SET ScriptName='guard_orgrimmar' WHERE entry=3296;
UPDATE creature_template SET ScriptName='guard_stormwind' WHERE entry IN (68,1976);
UPDATE creature_template SET ScriptName='guard_contested' WHERE entry IN (9460,4624,3502,11190,15184);
UPDATE creature_template SET ScriptName='guard_elwynnforest' WHERE entry=1423;
UPDATE creature_template SET ScriptName='guard_eversong' WHERE entry=16221;
UPDATE creature_template SET ScriptName='guard_darnassus' WHERE entry=4262;
UPDATE creature_template SET ScriptName='guard_teldrassil' WHERE entry=3571;
UPDATE creature_template SET ScriptName='guard_ironforge' WHERE entry=5595;
UPDATE creature_template SET ScriptName='guard_dunmorogh' WHERE entry IN (727,13076);
UPDATE creature_template SET ScriptName='guard_undercity' WHERE entry=5624;
UPDATE creature_template SET ScriptName='guard_bluffwatcher' WHERE entry=3084;
UPDATE creature_template SET ScriptName='guard_durotar' WHERE entry=5953;
UPDATE creature_template SET ScriptName='guard_mulgore' WHERE entry IN (3212,3215,3217,3218,3219,3220,3221,3222,3223,3224);
UPDATE creature_template SET ScriptName='guard_dunmorogh' WHERE entry IN (727,13076);
UPDATE creature_template SET ScriptName='guard_tirisfal' WHERE entry IN (1735,1738,2210,1744,1745,5725,1743,2209,1746,1742);
UPDATE creature_template SET ScriptName='guard_silvermoon' WHERE entry=16222;
UPDATE creature_template SET ScriptName='guard_exodar' WHERE entry=16733;
UPDATE creature_template SET ScriptName='guard_shattrath' WHERE entry=19687;
UPDATE creature_template SET ScriptName='guard_shattrath_aldor' WHERE entry=18549;
UPDATE creature_template SET ScriptName='guard_shattrath_scryer' WHERE entry=18568;

/* ITEM */
UPDATE item_template SET ScriptName='item_arcane_charges' WHERE entry=34475;
UPDATE item_template SET ScriptName='item_flying_machine' WHERE entry IN (34060,34061);
UPDATE item_template SET ScriptName='item_gor_dreks_ointment' WHERE entry=30175;
UPDATE item_template SET ScriptName='item_petrov_cluster_bombs' WHERE entry=33098;

/* NPC (usually creatures to be found in more than one specific zone) */
UPDATE creature_template SET ScriptName='npc_air_force_bots' WHERE entry IN (2614, 2615, 21974, 21993, 21996, 21997, 21999, 22001, 22002, 22003, 22063, 22065, 22066, 22068, 22069, 22070, 22071, 22078, 22079, 22080, 22086, 22087, 22088, 22090, 22124, 22125, 22126);
UPDATE creature_template SET ScriptName='npc_chicken_cluck' WHERE entry=620;
UPDATE creature_template SET ScriptName='npc_dancing_flames' WHERE entry=25305;
UPDATE creature_template SET ScriptName='npc_garments_of_quests' WHERE entry IN (12429,12423,12427,12430,12428);
UPDATE creature_template SET ScriptName='npc_guardian' WHERE entry=5764;
UPDATE creature_template SET ScriptName='npc_doctor' WHERE entry IN (12939,12920);
UPDATE creature_template SET ScriptName='npc_injured_patient' WHERE entry IN (12936,12937,12938,12923,12924,12925);
UPDATE creature_template SET ScriptName='npc_prof_blacksmith' WHERE entry IN (5164,11145,11146,11176,11177,11178,11191,11192,11193);
UPDATE creature_template SET ScriptName='npc_prof_leather' WHERE entry IN (7866,7867,7868,7869,7870,7871);
-- disabled, but can be used for custom
-- UPDATE creature_template SET ScriptName='' WHERE npcflag!=npcflag|65536 AND ScriptName='npc_innkeeper';
-- UPDATE creature_template SET ScriptName='npc_innkeeper' WHERE npcflag=npcflag|65536;
UPDATE creature_template SET ScriptName='npc_spring_rabbit' WHERE entry=32791;
UPDATE creature_template SET ScriptName='npc_redemption_target' WHERE entry IN (6172,6177,17542,17768);

/* SPELL */
UPDATE creature_template SET ScriptName='spell_dummy_npc' WHERE entry IN (
-- eastern kingdoms
1200,8888,13016,
-- kalimdor
9299,12296,12298,
-- outland
16880,16518,16847,17157,17326,17654,18879,21729,22105,24918,24922,25084,25085,
-- northrend
23678,25752,25753,25758,25792,25793,26268,26270,26421,26616,26643,26841,26924,27122,27808,28053,28054,28068,28093,28465,28600,29319,29327,29329,29330,29338,30146,30169,32149);

UPDATE gameobject_template SET ScriptName='spell_dummy_go' WHERE entry IN (181616,186949);

/* WORLD MAP SCRIPTS */
DELETE FROM world_template WHERE map IN (0, 1, 530, 571, 609);
INSERT INTO world_template VALUES
(0, 'world_map_eastern_kingdoms'),
(1, 'world_map_kalimdor'),
(530, 'world_map_outland'),
(571, 'world_map_northrend'),
(609, 'world_map_ebon_hold');

/*  */
/* ZONE */
/* */

/* ALTERAC MOUNTAINS */

/* ALTERAC VALLEY */

/* ARATHI HIGHLANDS */
UPDATE creature_template SET ScriptName='npc_professor_phizzlethorpe' WHERE entry=2768;
UPDATE creature_template SET ScriptName='npc_kinelory' WHERE entry=2713;

/* ASHENVALE */
UPDATE creature_template SET ScriptName='npc_muglash' WHERE entry=12717;
UPDATE gameobject_template SET ScriptName='go_naga_brazier' WHERE entry=178247;
UPDATE creature_template SET ScriptName='npc_ruul_snowhoof' WHERE entry=12818;
UPDATE creature_template SET ScriptName='npc_torek' WHERE entry=12858;
UPDATE creature_template SET ScriptName='npc_feero_ironhand' WHERE entry=4484;

/*  */
/* AUCHINDOUN */
/*  */

/* MANA TOMBS */
UPDATE creature_template SET ScriptName='boss_pandemonius' WHERE entry=18341;
UPDATE creature_template SET ScriptName='boss_nexusprince_shaffar' WHERE entry=18344;

/* AUCHENAI CRYPTS */
UPDATE creature_template SET ScriptName='boss_exarch_maladaar' WHERE entry=18373;
UPDATE creature_template SET ScriptName='mob_stolen_soul' WHERE entry=18441;
UPDATE creature_template SET ScriptName='boss_shirrak' WHERE entry=18371;

/* SETHEKK HALLS */
UPDATE instance_template SET ScriptName='instance_sethekk_halls' WHERE map=556;
UPDATE creature_template SET ScriptName='boss_talon_king_ikiss' WHERE entry=18473;
UPDATE creature_template SET ScriptName='boss_darkweaver_syth' WHERE entry=18472;
UPDATE creature_template SET ScriptName='boss_anzu' WHERE entry=23035;
DELETE FROM scripted_event_id WHERE id=14797;
INSERT INTO scripted_event_id VALUES
(14797,'event_spell_summon_raven_god');

/* SHADOW LABYRINTH */
UPDATE instance_template SET ScriptName='instance_shadow_labyrinth' WHERE map=555;
UPDATE creature_template SET ScriptName='boss_murmur' WHERE entry=18708;
UPDATE creature_template SET ScriptName='boss_grandmaster_vorpil' WHERE entry=18732;
UPDATE creature_template SET ScriptName='boss_blackheart_the_inciter' WHERE entry=18667;
UPDATE creature_template SET ScriptName='boss_ambassador_hellmaw' WHERE entry=18731;
UPDATE creature_template SET ScriptName='npc_void_traveler' WHERE entry=19226;

/*  */
/* AZJOL-NERUB */
/*  */

/* AHN'KAHET */
UPDATE creature_template SET ScriptName='boss_amanitar' WHERE entry=30258;
UPDATE creature_template SET ScriptName='npc_amanitar_mushroom' WHERE entry IN (30391,30435);
UPDATE creature_template SET ScriptName='boss_jedoga' WHERE entry=29310;
UPDATE creature_template SET ScriptName='npc_twilight_volunteer' WHERE entry=30385;
UPDATE creature_template SET ScriptName='boss_nadox' WHERE entry=29309;
UPDATE creature_template SET ScriptName='mob_ahnkahar_egg' WHERE entry IN (30172,30173);
UPDATE creature_template SET ScriptName='boss_taldaram' WHERE entry=29308;
UPDATE gameobject_template SET ScriptName='go_nerubian_device' WHERE entry IN (193093,193094);
UPDATE creature_template SET ScriptName='boss_volazj' WHERE entry=29311;
UPDATE instance_template SET ScriptName='instance_ahnkahet' WHERE map=619;

/* AZJOL-NERUB */
UPDATE creature_template SET ScriptName='boss_anubarak' WHERE entry=29120;
UPDATE creature_template SET ScriptName='npc_impale_target' WHERE entry=29184;
UPDATE creature_template SET ScriptName='boss_hadronox' WHERE entry=28921;
UPDATE creature_template SET ScriptName='boss_krikthir' WHERE entry=28684;
UPDATE instance_template SET ScriptName='instance_azjol-nerub' WHERE map=601;

/* AZSHARA */
UPDATE creature_template SET ScriptName='npc_rizzle_sprysprocket' WHERE entry=23002;
UPDATE creature_template SET ScriptName='npc_depth_charge' WHERE entry=23025;
UPDATE gameobject_template SET ScriptName='go_southfury_moonstone' WHERE entry=185566;
UPDATE creature_template SET ScriptName='mobs_spitelashes' WHERE entry IN (6190,6193,6194,6195,6196);
UPDATE creature_template SET ScriptName='npc_loramus_thalipedes' WHERE entry=7783;

/* AZUREMYST ISLE */
UPDATE creature_template SET ScriptName='npc_draenei_survivor' WHERE entry=16483;
UPDATE creature_template SET ScriptName='npc_engineer_spark_overgrind' WHERE entry=17243;
UPDATE creature_template SET ScriptName='npc_injured_draenei' WHERE entry=16971;
UPDATE creature_template SET ScriptName='npc_magwin' WHERE entry=17312;

/* BADLANDS */

/* BARRENS */
UPDATE creature_template SET ScriptName='npc_beaten_corpse' WHERE entry=10668;
UPDATE creature_template SET ScriptName='npc_gilthares' WHERE entry=3465;
UPDATE creature_template SET ScriptName='npc_taskmaster_fizzule' WHERE entry=7233;
UPDATE creature_template SET ScriptName='npc_twiggy_flathead' WHERE entry=6248;
DELETE FROM scripted_areatrigger WHERE entry=522;
INSERT INTO scripted_areatrigger VALUES (522,'at_twiggy_flathead');
UPDATE creature_template SET ScriptName='npc_wizzlecranks_shredder' WHERE entry=3439;

/* BLACK TEMPLE */
UPDATE instance_template SET ScriptName='instance_black_temple' WHERE map=564;
UPDATE creature_template SET ScriptName='npc_akama_shade' WHERE entry=22990; -- Akama at Shade of Akama
UPDATE creature_template SET ScriptName='npc_akama_illidan' WHERE entry=23089; -- Akama at Illidan
UPDATE creature_template SET ScriptName='mob_illidari_council' WHERE entry=23426; -- Illidari Council controller mob
UPDATE creature_template SET ScriptName='mob_blood_elf_council_voice_trigger' WHERE entry=23499; -- Voice Trigger Mob (Controls Aggro + Enrage yells)
UPDATE creature_template SET ScriptName='boss_veras_darkshadow' WHERE entry=22952; -- Rogue of Illidari Council
UPDATE creature_template SET ScriptName='boss_teron_gorefiend' WHERE entry=22871; -- Teron Gorefiend
UPDATE creature_template SET ScriptName='boss_supremus' WHERE entry=22898; -- Supremus
UPDATE creature_template SET ScriptName='boss_shade_of_akama' WHERE entry=22841; -- Shade of Akama
UPDATE creature_template SET ScriptName='boss_reliquary_of_souls' WHERE entry=22856; -- Reliquary Controller Mob
UPDATE creature_template SET ScriptName='boss_essence_of_suffering' WHERE entry=23418; -- Essence Of Suffering
UPDATE creature_template SET ScriptName='boss_essence_of_desire' WHERE entry=23419; -- Essence of Desire
UPDATE creature_template SET ScriptName='boss_essence_of_anger' WHERE entry=23420; -- Essence of Anger
UPDATE creature_template SET ScriptName='boss_najentus' WHERE entry=22887; -- High Warlord Naj'entus
UPDATE creature_template SET ScriptName='boss_gurtogg_bloodboil' WHERE entry=22948; -- Gurtogg Bloodboil
UPDATE creature_template SET ScriptName='boss_mother_shahraz' WHERE entry=22947; -- Mother Shahraz
UPDATE creature_template SET ScriptName='boss_lady_malande' WHERE entry=22951; -- Priest <3 at Illidari Council
UPDATE creature_template SET ScriptName='boss_illidan_stormrage' WHERE entry=22917; -- Illidan The Betrayer!
UPDATE creature_template SET ScriptName='boss_high_nethermancer_zerevor' WHERE entry=22950; -- Mage at Illidari Council
UPDATE creature_template SET ScriptName='boss_gathios_the_shatterer' WHERE entry=22949; -- Paladin at Illidari Council
UPDATE creature_template SET ScriptName='boss_maiev_shadowsong' WHERE entry=23197; -- Maiev Shadowsong
UPDATE creature_template SET ScriptName='mob_flame_of_azzinoth' WHERE entry=22997; -- Flame of Azzinoth (Illidan Phase 2)
UPDATE creature_template SET ScriptName='mob_blade_of_azzinoth' WHERE entry=22996; -- Blade of Azzinoth (Illidan Phase 2)
UPDATE creature_template SET ScriptName='mob_cage_trap_trigger' WHERE entry=23304; -- Cage Trap mob in Illidan Phase 3/4 Normal
UPDATE creature_template SET ScriptName='mob_shadow_demon' WHERE entry=23375; -- Shadow Demon in Illidan Demon Form
UPDATE creature_template SET ScriptName='npc_volcano' WHERE entry=23085; -- Supremus Volcano
UPDATE creature_template SET ScriptName='molten_flame' WHERE entry=23095; -- Molten Flame in SUpremus
UPDATE creature_template SET ScriptName='mob_ashtongue_channeler' WHERE entry=23421; -- Ashtongue CHanneler in Shade of AKama
UPDATE creature_template SET ScriptName='mob_ashtongue_sorcerer' WHERE entry=23215; -- Ashtongue Sorcerer in Shade of Akama
UPDATE creature_template SET ScriptName='npc_spirit_of_olum' WHERE entry=23411;
UPDATE creature_template SET ScriptName='npc_enslaved_soul' WHERE entry=23469;

/* BLACKFATHOM DEPTHS */
UPDATE instance_template SET ScriptName='instance_blackfathom_deeps' WHERE map=48;
UPDATE gameobject_template SET ScriptName='go_fire_of_akumai' WHERE entry IN (21118,21119,21120,21121);

/* BLACKROCK DEPTHS */
DELETE FROM scripted_areatrigger WHERE entry=1526;
INSERT INTO scripted_areatrigger VALUES (1526,'at_ring_of_law');
UPDATE instance_template SET ScriptName='instance_blackrock_depths' WHERE map =230;
UPDATE creature_template SET ScriptName='boss_emperor_dagran_thaurissan' WHERE entry=9019;
UPDATE creature_template SET ScriptName='boss_moira_bronzebeard' WHERE entry=8929;
UPDATE creature_template SET ScriptName='boss_ambassador_flamelash' WHERE entry=9156;
UPDATE creature_template SET ScriptName='boss_doomrel' WHERE entry=9039;
UPDATE creature_template SET ScriptName='boss_general_angerforge' WHERE entry=9033;
UPDATE creature_template SET ScriptName='boss_high_interrogator_gerstahn' WHERE entry=9018;
UPDATE creature_template SET ScriptName='boss_coren_direbrew' WHERE entry=23872;
UPDATE creature_template SET ScriptName='npc_grimstone' WHERE entry=10096;
UPDATE creature_template SET ScriptName='npc_theldren_trigger' WHERE entry=16079;
UPDATE creature_template SET ScriptName='npc_kharan_mighthammer' WHERE entry=9021;
UPDATE creature_template SET ScriptName='npc_rocknot' WHERE entry=9503;
UPDATE creature_template SET ScriptName='npc_marshal_windsor' WHERE entry=9023;
UPDATE creature_template SET ScriptName='npc_dughal_stormwing' WHERE entry=9022;
UPDATE creature_template SET ScriptName='npc_tobias_seecher' WHERE entry=9679;
UPDATE gameobject_template SET ScriptName='go_shadowforge_brazier' WHERE entry IN (174744, 174745);
UPDATE gameobject_template SET ScriptName='go_relic_coffer_door' WHERE entry IN (174554, 174555, 174556, 174557, 174558, 174559, 174560, 174561, 174562, 174563, 174564, 174566);

/* BLACKROCK SPIRE */
UPDATE instance_template SET ScriptName='instance_blackrock_spire' WHERE map=229;
UPDATE creature_template SET ScriptName='boss_overlord_wyrmthalak' WHERE entry=9568;
UPDATE creature_template SET ScriptName='boss_gyth' WHERE entry=10339;
UPDATE creature_template SET ScriptName='boss_pyroguard_emberseer' WHERE entry=9816;
DELETE FROM scripted_event_id WHERE id=4884;
INSERT INTO scripted_event_id VALUES
(4884,'event_spell_altar_emberseer');
UPDATE gameobject_template SET ScriptName='go_father_flame' WHERE entry=175245;

/* BLACKWING LAIR */
UPDATE instance_template SET ScriptName='instance_blackwing_lair' WHERE map=469;
UPDATE creature_template SET ScriptName='boss_razorgore' WHERE entry=12435;
UPDATE gameobject_template SET ScriptName='go_black_dragon_egg' WHERE entry=177807;
UPDATE creature_template SET ScriptName='boss_vaelastrasz' WHERE entry=13020;
UPDATE creature_template SET ScriptName='boss_broodlord' WHERE entry=12017;
UPDATE creature_template SET ScriptName='boss_firemaw' WHERE entry=11983;
UPDATE creature_template SET ScriptName='boss_ebonroc' WHERE entry=14601;
UPDATE creature_template SET ScriptName='boss_flamegor' WHERE entry=11981;
UPDATE creature_template SET ScriptName='boss_chromaggus' WHERE entry=14020;
UPDATE creature_template SET ScriptName='boss_victor_nefarius' WHERE entry=10162;
UPDATE creature_template SET ScriptName='boss_nefarian' WHERE entry=11583;

/* BLADE'S EDGE MOUNTAINS */
UPDATE creature_template SET ScriptName='mobs_nether_drake' WHERE entry IN (20021,21817,21820,21821,21823);
UPDATE creature_template SET ScriptName='npc_daranelle' WHERE entry=21469;
UPDATE creature_template SET ScriptName='npc_bloodmaul_stout_trigger' WHERE entry=21241;
UPDATE creature_template SET ScriptName='npc_simon_game_bunny' WHERE entry=22923;

/* BLASTED LANDS */
UPDATE creature_template SET ScriptName='npc_fallen_hero_of_horde' WHERE entry=7572;

/* BLOODMYST ISLE */
UPDATE creature_template SET ScriptName='mob_webbed_creature' WHERE entry=17680;

/* BOREAN TUNDRA */
UPDATE creature_template SET ScriptName='npc_nesingwary_trapper' WHERE entry=25835;
UPDATE creature_template SET ScriptName='npc_oil_stained_wolf' WHERE entry=25791;
UPDATE creature_template SET ScriptName='npc_sinkhole_kill_credit' WHERE entry IN (26248,26249);
UPDATE creature_template SET ScriptName='npc_lurgglbr' WHERE entry=25208;
UPDATE creature_template SET ScriptName='npc_beryl_sorcerer' WHERE entry=25316;
UPDATE creature_template SET ScriptName='npc_captured_beryl_sorcerer' WHERE entry=25474;
UPDATE creature_template SET ScriptName='npc_nexus_drake_hatchling' WHERE entry=26127;
UPDATE creature_template SET ScriptName='npc_scourged_flamespitter' WHERE entry=25582;
UPDATE creature_template SET ScriptName='npc_bonker_togglevolt' WHERE entry=25589;

/* BURNING STEPPES */
UPDATE creature_template SET ScriptName='npc_ragged_john' WHERE entry=9563;
UPDATE creature_template SET ScriptName='npc_grark_lorkrub' WHERE entry=9520;

/*  */
/* CAVERNS OF TIME */
/*  */

/* MT. HYJAL */
UPDATE instance_template SET ScriptName='instance_hyjal' WHERE map=534;
UPDATE creature_template SET ScriptName='npc_tyrande_whisperwind' WHERE entry=17948;
UPDATE creature_template SET ScriptName='npc_thrall' WHERE entry=17852;
UPDATE creature_template SET ScriptName='npc_jaina_proudmoore' WHERE entry=17772;
UPDATE creature_template SET ScriptName='boss_archimonde' WHERE entry=17968;
UPDATE creature_template SET ScriptName='npc_doomfire_spirit' WHERE entry=18104;

/* OLD HILLSBRAD */
UPDATE instance_template SET ScriptName='instance_old_hillsbrad' WHERE map=560;
UPDATE creature_template SET ScriptName='npc_erozion' WHERE entry=18723;
UPDATE creature_template SET ScriptName='npc_taretha' WHERE entry=18887;
UPDATE creature_template SET ScriptName='npc_thrall_old_hillsbrad' WHERE entry=17876;
DELETE FROM scripted_event_id WHERE id=11111;
INSERT INTO scripted_event_id VALUES
(11111,'event_go_barrel_old_hillsbrad');

/* THE CULLING OF STRATHOLME */
UPDATE instance_template SET ScriptName='instance_culling_of_stratholme' WHERE map=595;
UPDATE creature_template SET ScriptName='npc_chromie' WHERE entry IN (26527, 27915);
UPDATE creature_template SET ScriptName='spell_dummy_npc_crates_bunny' WHERE entry=27827;
DELETE FROM scripted_areatrigger WHERE entry=5291;
INSERT INTO scripted_areatrigger VALUES
(5291,'at_culling_of_stratholme');

/* THE DARK PORTAL */
UPDATE creature_template SET ScriptName='boss_chrono_lord_deja' WHERE entry=17879;
UPDATE creature_template SET ScriptName='boss_aeonus' WHERE entry=17881;
UPDATE creature_template SET ScriptName='boss_temporus' WHERE entry=17880;
UPDATE instance_template SET ScriptName='instance_dark_portal' WHERE map=269;
UPDATE creature_template SET ScriptName='npc_medivh_black_morass' WHERE entry=15608;
UPDATE creature_template SET ScriptName='npc_time_rift' WHERE entry=17838;

/*  */
/* COILFANG RESERVOIR */
/*  */

/* THE SLAVE PENS */
UPDATE creature_template SET ScriptName='boss_ahune' WHERE entry=25740;
UPDATE creature_template SET ScriptName='npc_frozen_core' WHERE entry=25865;
UPDATE creature_template SET ScriptName='npc_ice_spear_bunny' WHERE entry=25985;

/* THE UNDERBOG */
UPDATE creature_template SET ScriptName='mob_underbog_mushroom' WHERE entry=17990;
UPDATE creature_template SET ScriptName='boss_hungarfen' WHERE entry=17770;

/* THE STEAMVAULT */
UPDATE instance_template SET ScriptName='instance_steam_vault' WHERE map=545;
UPDATE creature_template SET ScriptName='boss_hydromancer_thespia' WHERE entry=17797;
UPDATE creature_template SET ScriptName='boss_mekgineer_steamrigger' WHERE entry=17796;
UPDATE creature_template SET ScriptName='boss_warlord_kalithresh' WHERE entry=17798;
UPDATE gameobject_template SET ScriptName='go_main_chambers_access_panel' WHERE entry IN (184125,184126);
UPDATE creature_template SET ScriptName='mob_naga_distiller' WHERE entry=17954;
UPDATE creature_template SET ScriptName='mob_steamrigger_mechanic' WHERE entry=17951;

/* SERPENTSHRINE CAVERN */
UPDATE instance_template SET ScriptName='instance_serpent_shrine' WHERE map=548;
UPDATE creature_template SET ScriptName='boss_hydross_the_unstable' WHERE entry=21216;
/* Leotheras the Blind event */
UPDATE creature_template SET ScriptName='boss_leotheras_the_blind' WHERE entry=21215;
/* Fathom-lord Karathress event */
UPDATE creature_template SET ScriptName='boss_fathomlord_karathress' WHERE entry=21214;
UPDATE creature_template SET ScriptName='boss_fathomguard_sharkkis' WHERE entry=21966;
UPDATE creature_template SET ScriptName='boss_fathomguard_tidalvess' WHERE entry=21965;
UPDATE creature_template SET ScriptName='boss_fathomguard_caribdis' WHERE entry=21964;
/* Morogrim Tidewalker event */
UPDATE creature_template SET ScriptName='boss_morogrim_tidewalker' WHERE entry=21213;
UPDATE creature_template SET ScriptName='mob_water_globule' WHERE entry=21913;
/* Lady Vashj event */
UPDATE creature_template SET ScriptName='boss_lady_vashj' WHERE entry=21212;
UPDATE creature_template SET ScriptName='mob_enchanted_elemental' WHERE entry=21958;
UPDATE gameobject_template SET ScriptName='go_shield_generator' WHERE entry IN (185051,185052,185053,185054);
/* The Lurker Below event */
UPDATE gameobject_template SET ScriptName='go_strange_pool' WHERE entry=184956;
UPDATE creature_template SET ScriptName='boss_the_lurker_below' WHERE entry=21217;

/* CRYSTALSONG FOREST */

/*  */
/* CRUSADER COLISEUM */
/*  */

/* TRAIL OF THE CHAMPION */

/* TRIAL OF THE CRUSADER */
UPDATE instance_template SET ScriptName='instance_trial_of_the_crusader' WHERE map=649;
UPDATE creature_template SET ScriptName='npc_barrett_ramsey' WHERE entry IN (34816, 35035, 35766, 35770, 35771);
UPDATE creature_template SET ScriptName='npc_beast_combat_stalker' WHERE entry=36549;
UPDATE creature_template SET ScriptName='boss_gormok' WHERE entry=34796;
UPDATE creature_template SET ScriptName='boss_acidmaw' WHERE entry=35144;
UPDATE creature_template SET ScriptName='boss_dreadscale' WHERE entry=34799;
UPDATE creature_template SET ScriptName='boss_icehowl' WHERE entry=34797;
UPDATE creature_template SET ScriptName='boss_jaraxxus' WHERE entry=34780;
UPDATE creature_template SET ScriptName='boss_anubarak_trial' WHERE entry=34564;
UPDATE creature_template SET ScriptName='boss_fjola' WHERE entry=34497;
UPDATE creature_template SET ScriptName='boss_eydis' WHERE entry=34496;
UPDATE creature_template SET ScriptName='npc_anubarak_spike' WHERE entry=34660;
UPDATE creature_template SET ScriptName='npc_frost_sphere' WHERE entry=34606;
UPDATE creature_template SET ScriptName='npc_nerubian_borrow' WHERE entry=34862;

/* DALARAN */
UPDATE creature_template SET ScriptName='npc_dalaran_guardian_mage' WHERE entry IN (29255, 29254);

/* DARKSHORE */
UPDATE creature_template SET ScriptName='npc_kerlonian' WHERE entry=11218;
UPDATE creature_template SET ScriptName='npc_prospector_remtravel' WHERE entry=2917;
UPDATE creature_template SET ScriptName='npc_threshwackonator' WHERE entry=6669;
UPDATE creature_template SET ScriptName='npc_volcor' WHERE entry=3692;
UPDATE creature_template SET ScriptName='npc_therylune' WHERE entry=3584;
UPDATE creature_template SET ScriptName='npc_rabid_bear' WHERE entry=2164;

/* DARNASSUS */

/* DEADMINES */
UPDATE creature_template SET ScriptName='boss_mr_smite' WHERE entry=646;
UPDATE instance_template SET ScriptName='instance_deadmines' WHERE map=36;
UPDATE gameobject_template SET ScriptName='go_defias_cannon' WHERE entry=16398;

/* DEADWIND PASS */

/* DESOLACE */
UPDATE creature_template SET ScriptName='npc_aged_dying_ancient_kodo' WHERE entry IN (4700, 4701, 4702, 11627);
UPDATE creature_template SET ScriptName='npc_dalinda_malem' WHERE entry=5644;
UPDATE creature_template SET ScriptName='npc_melizza_brimbuzzle' WHERE entry=12277;

/* DIRE MAUL */
UPDATE instance_template SET ScriptName='instance_dire_maul' WHERE map=429;

/* DRAGONBLIGHT */
UPDATE creature_template SET ScriptName='npc_destructive_ward' WHERE entry=27430;
UPDATE creature_template SET ScriptName='npc_crystalline_ice_giant' WHERE entry=26809;

/* DRAK'THARON KEEP */
UPDATE creature_template SET ScriptName='boss_novos' WHERE entry=26631;
UPDATE creature_template SET ScriptName='npc_crystal_channel_target' WHERE entry=26712;
UPDATE creature_template SET ScriptName='boss_tharonja' WHERE entry=26632;
UPDATE creature_template SET ScriptName='boss_trollgore' WHERE entry=26630;
UPDATE instance_template SET ScriptName='instance_draktharon_keep' WHERE map=600;

/* DUN MOROGH */

/* DUROTAR */
UPDATE creature_template SET ScriptName='npc_lazy_peon' WHERE entry=10556;

/* DUSKWOOD */

/* DUSTWALLOW MARSH */
UPDATE creature_template SET ScriptName='mobs_risen_husk_spirit' WHERE entry IN (23554,23555);
UPDATE creature_template SET ScriptName='npc_ogron' WHERE entry=4983;
UPDATE creature_template SET ScriptName='npc_morokk' WHERE entry=4500;
UPDATE creature_template SET ScriptName='npc_restless_apparition' WHERE entry=23861;
UPDATE creature_template SET ScriptName='npc_private_hendel' WHERE entry=4966;
UPDATE creature_template SET ScriptName='npc_stinky_ignatz' WHERE entry=4880;
UPDATE creature_template SET ScriptName='boss_tethyr' WHERE entry=23899;
DELETE FROM scripted_areatrigger WHERE entry=4752;
INSERT INTO scripted_areatrigger VALUES
(4752,'at_nats_landing');

/* EASTERN PLAGUELANDS */
UPDATE creature_template SET ScriptName='npc_eris_havenfire' WHERE entry=14494;

/* EBON HOLD */
UPDATE creature_template SET ScriptName='npc_death_knight_initiate' WHERE entry=28406;
UPDATE creature_template SET ScriptName='npc_unworthy_initiate_anchor' WHERE entry=29521;
UPDATE creature_template SET ScriptName='npc_unworthy_initiate' WHERE entry IN (29519,29520,29565,29566,29567);
UPDATE gameobject_template SET ScriptName='go_acherus_soul_prison' WHERE entry IN (191577,191580,191581,191582,191583,191584,191585,191586,191587,191588,191589,191590);
UPDATE creature_template SET ScriptName='npc_a_special_surprise' WHERE entry IN (29032,29061,29065,29067,29068,29070,29074,29072,29073,29071);
UPDATE creature_template SET ScriptName='npc_koltira_deathweaver' WHERE entry=28912;
UPDATE creature_template SET ScriptName='npc_eye_of_acherus' WHERE entry=28511;
UPDATE creature_template SET ScriptName='npc_scarlet_ghoul' WHERE entry=28845;
UPDATE creature_template SET ScriptName='npc_highlord_darion_mograine' WHERE entry=29173;
UPDATE creature_template SET ScriptName='npc_fellow_death_knight' WHERE entry IN (29199, 29204, 29200);
UPDATE creature_template SET ScriptName='npc_lich_king_light_dawn' WHERE entry=29183;
UPDATE creature_template SET ScriptName='npc_acherus_deathcharger' WHERE entry=28782;

/* ELWYNN FOREST */

/* EVERSONG WOODS */
UPDATE creature_template SET ScriptName='npc_kelerun_bloodmourn' WHERE entry=17807;
UPDATE gameobject_template SET ScriptName='go_harbinger_second_trial' WHERE entry=182052;
UPDATE creature_template SET ScriptName='npc_prospector_anvilward' WHERE entry=15420;
UPDATE creature_template SET ScriptName='npc_apprentice_mirveda' WHERE entry=15402;
UPDATE creature_template SET ScriptName='npc_infused_crystal' WHERE entry=16364;

/* FELWOOD */
DELETE FROM scripted_event_id WHERE id=8328;
INSERT INTO scripted_event_id VALUES
(8328, 'npc_kroshius');
UPDATE creature_template SET ScriptName='npc_kitten' WHERE entry=9937;
UPDATE creature_template SET ScriptName='npc_corrupt_saber' WHERE entry=10042;
UPDATE creature_template SET ScriptName='npc_niby_the_almighty' WHERE entry=14469;
UPDATE creature_template SET ScriptName='npc_kroshius' WHERE entry=14467;
UPDATE creature_template SET ScriptName='npc_captured_arkonarin' WHERE entry=11016;
UPDATE creature_template SET ScriptName='npc_arei' WHERE entry=9598;

/* FERALAS */
UPDATE creature_template SET ScriptName='npc_oox22fe' WHERE entry=7807;
UPDATE creature_template SET ScriptName='npc_shay_leafrunner' WHERE entry=7774;

/* GHOSTLANDS */
UPDATE creature_template SET ScriptName='npc_ranger_lilatha' WHERE entry=16295;

/* GNOMEREGAN */
UPDATE creature_template SET ScriptName='boss_thermaplugg' WHERE entry=7800;
UPDATE gameobject_template SET ScriptName='go_gnomeface_button' WHERE entry BETWEEN 142214 AND 142219;
UPDATE creature_template SET ScriptName='npc_blastmaster_emi_shortfuse' WHERE entry=7998;
UPDATE creature_template SET ScriptName='npc_kernobee' WHERE entry=7850;
UPDATE instance_template SET ScriptName='instance_gnomeregan' WHERE map=90;

/* GRIZZLY HILLS */
UPDATE creature_template SET ScriptName='npc_depleted_war_golem' WHERE entry=27017;
UPDATE creature_template SET ScriptName='npc_harrison_jones' WHERE entry=26814;

/* GRUUL'S LAIR */
UPDATE instance_template SET ScriptName='instance_gruuls_lair' WHERE map =565;
UPDATE creature_template SET ScriptName='boss_gruul' WHERE entry=19044;
/* Maulgar and Event */
UPDATE creature_template SET ScriptName='boss_high_king_maulgar' WHERE entry=18831;
UPDATE creature_template SET ScriptName='boss_kiggler_the_crazed' WHERE entry=18835;
UPDATE creature_template SET ScriptName='boss_blindeye_the_seer' WHERE entry=18836;
UPDATE creature_template SET ScriptName='boss_olm_the_summoner' WHERE entry=18834;
UPDATE creature_template SET ScriptName='boss_krosh_firehand' WHERE entry=18832;

/* GUNDRAK */
UPDATE creature_template SET ScriptName='boss_drakkari_colossus' WHERE entry=29307;
UPDATE creature_template SET ScriptName='boss_drakkari_elemental' WHERE entry=29573;
UPDATE creature_template SET ScriptName='npc_living_mojo' WHERE entry=29830;
UPDATE creature_template SET ScriptName='boss_eck' WHERE entry=29932;
UPDATE creature_template SET ScriptName='boss_galdarah' WHERE entry=29306;
UPDATE creature_template SET ScriptName='boss_moorabi' WHERE entry=29305;
UPDATE creature_template SET ScriptName='boss_sladran' WHERE entry=29304;
UPDATE gameobject_template SET ScriptName='go_gundrak_altar' WHERE entry IN (192518, 192519, 192520);
UPDATE instance_template SET ScriptName='instance_gundrak' WHERE map=604;

/*  */
/* HELLFIRE CITADEL */
/*  */

/* BLOOD FURNACE */
/* The Maker,Broggok,Kelidan,Broggok's cloud */
UPDATE creature_template SET ScriptName='boss_the_maker' WHERE entry=17381;
UPDATE creature_template SET ScriptName='boss_broggok' WHERE entry=17380;
UPDATE creature_template SET ScriptName='boss_kelidan_the_breaker' WHERE entry=17377;
UPDATE creature_template SET ScriptName='mob_broggok_poisoncloud' WHERE entry=17662;
UPDATE creature_template SET ScriptName='mob_shadowmoon_channeler' WHERE entry=17653;
UPDATE gameobject_template SET ScriptName='go_prison_cell_lever' WHERE entry=181982;
UPDATE instance_template SET ScriptName='instance_blood_furnace' WHERE map=542;

/* HELLFIRE RAMPARTS */
/* Vazruden,Omor the Unscarred,Watchkeeper Gargolmar */
UPDATE creature_template SET ScriptName='boss_omor_the_unscarred' WHERE entry=17308;
UPDATE creature_template SET ScriptName='boss_watchkeeper_gargolmar' WHERE entry=17306;
UPDATE creature_template SET ScriptName='boss_vazruden_herald' WHERE entry=17307;
UPDATE creature_template SET ScriptName='boss_vazruden' WHERE entry=17537;
UPDATE instance_template SET ScriptName='instance_ramparts' WHERE map=543;

/* SHATTERED HALLS */
/* Nethekurse and his spawned shadowfissure */
UPDATE creature_template SET ScriptName='boss_grand_warlock_nethekurse' WHERE entry=16807;
UPDATE creature_template SET ScriptName='boss_warbringer_omrogg' WHERE entry=16809;
UPDATE creature_template SET ScriptName='mob_fel_orc_convert' WHERE entry=17083;
UPDATE creature_template SET ScriptName='mob_lesser_shadow_fissure' WHERE entry=17471;
UPDATE creature_template SET ScriptName='mob_omrogg_heads' WHERE entry IN (19523,19524);
UPDATE creature_template SET ScriptName='boss_warchief_kargath_bladefist' WHERE entry=16808;
UPDATE instance_template SET ScriptName='instance_shattered_halls' WHERE map=540;

/* MAGTHERIDON'S LAIR */
UPDATE instance_template SET ScriptName='instance_magtheridons_lair' WHERE map=544;
UPDATE gameobject_template SET ScriptName='go_manticron_cube' WHERE entry=181713;
UPDATE creature_template SET ScriptName='boss_magtheridon' WHERE entry=17257;
UPDATE creature_template SET ScriptName='mob_abyssal' WHERE entry=17454;
UPDATE creature_template SET ScriptName='mob_hellfire_channeler' WHERE entry=17256;
UPDATE creature_template SET ScriptName='npc_target_trigger' WHERE entry=17474;

/* HELLFIRE PENINSULA */
UPDATE creature_template SET ScriptName='boss_doomlord_kazzak' WHERE entry=18728;
UPDATE creature_template SET ScriptName='npc_aeranas' WHERE entry=17085;
UPDATE creature_template SET ScriptName='npc_ancestral_wolf' WHERE entry=17077;
UPDATE creature_template SET ScriptName='npc_demoniac_scryer' WHERE entry=22258;
UPDATE creature_template SET ScriptName='npc_wounded_blood_elf' WHERE entry=16993;
UPDATE creature_template SET ScriptName='npc_fel_guard_hound' WHERE entry=21847;
UPDATE creature_template SET ScriptName='npc_anchorite_barada' WHERE entry=22431;
UPDATE creature_template SET ScriptName='npc_colonel_jules' WHERE entry=22432;

/* HILLSBRAD FOOTHILLS */

/* HINTERLANDS */
UPDATE creature_template SET ScriptName='npc_00x09hl' WHERE entry=7806;
UPDATE creature_template SET ScriptName='npc_rinji' WHERE entry=7780;

/* HOWLING FJORD */
DELETE FROM scripted_areatrigger WHERE entry IN (4778,4779);
INSERT INTO scripted_areatrigger VALUES
(4778,'at_ancient_male_vrykul'),
(4779,'at_nifflevar');
UPDATE creature_template SET ScriptName='npc_ancient_male_vrykul' WHERE entry=24314;
UPDATE creature_template SET ScriptName='npc_daegarn' WHERE entry=24151;
UPDATE creature_template SET ScriptName='npc_silvermoon_harry' WHERE entry=24539;
UPDATE creature_template SET ScriptName='npc_lich_king_village' WHERE entry=24248;
UPDATE creature_template SET ScriptName='npc_king_ymiron' WHERE entry=24321;
UPDATE creature_template SET ScriptName='npc_firecrackers_bunny' WHERE entry=24230;
UPDATE creature_template SET ScriptName='npc_apothecary_hanes' WHERE entry=23784;
UPDATE creature_template SET ScriptName='npc_scalawag_frog' WHERE entry=26503;

/*  */
/* ICECROWN CITADEL */
/*  */

/* ICECROWN CITADEL */
UPDATE instance_template SET ScriptName='instance_icecrown_citadel' WHERE map=631;
UPDATE creature_template SET ScriptName='boss_lord_marrowgar' WHERE entry=36612;
UPDATE creature_template SET ScriptName='boss_lady_deathwhisper' WHERE entry=36855;
UPDATE creature_template SET ScriptName='boss_deathbringer_saurfang' WHERE entry=37813;
UPDATE creature_template SET ScriptName='npc_queen_lanathel_intro' WHERE entry=38004;
UPDATE creature_template SET ScriptName='npc_blood_orb_control' WHERE entry=38008;
UPDATE creature_template SET ScriptName='npc_ball_of_flame' WHERE entry IN (38332,38451);
UPDATE creature_template SET ScriptName='npc_kinetic_bomb' WHERE entry=38454;
UPDATE creature_template SET ScriptName='npc_dark_nucleus' WHERE entry=38369;
UPDATE creature_template SET ScriptName='boss_taldaram_icc' WHERE entry=37973;
UPDATE creature_template SET ScriptName='boss_keleseth_icc' WHERE entry=37972;
UPDATE creature_template SET ScriptName='boss_valanar_icc' WHERE entry=37970;
UPDATE creature_template SET ScriptName='boss_blood_queen_lanathel' WHERE entry=37955;
UPDATE creature_template SET ScriptName='boss_sindragosa' WHERE entry=36853;
UPDATE creature_template SET ScriptName='npc_rimefang_icc' WHERE entry=37533;
UPDATE creature_template SET ScriptName='npc_spinestalker_icc' WHERE entry=37534;
UPDATE creature_template SET ScriptName='mob_frost_bomb' WHERE entry=37186;
UPDATE creature_template SET ScriptName='boss_festergut' WHERE entry=36626;
UPDATE creature_template SET ScriptName='boss_rotface' WHERE entry=36627;
UPDATE creature_template SET ScriptName='mob_little_ooze' WHERE entry=36897;
UPDATE creature_template SET ScriptName='mob_big_ooze' WHERE entry=36899;
UPDATE creature_template SET ScriptName='boss_valithria_dreamwalker' WHERE entry=36789;
UPDATE creature_template SET ScriptName='boss_professor_putricide' WHERE entry=36678;
UPDATE creature_template SET ScriptName='boss_the_lich_king_icc' WHERE entry=36597;
DELETE FROM scripted_event_id WHERE id IN (23426,23438);
INSERT INTO scripted_event_id VALUES
(23426,'event_gameobject_citadel_valve'),
(23438,'event_gameobject_citadel_valve');

/* FORGE OF SOULS */
UPDATE creature_template SET ScriptName='boss_bronjahm' WHERE entry=36497;
UPDATE creature_template SET ScriptName='npc_corrupted_soul_fragment' WHERE entry=36535;
UPDATE creature_template SET ScriptName='boss_devourer_of_souls' WHERE entry=36502;
UPDATE instance_template SET ScriptName='instance_forge_of_souls' WHERE map=632;

/* HALLS OF REFLECTION */

/* PIT OF SARON */
UPDATE instance_template SET ScriptName='instance_pit_of_saron' WHERE map=658;
UPDATE creature_template SET ScriptName='boss_forgemaster_garfrost' WHERE entry=36494;
UPDATE creature_template SET ScriptName='boss_krick' WHERE entry=36477;
UPDATE creature_template SET ScriptName='boss_ick' WHERE entry=36476;
UPDATE creature_template SET ScriptName='npc_exploding_orb' WHERE entry=36610;
UPDATE creature_template SET ScriptName='npc_ymirjar_deathbringer' WHERE entry=36892;
UPDATE creature_template SET ScriptName='npc_collapsing_icicle' WHERE entry=36847;
UPDATE creature_template SET ScriptName='boss_tyrannus' WHERE entry=36658;
UPDATE creature_template SET ScriptName='boss_rimefang_pos' WHERE entry=36661;
DELETE FROM scripted_areatrigger WHERE entry IN (5578,5581);
INSERT INTO scripted_areatrigger VALUES
(5578,'at_pit_of_saron'),
(5581,'at_pit_of_saron');

/* ICECROWN */
UPDATE creature_template SET ScriptName='npc_squad_leader' WHERE entry IN (31737,31833);
UPDATE creature_template SET ScriptName='npc_infantry' WHERE entry IN (31701,31832);
UPDATE creature_template SET ScriptName='npc_father_kamaros' WHERE entry IN (31279,32800);
UPDATE creature_template SET ScriptName='npc_saronite_mine_slave' WHERE entry=31397;

/* IRONFORGE */

/* ISLE OF QUEL'DANAS */
UPDATE creature_template SET ScriptName='npc_converted_sentry' WHERE entry=24981;

/* KARAZHAN */
UPDATE instance_template SET ScriptName='instance_karazhan' WHERE map=532;
UPDATE creature_template SET ScriptName='boss_midnight' WHERE entry=16151;
UPDATE creature_template SET ScriptName='boss_attumen' WHERE entry IN (15550,16152);
UPDATE creature_template SET ScriptName='boss_moroes' WHERE entry=15687;
UPDATE creature_template SET ScriptName='boss_maiden_of_virtue' WHERE entry=16457;
UPDATE creature_template SET ScriptName='boss_curator' WHERE entry=15691;
UPDATE creature_template SET ScriptName='boss_julianne' WHERE entry=17534;
UPDATE creature_template SET ScriptName='boss_romulo' WHERE entry=17533;
UPDATE creature_template SET ScriptName='boss_dorothee' WHERE entry=17535;
UPDATE creature_template SET ScriptName='boss_strawman' WHERE entry=17543;
UPDATE creature_template SET ScriptName='boss_tinhead' WHERE entry=17547;
UPDATE creature_template SET ScriptName='boss_roar' WHERE entry=17546;
UPDATE creature_template SET ScriptName='boss_crone' WHERE entry=18168;
UPDATE creature_template SET ScriptName='boss_terestian_illhoof' WHERE entry=15688;
UPDATE creature_template SET ScriptName='boss_shade_of_aran' WHERE entry=16524;
UPDATE creature_template SET ScriptName='boss_netherspite' WHERE entry=15689;
UPDATE creature_template SET ScriptName='boss_malchezaar' WHERE entry=15690;
UPDATE creature_template SET ScriptName='boss_nightbane' WHERE entry=17225;
UPDATE creature_template SET ScriptName='boss_bigbadwolf' WHERE entry=17521;
UPDATE creature_template SET ScriptName='mob_demon_chain' WHERE entry=17248;
UPDATE creature_template SET ScriptName='npc_fiendish_portal' WHERE entry=17265;
UPDATE creature_template SET ScriptName='npc_shade_of_aran_blizzard' WHERE entry=17161;
UPDATE creature_template SET ScriptName='npc_netherspite_portal' WHERE entry IN (17367,17368,17369);
UPDATE creature_template SET ScriptName='npc_infernal_target' WHERE entry=17644;
UPDATE creature_template SET ScriptName='npc_berthold' WHERE entry=16153;
UPDATE creature_template SET ScriptName='npc_barnes' WHERE entry=16812;
UPDATE creature_template SET ScriptName='npc_grandmother' WHERE entry=17603;
UPDATE creature_template SET ScriptName='npc_image_of_medivh' WHERE entry=17651;
UPDATE creature_template SET ScriptName='npc_image_arcanagos' WHERE entry=17652;
UPDATE creature_template SET ScriptName='npc_echo_of_medivh' WHERE entry=16816;
UPDATE creature_template SET ScriptName='npc_king_llane' WHERE entry=21684;
UPDATE creature_template SET ScriptName='npc_warchief_blackhand' WHERE entry=21752;
UPDATE creature_template SET ScriptName='npc_human_conjurer' WHERE entry=21683;
UPDATE creature_template SET ScriptName='npc_orc_warlock' WHERE entry=21750;
UPDATE creature_template SET ScriptName='npc_human_footman' WHERE entry=17211;
UPDATE creature_template SET ScriptName='npc_orc_grunt' WHERE entry=17469;
UPDATE creature_template SET ScriptName='npc_water_elemental' WHERE entry=21160;
UPDATE creature_template SET ScriptName='npc_summoned_daemon' WHERE entry=21726;
UPDATE creature_template SET ScriptName='npc_human_charger' WHERE entry=21664;
UPDATE creature_template SET ScriptName='npc_orc_wolf' WHERE entry=21748;
UPDATE creature_template SET ScriptName='npc_human_cleric' WHERE entry=21682;
UPDATE creature_template SET ScriptName='npc_orc_necrolyte' WHERE entry=21747;
DELETE FROM scripted_event_id WHERE id IN (10591,10951);
INSERT INTO scripted_event_id VALUES
(10591,'event_spell_summon_nightbane'),
(10951,'event_spell_medivh_journal');

/* LOCH MODAN */
UPDATE creature_template SET ScriptName='npc_mountaineer_pebblebitty' WHERE entry=3836;
UPDATE creature_template SET ScriptName='npc_miran' WHERE entry=1379;

/* MAGISTER'S TERRACE */
UPDATE instance_template SET ScriptName='instance_magisters_terrace' WHERE map=585;
UPDATE creature_template SET ScriptName='boss_selin_fireheart' WHERE entry=24723;
UPDATE creature_template SET ScriptName='mob_fel_crystal' WHERE entry=24722;
UPDATE creature_template SET ScriptName='boss_vexallus' WHERE entry=24744;
UPDATE creature_template SET ScriptName='mob_pure_energy' WHERE entry=24745;
UPDATE creature_template SET ScriptName='boss_priestess_delrissa' WHERE entry=24560;
UPDATE creature_template SET ScriptName='npc_kagani_nightstrike' WHERE entry=24557;
UPDATE creature_template SET ScriptName='npc_ellris_duskhallow' WHERE entry=24558;
UPDATE creature_template SET ScriptName='npc_eramas_brightblaze' WHERE entry=24554;
UPDATE creature_template SET ScriptName='npc_yazzai' WHERE entry=24561;
UPDATE creature_template SET ScriptName='npc_warlord_salaris' WHERE entry=24559;
UPDATE creature_template SET ScriptName='npc_garaxxas' WHERE entry=24555;
UPDATE creature_template SET ScriptName='npc_apoko' WHERE entry=24553;
UPDATE creature_template SET ScriptName='npc_zelfan' WHERE entry=24556;
UPDATE creature_template SET ScriptName='boss_felblood_kaelthas' WHERE entry=24664;
UPDATE creature_template SET ScriptName='mob_arcane_sphere' WHERE entry=24708;
UPDATE creature_template SET ScriptName='mob_felkael_phoenix' WHERE entry=24674;
UPDATE creature_template SET ScriptName='mob_felkael_phoenix_egg' WHERE entry=24675;
UPDATE creature_template SET ScriptName='npc_kalecgos' WHERE entry=24844;
DELETE FROM scripted_event_id WHERE id=16547;
INSERT INTO scripted_event_id VALUES
(16547,'event_go_scrying_orb');

/* MARAUDON */
UPDATE creature_template SET ScriptName='boss_noxxion' WHERE entry=13282;

/* MOLTEN CORE */
UPDATE instance_template SET ScriptName='instance_molten_core' WHERE map=409;
UPDATE creature_template SET ScriptName='boss_lucifron' WHERE entry=12118;
UPDATE creature_template SET ScriptName='boss_magmadar' WHERE entry=11982;
UPDATE creature_template SET ScriptName='boss_gehennas' WHERE entry=12259;
UPDATE creature_template SET ScriptName='boss_garr' WHERE entry=12057;
UPDATE creature_template SET ScriptName='boss_baron_geddon' WHERE entry=12056;
UPDATE creature_template SET ScriptName='boss_shazzrah' WHERE entry=12264;
UPDATE creature_template SET ScriptName='boss_golemagg' WHERE entry=11988;
UPDATE creature_template SET ScriptName='boss_sulfuron' WHERE entry=12098;
UPDATE creature_template SET ScriptName='boss_majordomo' WHERE entry=12018;
UPDATE creature_template SET ScriptName='boss_ragnaros' WHERE entry=11502;
UPDATE creature_template SET ScriptName='mob_firesworn' WHERE entry=12099;
UPDATE creature_template SET ScriptName='mob_core_rager' WHERE entry=11672;
UPDATE creature_template SET ScriptName='mob_flamewaker_priest' WHERE entry=11662;

/* MOONGLADE */
UPDATE creature_template SET ScriptName='npc_clintar_dw_spirit' WHERE entry=22916;
UPDATE creature_template SET ScriptName='npc_keeper_remulos' WHERE entry=11832;
UPDATE creature_template SET ScriptName='boss_eranikus' WHERE entry=15491;

/* MULGORE */
UPDATE creature_template SET ScriptName='npc_kyle_the_frenzied' WHERE entry=23616;

/* NAGRAND */
UPDATE creature_template SET ScriptName='mob_lump' WHERE entry=18351;
UPDATE creature_template SET ScriptName='npc_nagrand_captive' WHERE entry IN (18209,18210);
UPDATE creature_template SET ScriptName='npc_creditmarker_visit_with_ancestors' WHERE entry IN (18840,18841,18842,18843);
UPDATE creature_template SET ScriptName='npc_rethhedron' WHERE entry=22357;

/* NAXXRAMAS */
UPDATE instance_template SET ScriptName='instance_naxxramas' WHERE map=533;
UPDATE creature_template SET ScriptName='boss_anubrekhan' WHERE entry=15956;
UPDATE creature_template SET ScriptName='boss_faerlina' WHERE entry=15953;
UPDATE creature_template SET ScriptName='boss_maexxna' WHERE entry=15952;
UPDATE creature_template SET ScriptName='npc_web_wrap' WHERE entry=16486;
UPDATE creature_template SET ScriptName='boss_noth' WHERE entry=15954;
UPDATE creature_template SET ScriptName='boss_heigan' WHERE entry=15936;
UPDATE creature_template SET ScriptName='boss_loatheb' WHERE entry=16011;
UPDATE creature_template SET ScriptName='boss_razuvious' WHERE entry=16061;
UPDATE creature_template SET ScriptName='boss_gothik' WHERE entry=16060;
UPDATE creature_template SET ScriptName='spell_anchor' WHERE entry=16137;
UPDATE creature_template SET ScriptName='boss_thane_korthazz' WHERE entry=16064;
UPDATE creature_template SET ScriptName='boss_sir_zeliek' WHERE entry=16063;
UPDATE creature_template SET ScriptName='boss_lady_blaumeux' WHERE entry=16065;
UPDATE creature_template SET ScriptName='boss_rivendare_naxx' WHERE entry=30549;
UPDATE creature_template SET ScriptName='boss_patchwerk' WHERE entry=16028;
UPDATE creature_template SET ScriptName='boss_grobbulus' WHERE entry=15931;
UPDATE creature_template SET ScriptName='boss_gluth' WHERE entry=15932;
UPDATE creature_template SET ScriptName='boss_thaddius' WHERE entry=15928;
UPDATE creature_template SET ScriptName='boss_stalagg' WHERE entry=15929;
UPDATE creature_template SET ScriptName='boss_feugen' WHERE entry=15930;
UPDATE creature_template SET ScriptName='npc_tesla_coil' WHERE entry=16218;
UPDATE creature_template SET ScriptName='boss_sapphiron' WHERE entry=15989;
UPDATE gameobject_template SET ScriptName='go_sapphiron_birth' WHERE entry=181356;
UPDATE creature_template SET ScriptName='boss_kelthuzad' WHERE entry=15990;

/* NETHERSTORM */
DELETE FROM scripted_areatrigger WHERE entry=4497;
INSERT INTO scripted_areatrigger VALUES (4497,'at_commander_dawnforge');
UPDATE gameobject_template SET ScriptName='go_manaforge_control_console' WHERE entry IN (183770,183956,184311,184312);
UPDATE creature_template SET ScriptName='npc_manaforge_control_console' WHERE entry IN (20209,20417,20418,20440);
UPDATE creature_template SET ScriptName='npc_commander_dawnforge' WHERE entry=19831;
UPDATE creature_template SET ScriptName='npc_bessy' WHERE entry=20415;
UPDATE creature_template SET ScriptName='npc_maxx_a_million' WHERE entry=19589;
UPDATE creature_template SET ScriptName='npc_zeppit' WHERE entry=22484;
UPDATE creature_template SET ScriptName='npc_protectorate_demolitionist' WHERE entry=20802;
UPDATE creature_template SET ScriptName='npc_captured_vanguard' WHERE entry=20763;
UPDATE creature_template SET ScriptName='npc_drijya' WHERE entry=20281;
UPDATE creature_template SET ScriptName='npc_dimensius' WHERE entry=19554;

/*  */
/* THE NEXUS */
/*  */

/* EYE OF ETERNITY */
UPDATE instance_template SET ScriptName='instance_eye_of_eternity' WHERE map=616;
UPDATE creature_template SET ScriptName='boss_malygos' WHERE entry=28859;
UPDATE creature_template SET ScriptName='npc_power_spark' WHERE entry=30084;
UPDATE creature_template SET ScriptName='npc_wyrmrest_skytalon' WHERE entry=30161;
DELETE FROM scripted_event_id WHERE id=20711;
INSERT INTO scripted_event_id VALUES
(20711,'event_go_focusing_iris');

/* NEXUS */
UPDATE creature_template SET ScriptName='boss_anomalus' WHERE entry=26763;
UPDATE creature_template SET ScriptName='mob_chaotic_rift' WHERE entry=26918;
UPDATE creature_template SET ScriptName='boss_keristrasza' WHERE entry=26723;
UPDATE creature_template SET ScriptName='boss_ormorok' WHERE entry=26794;
UPDATE creature_template SET ScriptName='npc_crystal_spike_trigger' WHERE entry IN (27101, 27079);
UPDATE creature_template SET ScriptName='boss_telestra' WHERE entry=26731;
UPDATE gameobject_template SET ScriptName='go_containment_sphere' WHERE entry IN (188526, 188527, 188528);
UPDATE instance_template SET ScriptName='instance_nexus' WHERE map=576;

/* OCULUS */
UPDATE instance_template SET ScriptName='instance_oculus' WHERE map=578;
UPDATE creature_template SET ScriptName='boss_eregos' WHERE entry=27656;
UPDATE creature_template SET ScriptName='boss_urom' WHERE entry=27655;
UPDATE creature_template SET ScriptName='boss_varos' WHERE entry=27447;
UPDATE creature_template SET ScriptName='npc_azure_ring_captain' WHERE entry=28236;
UPDATE creature_template SET ScriptName='npc_arcane_beam' WHERE entry=28239;
UPDATE creature_template SET ScriptName='npc_centrifuge_core' WHERE entry=28183;
UPDATE creature_template SET ScriptName='npc_planar_anomaly' WHERE entry=30879;
UPDATE creature_template SET ScriptName='npc_oculus_drake' WHERE entry IN (27756, 27692, 27755);
DELETE FROM scripted_event_id WHERE id IN (10665,12229,18454,18455);
INSERT INTO scripted_event_id VALUES
(10665,'event_spell_call_captain'),
(12229,'event_spell_call_captain'),
(18454,'event_spell_call_captain'),
(18455,'event_spell_call_captain');

/* OBSIDIAN SANCTUM */
UPDATE instance_template SET ScriptName='instance_obsidian_sanctum' WHERE map=615;
UPDATE creature_template SET ScriptName='boss_sartharion' WHERE entry=28860;
UPDATE creature_template SET ScriptName='mob_vesperon' WHERE entry=30449;
UPDATE creature_template SET ScriptName='mob_shadron' WHERE entry=30451;
UPDATE creature_template SET ScriptName='mob_tenebron' WHERE entry=30452;
UPDATE creature_template SET ScriptName='mob_twilight_eggs' WHERE entry IN (30882,31204);
UPDATE creature_template SET ScriptName='npc_tenebron_egg_controller' WHERE entry=31138;
UPDATE creature_template SET ScriptName='npc_flame_tsunami' WHERE entry=30616;
UPDATE creature_template SET ScriptName='npc_fire_cyclone' WHERE entry=30648;

/* ONYXIA'S LAIR */
UPDATE instance_template SET ScriptName='instance_onyxias_lair' WHERE map=249;
UPDATE creature_template SET ScriptName='boss_onyxia' WHERE entry=10184;

/* ORGRIMMAR */
UPDATE creature_template SET ScriptName='npc_shenthul' WHERE entry=3401;
UPDATE creature_template SET ScriptName='npc_thrall_warchief' WHERE entry=4949;

/* RAGEFIRE CHASM */

/* RAZORFEN DOWNS */
UPDATE creature_template SET ScriptName='npc_belnistrasz' WHERE entry=8516;

/* RAZORFEN KRAUL */
UPDATE instance_template SET ScriptName='instance_razorfen_kraul' WHERE map=47;
UPDATE creature_template SET ScriptName='npc_willix_the_importer' WHERE entry=4508;
UPDATE creature_template SET ScriptName='npc_snufflenose_gopher' WHERE entry=4781;

/* REDRIDGE MOUNTAINS */
UPDATE creature_template SET ScriptName='npc_corporal_keeshan' WHERE entry=349;

/* RUBY SANCTUM */
UPDATE instance_template SET ScriptName='instance_ruby_sanctum' WHERE map=724;
UPDATE creature_template SET ScriptName='boss_baltharus' WHERE entry=39751;
UPDATE creature_template SET ScriptName='boss_saviana' WHERE entry=39747;
UPDATE creature_template SET ScriptName='boss_zarithrian' WHERE entry=39746;
UPDATE creature_template SET ScriptName='npc_baltharus_clone' WHERE entry=39899;
UPDATE creature_template SET ScriptName='boss_halion_real' WHERE entry=39863;
UPDATE creature_template SET ScriptName='boss_halion_twilight' WHERE entry=40142;

/* RUINS OF AHN'QIRAJ */
UPDATE instance_template SET ScriptName='instance_ruins_of_ahnqiraj' WHERE map=509;
UPDATE creature_template SET ScriptName='mob_anubisath_guardian' WHERE entry=15355;
UPDATE creature_template SET ScriptName='boss_kurinnaxx' WHERE entry=15348;
UPDATE creature_template SET ScriptName='boss_ayamiss' WHERE entry=15369;
UPDATE creature_template SET ScriptName='boss_moam' WHERE entry=15340;
UPDATE creature_template SET ScriptName='boss_ossirian' WHERE entry=15339;
UPDATE gameobject_template SET ScriptName='go_ossirian_crystal' WHERE entry=180619;
UPDATE creature_template SET ScriptName='npc_hive_zara_larva' WHERE entry=15555;
UPDATE creature_template SET ScriptName='boss_buru' WHERE entry=15370;
UPDATE creature_template SET ScriptName='npc_buru_egg' WHERE entry=15514;
UPDATE creature_template SET ScriptName='npc_general_andorov' WHERE entry=15471;
UPDATE creature_template SET ScriptName='npc_kaldorei_elite' WHERE entry=15473;

/* SCARLET MONASTERY */
UPDATE instance_template SET ScriptName='instance_scarlet_monastery' WHERE map=189;
UPDATE creature_template SET ScriptName='boss_arcanist_doan' WHERE entry=6487;
UPDATE creature_template SET ScriptName='boss_herod' WHERE entry=3975;
UPDATE creature_template SET ScriptName='boss_high_inquisitor_whitemane' WHERE entry=3977;
UPDATE creature_template SET ScriptName='boss_scarlet_commander_mograine' WHERE entry=3976;
UPDATE creature_template SET ScriptName='mob_scarlet_trainee' WHERE entry=6575;
UPDATE creature_template SET ScriptName='boss_headless_horseman' WHERE entry=23682;
UPDATE creature_template SET ScriptName='boss_head_of_horseman' WHERE entry=23775;

/* SCHOLOMANCE */
UPDATE instance_template SET ScriptName='instance_scholomance' WHERE map=289;
UPDATE creature_template SET ScriptName='boss_darkmaster_gandling' WHERE entry=1853;
UPDATE creature_template SET ScriptName='boss_jandice_barov' WHERE entry=10503;
DELETE FROM scripted_event_id WHERE id IN (5618, 5619, 5620, 5621, 5622, 5623);
INSERT INTO scripted_event_id VALUES
(5618,'event_spell_gandling_shadow_portal'),
(5619,'event_spell_gandling_shadow_portal'),
(5620,'event_spell_gandling_shadow_portal'),
(5621,'event_spell_gandling_shadow_portal'),
(5622,'event_spell_gandling_shadow_portal'),
(5623,'event_spell_gandling_shadow_portal');

/* SEARING GORGE */
UPDATE creature_template SET ScriptName='npc_dorius_stonetender' WHERE entry=8284;

/* SHADOWFANG KEEP */
UPDATE instance_template SET ScriptName='instance_shadowfang_keep' WHERE map=33;
UPDATE creature_template SET ScriptName='npc_shadowfang_prisoner' WHERE entry IN (3849,3850);
UPDATE creature_template SET ScriptName='npc_arugal' WHERE entry=10000;
UPDATE creature_template SET ScriptName='npc_deathstalker_vincent' WHERE entry=4444;
UPDATE creature_template SET ScriptName='mob_arugal_voidwalker' WHERE entry=4627;
UPDATE creature_template SET ScriptName='boss_arugal' WHERE entry=4275;
UPDATE creature_template SET ScriptName='npc_apothecary_hummel' WHERE entry=36296;
UPDATE creature_template SET ScriptName='npc_valentine_boss_manager' WHERE entry=36643;

/* SHADOWMOON VALLEY */
UPDATE creature_template SET ScriptName='boss_doomwalker' WHERE entry=17711;
UPDATE creature_template SET ScriptName='npc_dragonmaw_peon' WHERE entry=22252;
UPDATE creature_template SET ScriptName='mob_mature_netherwing_drake' WHERE entry=21648;
UPDATE creature_template SET ScriptName='mob_enslaved_netherwing_drake' WHERE entry=21722;
UPDATE creature_template SET ScriptName='npc_wilda' WHERE entry=21027;
UPDATE creature_template SET ScriptName='mob_torloth' WHERE entry=22076;
UPDATE creature_template SET ScriptName='npc_totem_of_spirits' WHERE entry=21071;
DELETE FROM scripted_event_id WHERE id IN (13513,13514,13515,13516);
INSERT INTO scripted_event_id VALUES
(13513,'event_spell_soul_captured_credit'),
(13514,'event_spell_soul_captured_credit'),
(13515,'event_spell_soul_captured_credit'),
(13516,'event_spell_soul_captured_credit');
UPDATE creature_template SET ScriptName='npc_lord_illidan_stormrage' WHERE entry=22083;
UPDATE gameobject_template SET ScriptName='go_crystal_prison' WHERE entry=185126;
UPDATE creature_template SET ScriptName='npc_spawned_oronok_tornheart' WHERE entry=21685;
UPDATE creature_template SET ScriptName='npc_domesticated_felboar' WHERE entry=21195;
UPDATE creature_template SET ScriptName='npc_shadowmoon_tuber_node' WHERE entry=21347;
UPDATE creature_template SET ScriptName='npc_veneratus_spawn_node' WHERE entry=21334;

/* SHATTRATH */
UPDATE creature_template SET ScriptName='npc_dirty_larry' WHERE entry=19720;
UPDATE creature_template SET ScriptName='npc_khadgars_servant' WHERE entry=19685;
UPDATE creature_template SET ScriptName='npc_salsalabim' WHERE entry=18584;

/* SHOLAZAR BASIN */
UPDATE creature_template SET ScriptName='npc_helice' WHERE entry=28787;
UPDATE creature_template SET ScriptName='npc_injured_rainspeaker' WHERE entry=28217;
UPDATE creature_template SET ScriptName='npc_mosswalker_victim' WHERE entry=28113;
UPDATE creature_template SET ScriptName='npc_tipsy_mcmanus' WHERE entry=28566;
UPDATE creature_template SET ScriptName='npc_wants_fruit_credit' WHERE entry IN (28535,28536,28537);
UPDATE gameobject_template SET ScriptName='go_quest_still_at_it_credit' WHERE entry IN (190635,190636);

/* SILITHUS */
UPDATE creature_template SET ScriptName='npc_anachronos_the_ancient' WHERE entry=15381;
UPDATE gameobject_template SET ScriptName='go_crystalline_tear' WHERE entry=180633;

/* SILVERMOON */

/* SILVERPINE FOREST */
UPDATE creature_template SET ScriptName='npc_deathstalker_erland' WHERE entry=1978;
UPDATE creature_template SET ScriptName='npc_deathstalker_faerleia' WHERE entry=2058;

/* STOCKADES */

/* STONETALON MOUNTAINS */
UPDATE creature_template SET ScriptName='npc_kaya' WHERE entry=11856;

/* STORM PEAKS */
UPDATE creature_template SET ScriptName='npc_floating_spirit' WHERE entry IN (30141,30143,30145);
UPDATE creature_template SET ScriptName='npc_restless_frostborn' WHERE entry IN (29974,30135,30144);
UPDATE creature_template SET ScriptName='npc_injured_miner' WHERE entry=29434;

/* STORMWIND CITY */
UPDATE creature_template SET ScriptName='npc_bartleby' WHERE entry=6090;
UPDATE creature_template SET ScriptName='npc_dashel_stonefist' WHERE entry=4961;
UPDATE creature_template SET ScriptName='npc_lady_katrana_prestor' WHERE entry=1749;
UPDATE creature_template SET ScriptName='npc_squire_rowe' WHERE entry=17804;
UPDATE creature_template SET ScriptName='npc_reginald_windsor' WHERE entry =12580;

/* STRANGLETHORN VALE */
UPDATE creature_template SET ScriptName='mob_yenniku' WHERE entry=2530;

/* STRATHOLME */
UPDATE instance_template SET ScriptName='instance_stratholme' WHERE map=329;
UPDATE creature_template SET ScriptName='boss_dathrohan_balnazzar' WHERE entry=10812;
UPDATE creature_template SET ScriptName='boss_maleki_the_pallid' WHERE entry=10438;
UPDATE creature_template SET ScriptName='boss_cannon_master_willey' WHERE entry=10997;
UPDATE creature_template SET ScriptName='boss_baroness_anastari' WHERE entry=10436;
UPDATE creature_template SET ScriptName='boss_silver_hand_bosses' WHERE entry IN (17910,17911,17912,17913,17914);
UPDATE creature_template SET ScriptName='mobs_spectral_ghostly_citizen' WHERE entry IN (10384,10385);
UPDATE creature_template SET ScriptName='mob_restless_soul' WHERE entry=11122;
UPDATE gameobject_template SET ScriptName='go_gauntlet_gate' WHERE entry=175357;
UPDATE gameobject_template SET ScriptName='go_service_gate' WHERE entry=175368;
UPDATE gameobject_template SET ScriptName='go_stratholme_postbox' WHERE entry IN (176346,176349,176350,176351,176352,176353);

/* SUNKEN TEMPLE */
UPDATE instance_template SET ScriptName='instance_sunken_temple' WHERE map=109;
DELETE FROM scripted_areatrigger WHERE entry=4016;
INSERT INTO scripted_areatrigger VALUES (4016,'at_shade_of_eranikus');
UPDATE creature_template SET ScriptName='npc_malfurion_stormrage' WHERE entry=15362;
DELETE FROM scripted_event_id WHERE id IN (3094,3095,3097,3098,3099,3100);
INSERT INTO scripted_event_id VALUES
(3094,'event_antalarion_statue_activation'),
(3095,'event_antalarion_statue_activation'),
(3097,'event_antalarion_statue_activation'),
(3098,'event_antalarion_statue_activation'),
(3099,'event_antalarion_statue_activation'),
(3100,'event_antalarion_statue_activation');
UPDATE creature_template SET ScriptName='npc_shade_of_hakkar' WHERE entry=8440;
UPDATE gameobject_template SET ScriptName='go_eternal_flame' WHERE entry IN (148418,148419,148420,148421);
DELETE FROM scripted_event_id WHERE id=8502;
INSERT INTO scripted_event_id VALUES
(8502,'event_avatar_of_hakkar');

/* SUNWELL PLATEAU */
UPDATE instance_template SET ScriptName='instance_sunwell_plateau' WHERE map=580;
UPDATE creature_template SET ScriptName='boss_brutallus' WHERE entry=24882;
UPDATE creature_template SET ScriptName='boss_kalecgos' WHERE entry=24850;
UPDATE creature_template SET ScriptName='boss_kalecgos_humanoid' WHERE entry=24891;
UPDATE creature_template SET ScriptName='boss_sathrovarr' WHERE entry=24892;
DELETE FROM scripted_areatrigger WHERE entry=4853;
INSERT INTO scripted_areatrigger VALUES (4853,'at_madrigosa');
UPDATE creature_template SET ScriptName='boss_alythess' WHERE entry=25166;
UPDATE creature_template SET ScriptName='boss_sacrolash' WHERE entry=25165;
UPDATE creature_template SET ScriptName='npc_shadow_image' WHERE entry=25214;
UPDATE creature_template SET ScriptName='boss_muru' WHERE entry=25741;
UPDATE creature_template SET ScriptName='boss_entropius' WHERE entry=25840;
UPDATE creature_template SET ScriptName='npc_portal_target' WHERE entry=25770;
UPDATE creature_template SET ScriptName='boss_kiljaeden' WHERE entry=25315;
UPDATE creature_template SET ScriptName='npc_kiljaeden_controller' WHERE entry=25608;
UPDATE creature_template SET ScriptName='spell_dummy_npc_brutallus_cloud' WHERE entry=25703;
UPDATE creature_template SET ScriptName='boss_felmyst' WHERE entry=25038;
UPDATE creature_template SET ScriptName='npc_shield_orb' WHERE entry=25502;
UPDATE creature_template SET ScriptName='npc_power_blue_flight' WHERE entry=25653;
UPDATE creature_template SET ScriptName='npc_demonic_vapor' WHERE entry=25265;
UPDATE creature_template SET ScriptName='npc_darkness' WHERE entry=25879;
UPDATE creature_template SET ScriptName='npc_singularity' WHERE entry=25855;

/* SWAMP OF SORROWS */
UPDATE creature_template SET ScriptName='npc_galen_goodward' WHERE entry=5391;

/* TANARIS */
UPDATE creature_template SET ScriptName='mob_aquementas' WHERE entry=9453;
UPDATE creature_template SET ScriptName='npc_custodian_of_time' WHERE entry=20129;
UPDATE creature_template SET ScriptName='npc_oox17tn' WHERE entry=7784;
UPDATE creature_template SET ScriptName='npc_stone_watcher_of_norgannon' WHERE entry=7918;
UPDATE creature_template SET ScriptName='npc_tooga' WHERE entry=5955;

/* TELDRASSIL */
UPDATE creature_template SET ScriptName='npc_mist' WHERE entry=3568;

/*  */
/* TEMPEST KEEP */
/*  */

/* THE MECHANAR */
UPDATE creature_template SET ScriptName='boss_nethermancer_sepethrea' WHERE entry=19221;
UPDATE creature_template SET ScriptName='boss_pathaleon_the_calculator' WHERE entry=19220;
UPDATE creature_template SET ScriptName='mob_nether_wraith' WHERE entry=21062;
UPDATE instance_template SET ScriptName='instance_mechanar' WHERE map=554;

/* THE BOTANICA */
UPDATE creature_template SET ScriptName='boss_high_botanist_freywinn' WHERE entry=17975;
UPDATE creature_template SET ScriptName='boss_laj' WHERE entry=17980;
UPDATE creature_template SET ScriptName='boss_warp_splinter' WHERE entry=17977;
UPDATE creature_template SET ScriptName='mob_warp_splinter_treant' WHERE entry=19949;

/* THE ARCATRAZ */
UPDATE instance_template SET ScriptName='instance_arcatraz' WHERE map=552;
UPDATE creature_template SET ScriptName='boss_harbinger_skyriss' WHERE entry=20912;
UPDATE creature_template SET ScriptName='boss_dalliah' WHERE entry=20885;
UPDATE creature_template SET ScriptName='boss_soccothrates' WHERE entry=20886;
UPDATE creature_template SET ScriptName='npc_warden_mellichar' WHERE entry=20904;
UPDATE creature_template SET ScriptName='npc_millhouse_manastorm' WHERE entry=20977;

/* THE EYE */
UPDATE instance_template SET ScriptName='instance_the_eye' WHERE map=550;
/* Al'ar event */
UPDATE creature_template SET ScriptName='boss_alar' WHERE entry=19514;
/* Void Reaver event */
UPDATE creature_template SET ScriptName='boss_void_reaver' WHERE entry=19516;
/* Astromancer event */
UPDATE creature_template SET ScriptName='boss_high_astromancer_solarian' WHERE entry=18805;
UPDATE creature_template SET ScriptName='mob_solarium_priest' WHERE entry=18806;
/* Kael'thas event */
UPDATE creature_template SET ScriptName='boss_kaelthas' WHERE entry=19622;
UPDATE creature_template SET ScriptName='boss_thaladred_the_darkener' WHERE entry=20064;
UPDATE creature_template SET ScriptName='boss_lord_sanguinar' WHERE entry=20060;
UPDATE creature_template SET ScriptName='boss_grand_astromancer_capernian' WHERE entry=20062;
UPDATE creature_template SET ScriptName='boss_master_engineer_telonicus' WHERE entry=20063;
UPDATE creature_template SET ScriptName='mob_phoenix_tk' WHERE entry=21362;
UPDATE creature_template SET ScriptName='mob_phoenix_egg_tk' WHERE entry=21364;

/* TEMPLE OF AHN'QIRAJ */
UPDATE instance_template SET ScriptName='instance_temple_of_ahnqiraj' WHERE map=531;
UPDATE creature_template SET ScriptName='boss_cthun' WHERE entry=15727;
UPDATE creature_template SET ScriptName='boss_skeram' WHERE entry=15263;
UPDATE creature_template SET ScriptName='boss_vem' WHERE entry=15544;
UPDATE creature_template SET ScriptName='boss_yauj' WHERE entry=15543;
UPDATE creature_template SET ScriptName='boss_kri' WHERE entry=15511;
UPDATE creature_template SET ScriptName='boss_sartura' WHERE entry=15516;
UPDATE creature_template SET ScriptName='boss_fankriss' WHERE entry=15510;
UPDATE creature_template SET ScriptName='boss_viscidus' WHERE entry=15299;
UPDATE creature_template SET ScriptName='npc_glob_of_viscidus' WHERE entry=15667;
UPDATE creature_template SET ScriptName='boss_huhuran' WHERE entry=15509;
UPDATE creature_template SET ScriptName='boss_veklor' WHERE entry=15276;
UPDATE creature_template SET ScriptName='boss_veknilash' WHERE entry=15275;
UPDATE creature_template SET ScriptName='boss_ouro' WHERE entry=15517;
UPDATE creature_template SET ScriptName='npc_ouro_spawner' WHERE entry=15957;
UPDATE creature_template SET ScriptName='boss_eye_of_cthun' WHERE entry=15589;
UPDATE creature_template SET ScriptName='mob_sartura_royal_guard' WHERE entry=15984;
UPDATE creature_template SET ScriptName='mob_giant_claw_tentacle' WHERE entry=15728;
UPDATE creature_template SET ScriptName='mob_anubisath_sentinel' WHERE entry=15264;
DELETE FROM scripted_areatrigger WHERE entry IN (4033,4034);
INSERT INTO scripted_areatrigger VALUES
(4033,'at_stomach_cthun'),
(4034,'at_stomach_cthun');

/* TEROKKAR FOREST */
UPDATE creature_template SET ScriptName='mob_netherweb_victim' WHERE entry=22355;
UPDATE creature_template SET ScriptName='mob_unkor_the_ruthless' WHERE entry=18262;
UPDATE creature_template SET ScriptName='npc_akuno' WHERE entry=22377;
UPDATE creature_template SET ScriptName='npc_hungry_nether_ray' WHERE entry=23439;
UPDATE creature_template SET ScriptName='npc_letoll' WHERE entry=22458;
UPDATE creature_template SET ScriptName='npc_mana_bomb_exp_trigger' WHERE entry=20767;
UPDATE gameobject_template SET ScriptName='go_mana_bomb' WHERE entry=184725;
UPDATE creature_template SET ScriptName='npc_captive_child' WHERE entry=22314;
UPDATE creature_template SET ScriptName='npc_isla_starmane' WHERE entry=18760;
UPDATE creature_template SET ScriptName="npc_skywing" WHERE entry=22424;
UPDATE creature_template SET ScriptName="npc_cenarion_sparrowhawk" WHERE entry=22972;
UPDATE creature_template SET ScriptName="npc_skyguard_prisoner" WHERE entry=23383;

/* THOUSAND NEEDLES */
UPDATE creature_template SET ScriptName='npc_kanati' WHERE entry=10638;
UPDATE creature_template SET ScriptName='npc_plucky_johnson' WHERE entry=6626;
UPDATE creature_template SET ScriptName='npc_paoka_swiftmountain' WHERE entry=10427;
UPDATE creature_template SET ScriptName='npc_lakota_windsong' WHERE entry=10646;

/* THUNDER BLUFF */

/* TIRISFAL GLADES */
UPDATE gameobject_template SET ScriptName='go_mausoleum_trigger' WHERE entry=104593;
UPDATE gameobject_template SET ScriptName='go_mausoleum_door' WHERE entry=176594;
UPDATE creature_template SET ScriptName='npc_calvin_montague' WHERE entry=6784;

/* ULDAMAN */
DELETE FROM scripted_event_id WHERE id IN (2228,2268);
INSERT INTO scripted_event_id VALUES
(2228,'event_spell_altar_boss_aggro'),
(2268,'event_spell_altar_boss_aggro');
UPDATE creature_template SET ScriptName='boss_archaedas' WHERE entry=2748;
UPDATE creature_template SET ScriptName='mob_archaeras_add' WHERE entry IN (7309,7076,7077,10120);
UPDATE instance_template SET ScriptName='instance_uldaman' WHERE map=70;

/*  */
/* ULDUAR */
/*  */

/* HALLS OF LIGHTNING */
UPDATE instance_template SET ScriptName='instance_halls_of_lightning' WHERE map=602;
UPDATE creature_template SET ScriptName='boss_bjarngrim' WHERE entry=28586;
UPDATE creature_template SET ScriptName='mob_stormforged_lieutenant' WHERE entry=29240;
UPDATE creature_template SET ScriptName='boss_volkhan' WHERE entry=28587;
UPDATE creature_template SET ScriptName='mob_molten_golem' WHERE entry=28695;
UPDATE creature_template SET ScriptName='npc_volkhan_anvil' WHERE entry=28823;
UPDATE creature_template SET ScriptName='boss_ionar' WHERE entry=28546;
UPDATE creature_template SET ScriptName='mob_spark_of_ionar' WHERE entry=28926;
UPDATE creature_template SET ScriptName='boss_loken' WHERE entry=28923;

/* HALLS OF STONE */
UPDATE instance_template SET ScriptName='instance_halls_of_stone' WHERE map=599;
UPDATE creature_template SET ScriptName='boss_maiden_of_grief' WHERE entry=27975;
UPDATE creature_template SET ScriptName='boss_sjonnir' WHERE entry=27978;
UPDATE creature_template SET ScriptName='npc_brann_hos' WHERE entry=28070;
UPDATE creature_template SET ScriptName='npc_dark_matter' WHERE entry=28235;
UPDATE creature_template SET ScriptName='npc_searing_gaze' WHERE entry=28265;

/* ULDUAR */
UPDATE instance_template SET ScriptName='instance_ulduar' WHERE map=603;
UPDATE gameobject_template SET ScriptName='go_ulduar_teleporter' WHERE entry=194569;
UPDATE creature_template SET ScriptName='boss_general_vezax' WHERE entry=33271;
UPDATE creature_template SET ScriptName='boss_auriaya' WHERE entry=33515;
UPDATE creature_template SET ScriptName='boss_feral_defender' WHERE entry=34035;
UPDATE creature_template SET ScriptName='boss_brundir' WHERE entry=32857;
UPDATE creature_template SET ScriptName='boss_molgeim' WHERE entry=32927;
UPDATE creature_template SET ScriptName='boss_steelbreaker' WHERE entry=32867;
UPDATE creature_template SET ScriptName='boss_ignis' WHERE entry=33118;
UPDATE creature_template SET ScriptName='npc_iron_construct' WHERE entry=33121;
UPDATE creature_template SET ScriptName='npc_scorch' WHERE entry=33221;
UPDATE creature_template SET ScriptName='boss_xt_002' WHERE entry=33293;
UPDATE creature_template SET ScriptName='boss_heart_deconstructor' WHERE entry=33329;
UPDATE creature_template SET ScriptName='npc_scrapbot' WHERE entry=33343;
UPDATE creature_template SET ScriptName='npc_xt_toy_pile' WHERE entry=33337;
UPDATE creature_template SET ScriptName='boss_razorscale' WHERE entry=33186;
UPDATE creature_template SET ScriptName='npc_expedition_commander' WHERE entry=33210;
UPDATE creature_template SET ScriptName='npc_razorscale_spawner' WHERE entry=33245;
UPDATE creature_template SET ScriptName='npc_harpoon_fire_state' WHERE entry=33282;
UPDATE creature_template SET ScriptName='npc_keeper_norgannon' WHERE entry=33686;
UPDATE creature_template SET ScriptName='npc_brann_ulduar' WHERE entry=33579;
UPDATE creature_template SET ScriptName='boss_flame_leviathan' WHERE entry=33113;
UPDATE creature_template SET ScriptName='npc_hodir_fury_reticle' WHERE entry=33108;
UPDATE creature_template SET ScriptName='npc_hodir_fury' WHERE entry=33212;
UPDATE creature_template SET ScriptName='npc_freya_ward' WHERE entry=33367;
UPDATE creature_template SET ScriptName='npc_mimiron_inferno' WHERE entry=33370;
UPDATE creature_template SET ScriptName='boss_kologarn' WHERE entry=32930;
UPDATE creature_template SET ScriptName='npc_focused_eyebeam' WHERE entry IN (33802,33632);
UPDATE creature_template SET ScriptName='npc_rubble_stalker' WHERE entry=33809;
UPDATE creature_template SET ScriptName='npc_storm_tempered_keeper' WHERE entry IN (33699,33722);
UPDATE creature_template SET ScriptName='npc_charged_sphere' WHERE entry=33715;
UPDATE creature_template SET ScriptName='boss_algalon' WHERE entry=32871;
UPDATE creature_template SET ScriptName='npc_living_constellation' WHERE entry=33052;
UPDATE creature_template SET ScriptName='npc_worm_hole' WHERE entry=34099;
UPDATE creature_template SET ScriptName='npc_black_hole' WHERE entry=32953;
UPDATE creature_template SET ScriptName='npc_collapsing_star' WHERE entry=32955;
UPDATE gameobject_template SET ScriptName='go_celestial_access' WHERE entry IN (194628,194752);
UPDATE creature_template SET ScriptName='boss_hodir' WHERE entry=32845;
UPDATE creature_template SET ScriptName='npc_flash_freeze' WHERE entry IN (32926,32938);
UPDATE creature_template SET ScriptName='npc_icicle_target' WHERE entry=33174;
UPDATE creature_template SET ScriptName='boss_thorim' WHERE entry=32865;
UPDATE creature_template SET ScriptName='boss_sif' WHERE entry=33196;
UPDATE creature_template SET ScriptName='npc_thunder_orb' WHERE entry=33378;
UPDATE creature_template SET ScriptName='npc_runic_colossus' WHERE entry=32872;
UPDATE creature_template SET ScriptName='boss_freya' WHERE entry=32906;
UPDATE creature_template SET ScriptName='npc_eonars_gift' WHERE entry=33228;
UPDATE creature_template SET ScriptName='npc_nature_bomb' WHERE entry=34129;
UPDATE creature_template SET ScriptName='npc_iron_roots' WHERE entry IN (33088,33168);
UPDATE creature_template SET ScriptName='npc_healthy_spore' WHERE entry=33215;
UPDATE creature_template SET ScriptName='npc_water_spirit' WHERE entry=33202;
UPDATE creature_template SET ScriptName='npc_snaplasher' WHERE entry=32916;
UPDATE creature_template SET ScriptName='npc_storm_lasher' WHERE entry=32919;
UPDATE creature_template SET ScriptName='boss_mimiron' WHERE entry=33350;
UPDATE creature_template SET ScriptName='boss_leviathan_mk2' WHERE entry=33432;
UPDATE creature_template SET ScriptName='boss_vx001' WHERE entry=33651;
UPDATE creature_template SET ScriptName='boss_aerial_unit' WHERE entry=33670;
UPDATE creature_template SET ScriptName='npc_proximity_mine' WHERE entry=34362;
UPDATE creature_template SET ScriptName='npc_bot_trigger' WHERE entry=33856;
UPDATE creature_template SET ScriptName='npc_rocket_strike' WHERE entry=34047;
UPDATE creature_template SET ScriptName='npc_frost_bomb' WHERE entry=34149;
UPDATE creature_template SET ScriptName='npc_mimiron_flames' WHERE entry IN (34363,34121);
UPDATE creature_template SET ScriptName='boss_leviathan_mk2_turret' WHERE entry=34071;
UPDATE creature_template SET ScriptName='npc_computer' WHERE entry=34143;
UPDATE gameobject_template SET ScriptName='go_big_red_button' WHERE entry=194739;
UPDATE creature_template SET ScriptName='npc_ulduar_keeper' WHERE entry IN (33241,33242,33244,33213);
UPDATE creature_template SET ScriptName='boss_sara' WHERE entry=33134;
UPDATE creature_template SET ScriptName='boss_yogg_saron' WHERE entry=33288;
UPDATE creature_template SET ScriptName='npc_ominous_cloud' WHERE entry=33292;
UPDATE creature_template SET ScriptName='npc_death_ray' WHERE entry=33881;
UPDATE creature_template SET ScriptName='npc_voice_yogg_saron' WHERE entry=33280;
UPDATE creature_template SET ScriptName='npc_brain_yogg_saron' WHERE entry=33890;
UPDATE creature_template SET ScriptName='npc_guardian_of_yogg' WHERE entry=33136;
UPDATE creature_template SET ScriptName='npc_immortal_guardian' WHERE entry=33988;
UPDATE creature_template SET ScriptName='npc_constrictor_tentacle' WHERE entry=33983;
UPDATE creature_template SET ScriptName='npc_descent_madness' WHERE entry=34072;
UPDATE creature_template SET ScriptName='npc_laughing_skull' WHERE entry=33990;
UPDATE creature_template SET ScriptName='npc_keeper_mimiron' WHERE entry=33412;
UPDATE creature_template SET ScriptName='npc_keeper_thorim' WHERE entry=33413;
DELETE FROM scripted_event_id WHERE id IN (9735,20907,20964,21030,21031,21032,21033,21045,21605,21606,21620);
INSERT INTO scripted_event_id VALUES
(9735, 'event_spell_saronite_barrier'), -- Vezax saronite barrier event
(20907,'event_boss_hodir'),             -- Hodir shatter chest event
(20964,'event_spell_harpoon_shot'),     -- Razorscale harpoon event
(21030,'event_go_ulduar_tower'),        -- Tower of Life destroyed event
(21031,'event_go_ulduar_tower'),        -- Tower of Storms destroyed event
(21032,'event_go_ulduar_tower'),        -- Tower of Frost destroyed event
(21033,'event_go_ulduar_tower'),        -- Tower of Flame destroyed event
(21045,'event_boss_hodir'),             -- Hodir attack start event
(21605,'event_ulduar'),                 -- Flame Leviathan shutdown event
(21606,'event_ulduar'),                 -- XT-002 Scrap repair event
(21620,'event_ulduar');                 -- Ignis construct shatter event

/* UN'GORO CRATER */
UPDATE creature_template SET ScriptName='npc_ame01' WHERE entry=9623;
UPDATE creature_template SET ScriptName='npc_ringo' WHERE entry=9999;

/* UNDERCITY */
UPDATE creature_template SET ScriptName='npc_lady_sylvanas_windrunner' WHERE entry=10181;

/*  */
/* UTGARDE KEEP */
/*  */

/* UTGARDE KEEP */
UPDATE instance_template SET ScriptName='instance_utgarde_keep' WHERE map=574;
UPDATE creature_template SET ScriptName='mob_dragonflayer_forge_master' WHERE entry=24079;
UPDATE creature_template SET ScriptName='boss_skarvald' WHERE entry=24200;
UPDATE creature_template SET ScriptName='boss_dalronn' WHERE entry=24201;
UPDATE creature_template SET ScriptName='boss_ingvar' WHERE entry=23954;
UPDATE creature_template SET ScriptName='npc_annhylde' WHERE entry=24068;
UPDATE creature_template SET ScriptName='boss_keleseth' WHERE entry=23953;
UPDATE creature_template SET ScriptName='mob_vrykul_skeleton' WHERE entry=23970;

/* UTGARDE PINNACLE */
UPDATE creature_template SET ScriptName='boss_gortok' WHERE entry=26687;
UPDATE creature_template SET ScriptName='npc_gortok_subboss' WHERE entry IN (26683,26684,26685,26686);
UPDATE creature_template SET ScriptName='boss_skadi' WHERE entry=26693;
UPDATE creature_template SET ScriptName='npc_grauf' WHERE entry=26893;
UPDATE creature_template SET ScriptName='npc_flame_breath_trigger' WHERE entry=28351;
UPDATE creature_template SET ScriptName='boss_svala' WHERE entry=29281;
UPDATE creature_template SET ScriptName='boss_ymiron' WHERE entry=26861;
UPDATE instance_template SET ScriptName='instance_pinnacle' WHERE map=575;
DELETE FROM scripted_areatrigger WHERE entry IN (4991,5140);
INSERT INTO scripted_areatrigger VALUES
(4991,'at_skadi'),
(5140,'at_svala_intro');
DELETE FROM scripted_event_id WHERE id IN (17728,20651);
INSERT INTO scripted_event_id VALUES
(17728,'event_spell_gortok_event'),
(20651,'event_achiev_kings_bane');

/* VAULT OF ARCHAVON */

/* VIOLET HOLD */
UPDATE instance_template SET ScriptName='instance_violet_hold' WHERE map=608;
UPDATE gameobject_template SET ScriptName='go_activation_crystal' WHERE entry=193611;
UPDATE creature_template SET ScriptName='npc_door_seal' WHERE entry=30896;
UPDATE creature_template SET ScriptName='npc_sinclari' WHERE entry=30658;
UPDATE creature_template SET ScriptName='npc_prison_event_controller' WHERE entry=30883;
UPDATE creature_template SET ScriptName='npc_teleportation_portal' WHERE entry IN (31011,30679,32174);
UPDATE creature_template SET ScriptName='boss_ichoron' WHERE entry IN (29313,32234);
UPDATE creature_template SET ScriptName='boss_erekem' WHERE entry IN (29315,32226);
UPDATE creature_template SET ScriptName='npc_erekem_guard' WHERE entry IN (29395,32228);

/* WAILING CAVERNS */
UPDATE instance_template SET ScriptName='instance_wailing_caverns' WHERE map=43;
UPDATE creature_template SET ScriptName='npc_disciple_of_naralex' WHERE entry=3678;

/* WESTERN PLAGUELANDS */
UPDATE creature_template SET ScriptName='npc_the_scourge_cauldron' WHERE entry=11152;
UPDATE creature_template SET ScriptName='npc_anchorite_truuen' WHERE entry=17238;
UPDATE creature_template SET ScriptName='npc_taelan_fordring' WHERE entry=1842;
UPDATE creature_template SET ScriptName='npc_isillien' WHERE entry=1840;
UPDATE creature_template SET ScriptName='npc_tirion_fordring' WHERE entry=12126;

/* WESTFALL */
UPDATE creature_template SET ScriptName='npc_daphne_stilwell' WHERE entry=6182;
UPDATE creature_template SET ScriptName='npc_defias_traitor' WHERE entry=467;

/* WETLANDS */
UPDATE creature_template SET ScriptName='npc_tapoke_slim_jahn' WHERE entry=4962;
UPDATE creature_template SET ScriptName='npc_mikhail' WHERE entry=4963;

/* WINTERSPRING */
UPDATE creature_template SET ScriptName='npc_ranshalla' WHERE entry=10300;
UPDATE gameobject_template SET ScriptName='go_elune_fire' WHERE entry IN (177417, 177404);

/* ZANGARMARSH */
DELETE FROM scripted_event_id WHERE id=11225;
INSERT INTO scripted_event_id VALUES (11225,'event_taxi_stormcrow');
UPDATE creature_template SET ScriptName='npc_cooshcoosh' WHERE entry=18586;
UPDATE creature_template SET ScriptName='npc_kayra_longmane' WHERE entry=17969;
UPDATE creature_template SET ScriptName='npc_fhwoor' WHERE entry=17877;

/* ZUL'AMAN */
UPDATE instance_template SET ScriptName='instance_zulaman' WHERE map=568;
UPDATE creature_template SET ScriptName='npc_harrison_jones_za' WHERE entry=24358;
UPDATE gameobject_template SET ScriptName='go_strange_gong' WHERE entry=187359;
UPDATE creature_template SET ScriptName='boss_akilzon' WHERE entry=23574;
UPDATE creature_template SET ScriptName='mob_soaring_eagle' WHERE entry=24858;
UPDATE creature_template SET ScriptName='boss_halazzi' WHERE entry=23577;
UPDATE creature_template SET ScriptName='boss_spirit_lynx' WHERE entry=24143;
UPDATE creature_template SET ScriptName='boss_janalai' WHERE entry=23578;
UPDATE creature_template SET ScriptName='boss_malacrass' WHERE entry=24239;
UPDATE creature_template SET ScriptName='boss_nalorakk' WHERE entry=23576;
UPDATE creature_template SET ScriptName='boss_zuljin' WHERE entry=23863;
UPDATE creature_template SET ScriptName='npc_feather_vortex' WHERE entry=24136;
UPDATE creature_template SET ScriptName='npc_dragonhawk_egg' WHERE entry=23817;
UPDATE creature_template SET ScriptName='npc_janalai_firebomb' WHERE entry=23920;
UPDATE creature_template SET ScriptName='npc_amanishi_hatcher' WHERE entry IN (23818,24504);
UPDATE creature_template SET ScriptName='npc_forest_frog' WHERE entry=24396;

/* ZUL'DRAK */
UPDATE creature_template SET ScriptName='npc_gurgthock' WHERE entry=30007;
UPDATE creature_template SET ScriptName='npc_ghoul_feeding_bunny' WHERE entry=28591;
UPDATE creature_template SET ScriptName='npc_decaying_ghoul' WHERE entry=28565;


/* ZUL'FARRAK */
UPDATE instance_template SET ScriptName='instance_zulfarrak' WHERE map=209;
DELETE FROM scripted_event_id WHERE id IN (2488,2609);
INSERT INTO scripted_event_id VALUES
(2488,'event_go_zulfarrak_gong'),
(2609,'event_spell_unlocking');
DELETE FROM scripted_areatrigger WHERE entry=1447;
INSERT INTO scripted_areatrigger VALUES (1447,'at_zulfarrak');
UPDATE creature_template SET ScriptName='boss_zumrah' WHERE entry=7271;

/* ZUL'GURUB */
UPDATE instance_template SET ScriptName='instance_zulgurub' WHERE map=309;
UPDATE creature_template SET ScriptName='boss_jeklik' WHERE entry=14517;
UPDATE creature_template SET ScriptName='boss_venoxis' WHERE entry=14507;
UPDATE creature_template SET ScriptName='boss_marli' WHERE entry=14510;
UPDATE creature_template SET ScriptName='boss_mandokir' WHERE entry=11382;
UPDATE creature_template SET ScriptName='mob_ohgan' WHERE entry=14988;
UPDATE creature_template SET ScriptName='boss_jindo' WHERE entry=11380;
UPDATE creature_template SET ScriptName='boss_hakkar' WHERE entry=14834;
UPDATE creature_template SET ScriptName='boss_thekal' WHERE entry=14509;
UPDATE creature_template SET ScriptName='boss_arlokk' WHERE entry=14515;
UPDATE gameobject_template SET ScriptName='go_gong_of_bethekk' WHERE entry=180526;
UPDATE creature_template SET ScriptName='boss_hazzarah' WHERE entry=15083;
UPDATE creature_template SET ScriptName='boss_renataki' WHERE entry=15084;
UPDATE creature_template SET ScriptName='mob_zealot_lorkhan' WHERE entry=11347;
UPDATE creature_template SET ScriptName='mob_zealot_zath' WHERE entry=11348;
UPDATE creature_template SET ScriptName='mob_healing_ward' WHERE entry=14987;
UPDATE creature_template SET ScriptName='npc_gurubashi_bat_rider' WHERE entry=14750;

/* EOF */

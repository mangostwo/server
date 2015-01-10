/**
 * ScriptDev2 is an extension for mangos providing enhanced features for
 * area triggers, creatures, game objects, instances, items, and spells beyond
 * the default database scripting in mangos.
 *
 * Copyright (C) 2006-2013  ScriptDev2 <http://www.scriptdev2.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

#include "precompiled.h"

// eastern kingdoms
void AddSC_blackrock_depths();                       // blackrock_depths
void AddSC_boss_ambassador_flamelash();
void AddSC_boss_coren_direbrew();
void AddSC_boss_draganthaurissan();
void AddSC_boss_general_angerforge();
void AddSC_boss_high_interrogator_gerstahn();
void AddSC_instance_blackrock_depths();
void AddSC_boss_overlordwyrmthalak();                // blackrock_spire
void AddSC_boss_pyroguard_emberseer();
void AddSC_boss_gyth();
void AddSC_instance_blackrock_spire();
void AddSC_boss_razorgore();                         // blackwing_lair
void AddSC_boss_vaelastrasz();
void AddSC_boss_broodlord();
void AddSC_boss_firemaw();
void AddSC_boss_ebonroc();
void AddSC_boss_flamegor();
void AddSC_boss_chromaggus();
void AddSC_boss_nefarian();
void AddSC_boss_victor_nefarius();
void AddSC_instance_blackwing_lair();
void AddSC_boss_mr_smite();                          // deadmines
void AddSC_deadmines();
void AddSC_instance_deadmines();
void AddSC_gnomeregan();                             // gnomeregan
void AddSC_boss_thermaplugg();
void AddSC_instance_gnomeregan();
void AddSC_boss_attumen();                           // karazhan
void AddSC_boss_curator();
void AddSC_boss_maiden_of_virtue();
void AddSC_boss_shade_of_aran();
void AddSC_boss_netherspite();
void AddSC_boss_nightbane();
void AddSC_boss_prince_malchezaar();
void AddSC_boss_terestian_illhoof();
void AddSC_boss_moroes();
void AddSC_bosses_opera();
void AddSC_chess_event();
void AddSC_instance_karazhan();
void AddSC_karazhan();
void AddSC_boss_felblood_kaelthas();                 // magisters_terrace
void AddSC_boss_selin_fireheart();
void AddSC_boss_vexallus();
void AddSC_boss_priestess_delrissa();
void AddSC_instance_magisters_terrace();
void AddSC_magisters_terrace();
void AddSC_boss_lucifron();                          // molten_core
void AddSC_boss_magmadar();
void AddSC_boss_gehennas();
void AddSC_boss_garr();
void AddSC_boss_baron_geddon();
void AddSC_boss_shazzrah();
void AddSC_boss_golemagg();
void AddSC_boss_sulfuron();
void AddSC_boss_majordomo();
void AddSC_boss_ragnaros();
void AddSC_instance_molten_core();
void AddSC_molten_core();
void AddSC_ebon_hold();                              // scarlet_enclave
void AddSC_boss_arcanist_doan();                     // scarlet_monastery
void AddSC_boss_herod();
void AddSC_boss_mograine_and_whitemane();
void AddSC_boss_headless_horseman();
void AddSC_instance_scarlet_monastery();
void AddSC_boss_darkmaster_gandling();               // scholomance
void AddSC_boss_jandicebarov();
void AddSC_instance_scholomance();
void AddSC_boss_hummel();                            // shadowfang_keep
void AddSC_shadowfang_keep();
void AddSC_instance_shadowfang_keep();
void AddSC_boss_maleki_the_pallid();                 // stratholme
void AddSC_boss_cannon_master_willey();
void AddSC_boss_baroness_anastari();
void AddSC_boss_dathrohan_balnazzar();
void AddSC_boss_order_of_silver_hand();
void AddSC_instance_stratholme();
void AddSC_stratholme();
void AddSC_instance_sunken_temple();                 // sunken_temple
void AddSC_sunken_temple();
void AddSC_boss_brutallus();                         // sunwell_plateau
void AddSC_boss_eredar_twins();
void AddSC_boss_felmyst();
void AddSC_boss_kalecgos();
void AddSC_boss_kiljaeden();
void AddSC_boss_muru();
void AddSC_instance_sunwell_plateau();
void AddSC_boss_archaedas();                         // uldaman
void AddSC_instance_uldaman();
void AddSC_uldaman();
void AddSC_boss_akilzon();                           // zulaman
void AddSC_boss_halazzi();
void AddSC_boss_janalai();
void AddSC_boss_malacrass();
void AddSC_boss_nalorakk();
void AddSC_instance_zulaman();
void AddSC_zulaman();
void AddSC_boss_zuljin();
void AddSC_boss_arlokk();                            // zulgurub
void AddSC_boss_hakkar();
void AddSC_boss_hazzarah();
void AddSC_boss_jeklik();
void AddSC_boss_jindo();
void AddSC_boss_mandokir();
void AddSC_boss_marli();
void AddSC_boss_ouro();
void AddSC_boss_renataki();
void AddSC_boss_thekal();
void AddSC_boss_venoxis();
void AddSC_instance_zulgurub();

void AddSC_alterac_mountains();
void AddSC_arathi_highlands();
void AddSC_blasted_lands();
void AddSC_burning_steppes();
void AddSC_dun_morogh();
void AddSC_eastern_plaguelands();
void AddSC_elwynn_forest();
void AddSC_eversong_woods();
void AddSC_ghostlands();
void AddSC_hinterlands();
void AddSC_ironforge();
void AddSC_isle_of_queldanas();
void AddSC_loch_modan();
void AddSC_redridge_mountains();
void AddSC_searing_gorge();
void AddSC_silvermoon_city();
void AddSC_silverpine_forest();
void AddSC_stormwind_city();
void AddSC_stranglethorn_vale();
void AddSC_swamp_of_sorrows();
void AddSC_tirisfal_glades();
void AddSC_undercity();
void AddSC_western_plaguelands();
void AddSC_westfall();
void AddSC_wetlands();

void AddEasternKingdomsScripts()
{
    AddSC_blackrock_depths();                               // blackrock_depths
    AddSC_boss_ambassador_flamelash();
    AddSC_boss_coren_direbrew();
    AddSC_boss_draganthaurissan();
    AddSC_boss_general_angerforge();
    AddSC_boss_high_interrogator_gerstahn();
    AddSC_instance_blackrock_depths();
    AddSC_boss_overlordwyrmthalak();                        // blackrock_spire
    AddSC_boss_pyroguard_emberseer();
    AddSC_boss_gyth();
    AddSC_instance_blackrock_spire();
    AddSC_boss_razorgore();                                 // blackwing_lair
    AddSC_boss_vaelastrasz();
    AddSC_boss_broodlord();
    AddSC_boss_firemaw();
    AddSC_boss_ebonroc();
    AddSC_boss_flamegor();
    AddSC_boss_chromaggus();
    AddSC_boss_nefarian();
    AddSC_boss_victor_nefarius();
    AddSC_instance_blackwing_lair();
    AddSC_deadmines();                                      // deadmines
    AddSC_boss_mr_smite();
    AddSC_instance_deadmines();
    AddSC_gnomeregan();                                     // gnomeregan
    AddSC_boss_thermaplugg();
    AddSC_instance_gnomeregan();
    AddSC_boss_attumen();                                   // karazhan
    AddSC_boss_curator();
    AddSC_boss_maiden_of_virtue();
    AddSC_boss_shade_of_aran();
    AddSC_boss_netherspite();
    AddSC_boss_nightbane();
    AddSC_boss_prince_malchezaar();
    AddSC_boss_terestian_illhoof();
    AddSC_boss_moroes();
    AddSC_bosses_opera();
    AddSC_chess_event();
    AddSC_instance_karazhan();
    AddSC_karazhan();
    AddSC_boss_felblood_kaelthas();                         // magisters_terrace
    AddSC_boss_selin_fireheart();
    AddSC_boss_vexallus();
    AddSC_boss_priestess_delrissa();
    AddSC_instance_magisters_terrace();
    AddSC_magisters_terrace();
    AddSC_boss_lucifron();                                  // molten_core
    AddSC_boss_magmadar();
    AddSC_boss_gehennas();
    AddSC_boss_garr();
    AddSC_boss_baron_geddon();
    AddSC_boss_shazzrah();
    AddSC_boss_golemagg();
    AddSC_boss_sulfuron();
    AddSC_boss_majordomo();
    AddSC_boss_ragnaros();
    AddSC_instance_molten_core();
    AddSC_molten_core();
    AddSC_ebon_hold();                                      // scarlet_enclave
    AddSC_boss_arcanist_doan();                             // scarlet_monastery
    AddSC_boss_herod();
    AddSC_boss_mograine_and_whitemane();
    AddSC_boss_headless_horseman();
    AddSC_instance_scarlet_monastery();
    AddSC_boss_darkmaster_gandling();                       // scholomance
    AddSC_boss_jandicebarov();
    AddSC_instance_scholomance();
    AddSC_boss_hummel();                                    // shadowfang_keep
    AddSC_shadowfang_keep();
    AddSC_instance_shadowfang_keep();
    AddSC_boss_maleki_the_pallid();                         // stratholme
    AddSC_boss_cannon_master_willey();
    AddSC_boss_baroness_anastari();
    AddSC_boss_dathrohan_balnazzar();
    AddSC_boss_order_of_silver_hand();
    AddSC_instance_stratholme();
    AddSC_stratholme();
    AddSC_instance_sunken_temple();                         // sunken_temple
    AddSC_sunken_temple();
    AddSC_boss_brutallus();                                 // sunwell_plateau
    AddSC_boss_eredar_twins();
    AddSC_boss_felmyst();
    AddSC_boss_kalecgos();
    AddSC_boss_kiljaeden();
    AddSC_boss_muru();
    AddSC_instance_sunwell_plateau();
    AddSC_boss_archaedas();                                 // uldaman
    AddSC_instance_uldaman();
    AddSC_uldaman();
    AddSC_boss_akilzon();                                   // zulaman
    AddSC_boss_halazzi();
    AddSC_boss_janalai();
    AddSC_boss_malacrass();
    AddSC_boss_nalorakk();
    AddSC_instance_zulaman();
    AddSC_zulaman();
    AddSC_boss_zuljin();
    AddSC_boss_arlokk();                                    // zulgurub
    AddSC_boss_hakkar();
    AddSC_boss_hazzarah();
    AddSC_boss_jeklik();
    AddSC_boss_jindo();
    AddSC_boss_mandokir();
    AddSC_boss_marli();
    AddSC_boss_ouro();
    AddSC_boss_renataki();
    AddSC_boss_thekal();
    AddSC_boss_venoxis();
    AddSC_instance_zulgurub();

    AddSC_alterac_mountains();
    AddSC_arathi_highlands();
    AddSC_blasted_lands();
    AddSC_burning_steppes();
    AddSC_dun_morogh();
    AddSC_eastern_plaguelands();
    AddSC_elwynn_forest();
    AddSC_eversong_woods();
    AddSC_ghostlands();
    AddSC_hinterlands();
    AddSC_ironforge();
    AddSC_isle_of_queldanas();
    AddSC_loch_modan();
    AddSC_redridge_mountains();
    AddSC_searing_gorge();
    AddSC_silvermoon_city();
    AddSC_silverpine_forest();
    AddSC_stormwind_city();
    AddSC_stranglethorn_vale();
    AddSC_swamp_of_sorrows();
    AddSC_tirisfal_glades();
    AddSC_undercity();
    AddSC_western_plaguelands();
    AddSC_westfall();
    AddSC_wetlands();
}

// kalimdor
void AddSC_instance_blackfathom_deeps();             // blackfathom_deeps
void AddSC_boss_aeonus();                            // COT, dark_portal
void AddSC_boss_chrono_lord_deja();
void AddSC_boss_temporus();
void AddSC_dark_portal();
void AddSC_instance_dark_portal();
void AddSC_hyjal();                                  // COT, hyjal
void AddSC_boss_archimonde();
void AddSC_instance_mount_hyjal();
void AddSC_instance_old_hillsbrad();                 // COT, old_hillsbrad
void AddSC_old_hillsbrad();
void AddSC_culling_of_stratholme();                  // COT, culling_of_stratholme
void AddSC_instance_culling_of_stratholme();
void AddSC_dire_maul();                              // dire_maul
void AddSC_instance_dire_maul();
void AddSC_boss_noxxion();                           // maraudon
void AddSC_boss_onyxia();                            // onyxias_lair
void AddSC_instance_onyxias_lair();
void AddSC_razorfen_downs();                         // razorfen_downs
void AddSC_instance_razorfen_kraul();                // razorfen_kraul
void AddSC_razorfen_kraul();
void AddSC_boss_ayamiss();                           // ruins_of_ahnqiraj
void AddSC_boss_buru();
void AddSC_boss_kurinnaxx();
void AddSC_boss_ossirian();
void AddSC_boss_moam();
void AddSC_boss_rajaxx();
void AddSC_ruins_of_ahnqiraj();
void AddSC_instance_ruins_of_ahnqiraj();
void AddSC_boss_cthun();                             // temple_of_ahnqiraj
void AddSC_boss_fankriss();
void AddSC_boss_huhuran();
void AddSC_bug_trio();
void AddSC_boss_sartura();
void AddSC_boss_skeram();
void AddSC_boss_twinemperors();
void AddSC_boss_viscidus();
void AddSC_mob_anubisath_sentinel();
void AddSC_instance_temple_of_ahnqiraj();
void AddSC_instance_wailing_caverns();               // wailing_caverns
void AddSC_wailing_caverns();
void AddSC_boss_zumrah();                            // zulfarrak
void AddSC_instance_zulfarrak();
void AddSC_zulfarrak();

void AddSC_ashenvale();
void AddSC_azshara();
void AddSC_azuremyst_isle();
void AddSC_bloodmyst_isle();
void AddSC_boss_azuregos();
void AddSC_darkshore();
void AddSC_desolace();
void AddSC_durotar();
void AddSC_dustwallow_marsh();
void AddSC_felwood();
void AddSC_feralas();
void AddSC_moonglade();
void AddSC_mulgore();
void AddSC_orgrimmar();
void AddSC_silithus();
void AddSC_stonetalon_mountains();
void AddSC_tanaris();
void AddSC_teldrassil();
void AddSC_the_barrens();
void AddSC_thousand_needles();
void AddSC_thunder_bluff();
void AddSC_ungoro_crater();
void AddSC_winterspring();

void AddKalimdorScripts()
{
    AddSC_instance_blackfathom_deeps();                     // blackfathom deeps
    AddSC_boss_aeonus();                                    // CoT, dark_portal
    AddSC_boss_chrono_lord_deja();
    AddSC_boss_temporus();
    AddSC_dark_portal();
    AddSC_instance_dark_portal();
    AddSC_hyjal();                                          // CoT, hyjal
    AddSC_boss_archimonde();
    AddSC_instance_mount_hyjal();
    AddSC_instance_old_hillsbrad();                         // CoT, old_hillsbrand
    AddSC_old_hillsbrad();
    AddSC_culling_of_stratholme();                          // CoT, culling_of_stratholme
    AddSC_instance_culling_of_stratholme();
    AddSC_dire_maul();                                      // dire_maul
    AddSC_instance_dire_maul();
    AddSC_boss_noxxion();                                   // maraudon
    AddSC_boss_onyxia();                                    // onyxias_lair
    AddSC_instance_onyxias_lair();
    AddSC_razorfen_downs();                                 // razorfen_downs
    AddSC_instance_razorfen_kraul();                        // razorfen_kraul
    AddSC_razorfen_kraul();
    AddSC_boss_ayamiss();                                   // ruins_of_ahnqiraj
    AddSC_boss_buru();
    AddSC_boss_kurinnaxx();
    AddSC_boss_ossirian();
    AddSC_boss_moam();
    AddSC_boss_rajaxx();
    AddSC_ruins_of_ahnqiraj();
    AddSC_instance_ruins_of_ahnqiraj();
    AddSC_boss_cthun();                                     // temple_of_ahnqiraj
    AddSC_boss_fankriss();
    AddSC_boss_huhuran();
    AddSC_bug_trio();
    AddSC_boss_sartura();
    AddSC_boss_skeram();
    AddSC_boss_twinemperors();
    AddSC_boss_viscidus();
    AddSC_mob_anubisath_sentinel();
    AddSC_instance_temple_of_ahnqiraj();
    AddSC_instance_wailing_caverns();                       // wailing_caverns
    AddSC_wailing_caverns();
    AddSC_boss_zumrah();                                    // zulfarrak
    AddSC_zulfarrak();
    AddSC_instance_zulfarrak();

    AddSC_ashenvale();
    AddSC_azshara();
    AddSC_azuremyst_isle();
    AddSC_bloodmyst_isle();
    AddSC_boss_azuregos();
    AddSC_darkshore();
    AddSC_desolace();
    AddSC_durotar();
    AddSC_dustwallow_marsh();
    AddSC_felwood();
    AddSC_feralas();
    AddSC_moonglade();
    AddSC_mulgore();
    AddSC_orgrimmar();
    AddSC_silithus();
    AddSC_stonetalon_mountains();
    AddSC_tanaris();
    AddSC_teldrassil();
    AddSC_the_barrens();
    AddSC_thousand_needles();
    AddSC_thunder_bluff();
    AddSC_ungoro_crater();
    AddSC_winterspring();
}

// northrend
void AddSC_boss_amanitar();                          // azjol-nerub, ahnkahet
void AddSC_boss_jedoga();
void AddSC_boss_nadox();
void AddSC_boss_taldaram();
void AddSC_boss_volazj();
void AddSC_instance_ahnkahet();
void AddSC_boss_anubarak();                          // azjol-nerub, azjol-nerub
void AddSC_boss_hadronox();
void AddSC_boss_krikthir();
void AddSC_instance_azjol_nerub();
void AddSC_trial_of_the_champion();                  // CC, trial_of_the_champion
void AddSC_boss_grand_champions();
void AddSC_instance_trial_of_the_champion();
void AddSC_boss_anubarak_trial();                    // CC, trial_of_the_crusader
void AddSC_boss_faction_champions();
void AddSC_boss_jaraxxus();
void AddSC_instance_trial_of_the_crusader();
void AddSC_northrend_beasts();
void AddSC_trial_of_the_crusader();
void AddSC_twin_valkyr();
void AddSC_boss_novos();                             // draktharon_keep
void AddSC_boss_tharonja();
void AddSC_boss_trollgore();
void AddSC_instance_draktharon_keep();
void AddSC_boss_colossus();                          // gundrak
void AddSC_boss_eck();
void AddSC_boss_galdarah();
void AddSC_boss_moorabi();
void AddSC_boss_sladran();
void AddSC_instance_gundrak();
void AddSC_boss_bronjahm();                          // ICC, forge_of_souls
void AddSC_boss_devourer_of_souls();
void AddSC_instance_forge_of_souls();
void AddSC_boss_falric();                            // ICC, halls_of_reflection
void AddSC_boss_lich_king();
void AddSC_boss_marwyn();
void AddSC_halls_of_reflection();
void AddSC_instance_halls_of_reflection();
void AddSC_boss_garfrost();                          // ICC, pit_of_saron
void AddSC_boss_krick_and_ick();
void AddSC_boss_tyrannus();
void AddSC_instance_pit_of_saron();
void AddSC_pit_of_saron();
void AddSC_blood_prince_council();                   // ICC, icecrown_citadel
void AddSC_boss_blood_queen_lanathel();
void AddSC_boss_deathbringer_saurfang();
void AddSC_boss_festergut();
void AddSC_boss_lady_deathwhisper();
void AddSC_boss_lord_marrowgar();
void AddSC_boss_professor_putricide();
void AddSC_boss_rotface();
void AddSC_boss_sindragosa();
void AddSC_boss_the_lich_king();
void AddSC_boss_valithria_dreamwalker();
void AddSC_gunship_battle();
void AddSC_instance_icecrown_citadel();
void AddSC_boss_anubrekhan();                        // naxxramas
void AddSC_boss_four_horsemen();
void AddSC_boss_faerlina();
void AddSC_boss_gluth();
void AddSC_boss_gothik();
void AddSC_boss_grobbulus();
void AddSC_boss_kelthuzad();
void AddSC_boss_loatheb();
void AddSC_boss_maexxna();
void AddSC_boss_noth();
void AddSC_boss_heigan();
void AddSC_boss_patchwerk();
void AddSC_boss_razuvious();
void AddSC_boss_sapphiron();
void AddSC_boss_thaddius();
void AddSC_instance_naxxramas();
void AddSC_boss_malygos();                           // nexus, eye_of_eternity
void AddSC_instance_eye_of_eternity();
void AddSC_boss_anomalus();                          // nexus, nexus
void AddSC_boss_keristrasza();
void AddSC_boss_ormorok();
void AddSC_boss_telestra();
void AddSC_instance_nexus();
void AddSC_boss_eregos();                            // nexus, oculus
void AddSC_boss_urom();
void AddSC_boss_varos();
void AddSC_instance_oculus();
void AddSC_oculus();
void AddSC_boss_sartharion();                        // obsidian_sanctum
void AddSC_instance_obsidian_sanctum();
void AddSC_boss_baltharus();                         // ruby_sanctum
void AddSC_boss_halion();
void AddSC_boss_saviana();
void AddSC_boss_zarithrian();
void AddSC_instance_ruby_sanctum();
void AddSC_boss_bjarngrim();                         // ulduar, halls_of_lightning
void AddSC_boss_ionar();
void AddSC_boss_loken();
void AddSC_boss_volkhan();
void AddSC_instance_halls_of_lightning();
void AddSC_boss_maiden_of_grief();                   // ulduar, halls_of_stone
void AddSC_boss_sjonnir();
void AddSC_halls_of_stone();
void AddSC_instance_halls_of_stone();
void AddSC_boss_assembly_of_iron();                  // ulduar, ulduar
void AddSC_boss_algalon();
void AddSC_boss_auriaya();
void AddSC_boss_flame_leviathan();
void AddSC_boss_freya();
void AddSC_boss_general_vezax();
void AddSC_boss_hodir();
void AddSC_boss_ignis();
void AddSC_boss_kologarn();
void AddSC_boss_mimiron();
void AddSC_boss_razorscale();
void AddSC_boss_thorim();
void AddSC_boss_xt_002();
void AddSC_boss_yogg_saron();
void AddSC_instance_ulduar();
void AddSC_ulduar();
void AddSC_boss_ingvar();                            // utgarde_keep, utgarde_keep
void AddSC_boss_keleseth();
void AddSC_boss_skarvald_and_dalronn();
void AddSC_instance_utgarde_keep();
void AddSC_utgarde_keep();
void AddSC_boss_gortok();                           // utgarde_keep, utgarde_pinnacle
void AddSC_boss_skadi();
void AddSC_boss_svala();
void AddSC_boss_ymiron();
void AddSC_instance_pinnacle();
void AddSC_boss_archavon();                          // vault_of_archavon
void AddSC_boss_emalon();
void AddSC_boss_koralon();
void AddSC_boss_toravon();
void AddSC_instance_vault_of_archavon();
void AddSC_boss_erekem();                            // violet_hold
void AddSC_boss_ichoron();
void AddSC_instance_violet_hold();
void AddSC_violet_hold();

void AddSC_borean_tundra();
void AddSC_dalaran();
void AddSC_dragonblight();
void AddSC_grizzly_hills();
void AddSC_howling_fjord();
void AddSC_icecrown();
void AddSC_sholazar_basin();
void AddSC_storm_peaks();
void AddSC_zuldrak();

// outland
void AddSC_boss_exarch_maladaar();                   // auchindoun, auchenai_crypts
void AddSC_boss_shirrak();
void AddSC_boss_nexusprince_shaffar();               // auchindoun, mana_tombs
void AddSC_boss_pandemonius();
void AddSC_boss_anzu();                              // auchindoun, sethekk_halls
void AddSC_boss_darkweaver_syth();
void AddSC_boss_talon_king_ikiss();
void AddSC_instance_sethekk_halls();
void AddSC_boss_ambassador_hellmaw();                // auchindoun, shadow_labyrinth
void AddSC_boss_blackheart_the_inciter();
void AddSC_boss_grandmaster_vorpil();
void AddSC_boss_murmur();
void AddSC_instance_shadow_labyrinth();
void AddSC_black_temple();                           // black_temple
void AddSC_boss_illidan();
void AddSC_boss_shade_of_akama();
void AddSC_boss_supremus();
void AddSC_boss_gurtogg_bloodboil();
void AddSC_boss_mother_shahraz();
void AddSC_boss_reliquary_of_souls();
void AddSC_boss_teron_gorefiend();
void AddSC_boss_najentus();
void AddSC_boss_illidari_council();
void AddSC_instance_black_temple();
void AddSC_boss_fathomlord_karathress();             // CR, serpent_shrine
void AddSC_boss_hydross_the_unstable();
void AddSC_boss_lady_vashj();
void AddSC_boss_leotheras_the_blind();
void AddSC_boss_morogrim_tidewalker();
void AddSC_boss_the_lurker_below();
void AddSC_instance_serpentshrine_cavern();
void AddSC_boss_ahune();                             // CR, slave_pens
void AddSC_boss_hydromancer_thespia();               // CR, steam_vault
void AddSC_boss_mekgineer_steamrigger();
void AddSC_boss_warlord_kalithresh();
void AddSC_instance_steam_vault();
void AddSC_boss_hungarfen();                         // CR, Underbog
void AddSC_boss_gruul();                             // gruuls_lair
void AddSC_boss_high_king_maulgar();
void AddSC_instance_gruuls_lair();
void AddSC_boss_broggok();                           // HC, blood_furnace
void AddSC_boss_kelidan_the_breaker();
void AddSC_boss_the_maker();
void AddSC_instance_blood_furnace();
void AddSC_boss_nazan_and_vazruden();                // HC, hellfire_ramparts
void AddSC_boss_omor_the_unscarred();
void AddSC_boss_watchkeeper_gargolmar();
void AddSC_instance_ramparts();
void AddSC_boss_magtheridon();                       // HC, magtheridons_lair
void AddSC_instance_magtheridons_lair();
void AddSC_boss_grand_warlock_nethekurse();          // HC, shattered_halls
void AddSC_boss_warbringer_omrogg();
void AddSC_boss_warchief_kargath_bladefist();
void AddSC_instance_shattered_halls();
void AddSC_arcatraz();                               // TK, arcatraz
void AddSC_boss_dalliah();
void AddSC_boss_harbinger_skyriss();
void AddSC_boss_soccothrates();
void AddSC_instance_arcatraz();
void AddSC_boss_high_botanist_freywinn();            // TK, botanica
void AddSC_boss_laj();
void AddSC_boss_warp_splinter();
void AddSC_boss_alar();                              // TK, the_eye
void AddSC_boss_high_astromancer_solarian();
void AddSC_boss_kaelthas();
void AddSC_boss_void_reaver();
void AddSC_instance_the_eye();
void AddSC_boss_nethermancer_sepethrea();            // TK, the_mechanar
void AddSC_boss_pathaleon_the_calculator();
void AddSC_instance_mechanar();

void AddSC_blades_edge_mountains();
void AddSC_boss_doomlordkazzak();
void AddSC_boss_doomwalker();
void AddSC_hellfire_peninsula();
void AddSC_nagrand();
void AddSC_netherstorm();
void AddSC_shadowmoon_valley();
void AddSC_shattrath_city();
void AddSC_terokkar_forest();
void AddSC_zangarmarsh();

// outland
void AddOutlandsScripts()
{
    AddSC_boss_exarch_maladaar();                           // auchindoun, auchenai_crypts
    AddSC_boss_shirrak();
    AddSC_boss_nexusprince_shaffar();                       // auchindoun, mana_tombs
    AddSC_boss_pandemonius();
    AddSC_boss_anzu();                                      // auchindoun, sethekk_halls
    AddSC_boss_darkweaver_syth();
    AddSC_boss_talon_king_ikiss();
    AddSC_instance_sethekk_halls();
    AddSC_boss_ambassador_hellmaw();                        // auchindoun, shadow_labyrinth
    AddSC_boss_blackheart_the_inciter();
    AddSC_boss_grandmaster_vorpil();
    AddSC_boss_murmur();
    AddSC_instance_shadow_labyrinth();
    AddSC_black_temple();                                   // black_temple
    AddSC_boss_illidan();
    AddSC_boss_shade_of_akama();
    AddSC_boss_supremus();
    AddSC_boss_gurtogg_bloodboil();
    AddSC_boss_mother_shahraz();
    AddSC_boss_reliquary_of_souls();
    AddSC_boss_teron_gorefiend();
    AddSC_boss_najentus();
    AddSC_boss_illidari_council();
    AddSC_instance_black_temple();
    AddSC_boss_fathomlord_karathress();                     // CR, serpent_shrine
    AddSC_boss_hydross_the_unstable();
    AddSC_boss_lady_vashj();
    AddSC_boss_leotheras_the_blind();
    AddSC_boss_morogrim_tidewalker();
    AddSC_boss_the_lurker_below();
    AddSC_instance_serpentshrine_cavern();
    AddSC_boss_ahune();                                     // CR, slave_pens
    AddSC_boss_hydromancer_thespia();                       // CR, steam_vault
    AddSC_boss_mekgineer_steamrigger();
    AddSC_boss_warlord_kalithresh();
    AddSC_instance_steam_vault();
    AddSC_boss_hungarfen();                                 // CR, Underbog
    AddSC_boss_gruul();                                     // gruuls_lair
    AddSC_boss_high_king_maulgar();
    AddSC_instance_gruuls_lair();
    AddSC_boss_broggok();                                   // HC, blood_furnace
    AddSC_boss_kelidan_the_breaker();
    AddSC_boss_the_maker();
    AddSC_instance_blood_furnace();
    AddSC_boss_nazan_and_vazruden();                        // HC, hellfire_ramparts
    AddSC_boss_omor_the_unscarred();
    AddSC_boss_watchkeeper_gargolmar();
    AddSC_instance_ramparts();
    AddSC_boss_magtheridon();                               // HC, magtheridons_lair
    AddSC_instance_magtheridons_lair();
    AddSC_boss_grand_warlock_nethekurse();                  // HC, shattered_halls
    AddSC_boss_warbringer_omrogg();
    AddSC_boss_warchief_kargath_bladefist();
    AddSC_instance_shattered_halls();
    AddSC_arcatraz();                                       // TK, arcatraz
    AddSC_boss_dalliah();
    AddSC_boss_harbinger_skyriss();
    AddSC_boss_soccothrates();
    AddSC_instance_arcatraz();
    AddSC_boss_high_botanist_freywinn();                    // TK, botanica
    AddSC_boss_laj();
    AddSC_boss_warp_splinter();
    AddSC_boss_alar();                                      // TK, the_eye
    AddSC_boss_high_astromancer_solarian();
    AddSC_boss_kaelthas();
    AddSC_boss_void_reaver();
    AddSC_instance_the_eye();
    AddSC_boss_nethermancer_sepethrea();                    // TK, the_mechanar
    AddSC_boss_pathaleon_the_calculator();
    AddSC_instance_mechanar();

    AddSC_blades_edge_mountains();
    AddSC_boss_doomlordkazzak();
    AddSC_boss_doomwalker();
    AddSC_hellfire_peninsula();
    AddSC_nagrand();
    AddSC_netherstorm();
    AddSC_shadowmoon_valley();
    AddSC_shattrath_city();
    AddSC_terokkar_forest();
    AddSC_zangarmarsh();
}


void AddNorthrendScripts()
{
    // northrend
    AddSC_boss_amanitar();                                  // azjol-nerub, ahnkahet
    AddSC_boss_jedoga();
    AddSC_boss_nadox();
    AddSC_boss_taldaram();
    AddSC_boss_volazj();
    AddSC_instance_ahnkahet();
    AddSC_boss_anubarak();                                  // azjol-nerub, azjol-nerub
    AddSC_boss_hadronox();
    AddSC_boss_krikthir();
    AddSC_instance_azjol_nerub();
    AddSC_boss_grand_champions();                           // CC, trial_of_the_champion
    AddSC_instance_trial_of_the_champion();
    AddSC_trial_of_the_champion();
    AddSC_boss_anubarak_trial();                            // CC, trial_of_the_crusader
    AddSC_boss_faction_champions();
    AddSC_boss_jaraxxus();
    AddSC_instance_trial_of_the_crusader();
    AddSC_northrend_beasts();
    AddSC_trial_of_the_crusader();
    AddSC_twin_valkyr();
    AddSC_boss_novos();                                     // draktharon_keep
    AddSC_boss_tharonja();
    AddSC_boss_trollgore();
    AddSC_instance_draktharon_keep();
    AddSC_boss_colossus();                                  // gundrak
    AddSC_boss_eck();
    AddSC_boss_galdarah();
    AddSC_boss_moorabi();
    AddSC_boss_sladran();
    AddSC_instance_gundrak();
    AddSC_boss_bronjahm();                                  // ICC, FH, forge_of_souls
    AddSC_boss_devourer_of_souls();
    AddSC_instance_forge_of_souls();
    AddSC_boss_falric();                                    // ICC, FH, halls_of_reflection
    AddSC_boss_lich_king();
    AddSC_boss_marwyn();
    AddSC_halls_of_reflection();
    AddSC_instance_halls_of_reflection();
    AddSC_boss_garfrost();                                  // ICC, FH, pit_of_saron
    AddSC_boss_krick_and_ick();
    AddSC_boss_tyrannus();
    AddSC_instance_pit_of_saron();
    AddSC_pit_of_saron();
    AddSC_blood_prince_council();                           // ICC, icecrown_citadel
    AddSC_boss_blood_queen_lanathel();
    AddSC_boss_deathbringer_saurfang();
    AddSC_boss_festergut();
    AddSC_boss_lady_deathwhisper();
    AddSC_boss_lord_marrowgar();
    AddSC_boss_professor_putricide();
    AddSC_boss_rotface();
    AddSC_boss_sindragosa();
    AddSC_boss_the_lich_king();
    AddSC_boss_valithria_dreamwalker();
    AddSC_gunship_battle();
    AddSC_instance_icecrown_citadel();
    AddSC_boss_anubrekhan();                                // naxxramas
    AddSC_boss_four_horsemen();
    AddSC_boss_faerlina();
    AddSC_boss_gluth();
    AddSC_boss_gothik();
    AddSC_boss_grobbulus();
    AddSC_boss_kelthuzad();
    AddSC_boss_loatheb();
    AddSC_boss_maexxna();
    AddSC_boss_noth();
    AddSC_boss_heigan();
    AddSC_boss_patchwerk();
    AddSC_boss_razuvious();
    AddSC_boss_sapphiron();
    AddSC_boss_thaddius();
    AddSC_instance_naxxramas();
    AddSC_boss_malygos();                                   // nexus, eye_of_eternity
    AddSC_instance_eye_of_eternity();
    AddSC_boss_anomalus();                                  // nexus, nexus
    AddSC_boss_keristrasza();
    AddSC_boss_ormorok();
    AddSC_boss_telestra();
    AddSC_instance_nexus();
    AddSC_boss_eregos();                                    // nexus, oculus
    AddSC_boss_urom();
    AddSC_boss_varos();
    AddSC_instance_oculus();
    AddSC_oculus();
    AddSC_boss_sartharion();                                // obsidian_sanctum
    AddSC_instance_obsidian_sanctum();
    AddSC_boss_baltharus();                                 // ruby_sanctum
    AddSC_boss_halion();
    AddSC_boss_saviana();
    AddSC_boss_zarithrian();
    AddSC_instance_ruby_sanctum();
    AddSC_boss_bjarngrim();                                 // ulduar, halls_of_lightning
    AddSC_boss_ionar();
    AddSC_boss_loken();
    AddSC_boss_volkhan();
    AddSC_instance_halls_of_lightning();
    AddSC_boss_maiden_of_grief();                           // ulduar, halls_of_stone
    AddSC_boss_sjonnir();
    AddSC_halls_of_stone();
    AddSC_instance_halls_of_stone();
    AddSC_boss_assembly_of_iron();                          // ulduar, ulduar
    AddSC_boss_algalon();
    AddSC_boss_auriaya();
    AddSC_boss_flame_leviathan();
    AddSC_boss_freya();
    AddSC_boss_general_vezax();
    AddSC_boss_hodir();
    AddSC_boss_ignis();
    AddSC_boss_kologarn();
    AddSC_boss_mimiron();
    AddSC_boss_razorscale();
    AddSC_boss_thorim();
    AddSC_boss_xt_002();
    AddSC_boss_yogg_saron();
    AddSC_instance_ulduar();
    AddSC_ulduar();
    AddSC_boss_ingvar();                                    // UK, utgarde_keep
    AddSC_boss_keleseth();
    AddSC_boss_skarvald_and_dalronn();
    AddSC_instance_utgarde_keep();
    AddSC_utgarde_keep();
    AddSC_boss_gortok();                                    // UK, utgarde_pinnacle
    AddSC_boss_skadi();
    AddSC_boss_svala();
    AddSC_boss_ymiron();
    AddSC_instance_pinnacle();
    AddSC_boss_archavon();                                  // vault_of_archavon
    AddSC_boss_emalon();
    AddSC_boss_koralon();
    AddSC_boss_toravon();
    AddSC_instance_vault_of_archavon();
    AddSC_boss_erekem();                                    // violet_hold
    AddSC_boss_ichoron();
    AddSC_instance_violet_hold();
    AddSC_violet_hold();

    AddSC_borean_tundra();
    AddSC_dalaran();
    AddSC_dragonblight();
    AddSC_grizzly_hills();
    AddSC_howling_fjord();
    AddSC_icecrown();
    AddSC_sholazar_basin();
    AddSC_storm_peaks();
    AddSC_zuldrak();
}


// world
void AddSC_areatrigger_scripts();
void AddSC_bosses_emerald_dragons();
void AddSC_generic_creature();
void AddSC_go_scripts();
void AddSC_guards();
void AddSC_item_scripts();
void AddSC_npc_professions();
void AddSC_npcs_special();
void AddSC_spell_scripts();
void AddSC_world_map_scripts();
void AddSC_world_map_ebon_hold();

void AddWorldScripts()
{
    AddSC_areatrigger_scripts();
    AddSC_bosses_emerald_dragons();
    AddSC_generic_creature();
    AddSC_go_scripts();
    AddSC_guards();
    AddSC_item_scripts();
    AddSC_npc_professions();
    AddSC_npcs_special();
    AddSC_spell_scripts();
    AddSC_world_map_scripts();
    AddSC_world_map_ebon_hold();
}

// battlegrounds
void AddSC_battleground();

void AddBattlegroundScripts()
{
    AddSC_battleground();
}

// initialize scripts
void AddScripts()
{
    AddWorldScripts();
    AddEasternKingdomsScripts();
    AddKalimdorScripts();
    AddOutlandsScripts();
    AddNorthrendScripts();

    AddBattlegroundScripts();
}

DELETE FROM script_texts WHERE entry=-1000006;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000006,'%s becomes enraged!',0,3,0,0,'EMOTE_BOSS_GENERIC_ENRAGED');

DELETE FROM script_texts WHERE entry IN (-1544012,-1564054, -1189004);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1544012,'REUSE_ME',0,0,0,0,'REUSE_ME'),
(-1564054,'REUSE_ME',0,0,0,0,'REUSE_ME'),
(-1189004,'REUSE_ME',0,0,0,0,'REUSE_ME');

DELETE FROM script_texts WHERE entry=-1533083;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1533083,'%s resumes his attacks!',0,2,0,0,'sapphiron EMOTE_GROUND');

DELETE FROM script_texts WHERE entry=-1533021;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1533021,'%s sprays slime across the room!',0,3,0,0,'grobbulus EMOTE_SPRAY_SLIME');

DELETE FROM script_texts WHERE entry=-1533022;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1533022,'%s lifts off into the air!',0,3,0,0,'sapphiron EMOTE_FLY');

DELETE FROM script_texts WHERE entry BETWEEN -1533157 AND -1533149;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1533149,'%s loses its link!',0,3,0,0,'tesla_coil EMOTE_LOSING_LINK'),
(-1533150,'%s overloads!',0,3,0,0,'tesla_coil EMOTE_TESLA_OVERLOAD'),
(-1533151,'The polarity has shifted!',0,3,0,0,'thaddius EMOTE_POLARITY_SHIFT'),

(-1533152,'%s decimates all nearby flesh!',0,3,0,0,'gluth EMOTE_DECIMATE'),

(-1533153,'A %s joins the fight!',0,3,0,0,'crypt_guard EMOTE_CRYPT_GUARD'),
(-1533154,'%s begins to unleash an insect swarm!',0,3,0,0,'anubrekhan EMOTE_INSECT_SWARM'),
(-1533155,'Corpse Scarabs appear from a Crypt Guard\'s corpse!',0,3,0,0,'anubrekhan EMOTE_CORPSE_SCARABS'),

(-1533156,'%s casts Unyielding Pain on everyone!',0,3,0,0,'lady_blaumeux EMOTE_UNYIELDING_PAIN'),
(-1533157,'%s casts Condemation on everyone!',0,3,0,0,'sir_zeliek EMOTE_CONDEMATION');

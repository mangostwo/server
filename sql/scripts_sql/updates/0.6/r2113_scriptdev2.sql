UPDATE script_texts SET content_default='BURN! You wretches! BURN!' WHERE entry = -1469009;
UPDATE script_texts SET content_default='This cannot be!  I am the master here!  You mortals are nothing to my kind!  Do you hear me? Nothing!' WHERE entry = -1469012;
UPDATE script_texts SET content_default='Ah...the heroes. You are persistent, aren\'t you? Your ally here attempted to match his power against mine - and paid the price. Now he shall serve me...by slaughtering you.' WHERE entry = -1469006;
UPDATE script_texts SET content_default='Too late, friends! Nefarius\' corruption has taken hold...I cannot...control myself.' WHERE entry = -1469026;
UPDATE script_texts SET content_default='I beg you, mortals - FLEE! Flee before I lose all sense of control! The black fire rages within my heart! I MUST- release it!' WHERE entry = -1469027;
UPDATE script_texts SET content_default='FLAME! DEATH! DESTRUCTION! Cower, mortals before the wrath of Lord...NO - I MUST fight this! Alexstrasza help me, I MUST fight it!' WHERE entry = -1469028;
UPDATE script_texts SET content_default='Nefarius\' hate has made me stronger than ever before! You should have fled while you could, mortals! The fury of Blackrock courses through my veins!' WHERE entry = -1469029;
UPDATE script_texts SET content_default='Forgive me, $N! Your death only adds to my failure!' WHERE entry = -1469030;

DELETE FROM script_texts WHERE entry=-1469032;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1469032,'Get up, little red wyrm...and destroy them!',0,1,0,0,'victor_nefarius SAY_NEFARIUS_CORRUPT_2');

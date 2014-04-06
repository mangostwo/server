UPDATE script_texts SET content_default='Well, I\'m not sure how far I\'ll make it in this state... I\'m feeling kind of faint...', comment='ringo SAY_RIN_START_1' WHERE entry=-1000416;
UPDATE script_texts SET content_default='Remember, if I faint again, the water that Spraggle gave you will revive me.', comment='ringo SAY_RIN_START_2' WHERE entry=-1000417;
UPDATE script_texts SET content_default='The heat... I can\'t take it...', comment='ringo SAY_FAINT_1' WHERE entry=-1000418;
UPDATE script_texts SET content_default='Maybe... you could carry me?', comment='ringo SAY_FAINT_2' WHERE entry=-1000419;

DELETE FROM script_texts WHERE entry BETWEEN -1000433 AND -1000420;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000420,'Uuuuuuggggghhhhh....',0,0,0,0,'ringo SAY_FAINT_3'),
(-1000421,'I\'m not feeling so well...',0,0,0,0,'ringo SAY_FAINT_4'),
(-1000422,'Where... Where am I?',0,0,0,0,'ringo SAY_WAKE_1'),
(-1000423,'I am feeling a little better now, thank you.',0,0,0,0,'ringo SAY_WAKE_2'),
(-1000424,'Yes, I must go on.',0,0,0,0,'ringo SAY_WAKE_3'),
(-1000425,'How am I feeling? Quite soaked, thank you.',0,0,0,0,'ringo SAY_WAKE_4'),
(-1000426,'Spraggle! I didn\'t think I\'d make it back!',0,0,0,0,'ringo SAY_RIN_END_1'),
(-1000427,'Ringo! You\'re okay!',0,0,0,0,'ringo SAY_SPR_END_2'),
(-1000428,'Oh... I\'m feeling faint...',0,0,0,0,'ringo SAY_RIN_END_3'),
(-1000429,'%s collapses onto the ground.',0,2,0,0,'ringo EMOTE_RIN_END_4'),
(-1000430,'%s stands up after a short pause.',0,2,0,0,'ringo EMOTE_RIN_END_5'),
(-1000431,'Ugh.',0,0,0,0,'ringo SAY_RIN_END_6'),
(-1000432,'Ringo? Wake up! Don\'t worry, I\'ll take care of you.',0,0,0,0,'ringo SAY_SPR_END_7'),
(-1000433,'%s fades away after a long pause.',0,2,0,0,'ringo EMOTE_RIN_END_8');

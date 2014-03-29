DELETE FROM script_texts WHERE entry BETWEEN -1000004 AND -1000001;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000001,'goes into a killing frenzy!',0,2,0,0,'EMOTE_GENERIC_FRENZY_KILL'),
(-1000002,'goes into a frenzy!',0,2,0,0,'EMOTE_GENERIC_FRENZY'),
(-1000003,'becomes enraged!',0,2,0,0,'EMOTE_GENERIC_ENRAGED'),
(-1000004,'goes into a berserker rage!',0,2,0,0,'EMOTE_GENERIC_BERSERK');

DELETE FROM script_texts WHERE entry BETWEEN -1000216 AND -1000207;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000207,'You have my blessing',0,0,0,0,'ashyen_and_keleth SAY_REWARD_BLESS'),
(-1000208,'Greetings citizen',0,0,7,0,'general_marcus SAY_GREETING'),
(-1000209,'Very well. Let\'s see what you have to show me, $N.',0,0,1,0,'anvilward SAY_ANVIL1'),
(-1000210,'What manner of trick is this, $R? If you seek to ambush me, I warn you I will not go down quietly!',0,0,1,0,'anvilward SAY_ANVIL2'),
(-1000211,'Warning! Emergency shutdown process initiated by $N. Shutdown will complete in two minutes.',0,2,0,0,'manaforge_control EMOTE_START'),
(-1000212,'Emergency shutdown will complete in one minute.',0,2,0,0,'manaforge_control EMOTE_60'),
(-1000213,'Emergency shutdown will complete in thirty seconds.',0,2,0,0,'manaforge_control EMOTE_30'),
(-1000214,'Emergency shutdown will complete in ten seconds.',0,2,0,0,'manaforge_control EMOTE_10'),
(-1000215,'Emergency shutdown complete.',0,2,0,0,'manaforge_control EMOTE_COMPLETE'),
(-1000216,'Emergency shutdown aborted.',0,2,0,0,'manaforge_control EMOTE_ABORT');

DELETE FROM script_texts WHERE entry BETWEEN -1329003 AND -1329000;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1329000,'Thanks to Egan',0,0,0,0,'freed_soul SAY_ZAPPED0'),
(-1329001,'Rivendare must die',0,0,0,0,'freed_soul SAY_ZAPPED1'),
(-1329002,'Who you gonna call?',0,0,0,0,'freed_soul SAY_ZAPPED2'),
(-1329003,'Don\'t cross those beams!',0,0,0,0,'freed_soul SAY_ZAPPED3');

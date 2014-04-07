DELETE FROM script_texts WHERE entry BETWEEN -1000821 AND -1000815;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000815,'Be healed!',0,1,0,0,'Eris Havenfire SAY_PHASE_HEAL'),
(-1000816,'We are saved! The peasants have escaped the Scourge!',0,1,0,0,'Eris Havenfire SAY_EVENT_END'),
(-1000817,'I have failed once more...',0,1,0,0,'Eris Havenfire SAY_EVENT_FAIL_1'),
(-1000818,'I now return to whence I came, only to find myself here once more to relive the same epic tragedy.',0,0,0,0,'Eris Havenfire SAY_EVENT_FAIL_2'),
(-1000819,'The Scourge are upon us! Run! Run for your lives!',0,1,0,0,'Peasant SAY_PEASANT_APPEAR_1'),
(-1000820,'Please help us! The Prince has gone mad!',0,1,0,0,'Peasant SAY_PEASANT_APPEAR_2'),
(-1000821,'Seek sanctuary in Hearthglen! It is our only hope!',0,1,0,0,'Peasant SAY_PEASANT_APPEAR_3');

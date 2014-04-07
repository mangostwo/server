DELETE FROM script_texts WHERE entry IN (-1509003,-1509004,-1509031,-1509029,-1509030);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1509003,'They come now. Try not to get yourself killed, young blood.',0,1,0,22,'andorov SAY_ANDOROV_INTRO_3'),
(-1509004,'Remember, Rajaxx, when I said I\'d kill you last?',0,1,0,0,'andorov SAY_ANDOROV_INTRO_1'),
(-1509031,'I lied...',0,1,0,0,'andorov SAY_ANDOROV_INTRO_2'),
(-1509029,'Come get some!',0,0,0,0,'andorov SAY_ANDOROV_INTRO_4'),
(-1509030,'Kill first, ask questions later... Incoming!',0,1,0,0,'andorov SAY_ANDOROV_ATTACK_START');
DELETE FROM gossip_texts WHERE entry IN (-3509000,-3509001);
INSERT INTO gossip_texts (entry,content_default,comment) VALUES
(-3509000,'Let\'s find out.','andorov GOSSIP_ITEM_START'),
(-3509001,'Let\'s see what you have.','andorov GOSSIP_ITEM_TRADE');

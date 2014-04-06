UPDATE script_texts SET type=6 WHERE entry=-1409004;

DELETE FROM script_texts WHERE entry IN (-1409007, -1409019, -1409020, -1409021, -1409022);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1409007,'Impossible! Stay your attack mortals! I submitt! I submitt!',8038,1,0,0,'majordomo SAY_DEFEAT_1'),
(-1409019,'You think you\'ve won already? Perhaps you\'ll need another lesson in pain!',0,1,0,0,'majordomo SAY_LAST_ADD'),
(-1409020,'Brashly you have come to rest the secrets of the living flame. You will soon regret the recklessness of your quest.',0,1,0,0,'majordomo SAY_DEFEAT_2'),
(-1409021,'I go now to summon the lord whos house this is. Should you seek an audiance with him your paltry lives will surly be forfit. Nevertheless seek out his lair if you dare!',0,1,0,0,'majordomo SAY_DEFEAT_3'),
(-1409022,'My flame! Please don\'t take away my flame... ',8042,1,0,0,'ragnaros SAY_ARRIVAL4_MAJ');

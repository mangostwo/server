DELETE FROM script_texts WHERE entry BETWEEN -1000620 AND -1000610;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000610,'The mosswalker victim groans in pain.',0,2,0,0,'mosswalker victim EMOTE_PAIN'),
(-1000611,'Maybe you make weather better too?',0,0,0,0,'mosswalker victim SAY_RESCUE_1'),
(-1000612,'We saved. You nice, $N.',0,0,0,0,'mosswalker victim SAY_RESCUE_2'),
(-1000613,'You save us! Yay for you!',0,0,0,0,'mosswalker victim SAY_RESCUE_3'),
(-1000614,'Thank you! You good!',0,0,0,0,'mosswalker victim SAY_RESCUE_4'),
(-1000615,'Use my shinies...make weather good again...make undead things go away.',0,0,0,0,'mosswalker victim SAY_DIE_1'),
(-1000616,'We gave shinies to shrine... we not greedy... why this happen?',0,0,0,0,'mosswalker victim SAY_DIE_2'),
(-1000617,'I do something bad? I sorry....',0,0,0,0,'mosswalker victim SAY_DIE_3'),
(-1000618,'We not do anything... to them... I no understand.',0,0,0,0,'mosswalker victim SAY_DIE_4'),
(-1000619,'Thank...you.',0,0,0,0,'mosswalker victim SAY_DIE_5'),
(-1000620,'Please take... my shinies. All done...',0,0,0,0,'mosswalker victim SAY_DIE_6');

DELETE FROM gossip_texts WHERE entry=-3000104;
INSERT INTO gossip_texts (entry,content_default,comment) VALUES
(-3000104,'<Check for pulse...>','mosswalker victim GOSSIP_ITEM_PULSE');

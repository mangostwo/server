DELETE FROM script_texts WHERE entry IN (-1603012,-1603013);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1603012,'As you wish, $N.',0,0,0,0,'keeper SAY_KEEPER_ACTIVE'),
(-1603013,'REUSE ME',0,0,0,0,'REUSE ME');
DELETE FROM gossip_texts WHERE entry IN (-3603013,-3603014);
INSERT INTO gossip_texts (entry,content_default,comment) VALUES
(-3603013,'Lend us your aid, keeper. Together we will defeat Yogg-Saron.','Ulduar Keeper GOSSIP_ITEM_LEND_AID'),
(-3603014,'Yes.','Ulduar Keeper GOSSIP_ITEM_KEEPER_CONFIRM');

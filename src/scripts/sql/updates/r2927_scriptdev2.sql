DELETE FROM script_texts WHERE entry IN (-1603040,-1603238);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1603040,'Move quickly! She won\'t remain grounded for long!',15648,1,0,0,'razorscale SAY_GROUNDED'),
(-1603238,'%s is extinguished by the water!',0,2,0,0,'ignis EMOTE_EXTINGUISH_SCORCH');
DELETE FROM gossip_texts WHERE entry=-3603009;
INSERT INTO gossip_texts (entry,content_default,comment) VALUES
(-3603009,'We are ready to help!','Expedition Commander GOSSIP_ITEM_START_RAZORSCALE');

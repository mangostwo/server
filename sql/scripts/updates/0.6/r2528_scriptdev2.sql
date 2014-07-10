DELETE FROM script_texts WHERE entry=-1229019;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1229019,'%s is knocked off his drake!',0,2,0,0,'rend EMOTE_KNOCKED_OFF');
UPDATE script_texts SET emote=1 WHERE entry IN (-1229004,-1229005,-1229007,-1229014,-1229018);
UPDATE script_texts SET emote=25 WHERE entry IN (-1229016);

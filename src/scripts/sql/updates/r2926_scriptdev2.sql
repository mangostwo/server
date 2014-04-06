DELETE FROM script_texts WHERE entry IN (-1603054,-1603055,-1603236,-1603237);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1603054,'%s\'s heart is exposed and leaking energy.',0,3,0,0,'xt-002 EMOTE_EXPOSE_HEART'),
(-1603055,'%s consumes a scrapbot to repair himself!',0,3,0,0,'xt-002 EMOTE_REPAIR'),
(-1603236,'%s\'s heart is severed from his body.',0,3,0,0,'xt-002 EMOTE_KILL_HEART'),
(-1603237,'%s begins to cause the earth to quake.',0,3,0,0,'xt-002 EMOTE_EARTH_QUAKE');

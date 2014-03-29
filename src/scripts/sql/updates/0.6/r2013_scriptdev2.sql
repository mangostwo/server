DELETE FROM script_texts WHERE entry IN (-1000642, -1000643);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000642,'It\'s a female.',0,5,0,0,'leopard icepaw SAY_ITS_FEMALE'),
(-1000643,'It\'s an angry male!',0,5,0,0,'leopard icepaw SAY_ITS_MALE');

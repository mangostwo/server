DELETE FROM script_texts WHERE entry IN (-1609287,-1609288);
DELETE FROM script_texts WHERE entry IN (-1609097,-1609098);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1609097,'%s rears up, beckoning you to ride it.',0,2,0,0,'Acherus Deathcharger EMOTE_HORSE_READY'),
(-1609098,'Impressive, death knight. Return to me in the world of the living for your reward.',0,0,0,2,'Salanar the Horseman SAY_RACE_FINISHED');

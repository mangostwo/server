UPDATE script_texts SET emote=5 WHERE entry IN (-1658063,-1658064);
UPDATE script_texts SET emote=396 WHERE entry IN (-1658067);
DELETE FROM script_texts WHERE entry IN (-1658051,-1658061,-1658068,-1658069);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1658051,'Heroes! We will hold off the undead as long as we can, even to our dying breath. Deal with the Scourgelord!',17148,1,0,0,'victus SAY_VICTUS_TRASH'),
(-1658061,'Brave champions, we owe you our lives, our freedom... Though it be a tiny gesture in the face of this enormous debt, I pledge that from this day forth, all will know of your deeds, and the blazing path of light you cut through the shadow of this dark citadel.',17149,1,0,0,'victus SAY_VICTUS_OUTRO_1'),
(-1658068,'Heroes! We will hold off the undead as long as we can, even to our dying breath. Deal with the Scourgelord!',17150,1,0,0,'ironskull SAY_IRONSKULL_TRASH'),
(-1658069,'Brave champions, we owe you our lives, our freedom... Though it be a tiny gesture in the face of this enormous debt, I pledge that from this day forth, all will know of your deeds, and the blazing path of light you cut through the shadow of this dark citadel.',17151,1,0,0,'ironskull SAY_IRONSKULL_OUTRO_1');

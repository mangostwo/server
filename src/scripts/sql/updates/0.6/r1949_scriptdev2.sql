DELETE FROM script_texts WHERE entry BETWEEN -1043019 AND -1043000;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1043000,'At last! Naralex can be awakened! Come aid me, brave adventurers!',0,6,0,0,'Disciple of Naralex - SAY_INTRO'),
(-1043001,'I must make the nescessary preparations before the awakening ritual can begin. You must protect me!',0,0,0,0,'SAY_PREPARE'),
(-1043002,'These caverns were once a temple of promise for regrowth in the Barrens. Now, they are the halls of nightmares.',0,0,0,0,'Disciple of Naralex - SAY_FIRST_CORNER'),
(-1043003,'Come. We must continue. There is much to be done before we can pull Naralex from his nightmare.',0,0,0,0,'Disciple of Naralex - SAY_CONTINUE'),
(-1043004,'Within this circle of fire I must cast the spell to banish the spirits of the slain Fanglords.',0,0,0,0,'Disciple of Naralex - SAY_CIRCLE_BANISH'),
(-1043005,'The caverns have been purified. To Naralex''s chamber we go!',0,0,0,0,'Disciple of Naralex - SAY_PURIFIED'),
(-1043006,'Beyond this corridor, Naralex lies in fitful sleep. Let us go awaken him before it is too late.',0,0,0,0,'Disciple of Naralex - SAY_NARALEX_CHAMBER'),
(-1043007,'Protect me brave souls as I delve into the Emerald Dream to rescue Naralex and put an end to this corruption!',0,1,0,0,'Disciple of Naralex - SAY_BEGIN_RITUAL'),
(-1043008,'%s begins to perform the awakening ritual on Naralex.',0,2,0,0,'Disciple of Naralex - EMOTE_RITUAL_BEGIN'),
(-1043009,'%s tosses fitfully in troubled sleep.',0,2,0,0,'Naralex - EMOTE_NARALEX_AWAKE'),
(-1043010,'%s writhes in agony. The Disciple seems to be breaking through.',0,2,0,0,'Naralex - EMOTE_BREAK_THROUGH'),
(-1043011,'%s dreams up a horrendous vision. Something stirs beneath the murky waters.',0,2,0,0,'Naralex - EMOTE_VISION'),
(-1043012,'This $N is a minin from Naralex\'s nightmare no doubt!.',0,0,0,0,'Disciple of Naralex - SAY_MUTANUS'),
(-1043013,'I AM AWAKE, AT LAST!',5789,1,0,0,'Naralex - SAY_NARALEX_AWAKE'),
(-1043014,'At last! Naralex awakes from the nightmare.',0,0,0,0,'Disciple of Naralex - SAY_AWAKE'),
(-1043015,'Ah, to be pulled from the dreaded nightmare! I thank you, my loyal Disciple, along with your brave companions.',0,0,0,0,'Naralex - SAY_NARALEX_THANKYOU'),
(-1043016,'We must go and gather with the other Disciplies. There is much work to be done before I can make another attempt to restore the Barrens. Farewell, brave souls!',0,0,0,0,'Naralex - SAY_FAREWELL'),
(-1043017,'Attacked! Help get this $N off of me!',0,0,0,0,'Disciple of Naralex - SAY_AGGRO_1'),
(-1043018,'Help!',0,0,0,0,'Disciple of Naralex - SAY_AGGRO_2'),
(-1043019,'Deal with this $N! I need to prepare to awake Naralex!',0,0,0,0,'Disciple of Naralex - SAY_AGGRO_3');

DELETE FROM gossip_texts WHERE entry=-3043000;
INSERT INTO gossip_texts (entry,content_default,comment) VALUES
(-3043000,'Let the event begin!','Disciple of Naralex - GOSSIP_ITEM_BEGIN');

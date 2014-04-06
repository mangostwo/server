DELETE FROM script_texts WHERE entry IN (-1603242,-1603243,-1603244,-1603245);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1603242,'%s activates Hodir\'s Fury.',0,3,0,0,'leviathan EMOTE_HODIR_FURY'),
(-1603243,'%s activates Freya\'s Ward.',0,3,0,0,'leviathan EMOTE_FREYA_WARD'),
(-1603244,'%s activates Mimiron\'s Inferno.',0,3,0,0,'leviathan EMOTE_MIMIRON_INFERNO'),
(-1603245,'%s activates Thorim\'s Hammer.',0,3,0,0,'leviathan EMOTE_THORIM_HAMMER');

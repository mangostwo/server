DELETE FROM script_texts WHERE entry IN (-1649071,-1649072,-1649073,-1649074);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1649071,'%s burrows into the ground!',0,3,0,0,'anubarak EMOTE_BURROW'),
(-1649072,'%s spikes pursue $N!',0,3,0,0,'anubarak EMOTE_PURSUE'),
(-1649073,'%s emerges from the ground!',0,3,0,0,'anubarak EMOTE_EMERGE'),
(-1649074,'%s unleashes a Leeching Swarm to heal himself!',0,3,0,0,'anubarak EMOTE_SWARM');

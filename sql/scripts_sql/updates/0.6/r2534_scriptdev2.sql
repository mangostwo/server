DELETE FROM script_texts WHERE entry=-1000193;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000193,'REUSE ME',0,0,0,0,'REUSE ME');
UPDATE script_texts SET language=0, comment='npc_redemption_target SAY_HEAL' WHERE entry IN (-1000187);

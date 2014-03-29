DELETE FROM script_texts WHERE entry BETWEEN -1000342 AND -1000340;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000340, '%s howls in delight at the sight of his lunch!',0,2,0,0,'kyle EMOTE_SEE_LUNCH'),
(-1000341, '%s eats his lunch.',0,2,0,0,'kyle EMOTE_EAT_LUNCH'),
(-1000342, '%s thanks you with a special dance.',0,2,0,0,'kyle EMOTE_DANCE');

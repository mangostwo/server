DELETE FROM script_texts WHERE entry IN (-1000323, -1000324);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000323, 'Mist! I feared I would never see you again! Yes, I am well, do not worry for me. You must rest and recover your health.',0,0,7,0,'mist SAY_AT_HOME'),
(-1000324, 'growls in acknowledgement before straightening and making her way off into the forest.',0,2,0,0,'mist EMOTE_AT_HOME');

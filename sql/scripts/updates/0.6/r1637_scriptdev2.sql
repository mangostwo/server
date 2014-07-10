UPDATE script_texts SET content_default='Foolishly you have sought your own demise.', comment='gothik SAY_SPEECH_1' WHERE entry=-1533040;

DELETE FROM script_texts WHERE entry BETWEEN -1533142 AND -1533140;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1533140,'Brazenly you have disregarded powers beyond your understanding.',0,1,0,0,'gothik SAY_SPEECH_2'),
(-1533141,'You have fought hard to invade the realm of the harvester.',0,1,0,0,'gothik SAY_SPEECH_3'),
(-1533142,'Now there is only one way out - to walk the lonely path of the damned.',0,1,0,0,'gothik SAY_SPEECH_4');

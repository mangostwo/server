DELETE FROM script_texts WHERE entry BETWEEN -1578023 AND -1578020;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1578020,'There will be no mercy!',13649,1,0,0,'varos SAY_AGGRO'),
(-1578021,'Blast them! Destroy them!',13650,1,0,0,'varos SAY_CALL_CAPTAIN_1'),
(-1578022,'Take no prisoners! Attack!',13651,1,0,0,'varos SAY_CALL_CAPTAIN_2'),
(-1578023,'Strike now! Obliterate them!',13652,1,0,0,'varos SAY_CALL_CAPTAIN_3');
DELETE FROM script_texts WHERE entry BETWEEN -1578029 AND -1578026;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1578026,'You were warned!',13653,1,0,0,'varos SAY_KILL_1'),
(-1578027,'The Oculus is ours!',13654,1,0,0,'varos SAY_KILL_2'),
(-1578028,'They are... too strong! Underestimated their... fortitude.',13655,1,0,0,'varos SAY_DEATH'),
(-1578029,'%s calls an Azure Ring Captain!',0,3,0,0,'varos EMOTE_CAPTAIN');

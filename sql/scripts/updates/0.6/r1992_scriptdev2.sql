DELETE FROM script_texts WHERE entry BETWEEN -1329015 AND -1329008;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1329008,'Intruders at the Service Gate! Lord Rivendare must be warned!',0,6,0,0,'barthilas SAY_WARN_BARON'),
(-1329009,'Intruders! More pawns of the Argent Dawn, no doubt. I already count one of their number among my prisoners. Withdraw from my domain before she is executed!',0,6,0,0,'baron rivendare SAY_ANNOUNCE_RUN_START'),
(-1329010,'You\'re still here? Your foolishness is amusing! The Argent Dawn wench needn\'t suffer in vain. Leave at once and she shall be spared!',0,6,0,0,'baron rivendare SAY_ANNOUNCE_RUN_10_MIN'),
(-1329011,'I shall take great pleasure in taking this poor wretch\'s life! It\'s not too late, she needn\'t suffer in vain. Turn back and her death shall be merciful!',0,6,0,0,'baron rivendare SAY_ANNOUNCE_RUN_5_MIN'),
(-1329012,'May this prisoner\'s death serve as a warning. None shall defy the Scourge and live!',0,6,0,0,'baron rivendare SAY_ANNOUNCE_RUN_FAIL'),
(-1329013,'So you see fit to toy with the Lich King\'s creations? Ramstein, be sure to give the intruders a proper greeting.',0,6,0,0,'baron rivendare SAY_ANNOUNCE_RAMSTEIN'),
(-1329014,'Time to take matters into my own hands. Come. Enter my domain and challenge the might of the Scourge!',0,6,0,0,'baron rivendare SAY_UNDEAD_DEFEAT'),
(-1329015,'You did it... you\'ve slain Baron Rivendare! The Argent Dawn shall hear of your valiant deeds!',0,0,0,0,'ysida SAY_EPILOGUE');

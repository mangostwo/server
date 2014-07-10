DELETE FROM script_texts WHERE entry BETWEEN -1603260 AND -1603248;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1603248,'Self-destruct sequence initiated.',15413,1,0,0,'mimiron SAY_SELF_DESTRUCT'),
(-1603249,'This area will self-destruct in ten minutes.',15415,1,0,0,'mimiron SAY_DESTRUCT_10_MIN'),
(-1603250,'This area will self-destruct in nine minutes.',15416,1,0,0,'mimiron SAY_DESTRUCT_9_MIN'),
(-1603251,'This area will self-destruct in eight minutes.',15417,1,0,0,'mimiron SAY_DESTRUCT_8_MIN'),
(-1603252,'This area will self-destruct in seven minutes.',15418,1,0,0,'mimiron SAY_DESTRUCT_7_MIN'),
(-1603253,'This area will self-destruct in six minutes.',15419,1,0,0,'mimiron SAY_DESTRUCT_6_MIN'),
(-1603254,'This area will self-destruct in five minutes.',15420,1,0,0,'mimiron SAY_DESTRUCT_5_MIN'),
(-1603255,'This area will self-destruct in four minutes.',15421,1,0,0,'mimiron SAY_DESTRUCT_4_MIN'),
(-1603256,'This area will self-destruct in three minutes.',15422,1,0,0,'mimiron SAY_DESTRUCT_3_MIN'),
(-1603257,'This area will self-destruct in two minutes.',15423,1,0,0,'mimiron SAY_DESTRUCT_2_MIN'),
(-1603258,'This area will self-destruct in one minute.',15424,1,0,0,'mimiron SAY_DESTRUCT_1_MIN'),
(-1603259,'Self-destruct sequence finalized. Have a nice day.',15425,1,0,0,'mimiron SAY_DESTRUCT_0_MIN'),
(-1603260,'Self-destruct sequence terminated. Overide code A905.',15414,1,0,0,'mimiron SAY_SELF_DESTRUCT_END');
UPDATE script_texts SET emote=1 WHERE entry=-1603194;

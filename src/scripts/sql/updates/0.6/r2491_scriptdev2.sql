DELETE FROM script_texts WHERE entry BETWEEN -1000814 AND -1000803;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1000803,'You do not fight alone, %n! Together, we will banish this spawn of hellfire!',0,1,0,0,'Oronok SAY_ORONOK_TOGETHER'),
(-1000804,'We will fight when you are ready.',0,0,0,0, 'Oronok SAY_ORONOK_READY'),
(-1000805,'We will set the elements free of your grasp by force!',0,1,0,0,'Oronok SAY_ORONOK_ELEMENTS'),
(-1000806,'What say the elements, Torlok? I only hear silence.',0,0,0,1,'Oronok SAY_ORONOK_EPILOGUE_1'),
(-1000807,'I hear what you hear, brother. Look behind you...',0,0,0,1,'Torlok SAY_TORLOK_EPILOGUE_2'),
(-1000808,'They are redeemed! Then we have won?',0,0,0,1,'Oronok SAY_ORONOK_EPILOGUE_3'),
(-1000809,'It is now as it should be, shaman. You have done well.',0,0,0,0,'Spirit of Earth SAY_EARTH_EPILOGUE_4'),
(-1000810,'Yes... Well enough for the elements that are here, but the cipher is known to another... The spirits of fire are in turmoil... If this force is not stopped, the world where these mortals came from will cease.',0,0,0,0,'Spirit of Fire SAY_FIRE_EPILOGUE_5'),
(-1000811,'Farewell, mortals... The earthmender knows what fire feels...',0,0,0,0, 'Spirit of Earth SAY_EARTH_EPILOGUE_6'),
(-1000812,'We leave, Torlok. I have only one request...',0,0,0,1,'Oronok SAY_ORONOK_EPILOGUE_7'),
(-1000813,'The Torn-heart men give their weapons to Earthmender Torlok.',0,2,0,0,'Torlok EMOTE_GIVE_WEAPONS'),
(-1000814,'Give these to the heroes that made this possible.',0,0,0,1,'Oronok SAY_ORONOK_EPILOGUE_8');

DELETE FROM gossip_texts WHERE entry=-3000109;
INSERT INTO gossip_texts (entry,content_default,comment) VALUES
(-3000109,'I am ready, Oronok. Let us destroy Cyrukh and free the elements!','oronok torn-heart GOSSIP_ITEM_FIGHT');

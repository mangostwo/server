DELETE FROM script_texts WHERE entry IN (-1540048, -1540049, -1540050);
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1540048,'Cowards! You\'ll never pull me into the shadows!',0,1,0,0,'kargath SAY_EVADE'),
(-1540049,'The Alliance dares to intrude this far into my fortress? Bring out the Honor Hold prisoners and call for the executioner! They\'ll pay with their lives for this trespass!',0,6,0,0,'kargath SAY_EXECUTE_ALLY'),
(-1540050,'It looks like we have a ranking officer among our captives...how amusing. Execute the green-skinned dog at once!',0,6,0,0,'kargath SAY_EXECUTE_HORDE');

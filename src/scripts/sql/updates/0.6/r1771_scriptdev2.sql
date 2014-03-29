DELETE FROM script_texts WHERE entry BETWEEN -1070004 AND -1070001;
INSERT INTO script_texts (entry,content_default,sound,type,language,emote,comment) VALUES
(-1070001,'Who dares awaken Archaedas? Who dares the wrath of the makers!',5855,1,0,0,'archaedas SAY_AGGRO'),
(-1070002,'Awake ye servants, defend the discs!',5856,1,0,0,'archaedas SAY_AWAKE_GUARDIANS'),
(-1070003,'To my side, brothers. For the makers!',5857,1,0,0,'archaedas SAY_AWAKE_WARDERS'),
(-1070004,'Reckless mortal.',5858,1,0,0,'archaedas SAY_UNIT_SLAIN');

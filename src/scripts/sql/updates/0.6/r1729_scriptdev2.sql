UPDATE script_texts SET content_default='Thank you for helping me $r. Please tell the king I am back.' WHERE entry=-1000578;
UPDATE script_waypoint SET location_x=4270.07, location_y=6188.42, location_z=0.059, waittime=15000 WHERE entry=25208 AND pointid=25;
DELETE FROM script_waypoint WHERE entry=25208 AND pointid=26;

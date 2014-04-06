UPDATE instance_template SET ScriptName='instance_zulfarrak' WHERE map=209;
DELETE FROM scripted_event_id WHERE id=2488;
INSERT INTO scripted_event_id VALUES (2488,'event_go_zulfarrak_gong');
DELETE FROM scripted_areatrigger WHERE entry=1447;
INSERT INTO scripted_areatrigger VALUES (1447,'at_zulfarrak');

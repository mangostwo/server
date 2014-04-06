UPDATE gameobject_template SET ScriptName='go_eternal_flame' WHERE entry IN (148418,148419,148420,148421);
UPDATE creature_template SET ScriptName='npc_shade_of_hakkar' WHERE entry=8440;
DELETE FROM scripted_event_id WHERE id=8502;
INSERT INTO scripted_event_id VALUES
(8502,'event_avatar_of_hakkar');

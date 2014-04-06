UPDATE creature_template SET ScriptName='' WHERE npcflag!=npcflag|65536 AND ScriptName='npc_innkeeper';
UPDATE creature_template SET ScriptName='npc_innkeeper' WHERE npcflag=npcflag|65536;

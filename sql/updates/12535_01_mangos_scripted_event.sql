ALTER TABLE  `db_version` CHANGE `required_12523_01_mangos_db_script_string` `required_12535_01_mangos_scripted_event` BIT(1) NULL DEFAULT NULL;

RENAME TABLE  `scripted_event_id` TO  `scripted_event` ;

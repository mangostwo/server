CREATE TABLE custom_texts (
    entry bigint NOT NULL,
    content_default text NOT NULL,
    content_loc1 text,
    content_loc2 text,
    content_loc3 text,
    content_loc4 text,
    content_loc5 text,
    content_loc6 text,
    content_loc7 text,
    content_loc8 text,
    sound integer DEFAULT 0 NOT NULL,
    type smallint DEFAULT (0)::smallint NOT NULL,
    language smallint DEFAULT (0)::smallint NOT NULL,
    emote smallint DEFAULT (0)::smallint NOT NULL,
    comment text,
    PRIMARY KEY(entry)
);

CREATE TABLE gossip_texts (
    entry bigint NOT NULL,
    content_default text NOT NULL,
    content_loc1 text,
    content_loc2 text,
    content_loc3 text,
    content_loc4 text,
    content_loc5 text,
    content_loc6 text,
    content_loc7 text,
    content_loc8 text,
    comment text,
    PRIMARY KEY(entry)
);

CREATE TABLE script_texts (
    entry bigint NOT NULL,
    content_default text NOT NULL,
    content_loc1 text,
    content_loc2 text,
    content_loc3 text,
    content_loc4 text,
    content_loc5 text,
    content_loc6 text,
    content_loc7 text,
    content_loc8 text,
    sound integer DEFAULT 0 NOT NULL,
    type smallint DEFAULT (0)::smallint NOT NULL,
    language smallint DEFAULT (0)::smallint NOT NULL,
    emote smallint DEFAULT (0)::smallint NOT NULL,
    comment text,
    PRIMARY KEY(entry)
);

CREATE TABLE script_waypoint (
    entry bigint NOT NULL,
    pointid bigint NOT NULL,
    location_x float DEFAULT (0)::float NOT NULL,
    location_y float DEFAULT (0)::float NOT NULL,
    location_z float DEFAULT (0)::float NOT NULL,
    waittime integer DEFAULT 0 NOT NULL,
    point_comment text,
    PRIMARY KEY(entry,pointid)
);

CREATE TABLE sd2_db_version (
    version varchar(255) NOT NULL DEFAULT ''
);

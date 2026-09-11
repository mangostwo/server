#!/bin/sh
# Entrypoint for the mangosd / realmd containers.
#
# On first start (when the target .conf does not yet exist) it seeds the config
# from the shipped .conf.dist and rewrites the database connection lines from
# environment variables. If the .conf already exists (e.g. bind-mounted by the
# operator) it is left untouched.
#
# Database environment variables (all optional, defaults shown):
#   DB_HOST    host of the MySQL server        (mysqldb)
#   DB_PORT    port of the MySQL server        (3306)
#   DB_USER    MySQL user                      (mangos)
#   DB_PASS    MySQL password                  (mangos)
#   DB_REALMD  realm/login database name       (realmd)
#   DB_WORLD   world database name             (mangos2)   [mangosd only]
#   DB_CHARS   character database name         (character2)[mangosd only]
set -eu

DAEMON="${1:?usage: docker-entrypoint.sh <mangosd|realmd> [command...]}"
shift

DB_HOST="${DB_HOST:-mysqldb}"
DB_PORT="${DB_PORT:-3306}"
DB_USER="${DB_USER:-mangos}"
DB_PASS="${DB_PASS:-mangos}"
DB_REALMD="${DB_REALMD:-realmd}"
DB_WORLD="${DB_WORLD:-mangos2}"
DB_CHARS="${DB_CHARS:-character2}"

CONF="/mangos/etc/${DAEMON}.conf"
# The template is kept outside /mangos/etc so it survives a bind-mount of the
# etc directory (compose mounts the host's etc over /mangos/etc).
DIST="/mangos-defaults/${DAEMON}.conf.dist"

# Rewrite a "Key = "..."" line in place with a fresh connection string.
set_dbinfo()
{
    key="$1"
    dbname="$2"
    value="${DB_HOST};${DB_PORT};${DB_USER};${DB_PASS};${dbname}"
    # Anchor on the key at start of line; replace the whole line so whatever
    # default the .dist carried is overwritten.
    sed -i "s|^${key}[[:space:]]*=.*|${key} = \"${value}\"|" "$CONF"
}

if [ ! -f "$CONF" ]
then
    echo "[entrypoint] ${CONF} not found; seeding from ${DIST}"
    cp "$DIST" "$CONF"

    set_dbinfo "LoginDatabaseInfo" "$DB_REALMD"
    if [ "$DAEMON" = "mangosd" ]
    then
        set_dbinfo "WorldDatabaseInfo"     "$DB_WORLD"
        set_dbinfo "CharacterDatabaseInfo" "$DB_CHARS"
    fi
    echo "[entrypoint] database connection info written to ${CONF}"
else
    echo "[entrypoint] ${CONF} exists; leaving it untouched"
fi

# With no extra arguments, run the daemon against the seeded config. Any
# arguments passed after the daemon name (e.g. a k8s command/args override)
# are exec'd verbatim instead, so the image stays fully controllable.
if [ "$#" -eq 0 ]
then
    exec "./${DAEMON}" -c "$CONF"
else
    exec "$@"
fi

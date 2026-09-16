# Running MangosTwo with Docker

This folder contains everything needed to run the world server (`mangosd`) and
login server (`realmd`) in containers, backed by a MySQL container.

There are two compose files:

| File                        | Use                                                            |
| --------------------------- | -------------------------------------------------------------- |
| `docker-compose.yml`        | Pull prebuilt images from GitHub Container Registry (default). |
| `docker-compose.build.yml`  | Build the images locally from the Dockerfiles here.            |

`docker-compose.build.yml` is an *override* layered on top of the base file.

## 1. Configure

Copy the example environment file and edit it:

```sh
cp .env.example .env
```

`docker compose` reads `.env` automatically. Every value is optional and falls
back to a sensible default; see `.env.example` for the full list. The common
ones:

- `REGISTRY_OWNER` / `MANGOS_TAG` — which image and tag to pull (registry mode).
- `DB_HOST`, `DB_PORT`, `DB_USER`, `DB_PASS`, `DB_REALMD`, `DB_WORLD`, `DB_CHARS`
  — the database connection the servers use.
- `MYSQL_USER`, `MYSQL_PASSWORD`, `MYSQL_ROOT_PASSWORD`, `MYSQL_ROOT_HOST`
  — the bundled MySQL container.
- `INIT_ENV_CONFIG` — whether the entrypoint seeds a `.conf` from the template
  when none exists (default `true`; see "Bringing your own configuration").

### Config files are generated for you

On first start each container seeds its config from the shipped template:

- If `mangosd.conf` / `realmd.conf` does **not** exist in the mounted `etc`
  folder, it is created from the `.conf.dist` and the database connection lines
  are filled in from the `DB_*` variables above.
- If the `.conf` already exists, it is left untouched — edit it by hand for
  anything the environment variables do not cover.

### Bringing your own configuration

Mount your own `mangosd.conf` / `realmd.conf` into `/mangos/etc` and the
entrypoint will detect it and run it as-is, without generating or rewriting
anything. To turn off seeding altogether (even when no `.conf` is present, e.g.
you supply it another way or override the command), set
`INIT_ENV_CONFIG=false`.

The config folder is bind-mounted from the repository:

- configuration: `./etc` → `/mangos/etc`

This path can be changed in `docker-compose.yml`. There is no separate data
volume — client data (`tiles`, `gomodels`, `dbc`) is placed under this same
mounted `etc` folder; see "Extract and add the client data" below. Inside
`mangosd.conf`, `DataDir` should point at wherever you place it under
`/mangos/etc`.

## 2. Initialise the databases

The MySQL data is stored in `../../dbdata`. The game databases must be created
and populated once before the servers will run — see the `mangostwo/database`
repository for the schema and migrations. The `mysqldb` service must be running
for this step; start just that service with:

```sh
docker compose up mysqldb
```

For the SQL import itself, follow the "Import the databases" section of the
official installation guide:

<https://www.getmangos.eu/wiki/documentation/installation-guides/guideslinux/installing-mangos-on-ubuntu-server-2204-r40014/>

Point the `mysql` client at the container instead of a local install, e.g.
`mysql -h 127.0.0.1 -P 3306 -u root -p`, using the `DB_*` / `MYSQL_*` values
from your `.env`.

## 3. Extract and add the client data

Run the extractor (`mangos-extractor`, built from `src/tools/extractor`) against
your client install to produce `tiles`, `gomodels` and `dbc`, then copy the
result into `./etc` (the folder bind-mounted to `/mangos/etc`) so the
container can see it. Set `DataDir` in `mangosd.conf` to match wherever you
place it under `/mangos/etc`.

## 4. Set the public address players connect to

The address the game **client** connects to is stored in the database, not in a
config file. After the databases are initialised, point the realm at your
public IP or DNS name:

```sql
UPDATE realmd.realmlist SET address = '203.0.113.10' WHERE id = 1;
```

`127.0.0.1` only works for a client on the same machine. `realmd`'s `BindIP`
stays `0.0.0.0` (listen on all interfaces) and does not need changing.

## Ports

- `8085` — world server (`mangosd`)
- `3724` — login server (`realmd`)
- `3306` — MySQL

## 6. Start

Registry images (default):

```sh
docker compose pull
docker compose up -d # Detach version
# or
docker compose up
```

Build locally instead:

```sh
docker compose -f docker-compose.yml -f docker-compose.build.yml up --build
```


## Attaching to the mangosd console

`mangosd` runs as the container's foreground process and reads commands from
stdin, so you can attach to it for interactive console access.

Find the container ID (or name) with:

```sh
docker compose ps
```

or, from anywhere, `docker ps` and look for the `mangosd` image/service. Then
attach to it:

```sh
docker attach <containerID>
```

**Detach with `Ctrl+P` then `Ctrl+Q`, not `Ctrl+C`.** `Ctrl+C` sends `SIGINT` to
`mangosd` inside the container and shuts the server down; the detach sequence
leaves the process running and just returns you to your shell.

## Kubernetes / other orchestrators

The entrypoint is baked into the image and behaves well outside compose:

- Mount your own `mangosd.conf` / `realmd.conf` and the entrypoint leaves it
  alone (it only seeds when absent).
- Override the command entirely and it is `exec`'d verbatim, e.g.
  `command: ["/usr/local/bin/docker-entrypoint.sh", "mangosd"]` with your own
  `args`.

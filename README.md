pg_command_launcher
====

pg_command_launcher is a PostgreSQL extension which enables us to make
PostgreSQL server execute the specified shell command at various timings.

## Install
    $ make USE_PGXS=1 PG_CONFIG=/opt/pgsql-X.Y.Z/bin/pg_config
    $ make USE_PGXS=1 PG_CONFIG=/opt/pgsql-X.Y.Z/bin/pg_config install
    $ vi $PGDATA/postgresql.conf
    shared_preload_libraries = 'pg_command_launcher'
    pg_command_launcher.command_at_startup = 'echo TEST'

## Parameters
* `pg_command_launcher.command_at_startup`  
  The shell command that will be executed at server startup.

* `pg_command_launcher.command_at_shutdown`  
  The shell command that will be executed at server shutdown.

# contrib/pg_command_launcher/Makefile

MODULE_big = pg_command_launcher
OBJS = pg_command_launcher.o $(WIN32RES)
PGFILEDESC = "pg_command_launcher - runs the specified command"

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/pg_command_launcher
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

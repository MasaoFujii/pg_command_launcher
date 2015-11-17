#include "postgres.h"

#include <signal.h>

#include "miscadmin.h"
#include "storage/ipc.h"
#include "utils/guc.h"

PG_MODULE_MAGIC;

/* Saved hook value in case of unload */
static shmem_startup_hook_type prev_shmem_startup_hook = NULL;

/* GUC variables */
static char	*command_at_startup = NULL;
static char	*command_at_shutdown = NULL;

/* Function declarations */
void		_PG_init(void);
void		_PG_fini(void);

static void pgcl_shmem_startup(void);
static void pgcl_shmem_shutdown(int code, Datum arg);
static void ExecuteCommand(char *command, char *commandName);

/*
 * Module load callback
 */
void
_PG_init(void)
{
	if (!process_shared_preload_libraries_in_progress)
	{
		ereport(WARNING,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("pg_command_launcher must be registered in shared_preload_libraries")));
		return;
	}

	/* Define (or redefine) custom GUC variables */
	DefineCustomStringVariable("pg_command_launcher.command_at_startup",
							 "Sets the shell command that will be called at server startup.",
							 NULL,
							 &command_at_startup,
							 "",
							 PGC_SIGHUP,
							 0,
							 NULL,
							 NULL,
							 NULL);

	DefineCustomStringVariable("pg_command_launcher.command_at_shutdown",
							 "Sets the shell command that will be called at server shutdown.",
							 NULL,
							 &command_at_shutdown,
							 "",
							 PGC_SIGHUP,
							 0,
							 NULL,
							 NULL,
							 NULL);

	EmitWarningsOnPlaceholders("pg_command_launcher");

	/* Install hook */
	prev_shmem_startup_hook = shmem_startup_hook;
	shmem_startup_hook = pgcl_shmem_startup;
}

/*
 * Module unload callback
 */
void
_PG_fini(void)
{
	/* Uninstall hook */
	shmem_startup_hook = prev_shmem_startup_hook;
}

/*
 * shmem_startup_hook
 */
static void
pgcl_shmem_startup(void)
{
	if (prev_shmem_startup_hook)
		prev_shmem_startup_hook();

	/*
	 * If we're in the postmaster (or a standalone backend...),
	 * set up a shmem exit hook to call command_at_shutdown.
	 */
	if (!IsUnderPostmaster)
		on_shmem_exit(pgcl_shmem_shutdown, (Datum) 0);

	if (command_at_startup)
		ExecuteCommand(command_at_startup, "command_at_startup");
}

static void
pgcl_shmem_shutdown(int code, Datum arg)
{
	if (command_at_shutdown)
		ExecuteCommand(command_at_shutdown, "command_at_shutdown");
}

static void
ExecuteCommand(char *command, char *commandName)
{
	int			rc;

	if (!command || !commandName)
		return;

	ereport(DEBUG3,
			(errmsg_internal("executing %s \"%s\"", commandName, command)));

	rc = system(command);
	if (rc != 0)
	{
		if (WIFEXITED(rc))
		{
			ereport(WARNING,
					(errmsg("%s failed with exit code %d",
							commandName, WEXITSTATUS(rc)),
					 errdetail("The failed command was: %s",
							   command)));
		}
		else if (WIFSIGNALED(rc))
		{
#if defined(WIN32)
			ereport(WARNING,
					(errmsg("%s was terminated by exception 0x%X",
							commandName, WTERMSIG(rc)),
					 errhint("See C include file \"ntstatus.h\" for a description of the hexadecimal value."),
					 errdetail("The failed command was: %s",
							   command)));
#elif defined(HAVE_DECL_SYS_SIGLIST) && HAVE_DECL_SYS_SIGLIST
			ereport(WARNING,
					(errmsg("%s was terminated by signal %d: %s",
							commandName, WTERMSIG(rc),
							WTERMSIG(rc) < NSIG ? sys_siglist[WTERMSIG(rc)] : "(unknown)"),
					 errdetail("The failed command was: %s",
							   command)));
#else
			ereport(WARNING,
					(errmsg("%s was terminated by signal %d",
							commandName, WTERMSIG(rc)),
					 errdetail("The failed command was: %s",
							   command)));
#endif
		}
		else
		{
			ereport(WARNING,
					(errmsg("%s exited with unrecognized status %d",
							commandName, rc),
					 errdetail("The failed command was: %s",
							   command)));
		}
	}
}

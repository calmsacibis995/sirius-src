#pragma ident	"%Z%%M%	%I%	%E% SMI"

#include <sys/param.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <err.h>
#include <sysexits.h>
#include <locale.h>
#include <libintl.h>

#include <netsmb/smb_lib.h>

#include "common.h"

#ifndef EX_DATAERR
#define	EX_DATAERR 1
#endif

static void help(void);


typedef int cmd_fn_t (int argc, char *argv[]);
typedef void cmd_usage_t (void);

#define	CMDFL_NO_KMOD	0x0001

static struct commands {
	const char	*name;
	cmd_fn_t	*fn;
	cmd_usage_t	*usage;
	int 		flags;
} commands[] = {
	{"crypt",	cmd_crypt,	NULL, CMDFL_NO_KMOD},
	{"help",	cmd_help,	help_usage, CMDFL_NO_KMOD},
	{"login",	cmd_login,	login_usage, 0},
	{"logout",	cmd_logout,	logout_usage, 0},
	{"logoutall",	cmd_logoutall,	logoutall_usage, 0},
	{"lookup",	cmd_lookup,	lookup_usage, CMDFL_NO_KMOD},
	{"status",	cmd_status,	status_usage, 0},
	{"view",	cmd_view,	view_usage, 0},
	{NULL, NULL, NULL, 0}
};

static struct commands *
lookupcmd(const char *name)
{
	struct commands *cmd;

	for (cmd = commands; cmd->name; cmd++) {
		if (strcmp(cmd->name, name) == 0)
			return (cmd);
	}
	return (NULL);
}

int
cmd_crypt(int argc, char *argv[])
{
	char *cp, *psw;

	if (argc < 2)
		psw = getpassphrase(gettext("Password:"));
	else
		psw = argv[1];
	/* XXX Better to embed malloc/free in smb_simplecrypt? */
	cp = malloc(4 + 2 * strlen(psw));
	if (cp == NULL)
		errx(EX_DATAERR, gettext("out of memory"));
	smb_simplecrypt(cp, psw);
	printf("%s\n", cp);
	free(cp);
	return (0);
}

int
cmd_help(int argc, char *argv[])
{
	struct commands *cmd;
	char *cp;

	if (argc < 2)
		help_usage();
	cp = argv[1];
	cmd = lookupcmd(cp);
	if (cmd == NULL)
		errx(EX_DATAERR, gettext("unknown command %s"), cp);
	if (cmd->usage == NULL)
		errx(EX_DATAERR,
		    gettext("no specific help for command %s"), cp);
	cmd->usage();
	return (0);
}

int
main(int argc, char *argv[])
{
	struct commands *cmd;
	char *cp;
	int opt;
	extern void dropsuid();

	(void) setlocale(LC_ALL, "");
	(void) textdomain(TEXT_DOMAIN);

	dropsuid();

	if (argc < 2)
		help();

	while ((opt = getopt(argc, argv, "dhv")) != EOF) {
		switch (opt) {
		case 'd':
			smb_debug++;
			break;
		case 'h':
			help();
			/* NOTREACHED */
		case 'v':
			smb_verbose++;
			break;
		default:
			help();
			/* NOTREACHED */
		}
	}
	if (optind >= argc)
		help();

	cp = argv[optind];
	cmd = lookupcmd(cp);
	if (cmd == NULL)
		errx(EX_DATAERR, gettext("unknown command %s"), cp);

	if ((cmd->flags & CMDFL_NO_KMOD) == 0 && smb_lib_init() != 0)
		exit(1);

	argc -= optind;
	argv += optind;
	optind = 1;
	return (cmd->fn(argc, argv));
}

static void
help(void) {
	printf("\n");
	printf(gettext("usage: %s [-hv] subcommand [args]\n"), __progname);
	printf(gettext("where subcommands are:\n"
	" crypt		slightly obscure password\n"
	" help		display help on specified subcommand\n"
	/* " lc 		display active connections\n" */
	" login		login to specified host\n"
	" logout 	logout from specified host\n"
	" logoutall	logout all users (requires privilege)\n"
	" lookup 	resolve NetBIOS name to IP address\n"
	/* " print		print file to the specified remote printer\n" */
	" status 	resolve IP address or DNS name to NetBIOS names\n"
	" view		list resources on specified host\n"
	"\n"));
	exit(1);
}

void
help_usage(void) {
	printf(gettext("usage: smbutil help command\n"));
	exit(1);
}

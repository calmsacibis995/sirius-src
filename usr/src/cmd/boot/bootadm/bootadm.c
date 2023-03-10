/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"

/*
 * bootadm(1M) is a new utility for managing bootability of
 * Solaris *Newboot* environments. It has two primary tasks:
 * 	- Allow end users to manage bootability of Newboot Solaris instances
 *	- Provide services to other subsystems in Solaris (primarily Install)
 */

/* Headers */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <limits.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/mnttab.h>
#include <sys/statvfs.h>
#include <libnvpair.h>
#include <ftw.h>
#include <fcntl.h>
#include <strings.h>
#include <utime.h>
#include <sys/systeminfo.h>
#include <sys/dktp/fdisk.h>
#include <sys/param.h>
#include <dirent.h>
#include <ctype.h>
#include <libgen.h>
#include <sys/sysmacros.h>
#include <libscf.h>

#if !defined(_OPB)
#include <sys/ucode.h>
#endif

#include <pwd.h>
#include <grp.h>
#include <device_info.h>
#include <sys/vtoc.h>
#include <sys/efi_partition.h>

#include <locale.h>

#include "message.h"
#include "bootadm.h"

#ifndef TEXT_DOMAIN
#define	TEXT_DOMAIN	"SUNW_OST_OSCMD"
#endif	/* TEXT_DOMAIN */

/* Type definitions */

/* Primary subcmds */
typedef enum {
	BAM_MENU = 3,
	BAM_ARCHIVE
} subcmd_t;

typedef enum {
    OPT_ABSENT = 0,	/* No option */
    OPT_REQ,		/* option required */
    OPT_OPTIONAL	/* option may or may not be present */
} option_t;

typedef struct {
	char	*subcmd;
	option_t option;
	error_t (*handler)();
	int	unpriv;			/* is this an unprivileged command */
} subcmd_defn_t;

#define	LINE_INIT	0	/* lineNum initial value */
#define	ENTRY_INIT	-1	/* entryNum initial value */
#define	ALL_ENTRIES	-2	/* selects all boot entries */

#define	GRUB_DIR		"/boot/grub"
#define	GRUB_STAGE2		GRUB_DIR "/stage2"
#define	GRUB_MENU		"/boot/grub/menu.lst"
#define	MENU_TMP		"/boot/grub/menu.lst.tmp"
#define	GRUB_BACKUP_MENU	"/etc/lu/GRUB_backup_menu"
#define	RAMDISK_SPECIAL		"/ramdisk"
#define	STUBBOOT		"/stubboot"
#define	MULTIBOOT		"/platform/i86pc/multiboot"
#define	GRUBSIGN_DIR		"/boot/grub/bootsign"
#define	GRUBSIGN_BACKUP		"/etc/bootsign"
#define	GRUBSIGN_UFS_PREFIX	"rootfs"
#define	GRUBSIGN_ZFS_PREFIX	"pool_"
#define	GRUBSIGN_LU_PREFIX	"BE_"
#define	UFS_SIGNATURE_LIST	"/var/run/grub_ufs_signatures"
#define	ZFS_LEGACY_MNTPT	"/tmp/bootadm_mnt_zfs_legacy"

#define	BOOTADM_RDONLY_TEST	"BOOTADM_RDONLY_TEST"

/* lock related */
#define	BAM_LOCK_FILE		"/var/run/bootadm.lock"
#define	LOCK_FILE_PERMS		(S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)

#define	CREATE_RAMDISK		"boot/solaris/bin/create_ramdisk"
#define	CREATE_DISKMAP		"boot/solaris/bin/create_diskmap"
#define	EXTRACT_BOOT_FILELIST	"boot/solaris/bin/extract_boot_filelist"
#define	GRUBDISK_MAP		"/var/run/solaris_grubdisk.map"

#define	GRUB_slice		"/etc/lu/GRUB_slice"
#define	GRUB_root		"/etc/lu/GRUB_root"
#define	GRUB_fdisk		"/etc/lu/GRUB_fdisk"
#define	GRUB_fdisk_target	"/etc/lu/GRUB_fdisk_target"
#define	FINDROOT_INSTALLGRUB	"/etc/lu/installgrub.findroot"
#define	LULIB			"/usr/lib/lu/lulib"
#define	LULIB_PROPAGATE_FILE	"lulib_propagate_file"
#define	CKSUM			"/usr/bin/cksum"
#define	LU_MENU_CKSUM		"/etc/lu/menu.cksum"
#define	BOOTADM			"/sbin/bootadm"

#define	INSTALLGRUB		"/sbin/installgrub"
#define	STAGE1			"/boot/grub/stage1"
#define	STAGE2			"/boot/grub/stage2"

typedef enum zfs_mnted {
	ZFS_MNT_ERROR = -1,
	LEGACY_MOUNTED = 1,
	LEGACY_ALREADY,
	ZFS_MOUNTED,
	ZFS_ALREADY
} zfs_mnted_t;




/*
 * The following two defines are used to detect and create the correct
 * boot archive  when safemode patching is underway.  LOFS_PATCH_FILE is a
 * contracted private interface between bootadm and the install
 * consolidation.  It is set by pdo.c when a patch with SUNW_PATCH_SAFEMODE
 * is applied.
 */

#define	LOFS_PATCH_FILE		"/var/run/.patch_loopback_mode"
#define	LOFS_PATCH_MNT		"/var/run/.patch_root_loopbackmnt"

/*
 * Default file attributes
 */
#define	DEFAULT_DEV_MODE	0644	/* default permissions */
#define	DEFAULT_DEV_UID		0	/* user root */
#define	DEFAULT_DEV_GID		3	/* group sys */

/*
 * Menu related
 * menu_cmd_t and menu_cmds must be kept in sync
 */
char *menu_cmds[] = {
	"default",	/* DEFAULT_CMD */
	"timeout",	/* TIMEOUT_CMD */
	"title",	/* TITLE_CMD */
	"root",		/* ROOT_CMD */
	"kernel",	/* KERNEL_CMD */
	"kernel$",	/* KERNEL_DOLLAR_CMD */
	"module",	/* MODULE_CMD */
	"module$",	/* MODULE_DOLLAR_CMD */
	" ",		/* SEP_CMD */
	"#",		/* COMMENT_CMD */
	"chainloader",	/* CHAINLOADER_CMD */
	"args",		/* ARGS_CMD */
	"findroot",	/* FINDROOT_CMD */
	NULL
};

#define	OPT_ENTRY_NUM	"entry"

/*
 * exec_cmd related
 */
typedef struct {
	line_t *head;
	line_t *tail;
} filelist_t;

#define	BOOT_FILE_LIST	"boot/solaris/filelist.ramdisk"
#define	ETC_FILE_LIST	"etc/boot/solaris/filelist.ramdisk"

#define	FILE_STAT	"boot/solaris/filestat.ramdisk"
#define	FILE_STAT_TMP	"boot/solaris/filestat.ramdisk.tmp"
#define	DIR_PERMS	(S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH)
#define	FILE_STAT_MODE	(S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)

/* Globals */
int bam_verbose;
int bam_force;
int bam_debug;
static char *prog;
static subcmd_t bam_cmd;
static char *bam_root;
static int bam_rootlen;
static int bam_root_readonly;
static int bam_alt_root;
static char *bam_subcmd;
static char *bam_opt;
static char **bam_argv;
static int bam_argc;
static int bam_check;
static int bam_smf_check;
static int bam_lock_fd = -1;
static int bam_zfs;
static char rootbuf[PATH_MAX] = "/";
static int bam_update_all;
static int bam_alt_platform;
static char *bam_platform;

/* function prototypes */
static void parse_args_internal(int, char *[]);
static void parse_args(int, char *argv[]);
static error_t bam_menu(char *, char *, int, char *[]);
static error_t bam_archive(char *, char *);

static void bam_exit(int);
static void bam_lock(void);
static void bam_unlock(void);

static int exec_cmd(char *, filelist_t *);
static error_t read_globals(menu_t *, char *, char *, int);
static int menu_on_bootdisk(char *os_root, char *menu_root);
static menu_t *menu_read(char *);
static error_t menu_write(char *, menu_t *);
static void linelist_free(line_t *);
static void menu_free(menu_t *);
static void filelist_free(filelist_t *);
static error_t list2file(char *, char *, char *, line_t *);
static error_t list_entry(menu_t *, char *, char *);
static error_t delete_all_entries(menu_t *, char *, char *);
static error_t update_entry(menu_t *mp, char *menu_root, char *opt);
static error_t update_temp(menu_t *mp, char *dummy, char *opt);

static error_t update_archive(char *, char *);
static error_t list_archive(char *, char *);
static error_t update_all(char *, char *);
static error_t read_list(char *, filelist_t *);
static error_t set_global(menu_t *, char *, int);
static error_t set_option(menu_t *, char *, char *);
static error_t set_kernel(menu_t *, menu_cmd_t, char *, char *, size_t);
static error_t get_kernel(menu_t *, menu_cmd_t, char *, size_t);
static char *expand_path(const char *);

static long s_strtol(char *);
static int s_fputs(char *, FILE *);

static int is_zfs(char *root);
static int is_ufs(char *root);
static int is_pcfs(char *root);
static int is_amd64(void);
static char *get_machine(void);
static void append_to_flist(filelist_t *, char *);
static char *mount_top_dataset(char *pool, zfs_mnted_t *mnted);
static int umount_top_dataset(char *pool, zfs_mnted_t mnted, char *mntpt);
static int ufs_add_to_sign_list(char *sign);
static error_t synchronize_BE_menu(void);

#if !defined(_OPB)
static void ucode_install();
#endif

/* Menu related sub commands */
static subcmd_defn_t menu_subcmds[] = {
	"set_option",		OPT_ABSENT,	set_option, 0,	/* PUB */
	"list_entry",		OPT_OPTIONAL,	list_entry, 1,	/* PUB */
	"delete_all_entries",	OPT_ABSENT,	delete_all_entries, 0, /* PVT */
	"update_entry",		OPT_REQ,	update_entry, 0, /* menu */
	"update_temp",		OPT_OPTIONAL,	update_temp, 0,	/* reboot */
	"upgrade",		OPT_ABSENT,	upgrade_menu, 0, /* menu */
	NULL,			0,		NULL, 0	/* must be last */
};

/* Archive related sub commands */
static subcmd_defn_t arch_subcmds[] = {
	"update",		OPT_ABSENT,	update_archive, 0, /* PUB */
	"update_all",		OPT_ABSENT,	update_all, 0,	/* PVT */
	"list",			OPT_OPTIONAL,	list_archive, 1, /* PUB */
	NULL,			0,		NULL, 0	/* must be last */
};

static struct {
	nvlist_t *new_nvlp;
	nvlist_t *old_nvlp;
	int need_update;
} walk_arg;


struct safefile {
	char *name;
	struct safefile *next;
};

static struct safefile *safefiles = NULL;
#define	NEED_UPDATE_FILE "/etc/svc/volatile/boot_archive_needs_update"

static void
usage(void)
{
	(void) fprintf(stderr, "USAGE:\n");


	/* archive usage */
	(void) fprintf(stderr,
	    "\t%s update-archive [-vn] [-R altroot [-p platform>]]\n", prog);
	(void) fprintf(stderr,
	    "\t%s list-archive [-R altroot [-p platform>]]\n", prog);
#if !defined(_OPB)
	/* x86 only */
	(void) fprintf(stderr, "\t%s set-menu [-R altroot] key=value\n", prog);
	(void) fprintf(stderr, "\t%s list-menu [-R altroot]\n", prog);
#endif
}

int
main(int argc, char *argv[])
{
	error_t ret;

	(void) setlocale(LC_ALL, "");
	(void) textdomain(TEXT_DOMAIN);

	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		prog++;
	}

	INJECT_ERROR1("ASSERT_ON", assert(0))

	/*
	 * Don't depend on caller's umask
	 */
	(void) umask(0022);

	parse_args(argc, argv);

	switch (bam_cmd) {
		case BAM_MENU:
			ret = bam_menu(bam_subcmd, bam_opt, bam_argc, bam_argv);
			break;
		case BAM_ARCHIVE:
			ret = bam_archive(bam_subcmd, bam_opt);
			break;
		default:
			usage();
			bam_exit(1);
	}

	if (ret != BAM_SUCCESS)
		bam_exit(1);

	bam_unlock();
	return (0);
}

/*
 * Equivalence of public and internal commands:
 *	update-archive  -- -a update
 *	list-archive	-- -a list
 *	set-menu	-- -m set_option
 *	list-menu	-- -m list_entry
 *	update-menu	-- -m update_entry
 */
static struct cmd_map {
	char *bam_cmdname;
	int bam_cmd;
	char *bam_subcmd;
} cmd_map[] = {
	{ "update-archive",	BAM_ARCHIVE,	"update"},
	{ "list-archive",	BAM_ARCHIVE,	"list"},
	{ "set-menu",		BAM_MENU,	"set_option"},
	{ "list-menu",		BAM_MENU,	"list_entry"},
	{ "update-menu",	BAM_MENU,	"update_entry"},
	{ NULL,			0,		NULL}
};

/*
 * Commands syntax published in bootadm(1M) are parsed here
 */
static void
parse_args(int argc, char *argv[])
{
	struct cmd_map *cmp = cmd_map;

	/* command conforming to the final spec */
	if (argc > 1 && argv[1][0] != '-') {
		/*
		 * Map commands to internal table.
		 */
		while (cmp->bam_cmdname) {
			if (strcmp(argv[1], cmp->bam_cmdname) == 0) {
				bam_cmd = cmp->bam_cmd;
				bam_subcmd = cmp->bam_subcmd;
				break;
			}
			cmp++;
		}
		if (cmp->bam_cmdname == NULL) {
			usage();
			bam_exit(1);
		}
		argc--;
		argv++;
	}

	parse_args_internal(argc, argv);
}

/*
 * A combination of public and private commands are parsed here.
 * The internal syntax and the corresponding functionality are:
 *	-a update	-- update-archive
 *	-a list		-- list-archive
 *	-a update-all	-- (reboot to sync all mounted OS archive)
 *	-m update_entry	-- update-menu
 *	-m list_entry	-- list-menu
 *	-m update_temp	-- (reboot -- [boot-args])
 *	-m delete_all_entries -- (called from install)
 */
static void
parse_args_internal(int argc, char *argv[])
{
	int c, error;
	extern char *optarg;
	extern int optind, opterr;

	/* Suppress error message from getopt */
	opterr = 0;

	error = 0;
	while ((c = getopt(argc, argv, "a:d:fm:no:vCR:p:Z")) != -1) {
		switch (c) {
		case 'a':
			if (bam_cmd) {
				error = 1;
				bam_error(MULT_CMDS, c);
			}
			bam_cmd = BAM_ARCHIVE;
			bam_subcmd = optarg;
			break;
		case 'd':
			if (bam_debug) {
				error = 1;
				bam_error(DUP_OPT, c);
			}
			bam_debug = s_strtol(optarg);
			break;
		case 'f':
			if (bam_force) {
				error = 1;
				bam_error(DUP_OPT, c);
			}
			bam_force = 1;
			break;
		case 'm':
			if (bam_cmd) {
				error = 1;
				bam_error(MULT_CMDS, c);
			}
			bam_cmd = BAM_MENU;
			bam_subcmd = optarg;
			break;
		case 'n':
			if (bam_check) {
				error = 1;
				bam_error(DUP_OPT, c);
			}
			bam_check = 1;
			break;
		case 'o':
			if (bam_opt) {
				error = 1;
				bam_error(DUP_OPT, c);
			}
			bam_opt = optarg;
			break;
		case 'v':
			if (bam_verbose) {
				error = 1;
				bam_error(DUP_OPT, c);
			}
			bam_verbose = 1;
			break;
		case 'C':
			bam_smf_check = 1;
			break;
		case 'R':
			if (bam_root) {
				error = 1;
				bam_error(DUP_OPT, c);
				break;
			} else if (realpath(optarg, rootbuf) == NULL) {
				error = 1;
				bam_error(CANT_RESOLVE, optarg,
				    strerror(errno));
				break;
			}
			bam_alt_root = 1;
			bam_root = rootbuf;
			bam_rootlen = strlen(rootbuf);
			break;
		case 'p':
			bam_alt_platform = 1;
			bam_platform = optarg;
			if ((strcmp(bam_platform, "i86pc") != 0) &&
			    (strcmp(bam_platform, "sun4u") != 0) &&
			    (strcmp(bam_platform, "sun4v") != 0)) {
				error = 1;
				bam_error(INVALID_PLAT, bam_platform);
			}
			break;
		case 'Z':
			bam_zfs = 1;
			break;
		case '?':
			error = 1;
			bam_error(BAD_OPT, optopt);
			break;
		default :
			error = 1;
			bam_error(BAD_OPT, c);
			break;
		}
	}

	/*
	 * An alternate platform requires an alternate root
	 */
	if (bam_alt_platform && bam_alt_root == 0) {
		usage();
		bam_exit(0);
	}

	/*
	 * A command option must be specfied
	 */
	if (!bam_cmd) {
		if (bam_opt && strcmp(bam_opt, "all") == 0) {
			usage();
			bam_exit(0);
		}
		bam_error(NEED_CMD);
		error = 1;
	}

	if (error) {
		usage();
		bam_exit(1);
	}

	if (optind > argc) {
		bam_error(INT_ERROR, "parse_args");
		bam_exit(1);
	} else if (optind < argc) {
		bam_argv = &argv[optind];
		bam_argc = argc - optind;
	}

	/*
	 * -n implies verbose mode
	 */
	if (bam_check)
		bam_verbose = 1;
}

static error_t
check_subcmd_and_options(
	char *subcmd,
	char *opt,
	subcmd_defn_t *table,
	error_t (**fp)())
{
	int i;

	if (subcmd == NULL) {
		bam_error(NEED_SUBCMD);
		return (BAM_ERROR);
	}

	if (strcmp(subcmd, "set_option") == 0) {
		if (bam_argc == 0 || bam_argv == NULL || bam_argv[0] == NULL) {
			bam_error(MISSING_ARG);
			usage();
			return (BAM_ERROR);
		} else if (bam_argc > 1 || bam_argv[1] != NULL) {
			bam_error(TRAILING_ARGS);
			usage();
			return (BAM_ERROR);
		}
	} else if (bam_argc || bam_argv) {
		bam_error(TRAILING_ARGS);
		usage();
		return (BAM_ERROR);
	}

	if (bam_root == NULL) {
		bam_root = rootbuf;
		bam_rootlen = 1;
	}

	/* verify that subcmd is valid */
	for (i = 0; table[i].subcmd != NULL; i++) {
		if (strcmp(table[i].subcmd, subcmd) == 0)
			break;
	}

	if (table[i].subcmd == NULL) {
		bam_error(INVALID_SUBCMD, subcmd);
		return (BAM_ERROR);
	}

	if (table[i].unpriv == 0 && geteuid() != 0) {
		bam_error(MUST_BE_ROOT);
		return (BAM_ERROR);
	}

	/*
	 * Currently only privileged commands need a lock
	 */
	if (table[i].unpriv == 0)
		bam_lock();

	/* subcmd verifies that opt is appropriate */
	if (table[i].option != OPT_OPTIONAL) {
		if ((table[i].option == OPT_REQ) ^ (opt != NULL)) {
			if (opt)
				bam_error(NO_OPT_REQ, subcmd);
			else
				bam_error(MISS_OPT, subcmd);
			return (BAM_ERROR);
		}
	}

	*fp = table[i].handler;

	return (BAM_SUCCESS);
}

/*
 * NOTE: A single "/" is also considered a trailing slash and will
 * be deleted.
 */
static void
elide_trailing_slash(const char *src, char *dst, size_t dstsize)
{
	size_t dstlen;

	assert(src);
	assert(dst);

	(void) strlcpy(dst, src, dstsize);

	dstlen = strlen(dst);
	if (dst[dstlen - 1] == '/') {
		dst[dstlen - 1] = '\0';
	}
}

static error_t
bam_menu(char *subcmd, char *opt, int largc, char *largv[])
{
	error_t			ret;
	char			menu_path[PATH_MAX];
	char			clean_menu_root[PATH_MAX];
	char			path[PATH_MAX];
	menu_t			*menu;
	char			menu_root[PATH_MAX];
	struct stat		sb;
	error_t (*f)(menu_t *mp, char *menu_path, char *opt);
	char			*special;
	char			*pool = NULL;
	zfs_mnted_t		zmnted;
	char			*zmntpt;
	char			*osdev;
	char			*osroot;
	const char		*fcn = "bam_menu()";

	/*
	 * Menu sub-command only applies to GRUB (i.e. x86)
	 */
	if (!is_grub(bam_alt_root ? bam_root : "/")) {
		bam_error(NOT_GRUB_BOOT);
		return (BAM_ERROR);
	}

	/*
	 * Check arguments
	 */
	ret = check_subcmd_and_options(subcmd, opt, menu_subcmds, &f);
	if (ret == BAM_ERROR) {
		return (BAM_ERROR);
	}

	assert(bam_root);

	(void) strlcpy(menu_root, bam_root, sizeof (menu_root));
	osdev = osroot = NULL;

	if (strcmp(subcmd, "update_entry") == 0) {
		assert(opt);

		osdev = strtok(opt, ",");
		assert(osdev);
		osroot = strtok(NULL, ",");
		if (osroot) {
			/* fixup bam_root so that it points at osroot */
			if (realpath(osroot, rootbuf) == NULL) {
				bam_error(CANT_RESOLVE, osroot,
				    strerror(errno));
				return (BAM_ERROR);
			}
			bam_alt_root = 1;
			bam_root  = rootbuf;
			bam_rootlen = strlen(rootbuf);
		}
	}

	/*
	 * We support menu on PCFS (under certain conditions), but
	 * not the OS root
	 */
	if (is_pcfs(bam_root)) {
		bam_error(PCFS_ROOT_NOTSUP, bam_root);
		return (BAM_ERROR);
	}

	if (stat(menu_root, &sb) == -1) {
		bam_error(CANNOT_LOCATE_GRUB_MENU);
		return (BAM_ERROR);
	}

	BAM_DPRINTF((D_MENU_ROOT, fcn, menu_root));

	/*
	 * We no longer use the GRUB slice file. If it exists, then
	 * the user is doing something that is unsupported (such as
	 * standard upgrading an old Live Upgrade BE). If that
	 * happens, mimic existing behavior i.e. pretend that it is
	 * not a BE. Emit a warning though.
	 */
	if (bam_alt_root) {
		(void) snprintf(path, sizeof (path), "%s%s", bam_root,
		    GRUB_slice);
	} else {
		(void) snprintf(path, sizeof (path), "%s", GRUB_slice);
	}

	if (stat(path, &sb) == 0)
		bam_error(GRUB_SLICE_FILE_EXISTS, path);

	if (is_zfs(menu_root)) {
		assert(strcmp(menu_root, bam_root) == 0);
		special = get_special(menu_root);
		INJECT_ERROR1("Z_MENU_GET_SPECIAL", special = NULL);
		if (special == NULL) {
			bam_error(CANT_FIND_SPECIAL, menu_root);
			return (BAM_ERROR);
		}
		pool = strtok(special, "/");
		INJECT_ERROR1("Z_MENU_GET_POOL", pool = NULL);
		if (pool == NULL) {
			free(special);
			bam_error(CANT_FIND_POOL, menu_root);
			return (BAM_ERROR);
		}
		BAM_DPRINTF((D_Z_MENU_GET_POOL_FROM_SPECIAL, fcn, pool));

		zmntpt = mount_top_dataset(pool, &zmnted);
		INJECT_ERROR1("Z_MENU_MOUNT_TOP_DATASET", zmntpt = NULL);
		if (zmntpt == NULL) {
			bam_error(CANT_MOUNT_POOL_DATASET, pool);
			free(special);
			return (BAM_ERROR);
		}
		BAM_DPRINTF((D_Z_GET_MENU_MOUNT_TOP_DATASET, fcn, zmntpt));

		(void) strlcpy(menu_root, zmntpt, sizeof (menu_root));
		BAM_DPRINTF((D_Z_GET_MENU_MENU_ROOT, fcn, menu_root));
	}

	elide_trailing_slash(menu_root, clean_menu_root,
	    sizeof (clean_menu_root));

	BAM_DPRINTF((D_CLEAN_MENU_ROOT, fcn, clean_menu_root));

	(void) strlcpy(menu_path, clean_menu_root, sizeof (menu_path));
	(void) strlcat(menu_path, GRUB_MENU, sizeof (menu_path));

	BAM_DPRINTF((D_MENU_PATH, fcn, menu_path));

	/*
	 * If listing the menu, display the menu location
	 */
	if (strcmp(subcmd, "list_entry") == 0) {
		bam_print(GRUB_MENU_PATH, menu_path);
	}


	menu = menu_read(menu_path);
	assert(menu);

	/*
	 * We already checked the following case in
	 * check_subcmd_and_suboptions() above. Complete the
	 * final step now.
	 */
	if (strcmp(subcmd, "set_option") == 0) {
		assert(largc == 1 && largv[0] && largv[1] == NULL);
		opt = largv[0];
	} else {
		assert(largc == 0 && largv == NULL);
	}

	ret = get_boot_cap(bam_root);
	if (ret != BAM_SUCCESS) {
		BAM_DPRINTF((D_BOOT_GET_CAP_FAILED, fcn));
		goto out;
	}

	/*
	 * Once the sub-cmd handler has run
	 * only the line field is guaranteed to have valid values
	 */
	if (strcmp(subcmd, "update_entry") == 0)
		ret = f(menu, menu_root, osdev);
	else if (strcmp(subcmd, "upgrade") == 0)
		ret = f(menu, bam_root, menu_root);
	else if (strcmp(subcmd, "list_entry") == 0)
		ret = f(menu, menu_path, opt);
	else
		ret = f(menu, NULL, opt);

	if (ret == BAM_WRITE) {
		BAM_DPRINTF((D_WRITING_MENU_ROOT, fcn, clean_menu_root));
		ret = menu_write(clean_menu_root, menu);
	}

out:
	INJECT_ERROR1("POOL_SET", pool = "/pooldata");
	assert((is_zfs(menu_root)) ^ (pool == NULL));
	if (pool) {
		(void) umount_top_dataset(pool, zmnted, zmntpt);
		free(special);
	}
	menu_free(menu);
	return (ret);
}


static error_t
bam_archive(
	char *subcmd,
	char *opt)
{
	error_t			ret;
	error_t			(*f)(char *root, char *opt);
	const char		*fcn = "bam_archive()";

	/*
	 * Add trailing / for archive subcommands
	 */
	if (rootbuf[strlen(rootbuf) - 1] != '/')
		(void) strcat(rootbuf, "/");
	bam_rootlen = strlen(rootbuf);

	/*
	 * Check arguments
	 */
	ret = check_subcmd_and_options(subcmd, opt, arch_subcmds, &f);
	if (ret != BAM_SUCCESS) {
		return (BAM_ERROR);
	}

	ret = get_boot_cap(rootbuf);
	if (ret != BAM_SUCCESS) {
		BAM_DPRINTF((D_BOOT_GET_CAP_FAILED, fcn));
		return (ret);
	}

	/*
	 * Check archive not supported with update_all
	 * since it is awkward to display out-of-sync
	 * information for each BE.
	 */
	if (bam_check && strcmp(subcmd, "update_all") == 0) {
		bam_error(CHECK_NOT_SUPPORTED, subcmd);
		return (BAM_ERROR);
	}

	if (strcmp(subcmd, "update_all") == 0)
		bam_update_all = 1;

#if !defined(_OPB)
	ucode_install(bam_root);
#endif

	ret = f(bam_root, opt);

	bam_update_all = 0;

	return (ret);
}

/*PRINTFLIKE1*/
void
bam_error(char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	(void) fprintf(stderr, "%s: ", prog);
	(void) vfprintf(stderr, format, ap);
	va_end(ap);
}

/*PRINTFLIKE1*/
void
bam_derror(char *format, ...)
{
	va_list ap;

	assert(bam_debug);

	va_start(ap, format);
	(void) fprintf(stderr, "DEBUG: ");
	(void) vfprintf(stderr, format, ap);
	va_end(ap);
}

/*PRINTFLIKE1*/
void
bam_print(char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	(void) vfprintf(stdout, format, ap);
	va_end(ap);
}

/*PRINTFLIKE1*/
void
bam_print_stderr(char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	(void) vfprintf(stderr, format, ap);
	va_end(ap);
}

static void
bam_exit(int excode)
{
	bam_unlock();
	exit(excode);
}

static void
bam_lock(void)
{
	struct flock lock;
	pid_t pid;

	bam_lock_fd = open(BAM_LOCK_FILE, O_CREAT|O_RDWR, LOCK_FILE_PERMS);
	if (bam_lock_fd < 0) {
		/*
		 * We may be invoked early in boot for archive verification.
		 * In this case, root is readonly and /var/run may not exist.
		 * Proceed without the lock
		 */
		if (errno == EROFS || errno == ENOENT) {
			bam_root_readonly = 1;
			return;
		}

		bam_error(OPEN_FAIL, BAM_LOCK_FILE, strerror(errno));
		bam_exit(1);
	}

	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;

	if (fcntl(bam_lock_fd, F_SETLK, &lock) == -1) {
		if (errno != EACCES && errno != EAGAIN) {
			bam_error(LOCK_FAIL, BAM_LOCK_FILE, strerror(errno));
			(void) close(bam_lock_fd);
			bam_lock_fd = -1;
			bam_exit(1);
		}
		pid = 0;
		(void) pread(bam_lock_fd, &pid, sizeof (pid_t), 0);
		bam_print(FILE_LOCKED, pid);

		lock.l_type = F_WRLCK;
		lock.l_whence = SEEK_SET;
		lock.l_start = 0;
		lock.l_len = 0;
		if (fcntl(bam_lock_fd, F_SETLKW, &lock) == -1) {
			bam_error(LOCK_FAIL, BAM_LOCK_FILE, strerror(errno));
			(void) close(bam_lock_fd);
			bam_lock_fd = -1;
			bam_exit(1);
		}
	}

	/* We own the lock now */
	pid = getpid();
	(void) write(bam_lock_fd, &pid, sizeof (pid));
}

static void
bam_unlock(void)
{
	struct flock unlock;

	/*
	 * NOP if we don't hold the lock
	 */
	if (bam_lock_fd < 0) {
		return;
	}

	unlock.l_type = F_UNLCK;
	unlock.l_whence = SEEK_SET;
	unlock.l_start = 0;
	unlock.l_len = 0;

	if (fcntl(bam_lock_fd, F_SETLK, &unlock) == -1) {
		bam_error(UNLOCK_FAIL, BAM_LOCK_FILE, strerror(errno));
	}

	if (close(bam_lock_fd) == -1) {
		bam_error(CLOSE_FAIL, BAM_LOCK_FILE, strerror(errno));
	}
	bam_lock_fd = -1;
}

static error_t
list_archive(char *root, char *opt)
{
	filelist_t flist;
	filelist_t *flistp = &flist;
	line_t *lp;

	assert(root);
	assert(opt == NULL);

	flistp->head = flistp->tail = NULL;
	if (read_list(root, flistp) != BAM_SUCCESS) {
		return (BAM_ERROR);
	}
	assert(flistp->head && flistp->tail);

	for (lp = flistp->head; lp; lp = lp->next) {
		bam_print(PRINT, lp->line);
	}

	filelist_free(flistp);

	return (BAM_SUCCESS);
}

/*
 * This routine writes a list of lines to a file.
 * The list is *not* freed
 */
static error_t
list2file(char *root, char *tmp, char *final, line_t *start)
{
	char		tmpfile[PATH_MAX];
	char		path[PATH_MAX];
	FILE		*fp;
	int		ret;
	struct stat	sb;
	mode_t		mode;
	uid_t		root_uid;
	gid_t		sys_gid;
	struct passwd	*pw;
	struct group	*gp;
	const char	*fcn = "list2file()";

	(void) snprintf(path, sizeof (path), "%s%s", root, final);

	if (start == NULL) {
		/* Empty GRUB menu */
		if (stat(path, &sb) != -1) {
			bam_print(UNLINK_EMPTY, path);
			if (unlink(path) != 0) {
				bam_error(UNLINK_FAIL, path, strerror(errno));
				return (BAM_ERROR);
			} else {
				return (BAM_SUCCESS);
			}
		}
		return (BAM_SUCCESS);
	}

	/*
	 * Preserve attributes of existing file if possible,
	 * otherwise ask the system for uid/gid of root/sys.
	 * If all fails, fall back on hard-coded defaults.
	 */
	if (stat(path, &sb) != -1) {
		mode = sb.st_mode;
		root_uid = sb.st_uid;
		sys_gid = sb.st_gid;
	} else {
		mode = DEFAULT_DEV_MODE;
		if ((pw = getpwnam(DEFAULT_DEV_USER)) != NULL) {
			root_uid = pw->pw_uid;
		} else {
			bam_error(CANT_FIND_USER,
			    DEFAULT_DEV_USER, DEFAULT_DEV_UID);
			root_uid = (uid_t)DEFAULT_DEV_UID;
		}
		if ((gp = getgrnam(DEFAULT_DEV_GROUP)) != NULL) {
			sys_gid = gp->gr_gid;
		} else {
			bam_error(CANT_FIND_GROUP,
			    DEFAULT_DEV_GROUP, DEFAULT_DEV_GID);
			sys_gid = (gid_t)DEFAULT_DEV_GID;
		}
	}

	(void) snprintf(tmpfile, sizeof (tmpfile), "%s%s", root, tmp);

	/* Truncate tmpfile first */
	fp = fopen(tmpfile, "w");
	if (fp == NULL) {
		bam_error(OPEN_FAIL, tmpfile, strerror(errno));
		return (BAM_ERROR);
	}
	ret = fclose(fp);
	INJECT_ERROR1("LIST2FILE_TRUNC_FCLOSE", ret = EOF);
	if (ret == EOF) {
		bam_error(CLOSE_FAIL, tmpfile, strerror(errno));
		return (BAM_ERROR);
	}

	/* Now open it in append mode */
	fp = fopen(tmpfile, "a");
	if (fp == NULL) {
		bam_error(OPEN_FAIL, tmpfile, strerror(errno));
		return (BAM_ERROR);
	}

	for (; start; start = start->next) {
		ret = s_fputs(start->line, fp);
		INJECT_ERROR1("LIST2FILE_FPUTS", ret = EOF);
		if (ret == EOF) {
			bam_error(WRITE_FAIL, tmpfile, strerror(errno));
			(void) fclose(fp);
			return (BAM_ERROR);
		}
	}

	ret = fclose(fp);
	INJECT_ERROR1("LIST2FILE_APPEND_FCLOSE", ret = EOF);
	if (ret == EOF) {
		bam_error(CLOSE_FAIL, tmpfile, strerror(errno));
		return (BAM_ERROR);
	}

	/*
	 * Set up desired attributes.  Ignore failures on filesystems
	 * not supporting these operations - pcfs reports unsupported
	 * operations as EINVAL.
	 */
	ret = chmod(tmpfile, mode);
	if (ret == -1 &&
	    errno != EINVAL && errno != ENOTSUP) {
		bam_error(CHMOD_FAIL, tmpfile, strerror(errno));
		return (BAM_ERROR);
	}

	ret = chown(tmpfile, root_uid, sys_gid);
	if (ret == -1 &&
	    errno != EINVAL && errno != ENOTSUP) {
		bam_error(CHOWN_FAIL, tmpfile, strerror(errno));
		return (BAM_ERROR);
	}


	/*
	 * Do an atomic rename
	 */
	ret = rename(tmpfile, path);
	INJECT_ERROR1("LIST2FILE_RENAME", ret = -1);
	if (ret != 0) {
		bam_error(RENAME_FAIL, path, strerror(errno));
		return (BAM_ERROR);
	}

	BAM_DPRINTF((D_WROTE_FILE, fcn, path));
	return (BAM_SUCCESS);
}

/*
 * This function should always return 0 - since we want
 * to create stat data for *all* files in the list.
 */
/*ARGSUSED*/
static int
cmpstat(
	const char *file,
	const struct stat *stat,
	int flags,
	struct FTW *ftw)
{
	uint_t sz;
	uint64_t *value;
	uint64_t filestat[2];
	int error;

	struct safefile *safefilep;
	FILE *fp;

	/*
	 * We only want regular files
	 */
	if (!S_ISREG(stat->st_mode))
		return (0);

	/*
	 * new_nvlp may be NULL if there were errors earlier
	 * but this is not fatal to update determination.
	 */
	if (walk_arg.new_nvlp) {
		filestat[0] = stat->st_size;
		filestat[1] = stat->st_mtime;
		error = nvlist_add_uint64_array(walk_arg.new_nvlp,
		    file + bam_rootlen, filestat, 2);
		if (error)
			bam_error(NVADD_FAIL, file, strerror(error));
	}

	/*
	 * The remaining steps are only required if we haven't made a
	 * decision about update or if we are checking (-n)
	 */
	if (walk_arg.need_update && !bam_check)
		return (0);

	/*
	 * If we are invoked as part of system/filesystem/boot-archive, then
	 * there are a number of things we should not worry about
	 */
	if (bam_smf_check) {
		/* ignore amd64 modules unless we are booted amd64. */
		if (!is_amd64() && strstr(file, "/amd64/") != 0)
			return (0);

		/* read in list of safe files */
		if (safefiles == NULL)
			if (fp = fopen("/boot/solaris/filelist.safe", "r")) {
				safefiles = s_calloc(1,
				    sizeof (struct safefile));
				safefilep = safefiles;
				safefilep->name = s_calloc(1, MAXPATHLEN +
				    MAXNAMELEN);
				safefilep->next = NULL;
				while (s_fgets(safefilep->name, MAXPATHLEN +
				    MAXNAMELEN, fp) != NULL) {
					safefilep->next = s_calloc(1,
					    sizeof (struct safefile));
					safefilep = safefilep->next;
					safefilep->name = s_calloc(1,
					    MAXPATHLEN + MAXNAMELEN);
					safefilep->next = NULL;
				}
				(void) fclose(fp);
			}
	}

	/*
	 * We need an update if file doesn't exist in old archive
	 */
	if (walk_arg.old_nvlp == NULL ||
	    nvlist_lookup_uint64_array(walk_arg.old_nvlp,
	    file + bam_rootlen, &value, &sz) != 0) {
		if (bam_smf_check)	/* ignore new during smf check */
			return (0);
		walk_arg.need_update = 1;
		if (bam_verbose)
			bam_print(PARSEABLE_NEW_FILE, file);
		return (0);
	}

	/*
	 * File exists in old archive. Check if file has changed
	 */
	assert(sz == 2);
	bcopy(value, filestat, sizeof (filestat));

	if (filestat[0] != stat->st_size ||
	    filestat[1] != stat->st_mtime) {
		if (bam_smf_check) {
			safefilep = safefiles;
			while (safefilep != NULL) {
				if (strcmp(file + bam_rootlen,
				    safefilep->name) == 0) {
					(void) creat(NEED_UPDATE_FILE, 0644);
					return (0);
				}
				safefilep = safefilep->next;
			}
		}
		walk_arg.need_update = 1;
		if (bam_verbose)
			if (bam_smf_check)
				bam_print("    %s\n", file);
			else
				bam_print(PARSEABLE_OUT_DATE, file);
	}

	return (0);
}

/*
 * Check flags and presence of required files.
 * The force flag and/or absence of files should
 * trigger an update.
 * Suppress stdout output if check (-n) option is set
 * (as -n should only produce parseable output.)
 */
static void
check_flags_and_files(char *root)
{
	char path[PATH_MAX];
	struct stat sb;

	/*
	 * if force, create archive unconditionally
	 */
	if (bam_force) {
		walk_arg.need_update = 1;
		if (bam_verbose && !bam_check)
			bam_print(UPDATE_FORCE);
		return;
	}

	/*
	 * If archive is missing, create archive
	 */
	if (is_sparc()) {
		(void) snprintf(path, sizeof (path), "%s%s%s%s", root,
		    ARCHIVE_PREFIX, get_machine(), ARCHIVE_SUFFIX);
	} else {
		if (bam_direct == BAM_DIRECT_DBOOT) {
			(void) snprintf(path, sizeof (path), "%s%s", root,
			    DIRECT_BOOT_ARCHIVE_64);
			if (stat(path, &sb) != 0) {
				if (bam_verbose && !bam_check)
					bam_print(UPDATE_ARCH_MISS, path);
				walk_arg.need_update = 1;
				return;
			}
		}
		(void) snprintf(path, sizeof (path), "%s%s", root,
		    DIRECT_BOOT_ARCHIVE_32);
	}

	if (stat(path, &sb) != 0) {
		if (bam_verbose && !bam_check)
			bam_print(UPDATE_ARCH_MISS, path);
		walk_arg.need_update = 1;
		return;
	}
}

static error_t
read_one_list(char *root, filelist_t  *flistp, char *filelist)
{
	char path[PATH_MAX];
	FILE *fp;
	char buf[BAM_MAXLINE];
	const char *fcn = "read_one_list()";

	(void) snprintf(path, sizeof (path), "%s%s", root, filelist);

	fp = fopen(path, "r");
	if (fp == NULL) {
		BAM_DPRINTF((D_FLIST_FAIL, fcn, path, strerror(errno)));
		return (BAM_ERROR);
	}
	while (s_fgets(buf, sizeof (buf), fp) != NULL) {
		/* skip blank lines */
		if (strspn(buf, " \t") == strlen(buf))
			continue;
		append_to_flist(flistp, buf);
	}
	if (fclose(fp) != 0) {
		bam_error(CLOSE_FAIL, path, strerror(errno));
		return (BAM_ERROR);
	}
	return (BAM_SUCCESS);
}

static error_t
read_list(char *root, filelist_t  *flistp)
{
	char path[PATH_MAX];
	char cmd[PATH_MAX];
	struct stat sb;
	int n, rval;
	const char *fcn = "read_list()";

	flistp->head = flistp->tail = NULL;

	/*
	 * build and check path to extract_boot_filelist.ksh
	 */
	n = snprintf(path, sizeof (path), "%s%s", root, EXTRACT_BOOT_FILELIST);
	if (n >= sizeof (path)) {
		bam_error(NO_FLIST);
		return (BAM_ERROR);
	}

	/*
	 * If extract_boot_filelist is present, exec it, otherwise read
	 * the filelists directly, for compatibility with older images.
	 */
	if (stat(path, &sb) == 0) {
		/*
		 * build arguments to exec extract_boot_filelist.ksh
		 */
		char *rootarg, *platarg;
		int platarglen = 1, rootarglen = 1;
		if (strlen(root) > 1)
			rootarglen += strlen(root) + strlen("-R ");
		if (bam_alt_platform)
			platarglen += strlen(bam_platform) + strlen("-p ");
		platarg = s_calloc(1, platarglen);
		rootarg = s_calloc(1, rootarglen);
		*platarg = 0;
		*rootarg = 0;

		if (strlen(root) > 1) {
			(void) snprintf(rootarg, rootarglen,
			    "-R %s", root);
		}
		if (bam_alt_platform) {
			(void) snprintf(platarg, platarglen,
			    "-p %s", bam_platform);
		}
		n = snprintf(cmd, sizeof (cmd), "%s %s %s /%s /%s",
		    path, rootarg, platarg, BOOT_FILE_LIST, ETC_FILE_LIST);
		free(platarg);
		free(rootarg);
		if (n >= sizeof (cmd)) {
			bam_error(NO_FLIST);
			return (BAM_ERROR);
		}
		if (exec_cmd(cmd, flistp) != 0) {
			BAM_DPRINTF((D_FLIST_FAIL, fcn, path, strerror(errno)));
			return (BAM_ERROR);
		}
	} else {
		/*
		 * Read current lists of files - only the first is mandatory
		 */
		rval = read_one_list(root, flistp, BOOT_FILE_LIST);
		if (rval != BAM_SUCCESS)
			return (rval);
		(void) read_one_list(root, flistp, ETC_FILE_LIST);
	}

	if (flistp->head == NULL) {
		bam_error(NO_FLIST);
		return (BAM_ERROR);
	}

	return (BAM_SUCCESS);
}

static void
getoldstat(char *root)
{
	char path[PATH_MAX];
	int fd, error;
	struct stat sb;
	char *ostat;

	(void) snprintf(path, sizeof (path), "%s%s", root, FILE_STAT);
	fd = open(path, O_RDONLY);
	if (fd == -1) {
		if (bam_verbose)
			bam_print(OPEN_FAIL, path, strerror(errno));
		walk_arg.need_update = 1;
		return;
	}

	if (fstat(fd, &sb) != 0) {
		bam_error(STAT_FAIL, path, strerror(errno));
		(void) close(fd);
		walk_arg.need_update = 1;
		return;
	}

	ostat = s_calloc(1, sb.st_size);

	if (read(fd, ostat, sb.st_size) != sb.st_size) {
		bam_error(READ_FAIL, path, strerror(errno));
		(void) close(fd);
		free(ostat);
		walk_arg.need_update = 1;
		return;
	}

	(void) close(fd);

	walk_arg.old_nvlp = NULL;
	error = nvlist_unpack(ostat, sb.st_size, &walk_arg.old_nvlp, 0);

	free(ostat);

	if (error) {
		bam_error(UNPACK_FAIL, path, strerror(error));
		walk_arg.old_nvlp = NULL;
		walk_arg.need_update = 1;
		return;
	}
}

/*
 * Checks if a file in the current (old) archive has
 * been deleted from the root filesystem. This is needed for
 * software like Trusted Extensions (TX) that switch early
 * in boot based on presence/absence of a kernel module.
 */
static void
check4stale(char *root)
{
	nvpair_t	*nvp;
	nvlist_t	*nvlp;
	char 		*file;
	char		path[PATH_MAX];
	struct stat	sb;

	/*
	 * Skip stale file check during smf check
	 */
	if (bam_smf_check)
		return;

	/* Nothing to do if no old stats */
	if ((nvlp = walk_arg.old_nvlp) == NULL)
		return;

	for (nvp = nvlist_next_nvpair(nvlp, NULL); nvp;
	    nvp = nvlist_next_nvpair(nvlp, nvp)) {
		file = nvpair_name(nvp);
		if (file == NULL)
			continue;
		(void) snprintf(path, sizeof (path), "%s/%s",
		    root, file);
		if (stat(path, &sb) == -1) {
			walk_arg.need_update = 1;
			if (bam_verbose)
				bam_print(PARSEABLE_STALE_FILE, path);
		}
	}
}

static void
create_newstat(void)
{
	int error;

	error = nvlist_alloc(&walk_arg.new_nvlp, NV_UNIQUE_NAME, 0);
	if (error) {
		/*
		 * Not fatal - we can still create archive
		 */
		walk_arg.new_nvlp = NULL;
		bam_error(NVALLOC_FAIL, strerror(error));
	}
}

static void
walk_list(char *root, filelist_t *flistp)
{
	char path[PATH_MAX];
	line_t *lp;

	for (lp = flistp->head; lp; lp = lp->next) {
		/*
		 * Don't follow symlinks.  A symlink must refer to
		 * a file that would appear in the archive through
		 * a direct reference.  This matches the archive
		 * construction behavior.
		 */
		(void) snprintf(path, sizeof (path), "%s%s", root, lp->line);
		if (nftw(path, cmpstat, 20, FTW_PHYS) == -1) {
			/*
			 * Some files may not exist.
			 * For example: etc/rtc_config on a x86 diskless system
			 * Emit verbose message only
			 */
			if (bam_verbose)
				bam_print(NFTW_FAIL, path, strerror(errno));
		}
	}
}

static void
savenew(char *root)
{
	char path[PATH_MAX];
	char path2[PATH_MAX];
	size_t sz;
	char *nstat;
	int fd, wrote, error;

	nstat = NULL;
	sz = 0;
	error = nvlist_pack(walk_arg.new_nvlp, &nstat, &sz,
	    NV_ENCODE_XDR, 0);
	if (error) {
		bam_error(PACK_FAIL, strerror(error));
		return;
	}

	(void) snprintf(path, sizeof (path), "%s%s", root, FILE_STAT_TMP);
	fd = open(path, O_RDWR|O_CREAT|O_TRUNC, FILE_STAT_MODE);
	if (fd == -1) {
		bam_error(OPEN_FAIL, path, strerror(errno));
		free(nstat);
		return;
	}
	wrote = write(fd, nstat, sz);
	if (wrote != sz) {
		bam_error(WRITE_FAIL, path, strerror(errno));
		(void) close(fd);
		free(nstat);
		return;
	}
	(void) close(fd);
	free(nstat);

	(void) snprintf(path2, sizeof (path2), "%s%s", root, FILE_STAT);
	if (rename(path, path2) != 0) {
		bam_error(RENAME_FAIL, path2, strerror(errno));
	}
}

static void
clear_walk_args(void)
{
	if (walk_arg.old_nvlp)
		nvlist_free(walk_arg.old_nvlp);
	if (walk_arg.new_nvlp)
		nvlist_free(walk_arg.new_nvlp);
	walk_arg.need_update = 0;
	walk_arg.old_nvlp = NULL;
	walk_arg.new_nvlp = NULL;
}

/*
 * Returns:
 *	0 - no update necessary
 *	1 - update required.
 *	BAM_ERROR (-1) - An error occurred
 *
 * Special handling for check (-n):
 * ================================
 * The check (-n) option produces parseable output.
 * To do this, we suppress all stdout messages unrelated
 * to out of sync files.
 * All stderr messages are still printed though.
 *
 */
static int
update_required(char *root)
{
	struct stat sb;
	char path[PATH_MAX];
	filelist_t flist;
	filelist_t *flistp = &flist;
	int need_update;

	flistp->head = flistp->tail = NULL;

	walk_arg.need_update = 0;

	/*
	 * Without consulting stat data, check if we need update
	 */
	check_flags_and_files(root);

	/*
	 * In certain deployment scenarios, filestat may not
	 * exist. Ignore it during boot-archive SMF check.
	 */
	if (bam_smf_check) {
		(void) snprintf(path, sizeof (path), "%s%s", root, FILE_STAT);
		if (stat(path, &sb) != 0)
			return (0);
	}

	/*
	 * consult stat data only if we haven't made a decision
	 * about update. If checking (-n) however, we always
	 * need stat data (since we want to compare old and new)
	 */
	if (!walk_arg.need_update || bam_check)
		getoldstat(root);

	/*
	 * Check if the archive contains files that are no longer
	 * present on the root filesystem.
	 */
	if (!walk_arg.need_update || bam_check)
		check4stale(root);

	/*
	 * read list of files
	 */
	if (read_list(root, flistp) != BAM_SUCCESS) {
		clear_walk_args();
		return (BAM_ERROR);
	}

	assert(flistp->head && flistp->tail);

	/*
	 * At this point either the update is required
	 * or the decision is pending. In either case
	 * we need to create new stat nvlist
	 */
	create_newstat();

	/*
	 * This walk does 2 things:
	 *  	- gets new stat data for every file
	 *	- (optional) compare old and new stat data
	 */
	walk_list(root, &flist);

	/* done with the file list */
	filelist_free(flistp);

	/*
	 * if we didn't succeed in  creating new stat data above
	 * just return result of update check so that archive is built.
	 */
	if (walk_arg.new_nvlp == NULL) {
		bam_error(NO_NEW_STAT);
		need_update = walk_arg.need_update;
		clear_walk_args();
		return (need_update ? 1 : 0);
	}


	/*
	 * If no update required, discard newstat
	 */
	if (!walk_arg.need_update) {
		clear_walk_args();
		return (0);
	}

	return (1);
}

static error_t
create_ramdisk(char *root)
{
	char *cmdline, path[PATH_MAX];
	size_t len;
	struct stat sb;

	/*
	 * Setup command args for create_ramdisk.ksh
	 */
	(void) snprintf(path, sizeof (path), "%s/%s", root, CREATE_RAMDISK);
	if (stat(path, &sb) != 0) {
		bam_error(ARCH_EXEC_MISS, path, strerror(errno));
		return (BAM_ERROR);
	}

	len = strlen(path) + strlen(root) + 10;	/* room for space + -R */
	if (bam_alt_platform)
		len += strlen(bam_platform) + strlen("-p ");
	cmdline = s_calloc(1, len);

	if (bam_alt_platform) {
		assert(strlen(root) > 1);
		(void) snprintf(cmdline, len, "%s -p %s -R %s",
		    path, bam_platform, root);
		/* chop off / at the end */
		cmdline[strlen(cmdline) - 1] = '\0';
	} else if (strlen(root) > 1) {
		(void) snprintf(cmdline, len, "%s -R %s", path, root);
		/* chop off / at the end */
		cmdline[strlen(cmdline) - 1] = '\0';
	} else
		(void) snprintf(cmdline, len, "%s", path);

	if (exec_cmd(cmdline, NULL) != 0) {
		bam_error(ARCHIVE_FAIL, cmdline);
		free(cmdline);
		return (BAM_ERROR);
	}
	free(cmdline);

	/*
	 * The existence of the expected archives used to be
	 * verified here. This check is done in create_ramdisk as
	 * it needs to be in sync with the altroot operated upon.
	 */

	return (BAM_SUCCESS);
}

/*
 * Checks if target filesystem is on a ramdisk
 * 1 - is miniroot
 * 0 - is not
 * When in doubt assume it is not a ramdisk.
 */
static int
is_ramdisk(char *root)
{
	struct extmnttab mnt;
	FILE *fp;
	int found;
	char mntpt[PATH_MAX];
	char *cp;

	/*
	 * There are 3 situations where creating archive is
	 * of dubious value:
	 *	- create boot_archive on a lofi-mounted boot_archive
	 *	- create it on a ramdisk which is the root filesystem
	 *	- create it on a ramdisk mounted somewhere else
	 * The first is not easy to detect and checking for it is not
	 * worth it.
	 * The other two conditions are handled here
	 */

	fp = fopen(MNTTAB, "r");
	if (fp == NULL) {
		bam_error(OPEN_FAIL, MNTTAB, strerror(errno));
		return (0);
	}

	resetmnttab(fp);

	/*
	 * Remove any trailing / from the mount point
	 */
	(void) strlcpy(mntpt, root, sizeof (mntpt));
	if (strcmp(root, "/") != 0) {
		cp = mntpt + strlen(mntpt) - 1;
		if (*cp == '/')
			*cp = '\0';
	}
	found = 0;
	while (getextmntent(fp, &mnt, sizeof (mnt)) == 0) {
		if (strcmp(mnt.mnt_mountp, mntpt) == 0) {
			found = 1;
			break;
		}
	}

	if (!found) {
		if (bam_verbose)
			bam_error(NOT_IN_MNTTAB, mntpt);
		(void) fclose(fp);
		return (0);
	}

	if (strstr(mnt.mnt_special, RAMDISK_SPECIAL) != NULL) {
		if (bam_verbose)
			bam_error(IS_RAMDISK, bam_root);
		(void) fclose(fp);
		return (1);
	}

	(void) fclose(fp);

	return (0);
}

static int
is_boot_archive(char *root)
{
	char		path[PATH_MAX];
	struct stat	sb;
	int		error;
	const char	*fcn = "is_boot_archive()";

	/*
	 * We can't create an archive without the create_ramdisk script
	 */
	(void) snprintf(path, sizeof (path), "%s/%s", root, CREATE_RAMDISK);
	error = stat(path, &sb);
	INJECT_ERROR1("NOT_ARCHIVE_BASED", error = -1);
	if (error == -1) {
		if (bam_verbose)
			bam_print(FILE_MISS, path);
		BAM_DPRINTF((D_NOT_ARCHIVE_BOOT, fcn, root));
		return (0);
	}

	BAM_DPRINTF((D_IS_ARCHIVE_BOOT, fcn, root));
	return (1);
}

/*
 * Need to call this for anything that operates on the GRUB menu
 * In the x86 live upgrade case the directory /boot/grub may be present
 * even on pre-newboot BEs. The authoritative way to check for a GRUB target
 * is to check for the presence of the stage2 binary which is present
 * only on GRUB targets (even on x86 boot partitions). Checking for the
 * presence of the multiboot binary is not correct as it is not present
 * on x86 boot partitions.
 */
int
is_grub(const char *root)
{
	char path[PATH_MAX];
	struct stat sb;
	const char *fcn = "is_grub()";

	(void) snprintf(path, sizeof (path), "%s%s", root, GRUB_STAGE2);
	if (stat(path, &sb) == -1) {
		BAM_DPRINTF((D_NO_GRUB_DIR, fcn, path));
		return (0);
	}

	return (1);
}

static int
is_zfs(char *root)
{
	struct statvfs		vfs;
	int			ret;
	const char		*fcn = "is_zfs()";

	ret = statvfs(root, &vfs);
	INJECT_ERROR1("STATVFS_ZFS", ret = 1);
	if (ret != 0) {
		bam_error(STATVFS_FAIL, root, strerror(errno));
		return (0);
	}

	if (strncmp(vfs.f_basetype, "zfs", strlen("zfs")) == 0) {
		BAM_DPRINTF((D_IS_ZFS, fcn, root));
		return (1);
	} else {
		BAM_DPRINTF((D_IS_NOT_ZFS, fcn, root));
		return (0);
	}
}

static int
is_ufs(char *root)
{
	struct statvfs		vfs;
	int			ret;
	const char		*fcn = "is_ufs()";

	ret = statvfs(root, &vfs);
	INJECT_ERROR1("STATVFS_UFS", ret = 1);
	if (ret != 0) {
		bam_error(STATVFS_FAIL, root, strerror(errno));
		return (0);
	}

	if (strncmp(vfs.f_basetype, "ufs", strlen("ufs")) == 0) {
		BAM_DPRINTF((D_IS_UFS, fcn, root));
		return (1);
	} else {
		BAM_DPRINTF((D_IS_NOT_UFS, fcn, root));
		return (0);
	}
}

static int
is_pcfs(char *root)
{
	struct statvfs		vfs;
	int			ret;
	const char		*fcn = "is_pcfs()";

	ret = statvfs(root, &vfs);
	INJECT_ERROR1("STATVFS_PCFS", ret = 1);
	if (ret != 0) {
		bam_error(STATVFS_FAIL, root, strerror(errno));
		return (0);
	}

	if (strncmp(vfs.f_basetype, "pcfs", strlen("pcfs")) == 0) {
		BAM_DPRINTF((D_IS_PCFS, fcn, root));
		return (1);
	} else {
		BAM_DPRINTF((D_IS_NOT_PCFS, fcn, root));
		return (0);
	}
}

static int
is_readonly(char *root)
{
	int		fd;
	int		error;
	char		testfile[PATH_MAX];
	const char	*fcn = "is_readonly()";

	/*
	 * Using statvfs() to check for a read-only filesystem is not
	 * reliable. The only way to reliably test is to attempt to
	 * create a file
	 */
	(void) snprintf(testfile, sizeof (testfile), "%s/%s.%d",
	    root, BOOTADM_RDONLY_TEST, getpid());

	(void) unlink(testfile);

	errno = 0;
	fd = open(testfile, O_RDWR|O_CREAT|O_EXCL, 0644);
	error = errno;
	INJECT_ERROR2("RDONLY_TEST_ERROR", fd = -1, error = EACCES);
	if (fd == -1 && error == EROFS) {
		BAM_DPRINTF((D_RDONLY_FS, fcn, root));
		return (1);
	} else if (fd == -1) {
		bam_error(RDONLY_TEST_ERROR, root, strerror(error));
	}

	(void) close(fd);
	(void) unlink(testfile);

	BAM_DPRINTF((D_RDWR_FS, fcn, root));
	return (0);
}

static error_t
update_archive(char *root, char *opt)
{
	error_t ret;

	assert(root);
	assert(opt == NULL);

	/*
	 * root must belong to a boot archive based OS,
	 */
	if (!is_boot_archive(root)) {
		/*
		 * Emit message only if not in context of update_all.
		 * If in update_all, emit only if verbose flag is set.
		 */
		if (!bam_update_all || bam_verbose)
			bam_print(NOT_ARCHIVE_BOOT, root);
		return (BAM_SUCCESS);
	}

	/*
	 * If smf check is requested when / is writable (can happen
	 * on first reboot following an upgrade because service
	 * dependency is messed up), skip the check.
	 */
	if (bam_smf_check && !bam_root_readonly)
		return (BAM_SUCCESS);

	/*
	 * root must be writable. This check applies to alternate
	 * root (-R option); bam_root_readonly applies to '/' only.
	 */
	if (!bam_smf_check && !bam_check && is_readonly(root)) {
		if (bam_verbose)
			bam_print(RDONLY_FS, root);
		return (BAM_SUCCESS);
	}

	/*
	 * Don't generate archive on ramdisk
	 */
	if (is_ramdisk(root)) {
		if (bam_verbose)
			bam_print(SKIP_RAMDISK);
		return (BAM_SUCCESS);
	}

	/*
	 * Now check if updated is really needed
	 */
	ret = update_required(root);

	/*
	 * The check command (-n) is *not* a dry run
	 * It only checks if the archive is in sync.
	 */
	if (bam_check) {
		bam_exit((ret != 0) ? 1 : 0);
	}

	if (ret == 1) {
		/* create the ramdisk */
		ret = create_ramdisk(root);
	}

	/* if the archive is updated, save the new stat data */
	if (ret == 0 && walk_arg.new_nvlp != NULL) {
		savenew(root);
	}

	clear_walk_args();

	return (ret);
}

static error_t
synchronize_BE_menu(void)
{
	struct stat	sb;
	char		cmdline[PATH_MAX];
	char		cksum_line[PATH_MAX];
	filelist_t	flist = {0};
	char		*old_cksum_str;
	char		*old_size_str;
	char		*old_file;
	char		*curr_cksum_str;
	char		*curr_size_str;
	char		*curr_file;
	FILE		*cfp;
	int		found;
	int		ret;
	const char	*fcn = "synchronize_BE_menu()";

	BAM_DPRINTF((D_FUNC_ENTRY0, fcn));

	/* Check if findroot enabled LU BE */
	if (stat(FINDROOT_INSTALLGRUB, &sb) != 0) {
		BAM_DPRINTF((D_NOT_LU_BE, fcn));
		return (BAM_SUCCESS);
	}

	if (stat(LU_MENU_CKSUM, &sb) != 0) {
		BAM_DPRINTF((D_NO_CKSUM_FILE, fcn, LU_MENU_CKSUM));
		goto menu_sync;
	}

	cfp = fopen(LU_MENU_CKSUM, "r");
	INJECT_ERROR1("CKSUM_FILE_MISSING", cfp = NULL);
	if (cfp == NULL) {
		bam_error(CANNOT_READ_LU_CKSUM, LU_MENU_CKSUM);
		goto menu_sync;
	}
	BAM_DPRINTF((D_CKSUM_FILE_OPENED, fcn, LU_MENU_CKSUM));

	found = 0;
	while (s_fgets(cksum_line, sizeof (cksum_line), cfp) != NULL) {
		INJECT_ERROR1("MULTIPLE_CKSUM", found = 1);
		if (found) {
			bam_error(MULTIPLE_LU_CKSUM, LU_MENU_CKSUM);
			(void) fclose(cfp);
			goto menu_sync;
		}
		found = 1;
	}
	BAM_DPRINTF((D_CKSUM_FILE_READ, fcn, LU_MENU_CKSUM));


	old_cksum_str = strtok(cksum_line, " \t");
	old_size_str = strtok(NULL, " \t");
	old_file = strtok(NULL, " \t");

	INJECT_ERROR1("OLD_CKSUM_NULL", old_cksum_str = NULL);
	INJECT_ERROR1("OLD_SIZE_NULL", old_size_str = NULL);
	INJECT_ERROR1("OLD_FILE_NULL", old_file = NULL);
	if (old_cksum_str == NULL || old_size_str == NULL || old_file == NULL) {
		bam_error(CANNOT_PARSE_LU_CKSUM, LU_MENU_CKSUM);
		goto menu_sync;
	}
	BAM_DPRINTF((D_CKSUM_FILE_PARSED, fcn, LU_MENU_CKSUM));

	/* Get checksum of current menu */
	(void) snprintf(cmdline, sizeof (cmdline), "%s %s",
	    CKSUM, GRUB_MENU);
	ret = exec_cmd(cmdline, &flist);
	INJECT_ERROR1("GET_CURR_CKSUM", ret = 1);
	if (ret != 0) {
		bam_error(MENU_CKSUM_FAIL);
		return (BAM_ERROR);
	}
	BAM_DPRINTF((D_CKSUM_GEN_SUCCESS, fcn));

	INJECT_ERROR1("GET_CURR_CKSUM_OUTPUT", flist.head = NULL);
	if ((flist.head == NULL) || (flist.head != flist.tail)) {
		bam_error(BAD_CKSUM);
		filelist_free(&flist);
		return (BAM_ERROR);
	}
	BAM_DPRINTF((D_CKSUM_GEN_OUTPUT_VALID, fcn));

	curr_cksum_str = strtok(flist.head->line, " \t");
	curr_size_str = strtok(NULL, " \t");
	curr_file = strtok(NULL, " \t");

	INJECT_ERROR1("CURR_CKSUM_NULL", curr_cksum_str = NULL);
	INJECT_ERROR1("CURR_SIZE_NULL", curr_size_str = NULL);
	INJECT_ERROR1("CURR_FILE_NULL", curr_file = NULL);
	if (curr_cksum_str == NULL || curr_size_str == NULL ||
	    curr_file == NULL) {
		bam_error(BAD_CKSUM_PARSE);
		filelist_free(&flist);
		return (BAM_ERROR);
	}
	BAM_DPRINTF((D_CKSUM_GEN_PARSED, fcn));

	if (strcmp(old_cksum_str, curr_cksum_str) == 0 &&
	    strcmp(old_size_str, curr_size_str) == 0 &&
	    strcmp(old_file, curr_file) == 0) {
		filelist_free(&flist);
		BAM_DPRINTF((D_CKSUM_NO_CHANGE, fcn));
		return (BAM_SUCCESS);
	}

	filelist_free(&flist);

	/* cksum doesn't match - the menu has changed */
	BAM_DPRINTF((D_CKSUM_HAS_CHANGED, fcn));

menu_sync:
	bam_print(PROP_GRUB_MENU);

	(void) snprintf(cmdline, sizeof (cmdline),
	    "/bin/sh -c '. %s > /dev/null; %s %s yes > /dev/null'",
	    LULIB, LULIB_PROPAGATE_FILE, GRUB_MENU);
	ret = exec_cmd(cmdline, NULL);
	INJECT_ERROR1("PROPAGATE_MENU", ret = 1);
	if (ret != 0) {
		bam_error(MENU_PROP_FAIL);
		return (BAM_ERROR);
	}
	BAM_DPRINTF((D_PROPAGATED_MENU, fcn));

	(void) snprintf(cmdline, sizeof (cmdline), "/bin/cp %s %s > /dev/null",
	    GRUB_MENU, GRUB_BACKUP_MENU);
	ret = exec_cmd(cmdline, NULL);
	INJECT_ERROR1("CREATE_BACKUP", ret = 1);
	if (ret != 0) {
		bam_error(MENU_BACKUP_FAIL, GRUB_BACKUP_MENU);
		return (BAM_ERROR);
	}
	BAM_DPRINTF((D_CREATED_BACKUP, fcn, GRUB_BACKUP_MENU));

	(void) snprintf(cmdline, sizeof (cmdline),
	    "/bin/sh -c '. %s > /dev/null; %s %s no > /dev/null'",
	    LULIB, LULIB_PROPAGATE_FILE, GRUB_BACKUP_MENU);
	ret = exec_cmd(cmdline, NULL);
	INJECT_ERROR1("PROPAGATE_BACKUP", ret = 1);
	if (ret != 0) {
		bam_error(BACKUP_PROP_FAIL, GRUB_BACKUP_MENU);
		return (BAM_ERROR);
	}
	BAM_DPRINTF((D_PROPAGATED_BACKUP, fcn, GRUB_BACKUP_MENU));

	(void) snprintf(cmdline, sizeof (cmdline), "%s %s > %s",
	    CKSUM, GRUB_MENU, LU_MENU_CKSUM);
	ret = exec_cmd(cmdline, NULL);
	INJECT_ERROR1("CREATE_CKSUM_FILE", ret = 1);
	if (ret != 0) {
		bam_error(MENU_CKSUM_WRITE_FAIL, LU_MENU_CKSUM);
		return (BAM_ERROR);
	}
	BAM_DPRINTF((D_CREATED_CKSUM_FILE, fcn, LU_MENU_CKSUM));

	(void) snprintf(cmdline, sizeof (cmdline),
	    "/bin/sh -c '. %s > /dev/null; %s %s no > /dev/null'",
	    LULIB, LULIB_PROPAGATE_FILE, LU_MENU_CKSUM);
	ret = exec_cmd(cmdline, NULL);
	INJECT_ERROR1("PROPAGATE_MENU_CKSUM_FILE", ret = 1);
	if (ret != 0) {
		bam_error(MENU_CKSUM_PROP_FAIL, LU_MENU_CKSUM);
		return (BAM_ERROR);
	}
	BAM_DPRINTF((D_PROPAGATED_CKSUM_FILE, fcn, LU_MENU_CKSUM));

	(void) snprintf(cmdline, sizeof (cmdline),
	    "/bin/sh -c '. %s > /dev/null; %s %s no > /dev/null'",
	    LULIB, LULIB_PROPAGATE_FILE, BOOTADM);
	ret = exec_cmd(cmdline, NULL);
	INJECT_ERROR1("PROPAGATE_BOOTADM_FILE", ret = 1);
	if (ret != 0) {
		bam_error(BOOTADM_PROP_FAIL, BOOTADM);
		return (BAM_ERROR);
	}
	BAM_DPRINTF((D_PROPAGATED_BOOTADM, fcn, BOOTADM));

	return (BAM_SUCCESS);
}

static error_t
update_all(char *root, char *opt)
{
	struct extmnttab mnt;
	struct stat sb;
	FILE *fp;
	char multibt[PATH_MAX];
	char creatram[PATH_MAX];
	error_t ret = BAM_SUCCESS;

	assert(root);
	assert(opt == NULL);

	if (bam_rootlen != 1 || *root != '/') {
		elide_trailing_slash(root, multibt, sizeof (multibt));
		bam_error(ALT_ROOT_INVALID, multibt);
		return (BAM_ERROR);
	}

	/*
	 * Check to see if we are in the midst of safemode patching
	 * If so skip building the archive for /. Instead build it
	 * against the latest bits obtained by creating a fresh lofs
	 * mount of root.
	 */
	if (stat(LOFS_PATCH_FILE, &sb) == 0)  {
		if (mkdir(LOFS_PATCH_MNT, 0755) == -1 &&
		    errno != EEXIST) {
			bam_error(MKDIR_FAILED, "%s", LOFS_PATCH_MNT,
			    strerror(errno));
			ret = BAM_ERROR;
			goto out;
		}
		(void) snprintf(multibt, sizeof (multibt),
		    "/sbin/mount -F lofs -o nosub /  %s", LOFS_PATCH_MNT);
		if (exec_cmd(multibt, NULL) != 0) {
			bam_error(MOUNT_FAILED, LOFS_PATCH_MNT, "lofs");
			ret = BAM_ERROR;
		}
		if (ret != BAM_ERROR) {
			(void) snprintf(rootbuf, sizeof (rootbuf), "%s/",
			    LOFS_PATCH_MNT);
			bam_rootlen = strlen(rootbuf);
			if (update_archive(rootbuf, opt) != BAM_SUCCESS)
				ret = BAM_ERROR;
			/*
			 * unmount the lofs mount since there could be
			 * multiple invocations of bootadm -a update_all
			 */
			(void) snprintf(multibt, sizeof (multibt),
			    "/sbin/umount %s", LOFS_PATCH_MNT);
			if (exec_cmd(multibt, NULL) != 0) {
				bam_error(UMOUNT_FAILED, LOFS_PATCH_MNT);
				ret = BAM_ERROR;
			}
		}
	} else {
		/*
		 * First update archive for current root
		 */
		if (update_archive(root, opt) != BAM_SUCCESS)
			ret = BAM_ERROR;
	}

	if (ret == BAM_ERROR)
		goto out;

	/*
	 * Now walk the mount table, performing archive update
	 * for all mounted Newboot root filesystems
	 */
	fp = fopen(MNTTAB, "r");
	if (fp == NULL) {
		bam_error(OPEN_FAIL, MNTTAB, strerror(errno));
		ret = BAM_ERROR;
		goto out;
	}

	resetmnttab(fp);

	while (getextmntent(fp, &mnt, sizeof (mnt)) == 0) {
		if (mnt.mnt_special == NULL)
			continue;
		if (strncmp(mnt.mnt_special, "/dev/", strlen("/dev/")) != 0)
			continue;
		if (strcmp(mnt.mnt_mountp, "/") == 0)
			continue;

		(void) snprintf(creatram, sizeof (creatram), "%s/%s",
		    mnt.mnt_mountp, CREATE_RAMDISK);

		if (stat(creatram, &sb) == -1)
			continue;

		/*
		 * We put a trailing slash to be consistent with root = "/"
		 * case, such that we don't have to print // in some cases.
		 */
		(void) snprintf(rootbuf, sizeof (rootbuf), "%s/",
		    mnt.mnt_mountp);
		bam_rootlen = strlen(rootbuf);

		/*
		 * It's possible that other mounts may be an alternate boot
		 * architecture, so check it again.
		 */
		if ((get_boot_cap(rootbuf) != BAM_SUCCESS) ||
		    (update_archive(rootbuf, opt) != BAM_SUCCESS))
			ret = BAM_ERROR;
	}

	(void) fclose(fp);

out:
	/*
	 * We no longer use biosdev for Live Upgrade. Hence
	 * there is no need to defer (to shutdown time) any fdisk
	 * updates
	 */
	if (stat(GRUB_fdisk, &sb) == 0 || stat(GRUB_fdisk_target, &sb) == 0) {
		bam_error(FDISK_FILES_FOUND, GRUB_fdisk, GRUB_fdisk_target);
	}

	/*
	 * If user has updated menu in current BE, propagate the
	 * updates to all BEs.
	 */
	if (synchronize_BE_menu() != BAM_SUCCESS)
		ret = BAM_ERROR;

	return (ret);
}

static void
append_line(menu_t *mp, line_t *lp)
{
	if (mp->start == NULL) {
		mp->start = lp;
	} else {
		mp->end->next = lp;
		lp->prev = mp->end;
	}
	mp->end = lp;
}

void
unlink_line(menu_t *mp, line_t *lp)
{
	/* unlink from list */
	if (lp->prev)
		lp->prev->next = lp->next;
	else
		mp->start = lp->next;
	if (lp->next)
		lp->next->prev = lp->prev;
	else
		mp->end = lp->prev;
}

static entry_t *
boot_entry_new(menu_t *mp, line_t *start, line_t *end)
{
	entry_t *ent, *prev;
	const char *fcn = "boot_entry_new()";

	assert(mp);
	assert(start);
	assert(end);

	ent = s_calloc(1, sizeof (entry_t));
	BAM_DPRINTF((D_ENTRY_NEW, fcn));
	ent->start = start;
	ent->end = end;

	if (mp->entries == NULL) {
		mp->entries = ent;
		BAM_DPRINTF((D_ENTRY_NEW_FIRST, fcn));
		return (ent);
	}

	prev = mp->entries;
	while (prev->next)
		prev = prev->next;
	prev->next = ent;
	ent->prev = prev;
	BAM_DPRINTF((D_ENTRY_NEW_LINKED, fcn));
	return (ent);
}

static void
boot_entry_addline(entry_t *ent, line_t *lp)
{
	if (ent)
		ent->end = lp;
}

/*
 * Check whether cmd matches the one indexed by which, and whether arg matches
 * str.  which must be either KERNEL_CMD or MODULE_CMD, and a match to the
 * respective *_DOLLAR_CMD is also acceptable.  The arg is searched using
 * strstr(), so it can be a partial match.
 */
static int
check_cmd(const char *cmd, const int which, const char *arg, const char *str)
{
	int			ret;
	const char		*fcn = "check_cmd()";

	BAM_DPRINTF((D_FUNC_ENTRY2, fcn, arg, str));

	if ((strcmp(cmd, menu_cmds[which]) != 0) &&
	    (strcmp(cmd, menu_cmds[which + 1]) != 0)) {
		BAM_DPRINTF((D_CHECK_CMD_CMD_NOMATCH,
		    fcn, cmd, menu_cmds[which]));
		return (0);
	}
	ret = (strstr(arg, str) != NULL);

	if (ret) {
		BAM_DPRINTF((D_RETURN_SUCCESS, fcn));
	} else {
		BAM_DPRINTF((D_RETURN_FAILURE, fcn));
	}

	return (ret);
}

static error_t
kernel_parser(entry_t *entry, char *cmd, char *arg, int linenum)
{
	const char		*fcn  = "kernel_parser()";

	assert(entry);
	assert(cmd);
	assert(arg);

	if (strcmp(cmd, menu_cmds[KERNEL_CMD]) != 0 &&
	    strcmp(cmd, menu_cmds[KERNEL_DOLLAR_CMD]) != 0) {
		BAM_DPRINTF((D_NOT_KERNEL_CMD, fcn, cmd));
		return (BAM_ERROR);
	}

	if (strncmp(arg, DIRECT_BOOT_32, sizeof (DIRECT_BOOT_32) - 1) == 0) {
		BAM_DPRINTF((D_SET_DBOOT_32, fcn, arg));
		entry->flags |= BAM_ENTRY_DBOOT | BAM_ENTRY_32BIT;
	} else if (strncmp(arg, DIRECT_BOOT_KERNEL,
	    sizeof (DIRECT_BOOT_KERNEL) - 1) == 0) {
		BAM_DPRINTF((D_SET_DBOOT, fcn, arg));
		entry->flags |= BAM_ENTRY_DBOOT;
	} else if (strncmp(arg, DIRECT_BOOT_64,
	    sizeof (DIRECT_BOOT_64) - 1) == 0) {
		BAM_DPRINTF((D_SET_DBOOT_64, fcn, arg));
		entry->flags |= BAM_ENTRY_DBOOT | BAM_ENTRY_64BIT;
	} else if (strncmp(arg, DIRECT_BOOT_FAILSAFE_KERNEL,
	    sizeof (DIRECT_BOOT_FAILSAFE_KERNEL) - 1) == 0) {
		BAM_DPRINTF((D_SET_DBOOT_FAILSAFE, fcn, arg));
		entry->flags |= BAM_ENTRY_DBOOT | BAM_ENTRY_FAILSAFE;
	} else if (strncmp(arg, MULTI_BOOT, sizeof (MULTI_BOOT) - 1) == 0) {
		BAM_DPRINTF((D_SET_MULTIBOOT, fcn, arg));
		entry->flags |= BAM_ENTRY_MULTIBOOT;
	} else if (strncmp(arg, MULTI_BOOT_FAILSAFE,
	    sizeof (MULTI_BOOT_FAILSAFE) - 1) == 0) {
		BAM_DPRINTF((D_SET_MULTIBOOT_FAILSAFE, fcn, arg));
		entry->flags |= BAM_ENTRY_MULTIBOOT | BAM_ENTRY_FAILSAFE;
	} else if (strstr(arg, XEN_KERNEL_SUBSTR)) {
		BAM_DPRINTF((D_SET_HV, fcn, arg));
		entry->flags |= BAM_ENTRY_HV;
	} else if (!(entry->flags & (BAM_ENTRY_BOOTADM|BAM_ENTRY_LU))) {
		BAM_DPRINTF((D_SET_HAND_KERNEL, fcn, arg));
		return (BAM_ERROR);
	} else {
		BAM_DPRINTF((D_IS_UNKNOWN_KERNEL, fcn, arg));
		bam_error(UNKNOWN_KERNEL_LINE, linenum);
		return (BAM_ERROR);
	}

	return (BAM_SUCCESS);
}

static error_t
module_parser(entry_t *entry, char *cmd, char *arg, int linenum)
{
	const char		*fcn = "module_parser()";

	assert(entry);
	assert(cmd);
	assert(arg);

	if (strcmp(cmd, menu_cmds[MODULE_CMD]) != 0 &&
	    strcmp(cmd, menu_cmds[MODULE_DOLLAR_CMD]) != 0) {
		BAM_DPRINTF((D_NOT_MODULE_CMD, fcn, cmd));
		return (BAM_ERROR);
	}

	if (strcmp(arg, DIRECT_BOOT_ARCHIVE) == 0 ||
	    strcmp(arg, DIRECT_BOOT_ARCHIVE_32) == 0 ||
	    strcmp(arg, DIRECT_BOOT_ARCHIVE_64) == 0 ||
	    strcmp(arg, MULTIBOOT_ARCHIVE) == 0 ||
	    strcmp(arg, FAILSAFE_ARCHIVE) == 0 ||
	    strcmp(arg, XEN_KERNEL_MODULE_LINE) == 0 ||
	    strcmp(arg, XEN_KERNEL_MODULE_LINE_ZFS) == 0) {
		BAM_DPRINTF((D_BOOTADM_LU_MODULE, fcn, arg));
		return (BAM_SUCCESS);
	} else if (!(entry->flags & BAM_ENTRY_BOOTADM) &&
	    !(entry->flags & BAM_ENTRY_LU)) {
		/* don't emit warning for hand entries */
		BAM_DPRINTF((D_IS_HAND_MODULE, fcn, arg));
		return (BAM_ERROR);
	} else {
		BAM_DPRINTF((D_IS_UNKNOWN_MODULE, fcn, arg));
		bam_error(UNKNOWN_MODULE_LINE, linenum);
		return (BAM_ERROR);
	}
}

/*
 * A line in menu.lst looks like
 * [ ]*<cmd>[ \t=]*<arg>*
 */
static void
line_parser(menu_t *mp, char *str, int *lineNum, int *entryNum)
{
	/*
	 * save state across calls. This is so that
	 * header gets the right entry# after title has
	 * been processed
	 */
	static line_t *prev = NULL;
	static entry_t *curr_ent = NULL;
	static int in_liveupgrade = 0;

	line_t	*lp;
	char *cmd, *sep, *arg;
	char save, *cp, *line;
	menu_flag_t flag = BAM_INVALID;
	const char *fcn = "line_parser()";

	if (str == NULL) {
		return;
	}

	/*
	 * First save a copy of the entire line.
	 * We use this later to set the line field.
	 */
	line = s_strdup(str);

	/* Eat up leading whitespace */
	while (*str == ' ' || *str == '\t')
		str++;

	if (*str == '#') {		/* comment */
		cmd = s_strdup("#");
		sep = NULL;
		arg = s_strdup(str + 1);
		flag = BAM_COMMENT;
		if (strstr(arg, BAM_LU_HDR) != NULL) {
			in_liveupgrade = 1;
		} else if (strstr(arg, BAM_LU_FTR) != NULL) {
			in_liveupgrade = 0;
		}
	} else if (*str == '\0') {	/* blank line */
		cmd = sep = arg = NULL;
		flag = BAM_EMPTY;
	} else {
		/*
		 * '=' is not a documented separator in grub syntax.
		 * However various development bits use '=' as a
		 * separator. In addition, external users also
		 * use = as a separator. So we will allow that usage.
		 */
		cp = str;
		while (*str != ' ' && *str != '\t' && *str != '=') {
			if (*str == '\0') {
				cmd = s_strdup(cp);
				sep = arg = NULL;
				break;
			}
			str++;
		}

		if (*str != '\0') {
			save = *str;
			*str = '\0';
			cmd = s_strdup(cp);
			*str = save;

			str++;
			save = *str;
			*str = '\0';
			sep = s_strdup(str - 1);
			*str = save;

			while (*str == ' ' || *str == '\t')
				str++;
			if (*str == '\0')
				arg = NULL;
			else
				arg = s_strdup(str);
		}
	}

	lp = s_calloc(1, sizeof (line_t));

	lp->cmd = cmd;
	lp->sep = sep;
	lp->arg = arg;
	lp->line = line;
	lp->lineNum = ++(*lineNum);
	if (cmd && strcmp(cmd, menu_cmds[TITLE_CMD]) == 0) {
		lp->entryNum = ++(*entryNum);
		lp->flags = BAM_TITLE;
		if (prev && prev->flags == BAM_COMMENT &&
		    prev->arg && strcmp(prev->arg, BAM_BOOTADM_HDR) == 0) {
			prev->entryNum = lp->entryNum;
			curr_ent = boot_entry_new(mp, prev, lp);
			curr_ent->flags |= BAM_ENTRY_BOOTADM;
			BAM_DPRINTF((D_IS_BOOTADM_ENTRY, fcn, arg));
		} else {
			curr_ent = boot_entry_new(mp, lp, lp);
			if (in_liveupgrade) {
				curr_ent->flags |= BAM_ENTRY_LU;
				BAM_DPRINTF((D_IS_LU_ENTRY, fcn, arg));
			}
		}
		curr_ent->entryNum = *entryNum;
	} else if (flag != BAM_INVALID) {
		/*
		 * For header comments, the entry# is "fixed up"
		 * by the subsequent title
		 */
		lp->entryNum = *entryNum;
		lp->flags = flag;
	} else {
		lp->entryNum = *entryNum;

		if (*entryNum == ENTRY_INIT) {
			lp->flags = BAM_GLOBAL;
		} else {
			lp->flags = BAM_ENTRY;

			if (cmd && arg) {
				if (strcmp(cmd, menu_cmds[ROOT_CMD]) == 0) {
					BAM_DPRINTF((D_IS_ROOT_CMD, fcn, arg));
					curr_ent->flags |= BAM_ENTRY_ROOT;
				} else if (strcmp(cmd, menu_cmds[FINDROOT_CMD])
				    == 0) {
					BAM_DPRINTF((D_IS_FINDROOT_CMD, fcn,
					    arg));
					curr_ent->flags |= BAM_ENTRY_FINDROOT;
				} else if (strcmp(cmd,
				    menu_cmds[CHAINLOADER_CMD]) == 0) {
					BAM_DPRINTF((D_IS_CHAINLOADER_CMD, fcn,
					    arg));
					curr_ent->flags |=
					    BAM_ENTRY_CHAINLOADER;
				} else if (kernel_parser(curr_ent, cmd, arg,
				    lp->lineNum) != BAM_SUCCESS) {
					(void) module_parser(curr_ent, cmd,
					    arg, lp->lineNum);
				}
			}
		}
	}

	/* record default, old default, and entry line ranges */
	if (lp->flags == BAM_GLOBAL &&
	    strcmp(lp->cmd, menu_cmds[DEFAULT_CMD]) == 0) {
		mp->curdefault = lp;
	} else if (lp->flags == BAM_COMMENT &&
	    strncmp(lp->arg, BAM_OLDDEF, strlen(BAM_OLDDEF)) == 0) {
		mp->olddefault = lp;
	} else if (lp->flags == BAM_COMMENT &&
	    strncmp(lp->arg, BAM_OLD_RC_DEF, strlen(BAM_OLD_RC_DEF)) == 0) {
		mp->old_rc_default = lp;
	} else if (lp->flags == BAM_ENTRY ||
	    (lp->flags == BAM_COMMENT &&
	    strcmp(lp->arg, BAM_BOOTADM_FTR) == 0)) {
		boot_entry_addline(curr_ent, lp);
	}
	append_line(mp, lp);

	prev = lp;
}

void
update_numbering(menu_t *mp)
{
	int lineNum;
	int entryNum;
	int old_default_value;
	line_t *lp, *prev, *default_lp, *default_entry;
	char buf[PATH_MAX];

	if (mp->start == NULL) {
		return;
	}

	lineNum = LINE_INIT;
	entryNum = ENTRY_INIT;
	old_default_value = ENTRY_INIT;
	lp = default_lp = default_entry = NULL;

	prev = NULL;
	for (lp = mp->start; lp; prev = lp, lp = lp->next) {
		lp->lineNum = ++lineNum;

		/*
		 * Get the value of the default command
		 */
		if (lp->entryNum == ENTRY_INIT && lp->cmd &&
		    strcmp(lp->cmd, menu_cmds[DEFAULT_CMD]) == 0 &&
		    lp->arg) {
			old_default_value = atoi(lp->arg);
			default_lp = lp;
		}

		/*
		 * If not a booting entry, nothing else to fix for this
		 * entry
		 */
		if (lp->entryNum == ENTRY_INIT)
			continue;

		/*
		 * Record the position of the default entry.
		 * The following works because global
		 * commands like default and timeout should precede
		 * actual boot entries, so old_default_value
		 * is already known (or default cmd is missing).
		 */
		if (default_entry == NULL &&
		    old_default_value != ENTRY_INIT &&
		    lp->entryNum == old_default_value) {
			default_entry = lp;
		}

		/*
		 * Now fixup the entry number
		 */
		if (lp->cmd && strcmp(lp->cmd, menu_cmds[TITLE_CMD]) == 0) {
			lp->entryNum = ++entryNum;
			/* fixup the bootadm header */
			if (prev && prev->flags == BAM_COMMENT &&
			    prev->arg &&
			    strcmp(prev->arg, BAM_BOOTADM_HDR) == 0) {
				prev->entryNum = lp->entryNum;
			}
		} else {
			lp->entryNum = entryNum;
		}
	}

	/*
	 * No default command in menu, simply return
	 */
	if (default_lp == NULL) {
		return;
	}

	free(default_lp->arg);
	free(default_lp->line);

	if (default_entry == NULL) {
		default_lp->arg = s_strdup("0");
	} else {
		(void) snprintf(buf, sizeof (buf), "%d",
		    default_entry->entryNum);
		default_lp->arg = s_strdup(buf);
	}

	/*
	 * The following is required since only the line field gets
	 * written back to menu.lst
	 */
	(void) snprintf(buf, sizeof (buf), "%s%s%s",
	    menu_cmds[DEFAULT_CMD], menu_cmds[SEP_CMD], default_lp->arg);
	default_lp->line = s_strdup(buf);
}


static menu_t *
menu_read(char *menu_path)
{
	FILE *fp;
	char buf[BAM_MAXLINE], *cp;
	menu_t *mp;
	int line, entry, len, n;

	mp = s_calloc(1, sizeof (menu_t));

	fp = fopen(menu_path, "r");
	if (fp == NULL) { /* Let the caller handle this error */
		return (mp);
	}


	/* Note: GRUB boot entry number starts with 0 */
	line = LINE_INIT;
	entry = ENTRY_INIT;
	cp = buf;
	len = sizeof (buf);
	while (s_fgets(cp, len, fp) != NULL) {
		n = strlen(cp);
		if (cp[n - 1] == '\\') {
			len -= n - 1;
			assert(len >= 2);
			cp += n - 1;
			continue;
		}
		line_parser(mp, buf, &line, &entry);
		cp = buf;
		len = sizeof (buf);
	}

	if (fclose(fp) == EOF) {
		bam_error(CLOSE_FAIL, menu_path, strerror(errno));
	}

	return (mp);
}

static error_t
selector(menu_t *mp, char *opt, int *entry, char **title)
{
	char *eq;
	char *opt_dup;
	int entryNum;

	assert(mp);
	assert(mp->start);
	assert(opt);

	opt_dup = s_strdup(opt);

	if (entry)
		*entry = ENTRY_INIT;
	if (title)
		*title = NULL;

	eq = strchr(opt_dup, '=');
	if (eq == NULL) {
		bam_error(INVALID_OPT, opt);
		free(opt_dup);
		return (BAM_ERROR);
	}

	*eq = '\0';
	if (entry && strcmp(opt_dup, OPT_ENTRY_NUM) == 0) {
		assert(mp->end);
		entryNum = s_strtol(eq + 1);
		if (entryNum < 0 || entryNum > mp->end->entryNum) {
			bam_error(INVALID_ENTRY, eq + 1);
			free(opt_dup);
			return (BAM_ERROR);
		}
		*entry = entryNum;
	} else if (title && strcmp(opt_dup, menu_cmds[TITLE_CMD]) == 0) {
		*title = opt + (eq - opt_dup) + 1;
	} else {
		bam_error(INVALID_OPT, opt);
		free(opt_dup);
		return (BAM_ERROR);
	}

	free(opt_dup);
	return (BAM_SUCCESS);
}

/*
 * If invoked with no titles/entries (opt == NULL)
 * only title lines in file are printed.
 *
 * If invoked with a title or entry #, all
 * lines in *every* matching entry are listed
 */
static error_t
list_entry(menu_t *mp, char *menu_path, char *opt)
{
	line_t *lp;
	int entry = ENTRY_INIT;
	int found;
	char *title = NULL;

	assert(mp);
	assert(menu_path);

	/* opt is optional */
	BAM_DPRINTF((D_FUNC_ENTRY2, "list_entry", menu_path,
	    opt ? opt : "<NULL>"));

	if (mp->start == NULL) {
		bam_error(NO_MENU, menu_path);
		return (BAM_ERROR);
	}

	if (opt != NULL) {
		if (selector(mp, opt, &entry, &title) != BAM_SUCCESS) {
			return (BAM_ERROR);
		}
		assert((entry != ENTRY_INIT) ^ (title != NULL));
	} else {
		(void) read_globals(mp, menu_path, menu_cmds[DEFAULT_CMD], 0);
		(void) read_globals(mp, menu_path, menu_cmds[TIMEOUT_CMD], 0);
	}

	found = 0;
	for (lp = mp->start; lp; lp = lp->next) {
		if (lp->flags == BAM_COMMENT || lp->flags == BAM_EMPTY)
			continue;
		if (opt == NULL && lp->flags == BAM_TITLE) {
			bam_print(PRINT_TITLE, lp->entryNum,
			    lp->arg);
			found = 1;
			continue;
		}
		if (entry != ENTRY_INIT && lp->entryNum == entry) {
			bam_print(PRINT, lp->line);
			found = 1;
			continue;
		}

		/*
		 * We set the entry value here so that all lines
		 * in entry get printed. If we subsequently match
		 * title in other entries, all lines in those
		 * entries get printed as well.
		 */
		if (title && lp->flags == BAM_TITLE && lp->arg &&
		    strncmp(title, lp->arg, strlen(title)) == 0) {
			bam_print(PRINT, lp->line);
			entry = lp->entryNum;
			found = 1;
			continue;
		}
	}

	if (!found) {
		bam_error(NO_MATCH_ENTRY);
		return (BAM_ERROR);
	}

	return (BAM_SUCCESS);
}

int
add_boot_entry(menu_t *mp,
	char *title,
	char *findroot,
	char *kernel,
	char *mod_kernel,
	char *module)
{
	int		lineNum;
	int		entryNum;
	char		linebuf[BAM_MAXLINE];
	menu_cmd_t	k_cmd;
	menu_cmd_t	m_cmd;
	const char	*fcn = "add_boot_entry()";

	assert(mp);

	INJECT_ERROR1("ADD_BOOT_ENTRY_FINDROOT_NULL", findroot = NULL);
	if (findroot == NULL) {
		bam_error(NULL_FINDROOT);
		return (BAM_ERROR);
	}

	if (title == NULL) {
		title = "Solaris";	/* default to Solaris */
	}
	if (kernel == NULL) {
		bam_error(SUBOPT_MISS, menu_cmds[KERNEL_CMD]);
		return (BAM_ERROR);
	}
	if (module == NULL) {
		if (bam_direct != BAM_DIRECT_DBOOT) {
			bam_error(SUBOPT_MISS, menu_cmds[MODULE_CMD]);
			return (BAM_ERROR);
		}

		/* Figure the commands out from the kernel line */
		if (strstr(kernel, "$ISADIR") != NULL) {
			module = DIRECT_BOOT_ARCHIVE;
			k_cmd = KERNEL_DOLLAR_CMD;
			m_cmd = MODULE_DOLLAR_CMD;
		} else if (strstr(kernel, "amd64") != NULL) {
			module = DIRECT_BOOT_ARCHIVE_64;
			k_cmd = KERNEL_CMD;
			m_cmd = MODULE_CMD;
		} else {
			module = DIRECT_BOOT_ARCHIVE_32;
			k_cmd = KERNEL_CMD;
			m_cmd = MODULE_CMD;
		}
	} else if ((bam_direct == BAM_DIRECT_DBOOT) &&
	    (strstr(kernel, "$ISADIR") != NULL)) {
		/*
		 * If it's a non-failsafe dboot kernel, use the "kernel$"
		 * command.  Otherwise, use "kernel".
		 */
		k_cmd = KERNEL_DOLLAR_CMD;
		m_cmd = MODULE_DOLLAR_CMD;
	} else {
		k_cmd = KERNEL_CMD;
		m_cmd = MODULE_CMD;
	}

	if (mp->start) {
		lineNum = mp->end->lineNum;
		entryNum = mp->end->entryNum;
	} else {
		lineNum = LINE_INIT;
		entryNum = ENTRY_INIT;
	}

	/*
	 * No separator for comment (HDR/FTR) commands
	 * The syntax for comments is #<comment>
	 */
	(void) snprintf(linebuf, sizeof (linebuf), "%s%s",
	    menu_cmds[COMMENT_CMD], BAM_BOOTADM_HDR);
	line_parser(mp, linebuf, &lineNum, &entryNum);

	(void) snprintf(linebuf, sizeof (linebuf), "%s%s%s",
	    menu_cmds[TITLE_CMD], menu_cmds[SEP_CMD], title);
	line_parser(mp, linebuf, &lineNum, &entryNum);

	(void) snprintf(linebuf, sizeof (linebuf), "%s%s%s",
	    menu_cmds[FINDROOT_CMD], menu_cmds[SEP_CMD], findroot);
	line_parser(mp, linebuf, &lineNum, &entryNum);
	BAM_DPRINTF((D_ADD_FINDROOT_NUM, fcn, lineNum, entryNum));

	(void) snprintf(linebuf, sizeof (linebuf), "%s%s%s",
	    menu_cmds[k_cmd], menu_cmds[SEP_CMD], kernel);
	line_parser(mp, linebuf, &lineNum, &entryNum);

	if (mod_kernel != NULL) {
		(void) snprintf(linebuf, sizeof (linebuf), "%s%s%s",
		    menu_cmds[m_cmd], menu_cmds[SEP_CMD], mod_kernel);
		line_parser(mp, linebuf, &lineNum, &entryNum);
	}

	(void) snprintf(linebuf, sizeof (linebuf), "%s%s%s",
	    menu_cmds[m_cmd], menu_cmds[SEP_CMD], module);
	line_parser(mp, linebuf, &lineNum, &entryNum);

	(void) snprintf(linebuf, sizeof (linebuf), "%s%s",
	    menu_cmds[COMMENT_CMD], BAM_BOOTADM_FTR);
	line_parser(mp, linebuf, &lineNum, &entryNum);

	return (entryNum);
}

static error_t
do_delete(menu_t *mp, int entryNum)
{
	line_t		*lp;
	line_t		*freed;
	entry_t		*ent;
	entry_t		*tmp;
	int		deleted;
	const char	*fcn = "do_delete()";

	assert(entryNum != ENTRY_INIT);

	tmp = NULL;

	ent = mp->entries;
	while (ent) {
		lp = ent->start;
		/* check entry number and make sure it's a bootadm entry */
		if (lp->flags != BAM_COMMENT ||
		    strcmp(lp->arg, BAM_BOOTADM_HDR) != 0 ||
		    (entryNum != ALL_ENTRIES && lp->entryNum != entryNum)) {
			ent = ent->next;
			continue;
		}

		/* free the entry content */
		do {
			freed = lp;
			lp = lp->next;	/* prev stays the same */
			BAM_DPRINTF((D_FREEING_LINE, fcn, freed->lineNum));
			unlink_line(mp, freed);
			line_free(freed);
		} while (freed != ent->end);

		/* free the entry_t structure */
		assert(tmp == NULL);
		tmp = ent;
		ent = ent->next;
		if (tmp->prev)
			tmp->prev->next = ent;
		else
			mp->entries = ent;
		if (ent)
			ent->prev = tmp->prev;
		BAM_DPRINTF((D_FREEING_ENTRY, fcn, tmp->entryNum));
		free(tmp);
		tmp = NULL;
		deleted = 1;
	}

	assert(tmp == NULL);

	if (!deleted && entryNum != ALL_ENTRIES) {
		bam_error(NO_BOOTADM_MATCH);
		return (BAM_ERROR);
	}

	/*
	 * Now that we have deleted an entry, update
	 * the entry numbering and the default cmd.
	 */
	update_numbering(mp);

	return (BAM_SUCCESS);
}

static error_t
delete_all_entries(menu_t *mp, char *dummy, char *opt)
{
	assert(mp);
	assert(dummy == NULL);
	assert(opt == NULL);

	BAM_DPRINTF((D_FUNC_ENTRY0, "delete_all_entries"));

	if (mp->start == NULL) {
		bam_print(EMPTY_MENU);
		return (BAM_SUCCESS);
	}

	if (do_delete(mp, ALL_ENTRIES) != BAM_SUCCESS) {
		return (BAM_ERROR);
	}

	return (BAM_WRITE);
}

static FILE *
create_diskmap(char *osroot)
{
	FILE *fp;
	char cmd[PATH_MAX];
	const char *fcn = "create_diskmap()";

	/* make sure we have a map file */
	fp = fopen(GRUBDISK_MAP, "r");
	if (fp == NULL) {
		(void) snprintf(cmd, sizeof (cmd),
		    "%s/%s > /dev/null", osroot, CREATE_DISKMAP);
		if (exec_cmd(cmd, NULL) != 0)
			return (NULL);
		fp = fopen(GRUBDISK_MAP, "r");
		INJECT_ERROR1("DISKMAP_CREATE_FAIL", fp = NULL);
		if (fp) {
			BAM_DPRINTF((D_CREATED_DISKMAP, fcn, GRUBDISK_MAP));
		} else {
			BAM_DPRINTF((D_CREATE_DISKMAP_FAIL, fcn, GRUBDISK_MAP));
		}
	}
	return (fp);
}

#define	SECTOR_SIZE	512

static int
get_partition(char *device)
{
	int i, fd, is_pcfs, partno = -1;
	struct mboot *mboot;
	char boot_sect[SECTOR_SIZE];
	char *wholedisk, *slice;

	/* form whole disk (p0) */
	slice = device + strlen(device) - 2;
	is_pcfs = (*slice != 's');
	if (!is_pcfs)
		*slice = '\0';
	wholedisk = s_calloc(1, strlen(device) + 3);
	(void) snprintf(wholedisk, strlen(device) + 3, "%sp0", device);
	if (!is_pcfs)
		*slice = 's';

	/* read boot sector */
	fd = open(wholedisk, O_RDONLY);
	free(wholedisk);
	if (fd == -1 || read(fd, boot_sect, SECTOR_SIZE) != SECTOR_SIZE) {
		return (partno);
	}
	(void) close(fd);

	/* parse fdisk table */
	mboot = (struct mboot *)((void *)boot_sect);
	for (i = 0; i < FD_NUMPART; i++) {
		struct ipart *part =
		    (struct ipart *)(uintptr_t)mboot->parts + i;
		if (is_pcfs) {	/* looking for solaris boot part */
			if (part->systid == 0xbe) {
				partno = i;
				break;
			}
		} else {	/* look for solaris partition, old and new */
			if (part->systid == SUNIXOS ||
			    part->systid == SUNIXOS2) {
				partno = i;
				break;
			}
		}
	}
	return (partno);
}

char *
get_grubroot(char *osroot, char *osdev, char *menu_root)
{
	char		*grubroot;	/* (hd#,#,#) */
	char		*slice;
	char		*grubhd;
	int		fdiskpart;
	int		found = 0;
	char		*devname;
	char		*ctdname = strstr(osdev, "dsk/");
	char		linebuf[PATH_MAX];
	FILE		*fp;
	const char	*fcn = "get_grubroot()";

	INJECT_ERROR1("GRUBROOT_INVALID_OSDEV", ctdname = NULL);
	if (ctdname == NULL) {
		bam_error(INVALID_DEV_DSK, osdev);
		return (NULL);
	}

	if (menu_root && !menu_on_bootdisk(osroot, menu_root)) {
		/* menu bears no resemblance to our reality */
		bam_error(CANNOT_GRUBROOT_BOOTDISK, fcn, osdev);
		return (NULL);
	}

	ctdname += strlen("dsk/");
	slice = strrchr(ctdname, 's');
	if (slice)
		*slice = '\0';

	fp = create_diskmap(osroot);
	if (fp == NULL) {
		bam_error(DISKMAP_FAIL, osroot);
		return (NULL);
	}

	rewind(fp);
	while (s_fgets(linebuf, sizeof (linebuf), fp) != NULL) {
		grubhd = strtok(linebuf, " \t\n");
		if (grubhd)
			devname = strtok(NULL, " \t\n");
		else
			devname = NULL;
		if (devname && strcmp(devname, ctdname) == 0) {
			found = 1;
			break;
		}
	}

	if (slice)
		*slice = 's';

	(void) fclose(fp);
	fp = NULL;

	INJECT_ERROR1("GRUBROOT_BIOSDEV_FAIL", found = 0);
	if (found == 0) {
		bam_error(BIOSDEV_FAIL, osdev);
		return (NULL);
	}

	fdiskpart = get_partition(osdev);
	INJECT_ERROR1("GRUBROOT_FDISK_FAIL", fdiskpart = -1);
	if (fdiskpart == -1) {
		bam_error(FDISKPART_FAIL, osdev);
		return (NULL);
	}

	grubroot = s_calloc(1, 10);
	if (slice) {
		(void) snprintf(grubroot, 10, "(hd%s,%d,%c)",
		    grubhd, fdiskpart, slice[1] + 'a' - '0');
	} else
		(void) snprintf(grubroot, 10, "(hd%s,%d)",
		    grubhd, fdiskpart);

	assert(fp == NULL);
	assert(strncmp(grubroot, "(hd", strlen("(hd")) == 0);
	return (grubroot);
}

static char *
find_primary_common(char *mntpt, char *fstype)
{
	char		signdir[PATH_MAX];
	char		tmpsign[MAXNAMELEN + 1];
	char		*lu;
	char		*ufs;
	char		*zfs;
	DIR		*dirp = NULL;
	struct dirent	*entp;
	struct stat	sb;
	const char	*fcn = "find_primary_common()";

	(void) snprintf(signdir, sizeof (signdir), "%s/%s",
	    mntpt, GRUBSIGN_DIR);

	if (stat(signdir, &sb) == -1) {
		BAM_DPRINTF((D_NO_SIGNDIR, fcn, signdir));
		return (NULL);
	}

	dirp = opendir(signdir);
	INJECT_ERROR1("SIGNDIR_OPENDIR_FAIL", dirp = NULL);
	if (dirp == NULL) {
		bam_error(OPENDIR_FAILED, signdir, strerror(errno));
		return (NULL);
	}

	ufs = zfs = lu = NULL;

	while (entp = readdir(dirp)) {
		if (strcmp(entp->d_name, ".") == 0 ||
		    strcmp(entp->d_name, "..") == 0)
			continue;

		(void) snprintf(tmpsign, sizeof (tmpsign), "%s", entp->d_name);

		if (lu == NULL &&
		    strncmp(tmpsign, GRUBSIGN_LU_PREFIX,
		    strlen(GRUBSIGN_LU_PREFIX)) == 0) {
			lu = s_strdup(tmpsign);
		}

		if (ufs == NULL &&
		    strncmp(tmpsign, GRUBSIGN_UFS_PREFIX,
		    strlen(GRUBSIGN_UFS_PREFIX)) == 0) {
			ufs = s_strdup(tmpsign);
		}

		if (zfs == NULL &&
		    strncmp(tmpsign, GRUBSIGN_ZFS_PREFIX,
		    strlen(GRUBSIGN_ZFS_PREFIX)) == 0) {
			zfs = s_strdup(tmpsign);
		}
	}

	BAM_DPRINTF((D_EXIST_PRIMARY_SIGNS, fcn,
	    zfs ? zfs : "NULL",
	    ufs ? ufs : "NULL",
	    lu ? lu : "NULL"));

	if (dirp) {
		(void) closedir(dirp);
		dirp = NULL;
	}

	if (strcmp(fstype, "ufs") == 0 && zfs) {
		bam_error(SIGN_FSTYPE_MISMATCH, zfs, "ufs");
		free(zfs);
		zfs = NULL;
	} else if (strcmp(fstype, "zfs") == 0 && ufs) {
		bam_error(SIGN_FSTYPE_MISMATCH, ufs, "zfs");
		free(ufs);
		ufs = NULL;
	}

	assert(dirp == NULL);

	/* For now, we let Live Upgrade take care of its signature itself */
	if (lu) {
		BAM_DPRINTF((D_FREEING_LU_SIGNS, fcn, lu));
		free(lu);
		lu = NULL;
	}

	return (zfs ? zfs : ufs);
}

static char *
find_backup_common(char *mntpt, char *fstype)
{
	FILE		*bfp = NULL;
	char		tmpsign[MAXNAMELEN + 1];
	char		backup[PATH_MAX];
	char		*ufs;
	char		*zfs;
	char		*lu;
	int		error;
	const char	*fcn = "find_backup_common()";

	/*
	 * We didn't find it in the primary directory.
	 * Look at the backup
	 */
	(void) snprintf(backup, sizeof (backup), "%s%s",
	    mntpt, GRUBSIGN_BACKUP);

	bfp = fopen(backup, "r");
	if (bfp == NULL) {
		error = errno;
		if (bam_verbose) {
			bam_error(OPEN_FAIL, backup, strerror(error));
		}
		BAM_DPRINTF((D_OPEN_FAIL, fcn, backup, strerror(error)));
		return (NULL);
	}

	ufs = zfs = lu = NULL;

	while (s_fgets(tmpsign, sizeof (tmpsign), bfp) != NULL) {

		if (lu == NULL &&
		    strncmp(tmpsign, GRUBSIGN_LU_PREFIX,
		    strlen(GRUBSIGN_LU_PREFIX)) == 0) {
			lu = s_strdup(tmpsign);
		}

		if (ufs == NULL &&
		    strncmp(tmpsign, GRUBSIGN_UFS_PREFIX,
		    strlen(GRUBSIGN_UFS_PREFIX)) == 0) {
			ufs = s_strdup(tmpsign);
		}

		if (zfs == NULL &&
		    strncmp(tmpsign, GRUBSIGN_ZFS_PREFIX,
		    strlen(GRUBSIGN_ZFS_PREFIX)) == 0) {
			zfs = s_strdup(tmpsign);
		}
	}

	BAM_DPRINTF((D_EXIST_BACKUP_SIGNS, fcn,
	    zfs ? zfs : "NULL",
	    ufs ? ufs : "NULL",
	    lu ? lu : "NULL"));

	if (bfp) {
		(void) fclose(bfp);
		bfp = NULL;
	}

	if (strcmp(fstype, "ufs") == 0 && zfs) {
		bam_error(SIGN_FSTYPE_MISMATCH, zfs, "ufs");
		free(zfs);
		zfs = NULL;
	} else if (strcmp(fstype, "zfs") == 0 && ufs) {
		bam_error(SIGN_FSTYPE_MISMATCH, ufs, "zfs");
		free(ufs);
		ufs = NULL;
	}

	assert(bfp == NULL);

	/* For now, we let Live Upgrade take care of its signature itself */
	if (lu) {
		BAM_DPRINTF((D_FREEING_LU_SIGNS, fcn, lu));
		free(lu);
		lu = NULL;
	}

	return (zfs ? zfs : ufs);
}

static char *
find_ufs_existing(char *osroot)
{
	char		*sign;
	const char	*fcn = "find_ufs_existing()";

	sign = find_primary_common(osroot, "ufs");
	if (sign == NULL) {
		sign = find_backup_common(osroot, "ufs");
		BAM_DPRINTF((D_EXIST_BACKUP_SIGN, fcn, sign ? sign : "NULL"));
	} else {
		BAM_DPRINTF((D_EXIST_PRIMARY_SIGN, fcn, sign));
	}

	return (sign);
}

char *
get_mountpoint(char *special, char *fstype)
{
	FILE		*mntfp;
	struct mnttab	mp = {0};
	struct mnttab	mpref = {0};
	int		error;
	int		ret;
	const char	*fcn = "get_mountpoint()";

	BAM_DPRINTF((D_FUNC_ENTRY2, fcn, special, fstype));

	mntfp = fopen(MNTTAB, "r");
	error = errno;
	INJECT_ERROR1("MNTTAB_ERR_GET_MNTPT", mntfp = NULL);
	if (mntfp == NULL) {
		bam_error(OPEN_FAIL, MNTTAB, strerror(error));
		return (NULL);
	}

	mpref.mnt_special = special;
	mpref.mnt_fstype = fstype;

	ret = getmntany(mntfp, &mp, &mpref);
	INJECT_ERROR1("GET_MOUNTPOINT_MNTANY", ret = 1);
	if (ret != 0) {
		(void) fclose(mntfp);
		BAM_DPRINTF((D_NO_MNTPT, fcn, special, fstype));
		return (NULL);
	}
	(void) fclose(mntfp);

	assert(mp.mnt_mountp);

	BAM_DPRINTF((D_GET_MOUNTPOINT_RET, fcn, special, mp.mnt_mountp));

	return (s_strdup(mp.mnt_mountp));
}

/*
 * Mounts a "legacy" top dataset (if needed)
 * Returns:	The mountpoint of the legacy top dataset or NULL on error
 * 		mnted returns one of the above values defined for zfs_mnted_t
 */
static char *
mount_legacy_dataset(char *pool, zfs_mnted_t *mnted)
{
	char		cmd[PATH_MAX];
	char		tmpmnt[PATH_MAX];
	filelist_t	flist = {0};
	char		*is_mounted;
	struct stat	sb;
	int		ret;
	const char	*fcn = "mount_legacy_dataset()";

	BAM_DPRINTF((D_FUNC_ENTRY1, fcn, pool));

	*mnted = ZFS_MNT_ERROR;

	(void) snprintf(cmd, sizeof (cmd),
	    "/sbin/zfs get -Ho value mounted %s",
	    pool);

	ret = exec_cmd(cmd, &flist);
	INJECT_ERROR1("Z_MOUNT_LEG_GET_MOUNTED_CMD", ret = 1);
	if (ret != 0) {
		bam_error(ZFS_MNTED_FAILED, pool);
		return (NULL);
	}

	INJECT_ERROR1("Z_MOUNT_LEG_GET_MOUNTED_OUT", flist.head = NULL);
	if ((flist.head == NULL) || (flist.head != flist.tail)) {
		bam_error(BAD_ZFS_MNTED, pool);
		filelist_free(&flist);
		return (NULL);
	}

	is_mounted = strtok(flist.head->line, " \t\n");
	INJECT_ERROR1("Z_MOUNT_LEG_GET_MOUNTED_STRTOK_YES", is_mounted = "yes");
	INJECT_ERROR1("Z_MOUNT_LEG_GET_MOUNTED_STRTOK_NO", is_mounted = "no");
	if (strcmp(is_mounted, "no") != 0) {
		filelist_free(&flist);
		*mnted = LEGACY_ALREADY;
		/* get_mountpoint returns a strdup'ed string */
		BAM_DPRINTF((D_Z_MOUNT_TOP_LEG_ALREADY, fcn, pool));
		return (get_mountpoint(pool, "zfs"));
	}

	filelist_free(&flist);

	/*
	 * legacy top dataset is not mounted. Mount it now
	 * First create a mountpoint.
	 */
	(void) snprintf(tmpmnt, sizeof (tmpmnt), "%s.%d",
	    ZFS_LEGACY_MNTPT, getpid());

	ret = stat(tmpmnt, &sb);
	if (ret == -1) {
		BAM_DPRINTF((D_Z_MOUNT_TOP_LEG_MNTPT_ABS, fcn, pool, tmpmnt));
		ret = mkdirp(tmpmnt, 0755);
		INJECT_ERROR1("Z_MOUNT_TOP_LEG_MNTPT_MKDIRP", ret = -1);
		if (ret == -1) {
			bam_error(MKDIR_FAILED, tmpmnt, strerror(errno));
			return (NULL);
		}
	} else {
		BAM_DPRINTF((D_Z_MOUNT_TOP_LEG_MNTPT_PRES, fcn, pool, tmpmnt));
	}

	(void) snprintf(cmd, sizeof (cmd),
	    "/sbin/mount -F zfs %s %s",
	    pool, tmpmnt);

	ret = exec_cmd(cmd, NULL);
	INJECT_ERROR1("Z_MOUNT_TOP_LEG_MOUNT_CMD", ret = 1);
	if (ret != 0) {
		bam_error(ZFS_MOUNT_FAILED, pool);
		(void) rmdir(tmpmnt);
		return (NULL);
	}

	*mnted = LEGACY_MOUNTED;
	BAM_DPRINTF((D_Z_MOUNT_TOP_LEG_MOUNTED, fcn, pool, tmpmnt));
	return (s_strdup(tmpmnt));
}

/*
 * Mounts the top dataset (if needed)
 * Returns:	The mountpoint of the top dataset or NULL on error
 * 		mnted returns one of the above values defined for zfs_mnted_t
 */
static char *
mount_top_dataset(char *pool, zfs_mnted_t *mnted)
{
	char		cmd[PATH_MAX];
	filelist_t	flist = {0};
	char		*is_mounted;
	char		*mntpt;
	char		*zmntpt;
	int		ret;
	const char	*fcn = "mount_top_dataset()";

	*mnted = ZFS_MNT_ERROR;

	BAM_DPRINTF((D_FUNC_ENTRY1, fcn, pool));

	/*
	 * First check if the top dataset is a "legacy" dataset
	 */
	(void) snprintf(cmd, sizeof (cmd),
	    "/sbin/zfs get -Ho value mountpoint %s",
	    pool);
	ret = exec_cmd(cmd, &flist);
	INJECT_ERROR1("Z_MOUNT_TOP_GET_MNTPT", ret = 1);
	if (ret != 0) {
		bam_error(ZFS_MNTPT_FAILED, pool);
		return (NULL);
	}

	if (flist.head && (flist.head == flist.tail)) {
		char *legacy = strtok(flist.head->line, " \t\n");
		if (legacy && strcmp(legacy, "legacy") == 0) {
			filelist_free(&flist);
			BAM_DPRINTF((D_Z_IS_LEGACY, fcn, pool));
			return (mount_legacy_dataset(pool, mnted));
		}
	}

	filelist_free(&flist);

	BAM_DPRINTF((D_Z_IS_NOT_LEGACY, fcn, pool));

	(void) snprintf(cmd, sizeof (cmd),
	    "/sbin/zfs get -Ho value mounted %s",
	    pool);

	ret = exec_cmd(cmd, &flist);
	INJECT_ERROR1("Z_MOUNT_TOP_NONLEG_GET_MOUNTED", ret = 1);
	if (ret != 0) {
		bam_error(ZFS_MNTED_FAILED, pool);
		return (NULL);
	}

	INJECT_ERROR1("Z_MOUNT_TOP_NONLEG_GET_MOUNTED_VAL", flist.head = NULL);
	if ((flist.head == NULL) || (flist.head != flist.tail)) {
		bam_error(BAD_ZFS_MNTED, pool);
		filelist_free(&flist);
		return (NULL);
	}

	is_mounted = strtok(flist.head->line, " \t\n");
	INJECT_ERROR1("Z_MOUNT_TOP_NONLEG_GET_MOUNTED_YES", is_mounted = "yes");
	INJECT_ERROR1("Z_MOUNT_TOP_NONLEG_GET_MOUNTED_NO", is_mounted = "no");
	if (strcmp(is_mounted, "no") != 0) {
		filelist_free(&flist);
		*mnted = ZFS_ALREADY;
		BAM_DPRINTF((D_Z_MOUNT_TOP_NONLEG_MOUNTED_ALREADY, fcn, pool));
		goto mounted;
	}

	filelist_free(&flist);
	BAM_DPRINTF((D_Z_MOUNT_TOP_NONLEG_MOUNTED_NOT_ALREADY, fcn, pool));

	/* top dataset is not mounted. Mount it now */
	(void) snprintf(cmd, sizeof (cmd),
	    "/sbin/zfs mount %s", pool);
	ret = exec_cmd(cmd, NULL);
	INJECT_ERROR1("Z_MOUNT_TOP_NONLEG_MOUNT_CMD", ret = 1);
	if (ret != 0) {
		bam_error(ZFS_MOUNT_FAILED, pool);
		return (NULL);
	}
	*mnted = ZFS_MOUNTED;
	BAM_DPRINTF((D_Z_MOUNT_TOP_NONLEG_MOUNTED_NOW, fcn, pool));
	/*FALLTHRU*/
mounted:
	/*
	 * Now get the mountpoint
	 */
	(void) snprintf(cmd, sizeof (cmd),
	    "/sbin/zfs get -Ho value mountpoint %s",
	    pool);

	ret = exec_cmd(cmd, &flist);
	INJECT_ERROR1("Z_MOUNT_TOP_NONLEG_GET_MNTPT_CMD", ret = 1);
	if (ret != 0) {
		bam_error(ZFS_MNTPT_FAILED, pool);
		goto error;
	}

	INJECT_ERROR1("Z_MOUNT_TOP_NONLEG_GET_MNTPT_OUT", flist.head = NULL);
	if ((flist.head == NULL) || (flist.head != flist.tail)) {
		bam_error(NULL_ZFS_MNTPT, pool);
		goto error;
	}

	mntpt = strtok(flist.head->line, " \t\n");
	INJECT_ERROR1("Z_MOUNT_TOP_NONLEG_GET_MNTPT_STRTOK", mntpt = "foo");
	if (*mntpt != '/') {
		bam_error(BAD_ZFS_MNTPT, pool, mntpt);
		goto error;
	}
	zmntpt = s_strdup(mntpt);

	filelist_free(&flist);

	BAM_DPRINTF((D_Z_MOUNT_TOP_NONLEG_MNTPT, fcn, pool, zmntpt));

	return (zmntpt);

error:
	filelist_free(&flist);
	(void) umount_top_dataset(pool, *mnted, NULL);
	BAM_DPRINTF((D_RETURN_FAILURE, fcn));
	return (NULL);
}

static int
umount_top_dataset(char *pool, zfs_mnted_t mnted, char *mntpt)
{
	char		cmd[PATH_MAX];
	int		ret;
	const char	*fcn = "umount_top_dataset()";

	INJECT_ERROR1("Z_UMOUNT_TOP_INVALID_STATE", mnted = ZFS_MNT_ERROR);
	switch (mnted) {
	case LEGACY_ALREADY:
	case ZFS_ALREADY:
		/* nothing to do */
		BAM_DPRINTF((D_Z_UMOUNT_TOP_ALREADY_NOP, fcn, pool,
		    mntpt ? mntpt : "NULL"));
		free(mntpt);
		return (BAM_SUCCESS);
	case LEGACY_MOUNTED:
		(void) snprintf(cmd, sizeof (cmd),
		    "/sbin/umount %s", pool);
		ret = exec_cmd(cmd, NULL);
		INJECT_ERROR1("Z_UMOUNT_TOP_LEGACY_UMOUNT_FAIL", ret = 1);
		if (ret != 0) {
			bam_error(UMOUNT_FAILED, pool);
			free(mntpt);
			return (BAM_ERROR);
		}
		if (mntpt)
			(void) rmdir(mntpt);
		free(mntpt);
		BAM_DPRINTF((D_Z_UMOUNT_TOP_LEGACY, fcn, pool));
		return (BAM_SUCCESS);
	case ZFS_MOUNTED:
		free(mntpt);
		(void) snprintf(cmd, sizeof (cmd),
		    "/sbin/zfs unmount %s", pool);
		ret = exec_cmd(cmd, NULL);
		INJECT_ERROR1("Z_UMOUNT_TOP_NONLEG_UMOUNT_FAIL", ret = 1);
		if (ret != 0) {
			bam_error(UMOUNT_FAILED, pool);
			return (BAM_ERROR);
		}
		BAM_DPRINTF((D_Z_UMOUNT_TOP_NONLEG, fcn, pool));
		return (BAM_SUCCESS);
	default:
		bam_error(INT_BAD_MNTSTATE, pool);
		return (BAM_ERROR);
	}
	/*NOTREACHED*/
}

/*
 * For ZFS, osdev can be one of two forms
 * It can be a "special" file as seen in mnttab: rpool/ROOT/szboot_0402
 * It can be a /dev/[r]dsk special file. We handle both instances
 */
static char *
get_pool(char *osdev)
{
	char		cmd[PATH_MAX];
	char		buf[PATH_MAX];
	filelist_t	flist = {0};
	char		*pool;
	char		*cp;
	char		*slash;
	int		ret;
	const char	*fcn = "get_pool()";

	INJECT_ERROR1("GET_POOL_OSDEV", osdev = NULL);
	if (osdev == NULL) {
		bam_error(GET_POOL_OSDEV_NULL);
		return (NULL);
	}

	BAM_DPRINTF((D_GET_POOL_OSDEV, fcn, osdev));

	if (osdev[0] != '/') {
		(void) strlcpy(buf, osdev, sizeof (buf));
		slash = strchr(buf, '/');
		if (slash)
			*slash = '\0';
		pool = s_strdup(buf);
		BAM_DPRINTF((D_GET_POOL_RET, fcn, pool));
		return (pool);
	} else if (strncmp(osdev, "/dev/dsk/", strlen("/dev/dsk/")) != 0 &&
	    strncmp(osdev, "/dev/rdsk/", strlen("/dev/rdsk/")) != 0) {
		bam_error(GET_POOL_BAD_OSDEV, osdev);
		return (NULL);
	}

	(void) snprintf(cmd, sizeof (cmd),
	    "/usr/sbin/fstyp -a %s 2>/dev/null | /bin/grep '^name:'",
	    osdev);

	ret = exec_cmd(cmd, &flist);
	INJECT_ERROR1("GET_POOL_FSTYP", ret = 1);
	if (ret != 0) {
		bam_error(FSTYP_A_FAILED, osdev);
		return (NULL);
	}

	INJECT_ERROR1("GET_POOL_FSTYP_OUT", flist.head = NULL);
	if ((flist.head == NULL) || (flist.head != flist.tail)) {
		bam_error(NULL_FSTYP_A, osdev);
		filelist_free(&flist);
		return (NULL);
	}

	(void) strtok(flist.head->line, "'");
	cp = strtok(NULL, "'");
	INJECT_ERROR1("GET_POOL_FSTYP_STRTOK", cp = NULL);
	if (cp == NULL) {
		bam_error(BAD_FSTYP_A, osdev);
		filelist_free(&flist);
		return (NULL);
	}

	pool = s_strdup(cp);

	filelist_free(&flist);

	BAM_DPRINTF((D_GET_POOL_RET, fcn, pool));

	return (pool);
}

static char *
find_zfs_existing(char *osdev)
{
	char		*pool;
	zfs_mnted_t	mnted;
	char		*mntpt;
	char		*sign;
	const char	*fcn = "find_zfs_existing()";

	pool = get_pool(osdev);
	INJECT_ERROR1("ZFS_FIND_EXIST_POOL", pool = NULL);
	if (pool == NULL) {
		bam_error(ZFS_GET_POOL_FAILED, osdev);
		return (NULL);
	}

	mntpt = mount_top_dataset(pool, &mnted);
	INJECT_ERROR1("ZFS_FIND_EXIST_MOUNT_TOP", mntpt = NULL);
	if (mntpt == NULL) {
		bam_error(ZFS_MOUNT_TOP_DATASET_FAILED, pool);
		free(pool);
		return (NULL);
	}

	sign = find_primary_common(mntpt, "zfs");
	if (sign == NULL) {
		sign = find_backup_common(mntpt, "zfs");
		BAM_DPRINTF((D_EXIST_BACKUP_SIGN, fcn, sign ? sign : "NULL"));
	} else {
		BAM_DPRINTF((D_EXIST_PRIMARY_SIGN, fcn, sign));
	}

	(void) umount_top_dataset(pool, mnted, mntpt);

	free(pool);

	return (sign);
}

static char *
find_existing_sign(char *osroot, char *osdev, char *fstype)
{
	const char		*fcn = "find_existing_sign()";

	INJECT_ERROR1("FIND_EXIST_NOTSUP_FS", fstype = "foofs");
	if (strcmp(fstype, "ufs") == 0) {
		BAM_DPRINTF((D_CHECK_UFS_EXIST_SIGN, fcn));
		return (find_ufs_existing(osroot));
	} else if (strcmp(fstype, "zfs") == 0) {
		BAM_DPRINTF((D_CHECK_ZFS_EXIST_SIGN, fcn));
		return (find_zfs_existing(osdev));
	} else {
		bam_error(GRUBSIGN_NOTSUP, fstype);
		return (NULL);
	}
}

#define	MH_HASH_SZ	16

typedef enum {
	MH_ERROR = -1,
	MH_NOMATCH,
	MH_MATCH
} mh_search_t;

typedef struct mcache {
	char	*mc_special;
	char	*mc_mntpt;
	char	*mc_fstype;
	struct mcache *mc_next;
} mcache_t;

typedef struct mhash {
	mcache_t *mh_hash[MH_HASH_SZ];
} mhash_t;

static int
mhash_fcn(char *key)
{
	int		i;
	uint64_t	sum = 0;

	for (i = 0; key[i] != '\0'; i++) {
		sum += (uchar_t)key[i];
	}

	sum %= MH_HASH_SZ;

	assert(sum < MH_HASH_SZ);

	return (sum);
}

static mhash_t *
cache_mnttab(void)
{
	FILE		*mfp;
	struct extmnttab mnt;
	mcache_t	*mcp;
	mhash_t		*mhp;
	char		*ctds;
	int		idx;
	int		error;
	char		*special_dup;
	const char	*fcn = "cache_mnttab()";

	mfp = fopen(MNTTAB, "r");
	error = errno;
	INJECT_ERROR1("CACHE_MNTTAB_MNTTAB_ERR", mfp = NULL);
	if (mfp == NULL) {
		bam_error(OPEN_FAIL, MNTTAB, strerror(error));
		return (NULL);
	}

	mhp = s_calloc(1, sizeof (mhash_t));

	resetmnttab(mfp);

	while (getextmntent(mfp, &mnt, sizeof (mnt)) == 0) {
		/* only cache ufs */
		if (strcmp(mnt.mnt_fstype, "ufs") != 0)
			continue;

		/* basename() modifies its arg, so dup it */
		special_dup = s_strdup(mnt.mnt_special);
		ctds = basename(special_dup);

		mcp = s_calloc(1, sizeof (mcache_t));
		mcp->mc_special = s_strdup(ctds);
		mcp->mc_mntpt = s_strdup(mnt.mnt_mountp);
		mcp->mc_fstype = s_strdup(mnt.mnt_fstype);
		BAM_DPRINTF((D_CACHE_MNTS, fcn, ctds,
		    mnt.mnt_mountp, mnt.mnt_fstype));
		idx = mhash_fcn(ctds);
		mcp->mc_next = mhp->mh_hash[idx];
		mhp->mh_hash[idx] = mcp;
		free(special_dup);
	}

	(void) fclose(mfp);

	return (mhp);
}

static void
free_mnttab(mhash_t *mhp)
{
	mcache_t	*mcp;
	int		i;

	for (i = 0; i < MH_HASH_SZ; i++) {
		/*LINTED*/
		while (mcp = mhp->mh_hash[i]) {
			mhp->mh_hash[i] = mcp->mc_next;
			free(mcp->mc_special);
			free(mcp->mc_mntpt);
			free(mcp->mc_fstype);
			free(mcp);
		}
	}

	for (i = 0; i < MH_HASH_SZ; i++) {
		assert(mhp->mh_hash[i] == NULL);
	}
	free(mhp);
}

static mh_search_t
search_hash(mhash_t *mhp, char *special, char **mntpt)
{
	int		idx;
	mcache_t	*mcp;
	const char 	*fcn = "search_hash()";

	assert(mntpt);

	*mntpt = NULL;

	INJECT_ERROR1("SEARCH_HASH_FULL_PATH", special = "/foo");
	if (strchr(special, '/')) {
		bam_error(INVALID_MHASH_KEY, special);
		return (MH_ERROR);
	}

	idx = mhash_fcn(special);

	for (mcp = mhp->mh_hash[idx]; mcp; mcp = mcp->mc_next) {
		if (strcmp(mcp->mc_special, special) == 0)
			break;
	}

	if (mcp == NULL) {
		BAM_DPRINTF((D_MNTTAB_HASH_NOMATCH, fcn, special));
		return (MH_NOMATCH);
	}

	assert(strcmp(mcp->mc_fstype, "ufs") == 0);
	*mntpt = mcp->mc_mntpt;
	BAM_DPRINTF((D_MNTTAB_HASH_MATCH, fcn, special));
	return (MH_MATCH);
}

static int
check_add_ufs_sign_to_list(FILE *tfp, char *mntpt)
{
	char		*sign;
	char		*signline;
	char		signbuf[MAXNAMELEN];
	int		len;
	int		error;
	const char	*fcn = "check_add_ufs_sign_to_list()";

	/* safe to specify NULL as "osdev" arg for UFS */
	sign = find_existing_sign(mntpt, NULL, "ufs");
	if (sign == NULL) {
		/* No existing signature, nothing to add to list */
		BAM_DPRINTF((D_NO_SIGN_TO_LIST, fcn, mntpt));
		return (0);
	}

	(void) snprintf(signbuf, sizeof (signbuf), "%s\n", sign);
	signline = signbuf;

	INJECT_ERROR1("UFS_MNTPT_SIGN_NOTUFS", signline = "pool_rpool10\n");
	if (strncmp(signline, GRUBSIGN_UFS_PREFIX,
	    strlen(GRUBSIGN_UFS_PREFIX))) {
		bam_error(INVALID_UFS_SIGNATURE, sign);
		free(sign);
		/* ignore invalid signatures */
		return (0);
	}

	len = fputs(signline, tfp);
	error = errno;
	INJECT_ERROR1("SIGN_LIST_PUTS_ERROR", len = 0);
	if (len != strlen(signline)) {
		bam_error(SIGN_LIST_FPUTS_ERR, sign, strerror(error));
		free(sign);
		return (-1);
	}

	free(sign);

	BAM_DPRINTF((D_SIGN_LIST_PUTS_DONE, fcn, mntpt));
	return (0);
}

/*
 * slice is a basename not a full pathname
 */
static int
process_slice_common(char *slice, FILE *tfp, mhash_t *mhp, char *tmpmnt)
{
	int		ret;
	char		cmd[PATH_MAX];
	char		path[PATH_MAX];
	struct stat	sbuf;
	char		*mntpt;
	filelist_t	flist = {0};
	char		*fstype;
	char		blkslice[PATH_MAX];
	const char	*fcn = "process_slice_common()";


	ret = search_hash(mhp, slice, &mntpt);
	switch (ret) {
		case MH_MATCH:
			if (check_add_ufs_sign_to_list(tfp, mntpt) == -1)
				return (-1);
			else
				return (0);
		case MH_NOMATCH:
			break;
		case MH_ERROR:
		default:
			return (-1);
	}

	(void) snprintf(path, sizeof (path), "/dev/rdsk/%s", slice);
	if (stat(path, &sbuf) == -1) {
		BAM_DPRINTF((D_SLICE_ENOENT, fcn, path));
		return (0);
	}

	/* Check if ufs */
	(void) snprintf(cmd, sizeof (cmd),
	    "/usr/sbin/fstyp /dev/rdsk/%s 2>/dev/null",
	    slice);

	if (exec_cmd(cmd, &flist) != 0) {
		if (bam_verbose)
			bam_print(FSTYP_FAILED, slice);
		return (0);
	}

	if ((flist.head == NULL) || (flist.head != flist.tail)) {
		if (bam_verbose)
			bam_print(FSTYP_BAD, slice);
		filelist_free(&flist);
		return (0);
	}

	fstype = strtok(flist.head->line, " \t\n");
	if (fstype == NULL || strcmp(fstype, "ufs") != 0) {
		if (bam_verbose)
			bam_print(NOT_UFS_SLICE, slice, fstype);
		filelist_free(&flist);
		return (0);
	}

	filelist_free(&flist);

	/*
	 * Since we are mounting the filesystem read-only, the
	 * the last mount field of the superblock is unchanged
	 * and does not need to be fixed up post-mount;
	 */

	(void) snprintf(blkslice, sizeof (blkslice), "/dev/dsk/%s",
	    slice);

	(void) snprintf(cmd, sizeof (cmd),
	    "/usr/sbin/mount -F ufs -o ro %s %s "
	    "> /dev/null 2>&1", blkslice, tmpmnt);

	if (exec_cmd(cmd, NULL) != 0) {
		if (bam_verbose)
			bam_print(MOUNT_FAILED, blkslice, "ufs");
		return (0);
	}

	ret = check_add_ufs_sign_to_list(tfp, tmpmnt);

	(void) snprintf(cmd, sizeof (cmd),
	    "/usr/sbin/umount -f %s > /dev/null 2>&1",
	    tmpmnt);

	if (exec_cmd(cmd, NULL) != 0) {
		bam_print(UMOUNT_FAILED, slice);
		return (0);
	}

	return (ret);
}

static int
process_vtoc_slices(
	char *s0,
	struct vtoc *vtoc,
	FILE *tfp,
	mhash_t *mhp,
	char *tmpmnt)
{
	int		idx;
	char		slice[PATH_MAX];
	size_t		len;
	char		*cp;
	const char	*fcn = "process_vtoc_slices()";

	len = strlen(s0);

	assert(s0[len - 2] == 's' && s0[len - 1] == '0');

	s0[len - 1] = '\0';

	(void) strlcpy(slice, s0, sizeof (slice));

	s0[len - 1] = '0';

	cp = slice + len - 1;

	for (idx = 0; idx < vtoc->v_nparts; idx++) {

		(void) snprintf(cp, sizeof (slice) - (len - 1), "%u", idx);

		if (vtoc->v_part[idx].p_size == 0) {
			BAM_DPRINTF((D_VTOC_SIZE_ZERO, fcn, slice));
			continue;
		}

		/* Skip "SWAP", "USR", "BACKUP", "VAR", "HOME", "ALTSCTR" */
		switch (vtoc->v_part[idx].p_tag) {
		case V_SWAP:
		case V_USR:
		case V_BACKUP:
		case V_VAR:
		case V_HOME:
		case V_ALTSCTR:
			BAM_DPRINTF((D_VTOC_NOT_ROOT_TAG, fcn, slice));
			continue;
		default:
			BAM_DPRINTF((D_VTOC_ROOT_TAG, fcn, slice));
			break;
		}

		/* skip unmountable and readonly slices */
		switch (vtoc->v_part[idx].p_flag) {
		case V_UNMNT:
		case V_RONLY:
			BAM_DPRINTF((D_VTOC_NOT_RDWR_FLAG, fcn, slice));
			continue;
		default:
			BAM_DPRINTF((D_VTOC_RDWR_FLAG, fcn, slice));
			break;
		}

		if (process_slice_common(slice, tfp, mhp, tmpmnt) == -1) {
			return (-1);
		}
	}

	return (0);
}

static int
process_efi_slices(
	char *s0,
	struct dk_gpt *efi,
	FILE *tfp,
	mhash_t *mhp,
	char *tmpmnt)
{
	int		idx;
	char		slice[PATH_MAX];
	size_t		len;
	char		*cp;
	const char	*fcn = "process_efi_slices()";

	len = strlen(s0);

	assert(s0[len - 2] == 's' && s0[len - 1] == '0');

	s0[len - 1] = '\0';

	(void) strlcpy(slice, s0, sizeof (slice));

	s0[len - 1] = '0';

	cp = slice + len - 1;

	for (idx = 0; idx < efi->efi_nparts; idx++) {

		(void) snprintf(cp, sizeof (slice) - (len - 1), "%u", idx);

		if (efi->efi_parts[idx].p_size == 0) {
			BAM_DPRINTF((D_EFI_SIZE_ZERO, fcn, slice));
			continue;
		}

		/* Skip "SWAP", "USR", "BACKUP", "VAR", "HOME", "ALTSCTR" */
		switch (efi->efi_parts[idx].p_tag) {
		case V_SWAP:
		case V_USR:
		case V_BACKUP:
		case V_VAR:
		case V_HOME:
		case V_ALTSCTR:
			BAM_DPRINTF((D_EFI_NOT_ROOT_TAG, fcn, slice));
			continue;
		default:
			BAM_DPRINTF((D_EFI_ROOT_TAG, fcn, slice));
			break;
		}

		/* skip unmountable and readonly slices */
		switch (efi->efi_parts[idx].p_flag) {
		case V_UNMNT:
		case V_RONLY:
			BAM_DPRINTF((D_EFI_NOT_RDWR_FLAG, fcn, slice));
			continue;
		default:
			BAM_DPRINTF((D_EFI_RDWR_FLAG, fcn, slice));
			break;
		}

		if (process_slice_common(slice, tfp, mhp, tmpmnt) == -1) {
			return (-1);
		}
	}

	return (0);
}

/*
 * s0 is a basename not a full path
 */
static int
process_slice0(char *s0, FILE *tfp, mhash_t *mhp, char *tmpmnt)
{
	struct vtoc		vtoc;
	struct dk_gpt		*efi;
	char			s0path[PATH_MAX];
	struct stat		sbuf;
	int			e_flag;
	int			v_flag;
	int			retval;
	int			err;
	int			fd;
	const char		*fcn = "process_slice0()";

	(void) snprintf(s0path, sizeof (s0path), "/dev/rdsk/%s", s0);

	if (stat(s0path, &sbuf) == -1) {
		BAM_DPRINTF((D_SLICE0_ENOENT, fcn, s0path));
		return (0);
	}

	fd = open(s0path, O_NONBLOCK|O_RDONLY);
	if (fd == -1) {
		bam_error(OPEN_FAIL, s0path, strerror(errno));
		return (0);
	}

	e_flag = v_flag = 0;
	retval = ((err = read_vtoc(fd, &vtoc)) >= 0) ? 0 : err;
	switch (retval) {
		case VT_EIO:
			BAM_DPRINTF((D_VTOC_READ_FAIL, fcn, s0path));
			break;
		case VT_EINVAL:
			BAM_DPRINTF((D_VTOC_INVALID, fcn, s0path));
			break;
		case VT_ERROR:
			BAM_DPRINTF((D_VTOC_UNKNOWN_ERR, fcn, s0path));
			break;
		case VT_ENOTSUP:
			e_flag = 1;
			BAM_DPRINTF((D_VTOC_NOTSUP, fcn, s0path));
			break;
		case 0:
			v_flag = 1;
			BAM_DPRINTF((D_VTOC_READ_SUCCESS, fcn, s0path));
			break;
		default:
			BAM_DPRINTF((D_VTOC_UNKNOWN_RETCODE, fcn, s0path));
			break;
	}


	if (e_flag) {
		e_flag = 0;
		retval = ((err = efi_alloc_and_read(fd, &efi)) >= 0) ? 0 : err;
		switch (retval) {
		case VT_EIO:
			BAM_DPRINTF((D_EFI_READ_FAIL, fcn, s0path));
			break;
		case VT_EINVAL:
			BAM_DPRINTF((D_EFI_INVALID, fcn, s0path));
			break;
		case VT_ERROR:
			BAM_DPRINTF((D_EFI_UNKNOWN_ERR, fcn, s0path));
			break;
		case VT_ENOTSUP:
			BAM_DPRINTF((D_EFI_NOTSUP, fcn, s0path));
			break;
		case 0:
			e_flag = 1;
			BAM_DPRINTF((D_EFI_READ_SUCCESS, fcn, s0path));
			break;
		default:
			BAM_DPRINTF((D_EFI_UNKNOWN_RETCODE, fcn, s0path));
			break;
		}
	}

	(void) close(fd);

	if (v_flag) {
		retval = process_vtoc_slices(s0,
		    &vtoc, tfp, mhp, tmpmnt);
	} else if (e_flag) {
		retval = process_efi_slices(s0,
		    efi, tfp, mhp, tmpmnt);
	} else {
		BAM_DPRINTF((D_NOT_VTOC_OR_EFI, fcn, s0path));
		return (0);
	}

	return (retval);
}

/*
 * Find and create a list of all existing UFS boot signatures
 */
static int
FindAllUfsSignatures(void)
{
	mhash_t		*mnttab_hash;
	DIR		*dirp = NULL;
	struct dirent	*dp;
	char		tmpmnt[PATH_MAX];
	char		cmd[PATH_MAX];
	struct stat	sb;
	int		fd;
	FILE		*tfp;
	size_t		len;
	int		ret;
	int		error;
	const char	*fcn = "FindAllUfsSignatures()";

	if (stat(UFS_SIGNATURE_LIST, &sb) != -1)  {
		bam_print(SIGNATURE_LIST_EXISTS, UFS_SIGNATURE_LIST);
		return (0);
	}

	fd = open(UFS_SIGNATURE_LIST".tmp",
	    O_RDWR|O_CREAT|O_TRUNC, 0644);
	error = errno;
	INJECT_ERROR1("SIGN_LIST_TMP_TRUNC", fd = -1);
	if (fd == -1) {
		bam_error(OPEN_FAIL, UFS_SIGNATURE_LIST".tmp", strerror(error));
		return (-1);
	}

	ret = close(fd);
	error = errno;
	INJECT_ERROR1("SIGN_LIST_TMP_CLOSE", ret = -1);
	if (ret == -1) {
		bam_error(CLOSE_FAIL, UFS_SIGNATURE_LIST".tmp",
		    strerror(error));
		(void) unlink(UFS_SIGNATURE_LIST".tmp");
		return (-1);
	}

	tfp = fopen(UFS_SIGNATURE_LIST".tmp", "a");
	error = errno;
	INJECT_ERROR1("SIGN_LIST_APPEND_FOPEN", tfp = NULL);
	if (tfp == NULL) {
		bam_error(OPEN_FAIL, UFS_SIGNATURE_LIST".tmp", strerror(error));
		(void) unlink(UFS_SIGNATURE_LIST".tmp");
		return (-1);
	}

	mnttab_hash = cache_mnttab();
	INJECT_ERROR1("CACHE_MNTTAB_ERROR", mnttab_hash = NULL);
	if (mnttab_hash == NULL) {
		(void) fclose(tfp);
		(void) unlink(UFS_SIGNATURE_LIST".tmp");
		bam_error(CACHE_MNTTAB_FAIL, fcn);
		return (-1);
	}

	(void) snprintf(tmpmnt, sizeof (tmpmnt),
	    "/tmp/bootadm_ufs_sign_mnt.%d", getpid());
	(void) unlink(tmpmnt);

	ret = mkdirp(tmpmnt, 0755);
	error = errno;
	INJECT_ERROR1("MKDIRP_SIGN_MNT", ret = -1);
	if (ret == -1) {
		bam_error(MKDIR_FAILED, tmpmnt, strerror(error));
		free_mnttab(mnttab_hash);
		(void) fclose(tfp);
		(void) unlink(UFS_SIGNATURE_LIST".tmp");
		return (-1);
	}

	dirp = opendir("/dev/rdsk");
	error = errno;
	INJECT_ERROR1("OPENDIR_DEV_RDSK", dirp = NULL);
	if (dirp == NULL) {
		bam_error(OPENDIR_FAILED, "/dev/rdsk", strerror(error));
		goto fail;
	}

	while (dp = readdir(dirp)) {
		if (strcmp(dp->d_name, ".") == 0 ||
		    strcmp(dp->d_name, "..") == 0)
			continue;

		/*
		 * we only look for the s0 slice. This is guranteed to
		 * have 's' at len - 2.
		 */
		len = strlen(dp->d_name);
		if (dp->d_name[len - 2 ] != 's' || dp->d_name[len - 1] != '0') {
			BAM_DPRINTF((D_SKIP_SLICE_NOTZERO, fcn, dp->d_name));
			continue;
		}

		ret = process_slice0(dp->d_name, tfp, mnttab_hash, tmpmnt);
		INJECT_ERROR1("PROCESS_S0_FAIL", ret = -1);
		if (ret == -1)
			goto fail;
	}

	(void) closedir(dirp);
	free_mnttab(mnttab_hash);
	(void) rmdir(tmpmnt);

	ret = fclose(tfp);
	error = errno;
	INJECT_ERROR1("FCLOSE_SIGNLIST_TMP", ret = EOF);
	if (ret == EOF) {
		bam_error(CLOSE_FAIL, UFS_SIGNATURE_LIST".tmp",
		    strerror(error));
		(void) unlink(UFS_SIGNATURE_LIST".tmp");
		return (-1);
	}

	/* We have a list of existing GRUB signatures. Sort it first */
	(void) snprintf(cmd, sizeof (cmd),
	    "/usr/bin/sort -u %s.tmp > %s.sorted",
	    UFS_SIGNATURE_LIST, UFS_SIGNATURE_LIST);

	ret = exec_cmd(cmd, NULL);
	INJECT_ERROR1("SORT_SIGN_LIST", ret = 1);
	if (ret != 0) {
		bam_error(GRUBSIGN_SORT_FAILED);
		(void) unlink(UFS_SIGNATURE_LIST".sorted");
		(void) unlink(UFS_SIGNATURE_LIST".tmp");
		return (-1);
	}

	(void) unlink(UFS_SIGNATURE_LIST".tmp");

	ret = rename(UFS_SIGNATURE_LIST".sorted", UFS_SIGNATURE_LIST);
	error = errno;
	INJECT_ERROR1("RENAME_TMP_SIGNLIST", ret = -1);
	if (ret == -1) {
		bam_error(RENAME_FAIL, UFS_SIGNATURE_LIST, strerror(error));
		(void) unlink(UFS_SIGNATURE_LIST".sorted");
		return (-1);
	}

	if (stat(UFS_SIGNATURE_LIST, &sb) == 0 && sb.st_size == 0) {
		BAM_DPRINTF((D_ZERO_LEN_SIGNLIST, fcn, UFS_SIGNATURE_LIST));
	}

	BAM_DPRINTF((D_RETURN_SUCCESS, fcn));
	return (0);

fail:
	if (dirp)
		(void) closedir(dirp);
	free_mnttab(mnttab_hash);
	(void) rmdir(tmpmnt);
	(void) fclose(tfp);
	(void) unlink(UFS_SIGNATURE_LIST".tmp");
	BAM_DPRINTF((D_RETURN_FAILURE, fcn));
	return (-1);
}

static char *
create_ufs_sign(void)
{
	struct stat	sb;
	int		signnum = -1;
	char		tmpsign[MAXNAMELEN + 1];
	char		*numstr;
	int		i;
	FILE		*tfp;
	int		ret;
	int		error;
	const char	*fcn = "create_ufs_sign()";

	bam_print(SEARCHING_UFS_SIGN);

	ret = FindAllUfsSignatures();
	INJECT_ERROR1("FIND_ALL_UFS", ret = -1);
	if (ret == -1) {
		bam_error(ERR_FIND_UFS_SIGN);
		return (NULL);
	}

	/* Make sure the list exists and is owned by root */
	INJECT_ERROR1("SIGNLIST_NOT_CREATED",
	    (void) unlink(UFS_SIGNATURE_LIST));
	if (stat(UFS_SIGNATURE_LIST, &sb) == -1 || sb.st_uid != 0) {
		(void) unlink(UFS_SIGNATURE_LIST);
		bam_error(UFS_SIGNATURE_LIST_MISS, UFS_SIGNATURE_LIST);
		return (NULL);
	}

	if (sb.st_size == 0) {
		bam_print(GRUBSIGN_UFS_NONE);
		i = 0;
		goto found;
	}

	/* The signature list was sorted when it was created */
	tfp = fopen(UFS_SIGNATURE_LIST, "r");
	error = errno;
	INJECT_ERROR1("FOPEN_SIGN_LIST", tfp = NULL);
	if (tfp == NULL) {
		bam_error(UFS_SIGNATURE_LIST_OPENERR,
		    UFS_SIGNATURE_LIST, strerror(error));
		(void) unlink(UFS_SIGNATURE_LIST);
		return (NULL);
	}

	for (i = 0; s_fgets(tmpsign, sizeof (tmpsign), tfp); i++) {

		if (strncmp(tmpsign, GRUBSIGN_UFS_PREFIX,
		    strlen(GRUBSIGN_UFS_PREFIX)) != 0) {
			(void) fclose(tfp);
			(void) unlink(UFS_SIGNATURE_LIST);
			bam_error(UFS_BADSIGN, tmpsign);
			return (NULL);
		}
		numstr = tmpsign + strlen(GRUBSIGN_UFS_PREFIX);

		if (numstr[0] == '\0' || !isdigit(numstr[0])) {
			(void) fclose(tfp);
			(void) unlink(UFS_SIGNATURE_LIST);
			bam_error(UFS_BADSIGN, tmpsign);
			return (NULL);
		}

		signnum = atoi(numstr);
		INJECT_ERROR1("NEGATIVE_SIGN", signnum = -1);
		if (signnum < 0) {
			(void) fclose(tfp);
			(void) unlink(UFS_SIGNATURE_LIST);
			bam_error(UFS_BADSIGN, tmpsign);
			return (NULL);
		}

		if (i != signnum) {
			BAM_DPRINTF((D_FOUND_HOLE_SIGNLIST, fcn, i));
			break;
		}
	}

	(void) fclose(tfp);

found:
	(void) snprintf(tmpsign, sizeof (tmpsign), "rootfs%d", i);

	/* add the ufs signature to the /var/run list of signatures */
	ret = ufs_add_to_sign_list(tmpsign);
	INJECT_ERROR1("UFS_ADD_TO_SIGN_LIST", ret = -1);
	if (ret == -1) {
		(void) unlink(UFS_SIGNATURE_LIST);
		bam_error(FAILED_ADD_SIGNLIST, tmpsign);
		return (NULL);
	}

	BAM_DPRINTF((D_RETURN_SUCCESS, fcn));

	return (s_strdup(tmpsign));
}

static char *
get_fstype(char *osroot)
{
	FILE		*mntfp;
	struct mnttab	mp = {0};
	struct mnttab	mpref = {0};
	int		error;
	int		ret;
	const char	*fcn = "get_fstype()";

	INJECT_ERROR1("GET_FSTYPE_OSROOT", osroot = NULL);
	if (osroot == NULL) {
		bam_error(GET_FSTYPE_ARGS);
		return (NULL);
	}

	mntfp = fopen(MNTTAB, "r");
	error = errno;
	INJECT_ERROR1("GET_FSTYPE_FOPEN", mntfp = NULL);
	if (mntfp == NULL) {
		bam_error(OPEN_FAIL, MNTTAB, strerror(error));
		return (NULL);
	}

	if (*osroot == '\0')
		mpref.mnt_mountp = "/";
	else
		mpref.mnt_mountp = osroot;

	ret = getmntany(mntfp, &mp, &mpref);
	INJECT_ERROR1("GET_FSTYPE_GETMNTANY", ret = 1);
	if (ret != 0) {
		bam_error(MNTTAB_MNTPT_NOT_FOUND, osroot, MNTTAB);
		(void) fclose(mntfp);
		return (NULL);
	}
	(void) fclose(mntfp);

	INJECT_ERROR1("GET_FSTYPE_NULL", mp.mnt_fstype = NULL);
	if (mp.mnt_fstype == NULL) {
		bam_error(MNTTAB_FSTYPE_NULL, osroot);
		return (NULL);
	}

	BAM_DPRINTF((D_RETURN_SUCCESS, fcn));

	return (s_strdup(mp.mnt_fstype));
}

static char *
create_zfs_sign(char *osdev)
{
	char		tmpsign[PATH_MAX];
	char		*pool;
	const char	*fcn = "create_zfs_sign()";

	BAM_DPRINTF((D_FUNC_ENTRY1, fcn, osdev));

	/*
	 * First find the pool name
	 */
	pool = get_pool(osdev);
	INJECT_ERROR1("CREATE_ZFS_SIGN_GET_POOL", pool = NULL);
	if (pool == NULL) {
		bam_error(GET_POOL_FAILED, osdev);
		return (NULL);
	}

	(void) snprintf(tmpsign, sizeof (tmpsign), "pool_%s", pool);

	BAM_DPRINTF((D_CREATED_ZFS_SIGN, fcn, tmpsign));

	free(pool);

	BAM_DPRINTF((D_RETURN_SUCCESS, fcn));

	return (s_strdup(tmpsign));
}

static char *
create_new_sign(char *osdev, char *fstype)
{
	char		*sign;
	const char	*fcn = "create_new_sign()";

	INJECT_ERROR1("NEW_SIGN_FSTYPE", fstype = "foofs");

	if (strcmp(fstype, "zfs") == 0) {
		BAM_DPRINTF((D_CREATE_NEW_ZFS, fcn));
		sign = create_zfs_sign(osdev);
	} else if (strcmp(fstype, "ufs") == 0) {
		BAM_DPRINTF((D_CREATE_NEW_UFS, fcn));
		sign = create_ufs_sign();
	} else {
		bam_error(GRUBSIGN_NOTSUP, fstype);
		sign = NULL;
	}

	BAM_DPRINTF((D_CREATED_NEW_SIGN, fcn, sign ? sign : "<NULL>"));
	return (sign);
}

static int
set_backup_common(char *mntpt, char *sign)
{
	FILE		*bfp;
	char		backup[PATH_MAX];
	char		tmpsign[PATH_MAX];
	int		error;
	char		*bdir;
	char		*backup_dup;
	struct stat	sb;
	int		ret;
	const char	*fcn = "set_backup_common()";

	(void) snprintf(backup, sizeof (backup), "%s%s",
	    mntpt, GRUBSIGN_BACKUP);

	/* First read the backup */
	bfp = fopen(backup, "r");
	if (bfp != NULL) {
		while (s_fgets(tmpsign, sizeof (tmpsign), bfp)) {
			if (strcmp(tmpsign, sign) == 0) {
				BAM_DPRINTF((D_FOUND_IN_BACKUP, fcn, sign));
				(void) fclose(bfp);
				return (0);
			}
		}
		(void) fclose(bfp);
		BAM_DPRINTF((D_NOT_FOUND_IN_EXIST_BACKUP, fcn, sign));
	} else {
		BAM_DPRINTF((D_BACKUP_NOT_EXIST, fcn, backup));
	}

	/*
	 * Didn't find the correct signature. First create
	 * the directory if necessary.
	 */

	/* dirname() modifies its argument so dup it */
	backup_dup = s_strdup(backup);
	bdir = dirname(backup_dup);
	assert(bdir);

	ret = stat(bdir, &sb);
	INJECT_ERROR1("SET_BACKUP_STAT", ret = -1);
	if (ret == -1) {
		BAM_DPRINTF((D_BACKUP_DIR_NOEXIST, fcn, bdir));
		ret = mkdirp(bdir, 0755);
		error = errno;
		INJECT_ERROR1("SET_BACKUP_MKDIRP", ret = -1);
		if (ret == -1) {
			bam_error(GRUBSIGN_BACKUP_MKDIRERR,
			    GRUBSIGN_BACKUP, strerror(error));
			free(backup_dup);
			return (-1);
		}
	}
	free(backup_dup);

	/*
	 * Open the backup in append mode to add the correct
	 * signature;
	 */
	bfp = fopen(backup, "a");
	error = errno;
	INJECT_ERROR1("SET_BACKUP_FOPEN_A", bfp = NULL);
	if (bfp == NULL) {
		bam_error(GRUBSIGN_BACKUP_OPENERR,
		    GRUBSIGN_BACKUP, strerror(error));
		return (-1);
	}

	(void) snprintf(tmpsign, sizeof (tmpsign), "%s\n", sign);

	ret = fputs(tmpsign, bfp);
	error = errno;
	INJECT_ERROR1("SET_BACKUP_FPUTS", ret = 0);
	if (ret != strlen(tmpsign)) {
		bam_error(GRUBSIGN_BACKUP_WRITEERR,
		    GRUBSIGN_BACKUP, strerror(error));
		(void) fclose(bfp);
		return (-1);
	}

	(void) fclose(bfp);

	if (bam_verbose)
		bam_print(GRUBSIGN_BACKUP_UPDATED, GRUBSIGN_BACKUP);

	BAM_DPRINTF((D_RETURN_SUCCESS, fcn));

	return (0);
}

static int
set_backup_ufs(char *osroot, char *sign)
{
	const char	*fcn = "set_backup_ufs()";

	BAM_DPRINTF((D_FUNC_ENTRY2, fcn, osroot, sign));
	return (set_backup_common(osroot, sign));
}

static int
set_backup_zfs(char *osdev, char *sign)
{
	char		*pool;
	char		*mntpt;
	zfs_mnted_t	mnted;
	int		ret;
	const char	*fcn = "set_backup_zfs()";

	BAM_DPRINTF((D_FUNC_ENTRY2, fcn, osdev, sign));

	pool = get_pool(osdev);
	INJECT_ERROR1("SET_BACKUP_GET_POOL", pool = NULL);
	if (pool == NULL) {
		bam_error(GET_POOL_FAILED, osdev);
		return (-1);
	}

	mntpt = mount_top_dataset(pool, &mnted);
	INJECT_ERROR1("SET_BACKUP_MOUNT_DATASET", mntpt = NULL);
	if (mntpt == NULL) {
		bam_error(FAIL_MNT_TOP_DATASET, pool);
		free(pool);
		return (-1);
	}

	ret = set_backup_common(mntpt, sign);

	(void) umount_top_dataset(pool, mnted, mntpt);

	free(pool);

	INJECT_ERROR1("SET_BACKUP_ZFS_FAIL", ret = 1);
	if (ret == 0) {
		BAM_DPRINTF((D_RETURN_SUCCESS, fcn));
	} else {
		BAM_DPRINTF((D_RETURN_FAILURE, fcn));
	}

	return (ret);
}

static int
set_backup(char *osroot, char *osdev, char *sign, char *fstype)
{
	const char	*fcn = "set_backup()";
	int		ret;

	INJECT_ERROR1("SET_BACKUP_FSTYPE", fstype = "foofs");

	if (strcmp(fstype, "ufs") == 0) {
		BAM_DPRINTF((D_SET_BACKUP_UFS, fcn));
		ret = set_backup_ufs(osroot, sign);
	} else if (strcmp(fstype, "zfs") == 0) {
		BAM_DPRINTF((D_SET_BACKUP_ZFS, fcn));
		ret = set_backup_zfs(osdev, sign);
	} else {
		bam_error(GRUBSIGN_NOTSUP, fstype);
		ret = -1;
	}

	if (ret == 0) {
		BAM_DPRINTF((D_RETURN_SUCCESS, fcn));
	} else {
		BAM_DPRINTF((D_RETURN_FAILURE, fcn));
	}

	return (ret);
}

static int
set_primary_common(char *mntpt, char *sign)
{
	char		signfile[PATH_MAX];
	char		signdir[PATH_MAX];
	struct stat	sb;
	int		fd;
	int		error;
	int		ret;
	const char	*fcn = "set_primary_common()";

	(void) snprintf(signfile, sizeof (signfile), "%s/%s/%s",
	    mntpt, GRUBSIGN_DIR, sign);

	if (stat(signfile, &sb) != -1) {
		if (bam_verbose)
			bam_print(PRIMARY_SIGN_EXISTS, sign);
		return (0);
	} else {
		BAM_DPRINTF((D_PRIMARY_NOT_EXIST, fcn, signfile));
	}

	(void) snprintf(signdir, sizeof (signdir), "%s/%s",
	    mntpt, GRUBSIGN_DIR);

	if (stat(signdir, &sb) == -1) {
		BAM_DPRINTF((D_PRIMARY_DIR_NOEXIST, fcn, signdir));
		ret = mkdirp(signdir, 0755);
		error = errno;
		INJECT_ERROR1("SET_PRIMARY_MKDIRP", ret = -1);
		if (ret == -1) {
			bam_error(GRUBSIGN_MKDIR_ERR, signdir, strerror(errno));
			return (-1);
		}
	}

	fd = open(signfile, O_RDWR|O_CREAT|O_TRUNC, 0444);
	error = errno;
	INJECT_ERROR1("PRIMARY_SIGN_CREAT", fd = -1);
	if (fd == -1) {
		bam_error(GRUBSIGN_PRIMARY_CREATERR, signfile, strerror(error));
		return (-1);
	}

	ret = fsync(fd);
	error = errno;
	INJECT_ERROR1("PRIMARY_FSYNC", ret = -1);
	if (ret != 0) {
		bam_error(GRUBSIGN_PRIMARY_SYNCERR, signfile, strerror(error));
	}

	(void) close(fd);

	if (bam_verbose)
		bam_print(GRUBSIGN_CREATED_PRIMARY, signfile);

	BAM_DPRINTF((D_RETURN_SUCCESS, fcn));

	return (0);
}

static int
set_primary_ufs(char *osroot, char *sign)
{
	const char	*fcn = "set_primary_ufs()";

	BAM_DPRINTF((D_FUNC_ENTRY2, fcn, osroot, sign));
	return (set_primary_common(osroot, sign));
}

static int
set_primary_zfs(char *osdev, char *sign)
{
	char		*pool;
	char		*mntpt;
	zfs_mnted_t	mnted;
	int		ret;
	const char	*fcn = "set_primary_zfs()";

	BAM_DPRINTF((D_FUNC_ENTRY2, fcn, osdev, sign));

	pool = get_pool(osdev);
	INJECT_ERROR1("SET_PRIMARY_ZFS_GET_POOL", pool = NULL);
	if (pool == NULL) {
		bam_error(GET_POOL_FAILED, osdev);
		return (-1);
	}

	/* Pool name must exist in the sign */
	ret = (strstr(sign, pool) != NULL);
	INJECT_ERROR1("SET_PRIMARY_ZFS_POOL_SIGN_INCOMPAT", ret = 0);
	if (ret == 0) {
		bam_error(POOL_SIGN_INCOMPAT, pool, sign);
		free(pool);
		return (-1);
	}

	mntpt = mount_top_dataset(pool, &mnted);
	INJECT_ERROR1("SET_PRIMARY_ZFS_MOUNT_DATASET", mntpt = NULL);
	if (mntpt == NULL) {
		bam_error(FAIL_MNT_TOP_DATASET, pool);
		free(pool);
		return (-1);
	}

	ret = set_primary_common(mntpt, sign);

	(void) umount_top_dataset(pool, mnted, mntpt);

	free(pool);

	INJECT_ERROR1("SET_PRIMARY_ZFS_FAIL", ret = 1);
	if (ret == 0) {
		BAM_DPRINTF((D_RETURN_SUCCESS, fcn));
	} else {
		BAM_DPRINTF((D_RETURN_FAILURE, fcn));
	}

	return (ret);
}

static int
set_primary(char *osroot, char *osdev, char *sign, char *fstype)
{
	const char	*fcn = "set_primary()";
	int		ret;

	INJECT_ERROR1("SET_PRIMARY_FSTYPE", fstype = "foofs");
	if (strcmp(fstype, "ufs") == 0) {
		BAM_DPRINTF((D_SET_PRIMARY_UFS, fcn));
		ret = set_primary_ufs(osroot, sign);
	} else if (strcmp(fstype, "zfs") == 0) {
		BAM_DPRINTF((D_SET_PRIMARY_ZFS, fcn));
		ret = set_primary_zfs(osdev, sign);
	} else {
		bam_error(GRUBSIGN_NOTSUP, fstype);
		ret = -1;
	}

	if (ret == 0) {
		BAM_DPRINTF((D_RETURN_SUCCESS, fcn));
	} else {
		BAM_DPRINTF((D_RETURN_FAILURE, fcn));
	}

	return (ret);
}

static int
ufs_add_to_sign_list(char *sign)
{
	FILE		*tfp;
	char		signline[MAXNAMELEN];
	char		cmd[PATH_MAX];
	int		ret;
	int		error;
	const char	*fcn = "ufs_add_to_sign_list()";

	INJECT_ERROR1("ADD_TO_SIGN_LIST_NOT_UFS", sign = "pool_rpool5");
	if (strncmp(sign, GRUBSIGN_UFS_PREFIX,
	    strlen(GRUBSIGN_UFS_PREFIX)) != 0) {
		bam_error(INVALID_UFS_SIGN, sign);
		(void) unlink(UFS_SIGNATURE_LIST);
		return (-1);
	}

	/*
	 * most failures in this routine are not a fatal error
	 * We simply unlink the /var/run file and continue
	 */

	ret = rename(UFS_SIGNATURE_LIST, UFS_SIGNATURE_LIST".tmp");
	error = errno;
	INJECT_ERROR1("ADD_TO_SIGN_LIST_RENAME", ret = -1);
	if (ret == -1) {
		bam_error(RENAME_FAIL, UFS_SIGNATURE_LIST".tmp",
		    strerror(error));
		(void) unlink(UFS_SIGNATURE_LIST);
		return (0);
	}

	tfp = fopen(UFS_SIGNATURE_LIST".tmp", "a");
	error = errno;
	INJECT_ERROR1("ADD_TO_SIGN_LIST_FOPEN", tfp = NULL);
	if (tfp == NULL) {
		bam_error(OPEN_FAIL, UFS_SIGNATURE_LIST".tmp", strerror(error));
		(void) unlink(UFS_SIGNATURE_LIST".tmp");
		return (0);
	}

	(void) snprintf(signline, sizeof (signline), "%s\n", sign);

	ret = fputs(signline, tfp);
	error = errno;
	INJECT_ERROR1("ADD_TO_SIGN_LIST_FPUTS", ret = 0);
	if (ret != strlen(signline)) {
		bam_error(SIGN_LIST_FPUTS_ERR, sign, strerror(error));
		(void) fclose(tfp);
		(void) unlink(UFS_SIGNATURE_LIST".tmp");
		return (0);
	}

	ret = fclose(tfp);
	error = errno;
	INJECT_ERROR1("ADD_TO_SIGN_LIST_FCLOSE", ret = EOF);
	if (ret == EOF) {
		bam_error(CLOSE_FAIL, UFS_SIGNATURE_LIST".tmp",
		    strerror(error));
		(void) unlink(UFS_SIGNATURE_LIST".tmp");
		return (0);
	}

	/* Sort the list again */
	(void) snprintf(cmd, sizeof (cmd),
	    "/usr/bin/sort -u %s.tmp > %s.sorted",
	    UFS_SIGNATURE_LIST, UFS_SIGNATURE_LIST);

	ret = exec_cmd(cmd, NULL);
	INJECT_ERROR1("ADD_TO_SIGN_LIST_SORT", ret = 1);
	if (ret != 0) {
		bam_error(GRUBSIGN_SORT_FAILED);
		(void) unlink(UFS_SIGNATURE_LIST".sorted");
		(void) unlink(UFS_SIGNATURE_LIST".tmp");
		return (0);
	}

	(void) unlink(UFS_SIGNATURE_LIST".tmp");

	ret = rename(UFS_SIGNATURE_LIST".sorted", UFS_SIGNATURE_LIST);
	error = errno;
	INJECT_ERROR1("ADD_TO_SIGN_LIST_RENAME2", ret = -1);
	if (ret == -1) {
		bam_error(RENAME_FAIL, UFS_SIGNATURE_LIST, strerror(error));
		(void) unlink(UFS_SIGNATURE_LIST".sorted");
		return (0);
	}

	BAM_DPRINTF((D_RETURN_SUCCESS, fcn));

	return (0);
}

static int
set_signature(char *osroot, char *osdev, char *sign, char *fstype)
{
	int		ret;
	const char	*fcn = "set_signature()";

	BAM_DPRINTF((D_FUNC_ENTRY4, fcn, osroot, osdev, sign, fstype));

	ret = set_backup(osroot, osdev, sign, fstype);
	INJECT_ERROR1("SET_SIGNATURE_BACKUP", ret = -1);
	if (ret == -1) {
		BAM_DPRINTF((D_RETURN_FAILURE, fcn));
		bam_error(SET_BACKUP_FAILED, sign, osroot, osdev);
		return (-1);
	}

	ret = set_primary(osroot, osdev, sign, fstype);
	INJECT_ERROR1("SET_SIGNATURE_PRIMARY", ret = -1);

	if (ret == 0) {
		BAM_DPRINTF((D_RETURN_SUCCESS, fcn));
	} else {
		BAM_DPRINTF((D_RETURN_FAILURE, fcn));
		bam_error(SET_PRIMARY_FAILED, sign, osroot, osdev);

	}
	return (ret);
}

char *
get_grubsign(char *osroot, char *osdev)
{
	char		*grubsign;	/* (<sign>,#,#) */
	char		*slice;
	int		fdiskpart;
	char		*sign;
	char		*fstype;
	int		ret;
	const char	*fcn = "get_grubsign()";

	BAM_DPRINTF((D_FUNC_ENTRY2, fcn, osroot, osdev));

	fstype = get_fstype(osroot);
	INJECT_ERROR1("GET_GRUBSIGN_FSTYPE", fstype = NULL);
	if (fstype == NULL) {
		bam_error(GET_FSTYPE_FAILED, osroot);
		return (NULL);
	}

	sign = find_existing_sign(osroot, osdev, fstype);
	INJECT_ERROR1("FIND_EXISTING_SIGN", sign = NULL);
	if (sign == NULL) {
		BAM_DPRINTF((D_GET_GRUBSIGN_NO_EXISTING, fcn, osroot, osdev));
		sign = create_new_sign(osdev, fstype);
		INJECT_ERROR1("CREATE_NEW_SIGN", sign = NULL);
		if (sign == NULL) {
			bam_error(GRUBSIGN_CREATE_FAIL, osdev);
			free(fstype);
			return (NULL);
		}
	}

	ret = set_signature(osroot, osdev, sign, fstype);
	INJECT_ERROR1("SET_SIGNATURE_FAIL", ret = -1);
	if (ret == -1) {
		bam_error(GRUBSIGN_WRITE_FAIL, osdev);
		free(sign);
		free(fstype);
		(void) unlink(UFS_SIGNATURE_LIST);
		return (NULL);
	}

	free(fstype);

	if (bam_verbose)
		bam_print(GRUBSIGN_FOUND_OR_CREATED, sign, osdev);

	fdiskpart = get_partition(osdev);
	INJECT_ERROR1("GET_GRUBSIGN_FDISK", fdiskpart = -1);
	if (fdiskpart == -1) {
		bam_error(FDISKPART_FAIL, osdev);
		free(sign);
		return (NULL);
	}

	slice = strrchr(osdev, 's');

	grubsign = s_calloc(1, MAXNAMELEN + 10);
	if (slice) {
		(void) snprintf(grubsign, MAXNAMELEN + 10, "(%s,%d,%c)",
		    sign, fdiskpart, slice[1] + 'a' - '0');
	} else
		(void) snprintf(grubsign, MAXNAMELEN + 10, "(%s,%d)",
		    sign, fdiskpart);

	free(sign);

	BAM_DPRINTF((D_GET_GRUBSIGN_SUCCESS, fcn, grubsign));

	return (grubsign);
}

static char *
get_title(char *rootdir)
{
	static char	title[80];
	char		*cp = NULL;
	char		release[PATH_MAX];
	FILE		*fp;
	const char	*fcn = "get_title()";

	/* open the /etc/release file */
	(void) snprintf(release, sizeof (release), "%s/etc/release", rootdir);

	fp = fopen(release, "r");
	if (fp == NULL) {
		bam_error(OPEN_FAIL, release, strerror(errno));
		cp = NULL;
		goto out;
	}

	while (s_fgets(title, sizeof (title), fp) != NULL) {
		cp = strstr(title, "Solaris");
		if (cp)
			break;
	}
	(void) fclose(fp);

out:
	cp = cp ? cp : "Solaris";

	BAM_DPRINTF((D_GET_TITLE, fcn, cp));

	return (cp);
}

char *
get_special(char *mountp)
{
	FILE		*mntfp;
	struct mnttab	mp = {0};
	struct mnttab	mpref = {0};
	int		error;
	int		ret;
	const char 	*fcn = "get_special()";

	INJECT_ERROR1("GET_SPECIAL_MNTPT", mountp = NULL);
	if (mountp == NULL) {
		bam_error(GET_SPECIAL_NULL_MNTPT);
		return (NULL);
	}

	mntfp = fopen(MNTTAB, "r");
	error = errno;
	INJECT_ERROR1("GET_SPECIAL_MNTTAB_OPEN", mntfp = NULL);
	if (mntfp == NULL) {
		bam_error(OPEN_FAIL, MNTTAB, strerror(error));
		return (NULL);
	}

	if (*mountp == '\0')
		mpref.mnt_mountp = "/";
	else
		mpref.mnt_mountp = mountp;

	ret = getmntany(mntfp, &mp, &mpref);
	INJECT_ERROR1("GET_SPECIAL_MNTTAB_SEARCH", ret = 1);
	if (ret != 0) {
		(void) fclose(mntfp);
		BAM_DPRINTF((D_GET_SPECIAL_NOT_IN_MNTTAB, fcn, mountp));
		return (NULL);
	}
	(void) fclose(mntfp);

	BAM_DPRINTF((D_GET_SPECIAL, fcn, mp.mnt_special));

	return (s_strdup(mp.mnt_special));
}

static void
free_physarray(char **physarray, int n)
{
	int			i;
	const char		*fcn = "free_physarray()";

	assert(physarray);
	assert(n);

	BAM_DPRINTF((D_FUNC_ENTRY_N1, fcn, n));

	for (i = 0; i < n; i++) {
		free(physarray[i]);
	}
	free(physarray);

	BAM_DPRINTF((D_RETURN_SUCCESS, fcn));
}

static int
zfs_get_physical(char *special, char ***physarray, int *n)
{
	char			sdup[PATH_MAX];
	char			cmd[PATH_MAX];
	char			dsk[PATH_MAX];
	char			*pool;
	filelist_t		flist = {0};
	line_t			*lp;
	line_t			*startlp;
	char			*comp1;
	int			i;
	int			ret;
	const char		*fcn = "zfs_get_physical()";

	assert(special);

	BAM_DPRINTF((D_FUNC_ENTRY1, fcn, special));

	INJECT_ERROR1("INVALID_ZFS_SPECIAL", special = "/foo");
	if (special[0] == '/') {
		bam_error(INVALID_ZFS_SPECIAL, special);
		return (-1);
	}

	(void) strlcpy(sdup, special, sizeof (sdup));

	pool = strtok(sdup, "/");
	INJECT_ERROR1("ZFS_GET_PHYS_POOL", pool = NULL);
	if (pool == NULL) {
		bam_error(CANT_FIND_POOL_FROM_SPECIAL, special);
		return (-1);
	}

	(void) snprintf(cmd, sizeof (cmd), "/sbin/zpool status %s", pool);

	ret = exec_cmd(cmd, &flist);
	INJECT_ERROR1("ZFS_GET_PHYS_STATUS", ret = 1);
	if (ret != 0) {
		bam_error(ZFS_GET_POOL_STATUS, pool);
		return (-1);
	}

	INJECT_ERROR1("ZFS_GET_PHYS_STATUS_OUT", flist.head = NULL);
	if (flist.head == NULL) {
		bam_error(BAD_ZPOOL_STATUS, pool);
		filelist_free(&flist);
		return (-1);
	}

	for (lp = flist.head; lp; lp = lp->next) {
		BAM_DPRINTF((D_STRTOK_ZPOOL_STATUS, fcn, lp->line));
		comp1 = strtok(lp->line, " \t");
		if (comp1 == NULL) {
			free(lp->line);
			lp->line = NULL;
		} else {
			comp1 = s_strdup(comp1);
			free(lp->line);
			lp->line = comp1;
		}
	}

	for (lp = flist.head; lp; lp = lp->next) {
		if (lp->line == NULL)
			continue;
		if (strcmp(lp->line, pool) == 0) {
			BAM_DPRINTF((D_FOUND_POOL_IN_ZPOOL_STATUS, fcn, pool));
			break;
		}
	}

	if (lp == NULL) {
		bam_error(NO_POOL_IN_ZPOOL_STATUS, pool);
		filelist_free(&flist);
		return (-1);
	}

	startlp = lp->next;
	for (i = 0, lp = startlp; lp; lp = lp->next) {
		if (lp->line == NULL)
			continue;
		if (strcmp(lp->line, "mirror") == 0)
			continue;
		if (lp->line[0] == '\0' || strcmp(lp->line, "errors:") == 0)
			break;
		i++;
		BAM_DPRINTF((D_COUNTING_ZFS_PHYS, fcn, i));
	}

	if (i == 0) {
		bam_error(NO_PHYS_IN_ZPOOL_STATUS, pool);
		filelist_free(&flist);
		return (-1);
	}

	*n = i;
	*physarray = s_calloc(*n, sizeof (char *));
	for (i = 0, lp = startlp; lp; lp = lp->next) {
		if (lp->line == NULL)
			continue;
		if (strcmp(lp->line, "mirror") == 0)
			continue;
		if (strcmp(lp->line, "errors:") == 0)
			break;
		if (strncmp(lp->line, "/dev/dsk/", strlen("/dev/dsk/")) != 0 &&
		    strncmp(lp->line, "/dev/rdsk/",
		    strlen("/dev/rdsk/")) != 0)  {
			(void) snprintf(dsk, sizeof (dsk), "/dev/dsk/%s",
			    lp->line);
		} else {
			(void) strlcpy(dsk, lp->line, sizeof (dsk));
		}
		BAM_DPRINTF((D_ADDING_ZFS_PHYS, fcn, dsk, pool));
		(*physarray)[i++] = s_strdup(dsk);
	}

	assert(i == *n);

	filelist_free(&flist);

	BAM_DPRINTF((D_RETURN_SUCCESS, fcn));
	return (0);
}

/*
 * Certain services needed to run metastat successfully may not
 * be enabled. Enable them now.
 */
/*
 * Checks if the specified service is online
 * Returns: 	1 if the service is online
 *		0 if the service is not online
 *		-1 on error
 */
static int
is_svc_online(char *svc)
{
	char			*state;
	const char		*fcn = "is_svc_online()";

	BAM_DPRINTF((D_FUNC_ENTRY1, fcn, svc));

	state = smf_get_state(svc);
	INJECT_ERROR2("GET_SVC_STATE", free(state), state = NULL);
	if (state == NULL) {
		bam_error(GET_SVC_STATE_ERR, svc);
		return (-1);
	}
	BAM_DPRINTF((D_GOT_SVC_STATUS, fcn, svc));

	if (strcmp(state, SCF_STATE_STRING_ONLINE) == 0) {
		BAM_DPRINTF((D_SVC_ONLINE, fcn, svc));
		free(state);
		return (1);
	}

	BAM_DPRINTF((D_SVC_NOT_ONLINE, fcn, state, svc));

	free(state);

	return (0);
}

static int
enable_svc(char *svc)
{
	int			ret;
	int			sleeptime;
	const char		*fcn = "enable_svc()";

	ret = is_svc_online(svc);
	if (ret == -1) {
		bam_error(SVC_IS_ONLINE_FAILED, svc);
		return (-1);
	} else if (ret == 1) {
		BAM_DPRINTF((D_SVC_ALREADY_ONLINE, fcn, svc));
		return (0);
	}

	/* Service is not enabled. Enable it now. */
	ret = smf_enable_instance(svc, 0);
	INJECT_ERROR1("ENABLE_SVC_FAILED", ret = -1);
	if (ret != 0) {
		bam_error(ENABLE_SVC_FAILED, svc);
		return (-1);
	}

	BAM_DPRINTF((D_SVC_ONLINE_INITIATED, fcn, svc));

	sleeptime = 0;
	do {
		ret = is_svc_online(svc);
		INJECT_ERROR1("SVC_ONLINE_SUCCESS", ret = 1);
		INJECT_ERROR1("SVC_ONLINE_FAILURE", ret = -1);
		INJECT_ERROR1("SVC_ONLINE_NOTYET", ret = 0);
		if (ret == -1) {
			bam_error(ERR_SVC_GET_ONLINE, svc);
			return (-1);
		} else if (ret == 1) {
			BAM_DPRINTF((D_SVC_NOW_ONLINE, fcn, svc));
			return (1);
		}
		(void) sleep(1);
	} while (sleeptime < 60);

	bam_error(TIMEOUT_ENABLE_SVC, svc);

	return (-1);
}

static int
ufs_get_physical(char *special, char ***physarray, int *n)
{
	char			cmd[PATH_MAX];
	char			*shortname;
	filelist_t		flist = {0};
	char			*meta;
	char			*type;
	char			*comp1;
	char			*comp2;
	char			*comp3;
	char			*comp4;
	int			i;
	line_t			*lp;
	int			ret;
	char			*svc;
	const char		*fcn = "ufs_get_physical()";

	assert(special);

	BAM_DPRINTF((D_FUNC_ENTRY1, fcn, special));

	if (strncmp(special, "/dev/md/", strlen("/dev/md/")) != 0) {
		bam_error(UFS_GET_PHYS_NOT_SVM, special);
		return (-1);
	}

	if (strncmp(special, "/dev/md/dsk/", strlen("/dev/md/dsk/")) == 0) {
		shortname = special + strlen("/dev/md/dsk/");
	} else if (strncmp(special, "/dev/md/rdsk/",
	    strlen("/dev/md/rdsk/")) == 0) {
		shortname = special + strlen("/dev/md/rdsk");
	} else {
		bam_error(UFS_GET_PHYS_INVALID_SVM, special);
		return (-1);
	}

	BAM_DPRINTF((D_UFS_SVM_SHORT, fcn, special, shortname));

	svc = "network/rpc/meta:default";
	if (enable_svc(svc) == -1) {
		bam_error(UFS_SVM_METASTAT_SVC_ERR, svc);
	}

	(void) snprintf(cmd, sizeof (cmd), "/sbin/metastat -p %s", shortname);

	ret = exec_cmd(cmd, &flist);
	INJECT_ERROR1("UFS_SVM_METASTAT", ret = 1);
	if (ret != 0) {
		bam_error(UFS_SVM_METASTAT_ERR, shortname);
		return (-1);
	}

	INJECT_ERROR1("UFS_SVM_METASTAT_OUT", flist.head = NULL);
	if (flist.head == NULL) {
		bam_error(BAD_UFS_SVM_METASTAT, shortname);
		filelist_free(&flist);
		return (-1);
	}

	/*
	 * Check if not a mirror. We only parse a single metadevice
	 * if not a mirror
	 */
	meta = strtok(flist.head->line, " \t");
	type = strtok(NULL, " \t");
	if (meta == NULL || type == NULL) {
		bam_error(ERROR_PARSE_UFS_SVM_METASTAT, shortname);
		filelist_free(&flist);
		return (-1);
	}
	if (strcmp(type, "-m") != 0) {
		comp1 = strtok(NULL, " \t");
		comp2 = strtok(NULL, " \t");
		if (comp1 == NULL || comp2 != NULL) {
			bam_error(INVALID_UFS_SVM_METASTAT, shortname);
			filelist_free(&flist);
			return (-1);
		}
		BAM_DPRINTF((D_UFS_SVM_ONE_COMP, fcn, comp1, shortname));
		*physarray = s_calloc(1, sizeof (char *));
		(*physarray)[0] = s_strdup(comp1);
		*n = 1;
		filelist_free(&flist);
		return (0);
	}

	/*
	 * Okay we have a mirror. Everything after the first line
	 * is a submirror
	 */
	for (i = 0, lp = flist.head->next; lp; lp = lp->next) {
		if (strstr(lp->line, "/dev/dsk/") == NULL &&
		    strstr(lp->line, "/dev/rdsk/") == NULL) {
			bam_error(CANNOT_PARSE_UFS_SVM_METASTAT, shortname);
			filelist_free(&flist);
			return (-1);
		}
		i++;
	}

	*physarray = s_calloc(i, sizeof (char *));
	*n = i;

	for (i = 0, lp = flist.head->next; lp; lp = lp->next) {
		comp1 = strtok(lp->line, " \t");
		comp2 = strtok(NULL, " \t");
		comp3 = strtok(NULL, " \t");
		comp4 = strtok(NULL, " \t");

		if (comp3 == NULL || comp4 == NULL ||
		    (strncmp(comp4, "/dev/dsk/", strlen("/dev/dsk/")) != 0 &&
		    strncmp(comp4, "/dev/rdsk/", strlen("/dev/rdsk/")) != 0)) {
			bam_error(CANNOT_PARSE_UFS_SVM_SUBMIRROR, shortname);
			filelist_free(&flist);
			free_physarray(*physarray, *n);
			return (-1);
		}

		(*physarray)[i++] = s_strdup(comp4);
	}

	assert(i == *n);

	filelist_free(&flist);

	BAM_DPRINTF((D_RETURN_SUCCESS, fcn));
	return (0);
}

static int
get_physical(char *menu_root, char ***physarray, int *n)
{
	char			*special;
	int			ret;
	const char		*fcn = "get_physical()";

	assert(menu_root);
	assert(physarray);
	assert(n);

	*physarray = NULL;
	*n = 0;

	BAM_DPRINTF((D_FUNC_ENTRY1, fcn, menu_root));

	/* First get the device special file from /etc/mnttab */
	special = get_special(menu_root);
	INJECT_ERROR1("GET_PHYSICAL_SPECIAL", special = NULL);
	if (special == NULL) {
		bam_error(GET_SPECIAL_NULL, menu_root);
		return (-1);
	}

	/* If already a physical device nothing to do */
	if (strncmp(special, "/dev/dsk/", strlen("/dev/dsk/")) == 0 ||
	    strncmp(special, "/dev/rdsk/", strlen("/dev/rdsk/")) == 0) {
		BAM_DPRINTF((D_GET_PHYSICAL_ALREADY, fcn, menu_root, special));
		BAM_DPRINTF((D_RETURN_SUCCESS, fcn));
		*physarray = s_calloc(1, sizeof (char *));
		(*physarray)[0] = special;
		*n = 1;
		return (0);
	}

	if (is_zfs(menu_root)) {
		ret = zfs_get_physical(special, physarray, n);
	} else if (is_ufs(menu_root)) {
		ret = ufs_get_physical(special, physarray, n);
	} else {
		bam_error(GET_PHYSICAL_NOTSUP_FSTYPE, menu_root, special);
		ret = -1;
	}

	free(special);

	INJECT_ERROR1("GET_PHYSICAL_RET", ret = -1);
	if (ret == -1) {
		BAM_DPRINTF((D_RETURN_FAILURE, fcn));
	} else {
		int	i;
		assert (*n > 0);
		for (i = 0; i < *n; i++) {
			BAM_DPRINTF((D_GET_PHYSICAL_RET, fcn, (*physarray)[i]));
		}
	}

	return (ret);
}

static int
is_bootdisk(char *osroot, char *physical)
{
	int			ret;
	char			*grubroot;
	char			*bootp;
	const char		*fcn = "is_bootdisk()";

	assert(osroot);
	assert(physical);

	BAM_DPRINTF((D_FUNC_ENTRY2, fcn, osroot, physical));

	bootp = strstr(physical, "p0:boot");
	if (bootp)
		*bootp = '\0';
	/*
	 * We just want the BIOS mapping for menu disk.
	 * Don't pass menu_root to get_grubroot() as the
	 * check that it is used for is not relevant here.
	 * The osroot is immaterial as well - it is only used to
	 * to find create_diskmap script. Everything hinges on
	 * "physical"
	 */
	grubroot = get_grubroot(osroot, physical, NULL);

	INJECT_ERROR1("IS_BOOTDISK_GRUBROOT", grubroot = NULL);
	if (grubroot == NULL) {
		bam_error(NO_GRUBROOT_FOR_DISK, fcn, physical);
		return (0);
	}
	ret = grubroot[3] == '0';
	free(grubroot);

	BAM_DPRINTF((D_RETURN_RET, fcn, ret));

	return (ret);
}

/*
 * Check if menu is on the boot device
 * Return 0 (false) on error
 */
static int
menu_on_bootdisk(char *osroot, char *menu_root)
{
	char		**physarray;
	int		ret;
	int		n;
	int		i;
	int		on_bootdisk;
	const char	*fcn = "menu_on_bootdisk()";

	BAM_DPRINTF((D_FUNC_ENTRY2, fcn, osroot, menu_root));

	ret = get_physical(menu_root, &physarray, &n);
	INJECT_ERROR1("MENU_ON_BOOTDISK_PHYSICAL", ret = -1);
	if (ret != 0) {
		bam_error(GET_PHYSICAL_MENU_NULL, menu_root);
		return (0);
	}

	assert(physarray);
	assert(n > 0);

	on_bootdisk = 0;
	for (i = 0; i < n; i++) {
		assert(strncmp(physarray[i], "/dev/dsk/",
		    strlen("/dev/dsk/")) == 0 ||
		    strncmp(physarray[i], "/dev/rdsk/",
		    strlen("/dev/rdsk/")) == 0);

		BAM_DPRINTF((D_CHECK_ON_BOOTDISK, fcn, physarray[i]));
		if (is_bootdisk(osroot, physarray[i])) {
			on_bootdisk = 1;
			BAM_DPRINTF((D_IS_ON_BOOTDISK, fcn, physarray[i]));
		}
	}

	free_physarray(physarray, n);

	INJECT_ERROR1("ON_BOOTDISK_YES", on_bootdisk = 1);
	INJECT_ERROR1("ON_BOOTDISK_NO", on_bootdisk = 0);
	if (on_bootdisk) {
		BAM_DPRINTF((D_RETURN_SUCCESS, fcn));
	} else {
		BAM_DPRINTF((D_RETURN_FAILURE, fcn));
	}

	return (on_bootdisk);
}

void
bam_add_line(menu_t *mp, entry_t *entry, line_t *prev, line_t *lp)
{
	const char	*fcn = "bam_add_line()";

	assert(mp);
	assert(entry);
	assert(prev);
	assert(lp);

	lp->next = prev->next;
	if (prev->next) {
		BAM_DPRINTF((D_ADD_LINE_PREV_NEXT, fcn));
		prev->next->prev = lp;
	} else {
		BAM_DPRINTF((D_ADD_LINE_NOT_PREV_NEXT, fcn));
	}
	prev->next = lp;
	lp->prev = prev;

	if (entry->end == prev) {
		BAM_DPRINTF((D_ADD_LINE_LAST_LINE_IN_ENTRY, fcn));
		entry->end = lp;
	}
	if (mp->end == prev) {
		assert(lp->next == NULL);
		mp->end = lp;
		BAM_DPRINTF((D_ADD_LINE_LAST_LINE_IN_MENU, fcn));
	}
}

/*
 * look for matching bootadm entry with specified parameters
 * Here are the rules (based on existing usage):
 * - If title is specified, match on title only
 * - Else, match on root/findroot, kernel, and module.
 *   Note that, if root_opt is non-zero, the absence of
 *   root line is considered a match.
 */
static entry_t *
find_boot_entry(
	menu_t *mp,
	char *title,
	char *kernel,
	char *findroot,
	char *root,
	char *module,
	int root_opt,
	int *entry_num)
{
	int		i;
	line_t		*lp;
	entry_t		*ent;
	const char	*fcn = "find_boot_entry()";

	if (entry_num)
		*entry_num = BAM_ERROR;

	/* find matching entry */
	for (i = 0, ent = mp->entries; ent; i++, ent = ent->next) {
		lp = ent->start;

		/* first line of entry must be bootadm comment */
		lp = ent->start;
		if (lp->flags != BAM_COMMENT ||
		    strcmp(lp->arg, BAM_BOOTADM_HDR) != 0) {
			continue;
		}

		/* advance to title line */
		lp = lp->next;
		if (title) {
			if (lp->flags == BAM_TITLE && lp->arg &&
			    strcmp(lp->arg, title) == 0) {
				BAM_DPRINTF((D_MATCHED_TITLE, fcn, title));
				break;
			}
			BAM_DPRINTF((D_NOMATCH_TITLE, fcn, title, lp->arg));
			continue;	/* check title only */
		}

		lp = lp->next;	/* advance to root line */
		if (lp == NULL) {
			continue;
		} else if (strcmp(lp->cmd, menu_cmds[FINDROOT_CMD]) == 0) {
			INJECT_ERROR1("FIND_BOOT_ENTRY_NULL_FINDROOT",
			    findroot = NULL);
			if (findroot == NULL) {
				BAM_DPRINTF((D_NOMATCH_FINDROOT_NULL,
				    fcn, lp->arg));
				continue;
			}
			/* findroot command found, try match  */
			if (strcmp(lp->arg, findroot) != 0) {
				BAM_DPRINTF((D_NOMATCH_FINDROOT,
				    fcn, findroot, lp->arg));
				continue;
			}
			BAM_DPRINTF((D_MATCHED_FINDROOT, fcn, findroot));
			lp = lp->next;	/* advance to kernel line */
		} else if (strcmp(lp->cmd, menu_cmds[ROOT_CMD]) == 0) {
			INJECT_ERROR1("FIND_BOOT_ENTRY_NULL_ROOT", root = NULL);
			if (root == NULL) {
				BAM_DPRINTF((D_NOMATCH_ROOT_NULL,
				    fcn, lp->arg));
				continue;
			}
			/* root cmd found, try match */
			if (strcmp(lp->arg, root) != 0) {
				BAM_DPRINTF((D_NOMATCH_ROOT,
				    fcn, root, lp->arg));
				continue;
			}
			BAM_DPRINTF((D_MATCHED_ROOT, fcn, root));
			lp = lp->next;	/* advance to kernel line */
		} else {
			INJECT_ERROR1("FIND_BOOT_ENTRY_ROOT_OPT_NO",
			    root_opt = 0);
			INJECT_ERROR1("FIND_BOOT_ENTRY_ROOT_OPT_YES",
			    root_opt = 1);
			/* no root command, see if root is optional */
			if (root_opt == 0) {
				BAM_DPRINTF((D_NO_ROOT_OPT, fcn));
				continue;
			}
			BAM_DPRINTF((D_ROOT_OPT, fcn));
		}

		if (lp == NULL || lp->next == NULL) {
			continue;
		}

		if (kernel &&
		    (!check_cmd(lp->cmd, KERNEL_CMD, lp->arg, kernel))) {
			continue;
		}
		BAM_DPRINTF((D_KERNEL_MATCH, fcn, kernel, lp->arg));

		/*
		 * Check for matching module entry (failsafe or normal).
		 * If it fails to match, we go around the loop again.
		 * For xpv entries, there are two module lines, so we
		 * do the check twice.
		 */
		lp = lp->next;	/* advance to module line */
		if (check_cmd(lp->cmd, MODULE_CMD, lp->arg, module) ||
		    (((lp = lp->next) != NULL) &&
		    check_cmd(lp->cmd, MODULE_CMD, lp->arg, module))) {
			/* match found */
			BAM_DPRINTF((D_MODULE_MATCH, fcn, module, lp->arg));
			break;
		}
	}

	if (ent && entry_num) {
		*entry_num = i;
	}

	if (ent) {
		BAM_DPRINTF((D_RETURN_RET, fcn, i));
	} else {
		BAM_DPRINTF((D_RETURN_RET, fcn, BAM_ERROR));
	}
	return (ent);
}

static int
update_boot_entry(menu_t *mp, char *title, char *findroot, char *root,
    char *kernel, char *mod_kernel, char *module, int root_opt)
{
	int		i;
	int		change_kernel = 0;
	entry_t		*ent;
	line_t		*lp;
	line_t		*tlp;
	char		linebuf[BAM_MAXLINE];
	const char	*fcn = "update_boot_entry()";

	/* note: don't match on title, it's updated on upgrade */
	ent = find_boot_entry(mp, NULL, kernel, findroot, root, module,
	    root_opt, &i);
	if ((ent == NULL) && (bam_direct == BAM_DIRECT_DBOOT)) {
		/*
		 * We may be upgrading a kernel from multiboot to
		 * directboot.  Look for a multiboot entry. A multiboot
		 * entry will not have a findroot line.
		 */
		ent = find_boot_entry(mp, NULL, "multiboot", NULL, root,
		    MULTIBOOT_ARCHIVE, root_opt, &i);
		if (ent != NULL) {
			BAM_DPRINTF((D_UPGRADE_FROM_MULTIBOOT, fcn, root));
			change_kernel = 1;
		}
	} else if (ent) {
		BAM_DPRINTF((D_FOUND_FINDROOT, fcn, findroot));
	}

	if (ent == NULL) {
		BAM_DPRINTF((D_ENTRY_NOT_FOUND_CREATING, fcn, findroot));
		return (add_boot_entry(mp, title, findroot,
		    kernel, mod_kernel, module));
	}

	/* replace title of existing entry and update findroot line */
	lp = ent->start;
	lp = lp->next;	/* title line */
	(void) snprintf(linebuf, sizeof (linebuf), "%s%s%s",
	    menu_cmds[TITLE_CMD], menu_cmds[SEP_CMD], title);
	free(lp->arg);
	free(lp->line);
	lp->arg = s_strdup(title);
	lp->line = s_strdup(linebuf);
	BAM_DPRINTF((D_CHANGING_TITLE, fcn, title));

	tlp = lp;	/* title line */
	lp = lp->next;	/* root line */

	/* if no root or findroot command, create a new line_t */
	if (strcmp(lp->cmd, menu_cmds[ROOT_CMD]) != 0 &&
	    strcmp(lp->cmd, menu_cmds[FINDROOT_CMD]) != 0) {
		lp = s_calloc(1, sizeof (line_t));
		bam_add_line(mp, ent, tlp, lp);
	} else {
		free(lp->cmd);
		free(lp->sep);
		free(lp->arg);
		free(lp->line);
	}

	lp->cmd = s_strdup(menu_cmds[FINDROOT_CMD]);
	lp->sep = s_strdup(menu_cmds[SEP_CMD]);
	lp->arg = s_strdup(findroot);
	(void) snprintf(linebuf, sizeof (linebuf), "%s%s%s",
	    menu_cmds[FINDROOT_CMD], menu_cmds[SEP_CMD], findroot);
	lp->line = s_strdup(linebuf);
	BAM_DPRINTF((D_ADDING_FINDROOT_LINE, fcn, findroot));

	/* kernel line */
	lp = lp->next;

	if (change_kernel) {
		/*
		 * We're upgrading from multiboot to directboot.
		 */
		if (strcmp(lp->cmd, menu_cmds[KERNEL_CMD]) == 0) {
			(void) snprintf(linebuf, sizeof (linebuf), "%s%s%s",
			    menu_cmds[KERNEL_DOLLAR_CMD], menu_cmds[SEP_CMD],
			    kernel);
			free(lp->cmd);
			free(lp->arg);
			free(lp->line);
			lp->cmd = s_strdup(menu_cmds[KERNEL_DOLLAR_CMD]);
			lp->arg = s_strdup(kernel);
			lp->line = s_strdup(linebuf);
			lp = lp->next;
			BAM_DPRINTF((D_ADDING_KERNEL_DOLLAR, fcn, kernel));
		}
		if (strcmp(lp->cmd, menu_cmds[MODULE_CMD]) == 0) {
			(void) snprintf(linebuf, sizeof (linebuf), "%s%s%s",
			    menu_cmds[MODULE_DOLLAR_CMD], menu_cmds[SEP_CMD],
			    module);
			free(lp->cmd);
			free(lp->arg);
			free(lp->line);
			lp->cmd = s_strdup(menu_cmds[MODULE_DOLLAR_CMD]);
			lp->arg = s_strdup(module);
			lp->line = s_strdup(linebuf);
			lp = lp->next;
			BAM_DPRINTF((D_ADDING_MODULE_DOLLAR, fcn, module));
		}
	}
	BAM_DPRINTF((D_RETURN_RET, fcn, i));
	return (i);
}

int
root_optional(char *osroot, char *menu_root)
{
	char			*ospecial;
	char			*mspecial;
	char			*slash;
	int			root_opt;
	int			ret1;
	int			ret2;
	const char		*fcn = "root_optional()";

	BAM_DPRINTF((D_FUNC_ENTRY2, fcn, osroot, menu_root));

	/*
	 * For all filesystems except ZFS, a straight compare of osroot
	 * and menu_root will tell us if root is optional.
	 * For ZFS, the situation is complicated by the fact that
	 * menu_root and osroot are always different
	 */
	ret1 = is_zfs(osroot);
	ret2 = is_zfs(menu_root);
	INJECT_ERROR1("ROOT_OPT_NOT_ZFS", ret1 = 0);
	if (!ret1 || !ret2) {
		BAM_DPRINTF((D_ROOT_OPT_NOT_ZFS, fcn, osroot, menu_root));
		root_opt = (strcmp(osroot, menu_root) == 0);
		goto out;
	}

	ospecial = get_special(osroot);
	INJECT_ERROR1("ROOT_OPTIONAL_OSPECIAL", ospecial = NULL);
	if (ospecial == NULL) {
		bam_error(GET_OSROOT_SPECIAL_ERR, osroot);
		return (0);
	}
	BAM_DPRINTF((D_ROOT_OPTIONAL_OSPECIAL, fcn, ospecial, osroot));

	mspecial = get_special(menu_root);
	INJECT_ERROR1("ROOT_OPTIONAL_MSPECIAL", mspecial = NULL);
	if (mspecial == NULL) {
		bam_error(GET_MENU_ROOT_SPECIAL_ERR, menu_root);
		free(ospecial);
		return (0);
	}
	BAM_DPRINTF((D_ROOT_OPTIONAL_MSPECIAL, fcn, mspecial, menu_root));

	slash = strchr(ospecial, '/');
	if (slash)
		*slash = '\0';
	BAM_DPRINTF((D_ROOT_OPTIONAL_FIXED_OSPECIAL, fcn, ospecial, osroot));

	root_opt = (strcmp(ospecial, mspecial) == 0);

	free(ospecial);
	free(mspecial);

out:
	INJECT_ERROR1("ROOT_OPTIONAL_NO", root_opt = 0);
	INJECT_ERROR1("ROOT_OPTIONAL_YES", root_opt = 1);
	if (root_opt) {
		BAM_DPRINTF((D_RETURN_SUCCESS, fcn));
	} else {
		BAM_DPRINTF((D_RETURN_FAILURE, fcn));
	}

	return (root_opt);
}

/*ARGSUSED*/
static error_t
update_entry(menu_t *mp, char *menu_root, char *osdev)
{
	int		entry;
	char		*grubsign;
	char		*grubroot;
	char		*title;
	char		osroot[PATH_MAX];
	char		*failsafe_kernel = NULL;
	struct stat	sbuf;
	char		failsafe[256];
	int		ret;
	const char	*fcn = "update_entry()";

	assert(mp);
	assert(menu_root);
	assert(osdev);
	assert(bam_root);

	BAM_DPRINTF((D_FUNC_ENTRY3, fcn, menu_root, osdev, bam_root));

	(void) strlcpy(osroot, bam_root, sizeof (osroot));

	title = get_title(osroot);
	assert(title);

	grubsign = get_grubsign(osroot, osdev);
	INJECT_ERROR1("GET_GRUBSIGN_FAIL", grubsign = NULL);
	if (grubsign == NULL) {
		bam_error(GET_GRUBSIGN_ERROR, osroot, osdev);
		return (BAM_ERROR);
	}

	/*
	 * It is not a fatal error if get_grubroot() fails
	 * We no longer rely on biosdev to populate the
	 * menu
	 */
	grubroot = get_grubroot(osroot, osdev, menu_root);
	INJECT_ERROR1("GET_GRUBROOT_FAIL", grubroot = NULL);
	if (grubroot) {
		BAM_DPRINTF((D_GET_GRUBROOT_SUCCESS,
		    fcn, osroot, osdev, menu_root));
	} else {
		BAM_DPRINTF((D_GET_GRUBROOT_FAILURE,
		    fcn, osroot, osdev, menu_root));
	}

	/* add the entry for normal Solaris */
	INJECT_ERROR1("UPDATE_ENTRY_MULTIBOOT",
	    bam_direct = BAM_DIRECT_MULTIBOOT);
	if (bam_direct == BAM_DIRECT_DBOOT) {
		entry = update_boot_entry(mp, title, grubsign, grubroot,
		    (bam_zfs ? DIRECT_BOOT_KERNEL_ZFS : DIRECT_BOOT_KERNEL),
		    NULL, DIRECT_BOOT_ARCHIVE,
		    root_optional(osroot, menu_root));
		BAM_DPRINTF((D_UPDATED_BOOT_ENTRY, fcn, bam_zfs, grubsign));
		if ((entry != BAM_ERROR) && (bam_is_hv == BAM_HV_PRESENT)) {
			(void) update_boot_entry(mp, NEW_HV_ENTRY, grubsign,
			    grubroot, XEN_MENU, bam_zfs ?
			    XEN_KERNEL_MODULE_LINE_ZFS : XEN_KERNEL_MODULE_LINE,
			    DIRECT_BOOT_ARCHIVE,
			    root_optional(osroot, menu_root));
			BAM_DPRINTF((D_UPDATED_HV_ENTRY,
			    fcn, bam_zfs, grubsign));
		}
	} else {
		entry = update_boot_entry(mp, title, grubsign, grubroot,
		    MULTI_BOOT, NULL, MULTIBOOT_ARCHIVE,
		    root_optional(osroot, menu_root));

		BAM_DPRINTF((D_UPDATED_MULTIBOOT_ENTRY, fcn, grubsign));
	}

	/*
	 * Add the entry for failsafe archive.  On a bfu'd system, the
	 * failsafe may be different than the installed kernel.
	 */
	(void) snprintf(failsafe, sizeof (failsafe), "%s%s",
	    osroot, FAILSAFE_ARCHIVE);
	if (stat(failsafe, &sbuf) == 0) {

		/* Figure out where the kernel line should point */
		(void) snprintf(failsafe, sizeof (failsafe), "%s%s", osroot,
		    DIRECT_BOOT_FAILSAFE_KERNEL);
		if (stat(failsafe, &sbuf) == 0) {
			failsafe_kernel = DIRECT_BOOT_FAILSAFE_LINE;
		} else {
			(void) snprintf(failsafe, sizeof (failsafe), "%s%s",
			    osroot, MULTI_BOOT_FAILSAFE);
			if (stat(failsafe, &sbuf) == 0) {
				failsafe_kernel = MULTI_BOOT_FAILSAFE_LINE;
			}
		}
		if (failsafe_kernel != NULL) {
			(void) update_boot_entry(mp, FAILSAFE_TITLE, grubsign,
			    grubroot, failsafe_kernel, NULL, FAILSAFE_ARCHIVE,
			    root_optional(osroot, menu_root));
			BAM_DPRINTF((D_UPDATED_FAILSAFE_ENTRY, fcn,
			    failsafe_kernel));
		}
	}
	free(grubroot);

	INJECT_ERROR1("UPDATE_ENTRY_ERROR", entry = BAM_ERROR);
	if (entry == BAM_ERROR) {
		bam_error(FAILED_TO_ADD_BOOT_ENTRY, title, grubsign);
		free(grubsign);
		return (BAM_ERROR);
	}
	free(grubsign);

	update_numbering(mp);
	ret = set_global(mp, menu_cmds[DEFAULT_CMD], entry);
	INJECT_ERROR1("SET_DEFAULT_ERROR", ret = BAM_ERROR);
	if (ret == BAM_ERROR) {
		bam_error(SET_DEFAULT_FAILED, entry);
	}
	BAM_DPRINTF((D_RETURN_SUCCESS, fcn));
	return (BAM_WRITE);
}

static void
save_default_entry(menu_t *mp, const char *which)
{
	int		lineNum;
	int		entryNum;
	int		entry = 0;	/* default is 0 */
	char		linebuf[BAM_MAXLINE];
	line_t		*lp = mp->curdefault;
	const char	*fcn = "save_default_entry()";

	if (mp->start) {
		lineNum = mp->end->lineNum;
		entryNum = mp->end->entryNum;
	} else {
		lineNum = LINE_INIT;
		entryNum = ENTRY_INIT;
	}

	if (lp)
		entry = s_strtol(lp->arg);

	(void) snprintf(linebuf, sizeof (linebuf), "#%s%d", which, entry);
	BAM_DPRINTF((D_SAVING_DEFAULT_TO, fcn, linebuf));
	line_parser(mp, linebuf, &lineNum, &entryNum);
	BAM_DPRINTF((D_SAVED_DEFAULT_TO, fcn, lineNum, entryNum));
}

static void
restore_default_entry(menu_t *mp, const char *which, line_t *lp)
{
	int		entry;
	char		*str;
	const char	*fcn = "restore_default_entry()";

	if (lp == NULL) {
		BAM_DPRINTF((D_RESTORE_DEFAULT_NULL, fcn));
		return;		/* nothing to restore */
	}

	BAM_DPRINTF((D_RESTORE_DEFAULT_STR, fcn, which));

	str = lp->arg + strlen(which);
	entry = s_strtol(str);
	(void) set_global(mp, menu_cmds[DEFAULT_CMD], entry);

	BAM_DPRINTF((D_RESTORED_DEFAULT_TO, fcn, entry));

	/* delete saved old default line */
	unlink_line(mp, lp);
	line_free(lp);
}

/*
 * This function is for supporting reboot with args.
 * The opt value can be:
 * NULL		delete temp entry, if present
 * entry=<n>	switches default entry to <n>
 * else		treated as boot-args and setup a temperary menu entry
 *		and make it the default
 * Note that we are always rebooting the current OS instance
 * so osroot == / always.
 */
#define	REBOOT_TITLE	"Solaris_reboot_transient"

/*ARGSUSED*/
static error_t
update_temp(menu_t *mp, char *dummy, char *opt)
{
	int		entry;
	char		*osdev;
	char		*fstype;
	char		*sign;
	char		*opt_ptr;
	char		*path;
	char		kernbuf[BUFSIZ];
	char		args_buf[BUFSIZ];
	char		signbuf[PATH_MAX];
	int		ret;
	const char	*fcn = "update_temp()";

	assert(mp);
	assert(dummy == NULL);

	/* opt can be NULL */
	BAM_DPRINTF((D_FUNC_ENTRY1, fcn, opt ? opt : "<NULL>"));
	BAM_DPRINTF((D_BAM_ROOT, fcn, bam_alt_root, bam_root));

	if (bam_alt_root || bam_rootlen != 1 ||
	    strcmp(bam_root, "/") != 0 ||
	    strcmp(rootbuf, "/") != 0) {
		bam_error(ALT_ROOT_INVALID, bam_root);
		return (BAM_ERROR);
	}

	/* If no option, delete exiting reboot menu entry */
	if (opt == NULL) {
		entry_t		*ent;
		BAM_DPRINTF((D_OPT_NULL, fcn));
		ent = find_boot_entry(mp, REBOOT_TITLE, NULL, NULL,
		    NULL, NULL, 0, &entry);
		if (ent == NULL) {	/* not found is ok */
			BAM_DPRINTF((D_TRANSIENT_NOTFOUND, fcn));
			return (BAM_SUCCESS);
		}
		(void) do_delete(mp, entry);
		restore_default_entry(mp, BAM_OLDDEF, mp->olddefault);
		mp->olddefault = NULL;
		BAM_DPRINTF((D_RESTORED_DEFAULT, fcn));
		BAM_DPRINTF((D_RETURN_SUCCESS, fcn));
		return (BAM_WRITE);
	}

	/* if entry= is specified, set the default entry */
	if (strncmp(opt, "entry=", strlen("entry=")) == 0) {
		int entryNum = s_strtol(opt + strlen("entry="));
		BAM_DPRINTF((D_ENTRY_EQUALS, fcn, opt));
		if (selector(mp, opt, &entry, NULL) == BAM_SUCCESS) {
			/* this is entry=# option */
			ret = set_global(mp, menu_cmds[DEFAULT_CMD], entry);
			BAM_DPRINTF((D_ENTRY_SET_IS, fcn, entry, ret));
			return (ret);
		} else {
			bam_error(SET_DEFAULT_FAILED, entryNum);
			return (BAM_ERROR);
		}
	}

	/*
	 * add a new menu entry based on opt and make it the default
	 */

	fstype = get_fstype("/");
	INJECT_ERROR1("REBOOT_FSTYPE_NULL", fstype = NULL);
	if (fstype == NULL) {
		bam_error(REBOOT_FSTYPE_FAILED);
		return (BAM_ERROR);
	}

	osdev = get_special("/");
	INJECT_ERROR1("REBOOT_SPECIAL_NULL", osdev = NULL);
	if (osdev == NULL) {
		free(fstype);
		bam_error(REBOOT_SPECIAL_FAILED);
		return (BAM_ERROR);
	}

	sign = find_existing_sign("/", osdev, fstype);
	INJECT_ERROR1("REBOOT_SIGN_NULL", sign = NULL);
	if (sign == NULL) {
		free(fstype);
		free(osdev);
		bam_error(REBOOT_SIGN_FAILED);
		return (BAM_ERROR);
	}

	free(fstype);
	free(osdev);
	(void) strlcpy(signbuf, sign, sizeof (signbuf));
	free(sign);

	assert(strchr(signbuf, '(') == NULL && strchr(signbuf, ',') == NULL &&
	    strchr(signbuf, ')') == NULL);

	/*
	 * There is no alternate root while doing reboot with args
	 * This version of bootadm is only delivered with a DBOOT
	 * version of Solaris.
	 */
	INJECT_ERROR1("REBOOT_NOT_DBOOT", bam_direct = BAM_DIRECT_MULTIBOOT);
	if (bam_direct != BAM_DIRECT_DBOOT) {
		bam_error(REBOOT_DIRECT_FAILED);
		return (BAM_ERROR);
	}

	/* add an entry for Solaris reboot */
	if (opt[0] == '-') {
		/* It's an option - first see if boot-file is set */
		ret = get_kernel(mp, KERNEL_CMD, kernbuf, sizeof (kernbuf));
		INJECT_ERROR1("REBOOT_GET_KERNEL", ret = BAM_ERROR);
		if (ret != BAM_SUCCESS) {
			bam_error(REBOOT_GET_KERNEL_FAILED);
			return (BAM_ERROR);
		}
		if (kernbuf[0] == '\0')
			(void) strlcpy(kernbuf, DIRECT_BOOT_KERNEL,
			    sizeof (kernbuf));
		(void) strlcat(kernbuf, " ", sizeof (kernbuf));
		(void) strlcat(kernbuf, opt, sizeof (kernbuf));
		BAM_DPRINTF((D_REBOOT_OPTION, fcn, kernbuf));
	} else if (opt[0] == '/') {
		/* It's a full path, so write it out. */
		(void) strlcpy(kernbuf, opt, sizeof (kernbuf));

		/*
		 * If someone runs:
		 *
		 *	# eeprom boot-args='-kd'
		 *	# reboot /platform/i86pc/kernel/unix
		 *
		 * we want to use the boot-args as part of the boot
		 * line.  On the other hand, if someone runs:
		 *
		 *	# reboot "/platform/i86pc/kernel/unix -kd"
		 *
		 * we don't need to mess with boot-args.  If there's
		 * no space in the options string, assume we're in the
		 * first case.
		 */
		if (strchr(opt, ' ') == NULL) {
			ret = get_kernel(mp, ARGS_CMD, args_buf,
			    sizeof (args_buf));
			INJECT_ERROR1("REBOOT_GET_ARGS", ret = BAM_ERROR);
			if (ret != BAM_SUCCESS) {
				bam_error(REBOOT_GET_ARGS_FAILED);
				return (BAM_ERROR);
			}

			if (args_buf[0] != '\0') {
				(void) strlcat(kernbuf, " ", sizeof (kernbuf));
				(void) strlcat(kernbuf, args_buf,
				    sizeof (kernbuf));
			}
		}
		BAM_DPRINTF((D_REBOOT_ABSPATH, fcn, kernbuf));
	} else {
		/*
		 * It may be a partial path, or it may be a partial
		 * path followed by options.  Assume that only options
		 * follow a space.  If someone sends us a kernel path
		 * that includes a space, they deserve to be broken.
		 */
		opt_ptr = strchr(opt, ' ');
		if (opt_ptr != NULL) {
			*opt_ptr = '\0';
		}

		path = expand_path(opt);
		if (path != NULL) {
			(void) strlcpy(kernbuf, path, sizeof (kernbuf));
			free(path);

			/*
			 * If there were options given, use those.
			 * Otherwise, copy over the default options.
			 */
			if (opt_ptr != NULL) {
				/* Restore the space in opt string */
				*opt_ptr = ' ';
				(void) strlcat(kernbuf, opt_ptr,
				    sizeof (kernbuf));
			} else {
				ret = get_kernel(mp, ARGS_CMD, args_buf,
				    sizeof (args_buf));
				INJECT_ERROR1("UPDATE_TEMP_PARTIAL_ARGS",
				    ret = BAM_ERROR);
				if (ret != BAM_SUCCESS) {
					bam_error(REBOOT_GET_ARGS_FAILED);
					return (BAM_ERROR);
				}

				if (args_buf[0] != '\0') {
					(void) strlcat(kernbuf, " ",
					    sizeof (kernbuf));
					(void) strlcat(kernbuf,
					    args_buf, sizeof (kernbuf));
				}
			}
			BAM_DPRINTF((D_REBOOT_RESOLVED_PARTIAL, fcn, kernbuf));
		} else {
			bam_error(UNKNOWN_KERNEL, opt);
			bam_print_stderr(UNKNOWN_KERNEL_REBOOT);
			return (BAM_ERROR);
		}
	}
	entry = add_boot_entry(mp, REBOOT_TITLE, signbuf, kernbuf,
	    NULL, NULL);
	INJECT_ERROR1("REBOOT_ADD_BOOT_ENTRY", entry = BAM_ERROR);
	if (entry == BAM_ERROR) {
		bam_error(REBOOT_WITH_ARGS_ADD_ENTRY_FAILED);
		return (BAM_ERROR);
	}

	save_default_entry(mp, BAM_OLDDEF);
	ret = set_global(mp, menu_cmds[DEFAULT_CMD], entry);
	INJECT_ERROR1("REBOOT_SET_GLOBAL", ret = BAM_ERROR);
	if (ret == BAM_ERROR) {
		bam_error(REBOOT_SET_DEFAULT_FAILED, entry);
	}
	BAM_DPRINTF((D_RETURN_SUCCESS, fcn));
	return (BAM_WRITE);
}

static error_t
set_global(menu_t *mp, char *globalcmd, int val)
{
	line_t		*lp;
	line_t		*found;
	line_t		*last;
	char		*cp;
	char		*str;
	char		prefix[BAM_MAXLINE];
	size_t		len;
	const char	*fcn = "set_global()";

	assert(mp);
	assert(globalcmd);

	if (strcmp(globalcmd, menu_cmds[DEFAULT_CMD]) == 0) {
		INJECT_ERROR1("SET_GLOBAL_VAL_NEG", val = -1);
		INJECT_ERROR1("SET_GLOBAL_MENU_EMPTY", mp->end = NULL);
		INJECT_ERROR1("SET_GLOBAL_VAL_TOO_BIG", val = 100);
		if (val < 0 || mp->end == NULL || val > mp->end->entryNum) {
			(void) snprintf(prefix, sizeof (prefix), "%d", val);
			bam_error(INVALID_ENTRY, prefix);
			return (BAM_ERROR);
		}
	}

	found = last = NULL;
	for (lp = mp->start; lp; lp = lp->next) {
		if (lp->flags != BAM_GLOBAL)
			continue;

		last = lp; /* track the last global found */

		INJECT_ERROR1("SET_GLOBAL_NULL_CMD", lp->cmd = NULL);
		if (lp->cmd == NULL) {
			bam_error(NO_CMD, lp->lineNum);
			continue;
		}
		if (strcmp(globalcmd, lp->cmd) != 0)
			continue;

		BAM_DPRINTF((D_FOUND_GLOBAL, fcn, globalcmd));

		if (found) {
			bam_error(DUP_CMD, globalcmd, lp->lineNum, bam_root);
		}
		found = lp;
	}

	if (found == NULL) {
		lp = s_calloc(1, sizeof (line_t));
		if (last == NULL) {
			lp->next = mp->start;
			mp->start = lp;
			mp->end = (mp->end) ? mp->end : lp;
		} else {
			lp->next = last->next;
			last->next = lp;
			if (lp->next == NULL)
				mp->end = lp;
		}
		lp->flags = BAM_GLOBAL; /* other fields not needed for writes */
		len = strlen(globalcmd) + strlen(menu_cmds[SEP_CMD]);
		len += 10;	/* val < 10 digits */
		lp->line = s_calloc(1, len);
		(void) snprintf(lp->line, len, "%s%s%d",
		    globalcmd, menu_cmds[SEP_CMD], val);
		BAM_DPRINTF((D_SET_GLOBAL_WROTE_NEW, fcn, lp->line));
		BAM_DPRINTF((D_RETURN_SUCCESS, fcn));
		return (BAM_WRITE);
	}

	/*
	 * We are changing an existing entry. Retain any prefix whitespace,
	 * but overwrite everything else. This preserves tabs added for
	 * readability.
	 */
	str = found->line;
	cp = prefix;
	while (*str == ' ' || *str == '\t')
		*(cp++) = *(str++);
	*cp = '\0'; /* Terminate prefix */
	len = strlen(prefix) + strlen(globalcmd);
	len += strlen(menu_cmds[SEP_CMD]) + 10;

	free(found->line);
	found->line = s_calloc(1, len);
	(void) snprintf(found->line, len,
	    "%s%s%s%d", prefix, globalcmd, menu_cmds[SEP_CMD], val);

	BAM_DPRINTF((D_SET_GLOBAL_REPLACED, fcn, found->line));
	BAM_DPRINTF((D_RETURN_SUCCESS, fcn));
	return (BAM_WRITE); /* need a write to menu */
}

/*
 * partial_path may be anything like "kernel/unix" or "kmdb".  Try to
 * expand it to a full unix path.  The calling function is expected to
 * output a message if an error occurs and NULL is returned.
 */
static char *
expand_path(const char *partial_path)
{
	int		new_path_len;
	char		*new_path;
	char		new_path2[PATH_MAX];
	struct stat	sb;
	const char	*fcn = "expand_path()";

	new_path_len = strlen(partial_path) + 64;
	new_path = s_calloc(1, new_path_len);

	/* First, try the simplest case - something like "kernel/unix" */
#if defined(__i386) || defined(__amd64)
	(void) snprintf(new_path, new_path_len, "/platform/i86pc/%s",
#else
	(void) snprintf(new_path, new_path_len, "/platform/s390x/%s",
#endif
	    partial_path);
	if (stat(new_path, &sb) == 0) {
		BAM_DPRINTF((D_EXPAND_PATH, fcn, new_path));
		return (new_path);
	}

	if (strcmp(partial_path, "kmdb") == 0) {
		(void) snprintf(new_path, new_path_len, "%s -k",
		    DIRECT_BOOT_KERNEL);
		BAM_DPRINTF((D_EXPAND_PATH, fcn, new_path));
		return (new_path);
	}

	/*
	 * We've quickly reached unsupported usage.  Try once more to
	 * see if we were just given a glom name.
	 */
#if defined(__i386) || defined(__amd64)
	(void) snprintf(new_path, new_path_len, "/platform/i86pc/%s/unix",
	    partial_path);
	(void) snprintf(new_path2, PATH_MAX, "/platform/i86pc/%s/amd64/unix",
	    partial_path);
#else
	(void) snprintf(new_path, new_path_len, "/platform/s390x/%s/unix",
	    partial_path);
	(void) snprintf(new_path2, PATH_MAX, "/platform/s390x/%s/unix",
	    partial_path);
#endif
	if (stat(new_path, &sb) == 0) {
		if (stat(new_path2, &sb) == 0) {
			/*
			 * We matched both, so we actually
			 * want to write the $ISADIR version.
			 */
#if defined(__i386) || defined(__amd64)
			(void) snprintf(new_path, new_path_len,
			    "/platform/i86pc/kernel/%s/$ISADIR/unix",
			    partial_path);
#else
			(void) snprintf(new_path, new_path_len,
			    "/platform/s390x/kernel/%s/unix",
			    partial_path);
#endif
		}
		BAM_DPRINTF((D_EXPAND_PATH, fcn, new_path));
		return (new_path);
	}

	free(new_path);
	BAM_DPRINTF((D_RETURN_FAILURE, fcn));
	return (NULL);
}

/*
 * The kernel cmd and arg have been changed, so
 * check whether the archive line needs to change.
 */
static void
set_archive_line(entry_t *entryp, line_t *kernelp)
{
	line_t		*lp = entryp->start;
	char		*new_archive;
	menu_cmd_t	m_cmd;
	const char	*fcn = "set_archive_line()";

	for (; lp != NULL; lp = lp->next) {
		if (strncmp(lp->cmd, menu_cmds[MODULE_CMD],
		    sizeof (menu_cmds[MODULE_CMD]) - 1) == 0) {
			break;
		}

		INJECT_ERROR1("SET_ARCHIVE_LINE_END_ENTRY", lp = entryp->end);
		if (lp == entryp->end) {
			BAM_DPRINTF((D_ARCHIVE_LINE_NONE, fcn,
			    entryp->entryNum));
			return;
		}
	}
	INJECT_ERROR1("SET_ARCHIVE_LINE_END_MENU", lp = NULL);
	if (lp == NULL) {
		BAM_DPRINTF((D_ARCHIVE_LINE_NONE, fcn, entryp->entryNum));
		return;
	}

	if (strstr(kernelp->arg, "$ISADIR") != NULL) {
		new_archive = DIRECT_BOOT_ARCHIVE;
		m_cmd = MODULE_DOLLAR_CMD;
	} else if (strstr(kernelp->arg, "amd64") != NULL) {
		new_archive = DIRECT_BOOT_ARCHIVE_64;
		m_cmd = MODULE_CMD;
	} else {
		new_archive = DIRECT_BOOT_ARCHIVE_32;
		m_cmd = MODULE_CMD;
	}

	if (strcmp(lp->arg, new_archive) == 0) {
		BAM_DPRINTF((D_ARCHIVE_LINE_NOCHANGE, fcn, lp->arg));
		return;
	}

	if (strcmp(lp->cmd, menu_cmds[m_cmd]) != 0) {
		free(lp->cmd);
		lp->cmd = s_strdup(menu_cmds[m_cmd]);
	}

	free(lp->arg);
	lp->arg = s_strdup(new_archive);
	update_line(lp);
	BAM_DPRINTF((D_ARCHIVE_LINE_REPLACED, fcn, lp->line));
}

/*
 * Title for an entry to set properties that once went in bootenv.rc.
 */
#define	BOOTENV_RC_TITLE	"Solaris bootenv rc"

/*
 * If path is NULL, return the kernel (optnum == KERNEL_CMD) or arguments
 * (optnum == ARGS_CMD) in the argument buf.  If path is a zero-length
 * string, reset the value to the default.  If path is a non-zero-length
 * string, set the kernel or arguments.
 */
static error_t
get_set_kernel(
	menu_t *mp,
	menu_cmd_t optnum,
	char *path,
	char *buf,
	size_t bufsize)
{
	int		entryNum;
	int		rv = BAM_SUCCESS;
	int		free_new_path = 0;
	entry_t		*entryp;
	line_t		*ptr;
	line_t		*kernelp;
	char		*new_arg;
	char		*old_args;
	char		*space;
	char		*new_path;
	char		old_space;
	size_t		old_kernel_len;
	size_t		new_str_len;
	char		*fstype;
	char		*osdev;
	char		*sign;
	char		signbuf[PATH_MAX];
	int		ret;
	const char	*fcn = "get_set_kernel()";

	assert(bufsize > 0);

	ptr = kernelp = NULL;
	new_arg = old_args = space = NULL;
	new_path = NULL;
	buf[0] = '\0';

	INJECT_ERROR1("GET_SET_KERNEL_NOT_DBOOT",
	    bam_direct = BAM_DIRECT_MULTIBOOT);
	if (bam_direct != BAM_DIRECT_DBOOT) {
		bam_error(NOT_DBOOT, optnum == KERNEL_CMD ? "kernel" : "args");
		return (BAM_ERROR);
	}

	/*
	 * If a user changed the default entry to a non-bootadm controlled
	 * one, we don't want to mess with it.  Just print an error and
	 * return.
	 */
	if (mp->curdefault) {
		entryNum = s_strtol(mp->curdefault->arg);
		for (entryp = mp->entries; entryp; entryp = entryp->next) {
			if (entryp->entryNum == entryNum)
				break;
		}
		if ((entryp != NULL) &&
		    ((entryp->flags & (BAM_ENTRY_BOOTADM|BAM_ENTRY_LU)) == 0)) {
			bam_error(DEFAULT_NOT_BAM);
			return (BAM_ERROR);
		}
	}

	entryp = find_boot_entry(mp, BOOTENV_RC_TITLE, NULL, NULL, NULL, NULL,
	    0, &entryNum);

	if (entryp != NULL) {
		for (ptr = entryp->start; ptr && ptr != entryp->end;
		    ptr = ptr->next) {
			if (strncmp(ptr->cmd, menu_cmds[KERNEL_CMD],
			    sizeof (menu_cmds[KERNEL_CMD]) - 1) == 0) {
				kernelp = ptr;
				break;
			}
		}
		if (kernelp == NULL) {
			bam_error(NO_KERNEL, entryNum);
			return (BAM_ERROR);
		}

		old_kernel_len = strcspn(kernelp->arg, " \t");
		space = old_args = kernelp->arg + old_kernel_len;
		while ((*old_args == ' ') || (*old_args == '\t'))
			old_args++;
	}

	if (path == NULL) {
		if (entryp == NULL) {
			BAM_DPRINTF((D_GET_SET_KERNEL_NO_RC, fcn));
			BAM_DPRINTF((D_RETURN_SUCCESS, fcn));
			return (BAM_SUCCESS);
		}
		assert(kernelp);
		if (optnum == ARGS_CMD) {
			if (old_args[0] != '\0') {
				(void) strlcpy(buf, old_args, bufsize);
				BAM_DPRINTF((D_GET_SET_KERNEL_ARGS, fcn, buf));
			}
		} else {
			/*
			 * We need to print the kernel, so we just turn the
			 * first space into a '\0' and print the beginning.
			 * We don't print anything if it's the default kernel.
			 */
			old_space = *space;
			*space = '\0';
			if (strcmp(kernelp->arg, DIRECT_BOOT_KERNEL) != 0) {
				(void) strlcpy(buf, kernelp->arg, bufsize);
				BAM_DPRINTF((D_GET_SET_KERNEL_KERN, fcn, buf));
			}
			*space = old_space;
		}
		BAM_DPRINTF((D_RETURN_SUCCESS, fcn));
		return (BAM_SUCCESS);
	}

	/*
	 * First, check if we're resetting an entry to the default.
	 */
	if ((path[0] == '\0') ||
	    ((optnum == KERNEL_CMD) &&
	    (strcmp(path, DIRECT_BOOT_KERNEL) == 0))) {
		if ((entryp == NULL) || (kernelp == NULL)) {
			/* No previous entry, it's already the default */
			BAM_DPRINTF((D_GET_SET_KERNEL_ALREADY, fcn));
			return (BAM_SUCCESS);
		}

		/*
		 * Check if we can delete the entry.  If we're resetting the
		 * kernel command, and the args is already empty, or if we're
		 * resetting the args command, and the kernel is already the
		 * default, we can restore the old default and delete the entry.
		 */
		if (((optnum == KERNEL_CMD) &&
		    ((old_args == NULL) || (old_args[0] == '\0'))) ||
		    ((optnum == ARGS_CMD) &&
		    (strncmp(kernelp->arg, DIRECT_BOOT_KERNEL,
		    sizeof (DIRECT_BOOT_KERNEL) - 1) == 0))) {
			kernelp = NULL;
			(void) do_delete(mp, entryNum);
			restore_default_entry(mp, BAM_OLD_RC_DEF,
			    mp->old_rc_default);
			mp->old_rc_default = NULL;
			rv = BAM_WRITE;
			BAM_DPRINTF((D_GET_SET_KERNEL_RESTORE_DEFAULT, fcn));
			goto done;
		}

		if (optnum == KERNEL_CMD) {
			/*
			 * At this point, we've already checked that old_args
			 * and entryp are valid pointers.  The "+ 2" is for
			 * a space a the string termination character.
			 */
			new_str_len = (sizeof (DIRECT_BOOT_KERNEL) - 1) +
			    strlen(old_args) + 2;
			new_arg = s_calloc(1, new_str_len);
			(void) snprintf(new_arg, new_str_len, "%s %s",
			    DIRECT_BOOT_KERNEL, old_args);
			free(kernelp->arg);
			kernelp->arg = new_arg;

			/*
			 * We have changed the kernel line, so we may need
			 * to update the archive line as well.
			 */
			set_archive_line(entryp, kernelp);
			BAM_DPRINTF((D_GET_SET_KERNEL_RESET_KERNEL_SET_ARG,
			    fcn, kernelp->arg));
		} else {
			/*
			 * We're resetting the boot args to nothing, so
			 * we only need to copy the kernel.  We've already
			 * checked that the kernel is not the default.
			 */
			new_arg = s_calloc(1, old_kernel_len + 1);
			(void) snprintf(new_arg, old_kernel_len + 1, "%s",
			    kernelp->arg);
			free(kernelp->arg);
			kernelp->arg = new_arg;
			BAM_DPRINTF((D_GET_SET_KERNEL_RESET_ARG_SET_KERNEL,
			    fcn, kernelp->arg));
		}
		rv = BAM_WRITE;
		goto done;
	}

	/*
	 * Expand the kernel file to a full path, if necessary
	 */
	if ((optnum == KERNEL_CMD) && (path[0] != '/')) {
		new_path = expand_path(path);
		if (new_path == NULL) {
			bam_error(UNKNOWN_KERNEL, path);
			BAM_DPRINTF((D_RETURN_FAILURE, fcn));
			return (BAM_ERROR);
		}
		free_new_path = 1;
	} else {
		new_path = path;
		free_new_path = 0;
	}

	/*
	 * At this point, we know we're setting a new value.  First, take care
	 * of the case where there was no previous entry.
	 */
	if (entryp == NULL) {

		/* Similar to code in update_temp */
		fstype = get_fstype("/");
		INJECT_ERROR1("GET_SET_KERNEL_FSTYPE", fstype = NULL);
		if (fstype == NULL) {
			bam_error(BOOTENV_FSTYPE_FAILED);
			rv = BAM_ERROR;
			goto done;
		}

		osdev = get_special("/");
		INJECT_ERROR1("GET_SET_KERNEL_SPECIAL", osdev = NULL);
		if (osdev == NULL) {
			free(fstype);
			bam_error(BOOTENV_SPECIAL_FAILED);
			rv = BAM_ERROR;
			goto done;
		}

		sign = find_existing_sign("/", osdev, fstype);
		INJECT_ERROR1("GET_SET_KERNEL_SIGN", sign = NULL);
		if (sign == NULL) {
			free(fstype);
			free(osdev);
			bam_error(BOOTENV_SIGN_FAILED);
			rv = BAM_ERROR;
			goto done;
		}

		free(fstype);
		free(osdev);
		(void) strlcpy(signbuf, sign, sizeof (signbuf));
		free(sign);
		assert(strchr(signbuf, '(') == NULL &&
		    strchr(signbuf, ',') == NULL &&
		    strchr(signbuf, ')') == NULL);

		if (optnum == KERNEL_CMD) {
			BAM_DPRINTF((D_GET_SET_KERNEL_NEW_KERN, fcn, new_path));
			entryNum = add_boot_entry(mp, BOOTENV_RC_TITLE,
			    signbuf, new_path, NULL, NULL);
		} else {
			new_str_len = strlen(DIRECT_BOOT_KERNEL) +
			    strlen(path) + 8;
			new_arg = s_calloc(1, new_str_len);

			(void) snprintf(new_arg, new_str_len, "%s %s",
			    DIRECT_BOOT_KERNEL, path);
			BAM_DPRINTF((D_GET_SET_KERNEL_NEW_ARG, fcn, new_arg));
			entryNum = add_boot_entry(mp, BOOTENV_RC_TITLE,
			    signbuf, new_arg, NULL, DIRECT_BOOT_ARCHIVE);
			free(new_arg);
		}
		INJECT_ERROR1("GET_SET_KERNEL_ADD_BOOT_ENTRY",
		    entryNum = BAM_ERROR);
		if (entryNum == BAM_ERROR) {
			bam_error(GET_SET_KERNEL_ADD_BOOT_ENTRY,
			    BOOTENV_RC_TITLE);
			rv = BAM_ERROR;
			goto done;
		}
		save_default_entry(mp, BAM_OLD_RC_DEF);
		ret = set_global(mp, menu_cmds[DEFAULT_CMD], entryNum);
		INJECT_ERROR1("GET_SET_KERNEL_SET_GLOBAL", ret = BAM_ERROR);
		if (ret == BAM_ERROR) {
			bam_error(GET_SET_KERNEL_SET_GLOBAL, entryNum);
		}
		rv = BAM_WRITE;
		goto done;
	}

	/*
	 * There was already an bootenv entry which we need to edit.
	 */
	if (optnum == KERNEL_CMD) {
		new_str_len = strlen(new_path) + strlen(old_args) + 2;
		new_arg = s_calloc(1, new_str_len);
		(void) snprintf(new_arg, new_str_len, "%s %s", new_path,
		    old_args);
		free(kernelp->arg);
		kernelp->arg = new_arg;

		/*
		 * If we have changed the kernel line, we may need to update
		 * the archive line as well.
		 */
		set_archive_line(entryp, kernelp);
		BAM_DPRINTF((D_GET_SET_KERNEL_REPLACED_KERNEL_SAME_ARG, fcn,
		    kernelp->arg));
	} else {
		new_str_len = old_kernel_len + strlen(path) + 8;
		new_arg = s_calloc(1, new_str_len);
		(void) strncpy(new_arg, kernelp->arg, old_kernel_len);
		(void) strlcat(new_arg, " ", new_str_len);
		(void) strlcat(new_arg, path, new_str_len);
		free(kernelp->arg);
		kernelp->arg = new_arg;
		BAM_DPRINTF((D_GET_SET_KERNEL_SAME_KERNEL_REPLACED_ARG, fcn,
		    kernelp->arg));
	}
	rv = BAM_WRITE;

done:
	if ((rv == BAM_WRITE) && kernelp)
		update_line(kernelp);
	if (free_new_path)
		free(new_path);
	if (rv == BAM_WRITE) {
		BAM_DPRINTF((D_RETURN_SUCCESS, fcn));
	} else {
		BAM_DPRINTF((D_RETURN_FAILURE, fcn));
	}
	return (rv);
}

static error_t
get_kernel(menu_t *mp, menu_cmd_t optnum, char *buf, size_t bufsize)
{
	const char	*fcn = "get_kernel()";
	BAM_DPRINTF((D_FUNC_ENTRY1, fcn, menu_cmds[optnum]));
	return (get_set_kernel(mp, optnum, NULL, buf, bufsize));
}

static error_t
set_kernel(menu_t *mp, menu_cmd_t optnum, char *path, char *buf, size_t bufsize)
{
	const char	*fcn = "set_kernel()";
	assert(path != NULL);
	BAM_DPRINTF((D_FUNC_ENTRY2, fcn, menu_cmds[optnum], path));
	return (get_set_kernel(mp, optnum, path, buf, bufsize));
}

/*ARGSUSED*/
static error_t
set_option(menu_t *mp, char *dummy, char *opt)
{
	int		optnum;
	int		optval;
	char		*val;
	char		buf[BUFSIZ] = "";
	error_t		rv;
	const char	*fcn = "set_option()";

	assert(mp);
	assert(opt);
	assert(dummy == NULL);

	/* opt is set from bam_argv[0] and is always non-NULL */
	BAM_DPRINTF((D_FUNC_ENTRY1, fcn, opt));

	val = strchr(opt, '=');
	if (val != NULL) {
		*val = '\0';
	}

	if (strcmp(opt, "default") == 0) {
		optnum = DEFAULT_CMD;
	} else if (strcmp(opt, "timeout") == 0) {
		optnum = TIMEOUT_CMD;
	} else if (strcmp(opt, menu_cmds[KERNEL_CMD]) == 0) {
		optnum = KERNEL_CMD;
	} else if (strcmp(opt, menu_cmds[ARGS_CMD]) == 0) {
		optnum = ARGS_CMD;
	} else {
		bam_error(INVALID_OPTION, opt);
		return (BAM_ERROR);
	}

	/*
	 * kernel and args are allowed without "=new_value" strings.  All
	 * others cause errors
	 */
	if ((val == NULL) && (optnum != KERNEL_CMD) && (optnum != ARGS_CMD)) {
		bam_error(NO_OPTION_ARG, opt);
		return (BAM_ERROR);
	} else if (val != NULL) {
		*val = '=';
	}

	if ((optnum == KERNEL_CMD) || (optnum == ARGS_CMD)) {
		BAM_DPRINTF((D_SET_OPTION, fcn, menu_cmds[optnum],
		    val ? val + 1 : "NULL"));

		if (val)
			rv = set_kernel(mp, optnum, val + 1, buf, sizeof (buf));
		else
			rv = get_kernel(mp, optnum, buf, sizeof (buf));
		if ((rv == BAM_SUCCESS) && (buf[0] != '\0'))
			(void) printf("%s\n", buf);
	} else {
		optval = s_strtol(val + 1);
		BAM_DPRINTF((D_SET_OPTION, fcn, menu_cmds[optnum], val + 1));
		rv = set_global(mp, menu_cmds[optnum], optval);
	}

	if (rv == BAM_WRITE || rv == BAM_SUCCESS) {
		BAM_DPRINTF((D_RETURN_SUCCESS, fcn));
	} else {
		BAM_DPRINTF((D_RETURN_FAILURE, fcn));
	}

	return (rv);
}

/*
 * The quiet argument suppresses messages. This is used
 * when invoked in the context of other commands (e.g. list_entry)
 */
static error_t
read_globals(menu_t *mp, char *menu_path, char *globalcmd, int quiet)
{
	line_t *lp;
	char *arg;
	int done, ret = BAM_SUCCESS;

	assert(mp);
	assert(menu_path);
	assert(globalcmd);

	if (mp->start == NULL) {
		if (!quiet)
			bam_error(NO_MENU, menu_path);
		return (BAM_ERROR);
	}

	done = 0;
	for (lp = mp->start; lp; lp = lp->next) {
		if (lp->flags != BAM_GLOBAL)
			continue;

		if (lp->cmd == NULL) {
			if (!quiet)
				bam_error(NO_CMD, lp->lineNum);
			continue;
		}

		if (strcmp(globalcmd, lp->cmd) != 0)
			continue;

		/* Found global. Check for duplicates */
		if (done && !quiet) {
			bam_error(DUP_CMD, globalcmd, lp->lineNum, bam_root);
			ret = BAM_ERROR;
		}

		arg = lp->arg ? lp->arg : "";
		bam_print(GLOBAL_CMD, globalcmd, arg);
		done = 1;
	}

	if (!done && bam_verbose)
		bam_print(NO_ENTRY, globalcmd);

	return (ret);
}

static error_t
menu_write(char *root, menu_t *mp)
{
	const char *fcn = "menu_write()";

	BAM_DPRINTF((D_MENU_WRITE_ENTER, fcn, root));
	return (list2file(root, MENU_TMP, GRUB_MENU, mp->start));
}

void
line_free(line_t *lp)
{
	if (lp == NULL)
		return;

	if (lp->cmd)
		free(lp->cmd);
	if (lp->sep)
		free(lp->sep);
	if (lp->arg)
		free(lp->arg);
	if (lp->line)
		free(lp->line);
	free(lp);
}

static void
linelist_free(line_t *start)
{
	line_t *lp;

	while (start) {
		lp = start;
		start = start->next;
		line_free(lp);
	}
}

static void
filelist_free(filelist_t *flistp)
{
	linelist_free(flistp->head);
	flistp->head = NULL;
	flistp->tail = NULL;
}

static void
menu_free(menu_t *mp)
{
	entry_t *ent, *tmp;
	assert(mp);

	if (mp->start)
		linelist_free(mp->start);
	ent = mp->entries;
	while (ent) {
		tmp = ent;
		ent = tmp->next;
		free(tmp);
	}

	free(mp);
}

/*
 * Utility routines
 */


/*
 * Returns 0 on success
 * Any other value indicates an error
 */
static int
exec_cmd(char *cmdline, filelist_t *flistp)
{
	char buf[BUFSIZ];
	int ret;
	FILE *ptr;
	sigset_t set;
	void (*disp)(int);

	/*
	 * For security
	 * - only absolute paths are allowed
	 * - set IFS to space and tab
	 */
	if (*cmdline != '/') {
		bam_error(ABS_PATH_REQ, cmdline);
		return (-1);
	}
	(void) putenv("IFS= \t");

	/*
	 * We may have been exec'ed with SIGCHLD blocked
	 * unblock it here
	 */
	(void) sigemptyset(&set);
	(void) sigaddset(&set, SIGCHLD);
	if (sigprocmask(SIG_UNBLOCK, &set, NULL) != 0) {
		bam_error(CANT_UNBLOCK_SIGCHLD, strerror(errno));
		return (-1);
	}

	/*
	 * Set SIGCHLD disposition to SIG_DFL for popen/pclose
	 */
	disp = sigset(SIGCHLD, SIG_DFL);
	if (disp == SIG_ERR) {
		bam_error(FAILED_SIG, strerror(errno));
		return (-1);
	}
	if (disp == SIG_HOLD) {
		bam_error(BLOCKED_SIG, cmdline);
		return (-1);
	}

	ptr = popen(cmdline, "r");
	if (ptr == NULL) {
		bam_error(POPEN_FAIL, cmdline, strerror(errno));
		return (-1);
	}

	/*
	 * If we simply do a pclose() following a popen(), pclose()
	 * will close the reader end of the pipe immediately even
	 * if the child process has not started/exited. pclose()
	 * does wait for cmd to terminate before returning though.
	 * When the executed command writes its output to the pipe
	 * there is no reader process and the command dies with
	 * SIGPIPE. To avoid this we read repeatedly until read
	 * terminates with EOF. This indicates that the command
	 * (writer) has closed the pipe and we can safely do a
	 * pclose().
	 *
	 * Since pclose() does wait for the command to exit,
	 * we can safely reap the exit status of the command
	 * from the value returned by pclose()
	 */
	while (s_fgets(buf, sizeof (buf), ptr) != NULL) {
		if (flistp == NULL) {
			/* s_fgets strips newlines, so insert them at the end */
			bam_print(PRINT, buf);
		} else {
			append_to_flist(flistp, buf);
		}
	}

	ret = pclose(ptr);
	if (ret == -1) {
		bam_error(PCLOSE_FAIL, cmdline, strerror(errno));
		return (-1);
	}

	if (WIFEXITED(ret)) {
		return (WEXITSTATUS(ret));
	} else {
		bam_error(EXEC_FAIL, cmdline, ret);
		return (-1);
	}
}

/*
 * Since this function returns -1 on error
 * it cannot be used to convert -1. However,
 * that is sufficient for what we need.
 */
static long
s_strtol(char *str)
{
	long l;
	char *res = NULL;

	if (str == NULL) {
		return (-1);
	}

	errno = 0;
	l = strtol(str, &res, 10);
	if (errno || *res != '\0') {
		return (-1);
	}

	return (l);
}

/*
 * Wrapper around fputs, that adds a newline (since fputs doesn't)
 */
static int
s_fputs(char *str, FILE *fp)
{
	char linebuf[BAM_MAXLINE];

	(void) snprintf(linebuf, sizeof (linebuf), "%s\n", str);
	return (fputs(linebuf, fp));
}

/*
 * Wrapper around fgets, that strips newlines returned by fgets
 */
char *
s_fgets(char *buf, int buflen, FILE *fp)
{
	int n;

	buf = fgets(buf, buflen, fp);
	if (buf) {
		n = strlen(buf);
		if (n == buflen - 1 && buf[n-1] != '\n')
			bam_error(TOO_LONG, buflen - 1, buf);
		buf[n-1] = (buf[n-1] == '\n') ? '\0' : buf[n-1];
	}

	return (buf);
}

void *
s_calloc(size_t nelem, size_t sz)
{
	void *ptr;

	ptr = calloc(nelem, sz);
	if (ptr == NULL) {
		bam_error(NO_MEM, nelem*sz);
		bam_exit(1);
	}
	return (ptr);
}

void *
s_realloc(void *ptr, size_t sz)
{
	ptr = realloc(ptr, sz);
	if (ptr == NULL) {
		bam_error(NO_MEM, sz);
		bam_exit(1);
	}
	return (ptr);
}

char *
s_strdup(char *str)
{
	char *ptr;

	if (str == NULL)
		return (NULL);

	ptr = strdup(str);
	if (ptr == NULL) {
		bam_error(NO_MEM, strlen(str) + 1);
		bam_exit(1);
	}
	return (ptr);
}

/*
 * Returns 1 if amd64 (or sparc, for syncing x86 diskless clients)
 * Returns 0 otherwise
 */
static int
is_amd64(void)
{
	static int amd64 = -1;
	char isabuf[257];	/* from sysinfo(2) manpage */

	if (amd64 != -1)
		return (amd64);

	if (bam_alt_platform) {
		if (strcmp(bam_platform, "i86pc") == 0) {
			amd64 = 1;		/* diskless server */
		}
	} else {
		if (sysinfo(SI_ISALIST, isabuf, sizeof (isabuf)) > 0 &&
		    strncmp(isabuf, "amd64 ", strlen("amd64 ")) == 0) {
			amd64 = 1;
		} else if (strstr(isabuf, "i386") == NULL) {
			amd64 = 1;		/* diskless server */
		}
	}
	if (amd64 == -1)
		amd64 = 0;

	return (amd64);
}

static char *
get_machine(void)
{
	static int cached = -1;
	static char mbuf[257];	/* from sysinfo(2) manpage */

	if (cached == 0)
		return (mbuf);

	if (bam_alt_platform) {
		return (bam_platform);
	} else {
		if (sysinfo(SI_MACHINE, mbuf, sizeof (mbuf)) > 0) {
			cached = 1;
		}
	}
	if (cached == -1) {
		mbuf[0] = '\0';
		cached = 0;
	}

	return (mbuf);
}

int
is_sparc(void)
{
	static int issparc = -1;
	char mbuf[257];	/* from sysinfo(2) manpage */

	if (issparc != -1)
		return (issparc);

	if (bam_alt_platform) {
		if (strncmp(bam_platform, "sun4", 4) == 0) {
			issparc = 1;
		}
	} else {
		if (sysinfo(SI_ARCHITECTURE, mbuf, sizeof (mbuf)) > 0 &&
		    strcmp(mbuf, "sparc") == 0) {
			issparc = 1;
		}
	}
	if (issparc == -1)
		issparc = 0;

	return (issparc);
}

static void
append_to_flist(filelist_t *flistp, char *s)
{
	line_t *lp;

	lp = s_calloc(1, sizeof (line_t));
	lp->line = s_strdup(s);
	if (flistp->head == NULL)
		flistp->head = lp;
	else
		flistp->tail->next = lp;
	flistp->tail = lp;
}

#if !defined(_OPB)

UCODE_VENDORS;

/*ARGSUSED*/
static void
ucode_install(char *root)
{
	int i;

	for (i = 0; ucode_vendors[i].filestr != NULL; i++) {
		int cmd_len = PATH_MAX + 256;
		char cmd[PATH_MAX + 256];
		char file[PATH_MAX];
		char timestamp[PATH_MAX];
		struct stat fstatus, tstatus;
		struct utimbuf u_times;

		(void) snprintf(file, PATH_MAX, "%s/%s/%s-ucode.txt",
		    bam_root, UCODE_INSTALL_PATH, ucode_vendors[i].filestr);

		if (stat(file, &fstatus) != 0 || !(S_ISREG(fstatus.st_mode)))
			continue;

		(void) snprintf(timestamp, PATH_MAX, "%s.ts", file);

		if (stat(timestamp, &tstatus) == 0 &&
		    fstatus.st_mtime <= tstatus.st_mtime)
			continue;

		(void) snprintf(cmd, cmd_len, "/usr/sbin/ucodeadm -i -R "
		    "%s/%s/%s %s > /dev/null 2>&1", bam_root,
		    UCODE_INSTALL_PATH, ucode_vendors[i].vendorstr, file);
		if (system(cmd) != 0)
			return;

		if (creat(timestamp, S_IRUSR | S_IWUSR) == -1)
			return;

		u_times.actime = fstatus.st_atime;
		u_times.modtime = fstatus.st_mtime;
		(void) utime(timestamp, &u_times);
	}
}
#endif

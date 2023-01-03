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

#ifndef	_LIBSMB_H
#define	_LIBSMB_H

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#ifdef	__cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/list.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>

#include <stdlib.h>
#include <libscf.h>
#include <libshare.h>
#include <sqlite/sqlite.h>

#include <smbsrv/string.h>
#include <smbsrv/smb_idmap.h>
#include <smbsrv/netbios.h>
#include <smbsrv/smb_share.h>
#include <smbsrv/ntstatus.h>
#include <smbsrv/smb_door_svc.h>
#include <smbsrv/alloc.h>
#include <smbsrv/codepage.h>
#include <smbsrv/crypt.h>
#include <smbsrv/ctype.h>
#include <smbsrv/hash_table.h>
#include <smbsrv/msgbuf.h>
#include <smbsrv/oem.h>
#include <smbsrv/smb_i18n.h>
#include <smbsrv/wintypes.h>
#include <smbsrv/smb_xdr.h>
#include <smbsrv/smbinfo.h>

#define	SMB_VARRUN_DIR "/var/run/smb"
#define	SMB_CCACHE_FILE "ccache"
#define	SMB_CCACHE_PATH SMB_VARRUN_DIR "/" SMB_CCACHE_FILE

/* Max value length of all SMB properties */
#define	MAX_VALUE_BUFLEN	512

#define	SMBD_FMRI_PREFIX		"network/smb/server"
#define	SMBD_DEFAULT_INSTANCE_FMRI	"svc:/network/smb/server:default"
#define	SMBD_PG_NAME			"smbd"
#define	SMBD_PROTECTED_PG_NAME		"read"

#define	SMBD_SMF_OK		0
#define	SMBD_SMF_NO_MEMORY	1	/* no memory for data structures */
#define	SMBD_SMF_SYSTEM_ERR	2	/* system error, use errno */
#define	SMBD_SMF_NO_PERMISSION	3	/* no permission for operation */
#define	SMBD_SMF_INVALID_ARG	4

#define	SCH_STATE_UNINIT	0
#define	SCH_STATE_INITIALIZING	1
#define	SCH_STATE_INIT		2

typedef struct smb_scfhandle {
	scf_handle_t		*scf_handle;
	int			scf_state;
	scf_service_t		*scf_service;
	scf_scope_t		*scf_scope;
	scf_transaction_t	*scf_trans;
	scf_transaction_entry_t	*scf_entry;
	scf_propertygroup_t	*scf_pg;
	scf_instance_t		*scf_instance;
	scf_iter_t		*scf_inst_iter;
	scf_iter_t		*scf_pg_iter;
} smb_scfhandle_t;

/*
 * CIFS Configuration Management
 */
typedef enum {
	SMB_CI_OPLOCK_ENABLE = 0,

	SMB_CI_AUTOHOME_MAP,

	SMB_CI_DOMAIN_SID,
	SMB_CI_DOMAIN_MEMB,
	SMB_CI_DOMAIN_NAME,
	SMB_CI_DOMAIN_SRV,

	SMB_CI_WINS_SRV1,
	SMB_CI_WINS_SRV2,
	SMB_CI_WINS_EXCL,

	SMB_CI_SRVSVC_SHRSET_ENABLE,
	SMB_CI_MLRPC_KALIVE,

	SMB_CI_MAX_WORKERS,
	SMB_CI_MAX_CONNECTIONS,
	SMB_CI_KEEPALIVE,
	SMB_CI_RESTRICT_ANON,

	SMB_CI_SIGNING_ENABLE,
	SMB_CI_SIGNING_REQD,
	SMB_CI_SIGNING_CHECK,

	SMB_CI_SYNC_ENABLE,

	SMB_CI_SECURITY,
	SMB_CI_NBSCOPE,
	SMB_CI_SYS_CMNT,
	SMB_CI_LM_LEVEL,

	SMB_CI_ADS_SITE,

	SMB_CI_DYNDNS_ENABLE,

	SMB_CI_MACHINE_PASSWD,
	SMB_CI_KPASSWD_SRV,
	SMB_CI_KPASSWD_DOMAIN,
	SMB_CI_KPASSWD_SEQNUM,
	SMB_CI_NETLOGON_SEQNUM,
	SMB_CI_MAX
} smb_cfg_id_t;

/* SMF helper functions */
extern smb_scfhandle_t *smb_smf_scf_init(char *);
extern void smb_smf_scf_fini(smb_scfhandle_t *);
extern int smb_smf_start_transaction(smb_scfhandle_t *);
extern int smb_smf_end_transaction(smb_scfhandle_t *);
extern int smb_smf_set_string_property(smb_scfhandle_t *, char *, char *);
extern int smb_smf_get_string_property(smb_scfhandle_t *, char *,
    char *, size_t);
extern int smb_smf_set_integer_property(smb_scfhandle_t *, char *, int64_t);
extern int smb_smf_get_integer_property(smb_scfhandle_t *, char *, int64_t *);
extern int smb_smf_set_boolean_property(smb_scfhandle_t *, char *, uint8_t);
extern int smb_smf_get_boolean_property(smb_scfhandle_t *, char *, uint8_t *);
extern int smb_smf_set_opaque_property(smb_scfhandle_t *, char *,
    void *, size_t);
extern int smb_smf_get_opaque_property(smb_scfhandle_t *, char *,
    void *, size_t);
extern int smb_smf_create_service_pgroup(smb_scfhandle_t *, char *);

/* Configuration management functions  */
extern int smb_config_get(smb_cfg_id_t, char *, int);
extern char *smb_config_getname(smb_cfg_id_t);
extern int smb_config_getstr(smb_cfg_id_t, char *, int);
extern int smb_config_getnum(smb_cfg_id_t, int64_t *);
extern boolean_t smb_config_getbool(smb_cfg_id_t);

extern int smb_config_set(smb_cfg_id_t, char *);
extern int smb_config_setstr(smb_cfg_id_t, char *);
extern int smb_config_setnum(smb_cfg_id_t, int64_t);
extern int smb_config_setbool(smb_cfg_id_t, boolean_t);

extern uint8_t smb_config_get_fg_flag(void);
extern char *smb_config_get_localsid(void);
extern int smb_config_secmode_fromstr(char *);
extern char *smb_config_secmode_tostr(int);
extern int smb_config_get_secmode(void);
extern int smb_config_set_secmode(int);
extern int smb_config_set_idmap_domain(char *);
extern int smb_config_refresh_idmap(void);

extern void smb_load_kconfig(smb_kmod_cfg_t *kcfg);

extern boolean_t smb_match_netlogon_seqnum(void);
extern int smb_setdomainprops(char *, char *, char *);
extern void smb_update_netlogon_seqnum(void);

/* smb_door_client.c */
typedef struct smb_joininfo {
	char domain_name[MAXHOSTNAMELEN];
	char domain_username[BUF_LEN + 1];
	char domain_passwd[BUF_LEN + 1];
	uint32_t mode;
} smb_joininfo_t;

/* APIs to communicate with SMB daemon via door calls */
extern uint32_t smb_join(smb_joininfo_t *info);
extern bool_t xdr_smb_dr_joininfo_t(XDR *, smb_joininfo_t *);


#define	SMB_DOMAIN_NOMACHINE_SID	-1
#define	SMB_DOMAIN_NODOMAIN_SID		-2

extern int nt_domain_init(char *resource_domain, uint32_t secmode);

/* Following set of functions, manipulate WINS server configuration */
extern int smb_wins_allow_list(char *config_list, char *allow_list);
extern int smb_wins_exclude_list(char *config_list, char *exclude_list);
extern boolean_t smb_wins_is_excluded(in_addr_t ipaddr,
    ipaddr_t *exclude_list, int nexclude);
extern void smb_wins_build_list(char *buf, uint32_t iplist[], int max_naddr);
extern int smb_wins_iplist(char *list, uint32_t iplist[], int max_naddr);

/*
 * Information on a particular domain: the domain name, the
 * name of a controller (PDC or BDC) and it's ip address.
 */
typedef struct smb_ntdomain {
	char domain[SMB_PI_MAX_DOMAIN];
	char server[SMB_PI_MAX_DOMAIN];
	uint32_t ipaddr;
} smb_ntdomain_t;

/* SMB domain information management functions */
extern smb_ntdomain_t *smb_getdomaininfo(uint32_t timeout);
extern void smb_setdomaininfo(char *domain, char *server, uint32_t ipaddr);
extern void smb_logdomaininfo(smb_ntdomain_t *di);

/*
 * Following set of function, handle calls to SMB Kernel driver, via
 * Kernel doors interface.
 */
extern uint64_t smb_dwncall_user_num(void);
extern int smb_dwncall_share(int, char *, char *);

/*
 * buffer context structure. This is used to keep track of the buffer
 * context.
 *
 * basep:  points to the beginning of the buffer
 * curp:   points to the current offset
 * endp:   points to the limit of the buffer
 */
typedef struct {
	unsigned char *basep;
	unsigned char *curp;
	unsigned char *endp;
} smb_ctxbuf_t;

extern int smb_ctxbuf_init(smb_ctxbuf_t *ctx, unsigned char *buf,
    size_t buflen);
extern int smb_ctxbuf_len(smb_ctxbuf_t *ctx);
extern int smb_ctxbuf_printf(smb_ctxbuf_t *ctx, const char *fmt, ...);

/* Functions to handle SMB daemon communications with idmap service */
extern int smb_idmap_start(void);
extern void smb_idmap_stop(void);
extern int smb_idmap_restart(void);

/* Miscellaneous functions */
extern void hexdump(unsigned char *, int);
extern size_t bintohex(const char *, size_t, char *, size_t);
extern size_t hextobin(const char *, size_t, char *, size_t);
extern char *trim_whitespace(char *buf);
extern void randomize(char *, unsigned);
extern void rand_hash(unsigned char *, size_t, unsigned char *, size_t);

extern int smb_resolve_netbiosname(char *, char *, size_t);
extern int smb_resolve_fqdn(char *, char *, size_t);
extern int smb_getdomainname(char *, size_t);
extern int smb_getfqdomainname(char *, size_t);
extern int smb_gethostname(char *, size_t, int);
extern int smb_getfqhostname(char *, size_t);
extern int smb_getnetbiosname(char *, size_t);
extern smb_sid_t *smb_getdomainsid(void);

extern int smb_get_nameservers(struct in_addr *, int);
extern void smb_tonetbiosname(char *, char *, char);


void smb_trace(const char *s);
void smb_tracef(const char *fmt, ...);

/*
 * Authentication
 */

#define	SMBAUTH_LM_MAGIC_STR	"KGS!@#$%"

#define	SMBAUTH_HASH_SZ		16	/* also LM/NTLM/NTLMv2 Hash size */
#define	SMBAUTH_LM_RESP_SZ	24	/* also NTLM Response size */
#define	SMBAUTH_LM_PWD_SZ	14	/* LM password size */
#define	SMBAUTH_V2_CLNT_CHALLENGE_SZ 8	/* both LMv2 and NTLMv2 */
#define	SMBAUTH_SESSION_KEY_SZ	SMBAUTH_HASH_SZ
#define	SMBAUTH_HEXHASH_SZ	(SMBAUTH_HASH_SZ * 2)

#define	SMBAUTH_FAILURE		1
#define	SMBAUTH_SUCCESS		0
#define	MD_DIGEST_LEN		16

/*
 * Name Types
 *
 * The list of names near the end of the data blob (i.e. the ndb_names
 * field of the smb_auth_data_blob_t data structure) can be classify into
 * the following types:
 *
 * 0x0000 Indicates the end of the list.
 * 0x0001 The name is a NetBIOS machine name (e.g. server name)
 * 0x0002 The name is an NT Domain NetBIOS name.
 * 0x0003 The name is the server's DNS hostname.
 * 0x0004 The name is a W2K Domain name (a DNS name).
 */
#define	SMBAUTH_NAME_TYPE_LIST_END		0x0000
#define	SMBAUTH_NAME_TYPE_SERVER_NETBIOS 	0x0001
#define	SMBAUTH_NAME_TYPE_DOMAIN_NETBIOS 	0x0002
#define	SMBAUTH_NAME_TYPE_SERVER_DNS		0x0003
#define	SMBAUTH_NAME_TYPE_DOMAIN_DNS 		0x0004

/*
 * smb_auth_name_entry_t
 *
 * Each name entry in the data blob consists of the following 3 fields:
 *
 * nne_type - name type
 * nne_len  - the length of the name
 * nne_name - the name, in uppercase UCS-2LE Unicode format
 */
typedef struct smb_auth_name_entry {
	unsigned short nne_type;
	unsigned short nne_len;
	mts_wchar_t nne_name[SMB_PI_MAX_DOMAIN * 2];
} smb_auth_name_entry_t;

/*
 * smb_auth_data_blob
 *
 * The format of this NTLMv2 data blob structure is as follow:
 *
 *	- Blob Signature 0x01010000 (4 bytes)
 * - Reserved (0x00000000) (4 bytes)
 * - Timestamp Little-endian, 64-bit signed value representing
 *   the number of tenths of a microsecond since January 1, 1601.
 *   (8 bytes)
 * - Client Challenge (8 bytes)
 * - Unknown1 (4 bytes)
 * - List of Target Information (variable length)
 * - Unknown2 (4 bytes)
 */
typedef struct smb_auth_data_blob {
	unsigned char ndb_signature[4];
	unsigned char ndb_reserved[4];
	uint64_t ndb_timestamp;
	unsigned char ndb_clnt_challenge[SMBAUTH_V2_CLNT_CHALLENGE_SZ];
	unsigned char ndb_unknown[4];
	smb_auth_name_entry_t ndb_names[2];
	unsigned char ndb_unknown2[4];
} smb_auth_data_blob_t;

#define	SMBAUTH_BLOB_MAXLEN (sizeof (smb_auth_data_blob_t))
#define	SMBAUTH_CI_MAXLEN   SMBAUTH_LM_RESP_SZ
#define	SMBAUTH_CS_MAXLEN   (SMBAUTH_BLOB_MAXLEN + SMBAUTH_HASH_SZ)

/*
 * smb_auth_info_t
 *
 * The structure contains all the authentication information
 * needed for the preparaton of the SMBSessionSetupAndx request
 * and the user session key.
 *
 * hash      - NTLM hash
 * hash_v2   - NTLMv2 hash
 * ci_len    - the length of the case-insensitive password
 * ci        - case-insensitive password
 *             (If NTLMv2 authentication mechanism is used, it
 *              represents the LMv2 response. Otherwise, it
 *              is empty.)
 * cs_len    - the length of the case-sensitive password
 * cs        - case-sensitive password
 *             (If NTLMv2 authentication mechanism is used, it
 *              represents the NTLMv2 response. Otherwise, it
 *              represents the NTLM response.)
 * data_blob - NTLMv2 data blob
 */
typedef struct smb_auth_info {
	unsigned char hash[SMBAUTH_HASH_SZ];
	unsigned char hash_v2[SMBAUTH_HASH_SZ];
	unsigned short ci_len;
	unsigned char ci[SMBAUTH_CI_MAXLEN];
	unsigned short cs_len;
	unsigned char cs[SMBAUTH_CS_MAXLEN];
	int lmcompatibility_lvl;
	smb_auth_data_blob_t data_blob;
} smb_auth_info_t;

/*
 * SMB password management
 */

#define	SMB_PWF_LM	0x01	/* LM hash is present */
#define	SMB_PWF_NT	0x02	/* NT hash is present */
#define	SMB_PWF_DISABLE	0x04	/* Account is disabled */

typedef struct smb_passwd {
	uid_t pw_uid;
	uint32_t pw_flags;
	unsigned char pw_lmhash[SMBAUTH_HASH_SZ];
	unsigned char pw_nthash[SMBAUTH_HASH_SZ];
} smb_passwd_t;

/*
 * Control flags passed to smb_pwd_setcntl
 */
#define	SMB_PWC_DISABLE	0x01
#define	SMB_PWC_ENABLE	0x02
#define	SMB_PWC_NOLM	0x04

#define	SMB_PWE_SUCCESS		0
#define	SMB_PWE_USER_UNKNOWN	1
#define	SMB_PWE_USER_DISABLE	2
#define	SMB_PWE_CLOSE_FAILED	3
#define	SMB_PWE_OPEN_FAILED	4
#define	SMB_PWE_WRITE_FAILED	6
#define	SMB_PWE_UPDATE_FAILED	7
#define	SMB_PWE_STAT_FAILED	8
#define	SMB_PWE_BUSY		9
#define	SMB_PWE_DENIED		10
#define	SMB_PWE_SYSTEM_ERROR	11
#define	SMB_PWE_INVALID_PARAM	12
#define	SMB_PWE_NO_MEMORY	13
#define	SMB_PWE_MAX		14

typedef struct smb_pwditer {
	void *spi_next;
} smb_pwditer_t;

typedef struct smb_luser {
	char *su_name;
	char *su_fullname;
	char *su_desc;
	uint32_t su_rid;
	uint32_t su_ctrl;
} smb_luser_t;

extern void smb_pwd_init(boolean_t);
extern void smb_pwd_fini(void);
extern smb_passwd_t *smb_pwd_getpasswd(const char *, smb_passwd_t *);
extern int smb_pwd_setpasswd(const char *, const char *);
extern int smb_pwd_setcntl(const char *, int);
extern int smb_pwd_num(void);

extern int smb_pwd_iteropen(smb_pwditer_t *);
extern smb_luser_t *smb_pwd_iterate(smb_pwditer_t *);
extern void smb_pwd_iterclose(smb_pwditer_t *);

extern int smb_auth_qnd_unicode(mts_wchar_t *dst, char *src, int length);
extern int smb_auth_hmac_md5(unsigned char *data, int data_len,
    unsigned char *key, int key_len, unsigned char *digest);

/*
 * A variation on HMAC-MD5 known as HMACT64 is used by Windows systems.
 * The HMACT64() function is the same as the HMAC-MD5() except that
 * it truncates the input key to 64 bytes rather than hashing it down
 * to 16 bytes using the MD5() function.
 */
#define	SMBAUTH_HMACT64(D, Ds, K, Ks, digest) \
	smb_auth_hmac_md5(D, Ds, K, (Ks > 64) ? 64 : Ks, digest)

extern int smb_auth_DES(unsigned char *, int, unsigned char *, int,
    unsigned char *, int);

extern int smb_auth_md4(unsigned char *, unsigned char *, int);
extern int smb_auth_lm_hash(char *, unsigned char *);
extern int smb_auth_ntlm_hash(char *, unsigned char *);

extern int smb_auth_set_info(char *, char *,
    unsigned char *, char *, unsigned char *,
    int, int, smb_auth_info_t *);

extern int smb_auth_ntlmv2_hash(unsigned char *,
	char *, char *, unsigned char *);

extern int smb_auth_gen_session_key(smb_auth_info_t *, unsigned char *);

boolean_t smb_auth_validate_lm(unsigned char *, uint32_t, smb_passwd_t *,
    unsigned char *, int, char *, char *);
boolean_t smb_auth_validate_nt(unsigned char *, uint32_t, smb_passwd_t *,
    unsigned char *, int, char *, char *);

/*
 * SMB MAC Signing
 */

#define	SMB_MAC_KEY_SZ	(SMBAUTH_SESSION_KEY_SZ + SMBAUTH_CS_MAXLEN)
#define	SMB_SIG_OFFS	14	/* signature field offset within header */
#define	SMB_SIG_SIZE	8	/* SMB signature size */

/*
 * Signing flags:
 *
 * SMB_SCF_ENABLE                 Signing is enabled.
 *
 * SMB_SCF_REQUIRED               Signing is enabled and required.
 *                                This flag shouldn't be set if
 *                                SMB_SCF_ENABLE isn't set.
 *
 * SMB_SCF_STARTED                Signing will start after receiving
 *                                the first non-anonymous SessionSetup
 *                                request.
 *
 * SMB_SCF_KEY_ISSET_THIS_LOGON   Indicates whether the MAC key has just
 *                                been set for this logon. (prior to
 *                                sending the SMBSessionSetup request)
 *
 */
#define	SMB_SCF_ENABLE		0x01
#define	SMB_SCF_REQUIRED	0x02
#define	SMB_SCF_STARTED		0x04
#define	SMB_SCF_KEY_ISSET_THIS_LOGON	0x08

/*
 * smb_sign_ctx
 *
 * SMB signing context.
 *
 *	ssc_seqnum				sequence number
 *	ssc_keylen				mac key length
 *	ssc_mid					multiplex id - reserved
 *	ssc_flags				flags
 *	ssc_mackey				mac key
 *	ssc_sign				mac signature
 *
 */
typedef struct smb_sign_ctx {
	unsigned int ssc_seqnum;
	unsigned short ssc_keylen;
	unsigned short ssc_mid;
	unsigned int ssc_flags;
	unsigned char ssc_mackey[SMB_MAC_KEY_SZ];
	unsigned char ssc_sign[SMB_SIG_SIZE];
} smb_sign_ctx_t;

extern int smb_mac_init(smb_sign_ctx_t *sign_ctx, smb_auth_info_t *auth);
extern int smb_mac_calc(smb_sign_ctx_t *sign_ctx,
    const unsigned char *buf, size_t buf_len, unsigned char *mac_sign);
extern int smb_mac_chk(smb_sign_ctx_t *sign_ctx,
    const unsigned char *buf, size_t buf_len);
extern int smb_mac_sign(smb_sign_ctx_t *sign_ctx,
    unsigned char *buf, size_t buf_len);
extern void smb_mac_inc_seqnum(smb_sign_ctx_t *sign_ctx);
extern void smb_mac_dec_seqnum(smb_sign_ctx_t *sign_ctx);

/*
 * Each domain is categorized using the enum values below.
 * The local domain refers to the local machine and is named
 * after the local hostname. The primary domain is the domain
 * that the system joined. All other domains are either
 * trusted or untrusted, as defined by the primary domain PDC.
 *
 * This enum must be kept in step with the table of strings
 * in ntdomain.c.
 */
typedef enum nt_domain_type {
	NT_DOMAIN_NULL,
	NT_DOMAIN_BUILTIN,
	NT_DOMAIN_LOCAL,
	NT_DOMAIN_PRIMARY,
	NT_DOMAIN_ACCOUNT,
	NT_DOMAIN_TRUSTED,
	NT_DOMAIN_UNTRUSTED,
	NT_DOMAIN_NUM_TYPES
} nt_domain_type_t;


/*
 * This is the information that is held about each domain. The database
 * is a linked list that is threaded through the domain structures. As
 * the number of domains in the database should be small (32 max), this
 * should be sufficient.
 */
typedef struct nt_domain {
	struct nt_domain *next;
	nt_domain_type_t type;
	char *name;
	smb_sid_t *sid;
} nt_domain_t;

nt_domain_t *nt_domain_new(nt_domain_type_t type, char *name, smb_sid_t *sid);
void nt_domain_delete(nt_domain_t *domain);
nt_domain_t *nt_domain_add(nt_domain_t *new_domain);
void nt_domain_remove(nt_domain_t *domain);
void nt_domain_flush(nt_domain_type_t domain_type);
void nt_domain_sync(void);
char *nt_domain_xlat_type(nt_domain_type_t domain_type);
nt_domain_type_t nt_domain_xlat_type_name(char *type_name);
nt_domain_t *nt_domain_lookup_name(char *domain_name);
nt_domain_t *nt_domain_lookup_sid(smb_sid_t *domain_sid);
nt_domain_t *nt_domain_lookupbytype(nt_domain_type_t type);
smb_sid_t *nt_domain_local_sid(void);

typedef enum {
	SMB_LGRP_BUILTIN = 1,
	SMB_LGRP_LOCAL
} smb_gdomain_t;

typedef struct smb_gsid {
	smb_sid_t *gs_sid;
	uint16_t gs_type;
} smb_gsid_t;

typedef struct smb_giter {
	sqlite_vm	*sgi_vm;
	sqlite		*sgi_db;
} smb_giter_t;

typedef struct smb_group {
	char			*sg_name;
	char			*sg_cmnt;
	uint32_t		sg_attr;
	uint32_t		sg_rid;
	smb_gsid_t		sg_id;
	smb_gdomain_t		sg_domain;
	smb_privset_t		*sg_privs;
	uint32_t		sg_nmembers;
	smb_gsid_t		*sg_members;
} smb_group_t;

int smb_lgrp_start(void);
void smb_lgrp_stop(void);
int smb_lgrp_add(char *, char *);
int smb_lgrp_rename(char *, char *);
int smb_lgrp_delete(char *);
int smb_lgrp_setcmnt(char *, char *);
int smb_lgrp_getcmnt(char *, char **);
int smb_lgrp_getpriv(char *, uint8_t, boolean_t *);
int smb_lgrp_setpriv(char *, uint8_t, boolean_t);
int smb_lgrp_add_member(char *, smb_sid_t *, uint16_t);
int smb_lgrp_del_member(char *, smb_sid_t *, uint16_t);
int smb_lgrp_getbyname(char *, smb_group_t *);
int smb_lgrp_getbyrid(uint32_t, smb_gdomain_t, smb_group_t *);
int smb_lgrp_numbydomain(smb_gdomain_t, int *);
int smb_lgrp_numbymember(smb_sid_t *, int *);
void smb_lgrp_free(smb_group_t *);
boolean_t smb_lgrp_is_member(smb_group_t *, smb_sid_t *);
char *smb_lgrp_strerror(int);
int smb_lgrp_iteropen(smb_giter_t *);
void smb_lgrp_iterclose(smb_giter_t *);
int smb_lgrp_iterate(smb_giter_t *, smb_group_t *);

int smb_lookup_sid(smb_sid_t *, char *buf, int buflen);
int smb_lookup_name(char *, smb_gsid_t *);

#define	SMB_LGRP_SUCCESS		0
#define	SMB_LGRP_INVALID_ARG		1
#define	SMB_LGRP_INVALID_MEMBER		2
#define	SMB_LGRP_INVALID_NAME		3
#define	SMB_LGRP_NOT_FOUND		4
#define	SMB_LGRP_EXISTS			5
#define	SMB_LGRP_NO_SID			6
#define	SMB_LGRP_NO_LOCAL_SID		7
#define	SMB_LGRP_SID_NOTLOCAL		8
#define	SMB_LGRP_WKSID			9
#define	SMB_LGRP_NO_MEMORY		10
#define	SMB_LGRP_DB_ERROR		11
#define	SMB_LGRP_DBINIT_ERROR		12
#define	SMB_LGRP_INTERNAL_ERROR		13
#define	SMB_LGRP_MEMBER_IN_GROUP	14
#define	SMB_LGRP_MEMBER_NOT_IN_GROUP	15
#define	SMB_LGRP_NO_SUCH_PRIV		16
#define	SMB_LGRP_NO_SUCH_DOMAIN		17
#define	SMB_LGRP_PRIV_HELD		18
#define	SMB_LGRP_PRIV_NOT_HELD		19
#define	SMB_LGRP_BAD_DATA		20
#define	SMB_LGRP_NO_MORE		21
#define	SMB_LGRP_DBOPEN_FAILED		22
#define	SMB_LGRP_DBEXEC_FAILED		23
#define	SMB_LGRP_DBINIT_FAILED		24
#define	SMB_LGRP_DOMLKP_FAILED		25
#define	SMB_LGRP_DOMINS_FAILED		26
#define	SMB_LGRP_INSERT_FAILED		27
#define	SMB_LGRP_DELETE_FAILED		28
#define	SMB_LGRP_UPDATE_FAILED		29
#define	SMB_LGRP_LOOKUP_FAILED		30
#define	SMB_LGRP_NOT_SUPPORTED		31

#define	SMB_LGRP_NAME_CHAR_MAX	32
#define	SMB_LGRP_COMMENT_MAX	256
#define	SMB_LGRP_NAME_MAX	(SMB_LGRP_NAME_CHAR_MAX * MTS_MB_CHAR_MAX + 1)

/*
 * values for smb_nic_t.smbflags
 */
#define	SMB_NICF_NBEXCL		0x01	/* Excluded from Netbios activities */
#define	SMB_NICF_ALIAS		0x02	/* This is an alias */

/*
 * smb_nic_t
 *     nic_host		actual host name
 *     nic_nbname	16-byte NetBIOS host name
 */
typedef struct {
	char		nic_host[MAXHOSTNAMELEN];
	char		nic_nbname[NETBIOS_NAME_SZ];
	char		nic_cmnt[SMB_PI_MAX_COMMENT];
	char		nic_ifname[LIFNAMSIZ];
	uint32_t	nic_ip;
	uint32_t	nic_mask;
	uint32_t	nic_bcast;
	uint32_t	nic_smbflags;
	uint64_t	nic_sysflags;
} smb_nic_t;

typedef struct smb_niciter {
	smb_nic_t ni_nic;
	int ni_cookie;
	int ni_seqnum;
} smb_niciter_t;

/* NIC config functions */
int smb_nic_init(void);
void smb_nic_fini(void);
int smb_nic_getnum(char *);
int smb_nic_addhost(const char *, const char *, int, const char **);
int smb_nic_delhost(const char *);
int smb_nic_getfirst(smb_niciter_t *);
int smb_nic_getnext(smb_niciter_t *);
boolean_t smb_nic_exists(uint32_t, boolean_t);

/* NIC Monitoring functions */
int smb_nicmon_start(const char *);
void smb_nicmon_stop(void);

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBSMB_H */

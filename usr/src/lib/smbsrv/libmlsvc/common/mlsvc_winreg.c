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
 * Windows Registry RPC (WINREG) server-side interface.
 *
 * The WINREG RPC interface returns Win32 error codes.
 *
 * HKLM		Hive Key Local Machine
 * HKU		Hive Key Users
 */

#include <sys/utsname.h>
#include <strings.h>

#include <smbsrv/libsmb.h>
#include <smbsrv/ntstatus.h>
#include <smbsrv/nterror.h>
#include <smbsrv/nmpipes.h>
#include <smbsrv/mlsvc_util.h>
#include <smbsrv/ndl/winreg.ndl>

/*
 * Local handle management keys.
 */
static int winreg_hk;
static int winreg_hklm;
static int winreg_hkuser;
static int winreg_hkkey;

/*
 * List of supported registry keys (case-insensitive).
 *	"System\\CurrentControlSet\\Services\\Alerter\\Parameters"
 */
static char *winreg_keys[] = {
	"System\\CurrentControlSet\\Control\\ProductOptions",
	"System\\CurrentControlSet\\Services\\Eventlog\\System"
};

typedef struct winreg_subkey {
	list_node_t sk_lnd;
	ndr_hdid_t sk_handle;
	char sk_name[MAXPATHLEN];
	boolean_t sk_predefined;
} winreg_subkey_t;

typedef struct winreg_keylist {
	list_t kl_list;
	int kl_count;
} winreg_keylist_t;

static winreg_keylist_t winreg_keylist;

static boolean_t winreg_key_has_subkey(const char *);
static char *winreg_lookup_value(const char *);

static int winreg_s_OpenHK(void *, struct mlrpc_xaction *);
static int winreg_s_OpenHKLM(void *, struct mlrpc_xaction *);
static int winreg_s_OpenHKUsers(void *, struct mlrpc_xaction *);
static int winreg_s_Close(void *, struct mlrpc_xaction *);
static int winreg_s_CreateKey(void *, struct mlrpc_xaction *);
static int winreg_s_DeleteKey(void *, struct mlrpc_xaction *);
static int winreg_s_DeleteValue(void *, struct mlrpc_xaction *);
static int winreg_s_EnumKey(void *, struct mlrpc_xaction *);
static int winreg_s_EnumValue(void *, struct mlrpc_xaction *);
static int winreg_s_FlushKey(void *, struct mlrpc_xaction *);
static int winreg_s_GetKeySec(void *, struct mlrpc_xaction *);
static int winreg_s_NotifyChange(void *, struct mlrpc_xaction *);
static int winreg_s_OpenKey(void *, struct mlrpc_xaction *);
static int winreg_s_QueryKey(void *, struct mlrpc_xaction *);
static int winreg_s_QueryValue(void *, struct mlrpc_xaction *);
static int winreg_s_SetKeySec(void *, struct mlrpc_xaction *);
static int winreg_s_CreateValue(void *, struct mlrpc_xaction *);
static int winreg_s_Shutdown(void *, struct mlrpc_xaction *);
static int winreg_s_AbortShutdown(void *, struct mlrpc_xaction *);
static int winreg_s_GetVersion(void *, struct mlrpc_xaction *);

static mlrpc_stub_table_t winreg_stub_table[] = {
	{ winreg_s_OpenHK,	WINREG_OPNUM_OpenHKCR },
	{ winreg_s_OpenHK,	WINREG_OPNUM_OpenHKCU },
	{ winreg_s_OpenHKLM,	WINREG_OPNUM_OpenHKLM },
	{ winreg_s_OpenHK,	WINREG_OPNUM_OpenHKPD },
	{ winreg_s_OpenHKUsers,	WINREG_OPNUM_OpenHKUsers },
	{ winreg_s_Close,	WINREG_OPNUM_Close },
	{ winreg_s_CreateKey,	WINREG_OPNUM_CreateKey },
	{ winreg_s_DeleteKey,	WINREG_OPNUM_DeleteKey },
	{ winreg_s_DeleteValue,	WINREG_OPNUM_DeleteValue },
	{ winreg_s_EnumKey,	WINREG_OPNUM_EnumKey },
	{ winreg_s_EnumValue,	WINREG_OPNUM_EnumValue },
	{ winreg_s_FlushKey,	WINREG_OPNUM_FlushKey },
	{ winreg_s_GetKeySec,	WINREG_OPNUM_GetKeySec },
	{ winreg_s_NotifyChange,	WINREG_OPNUM_NotifyChange },
	{ winreg_s_OpenKey,	WINREG_OPNUM_OpenKey },
	{ winreg_s_QueryKey,	WINREG_OPNUM_QueryKey },
	{ winreg_s_QueryValue,	WINREG_OPNUM_QueryValue },
	{ winreg_s_SetKeySec,	WINREG_OPNUM_SetKeySec },
	{ winreg_s_CreateValue,	WINREG_OPNUM_CreateValue },
	{ winreg_s_Shutdown,	WINREG_OPNUM_Shutdown },
	{ winreg_s_AbortShutdown,	WINREG_OPNUM_AbortShutdown },
	{ winreg_s_GetVersion,	WINREG_OPNUM_GetVersion },
	{ winreg_s_OpenHK,	WINREG_OPNUM_OpenHKCC },
	{ winreg_s_OpenHK,	WINREG_OPNUM_OpenHKDD },
	{ winreg_s_OpenHK,	WINREG_OPNUM_OpenHKPT },
	{ winreg_s_OpenHK,	WINREG_OPNUM_OpenHKPN },
	{0}
};

static mlrpc_service_t winreg_service = {
	"Winreg",			/* name */
	"Windows Registry",		/* desc */
	"\\winreg",			/* endpoint */
	PIPE_WINREG,			/* sec_addr_port */
	"338cd001-2244-31f1-aaaa900038001003", 1,	/* abstract */
	"8a885d04-1ceb-11c9-9fe808002b104860", 2,	/* transfer */
	0,				/* no bind_instance_size */
	0,				/* no bind_req() */
	0,				/* no unbind_and_close() */
	0,				/* use generic_call_stub() */
	&TYPEINFO(winreg_interface),	/* interface ti */
	winreg_stub_table		/* stub_table */
};

static char winreg_sysname[SYS_NMLN];

/*
 * winreg_initialize
 *
 * Initialize and register the WINREG RPC interface with the RPC runtime
 * library. It must be called in order to use either the client side
 * or the server side functions.
 */
void
winreg_initialize(void)
{
	winreg_subkey_t *key;
	struct utsname name;
	char *sysname;
	int i;

	list_create(&winreg_keylist.kl_list, sizeof (winreg_subkey_t),
	    offsetof(winreg_subkey_t, sk_lnd));
	winreg_keylist.kl_count = 0;

	for (i = 0; i < sizeof (winreg_keys)/sizeof (winreg_keys[0]); ++i) {
		if ((key = malloc(sizeof (winreg_subkey_t))) != NULL) {
			bzero(key, sizeof (winreg_subkey_t));
			(void) strlcpy(key->sk_name, winreg_keys[i],
			    MAXPATHLEN);
			key->sk_predefined = B_TRUE;
			list_insert_tail(&winreg_keylist.kl_list, key);
			++winreg_keylist.kl_count;
		}
	}

	if (uname(&name) < 0)
		sysname = "Solaris";
	else
		sysname = name.sysname;

	(void) strlcpy(winreg_sysname, sysname, SYS_NMLN);
	(void) mlrpc_register_service(&winreg_service);
}

/*
 * winreg_s_OpenHK
 *
 * Stub.
 */
static int
winreg_s_OpenHK(void *arg, struct mlrpc_xaction *mxa)
{
	struct winreg_OpenHKCR *param = arg;
	ndr_hdid_t *id;

	if ((id = ndr_hdalloc(mxa, &winreg_hk)) == NULL) {
		bzero(&param->handle, sizeof (winreg_handle_t));
		param->status = ERROR_ACCESS_DENIED;
	} else {
		bcopy(id, &param->handle, sizeof (winreg_handle_t));
		param->status = ERROR_SUCCESS;
	}

	return (MLRPC_DRC_OK);
}

/*
 * winreg_s_OpenHKLM
 *
 * This is a request to open the HKLM and get a handle.
 * The client should treat the handle as an opaque object.
 *
 * Status:
 *	ERROR_SUCCESS		Valid handle returned.
 *	ERROR_ACCESS_DENIED	Unable to allocate a handle.
 */
static int
winreg_s_OpenHKLM(void *arg, struct mlrpc_xaction *mxa)
{
	struct winreg_OpenHKLM *param = arg;
	ndr_hdid_t *id;

	if ((id = ndr_hdalloc(mxa, &winreg_hklm)) == NULL) {
		bzero(&param->handle, sizeof (winreg_handle_t));
		param->status = ERROR_ACCESS_DENIED;
	} else {
		bcopy(id, &param->handle, sizeof (winreg_handle_t));
		param->status = ERROR_SUCCESS;
	}

	return (MLRPC_DRC_OK);
}

/*
 * winreg_s_OpenHKUsers
 *
 * This is a request to get a HKUsers handle. I'm not sure we are
 * ready to fully support this interface yet, mostly due to the need
 * to support subsequent requests, but we may support enough now. It
 * seems okay with regedt32.
 */
static int
winreg_s_OpenHKUsers(void *arg, struct mlrpc_xaction *mxa)
{
	struct winreg_OpenHKUsers *param = arg;
	ndr_hdid_t *id;

	if ((id = ndr_hdalloc(mxa, &winreg_hkuser)) == NULL) {
		bzero(&param->handle, sizeof (winreg_handle_t));
		param->status = ERROR_ACCESS_DENIED;
	} else {
		bcopy(id, &param->handle, sizeof (winreg_handle_t));
		param->status = ERROR_SUCCESS;
	}

	return (MLRPC_DRC_OK);
}

/*
 * winreg_s_Close
 *
 * This is a request to close the WINREG interface specified by the
 * handle. We don't track handles (yet), so just zero out the handle
 * and return MLRPC_DRC_OK. Setting the handle to zero appears to be
 * standard behaviour.
 */
static int
winreg_s_Close(void *arg, struct mlrpc_xaction *mxa)
{
	struct winreg_Close *param = arg;
	ndr_hdid_t *id = (ndr_hdid_t *)&param->handle;

	ndr_hdfree(mxa, id);

	bzero(&param->result_handle, sizeof (winreg_handle_t));
	param->status = ERROR_SUCCESS;
	return (MLRPC_DRC_OK);
}

/*
 * winreg_s_CreateKey
 */
static int
winreg_s_CreateKey(void *arg, struct mlrpc_xaction *mxa)
{
	struct winreg_CreateKey *param = arg;
	ndr_hdid_t *id = (ndr_hdid_t *)&param->handle;
	ndr_handle_t *hd;
	winreg_subkey_t *key;
	char *subkey;
	DWORD *action;

	subkey = (char *)param->subkey.str;

	if (!ndr_is_admin(mxa) || (subkey == NULL)) {
		bzero(param, sizeof (struct winreg_CreateKey));
		param->status = ERROR_ACCESS_DENIED;
		return (MLRPC_DRC_OK);
	}

	hd = ndr_hdlookup(mxa, id);
	if (hd == NULL) {
		bzero(param, sizeof (struct winreg_CreateKey));
		param->status = ERROR_INVALID_HANDLE;
		return (MLRPC_DRC_OK);
	}

	if ((action = MLRPC_HEAP_NEW(mxa, DWORD)) == NULL) {
		bzero(param, sizeof (struct winreg_CreateKey));
		param->status = ERROR_NOT_ENOUGH_MEMORY;
		return (MLRPC_DRC_OK);
	}

	if (list_is_empty(&winreg_keylist.kl_list))
		goto new_key;

	/*
	 * Check for an existing key.
	 */
	key = list_head(&winreg_keylist.kl_list);
	do {
		if (strcasecmp(subkey, key->sk_name) == 0) {
			bcopy(&key->sk_handle, &param->result_handle,
			    sizeof (winreg_handle_t));
			*action = WINREG_ACTION_EXISTING_KEY;
			param->action = action;
			param->status = ERROR_SUCCESS;
			return (MLRPC_DRC_OK);
		}
	} while ((key = list_next(&winreg_keylist.kl_list, key)) != NULL);

new_key:
	/*
	 * Create a new key.
	 */
	id = ndr_hdalloc(mxa, &winreg_hkkey);
	key = malloc(sizeof (winreg_subkey_t));

	if ((id == NULL) || (key == NULL)) {
		bzero(param, sizeof (struct winreg_CreateKey));
		param->status = ERROR_NOT_ENOUGH_MEMORY;
		return (MLRPC_DRC_OK);
	}

	bcopy(id, &key->sk_handle, sizeof (ndr_hdid_t));
	(void) strlcpy(key->sk_name, subkey, MAXPATHLEN);
	key->sk_predefined = B_FALSE;
	list_insert_tail(&winreg_keylist.kl_list, key);
	++winreg_keylist.kl_count;

	bcopy(id, &param->result_handle, sizeof (winreg_handle_t));
	*action = WINREG_ACTION_NEW_KEY;
	param->action = action;
	param->status = ERROR_SUCCESS;
	return (MLRPC_DRC_OK);
}

/*
 * winreg_s_DeleteKey
 */
static int
winreg_s_DeleteKey(void *arg, struct mlrpc_xaction *mxa)
{
	struct winreg_DeleteKey *param = arg;
	ndr_hdid_t *id = (ndr_hdid_t *)&param->handle;
	winreg_subkey_t *key;
	char *subkey;

	subkey = (char *)param->subkey.str;

	if (!ndr_is_admin(mxa) || (subkey == NULL)) {
		param->status = ERROR_ACCESS_DENIED;
		return (MLRPC_DRC_OK);
	}

	if ((ndr_hdlookup(mxa, id) == NULL) ||
	    list_is_empty(&winreg_keylist.kl_list) ||
	    winreg_key_has_subkey(subkey)) {
		param->status = ERROR_ACCESS_DENIED;
		return (MLRPC_DRC_OK);
	}

	key = list_head(&winreg_keylist.kl_list);
	do {
		if (strcasecmp(subkey, key->sk_name) == 0) {
			if (key->sk_predefined == B_TRUE) {
				/* Predefined keys cannot be deleted */
				break;
			}

			list_remove(&winreg_keylist.kl_list, key);
			--winreg_keylist.kl_count;
			ndr_hdfree(mxa, &key->sk_handle);
			free(key);
			param->status = ERROR_SUCCESS;
			return (MLRPC_DRC_OK);
		}
	} while ((key = list_next(&winreg_keylist.kl_list, key)) != NULL);

	param->status = ERROR_ACCESS_DENIED;
	return (MLRPC_DRC_OK);
}

static boolean_t
winreg_key_has_subkey(const char *subkey)
{
	winreg_subkey_t *key;
	int keylen;

	if (list_is_empty(&winreg_keylist.kl_list))
		return (B_FALSE);

	keylen = strlen(subkey);

	key = list_head(&winreg_keylist.kl_list);
	do {
		if (strncasecmp(subkey, key->sk_name, keylen) == 0) {
			/*
			 * Potential match.  If sk_name is longer than
			 * subkey, then sk_name is a subkey of our key.
			 */
			if (keylen < strlen(key->sk_name))
				return (B_TRUE);
		}
	} while ((key = list_next(&winreg_keylist.kl_list, key)) != NULL);

	return (B_FALSE);
}

/*
 * winreg_s_DeleteValue
 */
/*ARGSUSED*/
static int
winreg_s_DeleteValue(void *arg, struct mlrpc_xaction *mxa)
{
	struct winreg_DeleteValue *param = arg;

	param->status = ERROR_ACCESS_DENIED;
	return (MLRPC_DRC_OK);
}

/*
 * winreg_s_EnumKey
 */
static int
winreg_s_EnumKey(void *arg, struct mlrpc_xaction *mxa)
{
	struct winreg_EnumKey *param = arg;
	ndr_hdid_t *id = (ndr_hdid_t *)&param->handle;

	if (ndr_hdlookup(mxa, id) == NULL) {
		bzero(param, sizeof (struct winreg_EnumKey));
		param->status = ERROR_NO_MORE_ITEMS;
		return (MLRPC_DRC_OK);
	}

	bzero(param, sizeof (struct winreg_EnumKey));
	param->status = ERROR_NO_MORE_ITEMS;
	return (MLRPC_DRC_OK);
}

/*
 * winreg_s_EnumValue
 */
static int
winreg_s_EnumValue(void *arg, struct mlrpc_xaction *mxa)
{
	struct winreg_EnumValue *param = arg;
	ndr_hdid_t *id = (ndr_hdid_t *)&param->handle;

	if (ndr_hdlookup(mxa, id) == NULL) {
		bzero(param, sizeof (struct winreg_EnumValue));
		param->status = ERROR_NO_MORE_ITEMS;
		return (MLRPC_DRC_OK);
	}

	bzero(param, sizeof (struct winreg_EnumValue));
	param->status = ERROR_NO_MORE_ITEMS;
	return (MLRPC_DRC_OK);
}

/*
 * winreg_s_FlushKey
 *
 * Flush the attributes associated with the specified open key to disk.
 */
static int
winreg_s_FlushKey(void *arg, struct mlrpc_xaction *mxa)
{
	struct winreg_FlushKey *param = arg;
	ndr_hdid_t *id = (ndr_hdid_t *)&param->handle;

	if (ndr_hdlookup(mxa, id) == NULL)
		param->status = ERROR_INVALID_HANDLE;
	else
		param->status = ERROR_SUCCESS;

	return (MLRPC_DRC_OK);
}

/*
 * winreg_s_GetKeySec
 */
/*ARGSUSED*/
static int
winreg_s_GetKeySec(void *arg, struct mlrpc_xaction *mxa)
{
	struct winreg_GetKeySec *param = arg;

	bzero(param, sizeof (struct winreg_GetKeySec));
	param->status = ERROR_ACCESS_DENIED;
	return (MLRPC_DRC_OK);
}

/*
 * winreg_s_NotifyChange
 */
static int
winreg_s_NotifyChange(void *arg, struct mlrpc_xaction *mxa)
{
	struct winreg_NotifyChange *param = arg;

	if (ndr_is_admin(mxa))
		param->status = ERROR_SUCCESS;
	else
		param->status = ERROR_ACCESS_DENIED;

	return (MLRPC_DRC_OK);
}

/*
 * winreg_s_OpenKey
 *
 * This is a request to open a windows registry key.
 * If we recognize the key, we return a handle.
 *
 * Returns:
 *	ERROR_SUCCESS		Valid handle returned.
 *	ERROR_FILE_NOT_FOUND	No key or unable to allocate a handle.
 */
static int
winreg_s_OpenKey(void *arg, struct mlrpc_xaction *mxa)
{
	struct winreg_OpenKey *param = arg;
	char *subkey = (char *)param->name.str;
	ndr_hdid_t *id = NULL;
	winreg_subkey_t *key;

	if (list_is_empty(&winreg_keylist.kl_list)) {
		bzero(&param->result_handle, sizeof (winreg_handle_t));
		param->status = ERROR_FILE_NOT_FOUND;
		return (MLRPC_DRC_OK);
	}

	key = list_head(&winreg_keylist.kl_list);
	do {
		if (strcasecmp(subkey, key->sk_name) == 0) {
			if (key->sk_predefined == B_TRUE)
				id = ndr_hdalloc(mxa, &winreg_hkkey);
			else
				id = &key->sk_handle;

			if (id == NULL)
				break;

			bcopy(id, &param->result_handle,
			    sizeof (winreg_handle_t));
			param->status = ERROR_SUCCESS;
			return (MLRPC_DRC_OK);
		}
	} while ((key = list_next(&winreg_keylist.kl_list, key)) != NULL);

	bzero(&param->result_handle, sizeof (winreg_handle_t));
	param->status = ERROR_FILE_NOT_FOUND;
	return (MLRPC_DRC_OK);
}

/*
 * winreg_s_QueryKey
 */
/*ARGSUSED*/
static int
winreg_s_QueryKey(void *arg, struct mlrpc_xaction *mxa)
{
	static char nullstr[2] = { 0, 0 };
	struct winreg_QueryKey *param = arg;

	bzero(param, sizeof (struct winreg_QueryKey));

	param->name.length = 2;
	param->name.allosize = 0;
	param->name.str = (unsigned char *)nullstr;
	param->status = ERROR_SUCCESS;
	return (MLRPC_DRC_OK);
}

/*
 * winreg_s_QueryValue
 *
 * This is a request to get the value associated with a specified name.
 *
 * Returns:
 *	ERROR_SUCCESS		Value returned.
 *	ERROR_FILE_NOT_FOUND	PrimaryModule is not supported.
 *	ERROR_CANTREAD          No such name or memory problem.
 */
static int
winreg_s_QueryValue(void *arg, struct mlrpc_xaction *mxa)
{
	struct winreg_QueryValue *param = arg;
	struct winreg_value *pv;
	char *name;
	char *value;
	DWORD slen;
	DWORD msize;

	name = (char *)param->value_name.str;

	if (strcasecmp(name, "PrimaryModule") == 0) {
		param->status = ERROR_FILE_NOT_FOUND;
		return (MLRPC_DRC_OK);
	}

	if ((value = winreg_lookup_value(name)) == NULL) {
		param->status = ERROR_CANTREAD;
		return (MLRPC_DRC_OK);
	}

	slen = mts_wcequiv_strlen(value) + sizeof (mts_wchar_t);
	msize = sizeof (struct winreg_value) + slen;

	param->value = (struct winreg_value *)MLRPC_HEAP_MALLOC(mxa, msize);
	param->type = MLRPC_HEAP_NEW(mxa, DWORD);
	param->value_size = MLRPC_HEAP_NEW(mxa, DWORD);
	param->value_size_total = MLRPC_HEAP_NEW(mxa, DWORD);

	if (param->value == NULL || param->type == NULL ||
	    param->value_size == NULL || param->value_size_total == NULL) {
		param->status = ERROR_CANTREAD;
		return (MLRPC_DRC_OK);
	}

	bzero(param->value, msize);
	pv = param->value;
	pv->vc_first_is = 0;
	pv->vc_length_is = slen;
	/*LINTED E_BAD_PTR_CAST_ALIGN*/
	(void) ndr_mbstowcs(NULL, (mts_wchar_t *)pv->value, value, slen);

	*param->type = 1;
	*param->value_size = slen;
	*param->value_size_total = slen;

	param->status = ERROR_SUCCESS;
	return (MLRPC_DRC_OK);
}

/*
 * Lookup a name in the registry and return the associated value.
 * Our registry is a case-insensitive, name-value pair table.
 *
 * Windows ProductType: WinNT, ServerNT, LanmanNT.
 *	Windows NT4.0 workstation: WinNT
 *	Windows NT4.0 server:      ServerNT
 *
 * If LanmanNT is used here, Windows 2000 sends LsarQueryInfoPolicy
 * with info level 6, which we don't support.  If we use ServerNT
 * (as reported by NT4.0 Server) Windows 2000 send requests for
 * levels 3 and 5, which are support.
 *
 * On success, returns a pointer to the value.  Otherwise returns
 * a null pointer.
 */
static char *
winreg_lookup_value(const char *name)
{
	static struct registry {
		char *name;
		char *value;
	} registry[] = {
		{ "ProductType", "ServerNT" },
		{ "Sources",	 NULL }	/* product name */
	};

	int i;

	for (i = 0; i < sizeof (registry)/sizeof (registry[0]); ++i) {
		if (strcasecmp(registry[i].name, name) == 0) {
			if (registry[i].value == NULL)
				return (winreg_sysname);
			else
				return (registry[i].value);
		}
	}

	return (0);
}

/*
 * winreg_s_SetKeySec
 */
/*ARGSUSED*/
static int
winreg_s_SetKeySec(void *arg, struct mlrpc_xaction *mxa)
{
	struct winreg_SetKeySec *param = arg;

	param->status = ERROR_ACCESS_DENIED;
	return (MLRPC_DRC_OK);
}

/*
 * winreg_s_CreateValue
 */
/*ARGSUSED*/
static int
winreg_s_CreateValue(void *arg, struct mlrpc_xaction *mxa)
{
	struct winreg_CreateValue *param = arg;

	param->status = ERROR_ACCESS_DENIED;
	return (MLRPC_DRC_OK);
}

/*
 * winreg_s_Shutdown
 *
 * Attempt to shutdown or reboot the system: access denied.
 */
/*ARGSUSED*/
static int
winreg_s_Shutdown(void *arg, struct mlrpc_xaction *mxa)
{
	struct winreg_Shutdown *param = arg;

	param->status = ERROR_ACCESS_DENIED;
	return (MLRPC_DRC_OK);
}

/*
 * winreg_s_AbortShutdown
 *
 * Abort a shutdown request.
 */
static int
winreg_s_AbortShutdown(void *arg, struct mlrpc_xaction *mxa)
{
	struct winreg_AbortShutdown *param = arg;

	if (ndr_is_admin(mxa))
		param->status = ERROR_SUCCESS;
	else
		param->status = ERROR_ACCESS_DENIED;

	return (MLRPC_DRC_OK);
}

/*
 * winreg_s_GetVersion
 *
 * Return the windows registry version.  The current version is 5.
 * This call is usually made prior to enumerating or querying registry
 * keys or values.
 */
/*ARGSUSED*/
static int
winreg_s_GetVersion(void *arg, struct mlrpc_xaction *mxa)
{
	struct winreg_GetVersion *param = arg;

	param->version = 5;
	param->status = ERROR_SUCCESS;
	return (MLRPC_DRC_OK);
}

#pragma ident	"%Z%%M%	%I%	%E% SMI"

/*
 * src/lib/krb5/asn.1/asn1_k_encode.h
 * 
 * Copyright 1994 by the Massachusetts Institute of Technology.
 * All Rights Reserved.
 *
 * Export of this software from the United States of America may
 *   require a specific license from the United States Government.
 *   It is the responsibility of any person or organization contemplating
 *   export to obtain such a license before exporting.
 * 
 * WITHIN THAT CONSTRAINT, permission to use, copy, modify, and
 * distribute this software and its documentation for any purpose and
 * without fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation, and that
 * the name of M.I.T. not be used in advertising or publicity pertaining
 * to distribution of the software without specific, written prior
 * permission.  Furthermore if you modify this software you must label
 * your software as modified software and not distribute it in such a
 * fashion that it might be confused with the original M.I.T. software.
 * M.I.T. makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 */

#ifndef __ASN1_ENCODE_KRB5_H__
#define __ASN1_ENCODE_KRB5_H__

#include "k5-int.h"
#include <stdio.h>
#include "asn1buf.h"

/*
   Overview

     Encoding routines for various ASN.1 "substructures" as defined in
     the krb5 protocol.

   Operations

    asn1_encode_krb5_flags
    asn1_encode_ap_options
    asn1_encode_ticket_flags
    asn1_encode_kdc_options
    asn1_encode_kerberos_time

    asn1_encode_realm
    asn1_encode_principal_name
    asn1_encode_encrypted_data
    asn1_encode_authorization_data
    asn1_encode_krb5_authdata_elt
    asn1_encode_kdc_rep
    asn1_encode_ticket
    asn1_encode_encryption_key
    asn1_encode_checksum
    asn1_encode_host_address
    asn1_encode_transited_encoding
    asn1_encode_enc_kdc_rep_part
    asn1_encode_kdc_req
    asn1_encode_kdc_req_body
    asn1_encode_krb_safe_body
    asn1_encode_krb_cred_info
    asn1_encode_last_req_entry
    asn1_encode_pa_data

    asn1_encode_host_addresses
    asn1_encode_last_req
    asn1_encode_sequence_of_pa_data
    asn1_encode_sequence_of_ticket
    asn1_encode_sequence_of_enctype
    asn1_encode_sequence_of_checksum
    asn1_encode_sequence_of_krb_cred_info
*/

/*
**** for simple val's ****
asn1_error_code asn1_encode_asn1_type(asn1buf *buf,
                                      const krb5_type val,
				      int *retlen);
   requires  *buf is allocated
   effects   Inserts the encoding of val into *buf and
              returns the length of this encoding in *retlen.
	     Returns ASN1_MISSING_FIELD if a required field is empty in val.
	     Returns ENOMEM if memory runs out.

**** for struct val's ****
asn1_error_code asn1_encode_asn1_type(asn1buf *buf,
                                      const krb5_type *val,
				      int *retlen);
   requires  *buf is allocated
   effects   Inserts the encoding of *val into *buf and
              returns the length of this encoding in *retlen.
	     Returns ASN1_MISSING_FIELD if a required field is empty in val.
	     Returns ENOMEM if memory runs out.

**** for array val's ****
asn1_error_code asn1_encode_asn1_type(asn1buf *buf,
                                      const krb5_type **val,
				      int *retlen);
   requires  *buf is allocated, **val != NULL, *val[0] != NULL,
              **val is a NULL-terminated array of pointers to krb5_type
   effects   Inserts the encoding of **val into *buf and
              returns the length of this encoding in *retlen.
	     Returns ASN1_MISSING_FIELD if a required field is empty in val.
	     Returns ENOMEM if memory runs out.
*/

asn1_error_code asn1_encode_ui_4 (asn1buf *buf,
					    const krb5_ui_4 val,
					    unsigned int *retlen);

asn1_error_code asn1_encode_msgtype (asn1buf *buf,
					       const /*krb5_msgtype*/int val,
					       unsigned int *retlen);

asn1_error_code asn1_encode_realm
	(asn1buf *buf, const krb5_principal val, unsigned int *retlen);

asn1_error_code asn1_encode_principal_name
	(asn1buf *buf, const krb5_principal val, unsigned int *retlen);

asn1_error_code asn1_encode_encrypted_data
	(asn1buf *buf, const krb5_enc_data *val, unsigned int *retlen);

asn1_error_code asn1_encode_krb5_flags
	(asn1buf *buf, const krb5_flags val, unsigned int *retlen);

asn1_error_code asn1_encode_ap_options
	(asn1buf *buf, const krb5_flags val, unsigned int *retlen);

asn1_error_code asn1_encode_ticket_flags
	(asn1buf *buf, const krb5_flags val, unsigned int *retlen);

asn1_error_code asn1_encode_kdc_options
	(asn1buf *buf, const krb5_flags val, unsigned int *retlen);

asn1_error_code asn1_encode_authorization_data
	(asn1buf *buf, const krb5_authdata **val, unsigned int *retlen);

asn1_error_code asn1_encode_krb5_authdata_elt
	(asn1buf *buf, const krb5_authdata *val, unsigned int *retlen);

asn1_error_code asn1_encode_kdc_rep
	(int msg_type, asn1buf *buf, const krb5_kdc_rep *val,
		   unsigned int *retlen);

asn1_error_code asn1_encode_enc_kdc_rep_part
	(asn1buf *buf, const krb5_enc_kdc_rep_part *val,
		   unsigned int *retlen);

asn1_error_code asn1_encode_ticket
	(asn1buf *buf, const krb5_ticket *val, unsigned int *retlen);

asn1_error_code asn1_encode_encryption_key
	(asn1buf *buf, const krb5_keyblock *val, unsigned int *retlen);

asn1_error_code asn1_encode_kerberos_time
	(asn1buf *buf, const krb5_timestamp val, unsigned int *retlen);

asn1_error_code asn1_encode_checksum
	(asn1buf *buf, const krb5_checksum *val, unsigned int *retlen);

asn1_error_code asn1_encode_host_address
	(asn1buf *buf, const krb5_address *val, unsigned int *retlen);

asn1_error_code asn1_encode_host_addresses
	(asn1buf *buf, const krb5_address **val, unsigned int *retlen);

asn1_error_code asn1_encode_transited_encoding
	(asn1buf *buf, const krb5_transited *val, unsigned int *retlen);

asn1_error_code asn1_encode_last_req
	(asn1buf *buf, const krb5_last_req_entry **val,
		   unsigned int *retlen);

asn1_error_code asn1_encode_sequence_of_pa_data
	(asn1buf *buf, const krb5_pa_data **val, unsigned int *retlen);

asn1_error_code asn1_encode_sequence_of_ticket
	(asn1buf *buf, const krb5_ticket **val, unsigned int *retlen);

asn1_error_code asn1_encode_sequence_of_enctype
	(asn1buf *buf,
		   const int len, const krb5_enctype *val,
		   unsigned int *retlen);

asn1_error_code asn1_encode_sequence_of_checksum
	(asn1buf *buf, const krb5_checksum **val, unsigned int *retlen);

asn1_error_code asn1_encode_kdc_req
	(int msg_type,
		   asn1buf *buf,
		   const krb5_kdc_req *val,
		   unsigned int *retlen);

asn1_error_code asn1_encode_kdc_req_body
	(asn1buf *buf, const krb5_kdc_req *val, unsigned int *retlen);

asn1_error_code asn1_encode_krb_safe_body
	(asn1buf *buf, const krb5_safe *val, unsigned int *retlen);

asn1_error_code asn1_encode_sequence_of_krb_cred_info
	(asn1buf *buf, const krb5_cred_info **val, unsigned int *retlen);

asn1_error_code asn1_encode_krb_cred_info
	(asn1buf *buf, const krb5_cred_info *val, unsigned int *retlen);

asn1_error_code asn1_encode_last_req_entry
	(asn1buf *buf, const krb5_last_req_entry *val,
		   unsigned int *retlen);

asn1_error_code asn1_encode_pa_data
	(asn1buf *buf, const krb5_pa_data *val, unsigned int *retlen);

asn1_error_code asn1_encode_alt_method
	(asn1buf *buf, const krb5_alt_method *val,
		   unsigned int *retlen);

asn1_error_code asn1_encode_etype_info_entry
	(asn1buf *buf, const krb5_etype_info_entry *val,
		   unsigned int *retlen, int etype_info2);

asn1_error_code asn1_encode_etype_info
	(asn1buf *buf, const krb5_etype_info_entry **val,
		   unsigned int *retlen, int etype_info2);

asn1_error_code asn1_encode_passwdsequence
	(asn1buf *buf, const passwd_phrase_element *val, unsigned int *retlen);

asn1_error_code asn1_encode_sequence_of_passwdsequence
	(asn1buf *buf, const passwd_phrase_element **val,
	unsigned int *retlen);

asn1_error_code asn1_encode_sam_flags
	(asn1buf * buf, const krb5_flags val, unsigned int *retlen);

asn1_error_code asn1_encode_sam_challenge
	(asn1buf *buf, const krb5_sam_challenge * val, unsigned int *retlen);

asn1_error_code asn1_encode_sam_challenge_2
	(asn1buf *buf, const krb5_sam_challenge_2 * val, unsigned int *retlen);

asn1_error_code asn1_encode_sam_challenge_2_body
	(asn1buf *buf, const krb5_sam_challenge_2_body * val,
		   unsigned int *retlen);

asn1_error_code asn1_encode_sam_key
	(asn1buf *buf, const krb5_sam_key *val, unsigned int *retlen);

asn1_error_code asn1_encode_enc_sam_response_enc
	(asn1buf *buf, const krb5_enc_sam_response_enc *val,
		   unsigned int *retlen);

asn1_error_code asn1_encode_enc_sam_response_enc_2
	(asn1buf *buf, const krb5_enc_sam_response_enc_2 *val,
		   unsigned int *retlen);

asn1_error_code asn1_encode_sam_response
	(asn1buf *buf, const krb5_sam_response *val, unsigned int *retlen);

asn1_error_code asn1_encode_sam_response_2
	(asn1buf *buf, const krb5_sam_response_2 *val, unsigned int *retlen);

asn1_error_code asn1_encode_predicted_sam_response
	(asn1buf *buf, const krb5_predicted_sam_response *val, 
		   unsigned int *retlen);

asn1_error_code asn1_encode_krb_saved_safe_body
	(asn1buf *buf, const krb5_data *body, unsigned int *retlen);

#endif

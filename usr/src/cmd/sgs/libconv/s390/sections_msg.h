#ifndef	_SECTIONS_MSG_DOT_H
#define	_SECTIONS_MSG_DOT_H

#ifndef	__lint

typedef int	Msg;

#define	MSG_ORIG(x)	&__sgs_msg[x]

extern	const char *	_sgs_msg(Msg);

#define	MSG_INTL(x)	_sgs_msg(x)


#define	MSG_SHT_NULL	1
#define	MSG_SHT_NULL_SIZE	12

#define	MSG_SHT_NULL_ALT	14
#define	MSG_SHT_NULL_ALT_SIZE	4

#define	MSG_SHT_PROGBITS	19
#define	MSG_SHT_PROGBITS_SIZE	16

#define	MSG_SHT_PROGBITS_ALT	36
#define	MSG_SHT_PROGBITS_ALT_SIZE	4

#define	MSG_SHT_SYMTAB	41
#define	MSG_SHT_SYMTAB_SIZE	14

#define	MSG_SHT_SYMTAB_ALT	56
#define	MSG_SHT_SYMTAB_ALT_SIZE	4

#define	MSG_SHT_STRTAB	61
#define	MSG_SHT_STRTAB_SIZE	14

#define	MSG_SHT_STRTAB_ALT	76
#define	MSG_SHT_STRTAB_ALT_SIZE	4

#define	MSG_SHT_RELA	81
#define	MSG_SHT_RELA_SIZE	12

#define	MSG_SHT_RELA_ALT	94
#define	MSG_SHT_RELA_ALT_SIZE	4

#define	MSG_SHT_HASH	99
#define	MSG_SHT_HASH_SIZE	12

#define	MSG_SHT_HASH_ALT	112
#define	MSG_SHT_HASH_ALT_SIZE	4

#define	MSG_SHT_DYNAMIC	117
#define	MSG_SHT_DYNAMIC_SIZE	15

#define	MSG_SHT_DYNAMIC_ALT	133
#define	MSG_SHT_DYNAMIC_ALT_SIZE	4

#define	MSG_SHT_NOTE	138
#define	MSG_SHT_NOTE_SIZE	12

#define	MSG_SHT_NOTE_ALT	151
#define	MSG_SHT_NOTE_ALT_SIZE	4

#define	MSG_SHT_NOBITS	156
#define	MSG_SHT_NOBITS_SIZE	14

#define	MSG_SHT_NOBITS_ALT	171
#define	MSG_SHT_NOBITS_ALT_SIZE	4

#define	MSG_SHT_REL	176
#define	MSG_SHT_REL_SIZE	11

#define	MSG_SHT_REL_ALT	188
#define	MSG_SHT_REL_ALT_SIZE	4

#define	MSG_SHT_SHLIB	193
#define	MSG_SHT_SHLIB_SIZE	13

#define	MSG_SHT_SHLIB_ALT	207
#define	MSG_SHT_SHLIB_ALT_SIZE	4

#define	MSG_SHT_DYNSYM	212
#define	MSG_SHT_DYNSYM_SIZE	14

#define	MSG_SHT_DYNSYM_ALT	227
#define	MSG_SHT_DYNSYM_ALT_SIZE	4

#define	MSG_SHT_UNKNOWN12	232
#define	MSG_SHT_UNKNOWN12_SIZE	13

#define	MSG_SHT_UNKNOWN13	246
#define	MSG_SHT_UNKNOWN13_SIZE	13

#define	MSG_SHT_INIT_ARRAY	260
#define	MSG_SHT_INIT_ARRAY_SIZE	18

#define	MSG_SHT_INIT_ARRAY_ALT	279
#define	MSG_SHT_INIT_ARRAY_ALT_SIZE	4

#define	MSG_SHT_FINI_ARRAY	284
#define	MSG_SHT_FINI_ARRAY_SIZE	18

#define	MSG_SHT_FINI_ARRAY_ALT	303
#define	MSG_SHT_FINI_ARRAY_ALT_SIZE	4

#define	MSG_SHT_PREINIT_ARRAY	308
#define	MSG_SHT_PREINIT_ARRAY_SIZE	21

#define	MSG_SHT_PREINIT_ARRAY_ALT	330
#define	MSG_SHT_PREINIT_ARRAY_ALT_SIZE	4

#define	MSG_SHT_GROUP	335
#define	MSG_SHT_GROUP_SIZE	13

#define	MSG_SHT_GROUP_ALT	349
#define	MSG_SHT_GROUP_ALT_SIZE	4

#define	MSG_SHT_SYMTAB_SHNDX	354
#define	MSG_SHT_SYMTAB_SHNDX_SIZE	20

#define	MSG_SHT_SYMTAB_SHNDX_ALT	375
#define	MSG_SHT_SYMTAB_SHNDX_ALT_SIZE	4

#define	MSG_SHT_SUNW_symsort	380
#define	MSG_SHT_SUNW_symsort_SIZE	20

#define	MSG_SHT_SUNW_symsort_ALT	401
#define	MSG_SHT_SUNW_symsort_ALT_SIZE	4

#define	MSG_SHT_SUNW_tlssort	406
#define	MSG_SHT_SUNW_tlssort_SIZE	20

#define	MSG_SHT_SUNW_tlssort_ALT	427
#define	MSG_SHT_SUNW_tlssort_ALT_SIZE	4

#define	MSG_SHT_SUNW_LDYNSYM	432
#define	MSG_SHT_SUNW_LDYNSYM_SIZE	20

#define	MSG_SHT_SUNW_LDYNSYM_ALT	453
#define	MSG_SHT_SUNW_LDYNSYM_ALT_SIZE	4

#define	MSG_SHT_SUNW_dof	458
#define	MSG_SHT_SUNW_dof_SIZE	16

#define	MSG_SHT_SUNW_dof_ALT	475
#define	MSG_SHT_SUNW_dof_ALT_SIZE	4

#define	MSG_SHT_SUNW_cap	480
#define	MSG_SHT_SUNW_cap_SIZE	16

#define	MSG_SHT_SUNW_cap_ALT	497
#define	MSG_SHT_SUNW_cap_ALT_SIZE	4

#define	MSG_SHT_SUNW_SIGNATURE	502
#define	MSG_SHT_SUNW_SIGNATURE_SIZE	22

#define	MSG_SHT_SUNW_SIGNATURE_ALT	525
#define	MSG_SHT_SUNW_SIGNATURE_ALT_SIZE	4

#define	MSG_SHT_SUNW_ANNOTATE	530
#define	MSG_SHT_SUNW_ANNOTATE_SIZE	21

#define	MSG_SHT_SUNW_ANNOTATE_ALT	552
#define	MSG_SHT_SUNW_ANNOTATE_ALT_SIZE	4

#define	MSG_SHT_SUNW_DEBUGSTR	557
#define	MSG_SHT_SUNW_DEBUGSTR_SIZE	21

#define	MSG_SHT_SUNW_DEBUGSTR_ALT	579
#define	MSG_SHT_SUNW_DEBUGSTR_ALT_SIZE	4

#define	MSG_SHT_SUNW_DEBUG	584
#define	MSG_SHT_SUNW_DEBUG_SIZE	18

#define	MSG_SHT_SUNW_DEBUG_ALT	603
#define	MSG_SHT_SUNW_DEBUG_ALT_SIZE	4

#define	MSG_SHT_SUNW_move	608
#define	MSG_SHT_SUNW_move_SIZE	17

#define	MSG_SHT_SUNW_move_ALT	626
#define	MSG_SHT_SUNW_move_ALT_SIZE	4

#define	MSG_SHT_SUNW_COMDAT	631
#define	MSG_SHT_SUNW_COMDAT_SIZE	19

#define	MSG_SHT_SUNW_COMDAT_ALT	651
#define	MSG_SHT_SUNW_COMDAT_ALT_SIZE	4

#define	MSG_SHT_SUNW_syminfo	656
#define	MSG_SHT_SUNW_syminfo_SIZE	20

#define	MSG_SHT_SUNW_syminfo_ALT	677
#define	MSG_SHT_SUNW_syminfo_ALT_SIZE	4

#define	MSG_SHT_SUNW_verdef	682
#define	MSG_SHT_SUNW_verdef_SIZE	19

#define	MSG_SHT_SUNW_verdef_ALT	702
#define	MSG_SHT_SUNW_verdef_ALT_SIZE	4

#define	MSG_SHT_SUNW_verneed	707
#define	MSG_SHT_SUNW_verneed_SIZE	20

#define	MSG_SHT_SUNW_verneed_ALT	728
#define	MSG_SHT_SUNW_verneed_ALT_SIZE	4

#define	MSG_SHT_SUNW_versym	733
#define	MSG_SHT_SUNW_versym_SIZE	19

#define	MSG_SHT_SUNW_versym_ALT	753
#define	MSG_SHT_SUNW_versym_ALT_SIZE	4

#define	MSG_SHT_AMD64_UNWIND	758
#define	MSG_SHT_AMD64_UNWIND_SIZE	20

#define	MSG_SHT_AMD64_UNWIND_ALT	779
#define	MSG_SHT_AMD64_UNWIND_ALT_SIZE	4

#define	MSG_SHT_SPARC_GOTDATA	784
#define	MSG_SHT_SPARC_GOTDATA_SIZE	21

#define	MSG_SHT_SPARC_GOTDATA_ALT	806
#define	MSG_SHT_SPARC_GOTDATA_ALT_SIZE	4

#define	MSG_SHN_AFTER	811
#define	MSG_SHN_AFTER_SIZE	13

#define	MSG_SHN_BEFORE	825
#define	MSG_SHN_BEFORE_SIZE	14

#define	MSG_SHF_WRITE	840
#define	MSG_SHF_WRITE_SIZE	9

#define	MSG_SHF_ALLOC	850
#define	MSG_SHF_ALLOC_SIZE	9

#define	MSG_SHF_EXECINSTR	860
#define	MSG_SHF_EXECINSTR_SIZE	13

#define	MSG_SHF_MERGE	874
#define	MSG_SHF_MERGE_SIZE	9

#define	MSG_SHF_STRINGS	884
#define	MSG_SHF_STRINGS_SIZE	11

#define	MSG_SHF_INFO_LINK	896
#define	MSG_SHF_INFO_LINK_SIZE	13

#define	MSG_SHF_LINK_ORDER	910
#define	MSG_SHF_LINK_ORDER_SIZE	14

#define	MSG_SHF_OS_NONCONFORMING	925
#define	MSG_SHF_OS_NONCONFORMING_SIZE	20

#define	MSG_SHF_GROUP	946
#define	MSG_SHF_GROUP_SIZE	9

#define	MSG_SHF_TLS	956
#define	MSG_SHF_TLS_SIZE	7

#define	MSG_SHF_EXCLUDE	964
#define	MSG_SHF_EXCLUDE_SIZE	11

#define	MSG_SHF_ORDERED	976
#define	MSG_SHF_ORDERED_SIZE	11

#define	MSG_SHF_AMD64_LARGE	988
#define	MSG_SHF_AMD64_LARGE_SIZE	15

#define	MSG_GBL_ZERO	1004
#define	MSG_GBL_ZERO_SIZE	1

static const char __sgs_msg[1006] = { 
/*    0 */ 0x00,  0x5b,  0x20,  0x53,  0x48,  0x54,  0x5f,  0x4e,  0x55,  0x4c,
/*   10 */ 0x4c,  0x20,  0x5d,  0x00,  0x4e,  0x55,  0x4c,  0x4c,  0x00,  0x5b,
/*   20 */ 0x20,  0x53,  0x48,  0x54,  0x5f,  0x50,  0x52,  0x4f,  0x47,  0x42,
/*   30 */ 0x49,  0x54,  0x53,  0x20,  0x5d,  0x00,  0x50,  0x42,  0x49,  0x54,
/*   40 */ 0x00,  0x5b,  0x20,  0x53,  0x48,  0x54,  0x5f,  0x53,  0x59,  0x4d,
/*   50 */ 0x54,  0x41,  0x42,  0x20,  0x5d,  0x00,  0x53,  0x59,  0x4d,  0x54,
/*   60 */ 0x00,  0x5b,  0x20,  0x53,  0x48,  0x54,  0x5f,  0x53,  0x54,  0x52,
/*   70 */ 0x54,  0x41,  0x42,  0x20,  0x5d,  0x00,  0x53,  0x54,  0x52,  0x54,
/*   80 */ 0x00,  0x5b,  0x20,  0x53,  0x48,  0x54,  0x5f,  0x52,  0x45,  0x4c,
/*   90 */ 0x41,  0x20,  0x5d,  0x00,  0x52,  0x45,  0x4c,  0x41,  0x00,  0x5b,
/*  100 */ 0x20,  0x53,  0x48,  0x54,  0x5f,  0x48,  0x41,  0x53,  0x48,  0x20,
/*  110 */ 0x5d,  0x00,  0x48,  0x41,  0x53,  0x48,  0x00,  0x5b,  0x20,  0x53,
/*  120 */ 0x48,  0x54,  0x5f,  0x44,  0x59,  0x4e,  0x41,  0x4d,  0x49,  0x43,
/*  130 */ 0x20,  0x5d,  0x00,  0x44,  0x59,  0x4e,  0x4d,  0x00,  0x5b,  0x20,
/*  140 */ 0x53,  0x48,  0x54,  0x5f,  0x4e,  0x4f,  0x54,  0x45,  0x20,  0x5d,
/*  150 */ 0x00,  0x4e,  0x4f,  0x54,  0x45,  0x00,  0x5b,  0x20,  0x53,  0x48,
/*  160 */ 0x54,  0x5f,  0x4e,  0x4f,  0x42,  0x49,  0x54,  0x53,  0x20,  0x5d,
/*  170 */ 0x00,  0x4e,  0x4f,  0x42,  0x49,  0x00,  0x5b,  0x20,  0x53,  0x48,
/*  180 */ 0x54,  0x5f,  0x52,  0x45,  0x4c,  0x20,  0x5d,  0x00,  0x52,  0x45,
/*  190 */ 0x4c,  0x20,  0x00,  0x5b,  0x20,  0x53,  0x48,  0x54,  0x5f,  0x53,
/*  200 */ 0x48,  0x4c,  0x49,  0x42,  0x20,  0x5d,  0x00,  0x53,  0x48,  0x4c,
/*  210 */ 0x42,  0x00,  0x5b,  0x20,  0x53,  0x48,  0x54,  0x5f,  0x44,  0x59,
/*  220 */ 0x4e,  0x53,  0x59,  0x4d,  0x20,  0x5d,  0x00,  0x44,  0x59,  0x4e,
/*  230 */ 0x53,  0x00,  0x5b,  0x20,  0x55,  0x4e,  0x4b,  0x4e,  0x4f,  0x57,
/*  240 */ 0x4e,  0x31,  0x32,  0x20,  0x5d,  0x00,  0x5b,  0x20,  0x55,  0x4e,
/*  250 */ 0x4b,  0x4e,  0x4f,  0x57,  0x4e,  0x31,  0x33,  0x20,  0x5d,  0x00,
/*  260 */ 0x5b,  0x20,  0x53,  0x48,  0x54,  0x5f,  0x49,  0x4e,  0x49,  0x54,
/*  270 */ 0x5f,  0x41,  0x52,  0x52,  0x41,  0x59,  0x20,  0x5d,  0x00,  0x49,
/*  280 */ 0x4e,  0x41,  0x52,  0x00,  0x5b,  0x20,  0x53,  0x48,  0x54,  0x5f,
/*  290 */ 0x46,  0x49,  0x4e,  0x49,  0x5f,  0x41,  0x52,  0x52,  0x41,  0x59,
/*  300 */ 0x20,  0x5d,  0x00,  0x46,  0x4e,  0x41,  0x52,  0x00,  0x5b,  0x20,
/*  310 */ 0x53,  0x48,  0x54,  0x5f,  0x50,  0x52,  0x45,  0x49,  0x4e,  0x49,
/*  320 */ 0x54,  0x5f,  0x41,  0x52,  0x52,  0x41,  0x59,  0x20,  0x5d,  0x00,
/*  330 */ 0x50,  0x4e,  0x41,  0x52,  0x00,  0x5b,  0x20,  0x53,  0x48,  0x54,
/*  340 */ 0x5f,  0x47,  0x52,  0x4f,  0x55,  0x50,  0x20,  0x5d,  0x00,  0x47,
/*  350 */ 0x52,  0x50,  0x20,  0x00,  0x5b,  0x20,  0x53,  0x48,  0x54,  0x5f,
/*  360 */ 0x53,  0x59,  0x4d,  0x54,  0x41,  0x42,  0x5f,  0x53,  0x48,  0x4e,
/*  370 */ 0x44,  0x58,  0x20,  0x5d,  0x00,  0x53,  0x48,  0x44,  0x58,  0x00,
/*  380 */ 0x5b,  0x20,  0x53,  0x48,  0x54,  0x5f,  0x53,  0x55,  0x4e,  0x57,
/*  390 */ 0x5f,  0x73,  0x79,  0x6d,  0x73,  0x6f,  0x72,  0x74,  0x20,  0x5d,
/*  400 */ 0x00,  0x53,  0x53,  0x52,  0x54,  0x00,  0x5b,  0x20,  0x53,  0x48,
/*  410 */ 0x54,  0x5f,  0x53,  0x55,  0x4e,  0x57,  0x5f,  0x74,  0x6c,  0x73,
/*  420 */ 0x73,  0x6f,  0x72,  0x74,  0x20,  0x5d,  0x00,  0x54,  0x53,  0x52,
/*  430 */ 0x54,  0x00,  0x5b,  0x20,  0x53,  0x48,  0x54,  0x5f,  0x53,  0x55,
/*  440 */ 0x4e,  0x57,  0x5f,  0x4c,  0x44,  0x59,  0x4e,  0x53,  0x59,  0x4d,
/*  450 */ 0x20,  0x5d,  0x00,  0x4c,  0x44,  0x53,  0x4d,  0x00,  0x5b,  0x20,
/*  460 */ 0x53,  0x48,  0x54,  0x5f,  0x53,  0x55,  0x4e,  0x57,  0x5f,  0x64,
/*  470 */ 0x6f,  0x66,  0x20,  0x5d,  0x00,  0x44,  0x4f,  0x46,  0x20,  0x00,
/*  480 */ 0x5b,  0x20,  0x53,  0x48,  0x54,  0x5f,  0x53,  0x55,  0x4e,  0x57,
/*  490 */ 0x5f,  0x63,  0x61,  0x70,  0x20,  0x5d,  0x00,  0x43,  0x41,  0x50,
/*  500 */ 0x20,  0x00,  0x5b,  0x20,  0x53,  0x48,  0x54,  0x5f,  0x53,  0x55,
/*  510 */ 0x4e,  0x57,  0x5f,  0x53,  0x49,  0x47,  0x4e,  0x41,  0x54,  0x55,
/*  520 */ 0x52,  0x45,  0x20,  0x5d,  0x00,  0x53,  0x49,  0x47,  0x4e,  0x00,
/*  530 */ 0x5b,  0x20,  0x53,  0x48,  0x54,  0x5f,  0x53,  0x55,  0x4e,  0x57,
/*  540 */ 0x5f,  0x41,  0x4e,  0x4e,  0x4f,  0x54,  0x41,  0x54,  0x45,  0x20,
/*  550 */ 0x5d,  0x00,  0x41,  0x4e,  0x4f,  0x54,  0x00,  0x5b,  0x20,  0x53,
/*  560 */ 0x48,  0x54,  0x5f,  0x53,  0x55,  0x4e,  0x57,  0x5f,  0x44,  0x45,
/*  570 */ 0x42,  0x55,  0x47,  0x53,  0x54,  0x52,  0x20,  0x5d,  0x00,  0x44,
/*  580 */ 0x42,  0x47,  0x53,  0x00,  0x5b,  0x20,  0x53,  0x48,  0x54,  0x5f,
/*  590 */ 0x53,  0x55,  0x4e,  0x57,  0x5f,  0x44,  0x45,  0x42,  0x55,  0x47,
/*  600 */ 0x20,  0x5d,  0x00,  0x44,  0x42,  0x47,  0x20,  0x00,  0x5b,  0x20,
/*  610 */ 0x53,  0x48,  0x54,  0x5f,  0x53,  0x55,  0x4e,  0x57,  0x5f,  0x6d,
/*  620 */ 0x6f,  0x76,  0x65,  0x20,  0x5d,  0x00,  0x4d,  0x4f,  0x56,  0x45,
/*  630 */ 0x00,  0x5b,  0x20,  0x53,  0x48,  0x54,  0x5f,  0x53,  0x55,  0x4e,
/*  640 */ 0x57,  0x5f,  0x43,  0x4f,  0x4d,  0x44,  0x41,  0x54,  0x20,  0x5d,
/*  650 */ 0x00,  0x43,  0x4f,  0x4d,  0x44,  0x00,  0x5b,  0x20,  0x53,  0x48,
/*  660 */ 0x54,  0x5f,  0x53,  0x55,  0x4e,  0x57,  0x5f,  0x73,  0x79,  0x6d,
/*  670 */ 0x69,  0x6e,  0x66,  0x6f,  0x20,  0x5d,  0x00,  0x53,  0x59,  0x4d,
/*  680 */ 0x49,  0x00,  0x5b,  0x20,  0x53,  0x48,  0x54,  0x5f,  0x53,  0x55,
/*  690 */ 0x4e,  0x57,  0x5f,  0x76,  0x65,  0x72,  0x64,  0x65,  0x66,  0x20,
/*  700 */ 0x5d,  0x00,  0x56,  0x45,  0x52,  0x44,  0x00,  0x5b,  0x20,  0x53,
/*  710 */ 0x48,  0x54,  0x5f,  0x53,  0x55,  0x4e,  0x57,  0x5f,  0x76,  0x65,
/*  720 */ 0x72,  0x6e,  0x65,  0x65,  0x64,  0x20,  0x5d,  0x00,  0x56,  0x45,
/*  730 */ 0x52,  0x4e,  0x00,  0x5b,  0x20,  0x53,  0x48,  0x54,  0x5f,  0x53,
/*  740 */ 0x55,  0x4e,  0x57,  0x5f,  0x76,  0x65,  0x72,  0x73,  0x79,  0x6d,
/*  750 */ 0x20,  0x5d,  0x00,  0x56,  0x45,  0x52,  0x53,  0x00,  0x5b,  0x20,
/*  760 */ 0x53,  0x48,  0x54,  0x5f,  0x41,  0x4d,  0x44,  0x36,  0x34,  0x5f,
/*  770 */ 0x55,  0x4e,  0x57,  0x49,  0x4e,  0x44,  0x20,  0x5d,  0x00,  0x55,
/*  780 */ 0x4e,  0x57,  0x44,  0x00,  0x5b,  0x20,  0x53,  0x48,  0x54,  0x5f,
/*  790 */ 0x53,  0x50,  0x41,  0x52,  0x43,  0x5f,  0x47,  0x4f,  0x54,  0x44,
/*  800 */ 0x41,  0x54,  0x41,  0x20,  0x5d,  0x00,  0x47,  0x4f,  0x54,  0x44,
/*  810 */ 0x00,  0x5b,  0x20,  0x53,  0x48,  0x4e,  0x5f,  0x41,  0x46,  0x54,
/*  820 */ 0x45,  0x52,  0x20,  0x5d,  0x00,  0x5b,  0x20,  0x53,  0x48,  0x4e,
/*  830 */ 0x5f,  0x42,  0x45,  0x46,  0x4f,  0x52,  0x45,  0x20,  0x5d,  0x00,
/*  840 */ 0x53,  0x48,  0x46,  0x5f,  0x57,  0x52,  0x49,  0x54,  0x45,  0x00,
/*  850 */ 0x53,  0x48,  0x46,  0x5f,  0x41,  0x4c,  0x4c,  0x4f,  0x43,  0x00,
/*  860 */ 0x53,  0x48,  0x46,  0x5f,  0x45,  0x58,  0x45,  0x43,  0x49,  0x4e,
/*  870 */ 0x53,  0x54,  0x52,  0x00,  0x53,  0x48,  0x46,  0x5f,  0x4d,  0x45,
/*  880 */ 0x52,  0x47,  0x45,  0x00,  0x53,  0x48,  0x46,  0x5f,  0x53,  0x54,
/*  890 */ 0x52,  0x49,  0x4e,  0x47,  0x53,  0x00,  0x53,  0x48,  0x46,  0x5f,
/*  900 */ 0x49,  0x4e,  0x46,  0x4f,  0x5f,  0x4c,  0x49,  0x4e,  0x4b,  0x00,
/*  910 */ 0x53,  0x48,  0x46,  0x5f,  0x4c,  0x49,  0x4e,  0x4b,  0x5f,  0x4f,
/*  920 */ 0x52,  0x44,  0x45,  0x52,  0x00,  0x53,  0x48,  0x46,  0x5f,  0x4f,
/*  930 */ 0x53,  0x5f,  0x4e,  0x4f,  0x4e,  0x43,  0x4f,  0x4e,  0x46,  0x4f,
/*  940 */ 0x52,  0x4d,  0x49,  0x4e,  0x47,  0x00,  0x53,  0x48,  0x46,  0x5f,
/*  950 */ 0x47,  0x52,  0x4f,  0x55,  0x50,  0x00,  0x53,  0x48,  0x46,  0x5f,
/*  960 */ 0x54,  0x4c,  0x53,  0x00,  0x53,  0x48,  0x46,  0x5f,  0x45,  0x58,
/*  970 */ 0x43,  0x4c,  0x55,  0x44,  0x45,  0x00,  0x53,  0x48,  0x46,  0x5f,
/*  980 */ 0x4f,  0x52,  0x44,  0x45,  0x52,  0x45,  0x44,  0x00,  0x53,  0x48,
/*  990 */ 0x46,  0x5f,  0x41,  0x4d,  0x44,  0x36,  0x34,  0x5f,  0x4c,  0x41,
/* 1000 */ 0x52,  0x47,  0x45,  0x00,  0x30,  0x00 };

#else	/* __lint */


typedef char *	Msg;

extern	const char *	_sgs_msg(Msg);

#define MSG_ORIG(x)	x
#define MSG_INTL(x)	x

#define	MSG_SHT_NULL	"[ SHT_NULL ]"
#define	MSG_SHT_NULL_SIZE	12

#define	MSG_SHT_NULL_ALT	"NULL"
#define	MSG_SHT_NULL_ALT_SIZE	4

#define	MSG_SHT_PROGBITS	"[ SHT_PROGBITS ]"
#define	MSG_SHT_PROGBITS_SIZE	16

#define	MSG_SHT_PROGBITS_ALT	"PBIT"
#define	MSG_SHT_PROGBITS_ALT_SIZE	4

#define	MSG_SHT_SYMTAB	"[ SHT_SYMTAB ]"
#define	MSG_SHT_SYMTAB_SIZE	14

#define	MSG_SHT_SYMTAB_ALT	"SYMT"
#define	MSG_SHT_SYMTAB_ALT_SIZE	4

#define	MSG_SHT_STRTAB	"[ SHT_STRTAB ]"
#define	MSG_SHT_STRTAB_SIZE	14

#define	MSG_SHT_STRTAB_ALT	"STRT"
#define	MSG_SHT_STRTAB_ALT_SIZE	4

#define	MSG_SHT_RELA	"[ SHT_RELA ]"
#define	MSG_SHT_RELA_SIZE	12

#define	MSG_SHT_RELA_ALT	"RELA"
#define	MSG_SHT_RELA_ALT_SIZE	4

#define	MSG_SHT_HASH	"[ SHT_HASH ]"
#define	MSG_SHT_HASH_SIZE	12

#define	MSG_SHT_HASH_ALT	"HASH"
#define	MSG_SHT_HASH_ALT_SIZE	4

#define	MSG_SHT_DYNAMIC	"[ SHT_DYNAMIC ]"
#define	MSG_SHT_DYNAMIC_SIZE	15

#define	MSG_SHT_DYNAMIC_ALT	"DYNM"
#define	MSG_SHT_DYNAMIC_ALT_SIZE	4

#define	MSG_SHT_NOTE	"[ SHT_NOTE ]"
#define	MSG_SHT_NOTE_SIZE	12

#define	MSG_SHT_NOTE_ALT	"NOTE"
#define	MSG_SHT_NOTE_ALT_SIZE	4

#define	MSG_SHT_NOBITS	"[ SHT_NOBITS ]"
#define	MSG_SHT_NOBITS_SIZE	14

#define	MSG_SHT_NOBITS_ALT	"NOBI"
#define	MSG_SHT_NOBITS_ALT_SIZE	4

#define	MSG_SHT_REL	"[ SHT_REL ]"
#define	MSG_SHT_REL_SIZE	11

#define	MSG_SHT_REL_ALT	"REL "
#define	MSG_SHT_REL_ALT_SIZE	4

#define	MSG_SHT_SHLIB	"[ SHT_SHLIB ]"
#define	MSG_SHT_SHLIB_SIZE	13

#define	MSG_SHT_SHLIB_ALT	"SHLB"
#define	MSG_SHT_SHLIB_ALT_SIZE	4

#define	MSG_SHT_DYNSYM	"[ SHT_DYNSYM ]"
#define	MSG_SHT_DYNSYM_SIZE	14

#define	MSG_SHT_DYNSYM_ALT	"DYNS"
#define	MSG_SHT_DYNSYM_ALT_SIZE	4

#define	MSG_SHT_UNKNOWN12	"[ UNKNOWN12 ]"
#define	MSG_SHT_UNKNOWN12_SIZE	13

#define	MSG_SHT_UNKNOWN13	"[ UNKNOWN13 ]"
#define	MSG_SHT_UNKNOWN13_SIZE	13

#define	MSG_SHT_INIT_ARRAY	"[ SHT_INIT_ARRAY ]"
#define	MSG_SHT_INIT_ARRAY_SIZE	18

#define	MSG_SHT_INIT_ARRAY_ALT	"INAR"
#define	MSG_SHT_INIT_ARRAY_ALT_SIZE	4

#define	MSG_SHT_FINI_ARRAY	"[ SHT_FINI_ARRAY ]"
#define	MSG_SHT_FINI_ARRAY_SIZE	18

#define	MSG_SHT_FINI_ARRAY_ALT	"FNAR"
#define	MSG_SHT_FINI_ARRAY_ALT_SIZE	4

#define	MSG_SHT_PREINIT_ARRAY	"[ SHT_PREINIT_ARRAY ]"
#define	MSG_SHT_PREINIT_ARRAY_SIZE	21

#define	MSG_SHT_PREINIT_ARRAY_ALT	"PNAR"
#define	MSG_SHT_PREINIT_ARRAY_ALT_SIZE	4

#define	MSG_SHT_GROUP	"[ SHT_GROUP ]"
#define	MSG_SHT_GROUP_SIZE	13

#define	MSG_SHT_GROUP_ALT	"GRP "
#define	MSG_SHT_GROUP_ALT_SIZE	4

#define	MSG_SHT_SYMTAB_SHNDX	"[ SHT_SYMTAB_SHNDX ]"
#define	MSG_SHT_SYMTAB_SHNDX_SIZE	20

#define	MSG_SHT_SYMTAB_SHNDX_ALT	"SHDX"
#define	MSG_SHT_SYMTAB_SHNDX_ALT_SIZE	4

#define	MSG_SHT_SUNW_symsort	"[ SHT_SUNW_symsort ]"
#define	MSG_SHT_SUNW_symsort_SIZE	20

#define	MSG_SHT_SUNW_symsort_ALT	"SSRT"
#define	MSG_SHT_SUNW_symsort_ALT_SIZE	4

#define	MSG_SHT_SUNW_tlssort	"[ SHT_SUNW_tlssort ]"
#define	MSG_SHT_SUNW_tlssort_SIZE	20

#define	MSG_SHT_SUNW_tlssort_ALT	"TSRT"
#define	MSG_SHT_SUNW_tlssort_ALT_SIZE	4

#define	MSG_SHT_SUNW_LDYNSYM	"[ SHT_SUNW_LDYNSYM ]"
#define	MSG_SHT_SUNW_LDYNSYM_SIZE	20

#define	MSG_SHT_SUNW_LDYNSYM_ALT	"LDSM"
#define	MSG_SHT_SUNW_LDYNSYM_ALT_SIZE	4

#define	MSG_SHT_SUNW_dof	"[ SHT_SUNW_dof ]"
#define	MSG_SHT_SUNW_dof_SIZE	16

#define	MSG_SHT_SUNW_dof_ALT	"DOF "
#define	MSG_SHT_SUNW_dof_ALT_SIZE	4

#define	MSG_SHT_SUNW_cap	"[ SHT_SUNW_cap ]"
#define	MSG_SHT_SUNW_cap_SIZE	16

#define	MSG_SHT_SUNW_cap_ALT	"CAP "
#define	MSG_SHT_SUNW_cap_ALT_SIZE	4

#define	MSG_SHT_SUNW_SIGNATURE	"[ SHT_SUNW_SIGNATURE ]"
#define	MSG_SHT_SUNW_SIGNATURE_SIZE	22

#define	MSG_SHT_SUNW_SIGNATURE_ALT	"SIGN"
#define	MSG_SHT_SUNW_SIGNATURE_ALT_SIZE	4

#define	MSG_SHT_SUNW_ANNOTATE	"[ SHT_SUNW_ANNOTATE ]"
#define	MSG_SHT_SUNW_ANNOTATE_SIZE	21

#define	MSG_SHT_SUNW_ANNOTATE_ALT	"ANOT"
#define	MSG_SHT_SUNW_ANNOTATE_ALT_SIZE	4

#define	MSG_SHT_SUNW_DEBUGSTR	"[ SHT_SUNW_DEBUGSTR ]"
#define	MSG_SHT_SUNW_DEBUGSTR_SIZE	21

#define	MSG_SHT_SUNW_DEBUGSTR_ALT	"DBGS"
#define	MSG_SHT_SUNW_DEBUGSTR_ALT_SIZE	4

#define	MSG_SHT_SUNW_DEBUG	"[ SHT_SUNW_DEBUG ]"
#define	MSG_SHT_SUNW_DEBUG_SIZE	18

#define	MSG_SHT_SUNW_DEBUG_ALT	"DBG "
#define	MSG_SHT_SUNW_DEBUG_ALT_SIZE	4

#define	MSG_SHT_SUNW_move	"[ SHT_SUNW_move ]"
#define	MSG_SHT_SUNW_move_SIZE	17

#define	MSG_SHT_SUNW_move_ALT	"MOVE"
#define	MSG_SHT_SUNW_move_ALT_SIZE	4

#define	MSG_SHT_SUNW_COMDAT	"[ SHT_SUNW_COMDAT ]"
#define	MSG_SHT_SUNW_COMDAT_SIZE	19

#define	MSG_SHT_SUNW_COMDAT_ALT	"COMD"
#define	MSG_SHT_SUNW_COMDAT_ALT_SIZE	4

#define	MSG_SHT_SUNW_syminfo	"[ SHT_SUNW_syminfo ]"
#define	MSG_SHT_SUNW_syminfo_SIZE	20

#define	MSG_SHT_SUNW_syminfo_ALT	"SYMI"
#define	MSG_SHT_SUNW_syminfo_ALT_SIZE	4

#define	MSG_SHT_SUNW_verdef	"[ SHT_SUNW_verdef ]"
#define	MSG_SHT_SUNW_verdef_SIZE	19

#define	MSG_SHT_SUNW_verdef_ALT	"VERD"
#define	MSG_SHT_SUNW_verdef_ALT_SIZE	4

#define	MSG_SHT_SUNW_verneed	"[ SHT_SUNW_verneed ]"
#define	MSG_SHT_SUNW_verneed_SIZE	20

#define	MSG_SHT_SUNW_verneed_ALT	"VERN"
#define	MSG_SHT_SUNW_verneed_ALT_SIZE	4

#define	MSG_SHT_SUNW_versym	"[ SHT_SUNW_versym ]"
#define	MSG_SHT_SUNW_versym_SIZE	19

#define	MSG_SHT_SUNW_versym_ALT	"VERS"
#define	MSG_SHT_SUNW_versym_ALT_SIZE	4

#define	MSG_SHT_AMD64_UNWIND	"[ SHT_AMD64_UNWIND ]"
#define	MSG_SHT_AMD64_UNWIND_SIZE	20

#define	MSG_SHT_AMD64_UNWIND_ALT	"UNWD"
#define	MSG_SHT_AMD64_UNWIND_ALT_SIZE	4

#define	MSG_SHT_SPARC_GOTDATA	"[ SHT_SPARC_GOTDATA ]"
#define	MSG_SHT_SPARC_GOTDATA_SIZE	21

#define	MSG_SHT_SPARC_GOTDATA_ALT	"GOTD"
#define	MSG_SHT_SPARC_GOTDATA_ALT_SIZE	4

#define	MSG_SHN_AFTER	"[ SHN_AFTER ]"
#define	MSG_SHN_AFTER_SIZE	13

#define	MSG_SHN_BEFORE	"[ SHN_BEFORE ]"
#define	MSG_SHN_BEFORE_SIZE	14

#define	MSG_SHF_WRITE	"SHF_WRITE"
#define	MSG_SHF_WRITE_SIZE	9

#define	MSG_SHF_ALLOC	"SHF_ALLOC"
#define	MSG_SHF_ALLOC_SIZE	9

#define	MSG_SHF_EXECINSTR	"SHF_EXECINSTR"
#define	MSG_SHF_EXECINSTR_SIZE	13

#define	MSG_SHF_MERGE	"SHF_MERGE"
#define	MSG_SHF_MERGE_SIZE	9

#define	MSG_SHF_STRINGS	"SHF_STRINGS"
#define	MSG_SHF_STRINGS_SIZE	11

#define	MSG_SHF_INFO_LINK	"SHF_INFO_LINK"
#define	MSG_SHF_INFO_LINK_SIZE	13

#define	MSG_SHF_LINK_ORDER	"SHF_LINK_ORDER"
#define	MSG_SHF_LINK_ORDER_SIZE	14

#define	MSG_SHF_OS_NONCONFORMING	"SHF_OS_NONCONFORMING"
#define	MSG_SHF_OS_NONCONFORMING_SIZE	20

#define	MSG_SHF_GROUP	"SHF_GROUP"
#define	MSG_SHF_GROUP_SIZE	9

#define	MSG_SHF_TLS	"SHF_TLS"
#define	MSG_SHF_TLS_SIZE	7

#define	MSG_SHF_EXCLUDE	"SHF_EXCLUDE"
#define	MSG_SHF_EXCLUDE_SIZE	11

#define	MSG_SHF_ORDERED	"SHF_ORDERED"
#define	MSG_SHF_ORDERED_SIZE	11

#define	MSG_SHF_AMD64_LARGE	"SHF_AMD64_LARGE"
#define	MSG_SHF_AMD64_LARGE_SIZE	15

#define	MSG_GBL_ZERO	"0"
#define	MSG_GBL_ZERO_SIZE	1

#endif	/* __lint */

#endif

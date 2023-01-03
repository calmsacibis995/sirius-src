#ifndef	_SYMBOLS_MSG_DOT_H
#define	_SYMBOLS_MSG_DOT_H

#ifndef	__lint

typedef int	Msg;

#define	MSG_ORIG(x)	&__sgs_msg[x]

extern	const char *	_sgs_msg(Msg);

#define	MSG_INTL(x)	_sgs_msg(x)


#define	MSG_STT_NOTYPE	1
#define	MSG_STT_NOTYPE_SIZE	4

#define	MSG_STT_NOTYPE_ALT	6
#define	MSG_STT_NOTYPE_ALT_SIZE	10

#define	MSG_STT_OBJECT	17
#define	MSG_STT_OBJECT_SIZE	4

#define	MSG_STT_OBJECT_ALT	22
#define	MSG_STT_OBJECT_ALT_SIZE	10

#define	MSG_STT_FUNC	37
#define	MSG_STT_FUNC_SIZE	4

#define	MSG_STT_FUNC_ALT	33
#define	MSG_STT_FUNC_ALT_SIZE	8

#define	MSG_STT_SECTION	42
#define	MSG_STT_SECTION_SIZE	4

#define	MSG_STT_SECTION_ALT	47
#define	MSG_STT_SECTION_ALT_SIZE	11

#define	MSG_STT_FILE	63
#define	MSG_STT_FILE_SIZE	4

#define	MSG_STT_FILE_ALT	59
#define	MSG_STT_FILE_ALT_SIZE	8

#define	MSG_STT_COMMON	68
#define	MSG_STT_COMMON_SIZE	4

#define	MSG_STT_COMMON_ALT	73
#define	MSG_STT_COMMON_ALT_SIZE	10

#define	MSG_STT_TLS	84
#define	MSG_STT_TLS_SIZE	4

#define	MSG_STT_TLS_ALT	89
#define	MSG_STT_TLS_ALT_SIZE	7

#define	MSG_STT_SPARC_REGISTER	97
#define	MSG_STT_SPARC_REGISTER_SIZE	4

#define	MSG_STT_SPARC_REGISTER_ALT	102
#define	MSG_STT_SPARC_REGISTER_ALT_SIZE	18

#define	MSG_STB_LOCAL	121
#define	MSG_STB_LOCAL_SIZE	4

#define	MSG_STB_LOCAL_ALT	126
#define	MSG_STB_LOCAL_ALT_SIZE	9

#define	MSG_STB_GLOBAL	136
#define	MSG_STB_GLOBAL_SIZE	4

#define	MSG_STB_GLOBAL_ALT	141
#define	MSG_STB_GLOBAL_ALT_SIZE	10

#define	MSG_STB_WEAK	156
#define	MSG_STB_WEAK_SIZE	4

#define	MSG_STB_WEAK_ALT	152
#define	MSG_STB_WEAK_ALT_SIZE	8

#define	MSG_STV_DEFAULT	172
#define	MSG_STV_DEFAULT_SIZE	1

#define	MSG_STV_DEFAULT_ALT	174
#define	MSG_STV_DEFAULT_ALT_SIZE	11

#define	MSG_STV_INTERNAL	100
#define	MSG_STV_INTERNAL_SIZE	1

#define	MSG_STV_INTERNAL_ALT	186
#define	MSG_STV_INTERNAL_ALT_SIZE	12

#define	MSG_STV_HIDDEN	199
#define	MSG_STV_HIDDEN_SIZE	1

#define	MSG_STV_HIDDEN_ALT	201
#define	MSG_STV_HIDDEN_ALT_SIZE	10

#define	MSG_STV_PROTECTED	212
#define	MSG_STV_PROTECTED_SIZE	1

#define	MSG_STV_PROTECTED_ALT	214
#define	MSG_STV_PROTECTED_ALT_SIZE	13

#define	MSG_STV_EXPORTED	233
#define	MSG_STV_EXPORTED_SIZE	1

#define	MSG_STV_EXPORTED_ALT	161
#define	MSG_STV_EXPORTED_ALT_SIZE	12

#define	MSG_STV_SINGLETON	237
#define	MSG_STV_SINGLETON_SIZE	1

#define	MSG_STV_SINGLETON_ALT	239
#define	MSG_STV_SINGLETON_ALT_SIZE	13

#define	MSG_STV_ELIMINATE	66
#define	MSG_STV_ELIMINATE_SIZE	1

#define	MSG_STV_ELIMINATE_ALT	253
#define	MSG_STV_ELIMINATE_ALT_SIZE	13

#define	MSG_SHN_UNDEF	267
#define	MSG_SHN_UNDEF_SIZE	5

#define	MSG_SHN_SUNW_IGNORE	273
#define	MSG_SHN_SUNW_IGNORE_SIZE	6

#define	MSG_SHN_ABS	235
#define	MSG_SHN_ABS_SIZE	3

#define	MSG_SHN_COMMON	281
#define	MSG_SHN_COMMON_SIZE	6

#define	MSG_SHN_AMD64_LCOMMON	280
#define	MSG_SHN_AMD64_LCOMMON_SIZE	7

#define	MSG_SHN_AFTER	288
#define	MSG_SHN_AFTER_SIZE	5

#define	MSG_SHN_BEFORE	294
#define	MSG_SHN_BEFORE_SIZE	6

#define	MSG_SHN_XINDEX	228
#define	MSG_SHN_XINDEX_SIZE	6

#define	MSG_SYM_FMT_VAL_32	301
#define	MSG_SYM_FMT_VAL_32_SIZE	9

#define	MSG_SYM_FMT_VAL_64	311
#define	MSG_SYM_FMT_VAL_64_SIZE	11

static const char __sgs_msg[323] = { 
/*    0 */ 0x00,  0x4e,  0x4f,  0x54,  0x59,  0x00,  0x53,  0x54,  0x54,  0x5f,
/*   10 */ 0x4e,  0x4f,  0x54,  0x59,  0x50,  0x45,  0x00,  0x4f,  0x42,  0x4a,
/*   20 */ 0x54,  0x00,  0x53,  0x54,  0x54,  0x5f,  0x4f,  0x42,  0x4a,  0x45,
/*   30 */ 0x43,  0x54,  0x00,  0x53,  0x54,  0x54,  0x5f,  0x46,  0x55,  0x4e,
/*   40 */ 0x43,  0x00,  0x53,  0x45,  0x43,  0x54,  0x00,  0x53,  0x54,  0x54,
/*   50 */ 0x5f,  0x53,  0x45,  0x43,  0x54,  0x49,  0x4f,  0x4e,  0x00,  0x53,
/*   60 */ 0x54,  0x54,  0x5f,  0x46,  0x49,  0x4c,  0x45,  0x00,  0x43,  0x4f,
/*   70 */ 0x4d,  0x4d,  0x00,  0x53,  0x54,  0x54,  0x5f,  0x43,  0x4f,  0x4d,
/*   80 */ 0x4d,  0x4f,  0x4e,  0x00,  0x54,  0x4c,  0x53,  0x20,  0x00,  0x53,
/*   90 */ 0x54,  0x54,  0x5f,  0x54,  0x4c,  0x53,  0x00,  0x52,  0x45,  0x47,
/*  100 */ 0x49,  0x00,  0x53,  0x54,  0x54,  0x5f,  0x53,  0x50,  0x41,  0x52,
/*  110 */ 0x43,  0x5f,  0x52,  0x45,  0x47,  0x49,  0x53,  0x54,  0x45,  0x52,
/*  120 */ 0x00,  0x4c,  0x4f,  0x43,  0x4c,  0x00,  0x53,  0x54,  0x42,  0x5f,
/*  130 */ 0x4c,  0x4f,  0x43,  0x41,  0x4c,  0x00,  0x47,  0x4c,  0x4f,  0x42,
/*  140 */ 0x00,  0x53,  0x54,  0x42,  0x5f,  0x47,  0x4c,  0x4f,  0x42,  0x41,
/*  150 */ 0x4c,  0x00,  0x53,  0x54,  0x42,  0x5f,  0x57,  0x45,  0x41,  0x4b,
/*  160 */ 0x00,  0x53,  0x54,  0x56,  0x5f,  0x45,  0x58,  0x50,  0x4f,  0x52,
/*  170 */ 0x54,  0x45,  0x44,  0x00,  0x53,  0x54,  0x56,  0x5f,  0x44,  0x45,
/*  180 */ 0x46,  0x41,  0x55,  0x4c,  0x54,  0x00,  0x53,  0x54,  0x56,  0x5f,
/*  190 */ 0x49,  0x4e,  0x54,  0x45,  0x52,  0x4e,  0x41,  0x4c,  0x00,  0x48,
/*  200 */ 0x00,  0x53,  0x54,  0x56,  0x5f,  0x48,  0x49,  0x44,  0x44,  0x45,
/*  210 */ 0x4e,  0x00,  0x50,  0x00,  0x53,  0x54,  0x56,  0x5f,  0x50,  0x52,
/*  220 */ 0x4f,  0x54,  0x45,  0x43,  0x54,  0x45,  0x44,  0x00,  0x58,  0x49,
/*  230 */ 0x4e,  0x44,  0x45,  0x58,  0x00,  0x41,  0x42,  0x53,  0x00,  0x53,
/*  240 */ 0x54,  0x56,  0x5f,  0x53,  0x49,  0x4e,  0x47,  0x4c,  0x45,  0x54,
/*  250 */ 0x4f,  0x4e,  0x00,  0x53,  0x54,  0x56,  0x5f,  0x45,  0x4c,  0x49,
/*  260 */ 0x4d,  0x49,  0x4e,  0x41,  0x54,  0x45,  0x00,  0x55,  0x4e,  0x44,
/*  270 */ 0x45,  0x46,  0x00,  0x49,  0x47,  0x4e,  0x4f,  0x52,  0x45,  0x00,
/*  280 */ 0x4c,  0x43,  0x4f,  0x4d,  0x4d,  0x4f,  0x4e,  0x00,  0x41,  0x46,
/*  290 */ 0x54,  0x45,  0x52,  0x00,  0x42,  0x45,  0x46,  0x4f,  0x52,  0x45,
/*  300 */ 0x00,  0x30,  0x78,  0x25,  0x38,  0x2e,  0x38,  0x6c,  0x6c,  0x78,
/*  310 */ 0x00,  0x30,  0x78,  0x25,  0x31,  0x36,  0x2e,  0x31,  0x36,  0x6c,
/*  320 */ 0x6c,  0x78,  0x00 };

#else	/* __lint */


typedef char *	Msg;

extern	const char *	_sgs_msg(Msg);

#define MSG_ORIG(x)	x
#define MSG_INTL(x)	x

#define	MSG_STT_NOTYPE	"NOTY"
#define	MSG_STT_NOTYPE_SIZE	4

#define	MSG_STT_NOTYPE_ALT	"STT_NOTYPE"
#define	MSG_STT_NOTYPE_ALT_SIZE	10

#define	MSG_STT_OBJECT	"OBJT"
#define	MSG_STT_OBJECT_SIZE	4

#define	MSG_STT_OBJECT_ALT	"STT_OBJECT"
#define	MSG_STT_OBJECT_ALT_SIZE	10

#define	MSG_STT_FUNC	"FUNC"
#define	MSG_STT_FUNC_SIZE	4

#define	MSG_STT_FUNC_ALT	"STT_FUNC"
#define	MSG_STT_FUNC_ALT_SIZE	8

#define	MSG_STT_SECTION	"SECT"
#define	MSG_STT_SECTION_SIZE	4

#define	MSG_STT_SECTION_ALT	"STT_SECTION"
#define	MSG_STT_SECTION_ALT_SIZE	11

#define	MSG_STT_FILE	"FILE"
#define	MSG_STT_FILE_SIZE	4

#define	MSG_STT_FILE_ALT	"STT_FILE"
#define	MSG_STT_FILE_ALT_SIZE	8

#define	MSG_STT_COMMON	"COMM"
#define	MSG_STT_COMMON_SIZE	4

#define	MSG_STT_COMMON_ALT	"STT_COMMON"
#define	MSG_STT_COMMON_ALT_SIZE	10

#define	MSG_STT_TLS	"TLS "
#define	MSG_STT_TLS_SIZE	4

#define	MSG_STT_TLS_ALT	"STT_TLS"
#define	MSG_STT_TLS_ALT_SIZE	7

#define	MSG_STT_SPARC_REGISTER	"REGI"
#define	MSG_STT_SPARC_REGISTER_SIZE	4

#define	MSG_STT_SPARC_REGISTER_ALT	"STT_SPARC_REGISTER"
#define	MSG_STT_SPARC_REGISTER_ALT_SIZE	18

#define	MSG_STB_LOCAL	"LOCL"
#define	MSG_STB_LOCAL_SIZE	4

#define	MSG_STB_LOCAL_ALT	"STB_LOCAL"
#define	MSG_STB_LOCAL_ALT_SIZE	9

#define	MSG_STB_GLOBAL	"GLOB"
#define	MSG_STB_GLOBAL_SIZE	4

#define	MSG_STB_GLOBAL_ALT	"STB_GLOBAL"
#define	MSG_STB_GLOBAL_ALT_SIZE	10

#define	MSG_STB_WEAK	"WEAK"
#define	MSG_STB_WEAK_SIZE	4

#define	MSG_STB_WEAK_ALT	"STB_WEAK"
#define	MSG_STB_WEAK_ALT_SIZE	8

#define	MSG_STV_DEFAULT	"D"
#define	MSG_STV_DEFAULT_SIZE	1

#define	MSG_STV_DEFAULT_ALT	"STV_DEFAULT"
#define	MSG_STV_DEFAULT_ALT_SIZE	11

#define	MSG_STV_INTERNAL	"I"
#define	MSG_STV_INTERNAL_SIZE	1

#define	MSG_STV_INTERNAL_ALT	"STV_INTERNAL"
#define	MSG_STV_INTERNAL_ALT_SIZE	12

#define	MSG_STV_HIDDEN	"H"
#define	MSG_STV_HIDDEN_SIZE	1

#define	MSG_STV_HIDDEN_ALT	"STV_HIDDEN"
#define	MSG_STV_HIDDEN_ALT_SIZE	10

#define	MSG_STV_PROTECTED	"P"
#define	MSG_STV_PROTECTED_SIZE	1

#define	MSG_STV_PROTECTED_ALT	"STV_PROTECTED"
#define	MSG_STV_PROTECTED_ALT_SIZE	13

#define	MSG_STV_EXPORTED	"X"
#define	MSG_STV_EXPORTED_SIZE	1

#define	MSG_STV_EXPORTED_ALT	"STV_EXPORTED"
#define	MSG_STV_EXPORTED_ALT_SIZE	12

#define	MSG_STV_SINGLETON	"S"
#define	MSG_STV_SINGLETON_SIZE	1

#define	MSG_STV_SINGLETON_ALT	"STV_SINGLETON"
#define	MSG_STV_SINGLETON_ALT_SIZE	13

#define	MSG_STV_ELIMINATE	"E"
#define	MSG_STV_ELIMINATE_SIZE	1

#define	MSG_STV_ELIMINATE_ALT	"STV_ELIMINATE"
#define	MSG_STV_ELIMINATE_ALT_SIZE	13

#define	MSG_SHN_UNDEF	"UNDEF"
#define	MSG_SHN_UNDEF_SIZE	5

#define	MSG_SHN_SUNW_IGNORE	"IGNORE"
#define	MSG_SHN_SUNW_IGNORE_SIZE	6

#define	MSG_SHN_ABS	"ABS"
#define	MSG_SHN_ABS_SIZE	3

#define	MSG_SHN_COMMON	"COMMON"
#define	MSG_SHN_COMMON_SIZE	6

#define	MSG_SHN_AMD64_LCOMMON	"LCOMMON"
#define	MSG_SHN_AMD64_LCOMMON_SIZE	7

#define	MSG_SHN_AFTER	"AFTER"
#define	MSG_SHN_AFTER_SIZE	5

#define	MSG_SHN_BEFORE	"BEFORE"
#define	MSG_SHN_BEFORE_SIZE	6

#define	MSG_SHN_XINDEX	"XINDEX"
#define	MSG_SHN_XINDEX_SIZE	6

#define	MSG_SYM_FMT_VAL_32	"0x%8.8llx"
#define	MSG_SYM_FMT_VAL_32_SIZE	9

#define	MSG_SYM_FMT_VAL_64	"0x%16.16llx"
#define	MSG_SYM_FMT_VAL_64_SIZE	11

#endif	/* __lint */

#endif

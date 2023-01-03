#ifndef	_DL_MSG_DOT_H
#define	_DL_MSG_DOT_H

#ifndef	__lint

typedef int	Msg;

#define	MSG_ORIG(x)	&__sgs_msg[x]

extern	const char *	_sgs_msg(Msg);

#define	MSG_INTL(x)	_sgs_msg(x)


#define	MSG_RTLD_LAZY	1
#define	MSG_RTLD_LAZY_SIZE	9

#define	MSG_RTLD_NOW	11
#define	MSG_RTLD_NOW_SIZE	8

#define	MSG_RTLD_GLOBAL	20
#define	MSG_RTLD_GLOBAL_SIZE	11

#define	MSG_RTLD_LOCAL	32
#define	MSG_RTLD_LOCAL_SIZE	10

#define	MSG_RTLD_PARENT	43
#define	MSG_RTLD_PARENT_SIZE	11

#define	MSG_RTLD_GROUP	55
#define	MSG_RTLD_GROUP_SIZE	10

#define	MSG_RTLD_WORLD	66
#define	MSG_RTLD_WORLD_SIZE	10

#define	MSG_RTLD_NODELETE	77
#define	MSG_RTLD_NODELETE_SIZE	13

#define	MSG_RTLD_NOLOAD	91
#define	MSG_RTLD_NOLOAD_SIZE	11

#define	MSG_RTLD_FIRST	103
#define	MSG_RTLD_FIRST_SIZE	10

#define	MSG_RTLD_CONFGEN	114
#define	MSG_RTLD_CONFGEN_SIZE	12

#define	MSG_RTLD_REL_RELATIVE	127
#define	MSG_RTLD_REL_RELATIVE_SIZE	17

#define	MSG_RTLD_REL_EXEC	145
#define	MSG_RTLD_REL_EXEC_SIZE	13

#define	MSG_RTLD_REL_DEPENDS	159
#define	MSG_RTLD_REL_DEPENDS_SIZE	16

#define	MSG_RTLD_REL_PRELOAD	176
#define	MSG_RTLD_REL_PRELOAD_SIZE	16

#define	MSG_RTLD_REL_SELF	193
#define	MSG_RTLD_REL_SELF_SIZE	13

#define	MSG_RTLD_REL_WEAK	207
#define	MSG_RTLD_REL_WEAK_SIZE	13

#define	MSG_RTLD_REL_ALL	221
#define	MSG_RTLD_REL_ALL_SIZE	12

#define	MSG_RTLD_MEMORY	234
#define	MSG_RTLD_MEMORY_SIZE	11

#define	MSG_RTLD_STRIP	246
#define	MSG_RTLD_STRIP_SIZE	10

#define	MSG_RTLD_NOHEAP	257
#define	MSG_RTLD_NOHEAP_SIZE	11

#define	MSG_RTLD_CONFSET	269
#define	MSG_RTLD_CONFSET_SIZE	12

#define	MSG_GBL_SEP	282
#define	MSG_GBL_SEP_SIZE	3

#define	MSG_GBL_QUOTE	286
#define	MSG_GBL_QUOTE_SIZE	1

#define	MSG_GBL_ZERO	288
#define	MSG_GBL_ZERO_SIZE	1

static const char __sgs_msg[290] = { 
/*    0 */ 0x00,  0x52,  0x54,  0x4c,  0x44,  0x5f,  0x4c,  0x41,  0x5a,  0x59,
/*   10 */ 0x00,  0x52,  0x54,  0x4c,  0x44,  0x5f,  0x4e,  0x4f,  0x57,  0x00,
/*   20 */ 0x52,  0x54,  0x4c,  0x44,  0x5f,  0x47,  0x4c,  0x4f,  0x42,  0x41,
/*   30 */ 0x4c,  0x00,  0x52,  0x54,  0x4c,  0x44,  0x5f,  0x4c,  0x4f,  0x43,
/*   40 */ 0x41,  0x4c,  0x00,  0x52,  0x54,  0x4c,  0x44,  0x5f,  0x50,  0x41,
/*   50 */ 0x52,  0x45,  0x4e,  0x54,  0x00,  0x52,  0x54,  0x4c,  0x44,  0x5f,
/*   60 */ 0x47,  0x52,  0x4f,  0x55,  0x50,  0x00,  0x52,  0x54,  0x4c,  0x44,
/*   70 */ 0x5f,  0x57,  0x4f,  0x52,  0x4c,  0x44,  0x00,  0x52,  0x54,  0x4c,
/*   80 */ 0x44,  0x5f,  0x4e,  0x4f,  0x44,  0x45,  0x4c,  0x45,  0x54,  0x45,
/*   90 */ 0x00,  0x52,  0x54,  0x4c,  0x44,  0x5f,  0x4e,  0x4f,  0x4c,  0x4f,
/*  100 */ 0x41,  0x44,  0x00,  0x52,  0x54,  0x4c,  0x44,  0x5f,  0x46,  0x49,
/*  110 */ 0x52,  0x53,  0x54,  0x00,  0x52,  0x54,  0x4c,  0x44,  0x5f,  0x43,
/*  120 */ 0x4f,  0x4e,  0x46,  0x47,  0x45,  0x4e,  0x00,  0x52,  0x54,  0x4c,
/*  130 */ 0x44,  0x5f,  0x52,  0x45,  0x4c,  0x5f,  0x52,  0x45,  0x4c,  0x41,
/*  140 */ 0x54,  0x49,  0x56,  0x45,  0x00,  0x52,  0x54,  0x4c,  0x44,  0x5f,
/*  150 */ 0x52,  0x45,  0x4c,  0x5f,  0x45,  0x58,  0x45,  0x43,  0x00,  0x52,
/*  160 */ 0x54,  0x4c,  0x44,  0x5f,  0x52,  0x45,  0x4c,  0x5f,  0x44,  0x45,
/*  170 */ 0x50,  0x45,  0x4e,  0x44,  0x53,  0x00,  0x52,  0x54,  0x4c,  0x44,
/*  180 */ 0x5f,  0x52,  0x45,  0x4c,  0x5f,  0x50,  0x52,  0x45,  0x4c,  0x4f,
/*  190 */ 0x41,  0x44,  0x00,  0x52,  0x54,  0x4c,  0x44,  0x5f,  0x52,  0x45,
/*  200 */ 0x4c,  0x5f,  0x53,  0x45,  0x4c,  0x46,  0x00,  0x52,  0x54,  0x4c,
/*  210 */ 0x44,  0x5f,  0x52,  0x45,  0x4c,  0x5f,  0x57,  0x45,  0x41,  0x4b,
/*  220 */ 0x00,  0x52,  0x54,  0x4c,  0x44,  0x5f,  0x52,  0x45,  0x4c,  0x5f,
/*  230 */ 0x41,  0x4c,  0x4c,  0x00,  0x52,  0x54,  0x4c,  0x44,  0x5f,  0x4d,
/*  240 */ 0x45,  0x4d,  0x4f,  0x52,  0x59,  0x00,  0x52,  0x54,  0x4c,  0x44,
/*  250 */ 0x5f,  0x53,  0x54,  0x52,  0x49,  0x50,  0x00,  0x52,  0x54,  0x4c,
/*  260 */ 0x44,  0x5f,  0x4e,  0x4f,  0x48,  0x45,  0x41,  0x50,  0x00,  0x52,
/*  270 */ 0x54,  0x4c,  0x44,  0x5f,  0x43,  0x4f,  0x4e,  0x46,  0x53,  0x45,
/*  280 */ 0x54,  0x00,  0x20,  0x7c,  0x20,  0x00,  0x22,  0x00,  0x30,  0x00 };

#else	/* __lint */


typedef char *	Msg;

extern	const char *	_sgs_msg(Msg);

#define MSG_ORIG(x)	x
#define MSG_INTL(x)	x

#define	MSG_RTLD_LAZY	"RTLD_LAZY"
#define	MSG_RTLD_LAZY_SIZE	9

#define	MSG_RTLD_NOW	"RTLD_NOW"
#define	MSG_RTLD_NOW_SIZE	8

#define	MSG_RTLD_GLOBAL	"RTLD_GLOBAL"
#define	MSG_RTLD_GLOBAL_SIZE	11

#define	MSG_RTLD_LOCAL	"RTLD_LOCAL"
#define	MSG_RTLD_LOCAL_SIZE	10

#define	MSG_RTLD_PARENT	"RTLD_PARENT"
#define	MSG_RTLD_PARENT_SIZE	11

#define	MSG_RTLD_GROUP	"RTLD_GROUP"
#define	MSG_RTLD_GROUP_SIZE	10

#define	MSG_RTLD_WORLD	"RTLD_WORLD"
#define	MSG_RTLD_WORLD_SIZE	10

#define	MSG_RTLD_NODELETE	"RTLD_NODELETE"
#define	MSG_RTLD_NODELETE_SIZE	13

#define	MSG_RTLD_NOLOAD	"RTLD_NOLOAD"
#define	MSG_RTLD_NOLOAD_SIZE	11

#define	MSG_RTLD_FIRST	"RTLD_FIRST"
#define	MSG_RTLD_FIRST_SIZE	10

#define	MSG_RTLD_CONFGEN	"RTLD_CONFGEN"
#define	MSG_RTLD_CONFGEN_SIZE	12

#define	MSG_RTLD_REL_RELATIVE	"RTLD_REL_RELATIVE"
#define	MSG_RTLD_REL_RELATIVE_SIZE	17

#define	MSG_RTLD_REL_EXEC	"RTLD_REL_EXEC"
#define	MSG_RTLD_REL_EXEC_SIZE	13

#define	MSG_RTLD_REL_DEPENDS	"RTLD_REL_DEPENDS"
#define	MSG_RTLD_REL_DEPENDS_SIZE	16

#define	MSG_RTLD_REL_PRELOAD	"RTLD_REL_PRELOAD"
#define	MSG_RTLD_REL_PRELOAD_SIZE	16

#define	MSG_RTLD_REL_SELF	"RTLD_REL_SELF"
#define	MSG_RTLD_REL_SELF_SIZE	13

#define	MSG_RTLD_REL_WEAK	"RTLD_REL_WEAK"
#define	MSG_RTLD_REL_WEAK_SIZE	13

#define	MSG_RTLD_REL_ALL	"RTLD_REL_ALL"
#define	MSG_RTLD_REL_ALL_SIZE	12

#define	MSG_RTLD_MEMORY	"RTLD_MEMORY"
#define	MSG_RTLD_MEMORY_SIZE	11

#define	MSG_RTLD_STRIP	"RTLD_STRIP"
#define	MSG_RTLD_STRIP_SIZE	10

#define	MSG_RTLD_NOHEAP	"RTLD_NOHEAP"
#define	MSG_RTLD_NOHEAP_SIZE	11

#define	MSG_RTLD_CONFSET	"RTLD_CONFSET"
#define	MSG_RTLD_CONFSET_SIZE	12

#define	MSG_GBL_SEP	" | "
#define	MSG_GBL_SEP_SIZE	3

#define	MSG_GBL_QUOTE	"\""
#define	MSG_GBL_QUOTE_SIZE	1

#define	MSG_GBL_ZERO	"0"
#define	MSG_GBL_ZERO_SIZE	1

#endif	/* __lint */

#endif

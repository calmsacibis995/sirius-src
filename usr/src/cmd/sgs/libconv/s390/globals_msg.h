#ifndef	_GLOBALS_MSG_DOT_H
#define	_GLOBALS_MSG_DOT_H

#ifndef	__lint

typedef int	Msg;

#define	MSG_ORIG(x)	&__sgs_msg[x]

extern	const char *	_sgs_msg(Msg);

#define	MSG_INTL(x)	_sgs_msg(x)


#define	MSG_GBL_FMT_DEC_32	1
#define	MSG_GBL_FMT_DEC_32_SIZE	3

#define	MSG_GBL_FMT_DEC_64	5
#define	MSG_GBL_FMT_DEC_64_SIZE	4

#define	MSG_GBL_FMT_DECS_32	10
#define	MSG_GBL_FMT_DECS_32_SIZE	5

#define	MSG_GBL_FMT_DECS_64	16
#define	MSG_GBL_FMT_DECS_64_SIZE	6

#define	MSG_GBL_FMT_HEX_32	23
#define	MSG_GBL_FMT_HEX_32_SIZE	5

#define	MSG_GBL_FMT_HEX_64	29
#define	MSG_GBL_FMT_HEX_64_SIZE	6

#define	MSG_GBL_FMT_HEXS_32	36
#define	MSG_GBL_FMT_HEXS_32_SIZE	7

#define	MSG_GBL_FMT_HEXS_64	44
#define	MSG_GBL_FMT_HEXS_64_SIZE	8

#define	MSG_GBL_OSQBRKT	53
#define	MSG_GBL_OSQBRKT_SIZE	2

#define	MSG_GBL_CSQBRKT	56
#define	MSG_GBL_CSQBRKT_SIZE	2

#define	MSG_GBL_SEP	54
#define	MSG_GBL_SEP_SIZE	1

static const char __sgs_msg[59] = { 
/*    0 */ 0x00,  0x25,  0x6c,  0x64,  0x00,  0x25,  0x6c,  0x6c,  0x64,  0x00,
/*   10 */ 0x20,  0x25,  0x6c,  0x64,  0x20,  0x00,  0x20,  0x25,  0x6c,  0x6c,
/*   20 */ 0x64,  0x20,  0x00,  0x30,  0x78,  0x25,  0x6c,  0x78,  0x00,  0x30,
/*   30 */ 0x78,  0x25,  0x6c,  0x6c,  0x78,  0x00,  0x20,  0x30,  0x78,  0x25,
/*   40 */ 0x6c,  0x78,  0x20,  0x00,  0x20,  0x30,  0x78,  0x25,  0x6c,  0x6c,
/*   50 */ 0x78,  0x20,  0x00,  0x5b,  0x20,  0x00,  0x20,  0x5d,  0x00 };

#else	/* __lint */


typedef char *	Msg;

extern	const char *	_sgs_msg(Msg);

#define MSG_ORIG(x)	x
#define MSG_INTL(x)	x

#define	MSG_GBL_FMT_DEC_32	"%ld"
#define	MSG_GBL_FMT_DEC_32_SIZE	3

#define	MSG_GBL_FMT_DEC_64	"%lld"
#define	MSG_GBL_FMT_DEC_64_SIZE	4

#define	MSG_GBL_FMT_DECS_32	" %ld "
#define	MSG_GBL_FMT_DECS_32_SIZE	5

#define	MSG_GBL_FMT_DECS_64	" %lld "
#define	MSG_GBL_FMT_DECS_64_SIZE	6

#define	MSG_GBL_FMT_HEX_32	"0x%lx"
#define	MSG_GBL_FMT_HEX_32_SIZE	5

#define	MSG_GBL_FMT_HEX_64	"0x%llx"
#define	MSG_GBL_FMT_HEX_64_SIZE	6

#define	MSG_GBL_FMT_HEXS_32	" 0x%lx "
#define	MSG_GBL_FMT_HEXS_32_SIZE	7

#define	MSG_GBL_FMT_HEXS_64	" 0x%llx "
#define	MSG_GBL_FMT_HEXS_64_SIZE	8

#define	MSG_GBL_OSQBRKT	"[ "
#define	MSG_GBL_OSQBRKT_SIZE	2

#define	MSG_GBL_CSQBRKT	" ]"
#define	MSG_GBL_CSQBRKT_SIZE	2

#define	MSG_GBL_SEP	" "
#define	MSG_GBL_SEP_SIZE	1

#endif	/* __lint */

#endif

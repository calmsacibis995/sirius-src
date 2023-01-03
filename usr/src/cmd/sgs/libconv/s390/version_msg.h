#ifndef	_VERSION_MSG_DOT_H
#define	_VERSION_MSG_DOT_H

#ifndef	__lint

typedef int	Msg;

#define	MSG_ORIG(x)	&__sgs_msg[x]

extern	const char *	_sgs_msg(Msg);

#define	MSG_INTL(x)	_sgs_msg(x)


#define	MSG_VER_FLG_WEAK	1
#define	MSG_VER_FLG_WEAK_SIZE	8

#define	MSG_VER_FLG_BASE	10
#define	MSG_VER_FLG_BASE_SIZE	8

#define	MSG_VERSYM_ELIMINATE	19
#define	MSG_VERSYM_ELIMINATE_SIZE	4

#define	MSG_GBL_NULL	0
#define	MSG_GBL_NULL_SIZE	0

#define	MSG_VERSYM_FMT	24
#define	MSG_VERSYM_FMT_SIZE	2

#define	MSG_VERSYM_GNUH_FMT	27
#define	MSG_VERSYM_GNUH_FMT_SIZE	3

static const char __sgs_msg[31] = { 
/*    0 */ 0x00,  0x5b,  0x20,  0x57,  0x45,  0x41,  0x4b,  0x20,  0x5d,  0x00,
/*   10 */ 0x5b,  0x20,  0x42,  0x41,  0x53,  0x45,  0x20,  0x5d,  0x00,  0x45,
/*   20 */ 0x4c,  0x49,  0x4d,  0x00,  0x25,  0x64,  0x00,  0x25,  0x64,  0x48,
	0x00 };

#else	/* __lint */


typedef char *	Msg;

extern	const char *	_sgs_msg(Msg);

#define MSG_ORIG(x)	x
#define MSG_INTL(x)	x

#define	MSG_VER_FLG_WEAK	"[ WEAK ]"
#define	MSG_VER_FLG_WEAK_SIZE	8

#define	MSG_VER_FLG_BASE	"[ BASE ]"
#define	MSG_VER_FLG_BASE_SIZE	8

#define	MSG_VERSYM_ELIMINATE	"ELIM"
#define	MSG_VERSYM_ELIMINATE_SIZE	4

#define	MSG_GBL_NULL	""
#define	MSG_GBL_NULL_SIZE	0

#define	MSG_VERSYM_FMT	"%d"
#define	MSG_VERSYM_FMT_SIZE	2

#define	MSG_VERSYM_GNUH_FMT	"%dH"
#define	MSG_VERSYM_GNUH_FMT_SIZE	3

#endif	/* __lint */

#endif

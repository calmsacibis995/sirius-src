#ifndef	_LDDSTUB_MSG_DOT_H
#define	_LDDSTUB_MSG_DOT_H

#ifndef	__lint

typedef int	Msg;

#define	MSG_ORIG(x)	&__sgs_msg[x]

extern	const char *	_sgs_msg(Msg);

#define	MSG_INTL(x)	_sgs_msg(x)


#define	MSG_ORG_LDDSTUB	1
#define	MSG_ORG_LDDSTUB_SIZE	15

#define	MSG_ORG_LDDSTUB_64	17
#define	MSG_ORG_LDDSTUB_64_SIZE	18

#define	MSG_PTH_LDDSTUB	36
#define	MSG_PTH_LDDSTUB_SIZE	16

#define	MSG_PTH_LDDSTUB_64	53
#define	MSG_PTH_LDDSTUB_64_SIZE	19

static const char __sgs_msg[73] = { 
/*    0 */ 0x00,  0x2f,  0x2e,  0x2e,  0x2f,  0x6c,  0x69,  0x62,  0x2f,  0x6c,
/*   10 */ 0x64,  0x64,  0x73,  0x74,  0x75,  0x62,  0x00,  0x2f,  0x2e,  0x2e,
/*   20 */ 0x2f,  0x6c,  0x69,  0x62,  0x2f,  0x36,  0x34,  0x2f,  0x6c,  0x64,
/*   30 */ 0x64,  0x73,  0x74,  0x75,  0x62,  0x00,  0x2f,  0x75,  0x73,  0x72,
/*   40 */ 0x2f,  0x6c,  0x69,  0x62,  0x2f,  0x6c,  0x64,  0x64,  0x73,  0x74,
/*   50 */ 0x75,  0x62,  0x00,  0x2f,  0x75,  0x73,  0x72,  0x2f,  0x6c,  0x69,
/*   60 */ 0x62,  0x2f,  0x36,  0x34,  0x2f,  0x6c,  0x64,  0x64,  0x73,  0x74,
/*   70 */ 0x75,  0x62,  0x00 };

#else	/* __lint */


typedef char *	Msg;

extern	const char *	_sgs_msg(Msg);

#define MSG_ORIG(x)	x
#define MSG_INTL(x)	x

#define	MSG_ORG_LDDSTUB	"/../lib/lddstub"
#define	MSG_ORG_LDDSTUB_SIZE	15

#define	MSG_ORG_LDDSTUB_64	"/../lib/64/lddstub"
#define	MSG_ORG_LDDSTUB_64_SIZE	18

#define	MSG_PTH_LDDSTUB	"/usr/lib/lddstub"
#define	MSG_PTH_LDDSTUB_SIZE	16

#define	MSG_PTH_LDDSTUB_64	"/usr/lib/64/lddstub"
#define	MSG_PTH_LDDSTUB_64_SIZE	19

#endif	/* __lint */

#endif

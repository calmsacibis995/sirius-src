#ifndef	_DEFTAG_MSG_DOT_H
#define	_DEFTAG_MSG_DOT_H

#ifndef	__lint

typedef int	Msg;

#define	MSG_ORIG(x)	&__sgs_msg[x]

extern	const char *	_sgs_msg(Msg);

#define	MSG_INTL(x)	_sgs_msg(x)


#define	MSG_REF_DYN_SEEN	1
#define	MSG_REF_DYN_SEEN_SIZE	12

#define	MSG_REF_DYN_NEED	14
#define	MSG_REF_DYN_NEED_SIZE	12

#define	MSG_REF_REL_NEED	27
#define	MSG_REF_REL_NEED_SIZE	12

static const char __sgs_msg[40] = { 
/*    0 */ 0x00,  0x52,  0x45,  0x46,  0x5f,  0x44,  0x59,  0x4e,  0x5f,  0x53,
/*   10 */ 0x45,  0x45,  0x4e,  0x00,  0x52,  0x45,  0x46,  0x5f,  0x44,  0x59,
/*   20 */ 0x4e,  0x5f,  0x4e,  0x45,  0x45,  0x44,  0x00,  0x52,  0x45,  0x46,
/*   30 */ 0x5f,  0x52,  0x45,  0x4c,  0x5f,  0x4e,  0x45,  0x45,  0x44,  0x00 };

#else	/* __lint */


typedef char *	Msg;

extern	const char *	_sgs_msg(Msg);

#define MSG_ORIG(x)	x
#define MSG_INTL(x)	x

#define	MSG_REF_DYN_SEEN	"REF_DYN_SEEN"
#define	MSG_REF_DYN_SEEN_SIZE	12

#define	MSG_REF_DYN_NEED	"REF_DYN_NEED"
#define	MSG_REF_DYN_NEED_SIZE	12

#define	MSG_REF_REL_NEED	"REF_REL_NEED"
#define	MSG_REF_REL_NEED_SIZE	12

#endif	/* __lint */

#endif

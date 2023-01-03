#ifndef	_DEMANGLE_MSG_DOT_H
#define	_DEMANGLE_MSG_DOT_H

#ifndef	__lint

typedef int	Msg;

#define	MSG_ORIG(x)	&__sgs_msg[x]

extern	const char *	_sgs_msg(Msg);

#define	MSG_INTL(x)	_sgs_msg(x)


#define	MSG_DEM_SYM	1
#define	MSG_DEM_SYM_SIZE	14

#define	MSG_DEM_LIB	16
#define	MSG_DEM_LIB_SIZE	16

static const char __sgs_msg[33] = { 
/*    0 */ 0x00,  0x63,  0x70,  0x6c,  0x75,  0x73,  0x5f,  0x64,  0x65,  0x6d,
/*   10 */ 0x61,  0x6e,  0x67,  0x6c,  0x65,  0x00,  0x6c,  0x69,  0x62,  0x64,
/*   20 */ 0x65,  0x6d,  0x61,  0x6e,  0x67,  0x6c,  0x65,  0x2e,  0x73,  0x6f,
/*   30 */ 0x2e,  0x31,  0x00 };

#else	/* __lint */


typedef char *	Msg;

extern	const char *	_sgs_msg(Msg);

#define MSG_ORIG(x)	x
#define MSG_INTL(x)	x

#define	MSG_DEM_SYM	"cplus_demangle"
#define	MSG_DEM_SYM_SIZE	14

#define	MSG_DEM_LIB	"libdemangle.so.1"
#define	MSG_DEM_LIB_SIZE	16

#endif	/* __lint */

#endif

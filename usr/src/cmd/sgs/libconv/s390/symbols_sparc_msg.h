#ifndef	_SYMBOLS_SPARC_MSG_DOT_H
#define	_SYMBOLS_SPARC_MSG_DOT_H

#ifndef	__lint

typedef int	Msg;

#define	MSG_ORIG(x)	&__sgs_msg[x]

extern	const char *	_sgs_msg(Msg);

#define	MSG_INTL(x)	_sgs_msg(x)


#define	MSG_STO_REGISTERG1	1
#define	MSG_STO_REGISTERG1_SIZE	6

#define	MSG_STO_REGISTERG2	8
#define	MSG_STO_REGISTERG2_SIZE	6

#define	MSG_STO_REGISTERG3	15
#define	MSG_STO_REGISTERG3_SIZE	6

#define	MSG_STO_REGISTERG4	22
#define	MSG_STO_REGISTERG4_SIZE	6

#define	MSG_STO_REGISTERG5	29
#define	MSG_STO_REGISTERG5_SIZE	6

#define	MSG_STO_REGISTERG6	36
#define	MSG_STO_REGISTERG6_SIZE	6

#define	MSG_STO_REGISTERG7	43
#define	MSG_STO_REGISTERG7_SIZE	6

static const char __sgs_msg[50] = { 
/*    0 */ 0x00,  0x52,  0x45,  0x47,  0x5f,  0x47,  0x31,  0x00,  0x52,  0x45,
/*   10 */ 0x47,  0x5f,  0x47,  0x32,  0x00,  0x52,  0x45,  0x47,  0x5f,  0x47,
/*   20 */ 0x33,  0x00,  0x52,  0x45,  0x47,  0x5f,  0x47,  0x34,  0x00,  0x52,
/*   30 */ 0x45,  0x47,  0x5f,  0x47,  0x35,  0x00,  0x52,  0x45,  0x47,  0x5f,
/*   40 */ 0x47,  0x36,  0x00,  0x52,  0x45,  0x47,  0x5f,  0x47,  0x37,  0x00 };

#else	/* __lint */


typedef char *	Msg;

extern	const char *	_sgs_msg(Msg);

#define MSG_ORIG(x)	x
#define MSG_INTL(x)	x

#define	MSG_STO_REGISTERG1	"REG_G1"
#define	MSG_STO_REGISTERG1_SIZE	6

#define	MSG_STO_REGISTERG2	"REG_G2"
#define	MSG_STO_REGISTERG2_SIZE	6

#define	MSG_STO_REGISTERG3	"REG_G3"
#define	MSG_STO_REGISTERG3_SIZE	6

#define	MSG_STO_REGISTERG4	"REG_G4"
#define	MSG_STO_REGISTERG4_SIZE	6

#define	MSG_STO_REGISTERG5	"REG_G5"
#define	MSG_STO_REGISTERG5_SIZE	6

#define	MSG_STO_REGISTERG6	"REG_G6"
#define	MSG_STO_REGISTERG6_SIZE	6

#define	MSG_STO_REGISTERG7	"REG_G7"
#define	MSG_STO_REGISTERG7_SIZE	6

#endif	/* __lint */

#endif

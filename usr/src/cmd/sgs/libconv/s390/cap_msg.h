#ifndef	_CAP_MSG_DOT_H
#define	_CAP_MSG_DOT_H

#ifndef	__lint

typedef int	Msg;

#define	MSG_ORIG(x)	&__sgs_msg[x]

extern	const char *	_sgs_msg(Msg);

#define	MSG_INTL(x)	_sgs_msg(x)


#define	MSG_CA_SUNW_NULL	1
#define	MSG_CA_SUNW_NULL_SIZE	12

#define	MSG_CA_SUNW_HW_1	14
#define	MSG_CA_SUNW_HW_1_SIZE	12

#define	MSG_CA_SUNW_SF_1	27
#define	MSG_CA_SUNW_SF_1_SIZE	12

#define	MSG_GBL_ZERO	40
#define	MSG_GBL_ZERO_SIZE	1

#define	MSG_GBL_OSQBRKT	42
#define	MSG_GBL_OSQBRKT_SIZE	10

#define	MSG_GBL_CSQBRKT	53
#define	MSG_GBL_CSQBRKT_SIZE	2

static const char __sgs_msg[56] = { 
/*    0 */ 0x00,  0x43,  0x41,  0x5f,  0x53,  0x55,  0x4e,  0x57,  0x5f,  0x4e,
/*   10 */ 0x55,  0x4c,  0x4c,  0x00,  0x43,  0x41,  0x5f,  0x53,  0x55,  0x4e,
/*   20 */ 0x57,  0x5f,  0x48,  0x57,  0x5f,  0x31,  0x00,  0x43,  0x41,  0x5f,
/*   30 */ 0x53,  0x55,  0x4e,  0x57,  0x5f,  0x53,  0x46,  0x5f,  0x31,  0x00,
/*   40 */ 0x30,  0x00,  0x30,  0x78,  0x25,  0x6c,  0x6c,  0x78,  0x20,  0x20,
/*   50 */ 0x5b,  0x20,  0x00,  0x20,  0x5d,  0x00 };

#else	/* __lint */


typedef char *	Msg;

extern	const char *	_sgs_msg(Msg);

#define MSG_ORIG(x)	x
#define MSG_INTL(x)	x

#define	MSG_CA_SUNW_NULL	"CA_SUNW_NULL"
#define	MSG_CA_SUNW_NULL_SIZE	12

#define	MSG_CA_SUNW_HW_1	"CA_SUNW_HW_1"
#define	MSG_CA_SUNW_HW_1_SIZE	12

#define	MSG_CA_SUNW_SF_1	"CA_SUNW_SF_1"
#define	MSG_CA_SUNW_SF_1_SIZE	12

#define	MSG_GBL_ZERO	"0"
#define	MSG_GBL_ZERO_SIZE	1

#define	MSG_GBL_OSQBRKT	"0x%llx  [ "
#define	MSG_GBL_OSQBRKT_SIZE	10

#define	MSG_GBL_CSQBRKT	" ]"
#define	MSG_GBL_CSQBRKT_SIZE	2

#endif	/* __lint */

#endif

#ifndef	_SEGMENTS_MSG_DOT_H
#define	_SEGMENTS_MSG_DOT_H

#ifndef	__lint

typedef int	Msg;

#define	MSG_ORIG(x)	&__sgs_msg[x]

extern	const char *	_sgs_msg(Msg);

#define	MSG_INTL(x)	_sgs_msg(x)


#define	MSG_FLG_SG_VADDR	1
#define	MSG_FLG_SG_VADDR_SIZE	12

#define	MSG_FLG_SG_PADDR	14
#define	MSG_FLG_SG_PADDR_SIZE	12

#define	MSG_FLG_SG_LENGTH	27
#define	MSG_FLG_SG_LENGTH_SIZE	13

#define	MSG_FLG_SG_ALIGN	41
#define	MSG_FLG_SG_ALIGN_SIZE	12

#define	MSG_FLG_SG_ROUND	54
#define	MSG_FLG_SG_ROUND_SIZE	12

#define	MSG_FLG_SG_FLAGS	67
#define	MSG_FLG_SG_FLAGS_SIZE	12

#define	MSG_FLG_SG_TYPE	80
#define	MSG_FLG_SG_TYPE_SIZE	11

#define	MSG_FLG_SG_ORDER	92
#define	MSG_FLG_SG_ORDER_SIZE	12

#define	MSG_FLG_SG_NOHDR	105
#define	MSG_FLG_SG_NOHDR_SIZE	12

#define	MSG_FLG_SG_EMPTY	118
#define	MSG_FLG_SG_EMPTY_SIZE	12

#define	MSG_FLG_SG_KEY	131
#define	MSG_FLG_SG_KEY_SIZE	10

#define	MSG_FLG_SG_DISABLED	142
#define	MSG_FLG_SG_DISABLED_SIZE	15

#define	MSG_FLG_SG_PHREQ	158
#define	MSG_FLG_SG_PHREQ_SIZE	12

#define	MSG_GBL_ZERO	171
#define	MSG_GBL_ZERO_SIZE	1

static const char __sgs_msg[173] = { 
/*    0 */ 0x00,  0x46,  0x4c,  0x47,  0x5f,  0x53,  0x47,  0x5f,  0x56,  0x41,
/*   10 */ 0x44,  0x44,  0x52,  0x00,  0x46,  0x4c,  0x47,  0x5f,  0x53,  0x47,
/*   20 */ 0x5f,  0x50,  0x41,  0x44,  0x44,  0x52,  0x00,  0x46,  0x4c,  0x47,
/*   30 */ 0x5f,  0x53,  0x47,  0x5f,  0x4c,  0x45,  0x4e,  0x47,  0x54,  0x48,
/*   40 */ 0x00,  0x46,  0x4c,  0x47,  0x5f,  0x53,  0x47,  0x5f,  0x41,  0x4c,
/*   50 */ 0x49,  0x47,  0x4e,  0x00,  0x46,  0x4c,  0x47,  0x5f,  0x53,  0x47,
/*   60 */ 0x5f,  0x52,  0x4f,  0x55,  0x4e,  0x44,  0x00,  0x46,  0x4c,  0x47,
/*   70 */ 0x5f,  0x53,  0x47,  0x5f,  0x46,  0x4c,  0x41,  0x47,  0x53,  0x00,
/*   80 */ 0x46,  0x4c,  0x47,  0x5f,  0x53,  0x47,  0x5f,  0x54,  0x59,  0x50,
/*   90 */ 0x45,  0x00,  0x46,  0x4c,  0x47,  0x5f,  0x53,  0x47,  0x5f,  0x4f,
/*  100 */ 0x52,  0x44,  0x45,  0x52,  0x00,  0x46,  0x4c,  0x47,  0x5f,  0x53,
/*  110 */ 0x47,  0x5f,  0x4e,  0x4f,  0x48,  0x44,  0x52,  0x00,  0x46,  0x4c,
/*  120 */ 0x47,  0x5f,  0x53,  0x47,  0x5f,  0x45,  0x4d,  0x50,  0x54,  0x59,
/*  130 */ 0x00,  0x46,  0x4c,  0x47,  0x5f,  0x53,  0x47,  0x5f,  0x4b,  0x45,
/*  140 */ 0x59,  0x00,  0x46,  0x4c,  0x47,  0x5f,  0x53,  0x47,  0x5f,  0x44,
/*  150 */ 0x49,  0x53,  0x41,  0x42,  0x4c,  0x45,  0x44,  0x00,  0x46,  0x4c,
/*  160 */ 0x47,  0x5f,  0x53,  0x47,  0x5f,  0x50,  0x48,  0x52,  0x45,  0x51,
/*  170 */ 0x00,  0x30,  0x00 };

#else	/* __lint */


typedef char *	Msg;

extern	const char *	_sgs_msg(Msg);

#define MSG_ORIG(x)	x
#define MSG_INTL(x)	x

#define	MSG_FLG_SG_VADDR	"FLG_SG_VADDR"
#define	MSG_FLG_SG_VADDR_SIZE	12

#define	MSG_FLG_SG_PADDR	"FLG_SG_PADDR"
#define	MSG_FLG_SG_PADDR_SIZE	12

#define	MSG_FLG_SG_LENGTH	"FLG_SG_LENGTH"
#define	MSG_FLG_SG_LENGTH_SIZE	13

#define	MSG_FLG_SG_ALIGN	"FLG_SG_ALIGN"
#define	MSG_FLG_SG_ALIGN_SIZE	12

#define	MSG_FLG_SG_ROUND	"FLG_SG_ROUND"
#define	MSG_FLG_SG_ROUND_SIZE	12

#define	MSG_FLG_SG_FLAGS	"FLG_SG_FLAGS"
#define	MSG_FLG_SG_FLAGS_SIZE	12

#define	MSG_FLG_SG_TYPE	"FLG_SG_TYPE"
#define	MSG_FLG_SG_TYPE_SIZE	11

#define	MSG_FLG_SG_ORDER	"FLG_SG_ORDER"
#define	MSG_FLG_SG_ORDER_SIZE	12

#define	MSG_FLG_SG_NOHDR	"FLG_SG_NOHDR"
#define	MSG_FLG_SG_NOHDR_SIZE	12

#define	MSG_FLG_SG_EMPTY	"FLG_SG_EMPTY"
#define	MSG_FLG_SG_EMPTY_SIZE	12

#define	MSG_FLG_SG_KEY	"FLG_SG_KEY"
#define	MSG_FLG_SG_KEY_SIZE	10

#define	MSG_FLG_SG_DISABLED	"FLG_SG_DISABLED"
#define	MSG_FLG_SG_DISABLED_SIZE	15

#define	MSG_FLG_SG_PHREQ	"FLG_SG_PHREQ"
#define	MSG_FLG_SG_PHREQ_SIZE	12

#define	MSG_GBL_ZERO	"0"
#define	MSG_GBL_ZERO_SIZE	1

#endif	/* __lint */

#endif

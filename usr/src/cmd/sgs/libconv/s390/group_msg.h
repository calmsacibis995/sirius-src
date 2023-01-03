#ifndef	_GROUP_MSG_DOT_H
#define	_GROUP_MSG_DOT_H

#ifndef	__lint

typedef int	Msg;

#define	MSG_ORIG(x)	&__sgs_msg[x]

extern	const char *	_sgs_msg(Msg);

#define	MSG_INTL(x)	_sgs_msg(x)


#define	MSG_GPH_ZERO	1
#define	MSG_GPH_ZERO_SIZE	8

#define	MSG_GPH_LDSO	10
#define	MSG_GPH_LDSO_SIZE	8

#define	MSG_GPH_FIRST	19
#define	MSG_GPH_FIRST_SIZE	9

#define	MSG_GPH_FILTEE	29
#define	MSG_GPH_FILTEE_SIZE	10

#define	MSG_GPH_INITIAL	40
#define	MSG_GPH_INITIAL_SIZE	11

#define	MSG_GPH_NOPENDLAZY	52
#define	MSG_GPH_NOPENDLAZY_SIZE	14

#define	MSG_GPD_DLSYM	67
#define	MSG_GPD_DLSYM_SIZE	9

#define	MSG_GPD_RELOC	77
#define	MSG_GPD_RELOC_SIZE	9

#define	MSG_GPD_ADDEPS	87
#define	MSG_GPD_ADDEPS_SIZE	10

#define	MSG_GPD_PARENT	98
#define	MSG_GPD_PARENT_SIZE	10

#define	MSG_GPD_FILTER	109
#define	MSG_GPD_FILTER_SIZE	10

#define	MSG_GPD_PROMOTE	120
#define	MSG_GPD_PROMOTE_SIZE	11

#define	MSG_GPD_REMOVE	132
#define	MSG_GPD_REMOVE_SIZE	10

#define	MSG_GBL_NULL	0
#define	MSG_GBL_NULL_SIZE	0

static const char __sgs_msg[143] = { 
/*    0 */ 0x00,  0x47,  0x50,  0x48,  0x5f,  0x5a,  0x45,  0x52,  0x4f,  0x00,
/*   10 */ 0x47,  0x50,  0x48,  0x5f,  0x4c,  0x44,  0x53,  0x4f,  0x00,  0x47,
/*   20 */ 0x50,  0x48,  0x5f,  0x46,  0x49,  0x52,  0x53,  0x54,  0x00,  0x47,
/*   30 */ 0x50,  0x48,  0x5f,  0x46,  0x49,  0x4c,  0x54,  0x45,  0x45,  0x00,
/*   40 */ 0x47,  0x50,  0x48,  0x5f,  0x49,  0x4e,  0x49,  0x54,  0x49,  0x41,
/*   50 */ 0x4c,  0x00,  0x47,  0x50,  0x48,  0x5f,  0x4e,  0x4f,  0x50,  0x45,
/*   60 */ 0x4e,  0x44,  0x4c,  0x41,  0x5a,  0x59,  0x00,  0x47,  0x50,  0x44,
/*   70 */ 0x5f,  0x44,  0x4c,  0x53,  0x59,  0x4d,  0x00,  0x47,  0x50,  0x44,
/*   80 */ 0x5f,  0x52,  0x45,  0x4c,  0x4f,  0x43,  0x00,  0x47,  0x50,  0x44,
/*   90 */ 0x5f,  0x41,  0x44,  0x44,  0x45,  0x50,  0x53,  0x00,  0x47,  0x50,
/*  100 */ 0x44,  0x5f,  0x50,  0x41,  0x52,  0x45,  0x4e,  0x54,  0x00,  0x47,
/*  110 */ 0x50,  0x44,  0x5f,  0x46,  0x49,  0x4c,  0x54,  0x45,  0x52,  0x00,
/*  120 */ 0x47,  0x50,  0x44,  0x5f,  0x50,  0x52,  0x4f,  0x4d,  0x4f,  0x54,
/*  130 */ 0x45,  0x00,  0x47,  0x50,  0x44,  0x5f,  0x52,  0x45,  0x4d,  0x4f,
/*  140 */ 0x56,  0x45,  0x00 };

#else	/* __lint */


typedef char *	Msg;

extern	const char *	_sgs_msg(Msg);

#define MSG_ORIG(x)	x
#define MSG_INTL(x)	x

#define	MSG_GPH_ZERO	"GPH_ZERO"
#define	MSG_GPH_ZERO_SIZE	8

#define	MSG_GPH_LDSO	"GPH_LDSO"
#define	MSG_GPH_LDSO_SIZE	8

#define	MSG_GPH_FIRST	"GPH_FIRST"
#define	MSG_GPH_FIRST_SIZE	9

#define	MSG_GPH_FILTEE	"GPH_FILTEE"
#define	MSG_GPH_FILTEE_SIZE	10

#define	MSG_GPH_INITIAL	"GPH_INITIAL"
#define	MSG_GPH_INITIAL_SIZE	11

#define	MSG_GPH_NOPENDLAZY	"GPH_NOPENDLAZY"
#define	MSG_GPH_NOPENDLAZY_SIZE	14

#define	MSG_GPD_DLSYM	"GPD_DLSYM"
#define	MSG_GPD_DLSYM_SIZE	9

#define	MSG_GPD_RELOC	"GPD_RELOC"
#define	MSG_GPD_RELOC_SIZE	9

#define	MSG_GPD_ADDEPS	"GPD_ADDEPS"
#define	MSG_GPD_ADDEPS_SIZE	10

#define	MSG_GPD_PARENT	"GPD_PARENT"
#define	MSG_GPD_PARENT_SIZE	10

#define	MSG_GPD_FILTER	"GPD_FILTER"
#define	MSG_GPD_FILTER_SIZE	10

#define	MSG_GPD_PROMOTE	"GPD_PROMOTE"
#define	MSG_GPD_PROMOTE_SIZE	11

#define	MSG_GPD_REMOVE	"GPD_REMOVE"
#define	MSG_GPD_REMOVE_SIZE	10

#define	MSG_GBL_NULL	""
#define	MSG_GBL_NULL_SIZE	0

#endif	/* __lint */

#endif

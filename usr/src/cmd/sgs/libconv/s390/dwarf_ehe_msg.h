#ifndef	_DWARF_EHE_MSG_DOT_H
#define	_DWARF_EHE_MSG_DOT_H

#ifndef	__lint

typedef int	Msg;

#define	MSG_ORIG(x)	&__sgs_msg[x]

extern	const char *	_sgs_msg(Msg);

#define	MSG_INTL(x)	_sgs_msg(x)


#define	MSG_DWEHE_OMIT	1
#define	MSG_DWEHE_OMIT_SIZE	6

#define	MSG_DWEHE_ABSPTR	8
#define	MSG_DWEHE_ABSPTR_SIZE	8

#define	MSG_DWEHE_ULEB128	17
#define	MSG_DWEHE_ULEB128_SIZE	9

#define	MSG_DWEHE_UDATA2	27
#define	MSG_DWEHE_UDATA2_SIZE	8

#define	MSG_DWEHE_UDATA4	36
#define	MSG_DWEHE_UDATA4_SIZE	8

#define	MSG_DWEHE_UDATA8	45
#define	MSG_DWEHE_UDATA8_SIZE	8

#define	MSG_DWEHE_SLEB128	54
#define	MSG_DWEHE_SLEB128_SIZE	9

#define	MSG_DWEHE_SDATA2	64
#define	MSG_DWEHE_SDATA2_SIZE	8

#define	MSG_DWEHE_SDATA4	73
#define	MSG_DWEHE_SDATA4_SIZE	8

#define	MSG_DWEHE_SDATA8	82
#define	MSG_DWEHE_SDATA8_SIZE	8

#define	MSG_DWEHE_PCREL	91
#define	MSG_DWEHE_PCREL_SIZE	7

#define	MSG_DWEHE_TEXTREL	99
#define	MSG_DWEHE_TEXTREL_SIZE	9

#define	MSG_DWEHE_DATAREL	109
#define	MSG_DWEHE_DATAREL_SIZE	9

#define	MSG_DWEHE_FUNCREL	119
#define	MSG_DWEHE_FUNCREL_SIZE	9

#define	MSG_DWEHE_ALIGNED	129
#define	MSG_DWEHE_ALIGNED_SIZE	9

#define	MSG_DWEHE_INDIRECT	139
#define	MSG_DWEHE_INDIRECT_SIZE	10

#define	MSG_GBL_OSQBRKT	150
#define	MSG_GBL_OSQBRKT_SIZE	1

#define	MSG_GBL_CSQBRKT	152
#define	MSG_GBL_CSQBRKT_SIZE	1

static const char __sgs_msg[154] = { 
/*    0 */ 0x00,  0x20,  0x6f,  0x6d,  0x69,  0x74,  0x20,  0x00,  0x20,  0x61,
/*   10 */ 0x62,  0x73,  0x70,  0x74,  0x72,  0x20,  0x00,  0x20,  0x75,  0x6c,
/*   20 */ 0x65,  0x62,  0x31,  0x32,  0x38,  0x20,  0x00,  0x20,  0x75,  0x64,
/*   30 */ 0x61,  0x74,  0x61,  0x32,  0x20,  0x00,  0x20,  0x75,  0x64,  0x61,
/*   40 */ 0x74,  0x61,  0x34,  0x20,  0x00,  0x20,  0x75,  0x64,  0x61,  0x74,
/*   50 */ 0x61,  0x38,  0x20,  0x00,  0x20,  0x73,  0x6c,  0x65,  0x62,  0x31,
/*   60 */ 0x32,  0x38,  0x20,  0x00,  0x20,  0x73,  0x64,  0x61,  0x74,  0x61,
/*   70 */ 0x32,  0x20,  0x00,  0x20,  0x73,  0x64,  0x61,  0x74,  0x61,  0x34,
/*   80 */ 0x20,  0x00,  0x20,  0x73,  0x64,  0x61,  0x74,  0x61,  0x38,  0x20,
/*   90 */ 0x00,  0x20,  0x70,  0x63,  0x72,  0x65,  0x6c,  0x20,  0x00,  0x20,
/*  100 */ 0x74,  0x65,  0x78,  0x74,  0x72,  0x65,  0x6c,  0x20,  0x00,  0x20,
/*  110 */ 0x64,  0x61,  0x74,  0x61,  0x72,  0x65,  0x6c,  0x20,  0x00,  0x20,
/*  120 */ 0x66,  0x75,  0x6e,  0x63,  0x72,  0x65,  0x6c,  0x20,  0x00,  0x20,
/*  130 */ 0x61,  0x6c,  0x69,  0x67,  0x6e,  0x65,  0x64,  0x20,  0x00,  0x20,
/*  140 */ 0x69,  0x6e,  0x64,  0x69,  0x72,  0x65,  0x63,  0x74,  0x20,  0x00,
/*  150 */ 0x5b,  0x00,  0x5d,  0x00 };

#else	/* __lint */


typedef char *	Msg;

extern	const char *	_sgs_msg(Msg);

#define MSG_ORIG(x)	x
#define MSG_INTL(x)	x

#define	MSG_DWEHE_OMIT	" omit "
#define	MSG_DWEHE_OMIT_SIZE	6

#define	MSG_DWEHE_ABSPTR	" absptr "
#define	MSG_DWEHE_ABSPTR_SIZE	8

#define	MSG_DWEHE_ULEB128	" uleb128 "
#define	MSG_DWEHE_ULEB128_SIZE	9

#define	MSG_DWEHE_UDATA2	" udata2 "
#define	MSG_DWEHE_UDATA2_SIZE	8

#define	MSG_DWEHE_UDATA4	" udata4 "
#define	MSG_DWEHE_UDATA4_SIZE	8

#define	MSG_DWEHE_UDATA8	" udata8 "
#define	MSG_DWEHE_UDATA8_SIZE	8

#define	MSG_DWEHE_SLEB128	" sleb128 "
#define	MSG_DWEHE_SLEB128_SIZE	9

#define	MSG_DWEHE_SDATA2	" sdata2 "
#define	MSG_DWEHE_SDATA2_SIZE	8

#define	MSG_DWEHE_SDATA4	" sdata4 "
#define	MSG_DWEHE_SDATA4_SIZE	8

#define	MSG_DWEHE_SDATA8	" sdata8 "
#define	MSG_DWEHE_SDATA8_SIZE	8

#define	MSG_DWEHE_PCREL	" pcrel "
#define	MSG_DWEHE_PCREL_SIZE	7

#define	MSG_DWEHE_TEXTREL	" textrel "
#define	MSG_DWEHE_TEXTREL_SIZE	9

#define	MSG_DWEHE_DATAREL	" datarel "
#define	MSG_DWEHE_DATAREL_SIZE	9

#define	MSG_DWEHE_FUNCREL	" funcrel "
#define	MSG_DWEHE_FUNCREL_SIZE	9

#define	MSG_DWEHE_ALIGNED	" aligned "
#define	MSG_DWEHE_ALIGNED_SIZE	9

#define	MSG_DWEHE_INDIRECT	" indirect "
#define	MSG_DWEHE_INDIRECT_SIZE	10

#define	MSG_GBL_OSQBRKT	"["
#define	MSG_GBL_OSQBRKT_SIZE	1

#define	MSG_GBL_CSQBRKT	"]"
#define	MSG_GBL_CSQBRKT_SIZE	1

#endif	/* __lint */

#endif

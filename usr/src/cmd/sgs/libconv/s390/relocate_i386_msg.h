#ifndef	_RELOCATE_I386_MSG_DOT_H
#define	_RELOCATE_I386_MSG_DOT_H

#ifndef	__lint

typedef int	Msg;

#define	MSG_ORIG(x)	&__sgs_msg[x]

extern	const char *	_sgs_msg(Msg);

#define	MSG_INTL(x)	_sgs_msg(x)


#define	MSG_R_386_NONE	1
#define	MSG_R_386_NONE_SIZE	10

#define	MSG_R_386_32	12
#define	MSG_R_386_32_SIZE	8

#define	MSG_R_386_PC32	21
#define	MSG_R_386_PC32_SIZE	10

#define	MSG_R_386_GOT32	32
#define	MSG_R_386_GOT32_SIZE	11

#define	MSG_R_386_PLT32	44
#define	MSG_R_386_PLT32_SIZE	11

#define	MSG_R_386_COPY	56
#define	MSG_R_386_COPY_SIZE	10

#define	MSG_R_386_GLOB_DAT	67
#define	MSG_R_386_GLOB_DAT_SIZE	14

#define	MSG_R_386_JMP_SLOT	82
#define	MSG_R_386_JMP_SLOT_SIZE	14

#define	MSG_R_386_RELATIVE	97
#define	MSG_R_386_RELATIVE_SIZE	14

#define	MSG_R_386_GOTOFF	112
#define	MSG_R_386_GOTOFF_SIZE	12

#define	MSG_R_386_GOTPC	125
#define	MSG_R_386_GOTPC_SIZE	11

#define	MSG_R_386_32PLT	137
#define	MSG_R_386_32PLT_SIZE	11

#define	MSG_R_386_TLS_GD_PLT	149
#define	MSG_R_386_TLS_GD_PLT_SIZE	16

#define	MSG_R_386_TLS_LDM_PLT	166
#define	MSG_R_386_TLS_LDM_PLT_SIZE	17

#define	MSG_R_386_TLS_TPOFF	184
#define	MSG_R_386_TLS_TPOFF_SIZE	15

#define	MSG_R_386_TLS_IE	200
#define	MSG_R_386_TLS_IE_SIZE	12

#define	MSG_R_386_TLS_GOTIE	213
#define	MSG_R_386_TLS_GOTIE_SIZE	15

#define	MSG_R_386_TLS_LE	229
#define	MSG_R_386_TLS_LE_SIZE	12

#define	MSG_R_386_TLS_GD	242
#define	MSG_R_386_TLS_GD_SIZE	12

#define	MSG_R_386_TLS_LDM	255
#define	MSG_R_386_TLS_LDM_SIZE	13

#define	MSG_R_386_16	269
#define	MSG_R_386_16_SIZE	8

#define	MSG_R_386_PC16	278
#define	MSG_R_386_PC16_SIZE	10

#define	MSG_R_386_8	289
#define	MSG_R_386_8_SIZE	7

#define	MSG_R_386_PC8	297
#define	MSG_R_386_PC8_SIZE	9

#define	MSG_R_386_UNKNOWN24	307
#define	MSG_R_386_UNKNOWN24_SIZE	15

#define	MSG_R_386_UNKNOWN25	323
#define	MSG_R_386_UNKNOWN25_SIZE	15

#define	MSG_R_386_UNKNOWN26	339
#define	MSG_R_386_UNKNOWN26_SIZE	15

#define	MSG_R_386_UNKNOWN27	355
#define	MSG_R_386_UNKNOWN27_SIZE	15

#define	MSG_R_386_UNKNOWN28	371
#define	MSG_R_386_UNKNOWN28_SIZE	15

#define	MSG_R_386_UNKNOWN29	387
#define	MSG_R_386_UNKNOWN29_SIZE	15

#define	MSG_R_386_UNKNOWN30	403
#define	MSG_R_386_UNKNOWN30_SIZE	15

#define	MSG_R_386_UNKNOWN31	419
#define	MSG_R_386_UNKNOWN31_SIZE	15

#define	MSG_R_386_TLS_LDO_32	435
#define	MSG_R_386_TLS_LDO_32_SIZE	16

#define	MSG_R_386_UNKNOWN33	452
#define	MSG_R_386_UNKNOWN33_SIZE	15

#define	MSG_R_386_UNKNOWN34	468
#define	MSG_R_386_UNKNOWN34_SIZE	15

#define	MSG_R_386_TLS_DTPMOD32	484
#define	MSG_R_386_TLS_DTPMOD32_SIZE	18

#define	MSG_R_386_TLS_DTPOFF32	503
#define	MSG_R_386_TLS_DTPOFF32_SIZE	18

#define	MSG_R_386_UNKNOWN37	522
#define	MSG_R_386_UNKNOWN37_SIZE	15

#define	MSG_R_386_SIZE32	538
#define	MSG_R_386_SIZE32_SIZE	12

static const char __sgs_msg[551] = { 
/*    0 */ 0x00,  0x52,  0x5f,  0x33,  0x38,  0x36,  0x5f,  0x4e,  0x4f,  0x4e,
/*   10 */ 0x45,  0x00,  0x52,  0x5f,  0x33,  0x38,  0x36,  0x5f,  0x33,  0x32,
/*   20 */ 0x00,  0x52,  0x5f,  0x33,  0x38,  0x36,  0x5f,  0x50,  0x43,  0x33,
/*   30 */ 0x32,  0x00,  0x52,  0x5f,  0x33,  0x38,  0x36,  0x5f,  0x47,  0x4f,
/*   40 */ 0x54,  0x33,  0x32,  0x00,  0x52,  0x5f,  0x33,  0x38,  0x36,  0x5f,
/*   50 */ 0x50,  0x4c,  0x54,  0x33,  0x32,  0x00,  0x52,  0x5f,  0x33,  0x38,
/*   60 */ 0x36,  0x5f,  0x43,  0x4f,  0x50,  0x59,  0x00,  0x52,  0x5f,  0x33,
/*   70 */ 0x38,  0x36,  0x5f,  0x47,  0x4c,  0x4f,  0x42,  0x5f,  0x44,  0x41,
/*   80 */ 0x54,  0x00,  0x52,  0x5f,  0x33,  0x38,  0x36,  0x5f,  0x4a,  0x4d,
/*   90 */ 0x50,  0x5f,  0x53,  0x4c,  0x4f,  0x54,  0x00,  0x52,  0x5f,  0x33,
/*  100 */ 0x38,  0x36,  0x5f,  0x52,  0x45,  0x4c,  0x41,  0x54,  0x49,  0x56,
/*  110 */ 0x45,  0x00,  0x52,  0x5f,  0x33,  0x38,  0x36,  0x5f,  0x47,  0x4f,
/*  120 */ 0x54,  0x4f,  0x46,  0x46,  0x00,  0x52,  0x5f,  0x33,  0x38,  0x36,
/*  130 */ 0x5f,  0x47,  0x4f,  0x54,  0x50,  0x43,  0x00,  0x52,  0x5f,  0x33,
/*  140 */ 0x38,  0x36,  0x5f,  0x33,  0x32,  0x50,  0x4c,  0x54,  0x00,  0x52,
/*  150 */ 0x5f,  0x33,  0x38,  0x36,  0x5f,  0x54,  0x4c,  0x53,  0x5f,  0x47,
/*  160 */ 0x44,  0x5f,  0x50,  0x4c,  0x54,  0x00,  0x52,  0x5f,  0x33,  0x38,
/*  170 */ 0x36,  0x5f,  0x54,  0x4c,  0x53,  0x5f,  0x4c,  0x44,  0x4d,  0x5f,
/*  180 */ 0x50,  0x4c,  0x54,  0x00,  0x52,  0x5f,  0x33,  0x38,  0x36,  0x5f,
/*  190 */ 0x54,  0x4c,  0x53,  0x5f,  0x54,  0x50,  0x4f,  0x46,  0x46,  0x00,
/*  200 */ 0x52,  0x5f,  0x33,  0x38,  0x36,  0x5f,  0x54,  0x4c,  0x53,  0x5f,
/*  210 */ 0x49,  0x45,  0x00,  0x52,  0x5f,  0x33,  0x38,  0x36,  0x5f,  0x54,
/*  220 */ 0x4c,  0x53,  0x5f,  0x47,  0x4f,  0x54,  0x49,  0x45,  0x00,  0x52,
/*  230 */ 0x5f,  0x33,  0x38,  0x36,  0x5f,  0x54,  0x4c,  0x53,  0x5f,  0x4c,
/*  240 */ 0x45,  0x00,  0x52,  0x5f,  0x33,  0x38,  0x36,  0x5f,  0x54,  0x4c,
/*  250 */ 0x53,  0x5f,  0x47,  0x44,  0x00,  0x52,  0x5f,  0x33,  0x38,  0x36,
/*  260 */ 0x5f,  0x54,  0x4c,  0x53,  0x5f,  0x4c,  0x44,  0x4d,  0x00,  0x52,
/*  270 */ 0x5f,  0x33,  0x38,  0x36,  0x5f,  0x31,  0x36,  0x00,  0x52,  0x5f,
/*  280 */ 0x33,  0x38,  0x36,  0x5f,  0x50,  0x43,  0x31,  0x36,  0x00,  0x52,
/*  290 */ 0x5f,  0x33,  0x38,  0x36,  0x5f,  0x38,  0x00,  0x52,  0x5f,  0x33,
/*  300 */ 0x38,  0x36,  0x5f,  0x50,  0x43,  0x38,  0x00,  0x52,  0x5f,  0x33,
/*  310 */ 0x38,  0x36,  0x5f,  0x55,  0x4e,  0x4b,  0x4e,  0x4f,  0x57,  0x4e,
/*  320 */ 0x32,  0x34,  0x00,  0x52,  0x5f,  0x33,  0x38,  0x36,  0x5f,  0x55,
/*  330 */ 0x4e,  0x4b,  0x4e,  0x4f,  0x57,  0x4e,  0x32,  0x35,  0x00,  0x52,
/*  340 */ 0x5f,  0x33,  0x38,  0x36,  0x5f,  0x55,  0x4e,  0x4b,  0x4e,  0x4f,
/*  350 */ 0x57,  0x4e,  0x32,  0x36,  0x00,  0x52,  0x5f,  0x33,  0x38,  0x36,
/*  360 */ 0x5f,  0x55,  0x4e,  0x4b,  0x4e,  0x4f,  0x57,  0x4e,  0x32,  0x37,
/*  370 */ 0x00,  0x52,  0x5f,  0x33,  0x38,  0x36,  0x5f,  0x55,  0x4e,  0x4b,
/*  380 */ 0x4e,  0x4f,  0x57,  0x4e,  0x32,  0x38,  0x00,  0x52,  0x5f,  0x33,
/*  390 */ 0x38,  0x36,  0x5f,  0x55,  0x4e,  0x4b,  0x4e,  0x4f,  0x57,  0x4e,
/*  400 */ 0x32,  0x39,  0x00,  0x52,  0x5f,  0x33,  0x38,  0x36,  0x5f,  0x55,
/*  410 */ 0x4e,  0x4b,  0x4e,  0x4f,  0x57,  0x4e,  0x33,  0x30,  0x00,  0x52,
/*  420 */ 0x5f,  0x33,  0x38,  0x36,  0x5f,  0x55,  0x4e,  0x4b,  0x4e,  0x4f,
/*  430 */ 0x57,  0x4e,  0x33,  0x31,  0x00,  0x52,  0x5f,  0x33,  0x38,  0x36,
/*  440 */ 0x5f,  0x54,  0x4c,  0x53,  0x5f,  0x4c,  0x44,  0x4f,  0x5f,  0x33,
/*  450 */ 0x32,  0x00,  0x52,  0x5f,  0x33,  0x38,  0x36,  0x5f,  0x55,  0x4e,
/*  460 */ 0x4b,  0x4e,  0x4f,  0x57,  0x4e,  0x33,  0x33,  0x00,  0x52,  0x5f,
/*  470 */ 0x33,  0x38,  0x36,  0x5f,  0x55,  0x4e,  0x4b,  0x4e,  0x4f,  0x57,
/*  480 */ 0x4e,  0x33,  0x34,  0x00,  0x52,  0x5f,  0x33,  0x38,  0x36,  0x5f,
/*  490 */ 0x54,  0x4c,  0x53,  0x5f,  0x44,  0x54,  0x50,  0x4d,  0x4f,  0x44,
/*  500 */ 0x33,  0x32,  0x00,  0x52,  0x5f,  0x33,  0x38,  0x36,  0x5f,  0x54,
/*  510 */ 0x4c,  0x53,  0x5f,  0x44,  0x54,  0x50,  0x4f,  0x46,  0x46,  0x33,
/*  520 */ 0x32,  0x00,  0x52,  0x5f,  0x33,  0x38,  0x36,  0x5f,  0x55,  0x4e,
/*  530 */ 0x4b,  0x4e,  0x4f,  0x57,  0x4e,  0x33,  0x37,  0x00,  0x52,  0x5f,
/*  540 */ 0x33,  0x38,  0x36,  0x5f,  0x53,  0x49,  0x5a,  0x45,  0x33,  0x32,
	0x00 };

#else	/* __lint */


typedef char *	Msg;

extern	const char *	_sgs_msg(Msg);

#define MSG_ORIG(x)	x
#define MSG_INTL(x)	x

#define	MSG_R_386_NONE	"R_386_NONE"
#define	MSG_R_386_NONE_SIZE	10

#define	MSG_R_386_32	"R_386_32"
#define	MSG_R_386_32_SIZE	8

#define	MSG_R_386_PC32	"R_386_PC32"
#define	MSG_R_386_PC32_SIZE	10

#define	MSG_R_386_GOT32	"R_386_GOT32"
#define	MSG_R_386_GOT32_SIZE	11

#define	MSG_R_386_PLT32	"R_386_PLT32"
#define	MSG_R_386_PLT32_SIZE	11

#define	MSG_R_386_COPY	"R_386_COPY"
#define	MSG_R_386_COPY_SIZE	10

#define	MSG_R_386_GLOB_DAT	"R_386_GLOB_DAT"
#define	MSG_R_386_GLOB_DAT_SIZE	14

#define	MSG_R_386_JMP_SLOT	"R_386_JMP_SLOT"
#define	MSG_R_386_JMP_SLOT_SIZE	14

#define	MSG_R_386_RELATIVE	"R_386_RELATIVE"
#define	MSG_R_386_RELATIVE_SIZE	14

#define	MSG_R_386_GOTOFF	"R_386_GOTOFF"
#define	MSG_R_386_GOTOFF_SIZE	12

#define	MSG_R_386_GOTPC	"R_386_GOTPC"
#define	MSG_R_386_GOTPC_SIZE	11

#define	MSG_R_386_32PLT	"R_386_32PLT"
#define	MSG_R_386_32PLT_SIZE	11

#define	MSG_R_386_TLS_GD_PLT	"R_386_TLS_GD_PLT"
#define	MSG_R_386_TLS_GD_PLT_SIZE	16

#define	MSG_R_386_TLS_LDM_PLT	"R_386_TLS_LDM_PLT"
#define	MSG_R_386_TLS_LDM_PLT_SIZE	17

#define	MSG_R_386_TLS_TPOFF	"R_386_TLS_TPOFF"
#define	MSG_R_386_TLS_TPOFF_SIZE	15

#define	MSG_R_386_TLS_IE	"R_386_TLS_IE"
#define	MSG_R_386_TLS_IE_SIZE	12

#define	MSG_R_386_TLS_GOTIE	"R_386_TLS_GOTIE"
#define	MSG_R_386_TLS_GOTIE_SIZE	15

#define	MSG_R_386_TLS_LE	"R_386_TLS_LE"
#define	MSG_R_386_TLS_LE_SIZE	12

#define	MSG_R_386_TLS_GD	"R_386_TLS_GD"
#define	MSG_R_386_TLS_GD_SIZE	12

#define	MSG_R_386_TLS_LDM	"R_386_TLS_LDM"
#define	MSG_R_386_TLS_LDM_SIZE	13

#define	MSG_R_386_16	"R_386_16"
#define	MSG_R_386_16_SIZE	8

#define	MSG_R_386_PC16	"R_386_PC16"
#define	MSG_R_386_PC16_SIZE	10

#define	MSG_R_386_8	"R_386_8"
#define	MSG_R_386_8_SIZE	7

#define	MSG_R_386_PC8	"R_386_PC8"
#define	MSG_R_386_PC8_SIZE	9

#define	MSG_R_386_UNKNOWN24	"R_386_UNKNOWN24"
#define	MSG_R_386_UNKNOWN24_SIZE	15

#define	MSG_R_386_UNKNOWN25	"R_386_UNKNOWN25"
#define	MSG_R_386_UNKNOWN25_SIZE	15

#define	MSG_R_386_UNKNOWN26	"R_386_UNKNOWN26"
#define	MSG_R_386_UNKNOWN26_SIZE	15

#define	MSG_R_386_UNKNOWN27	"R_386_UNKNOWN27"
#define	MSG_R_386_UNKNOWN27_SIZE	15

#define	MSG_R_386_UNKNOWN28	"R_386_UNKNOWN28"
#define	MSG_R_386_UNKNOWN28_SIZE	15

#define	MSG_R_386_UNKNOWN29	"R_386_UNKNOWN29"
#define	MSG_R_386_UNKNOWN29_SIZE	15

#define	MSG_R_386_UNKNOWN30	"R_386_UNKNOWN30"
#define	MSG_R_386_UNKNOWN30_SIZE	15

#define	MSG_R_386_UNKNOWN31	"R_386_UNKNOWN31"
#define	MSG_R_386_UNKNOWN31_SIZE	15

#define	MSG_R_386_TLS_LDO_32	"R_386_TLS_LDO_32"
#define	MSG_R_386_TLS_LDO_32_SIZE	16

#define	MSG_R_386_UNKNOWN33	"R_386_UNKNOWN33"
#define	MSG_R_386_UNKNOWN33_SIZE	15

#define	MSG_R_386_UNKNOWN34	"R_386_UNKNOWN34"
#define	MSG_R_386_UNKNOWN34_SIZE	15

#define	MSG_R_386_TLS_DTPMOD32	"R_386_TLS_DTPMOD32"
#define	MSG_R_386_TLS_DTPMOD32_SIZE	18

#define	MSG_R_386_TLS_DTPOFF32	"R_386_TLS_DTPOFF32"
#define	MSG_R_386_TLS_DTPOFF32_SIZE	18

#define	MSG_R_386_UNKNOWN37	"R_386_UNKNOWN37"
#define	MSG_R_386_UNKNOWN37_SIZE	15

#define	MSG_R_386_SIZE32	"R_386_SIZE32"
#define	MSG_R_386_SIZE32_SIZE	12

#endif	/* __lint */

#endif
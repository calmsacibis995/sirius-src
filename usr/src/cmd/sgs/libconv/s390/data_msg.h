#ifndef	_DATA_MSG_DOT_H
#define	_DATA_MSG_DOT_H

#ifndef	__lint

typedef int	Msg;

#define	MSG_ORIG(x)	&__sgs_msg[x]

extern	const char *	_sgs_msg(Msg);

#define	MSG_INTL(x)	_sgs_msg(x)


#define	MSG_DATA_BYTE	1
#define	MSG_DATA_BYTE_SIZE	4

#define	MSG_DATA_ADDR	6
#define	MSG_DATA_ADDR_SIZE	4

#define	MSG_DATA_DYN	11
#define	MSG_DATA_DYN_SIZE	3

#define	MSG_DATA_EHDR	15
#define	MSG_DATA_EHDR_SIZE	4

#define	MSG_DATA_HALF	20
#define	MSG_DATA_HALF_SIZE	4

#define	MSG_DATA_OFF	25
#define	MSG_DATA_OFF_SIZE	3

#define	MSG_DATA_PHDR	29
#define	MSG_DATA_PHDR_SIZE	4

#define	MSG_DATA_RELA	34
#define	MSG_DATA_RELA_SIZE	4

#define	MSG_DATA_REL	39
#define	MSG_DATA_REL_SIZE	3

#define	MSG_DATA_SHDR	43
#define	MSG_DATA_SHDR_SIZE	4

#define	MSG_DATA_SWORD	48
#define	MSG_DATA_SWORD_SIZE	5

#define	MSG_DATA_SYM	54
#define	MSG_DATA_SYM_SIZE	3

#define	MSG_DATA_WORD	49
#define	MSG_DATA_WORD_SIZE	4

#define	MSG_DATA_VDEF	58
#define	MSG_DATA_VDEF_SIZE	4

#define	MSG_DATA_VNEED	63
#define	MSG_DATA_VNEED_SIZE	5

#define	MSG_DATA_SXWORD	69
#define	MSG_DATA_SXWORD_SIZE	6

#define	MSG_DATA_XWORD	70
#define	MSG_DATA_XWORD_SIZE	5

#define	MSG_DATA_SYMINFO	76
#define	MSG_DATA_SYMINFO_SIZE	7

#define	MSG_DATA_NOTE	84
#define	MSG_DATA_NOTE_SIZE	4

#define	MSG_DATA_MOVE	89
#define	MSG_DATA_MOVE_SIZE	4

#define	MSG_DATA_MOVEP	94
#define	MSG_DATA_MOVEP_SIZE	5

#define	MSG_DATA_CAP	100
#define	MSG_DATA_CAP_SIZE	3

static const char __sgs_msg[104] = { 
/*    0 */ 0x00,  0x42,  0x59,  0x54,  0x45,  0x00,  0x41,  0x44,  0x44,  0x52,
/*   10 */ 0x00,  0x44,  0x59,  0x4e,  0x00,  0x45,  0x48,  0x44,  0x52,  0x00,
/*   20 */ 0x48,  0x41,  0x4c,  0x46,  0x00,  0x4f,  0x46,  0x46,  0x00,  0x50,
/*   30 */ 0x48,  0x44,  0x52,  0x00,  0x52,  0x45,  0x4c,  0x41,  0x00,  0x52,
/*   40 */ 0x45,  0x4c,  0x00,  0x53,  0x48,  0x44,  0x52,  0x00,  0x53,  0x57,
/*   50 */ 0x4f,  0x52,  0x44,  0x00,  0x53,  0x59,  0x4d,  0x00,  0x56,  0x44,
/*   60 */ 0x45,  0x46,  0x00,  0x56,  0x4e,  0x45,  0x45,  0x44,  0x00,  0x53,
/*   70 */ 0x58,  0x57,  0x4f,  0x52,  0x44,  0x00,  0x53,  0x59,  0x4d,  0x49,
/*   80 */ 0x4e,  0x46,  0x4f,  0x00,  0x4e,  0x4f,  0x54,  0x45,  0x00,  0x4d,
/*   90 */ 0x4f,  0x56,  0x45,  0x00,  0x4d,  0x4f,  0x56,  0x45,  0x50,  0x00,
/*  100 */ 0x43,  0x41,  0x50,  0x00 };

#else	/* __lint */


typedef char *	Msg;

extern	const char *	_sgs_msg(Msg);

#define MSG_ORIG(x)	x
#define MSG_INTL(x)	x

#define	MSG_DATA_BYTE	"BYTE"
#define	MSG_DATA_BYTE_SIZE	4

#define	MSG_DATA_ADDR	"ADDR"
#define	MSG_DATA_ADDR_SIZE	4

#define	MSG_DATA_DYN	"DYN"
#define	MSG_DATA_DYN_SIZE	3

#define	MSG_DATA_EHDR	"EHDR"
#define	MSG_DATA_EHDR_SIZE	4

#define	MSG_DATA_HALF	"HALF"
#define	MSG_DATA_HALF_SIZE	4

#define	MSG_DATA_OFF	"OFF"
#define	MSG_DATA_OFF_SIZE	3

#define	MSG_DATA_PHDR	"PHDR"
#define	MSG_DATA_PHDR_SIZE	4

#define	MSG_DATA_RELA	"RELA"
#define	MSG_DATA_RELA_SIZE	4

#define	MSG_DATA_REL	"REL"
#define	MSG_DATA_REL_SIZE	3

#define	MSG_DATA_SHDR	"SHDR"
#define	MSG_DATA_SHDR_SIZE	4

#define	MSG_DATA_SWORD	"SWORD"
#define	MSG_DATA_SWORD_SIZE	5

#define	MSG_DATA_SYM	"SYM"
#define	MSG_DATA_SYM_SIZE	3

#define	MSG_DATA_WORD	"WORD"
#define	MSG_DATA_WORD_SIZE	4

#define	MSG_DATA_VDEF	"VDEF"
#define	MSG_DATA_VDEF_SIZE	4

#define	MSG_DATA_VNEED	"VNEED"
#define	MSG_DATA_VNEED_SIZE	5

#define	MSG_DATA_SXWORD	"SXWORD"
#define	MSG_DATA_SXWORD_SIZE	6

#define	MSG_DATA_XWORD	"XWORD"
#define	MSG_DATA_XWORD_SIZE	5

#define	MSG_DATA_SYMINFO	"SYMINFO"
#define	MSG_DATA_SYMINFO_SIZE	7

#define	MSG_DATA_NOTE	"NOTE"
#define	MSG_DATA_NOTE_SIZE	4

#define	MSG_DATA_MOVE	"MOVE"
#define	MSG_DATA_MOVE_SIZE	4

#define	MSG_DATA_MOVEP	"MOVEP"
#define	MSG_DATA_MOVEP_SIZE	5

#define	MSG_DATA_CAP	"CAP"
#define	MSG_DATA_CAP_SIZE	3

#endif	/* __lint */

#endif

#ifndef	_PHDR_MSG_DOT_H
#define	_PHDR_MSG_DOT_H

#ifndef	__lint

typedef int	Msg;

#define	MSG_ORIG(x)	&__sgs_msg[x]

extern	const char *	_sgs_msg(Msg);

#define	MSG_INTL(x)	_sgs_msg(x)


#define	MSG_PT_NULL	1
#define	MSG_PT_NULL_SIZE	11

#define	MSG_PT_NULL_ALT	13
#define	MSG_PT_NULL_ALT_SIZE	7

#define	MSG_PT_LOAD	21
#define	MSG_PT_LOAD_SIZE	11

#define	MSG_PT_LOAD_ALT	33
#define	MSG_PT_LOAD_ALT_SIZE	7

#define	MSG_PT_DYNAMIC	41
#define	MSG_PT_DYNAMIC_SIZE	14

#define	MSG_PT_DYNAMIC_ALT	56
#define	MSG_PT_DYNAMIC_ALT_SIZE	10

#define	MSG_PT_INTERP	67
#define	MSG_PT_INTERP_SIZE	13

#define	MSG_PT_INTERP_ALT	81
#define	MSG_PT_INTERP_ALT_SIZE	9

#define	MSG_PT_NOTE	91
#define	MSG_PT_NOTE_SIZE	11

#define	MSG_PT_NOTE_ALT	103
#define	MSG_PT_NOTE_ALT_SIZE	7

#define	MSG_PT_SHLIB	111
#define	MSG_PT_SHLIB_SIZE	12

#define	MSG_PT_SHLIB_ALT	124
#define	MSG_PT_SHLIB_ALT_SIZE	8

#define	MSG_PT_PHDR	133
#define	MSG_PT_PHDR_SIZE	11

#define	MSG_PT_PHDR_ALT	145
#define	MSG_PT_PHDR_ALT_SIZE	7

#define	MSG_PT_TLS	153
#define	MSG_PT_TLS_SIZE	10

#define	MSG_PT_TLS_ALT	164
#define	MSG_PT_TLS_ALT_SIZE	6

#define	MSG_PT_SUNWBSS	171
#define	MSG_PT_SUNWBSS_SIZE	14

#define	MSG_PT_SUNWBSS_ALT	186
#define	MSG_PT_SUNWBSS_ALT_SIZE	10

#define	MSG_PT_SUNWSTACK	197
#define	MSG_PT_SUNWSTACK_SIZE	16

#define	MSG_PT_SUNWSTACK_ALT	214
#define	MSG_PT_SUNWSTACK_ALT_SIZE	12

#define	MSG_PT_SUNWBSS	171
#define	MSG_PT_SUNWBSS_SIZE	14

#define	MSG_PT_SUNWBSS_ALT	186
#define	MSG_PT_SUNWBSS_ALT_SIZE	10

#define	MSG_PT_SUNWDTRACE	227
#define	MSG_PT_SUNWDTRACE_SIZE	17

#define	MSG_PT_SUNWDTRACE_ALT	245
#define	MSG_PT_SUNWDTRACE_ALT_SIZE	13

#define	MSG_PT_SUNWCAP	259
#define	MSG_PT_SUNWCAP_SIZE	14

#define	MSG_PT_SUNWCAP_ALT	274
#define	MSG_PT_SUNWCAP_ALT_SIZE	10

#define	MSG_PT_SUNW_UNWIND	285
#define	MSG_PT_SUNW_UNWIND_SIZE	18

#define	MSG_PT_SUNW_UNWIND_ALT	304
#define	MSG_PT_SUNW_UNWIND_ALT_SIZE	14

#define	MSG_PF_X	319
#define	MSG_PF_X_SIZE	4

#define	MSG_PF_W	324
#define	MSG_PF_W_SIZE	4

#define	MSG_PF_R	329
#define	MSG_PF_R_SIZE	4

#define	MSG_PF_SUNW_FAILURE	334
#define	MSG_PF_SUNW_FAILURE_SIZE	15

#define	MSG_GBL_ZERO	350
#define	MSG_GBL_ZERO_SIZE	1

static const char __sgs_msg[352] = { 
/*    0 */ 0x00,  0x5b,  0x20,  0x50,  0x54,  0x5f,  0x4e,  0x55,  0x4c,  0x4c,
/*   10 */ 0x20,  0x5d,  0x00,  0x50,  0x54,  0x5f,  0x4e,  0x55,  0x4c,  0x4c,
/*   20 */ 0x00,  0x5b,  0x20,  0x50,  0x54,  0x5f,  0x4c,  0x4f,  0x41,  0x44,
/*   30 */ 0x20,  0x5d,  0x00,  0x50,  0x54,  0x5f,  0x4c,  0x4f,  0x41,  0x44,
/*   40 */ 0x00,  0x5b,  0x20,  0x50,  0x54,  0x5f,  0x44,  0x59,  0x4e,  0x41,
/*   50 */ 0x4d,  0x49,  0x43,  0x20,  0x5d,  0x00,  0x50,  0x54,  0x5f,  0x44,
/*   60 */ 0x59,  0x4e,  0x41,  0x4d,  0x49,  0x43,  0x00,  0x5b,  0x20,  0x50,
/*   70 */ 0x54,  0x5f,  0x49,  0x4e,  0x54,  0x45,  0x52,  0x50,  0x20,  0x5d,
/*   80 */ 0x00,  0x50,  0x54,  0x5f,  0x49,  0x4e,  0x54,  0x45,  0x52,  0x50,
/*   90 */ 0x00,  0x5b,  0x20,  0x50,  0x54,  0x5f,  0x4e,  0x4f,  0x54,  0x45,
/*  100 */ 0x20,  0x5d,  0x00,  0x50,  0x54,  0x5f,  0x4e,  0x4f,  0x54,  0x45,
/*  110 */ 0x00,  0x5b,  0x20,  0x50,  0x54,  0x5f,  0x53,  0x48,  0x4c,  0x49,
/*  120 */ 0x42,  0x20,  0x5d,  0x00,  0x50,  0x54,  0x5f,  0x53,  0x48,  0x4c,
/*  130 */ 0x49,  0x42,  0x00,  0x5b,  0x20,  0x50,  0x54,  0x5f,  0x50,  0x48,
/*  140 */ 0x44,  0x52,  0x20,  0x5d,  0x00,  0x50,  0x54,  0x5f,  0x50,  0x48,
/*  150 */ 0x44,  0x52,  0x00,  0x5b,  0x20,  0x50,  0x54,  0x5f,  0x54,  0x4c,
/*  160 */ 0x53,  0x20,  0x5d,  0x00,  0x50,  0x54,  0x5f,  0x54,  0x4c,  0x53,
/*  170 */ 0x00,  0x5b,  0x20,  0x50,  0x54,  0x5f,  0x53,  0x55,  0x4e,  0x57,
/*  180 */ 0x42,  0x53,  0x53,  0x20,  0x5d,  0x00,  0x50,  0x54,  0x5f,  0x53,
/*  190 */ 0x55,  0x4e,  0x57,  0x42,  0x53,  0x53,  0x00,  0x5b,  0x20,  0x50,
/*  200 */ 0x54,  0x5f,  0x53,  0x55,  0x4e,  0x57,  0x53,  0x54,  0x41,  0x43,
/*  210 */ 0x4b,  0x20,  0x5d,  0x00,  0x50,  0x54,  0x5f,  0x53,  0x55,  0x4e,
/*  220 */ 0x57,  0x53,  0x54,  0x41,  0x43,  0x4b,  0x00,  0x5b,  0x20,  0x50,
/*  230 */ 0x54,  0x5f,  0x53,  0x55,  0x4e,  0x57,  0x44,  0x54,  0x52,  0x41,
/*  240 */ 0x43,  0x45,  0x20,  0x5d,  0x00,  0x50,  0x54,  0x5f,  0x53,  0x55,
/*  250 */ 0x4e,  0x57,  0x44,  0x54,  0x52,  0x41,  0x43,  0x45,  0x00,  0x5b,
/*  260 */ 0x20,  0x50,  0x54,  0x5f,  0x53,  0x55,  0x4e,  0x57,  0x43,  0x41,
/*  270 */ 0x50,  0x20,  0x5d,  0x00,  0x50,  0x54,  0x5f,  0x53,  0x55,  0x4e,
/*  280 */ 0x57,  0x43,  0x41,  0x50,  0x00,  0x5b,  0x20,  0x50,  0x54,  0x5f,
/*  290 */ 0x53,  0x55,  0x4e,  0x57,  0x5f,  0x55,  0x4e,  0x57,  0x49,  0x4e,
/*  300 */ 0x44,  0x20,  0x5d,  0x00,  0x50,  0x54,  0x5f,  0x53,  0x55,  0x4e,
/*  310 */ 0x57,  0x5f,  0x55,  0x4e,  0x57,  0x49,  0x4e,  0x44,  0x00,  0x50,
/*  320 */ 0x46,  0x5f,  0x58,  0x00,  0x50,  0x46,  0x5f,  0x57,  0x00,  0x50,
/*  330 */ 0x46,  0x5f,  0x52,  0x00,  0x50,  0x46,  0x5f,  0x53,  0x55,  0x4e,
/*  340 */ 0x57,  0x5f,  0x46,  0x41,  0x49,  0x4c,  0x55,  0x52,  0x45,  0x00,
/*  350 */ 0x30,  0x00 };

#else	/* __lint */


typedef char *	Msg;

extern	const char *	_sgs_msg(Msg);

#define MSG_ORIG(x)	x
#define MSG_INTL(x)	x

#define	MSG_PT_NULL	"[ PT_NULL ]"
#define	MSG_PT_NULL_SIZE	11

#define	MSG_PT_NULL_ALT	"PT_NULL"
#define	MSG_PT_NULL_ALT_SIZE	7

#define	MSG_PT_LOAD	"[ PT_LOAD ]"
#define	MSG_PT_LOAD_SIZE	11

#define	MSG_PT_LOAD_ALT	"PT_LOAD"
#define	MSG_PT_LOAD_ALT_SIZE	7

#define	MSG_PT_DYNAMIC	"[ PT_DYNAMIC ]"
#define	MSG_PT_DYNAMIC_SIZE	14

#define	MSG_PT_DYNAMIC_ALT	"PT_DYNAMIC"
#define	MSG_PT_DYNAMIC_ALT_SIZE	10

#define	MSG_PT_INTERP	"[ PT_INTERP ]"
#define	MSG_PT_INTERP_SIZE	13

#define	MSG_PT_INTERP_ALT	"PT_INTERP"
#define	MSG_PT_INTERP_ALT_SIZE	9

#define	MSG_PT_NOTE	"[ PT_NOTE ]"
#define	MSG_PT_NOTE_SIZE	11

#define	MSG_PT_NOTE_ALT	"PT_NOTE"
#define	MSG_PT_NOTE_ALT_SIZE	7

#define	MSG_PT_SHLIB	"[ PT_SHLIB ]"
#define	MSG_PT_SHLIB_SIZE	12

#define	MSG_PT_SHLIB_ALT	"PT_SHLIB"
#define	MSG_PT_SHLIB_ALT_SIZE	8

#define	MSG_PT_PHDR	"[ PT_PHDR ]"
#define	MSG_PT_PHDR_SIZE	11

#define	MSG_PT_PHDR_ALT	"PT_PHDR"
#define	MSG_PT_PHDR_ALT_SIZE	7

#define	MSG_PT_TLS	"[ PT_TLS ]"
#define	MSG_PT_TLS_SIZE	10

#define	MSG_PT_TLS_ALT	"PT_TLS"
#define	MSG_PT_TLS_ALT_SIZE	6

#define	MSG_PT_SUNWBSS	"[ PT_SUNWBSS ]"
#define	MSG_PT_SUNWBSS_SIZE	14

#define	MSG_PT_SUNWBSS_ALT	"PT_SUNWBSS"
#define	MSG_PT_SUNWBSS_ALT_SIZE	10

#define	MSG_PT_SUNWSTACK	"[ PT_SUNWSTACK ]"
#define	MSG_PT_SUNWSTACK_SIZE	16

#define	MSG_PT_SUNWSTACK_ALT	"PT_SUNWSTACK"
#define	MSG_PT_SUNWSTACK_ALT_SIZE	12

#define	MSG_PT_SUNWBSS	"[ PT_SUNWBSS ]"
#define	MSG_PT_SUNWBSS_SIZE	14

#define	MSG_PT_SUNWBSS_ALT	"PT_SUNWBSS"
#define	MSG_PT_SUNWBSS_ALT_SIZE	10

#define	MSG_PT_SUNWDTRACE	"[ PT_SUNWDTRACE ]"
#define	MSG_PT_SUNWDTRACE_SIZE	17

#define	MSG_PT_SUNWDTRACE_ALT	"PT_SUNWDTRACE"
#define	MSG_PT_SUNWDTRACE_ALT_SIZE	13

#define	MSG_PT_SUNWCAP	"[ PT_SUNWCAP ]"
#define	MSG_PT_SUNWCAP_SIZE	14

#define	MSG_PT_SUNWCAP_ALT	"PT_SUNWCAP"
#define	MSG_PT_SUNWCAP_ALT_SIZE	10

#define	MSG_PT_SUNW_UNWIND	"[ PT_SUNW_UNWIND ]"
#define	MSG_PT_SUNW_UNWIND_SIZE	18

#define	MSG_PT_SUNW_UNWIND_ALT	"PT_SUNW_UNWIND"
#define	MSG_PT_SUNW_UNWIND_ALT_SIZE	14

#define	MSG_PF_X	"PF_X"
#define	MSG_PF_X_SIZE	4

#define	MSG_PF_W	"PF_W"
#define	MSG_PF_W_SIZE	4

#define	MSG_PF_R	"PF_R"
#define	MSG_PF_R_SIZE	4

#define	MSG_PF_SUNW_FAILURE	"PF_SUNW_FAILURE"
#define	MSG_PF_SUNW_FAILURE_SIZE	15

#define	MSG_GBL_ZERO	"0"
#define	MSG_GBL_ZERO_SIZE	1

#endif	/* __lint */

#endif

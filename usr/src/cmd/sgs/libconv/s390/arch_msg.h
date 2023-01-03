#ifndef	_ARCH_MSG_DOT_H
#define	_ARCH_MSG_DOT_H

#ifndef	__lint

typedef int	Msg;

#define	MSG_ORIG(x)	&__sgs_msg[x]

extern	const char *	_sgs_msg(Msg);

#define	MSG_INTL(x)	_sgs_msg(x)


#define	MSG_ARCH_SPARCV9	1
#define	MSG_ARCH_SPARCV9_SIZE	7

#define	MSG_ARCH_AMD64	9
#define	MSG_ARCH_AMD64_SIZE	5

#define	MSG_ARCH_S390X	15
#define	MSG_ARCH_S390X_SIZE	5

#define	MSG_LD_NOEXEC64	21
#define	MSG_LD_NOEXEC64_SIZE	12

#define	MSG_STR_SLASH	34
#define	MSG_STR_SLASH_SIZE	1

static const char __sgs_msg[36] = { 
/*    0 */ 0x00,  0x73,  0x70,  0x61,  0x72,  0x63,  0x76,  0x39,  0x00,  0x61,
/*   10 */ 0x6d,  0x64,  0x36,  0x34,  0x00,  0x73,  0x33,  0x39,  0x30,  0x78,
/*   20 */ 0x00,  0x4c,  0x44,  0x5f,  0x4e,  0x4f,  0x45,  0x58,  0x45,  0x43,
/*   30 */ 0x5f,  0x36,  0x34,  0x00,  0x2f,  0x00 };

#else	/* __lint */


typedef char *	Msg;

extern	const char *	_sgs_msg(Msg);

#define MSG_ORIG(x)	x
#define MSG_INTL(x)	x

#define	MSG_ARCH_SPARCV9	"sparcv9"
#define	MSG_ARCH_SPARCV9_SIZE	7

#define	MSG_ARCH_AMD64	"amd64"
#define	MSG_ARCH_AMD64_SIZE	5

#define	MSG_ARCH_S390X	"s390x"
#define	MSG_ARCH_S390X_SIZE	5

#define	MSG_LD_NOEXEC64	"LD_NOEXEC_64"
#define	MSG_LD_NOEXEC64_SIZE	12

#define	MSG_STR_SLASH	"/"
#define	MSG_STR_SLASH_SIZE	1

#endif	/* __lint */

#endif

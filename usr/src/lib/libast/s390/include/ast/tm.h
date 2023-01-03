
/* : : generated by proto : : */
/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*           Copyright (c) 1985-2007 AT&T Knowledge Ventures            *
*                      and is licensed under the                       *
*                  Common Public License, Version 1.0                  *
*                      by AT&T Knowledge Ventures                      *
*                                                                      *
*                A copy of the License is available at                 *
*            http://www.opensource.org/licenses/cpl1.0.txt             *
*         (with md5 checksum 059e8cd6165cb4c31e351f2b69388fd9)         *
*                                                                      *
*              Information and Software Systems Research               *
*                            AT&T Research                             *
*                           Florham Park NJ                            *
*                                                                      *
*                 Glenn Fowler <gsf@research.att.com>                  *
*                  David Korn <dgk@research.att.com>                   *
*                   Phong Vo <kpv@research.att.com>                    *
*                                                                      *
***********************************************************************/
                  
/*
 * Glenn Fowler
 * AT&T Research
 *
 * time conversion support definitions
 */

#ifndef _TM_H
#if !defined(__PROTO__)
#include <prototyped.h>
#endif
#if !defined(__LINKAGE__)
#define __LINKAGE__		/* 2004-08-11 transition */
#endif

#define _TM_H

#define TM_VERSION	20070319L

#include <ast.h>
#include <times.h>

#undef	daylight

#define tmset(z)	tminit(z)
#define tmisleapyear(y)	(!((y)%4)&&(((y)%100)||!((((y)<1900)?((y)+1900):(y))%400)))

#define TM_ADJUST	(1<<0)		/* local doesn't do leap secs	*/
#define TM_LEAP		(1<<1)		/* do leap seconds		*/
#define TM_UTC		(1<<2)		/* universal coordinated ref	*/

#define TM_PEDANTIC	(1<<3)		/* pedantic date parse		*/
#define TM_DATESTYLE	(1<<4)		/* date(1) style mmddHHMMccyy	*/
#define TM_SUBSECOND	(1<<5)		/* <something>%S => ...%S.%P	*/

#define TM_DST		(-60)		/* default minutes for DST	*/
#define TM_LOCALZONE	(25 * 60)	/* use local time zone offset	*/
#define TM_UTCZONE	(26 * 60)	/* UTC "time zone"		*/
#define TM_MAXLEAP	1		/* max leap secs per leap	*/
#define TM_WINDOW	69		/* century windowing guard year	*/

/*
 * these indices must agree with tm_dform[]
 */

#define TM_MONTH_ABBREV		0
#define TM_MONTH		12
#define TM_DAY_ABBREV		24
#define TM_DAY			31
#define TM_TIME			38
#define TM_DATE			39
#define TM_DEFAULT		40
#define TM_MERIDIAN		41

#define TM_UT			43
#define TM_DT			47
#define TM_SUFFIXES		51
#define TM_PARTS		55
#define TM_HOURS		62
#define TM_DAYS			66
#define TM_LAST			69
#define TM_THIS			72
#define TM_NEXT			75
#define TM_EXACT		78
#define TM_NOISE		81
#define TM_ORDINAL		85
#define TM_DIGITS		95
#define TM_CTIME		105
#define TM_DATE_1		106
#define TM_INTERNATIONAL	107
#define TM_RECENT		108
#define TM_DISTANT		109
#define TM_MERIDIAN_TIME	110
#define TM_ERA			111
#define TM_ERA_DATE		112
#define TM_ERA_TIME		113
#define TM_ERA_DEFAULT		114
#define TM_ERA_YEAR		115
#define TM_ORDINALS		116
#define TM_FINAL		126

#define TM_NFORM		129

typedef struct				/* leap second info		*/
{
	time_t		time;		/* the leap second event	*/
	int		total;		/* inclusive total since epoch	*/
} Tm_leap_t;

typedef struct				/* time zone info		*/
{
	char*		type;		/* type name			*/
	char*		standard;	/* standard time name		*/
	char*		daylight;	/* daylight or summertime name	*/
	short		west;		/* minutes west of GMT		*/
	short		dst;		/* add to tz.west for DST	*/
} Tm_zone_t;

typedef struct				/* tm library readonly data	*/
{
	char**		format;		/* default TM_* format strings	*/
	char*		lex;		/* format lex type classes	*/
	char*		digit;		/* output digits		*/
	short*		days;		/* days in month i		*/
	short*		sum;		/* days in months before i	*/
	Tm_leap_t*	leap;		/* leap second table		*/
	Tm_zone_t*	zone;		/* alternate timezone table	*/
} Tm_data_t;

typedef struct				/* tm library global info	*/
{
	char*		deformat;	/* TM_DEFAULT override		*/
	int		flags;		/* flags			*/
	char**		format;		/* current format strings	*/
	Tm_zone_t*	date;		/* timezone from last tmdate()	*/
	Tm_zone_t*	local;		/* local timezone		*/
	Tm_zone_t*	zone;		/* current timezone		*/
} Tm_info_t;

typedef struct Tm_s
{
	int			tm_sec;
	int			tm_min;
	int			tm_hour;
	int			tm_mday;
	int			tm_mon;
	int			tm_year;
	int			tm_wday;
	int			tm_yday;
	int			tm_isdst;
	uint32_t		tm_nsec;
	Tm_zone_t*		tm_zone;
} Tm_t;

#if _BLD_ast && defined(__EXPORT__)
#undef __MANGLE__
#define __MANGLE__ __LINKAGE__ __EXPORT__
#endif
#if !_BLD_ast && defined(__IMPORT__)
#undef __MANGLE__
#define __MANGLE__ __LINKAGE__ __IMPORT__
#endif

extern __MANGLE__ Tm_data_t*	_tm_datap_;
extern __MANGLE__ Tm_info_t*	_tm_infop_;

#define tm_data		(*_tm_datap_)
#define tm_info		(*_tm_infop_)

#undef __MANGLE__
#define __MANGLE__ __LINKAGE__

#if _BLD_ast && defined(__EXPORT__)
#undef __MANGLE__
#define __MANGLE__ __LINKAGE__		__EXPORT__
#endif

extern __MANGLE__ time_t		tmdate __PROTO__((const char*, char**, time_t*));
extern __MANGLE__ int		tmequiv __PROTO__((Tm_t*));
extern __MANGLE__ Tm_t*		tmfix __PROTO__((Tm_t*));
extern __MANGLE__ char*		tmfmt __PROTO__((char*, size_t, const char*, time_t*));
extern __MANGLE__ char*		tmform __PROTO__((char*, const char*, time_t*));
extern __MANGLE__ int		tmgoff __PROTO__((const char*, char**, int));
extern __MANGLE__ void		tminit __PROTO__((Tm_zone_t*));
extern __MANGLE__ time_t		tmleap __PROTO__((time_t*));
extern __MANGLE__ int		tmlex __PROTO__((const char*, char**, char**, int, char**, int));
extern __MANGLE__ char**		tmlocale __PROTO__((void));
extern __MANGLE__ Tm_t*		tmmake __PROTO__((time_t*));
extern __MANGLE__ char*		tmpoff __PROTO__((char*, size_t, const char*, int, int));
extern __MANGLE__ time_t		tmscan __PROTO__((const char*, char**, const char*, char**, time_t*, long));
extern __MANGLE__ int		tmsleep __PROTO__((time_t, time_t));
extern __MANGLE__ time_t		tmtime __PROTO__((Tm_t*, int));
extern __MANGLE__ Tm_zone_t*	tmtype __PROTO__((const char*, char**));
extern __MANGLE__ int		tmweek __PROTO__((Tm_t*, int, int, int));
extern __MANGLE__ int		tmword __PROTO__((const char*, char**, const char*, char**, int));
extern __MANGLE__ Tm_zone_t*	tmzone __PROTO__((const char*, char**, const char*, int*));

#undef __MANGLE__
#define __MANGLE__ __LINKAGE__

#endif
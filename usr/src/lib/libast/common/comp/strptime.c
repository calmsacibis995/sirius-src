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
#pragma prototyped
/*
 * strptime implementation
 */

#define strptime	______strptime

#include <ast.h>
#include <tm.h>

#undef	strptime

#undef	_def_map_ast
#include <ast_map.h>

#if _lib_strptime

NoN(strptime)

#else

#if defined(__EXPORT__)
#define extern	__EXPORT__
#endif

extern char*
strptime(const char* s, const char* format, struct tm* ts)
{
	char*	e;
	char*	f;
	time_t	t;
	Tm_t	tm;
	Tm_t*	tn;

	tm.tm_sec = ts->tm_sec;
	tm.tm_min = ts->tm_min;
	tm.tm_hour = ts->tm_hour;
	tm.tm_mday = ts->tm_mday;
	tm.tm_mon = ts->tm_mon;
	tm.tm_year = ts->tm_year;
	tm.tm_wday = ts->tm_wday;
	tm.tm_yday = ts->tm_yday;
	tm.tm_isdst = ts->tm_isdst;
	t = tmtime(&tm, TM_LOCALZONE);
	t = tmscan(s, &e, format, &f, &t, 0);
	if (e == (char*)s || *f)
		return 0;
	tn = tmmake(&t);
	ts->tm_sec = tn->tm_sec;
	ts->tm_min = tn->tm_min;
	ts->tm_hour = tn->tm_hour;
	ts->tm_mday = tn->tm_mday;
	ts->tm_mon = tn->tm_mon;
	ts->tm_year = tn->tm_year;
	ts->tm_wday = tn->tm_wday;
	ts->tm_yday = tn->tm_yday;
	ts->tm_isdst = tn->tm_isdst;
	return e;
}

#endif

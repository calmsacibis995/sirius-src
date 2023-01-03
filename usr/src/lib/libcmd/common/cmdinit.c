/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*           Copyright (c) 1992-2007 AT&T Knowledge Ventures            *
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
*                                                                      *
***********************************************************************/
#pragma prototyped
/*
 * command initialization
 */

#include <cmd.h>

int	_cmd_quit = 0;

int
_cmd_init(int argc, char** argv, void* context, const char* catalog, int flags)
{
	register char*	cp;

	if (argc < 0)
	{
		_cmd_quit = 1;
		return -1;
	}
	_cmd_quit = 0;
	if (cp = strrchr(argv[0], '/'))
		cp++;
	else
		cp = argv[0];
	error_info.id = cp;
	if (!error_info.catalog)
		error_info.catalog = catalog;
	opt_info.index = 0;
	if (context)
		error_info.flags |= flags;
	return 0;
}

#if __OBSOLETE__ < 20080101

#if defined(__EXPORT__)
#define extern	__EXPORT__
#endif

#undef	cmdinit

extern void
cmdinit(char** argv, void* context, const char* catalog, int flags)
{
	_cmd_init(0, argv, context, catalog, flags);
}

#endif

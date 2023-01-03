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
 * setlocale() intercept
 * maintains a bitmask of non-default categories
 * and a permanent locale namespace for pointer comparison
 * and persistent private data for locale related functions
 */

#include <ast_standards.h>

#include "lclib.h"

#include <ast_wchar.h>
#include <ctype.h>
#include <mc.h>
#include <namval.h>

#if ( _lib_wcwidth || _lib_wctomb ) && _hdr_wctype
#include <wctype.h>
#endif

#if _lib_wcwidth
#undef	wcwidth
#else
#define wcwidth			0
#endif

#if _lib_wctomb
#undef	wctomb
#else
#define wctomb			0
#endif

#ifdef mblen
#undef	mblen
extern int		mblen(const char*, size_t);
#endif

#undef	mbtowc
#undef	setlocale
#undef	strcmp
#undef	strcoll
#undef	strxfrm
#undef	valid

#ifndef AST_LC_CANONICAL
#define AST_LC_CANONICAL	LC_abbreviated
#endif

#if _UWIN

#include <ast_windows.h>

#undef	_lib_setlocale
#define _lib_setlocale		1

#define setlocale(c,l)		native_setlocale(c,l)

extern char*			uwin_setlocale(int, const char*);

/*
 * convert locale to native locale name in buf
 */

static char*
native_locale(const char* locale, char* buf, size_t siz)
{
	Lc_t*				lc;
	const Lc_attribute_list_t*	ap;
	int				i;
	unsigned long			lcid;
	unsigned long			lang;
	unsigned long			ctry;
	char				lbuf[128];
	char				cbuf[128];

	if (locale && *locale)
	{
		if (!(lc = lcmake(locale)))
			return 0;
		lang = lc->language->index;
		ctry = 0;
		for (ap = lc->attributes; ap; ap = ap->next)
			if (ctry = ap->attribute->index)
				break;
		if (!ctry)
		{
			for (i = 0; i < elementsof(lc->territory->languages); i++)
				if (lc->territory->languages[i] == lc->language)
				{
					ctry = lc->territory->indices[i];
					break;
				}
			if (!ctry)
				ctry = SUBLANG_DEFAULT;
		}
		lcid = MAKELCID(MAKELANGID(lang, ctry), SORT_DEFAULT);
	}
	else
		lcid = GetUserDefaultLCID();
	if (GetLocaleInfo(lcid, LOCALE_SENGLANGUAGE, lbuf, sizeof(lbuf)) <= 0 ||
	    GetLocaleInfo(lcid, LOCALE_SENGCOUNTRY, cbuf, sizeof(cbuf)) <= 0)
		return 0;
	if (lc->charset->ms)
		sfsprintf(buf, siz, "%s_%s.%s", lbuf, cbuf, lc->charset->ms);
	else
		sfsprintf(buf, siz, "%s_%s", lbuf, cbuf);
	return buf;
}

/*
 * locale!=0 here
 */

static char*
native_setlocale(int category, const char* locale)
{
	char*		usr;
	char*		sys;
	char		buf[256];

	/*
	 * win32 doesn't have LC_MESSAGES
	 */

	if (category == LC_MESSAGES)
		return (char*)locale;
	if (!(usr = native_locale(locale, buf, sizeof(buf))))
		return 0;
	sys = uwin_setlocale(category, usr);
	if (ast.locale.set & AST_LC_debug)
		sfprintf(sfstderr, "locale uwin %17s %-24s %-24s\n", categories[lcindex(category, 0)].name, usr, sys);
	return sys;
}

#else

#define native_locale(a,b,c)	((char*)0)

#endif

/*
 * LC_COLLATE and LC_CTYPE native support
 */

#if !_lib_mbtowc || MB_LEN_MAX <= 1
#define mblen		0
#define mbtowc		0
#endif

#if !_lib_strcoll
#define	strcoll		0
#endif

#if !_lib_strxfrm
#define	strxfrm		0
#endif

/*
 * LC_COLLATE and LC_CTYPE debug support
 *
 * mutibyte debug encoding
 *
 *	DL0 [ '0' .. '4' ] c1 ... c4 DR0
 *	DL1 [ '0' .. '4' ] c1 ... c4 DR1
 *
 * with these ligatures
 *
 *	ch CH sst SST
 *
 * and private collation order
 *
 * wide character display width is the low order 3 bits
 * wctomb() uses DL1...DR1
 */

#define DEBUG_MB_CUR_MAX	7

#if DEBUG_MB_CUR_MAX < MB_LEN_MAX
#undef	DEBUG_MB_CUR_MAX
#define DEBUG_MB_CUR_MAX	MB_LEN_MAX
#endif

#define DL0	'<'
#define DL1	0xab		/* 8-bit mini << on xterm	*/
#define DR0	'>'
#define DR1	0xbb		/* 8-bit mini >> on xterm	*/

#define DB	((int)sizeof(wchar_t)*8-1)
#define DC	7		/* wchar_t embedded char bits	*/
#define DX	(DB/DC)		/* wchar_t max embedded chars	*/
#define DZ	(DB-DX*DC+1)	/* wchar_t embedded size bits	*/
#define DD	3		/* # mb delimiter chars <n...>	*/

static unsigned char debug_order[] =
{
	  0,   1,   2,   3,   4,   5,   6,   7,
	  8,   9,  10,  11,  12,  13,  14,  15,
	 16,  17,  18,  19,  20,  21,  22,  23,
	 24,  25,  26,  27,  28,  29,  30,  31,
	 99, 100, 101, 102,  98, 103, 104, 105,
	106, 107, 108,  43, 109,  44,  42, 110,
	 32,  33,  34,  35,  36,  37,  38,  39,
	 40,  41, 111, 112, 113, 114, 115, 116,
	117,  71,  72,  73,  74,  75,  76,  77,
	 78,  79,  80,  81,  82,  83,  84,  85,
	 86,  87,  88,  89,  90,  91,  92,  93,
	 94,  95,  96, 118, 119, 120, 121,  97,
	122,  45,  46,  47,  48,  49,  50,  51,
	 52,  53,  54,  55,  56,  57,  58,  59,
	 60,  61,  62,  63,  64,  65,  66,  67,
	 68,  69,  70, 123, 124, 125, 126, 127,
	128, 129, 130, 131, 132, 133, 134, 135,
	136, 137, 138, 139, 140, 141, 142, 143,
	144, 145, 146, 147, 148, 149, 150, 151,
	152, 153, 154, 155, 156, 157, 158, 159,
	160, 161, 162, 163, 164, 165, 166, 167,
	168, 169, 170, 171, 172, 173, 174, 175,
	176, 177, 178, 179, 180, 181, 182, 183,
	184, 185, 186, 187, 188, 189, 190, 191,
	192, 193, 194, 195, 196, 197, 198, 199,
	200, 201, 202, 203, 204, 205, 206, 207,
	208, 209, 210, 211, 212, 213, 214, 215,
	216, 217, 218, 219, 220, 221, 222, 223,
	224, 225, 226, 227, 228, 229, 230, 231,
	232, 233, 234, 235, 236, 237, 238, 239,
	240, 241, 242, 243, 244, 245, 246, 247,
	248, 249, 250, 251, 252, 253, 254, 255,
};

static int
debug_mbtowc(register wchar_t* p, register const char* s, size_t n)
{
	register const char*	q;
	register const char*	r;
	register int		w;
	register int		dr;
	wchar_t			c;

	if (n < 1)
		return -1;
	if (!s || !*s)
		return 0;
	switch (((unsigned char*)s)[0])
	{
	case DL0:
		dr = DR0;
		break;
	case DL1:
		dr = DR1;
		break;
	default:
		if (p)
			*p = ((unsigned char*)s)[0] & ((1<<DC)-1);
		return 1;
	}
	if (n < 2)
		return -1;
	if ((w = ((unsigned char*)s)[1]) == ((unsigned char*)s)[0])
	{
		if (p)
			*p = w;
		return 2;
	}
	if (w < '0' || w > ('0' + DX))
		return -1;
	if ((w -= '0' - DD) > n)
		return -1;
	r = s + w - 1;
	q = s += 2;
	while (q < r && *q)
		q++;
	if (q != r || *((unsigned char*)q) != dr)
		return -1;
	if (p)
	{
		c = 0;
		while (--q >= s)
		{
			c <<= DC;
			c |= *((unsigned char*)q);
		}
		c <<= DZ;
		c |= w - DD;
		*p = c;
	}
	return w;
}

static int
debug_wctomb(char* s, wchar_t c)
{
	int	w;
	int	i;
	int	k;

	w = 0;
	if (c >= 0 && c <= UCHAR_MAX)
	{
		w++;
		if (s)
			*s = c;
	}
	else if ((i = c & ((1<<DZ)-1)) > DX)
		return -1;
	else
	{
		w++;
		if (s)
			*s++ = DL1;
		c >>= DZ;
		w++;
		if (s)
			*s++ = i + '0';
		while (i--)
		{
			w++;
			if (s)
				*s++ = (k = c & ((1<<DC)-1)) ? k : '?';
			c >>= DC;
		}
		w++;
		if (s)
			*s++ = DR1;
	}
	return w;
}

static int
debug_mblen(const char* s, size_t n)
{
	return debug_mbtowc(NiL, s, n);
}

static int
debug_wcwidth(wchar_t c)
{
	if (c >= 0 && c <= UCHAR_MAX)
		return 1;
	if ((c &= ((1<<DZ)-1)) > DX)
		return -1;
	return c + DD;
}

static size_t
debug_strxfrm(register char* t, register const char* s, size_t n)
{
	register const char*	q;
	register const char*	r;
	register char*		e;
	register size_t		z;
	register int		w;

	z = 0;
	if (e = t)
		e += n;
	while (s[0])
	{
		if ((((unsigned char*)s)[0] == DL0 || ((unsigned char*)s)[0] == DL1) && (w = s[1]) >= '0' && w <= ('0' + DC))
		{
			w -= '0';
			q = s + 2;
			r = q + w;
			while (q < r && *q)
				q++;
			if (*((unsigned char*)q) == DR0 || *((unsigned char*)q) == DR1)
			{
				if (t)
				{
					for (q = s + 2; q < r; q++)
						if (t < e)
							*t++ = debug_order[*q];
					while (w++ < DX)
						if (t < e)
							*t++ = 1;
				}
				s = r + 1;
				z += DX;
				continue;
			}
		}
		if ((s[0] == 'c' || s[0] == 'C') && (s[1] == 'h' || s[1] == 'H'))
		{
			if (t)
			{
				if (t < e)
					*t++ = debug_order[s[0]];
				if (t < e)
					*t++ = debug_order[s[1]];
				if (t < e)
					*t++ = 1;
				if (t < e)
					*t++ = 1;
			}
			s += 2;
			z += DX;
			continue;
		}
		if ((s[0] == 's' || s[0] == 'S') && (s[1] == 's' || s[1] == 'S') && (s[2] == 't' || s[2] == 'T'))
		{
			if (t)
			{
				if (t < e)
					*t++ = debug_order[s[0]];
				if (t < e)
					*t++ = debug_order[s[1]];
				if (t < e)
					*t++ = debug_order[s[2]];
				if (t < e)
					*t++ = 1;
			}
			s += 3;
			z += DX;
			continue;
		}
		if (t)
		{
			if (t < e)
				*t++ = debug_order[s[0]];
			if (t < e)
				*t++ = 1;
			if (t < e)
				*t++ = 1;
			if (t < e)
				*t++ = 1;
		}
		s++;
		z += DX;
	}
	if (t < e)
		*t = 0;
	return z;
}

static int
debug_strcoll(const char* a, const char* b)
{
	char	ab[1024];
	char	bb[1024];

	debug_strxfrm(ab, a, sizeof(ab) - 1);
	ab[sizeof(ab)-1] = 0;
	debug_strxfrm(bb, b, sizeof(bb) - 1);
	bb[sizeof(bb)-1] = 0;
	return strcmp(ab, bb);
}

/*
 * default locale
 */

static int
default_wcwidth(wchar_t w)
{
	return w >= 0 && w <= 255 && !iscntrl(w) ? 1 : -1;
}

/*
 * called when LC_COLLATE initialized or changes
 */

static int
set_collate(Lc_category_t* cp)
{
	if (locales[cp->internal]->flags & LC_debug)
	{
		ast.collate = debug_strcoll;
		ast.mb_xfrm = debug_strxfrm;
	}
	else if (locales[cp->internal]->flags & LC_default)
	{
		ast.collate = strcmp;
		ast.mb_xfrm = 0;
	}
	else
	{
		ast.collate = strcoll;
		ast.mb_xfrm = strxfrm;
	}
	return 0;
}

/*
 * called when LC_CTYPE initialized or changes
 */

static int
set_ctype(Lc_category_t* cp)
{
	if (locales[cp->internal]->flags & LC_debug)
	{
		ast.mb_cur_max = DEBUG_MB_CUR_MAX;
		ast.mb_len = debug_mblen;
		ast.mb_towc = debug_mbtowc;
		ast.mb_width = debug_wcwidth;
		ast.mb_conv = debug_wctomb;
	}
	else if ((locales[cp->internal]->flags & LC_default) || (ast.mb_cur_max = MB_CUR_MAX) <= 1 || !(ast.mb_len = mblen) || !(ast.mb_towc = mbtowc))
	{
		ast.mb_cur_max = 1;
		ast.mb_len = 0;
		ast.mb_towc = 0;
		ast.mb_width = default_wcwidth;
		ast.mb_conv = 0;
	}
	else
	{
		if (!(ast.mb_width = wcwidth))
			ast.mb_width = default_wcwidth;
		ast.mb_conv = wctomb;
	}
	return 0;
}

/*
 * called when LC_NUMERIC initialized or changes
 */

static int
set_numeric(Lc_category_t* cp)
{
	register int		category = cp->internal;
	struct lconv*		lp;
	Lc_numeric_t*		dp;

	static Lc_numeric_t	default_numeric = { '.', -1 };

	if (!LCINFO(category)->data)
	{
		if ((lp = localeconv()) && (dp = newof(0, Lc_numeric_t, 1, 0)))
		{
			dp->decimal = lp->decimal_point && *lp->decimal_point ? *(unsigned char*)lp->decimal_point : '.';
			dp->thousand = lp->thousands_sep && *lp->thousands_sep ? *(unsigned char*)lp->thousands_sep : -1;
		}
		else
			dp = &default_numeric;
		LCINFO(category)->data = (void*)dp;
		if (ast.locale.set & (AST_LC_debug|AST_LC_setlocale))
			sfprintf(sfstderr, "locale info %17s decimal '%c' thousands '%c'\n", categories[category].name, dp->decimal, dp->thousand >= 0 ? dp->thousand : 'X');
	}
	return 0;
}

/*
 * this table is indexed by AST_LC_[A-Z]*
 */

Lc_category_t		categories[] =
{
{ "LC_ALL",           LC_ALL,           AST_LC_ALL,           0               },
{ "LC_COLLATE",       LC_COLLATE,       AST_LC_COLLATE,       set_collate     },
{ "LC_CTYPE",         LC_CTYPE,         AST_LC_CTYPE,         set_ctype       },
{ "LC_MESSAGES",      LC_MESSAGES,      AST_LC_MESSAGES,      0               },
{ "LC_MONETARY",      LC_MONETARY,      AST_LC_MONETARY,      0               },
{ "LC_NUMERIC",       LC_NUMERIC,       AST_LC_NUMERIC,       set_numeric     },
{ "LC_TIME",          LC_TIME,          AST_LC_TIME,          0               },
{ "LC_IDENTIFICATION",LC_IDENTIFICATION,AST_LC_IDENTIFICATION,0               },
{ "LC_ADDRESS",       LC_ADDRESS,       AST_LC_ADDRESS,       0               },
{ "LC_NAME",          LC_NAME,          AST_LC_NAME,          0               },
{ "LC_TELEPHONE",     LC_TELEPHONE,     AST_LC_TELEPHONE,     0               },
{ "LC_XLITERATE",     LC_XLITERATE,     AST_LC_XLITERATE,     0               },
{ "LC_MEASUREMENT",   LC_MEASUREMENT,   AST_LC_MEASUREMENT,   0               },
{ "LC_PAPER",         LC_PAPER,         AST_LC_PAPER,         0               },
};

static const Namval_t	options[] =
{
	"debug",		AST_LC_debug,
	"find",			AST_LC_find,
	"setlocale",		AST_LC_setlocale,
	"translate",		AST_LC_translate,
	0,			0
};

/*
 * called by stropt() to set options
 */

static int
setopt(void* a, const void* p, int n, const char* v)
{
	if (p)
	{
		if (n)
			ast.locale.set |= ((Namval_t*)p)->value;
		else
			ast.locale.set &= ~((Namval_t*)p)->value;
	}
	return 0;
}

#if !_lib_setlocale

#define setlocale(c,l)		default_setlocale(c,l)

static char*
default_setlocale(int category, const char* locale)
{
	Lc_t*		lc;

	if (locale)
	{
		if (!(lc = lcmake(locale)) || !(lc->flags & LC_default))
			return 0;
		locales[0]->flags &= ~lc->flags;
		locales[1]->flags &= ~lc->flags;
		return lc->name;
	}
	return (locales[1]->flags & (1<<category)) ? locales[1]->name : locales[0]->name;
}

#endif

/*
 * set a single AST_LC_* locale category
 * the caller must validate category
 * lc==0 restores the previous state
 */

static char*
single(int category, Lc_t* lc)
{
	const char*	sys;
	int		i;

	if (!lc && !(lc = categories[category].prev))
		lc = lcmake(NiL);
	if (locales[category] != lc)
	{
		if (categories[category].external == -categories[category].internal)
		{
			sys = 0;
			for (i = 1; i < AST_LC_COUNT; i++)
				if (locales[i] == lc)
				{
					sys = (char*)lc->name;
					break;
				}
		}
		else if (lc->flags & (LC_debug|LC_local))
			sys = setlocale(categories[category].external, lcmake(NiL)->name);
		else if (!(sys = setlocale(categories[category].external, lc->name)) &&
			 (streq(lc->name, lc->code) || !(sys = setlocale(categories[category].external, lc->code))) &&
			 !streq(lc->code, lc->language->code))
				sys = setlocale(categories[category].external, lc->language->code);
		if (ast.locale.set & (AST_LC_debug|AST_LC_setlocale))
			sfprintf(sfstderr, "locale set  %17s %-24s %-24s\n", categories[category].name, lc->name, sys);
		if (!sys)
		{
			/*
			 * check for local override
			 * currently this means an LC_MESSAGES dir exists
			 */

			if (!(lc->flags & LC_checked))
			{
				char	path[PATH_MAX];

				if (mcfind(path, lc->code, NiL, LC_MESSAGES, 0))
					lc->flags |= LC_local;
				lc->flags |= LC_checked;
			}
			if (!(lc->flags & LC_local))
				return 0;
			if (categories[category].external != -categories[category].internal)
				setlocale(categories[category].external, lcmake(NiL)->name);
		}
		locales[category] = lc;
		if (categories[category].setf && (*categories[category].setf)(&categories[category]))
		{
			locales[category] = categories[category].prev;
			return 0;
		}
		if (lc->flags & LC_default)
			ast.locale.set &= ~(1<<category);
		else
			ast.locale.set |= (1<<category);
	}
	return (char*)lc->name;
}

/*
 * set composite AST_LC_ALL locale categories
 * return <0:composite-error 0:not-composite >0:composite-ok
 */

static int
composite(register const char* s, int initialize)
{
	register const char*	t;
	register int		i;
	register int		j;
	register int		k;
	int			n;
	const char*		w;
	Lc_t*			p;
	int			cat[AST_LC_COUNT];
	int			stk[AST_LC_COUNT];
	char			buf[PATH_MAX / 2];

	k = n = 0;
	while (s[0] == 'L' && s[1] == 'C' && s[2] == '_')
	{
		n++;
		j = 0;
		w = s;
		for (i = 1; i < AST_LC_COUNT; i++)
		{
			s = w;
			t = categories[i].name;
			while (*t && *s++ == *t++);
			if (!*t && *s++ == '=')
			{
				cat[j++] = i;
				if (s[0] != 'L' || s[1] != 'C' || s[2] != '_')
					break;
				w = s;
				i = -1;
			}
		}
		for (s = w; *s && *s != '='; s++);
		if (!*s)
		{
			for (i = 0; i < k; i++)
				single(stk[i], NiL);
			return -1;
		}
		w = ++s;
		for (;;)
		{
			if (!*s)
			{
				p = lcmake(w);
				break;
			}
			else if (*s++ == ';')
			{
				if ((n = s - w - 1) >= sizeof(buf))
					n = sizeof(buf) - 1;
				memcpy(buf, w, n);
				buf[n] = 0;
				p = lcmake(buf);
				break;
			}
		}
		for (i = 0; i < j; i++)
			if (!initialize)
			{
				if (!single(cat[i], p))
				{
					for (i = 0; i < k; i++)
						single(stk[i], NiL);
					return -1;
				}
				stk[k++] = cat[i];
			}
			else if (!categories[cat[i]].prev)
				categories[cat[i]].prev = p;
	}
	while (s[0] == '/' && s[1] && n < AST_LC_COUNT)
	{
		n++;
		for (w = ++s; *s && *s != '/'; s++);
		if (!*s)
			p = lcmake(w);
		else
		{
			if ((j = s - w - 1) >= sizeof(buf))
				j = sizeof(buf) - 1;
			memcpy(buf, w, j);
			buf[j] = 0;
			p = lcmake(buf);
		}
		if (!initialize)
		{
			if (!single(n, p))
			{
				for (i = 1; i < n; i++)
					single(i, NiL);
				return -1;
			}
		}
		else if (!categories[n].prev)
			categories[n].prev = p;
	}
	return n;
}

/*
 * setlocale() intercept
 */

char*
_ast_setlocale(int category, const char* locale)
{
	register char*		s;
	register int		i;
	register int		j;
	int			k;
	char*			a;
	Lc_t*			p;
	int			cat[AST_LC_COUNT];

	static Sfio_t*		sp;
	static int		initialized;
	static char		local[] = "local";

	if ((category = lcindex(category, 0)) < 0)
		return 0;
	if (!locale)
	{
		/*
		 * return the current state
		 */

	compose:
		if (category != AST_LC_ALL)
			return (char*)locales[category]->name;
		if (!sp && !(sp = sfstropen()))
			return 0;
		for (i = 1; i < AST_LC_COUNT; i++)
			cat[i] = -1;
		for (i = 1, k = 0; i < AST_LC_COUNT; i++)
			if (cat[i] < 0)
			{
				k++;
				cat[i] = i;
				for (j = i + 1; j < AST_LC_COUNT; j++)
					if (locales[j] == locales[i])
						cat[j] = i;
			}
		if (k == 1)
			return (char*)locales[1]->name;
		for (i = 1; i < AST_LC_COUNT; i++)
			if (cat[i] >= 0 && !(locales[i]->flags & LC_default))
			{
				if (sfstrtell(sp))
					sfprintf(sp, ";");
				for (j = i, k = cat[i]; j < AST_LC_COUNT; j++)
					if (cat[j] == k)
					{
						cat[j] = -1;
						sfprintf(sp, "%s=", categories[j].name);
					}
				sfprintf(sp, "%s", locales[i]->name);
			}
		if (!sfstrtell(sp))
			return (char*)locales[0]->name;
		return sfstruse(sp);
	}
	if (!ast.locale.serial++)
		stropt(getenv("LC_OPTIONS"), options, sizeof(*options), setopt, NiL);
	if (!*locale)
	{
		if (!initialized)
		{
			char*	u;
			char	tmp[256];

			/*
			 * initialize from the environment
			 * precedence determined by X/Open
			 */

			u = 0;
			if (!(a = getenv("LC_ALL")) || !*a)
			{
				for (i = 1; i < AST_LC_COUNT; i++)
					if ((s = getenv(categories[i].name)) && *s)
					{
						if (streq(s, local) && (u || (u = native_locale(locale, tmp, sizeof(tmp)))))
							s = u;
						categories[i].prev = lcmake(s);
					}
				a = getenv("LANG");
			}
			if (a)
			{
				if (streq(a, local) && (u || (u = native_locale(locale, tmp, sizeof(tmp)))))
					a = u;
				if (composite(a, 1))
					a = 0;
			}
			p = 0;
			for (i = 1; i < AST_LC_COUNT; i++)
			{
				if (!categories[i].prev)
				{
					if (!p && !(p = lcmake(a)))
						break;
					categories[i].prev = p;
				}
				if (!single(i, categories[i].prev))
				{
					while (i--)
						single(i, NiL);
					return 0;
				}
			}
			if (ast.locale.set & AST_LC_debug)
				for (i = 1; i < AST_LC_COUNT; i++)
					sfprintf(sfstderr, "locale env  %17s %s\n", categories[i].name, locales[i]->name);
			initialized = 1;
		}
		goto compose;
	}
	else if (category != AST_LC_ALL)
		return single(category, lcmake(locale));
	else if (!(i = composite(locale, 0)))
	{
		if (!(p = lcmake(locale)))
			return 0;
		for (i = 1; i < AST_LC_COUNT; i++)
			if (!single(i, p))
			{
				while (i--)
					single(i, NiL);
				return 0;
			}
	}
	else if (i < 0)
		return 0;
	goto compose;
}

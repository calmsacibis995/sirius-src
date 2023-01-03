/*
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"

/*
 * /src/NTP/REPOSITORY/v4/libparse/clk_schmid.c,v 3.22 1997/01/19 12:44:41 kardel Exp
 *  
 * clk_schmid.c,v 3.22 1997/01/19 12:44:41 kardel Exp
 *
 * Schmid clock support
 *
 * Copyright (C) 1992,1993,1994,1995,1996 by Frank Kardel
 * Friedrich-Alexander Universit�t Erlangen-N�rnberg, Germany
 *                                    
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(REFCLOCK) && (defined(PARSE) || defined(PARSEPPS)) && defined(CLOCK_SCHMID)

#include <sys/types.h>
#include <sys/time.h>
#include <sys/errno.h>
#include "ntp_fp.h"
#include "ntp_unixtime.h"
#include "ntp_calendar.h"

#include "parse.h"

/*
 * Description courtesy of Adam W. Feigin et. al (Swisstime iis.ethz.ch)
 *
 * The command to Schmid's DCF77 clock is a single byte; each bit
 * allows the user to select some part of the time string, as follows (the
 * output for the lsb is sent first).
 * 
 * Bit 0:	time in MEZ, 4 bytes *binary, not BCD*; hh.mm.ss.tenths
 * Bit 1:	date 3 bytes *binary, not BCD: dd.mm.yy
 * Bit 2:	week day, 1 byte (unused here)
 * Bit 3:	time zone, 1 byte, 0=MET, 1=MEST. (unused here)
 * Bit 4:	clock status, 1 byte,	0=time invalid,
 *					1=time from crystal backup,
 *					3=time from DCF77
 * Bit 5:	transmitter status, 1 byte,
 *					bit 0: backup antenna
 *					bit 1: time zone change within 1h
 *					bit 3,2: TZ 01=MEST, 10=MET
 *					bit 4: leap second will be
 *						added within one hour
 *					bits 5-7: Zero
 * Bit 6:	time in backup mode, units of 5 minutes (unused here)
 *
 */
#define WS_TIME		0x01
#define WS_SIGNAL	0x02

#define WS_ALTERNATE	0x01
#define WS_ANNOUNCE	0x02
#define WS_TZ		0x0c
#define   WS_MET	0x08
#define   WS_MEST	0x04
#define WS_LEAP		0x10

static u_long cvt_schmid P((char *, unsigned int, void *, clocktime_t *, void *));

clockformat_t clock_schmid =
{
  NULL,				/* no input handling */
  cvt_schmid,			/* Schmid conversion */
  syn_simple,			/* easy time stamps */
  NULL,				/* not direct PPS monitoring */
  NULL,				/* no time code synthesizer monitoring */
  (void *)0,			/* conversion configuration */
  "Schmid",			/* Schmid receiver */
  12,				/* binary data buffer */
  F_END|SYNC_START,		/* END packet delimiter / synchronisation */
  0,				/* no private data (complete messages) */
  { 0, 0},
  '\0',
  (unsigned char)'\375',
  '\0'
};


static u_long
cvt_schmid(bp, size, vf, clock, vt)
  register char 	 *bp;
  register unsigned int   size;
  register void 	 *vf;
  register clocktime_t   *clock;
  register void 	 *vt;
{
  register unsigned char *buffer = (unsigned char *) bp;
  if ((size != 11) || (buffer[10] != (unsigned char)'\375'))
    {
      return CVT_NONE;
    }
  else
    {
      if (buffer[0] > 23 || buffer[1] > 59 || buffer[2] > 59 || buffer[3] >  9) /* Time */
	{
	  return CVT_FAIL|CVT_BADTIME;
	}
      else
	if (buffer[4] <  1 || buffer[4] > 31 || buffer[5] <  1 || buffer[5] > 12
	    ||  buffer[6] > 99)
	  {
	    return CVT_FAIL|CVT_BADDATE;
	  }
	else
	  {
	    clock->hour    = buffer[0];
	    clock->minute  = buffer[1];
	    clock->second  = buffer[2];
	    clock->usecond = buffer[3] * 100000;
	    clock->day     = buffer[4];
	    clock->month   = buffer[5];
	    clock->year    = buffer[6];

	    clock->flags   = 0;

	    switch (buffer[8] & WS_TZ)
	      {
	      case WS_MET:
		clock->utcoffset = -1*60*60;
		break;

	      case WS_MEST:
		clock->utcoffset = -2*60*60;
		clock->flags    |= PARSEB_DST;
		break;

	      default:
		return CVT_FAIL|CVT_BADFMT;
	      }
	  
	    if (!(buffer[7] & WS_TIME))
	      {
		clock->flags |= PARSEB_POWERUP;
	      }

	    if (!(buffer[7] & WS_SIGNAL))
	      {
		clock->flags |= PARSEB_NOSYNC;
	      }

	    if (buffer[7] & WS_SIGNAL)
	      {
		if (buffer[8] & WS_ALTERNATE)
		  {
		    clock->flags |= PARSEB_ALTERNATE;
		  }

		if (buffer[8] & WS_ANNOUNCE)
		  {
		    clock->flags |= PARSEB_ANNOUNCE;
		  }

		if (buffer[8] & WS_LEAP)
		  {
		    clock->flags |= PARSEB_LEAPADD; /* default: DCF77 data format deficiency */
		  }
	      }

	    clock->flags |= PARSEB_S_LEAP|PARSEB_S_ANTENNA;
	  
	    return CVT_OK;
	  }
    }
}

#else /* not (REFCLOCK && (PARSE || PARSEPPS) && CLOCK_SCHMID) */
int clk_schmid_bs;
#endif /* not (REFCLOCK && (PARSE || PARSEPPS) && CLOCK_SCHMID) */

/*
 * History:
 *
 * clk_schmid.c,v
 * Revision 3.22  1997/01/19 12:44:41  kardel
 * 3-5.88.1 reconcilation
 *
 * Revision 3.21  1996/12/01 16:04:15  kardel
 * freeze for 5.86.12.2 PARSE-Patch
 *
 * Revision 3.20  1996/11/24 20:09:46  kardel
 * RELEASE_5_86_12_2 reconcilation
 *
 * Revision 3.19  1996/10/05 13:30:21  kardel
 * general update
 *
 * Revision 3.18  1994/10/03  21:59:26  kardel
 * 3.4e cleanup/integration
 *
 * Revision 3.17  1994/10/03  10:04:09  kardel
 * 3.4e reconcilation
 *
 * Revision 3.16  1994/05/30  10:20:03  kardel
 * LONG cleanup
 *
 * Revision 3.15  1994/05/12  12:34:48  kardel
 * data type cleanup
 *
 * Revision 3.14  1994/04/12  14:56:31  kardel
 * fix declaration
 *
 * Revision 3.13  1994/02/20  13:04:41  kardel
 * parse add/delete second support
 *
 * Revision 3.12  1994/02/02  17:45:25  kardel
 * rcs ids fixed
 *
 * Revision 3.10  1994/01/25  19:05:15  kardel
 * 94/01/23 reconcilation
 *
 * Revision 3.9  1994/01/23  17:21:56  kardel
 * 1994 reconcilation
 *
 * Revision 3.8  1993/11/01  20:00:18  kardel
 * parse Solaris support (initial version)
 *
 * Revision 3.7  1993/10/30  09:44:43  kardel
 * conditional compilation flag cleanup
 *
 * Revision 3.6  1993/10/09  15:01:32  kardel
 * file structure unified
 *
 * Revision 3.5  1993/10/03  19:10:47  kardel
 * restructured I/O handling
 *
 * Revision 3.4  1993/09/27  21:08:09  kardel
 * utcoffset now in seconds
 *
 * Revision 3.3  1993/09/26  23:40:27  kardel
 * new parse driver logic
 *
 * Revision 3.2  1993/07/09  11:37:19  kardel
 * Initial restructured version + GPS support
 *
 * Revision 3.1  1993/07/06  10:00:22  kardel
 * DCF77 driver goes generic...
 *
 */

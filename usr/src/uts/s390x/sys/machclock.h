/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License                  
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 *                                                              
 * Copyright 2008 Sine Nomine Associates.                        
 * All rights reserved.                                           
 * Use is subject to license terms.                                
 */
/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef _SYS_MACHCLOCK_H
#define	_SYS_MACHCLOCK_H

#ifdef	__cplusplus
extern "C" {
#endif

#if defined(CPU_MODULE)

/*
 * Constants used to convert hi-res timestamps into microseconds
 * (see <sys/clock.h> file for more information)
 */

#define	NSEC_SHIFT	13
#define	VTRACE_SHIFT	13

#endif /* CPU_MODULE */

#ifndef _ASM

#ifdef	_KERNEL

/*
 * Hardware watchdog variables and knobs
 */

#define	CLK_WATCHDOG_DEFAULT	10	/* 10 seconds */

extern int	watchdog_enable;
extern int	watchdog_available;
extern int	watchdog_activated;
extern uint_t	watchdog_timeout_seconds;

/*
 * TOD clock operations
 */
timestruc_t	tod_get(void);
void		tod_set(timestruc_t);
uint_t		tod_set_watchdog_timer(uint_t);
uint_t		tod_clear_watchdog_timer(void);
void		tod_set_power_alarm(timestruc_t);
void		tod_clear_power_alarm(void);
uint64_t	tod_get_cpufrequency(void);

#endif	/* _KERNEL */

#endif	/* _ASM */

#ifdef	__cplusplus
}
#endif

#endif	/* !_SYS_MACHCLOCK_H */

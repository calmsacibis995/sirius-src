/*------------------------------------------------------------------*/
/* 								    */
/* Name        - sclp.c     					    */
/* 								    */
/* Function    - Handle communications with the service processor.  */
/* 								    */
/* Name	       - Neale Ferguson					    */
/* 								    */
/* Date        - July, 2006  					    */
/* 								    */
/*------------------------------------------------------------------*/

/*==================================================================*/
/* 								    */
/* CDDL HEADER START						    */
/* 								    */
/* The contents of this file are subject to the terms of the	    */
/* Common Development and Distribution License                      */
/* (the "License").  You may not use this file except in compliance */
/* with the License.						    */
/* 								    */
/* You can obtain a copy of the license at: 			    */
/* - usr/src/OPENSOLARIS.LICENSE, or,				    */
/* - http://www.opensolaris.org/os/licensing.			    */
/* See the License for the specific language governing permissions  */
/* and limitations under the License.				    */
/* 								    */
/* When distributing Covered Code, include this CDDL HEADER in each */
/* file and include the License file at usr/src/OPENSOLARIS.LICENSE.*/
/* If applicable, add the following below this CDDL HEADER, with    */
/* the fields enclosed by brackets "[]" replaced with your own      */
/* identifying information: 					    */
/* Portions Copyright [yyyy] [name of copyright owner]		    */
/* 								    */
/* CDDL HEADER END						    */
/*                                                                  */
/* Copyright 2008 Sine Nomine Associates.                           */
/* All rights reserved.                                             */
/* Use is subject to license terms.                                 */
/* 								    */
/*==================================================================*/

/*------------------------------------------------------------------*/
/*                 D e f i n e s                                    */
/*------------------------------------------------------------------*/

/*========================= End of Defines =========================*/

/*------------------------------------------------------------------*/
/*                 I n c l u d e s                                  */
/*------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/asm_linkage.h>
#include <sys/regset.h>
#include <sys/exts390x.h>
#include <sys/sclp.h>
#include <sys/errno.h>
#include <sys/machs390x.h>
#include <sys/mutex.h>
#include <sys/avintr.h>
#include <sys/intr.h>
#include <sys/ios390x.h>
#include <sys/disp.h>
#include <sys/kmem.h>
#include <sys/queue.h>
#include <sys/smp_impldefs.h>
#include <sys/varargs.h>
#include <sys/log.h>
#include <sys/taskq_impl.h>

/*========================= End of Includes ========================*/

/*------------------------------------------------------------------*/
/*                 T y p e d e f s                                  */
/*------------------------------------------------------------------*/

/*========================= End of Typedefs ========================*/

/*------------------------------------------------------------------*/
/*                   P r o t o t y p e s                            */
/*------------------------------------------------------------------*/

void msgnoh(const char *fmt, ...);
size_t sclp_read(char *buf, size_t bufSize);
void sclp_intr(uint_t intparm, uint_t intcode, uint_t subcode);

static void sclp_task(void *args);
static void sclp_shutdown(char *ptr);

/*========================= End of Prototypes ======================*/

/*------------------------------------------------------------------*/
/*                 E x t e r n a l   R e f e r e n c e s            */
/*------------------------------------------------------------------*/

extern void halt(char *);
extern char sccb_page[4096];

/*====================== External Refereences ======================*/

/*------------------------------------------------------------------*/
/*                 G l o b a l   V a r i a b l e s                  */
/*------------------------------------------------------------------*/

volatile uchar_t sclp_ecb = 0;
volatile uint32_t sclp_subcode = 0;
kmutex_t sclp_lock;
kcondvar_t sclp_cv;
ddi_taskq_t *sclp_tq = NULL;

static boolean_t sclp_wakeup = B_FALSE;
static char qdata[16364];
static int wndx = 0;
static int rndx = 0;

static sclp_req_t sclp_curreq;

static char msgbuf[256];
static size_t msgpos;

/*====================== End of Global Variables ===================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- msgnoh.                                           */
/*                                                                  */
/* Function	- A quick debug-related method for putting messages */
/*		  to the console without worrying about interrupts. */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
msgnoh(const char *fmt, ...)
{
	va_list val;
	int i;

	va_start(val, fmt);
	strcpy(msgbuf, "MSGNOH * AT * - ");
	msgpos = strlen(msgbuf);

	msgpos += vsnprintf(&msgbuf[msgpos],
			    sizeof(msgbuf) - msgpos,
			    fmt,
			    val);

	a2e(msgbuf,  msgpos);

	__asm__("	lgr	1,%0\n"
		"	lgr	3,%1\n"
		"	lghi	4,0\n"
		"	diag	1,3,0x8\n"
		:
		: "r" (msgbuf), "r" (msgpos)
		: "1", "2", "3", "4");
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- sclp_wait.                                        */
/*                                                                  */
/* Function	- Wait for a service processor operation to comp-   */
/*		  lete.                        		 	    */
/*		                               		 	    */
/* Note		- It may appear that this is a dreaded busy loop    */
/*		  (and it is), but the interrupt is almost always   */
/*                presented before this routine is even called so   */
/*                there's little to no looping involved.            */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static __inline__ void
sclp_wait() 
{
	drv_usecwait(1000);
//	while (sclp_ecb == 0);
	sclp_ecb = 0;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- servc.                                            */
/*                                                                  */
/* Function	- Issue the SERVC opcode.                           */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static __inline__ size_t
servc(sclp_req_t req, void *sclpcb)
{
	sccbhdr *hdr = (sccbhdr *)sclpcb;
	int cc;

	sclp_curreq = req;

	__asm__ __volatile__ ("	.insn	rre,0xb2200000,%1,%2\n"
			      "	ipm	%0\n"
			      " srl	%0,28\n"
			      : "=r" (cc)
			      : "r" (req), "r" (sclpcb)
			      : "cc");

	switch (cc) {
	case 0 :		// Operation started okay
		return 0;
	case 2 : 		// SCLP busy
		return -EBUSY;
	case 3 :		// Not operational
		return -EIO;
	}

	return -EINVAL;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- sclp_init.                                        */
/*                                                                  */
/* Function	- Set up SCLP communications.                       */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
sclp_init() 
{
	emsccb *ev = (emsccb *) &sccb_page;
	int rc;

	mutex_init(&sclp_lock, 0, MUTEX_SPIN, (void *)PIL_MAX);
	cv_init(&sclp_cv, NULL, CV_DRIVER, NULL); 

	/* Create task queue */
	sclp_tq = ddi_taskq_create(NULL,
				   "sclp_task_queue",
				   1,
				   TASKQ_DEFAULTPRI,
				   0);
	if (sclp_tq == NULL) {
		panic("Can't create SCLP task queue\n");
	}

	rc = ddi_taskq_dispatch(sclp_tq, sclp_task, NULL, DDI_SLEEP);
	if (rc != 0) {
		panic("Can't dispatch SCLP tash\n");
	}

	bzero(ev, sizeof(*ev));

	ev->hdr.len = sizeof(*ev);
	ev->cnt	    = sizeof(ev->mask[0]);
	ev->mask[0] = EVM_CMD | EVM_SGQ;
	ev->mask[1] = EVM_MSG | EVM_CPI;

	sclp_ecb   = 0;

	if (servc(SCP_WEVM, ev) == 0) {
		sclp_wait();
	}	
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- sclp_term.                                        */
/*                                                                  */
/* Function	- Terminate SCLP processing.                        */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
sclp_term() 
{
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- sclp_write.                                       */
/*                                                                  */
/* Function	- Queue messages to be written to the console.      */
/*		                               		 	    */
/*------------------------------------------------------------------*/

size_t
sclp_write(char *msg, size_t len) 
{
	int oldpil;
	int ndx;

	/* Deny invalid/zero length */
	if (len < 0) {
		return 0;
	}

	/* Single thread */
	mutex_enter(&sclp_lock);

	ndx = wndx;
	wndx += len + 1;
	if (wndx > sizeof(qdata)) {
		int i = 0;

		urgent("Interal SCLP buffer filled...dumping contents\n");

		while (i < ndx) {
			urgent(&qdata[i]);
			i += strlen(&qdata[i]) + 1;
		}
		rndx = 0;
		ndx = 0;
		wndx = len + 1;
	}

	bcopy(msg, &qdata[ndx], len);
	qdata[ndx + len] = '\0';

	sclp_wakeup = B_TRUE;

	if (sclp_tq != NULL) {
		cv_signal(&sclp_cv);
	}

	/* Allow others */
	mutex_exit(&sclp_lock);

	return (len);
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- sclp_write.                                       */
/*                                                                  */
/* Function	- Queue messages to be written to the console.      */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
sclp_task(void *args)
{
	wrsccb *wr_sccb;
	scmsgto *to;
	char *text;
	int need;
	char *msg;
	int len;
	int rc;
	int oldmask;
	int oldssg;
	clock_t waittime;

	/* Process until we're told to quit */
	for (;;) {

		/* Obtain lock */
		mutex_enter(&sclp_lock);

		/* Wait for more work */
		while (sclp_wakeup == B_FALSE) {

			/* Calc how long to wait */
			waittime = ddi_get_lbolt();
			if (rndx < wndx) {
				waittime += USEC_TO_TICK(250000);
			}
			else {
				waittime += USEC_TO_TICK(1000000);
			}
	
			/* Wait for a while */
			rc = cv_timedwait_sig(&sclp_cv,
					      &sclp_lock,
					      waittime);
		}

		/* Emit all queued messages */
		while (rndx < wndx) {
			msg = &qdata[rndx];
			len = strlen(&qdata[rndx]);
			rndx += len + 1;

			/* Calc lengths */
			need = sizeof(wrsccb) + sizeof(scmsgto);
			if (len + need > sizeof(sccb_page)) {
				len = sizeof(sccb_page) - need;
			}
			need += len;
	
			/* Calc ptrs to sections */
			wr_sccb = ((void *) &sccb_page);
			to = ((void *) wr_sccb) + sizeof(*wr_sccb);
			text = ((void *) to) + sizeof(*to);

			/* Write SCCB */
			wr_sccb->hdr.len = need;
	
			/* Event buffer */
			wr_sccb->msgbf.ev.evlen = sizeof(scmsgbf) +
						  sizeof(scmsgto) + len;
			wr_sccb->msgbf.ev.evtype = EVT_MSG;
		
			/* Message Data block */
			wr_sccb->msgbf.md.hdr.mdlen = sizeof(scmdbhd) +
						      sizeof(scmsggo) + 
						      sizeof(scmsgto) +
						      len;
			wr_sccb->msgbf.md.hdr.mdtag = MDBT_MDB;
			wr_sccb->msgbf.md.hdr.mdtype = 1;
			wr_sccb->msgbf.md.hdr.mdrev = 1;
		
			/* GDS */
			wr_sccb->msgbf.md.go.golen = sizeof(scmsggo);
			wr_sccb->msgbf.md.go.gotype = 1;
		
			/* Message To Operator */
			to->mtlen = sizeof(scmsgto) + len;
			to->mttype = GO_MTO;
			to->mtlflg = LT_ETX;
		
			/* Copy and convert to ebcdic */
			bcopy(msg, text, len);
			a2e(text, len);

			/* Send to console */
			mutex_exit(&sclp_lock);
			rc = servc(SCP_WEVD, sccb_page);
			if (rc == 0) {
				sclp_wait();
			}
			mutex_enter(&sclp_lock);

			/* Message wasn't displayed...requeue */
			if (rc != 0) {
				rndx = rndx - len - 1;
				break;
			}
		}

		/* Reset to empty queue */
		if (rndx >= wndx) {
			wndx = 0;
			rndx = 0;
		}

		/* Done for now */
		sclp_wakeup = B_FALSE;
		mutex_exit(&sclp_lock);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name        - sclp_read.                                         */
/*                                                                  */
/* Function    - Read messages from the hardware console via SCLP.  */
/*                                                                  */
/*------------------------------------------------------------------*/

size_t
sclp_read(char *buf, size_t bufSize)
{
	rdsccb	*rd_sccb;
	char	*text;
	size_t  rc,
		len;

	clear_page(sccb_page);
	rd_sccb = ((void *) &sccb_page);

	rd_sccb->hdr.len	= 4096;
	rd_sccb->hdr.mask[0]	= SCVLRSP;
	rc = servc(SCP_REVD, rd_sccb);

	if (rc < 0) 
		return (rc);
	else {
#if 0
		text = (char *) (rd_sccb + rd_sccb->ev.evlen);
		len  = rd_sccb->hdr.len - rd_sccb->ev.evlen;
		len  = (len > bufSize ? bufSize : len);
		bcopy(text, buf, len);
		e2a(buf, len);
		return (len);
#endif
	}

	return 0;
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- sclp_intr.                                        */
/*                                                                  */
/* Function	-						    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

void
sclp_intr(uint_t intparm, uint_t intcode, uint_t subcode)
{
	uintptr_t ptr;
	uintptr_t end;
	sccbhdr *hdr;

	ptr = ((uint64_t)intparm & ~0x03);
	hdr = (sccbhdr *) ptr;

	if (hdr != NULL) {
		end = ptr + hdr->len;
		if (sclp_curreq == SCP_REVD) {
			scevbuf *ev;
			scvecmj *mj;
			scvecmn	*mn;

			ptr += sizeof(sccbhdr);
			ev = (scevbuf *) ptr;

			while (ptr < end && ev->evlen > 0) {
				ptr += ev->evlen;

				if (ev->evtype == EVT_SGQ) {
					sclp_shutdown(((char *)ev) + sizeof(*ev));
				}
				
				ev = (scevbuf *) ptr;
			}
		}
	}
		
	if (intparm & 0x03) {
		sclp_read(NULL, 0);
	}
}

/*========================= End of Function ========================*/

/*------------------------------------------------------------------*/
/*                                                                  */
/* Name		- sclp_shutdown.                                    */
/*                                                                  */
/* Function	-						    */
/*		                               		 	    */
/*------------------------------------------------------------------*/

static void
sclp_shutdown(char *ptr)
{
	proc_t *initpp;
	uint_t seconds;

	seconds = (ptr[0] << 8) + ptr[1];
	printf("Shutdown signal received...shutdown must occur within %d seconds",
		seconds);

	/*
	 * If we're still booting and init(1) isn't set up yet, simply halt.
	 */
	mutex_enter(&pidlock);
	initpp = prfind(P_INITPID);
	mutex_exit(&pidlock);
	if (initpp == NULL) {
		halt("Power off the System");   /* just in case */
	}

	/*
	 * else, graceful shutdown with inittab and all getting involved
	 */
	psignal(initpp, SIGPWR);
}

/*========================= End of Function ========================*/


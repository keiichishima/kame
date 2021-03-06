/*	$NetBSD: lpt_pcc.c,v 1.2 1999/02/14 17:54:28 scw Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Device Driver back-end for the MVME147's parallel printer port
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/syslog.h>

#include <machine/cpu.h>

#include <mvme68k/dev/lptvar.h>
#include <mvme68k/dev/lpt_pccreg.h>
#include <mvme68k/dev/pccreg.h>
#include <mvme68k/dev/pccvar.h>



static int	lpt_pcc_intr __P((void *));
static void	lpt_pcc_open __P((struct lpt_softc *, int));
static void	lpt_pcc_close __P((struct lpt_softc *));
static void	lpt_pcc_iprime __P((struct lpt_softc *));
static void	lpt_pcc_speed __P((struct lpt_softc *, int));
static int	lpt_pcc_notrdy __P((struct lpt_softc *, int));
static void	lpt_pcc_wr_data __P((struct lpt_softc *, u_char));

struct lpt_funcs lpt_pcc_funcs = {
	lpt_pcc_open,
	lpt_pcc_close,
	lpt_pcc_iprime,
	lpt_pcc_speed,
	lpt_pcc_notrdy,
	lpt_pcc_wr_data
};

/*
 * Autoconfig stuff
 */
static int  lpt_pcc_match  __P((struct device *, struct cfdata *, void *));
static void lpt_pcc_attach __P((struct device *, struct device *, void *));

struct cfattach lpt_pcc_ca = {
	sizeof(struct lpt_softc), lpt_pcc_match, lpt_pcc_attach
};

extern struct cfdriver lpt_cd;


/*ARGSUSED*/
static	int
lpt_pcc_match(parent, cf, args)
	struct device *parent;
	struct cfdata *cf;
	void *args;
{
	struct pcc_attach_args *pa = args;

	if (strcmp(pa->pa_name, lpt_cd.cd_name))
		return (0);

	pa->pa_ipl = cf->pcccf_ipl;
	return (1);
}

/*ARGSUSED*/
static void
lpt_pcc_attach(parent, self, args)
struct	device *parent, *self;
void	*args;
{
	struct lpt_softc *sc = (void *)self;
	struct pcc_attach_args *pa = args;
	union lpt_regs *regs;

	/*
	 * Get pointer to regs
	 */
	regs = sc->sc_regs = (void *) PCC_VADDR(pa->pa_offset);
	sc->sc_ipl = pa->pa_ipl & PCC_IMASK;
	sc->sc_laststatus = 0;
	sc->sc_funcs = &lpt_pcc_funcs;

	printf(": PCC Parallel Printer\n");

	/*
	 * Disable interrupts until device is opened
	 */
	sys_pcc->pr_int = 0;

	/*
	 * Main attachment code
	 */
	lpt_attach_subr(sc);

	/*
	 * Hook into the printer interrupt
	 */
	pccintr_establish(PCCV_PRINTER, lpt_pcc_intr, sc->sc_ipl, sc);
}

/*
 * Handle printer interrupts which occur when the printer is ready to accept
 * another char.
 */
int
lpt_pcc_intr(arg)
	void *arg;
{
	struct lpt_softc *sc = (struct lpt_softc *)arg;
	int i;

	/* is printer online and ready for output */
	if (lpt_pcc_notrdy(sc, 0) && lpt_pcc_notrdy(sc, 1))
		return 0;

	i = lpt_intr(sc);

	if ( sys_pcc->pr_int & LPI_ACKINT )
		sys_pcc->pr_int = sc->sc_icr | LPI_ACKINT;

	return i;
}


static void
lpt_pcc_open(sc, int_ena)
	struct lpt_softc *sc;
	int int_ena;
{
	int sps;

	sys_pcc->pr_int = LPI_ACKINT | LPI_FAULTINT;

	if ( int_ena == 0 ) {
		sps = splhigh();
		sc->sc_icr = sc->sc_ipl | LPI_ENABLE;
		sys_pcc->pr_int = sc->sc_icr;
		splx(sps);
	}
}

static void
lpt_pcc_close(sc)
	struct lpt_softc *sc;
{
	sys_pcc->pr_int = 0;
	sc->sc_icr = sc->sc_ipl;
	sys_pcc->pr_int = sc->sc_icr;
}

static void
lpt_pcc_iprime(sc)
	struct lpt_softc *sc;
{
	sys_pcc->pr_cr = LPC_INPUT_PRIME;
	delay(100);
}

static void
lpt_pcc_speed(sc, speed)
	struct lpt_softc *sc;
	int speed;
{
	if ( speed == LPT_STROBE_FAST )
		sys_pcc->pr_cr = LPC_FAST_STROBE;
	else
		sys_pcc->pr_cr = 0;
}

static int
lpt_pcc_notrdy(sc, err)
	struct lpt_softc *sc;
	int err;
{
	union lpt_regs *regs = sc->sc_regs;
	u_char status;
	u_char new;

#define	LPS_INVERT	(LPS_SELECT)
#define	LPS_MASK	(LPS_SELECT|LPS_FAULT|LPS_BUSY|LPS_PAPER_EMPTY)

	status = (regs->pr_status ^ LPS_INVERT) & LPS_MASK;

	if ( err ) {
		new = status & ~sc->sc_laststatus;
		sc->sc_laststatus = status;

		if (new & LPS_SELECT)
			log(LOG_NOTICE, "%s: offline\n",
				sc->sc_dev.dv_xname);
		else if (new & LPS_PAPER_EMPTY)
			log(LOG_NOTICE, "%s: out of paper\n",
				sc->sc_dev.dv_xname);
		else if (new & LPS_FAULT)
			log(LOG_NOTICE, "%s: output error\n",
				sc->sc_dev.dv_xname);
	}

	sys_pcc->pr_int = sc->sc_icr | LPI_FAULTINT;

	return status;
}

static void
lpt_pcc_wr_data(sc, data)
	struct lpt_softc *sc;
	u_char data;
{
	union lpt_regs *regs = sc->sc_regs;

	regs->pr_data = data;
}

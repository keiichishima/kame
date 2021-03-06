/*	$NetBSD: obio.c,v 1.1 2002/03/27 21:51:30 thorpej Exp $	*/

/*
 * Copyright (c) 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * On-board device autoconfiguration support for Intel IQ80321
 * evaluation boards.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

#include <arm/xscale/i80321reg.h>

#include <evbarm/iq80321/iq80321reg.h>
#include <evbarm/iq80321/obiovar.h>

#include "locators.h"

int	obio_match(struct device *, struct cfdata *, void *);
void	obio_attach(struct device *, struct device *, void *);

struct cfattach obio_ca = {
	sizeof(struct device), obio_match, obio_attach,
};

int	obio_print(void *, const char *);
int	obio_submatch(struct device *, struct cfdata *, void *);

/* there can be only one */
int	obio_found;

struct {
	const char *od_name;
	bus_addr_t od_addr;
	int od_irq;
} obio_devices[] =
{
	{ "com",	IQ80321_UART1,		ICU_INT_XINT(1) },

	{ NULL,		0,			0 },
};

int
obio_match(struct device *parent, struct cfdata *cf, void *aux)
{
#if 0
	struct mainbus_attach_args *ma = aux;
#endif

	if (obio_found)
		return (0);

#if 1
	/* XXX Shoot arch/arm/mainbus in the head. */
	return (1);
#else
	if (strcmp(cf->cf_driver->cd_name, ma->ma_name) == 0)
		return (1);

	return (0);
#endif
}

void
obio_attach(struct device *parent, struct device *self, void *aux)
{
	struct obio_attach_args oba;
	int i;

	obio_found = 1;

	printf("\n");

	for (i = 0; obio_devices[i].od_name != NULL; i++) {
		oba.oba_name = obio_devices[i].od_name;
		oba.oba_st = &obio_bs_tag;
		oba.oba_addr = obio_devices[i].od_addr;
		oba.oba_irq = obio_devices[i].od_irq;
		(void) config_found_sm(self, &oba, obio_print, obio_submatch);
	}
}

int
obio_print(void *aux, const char *pnp)
{
	struct obio_attach_args *oba = aux;

	if (pnp)
		printf("%s at %s", oba->oba_name, pnp);

	printf(" addr 0x%08lx", oba->oba_addr);

	return (UNCONF);
}

int
obio_submatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct obio_attach_args *oba = aux;

	if (cf->cf_loc[OBIOCF_ADDR] != OBIOCF_ADDR_DEFAULT &&
	    cf->cf_loc[OBIOCF_ADDR] != oba->oba_addr)
		return (0);

	return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}

/*
 *       Copyright (c) 1997 by Matthew N. Dodd <winter@jurai.net>
 *       All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification, immediately at the beginning of the file.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Credits:  Based on and part of the DPT driver for FreeBSD written and
 *           maintained by Simon Shapiro <shimon@simon-shapiro.org>
 */

/*
 * $FreeBSD: src/sys/dev/dpt/dpt_eisa.c,v 1.12 2000/01/29 14:31:57 peter Exp $
 */

#include "opt_dpt.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/bus.h>

#include <machine/bus_pio.h>
#include <machine/bus.h>
#include <machine/resource.h>
#include <sys/rman.h>

#include <cam/scsi/scsi_all.h>

#include <dev/dpt/dpt.h>

#include <dev/eisa/eisaconf.h>

#include <machine/clock.h>

#include <vm/vm.h>
#include <vm/vm_param.h>
#include <vm/pmap.h>

#define DPT_EISA_IOSIZE			0x100
#define DPT_EISA_SLOT_OFFSET		0x0c00
#define DPT_EISA_EATA_REG_OFFSET	0x0088

#define	DPT_EISA_DPT2402		0x12142402
#define	DPT_EISA_DPTA401		0x1214A401
#define	DPT_EISA_DPTA402		0x1214A402
#define	DPT_EISA_DPTA410		0x1214A410
#define	DPT_EISA_DPTA411		0x1214A411
#define	DPT_EISA_DPTA412		0x1214A412
#define	DPT_EISA_DPTA420		0x1214A420
#define	DPT_EISA_DPTA501		0x1214A501
#define	DPT_EISA_DPTA502		0x1214A502
#define	DPT_EISA_DPTA701		0x1214A701
#define	DPT_EISA_DPTBC01		0x1214BC01
#define	DPT_EISA_NEC8200		0x12148200
#define	DPT_EISA_ATT2408		0x12142408

/* Function Prototypes */

static const char	*dpt_eisa_match(eisa_id_t);

static int
dpt_eisa_probe(device_t dev)
{
	const char *	desc;
	u_int32_t	io_base;
	dpt_conf_t *	conf;

	desc = dpt_eisa_match(eisa_get_id(dev));
	if (!desc)
		return (ENXIO);
	device_set_desc(dev, desc);

	io_base = (eisa_get_slot(dev) * EISA_SLOT_SIZE) + DPT_EISA_SLOT_OFFSET;

	conf = dpt_pio_get_conf(io_base + DPT_EISA_EATA_REG_OFFSET);
	if (!conf) {
		printf("dpt: dpt_pio_get_conf() failed.\n");
		return (ENXIO);
	}

	eisa_add_iospace(dev, io_base, DPT_EISA_IOSIZE, RESVADDR_NONE);
	eisa_add_intr(dev, conf->IRQ,
		      (conf->IRQ_TR ? EISA_TRIGGER_LEVEL : EISA_TRIGGER_EDGE));

	return 0;
}

static int
dpt_eisa_attach(device_t dev)
{
	dpt_softc_t	*dpt;
	struct resource *io = 0;
	struct resource *irq = 0;
        int		unit = device_get_unit(dev);
	int		s;
	int		rid;
	void		*ih;

	rid = 0;
	io = bus_alloc_resource(dev, SYS_RES_IOPORT, &rid,
				0, ~0, 1, RF_ACTIVE);
	if (!io) {
		device_printf(dev, "No I/O space?!\n");
		return ENOMEM;
	}

	dpt = dpt_alloc(unit, rman_get_bustag(io),
			rman_get_bushandle(io) + DPT_EISA_EATA_REG_OFFSET);
	if (dpt == NULL)
		goto bad;

	/* Allocate a dmatag representing the capabilities of this attachment */
	/* XXX Should be a child of the EISA bus dma tag */
	if (bus_dma_tag_create(/*parent*/NULL, /*alignemnt*/1, /*boundary*/0,
			       /*lowaddr*/BUS_SPACE_MAXADDR_32BIT,
			       /*highaddr*/BUS_SPACE_MAXADDR,
			       /*filter*/NULL, /*filterarg*/NULL,
			       /*maxsize*/BUS_SPACE_MAXSIZE_32BIT,
			       /*nsegments*/BUS_SPACE_UNRESTRICTED,
			       /*maxsegsz*/BUS_SPACE_MAXSIZE_32BIT,
			       /*flags*/0, &dpt->parent_dmat) != 0) {
		dpt_free(dpt);
		goto bad;
	}

	rid = 0;
	irq = bus_alloc_resource(dev, SYS_RES_IRQ, &rid,
				 0, ~0, 1, RF_ACTIVE);
	if (!irq) {
		device_printf(dev, "No irq?!\n");
		goto bad;
	}

	s = splcam();
	if (dpt_init(dpt) != 0) {
		dpt_free(dpt);
		goto bad;
	}

	/* Register with the XPT */
	dpt_attach(dpt);
	bus_setup_intr(dev, irq, INTR_TYPE_CAM, dpt_intr, dpt, &ih);

	splx(s);

	return 0;

 bad:
	if (io)
		bus_release_resource(dev, SYS_RES_IOPORT, 0, io);
	if (irq)
		bus_release_resource(dev, SYS_RES_IRQ, 0, irq);
	return -1;
}

static const char	*
dpt_eisa_match(type)
	eisa_id_t	type;
{
	switch (type) {
		case DPT_EISA_DPT2402 :
			return ("DPT PM2012A/9X");
			break;
		case DPT_EISA_DPTA401 :
			return ("DPT PM2012B/9X");
			break;
		case DPT_EISA_DPTA402 :
			return ("DPT PM2012B2/9X");
			break;
		case DPT_EISA_DPTA410 :
			return ("DPT PM2x22A/9X");
			break;
		case DPT_EISA_DPTA411 :
			return ("DPT Spectre");
			break;
		case DPT_EISA_DPTA412 :
			return ("DPT PM2021A/9X");
			break;
		case DPT_EISA_DPTA420 :
			return ("DPT Smart Cache IV (PM2042)");
			break;
		case DPT_EISA_DPTA501 :
			return ("DPT PM2012B1/9X");
			break;
		case DPT_EISA_DPTA502 :
			return ("DPT PM2012Bx/9X");
			break;
		case DPT_EISA_DPTA701 :
			return ("DPT PM2011B1/9X");
			break;
		case DPT_EISA_DPTBC01 :
			return ("DPT PM3011/7X ESDI");
			break;
		case DPT_EISA_NEC8200 :
			return ("NEC EATA SCSI");
			break;
		case DPT_EISA_ATT2408 :
			return ("ATT EATA SCSI");
			break;
		default:
			break;
	}
	
	return (NULL);
}

static device_method_t dpt_eisa_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe,		dpt_eisa_probe),
	DEVMETHOD(device_attach,	dpt_eisa_attach),

	{ 0, 0 }
};

static driver_t dpt_eisa_driver = {
	"dpt",
	dpt_eisa_methods,
	1,			/* unused */
};

static devclass_t dpt_devclass;

DRIVER_MODULE(dpt, eisa, dpt_eisa_driver, dpt_devclass, 0, 0);

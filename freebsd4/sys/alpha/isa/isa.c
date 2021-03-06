/*-
 * Copyright (c) 1998 Doug Rabson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/alpha/isa/isa.c,v 1.19 1999/10/12 21:35:41 dfr Exp $
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/bus.h>
#include <machine/bus.h>
#include <sys/malloc.h>
#include <sys/rman.h>

#include <isa/isareg.h>
#include <isa/isavar.h>
#include <isa/isa_common.h>
#include <alpha/isa/isavar.h>
#include <machine/intr.h>
#include <machine/intrcnt.h>
#include <machine/resource.h>

static struct rman isa_irq_rman;
static struct rman isa_drq_rman;

static void
isa_intr_enable(int irq)
{
	int s = splhigh();
	if (irq < 8)
		outb(IO_ICU1+1, inb(IO_ICU1+1) & ~(1 << irq));
	else
		outb(IO_ICU2+1, inb(IO_ICU2+1) & ~(1 << (irq - 8)));
	splx(s);
}

static void
isa_intr_disable(int irq)
{
	int s = splhigh();
	if (irq < 8)
		outb(IO_ICU1+1, inb(IO_ICU1+1) | (1 << irq));
	else
		outb(IO_ICU2+1, inb(IO_ICU2+1) | (1 << (irq - 8)));
	splx(s);
}

intrmask_t
isa_irq_pending(void)
{
	u_char irr1;
	u_char irr2;

	irr1 = inb(IO_ICU1);
	irr2 = inb(IO_ICU2);
	return ((irr2 << 8) | irr1);
}

intrmask_t
isa_irq_mask(void)
{
	u_char irr1;
	u_char irr2;

	irr1 = inb(IO_ICU1+1);
	irr2 = inb(IO_ICU2+1);
	return ((irr2 << 8) | irr1);
}

void
isa_init(void)
{
	isa_init_intr();
}

void
isa_init_intr(void)
{
	static int initted = 0;

	if (initted) return;
	initted = 1;
		
	isa_irq_rman.rm_start = 0;
	isa_irq_rman.rm_end = 15;
	isa_irq_rman.rm_type = RMAN_ARRAY;
	isa_irq_rman.rm_descr = "ISA Interrupt request lines";
	if (rman_init(&isa_irq_rman)
	    || rman_manage_region(&isa_irq_rman, 0, 1)
	    || rman_manage_region(&isa_irq_rman, 3, 15))
		panic("isa_probe isa_irq_rman");

	isa_drq_rman.rm_start = 0;
	isa_drq_rman.rm_end = 7;
	isa_drq_rman.rm_type = RMAN_ARRAY;
	isa_drq_rman.rm_descr = "ISA DMA request lines";
	if (rman_init(&isa_drq_rman)
	    || rman_manage_region(&isa_drq_rman, 0, 7))
		panic("isa_probe isa_drq_rman");

	/* mask all isa interrupts */
	outb(IO_ICU1+1, 0xff);
	outb(IO_ICU2+1, 0xff);

	/* make sure chaining irq is enabled */
	isa_intr_enable(2);
}

struct resource *
isa_alloc_intr(device_t bus, device_t child, int irq)
{
	return rman_reserve_resource(&isa_irq_rman, irq, irq, 1,
				     0, child);
}

int
isa_release_intr(device_t bus, device_t child, struct resource *r)
{
	return rman_release_resource(r);
}

/*
 * This implementation simply passes the request up to the parent
 * bus, which in our case is the pci chipset device, substituting any
 * configured values if the caller defaulted.  We can get away with
 * this because there is no special mapping for ISA resources on this
 * platform.  When porting this code to another architecture, it may be
 * necessary to interpose a mapping layer here.
 *
 * We manage our own interrupt resources since ISA interrupts go through
 * the ISA PIC, not the PCI interrupt controller.
 */
struct resource *
isa_alloc_resource(device_t bus, device_t child, int type, int *rid,
		   u_long start, u_long end, u_long count, u_int flags)
{
	/*
	 * Consider adding a resource definition. We allow rid 0-1 for
	 * irq and drq, 0-3 for memory and 0-7 for ports which is
	 * sufficient for isapnp.
	 */
	int passthrough = (device_get_parent(child) != bus);
	int isdefault = (start == 0UL && end == ~0UL);
	struct isa_device* idev = DEVTOISA(child);
	struct resource_list *rl = &idev->id_resources;
	struct resource_list_entry *rle;
	struct resource *res;
	
	if (!passthrough && !isdefault) {
		rle = resource_list_find(rl, type, *rid);
		if (!rle) {
			if (*rid < 0)
				return 0;
			switch (type) {
			case SYS_RES_IRQ:
				if (*rid >= ISA_NIRQ)
					return 0;
				break;
			case SYS_RES_DRQ:
				if (*rid >= ISA_NDRQ)
					return 0;
				break;
			case SYS_RES_MEMORY:
				if (*rid >= ISA_NMEM)
					return 0;
				break;
			case SYS_RES_IOPORT:
				if (*rid >= ISA_NPORT)
					return 0;
				break;
			default:
				return 0;
			}
			resource_list_add(rl, type, *rid, start, end, count);
		}
	}

	if (type != SYS_RES_IRQ && type != SYS_RES_DRQ)
		return resource_list_alloc(rl, bus, child, type, rid,
					   start, end, count, flags);

	if (!passthrough) {
		rl = device_get_ivars(child);
		rle = resource_list_find(rl, type, *rid);
		if (!rle)
			return 0;
		if (rle->res)
			panic("isa_alloc_resource: resource entry is busy");
		if (isdefault) {
			start = end = rle->start;
			count = 1;
		}
	}

	if (type == SYS_RES_IRQ)
	    res = rman_reserve_resource(&isa_irq_rman, start, start, 1,
					0, child);
	else
	    res = rman_reserve_resource(&isa_drq_rman, start, start, 1,
					0, child);
	    
	if (res && !passthrough) {
		rle = resource_list_find(rl, type, *rid);
		rle->start = rman_get_start(res);
		rle->end = rman_get_end(res);
		rle->count = 1;
		rle->res = res;
	}

	return res;
}

int
isa_release_resource(device_t bus, device_t child, int type, int rid,
		     struct resource *res)
{
	int passthrough = (device_get_parent(child) != bus);
	struct isa_device* idev = DEVTOISA(child);
	struct resource_list *rl = &idev->id_resources;
	struct resource_list_entry *rle;
	int error;

	if (type != SYS_RES_IRQ)
		return resource_list_release(rl, bus, child, type, rid, res);

	error = rman_release_resource(res);

	if (!passthrough && !error) {
		rle = resource_list_find(rl, SYS_RES_IRQ, rid);
		if (rle)
			rle->res = NULL;
		else
			error = ENOENT;
	}

	return error;
}

struct isa_intr {
	void *ih;
	driver_intr_t *intr;
	void *arg;
	int irq;
};

/*
 * Wrap ISA interrupt routines so that we can feed non-specific
 * EOI to the PICs.
 */

static void
isa_handle_intr(void *arg)
{
	struct isa_intr *ii = arg;
	int irq = ii->irq;

	ii->intr(ii->arg);

	if (ii->irq > 7)
		outb(IO_ICU2, 0x20 | (irq & 7));
	outb(IO_ICU1, 0x20 | (irq > 7 ? 2 : irq));
}

int
isa_setup_intr(device_t dev, device_t child,
	       struct resource *irq, int flags,
	       driver_intr_t *intr, void *arg, void **cookiep)
{
	struct isa_intr *ii;
	int error;
	
	error = rman_activate_resource(irq);
	if (error)
		return error;

	ii = malloc(sizeof(struct isa_intr), M_DEVBUF, M_NOWAIT);
	if (!ii)
		return ENOMEM;
	ii->intr = intr;
	ii->arg = arg;
	ii->irq = irq->r_start;

	error = alpha_setup_intr(0x800 + (irq->r_start << 4),
			 isa_handle_intr, ii, &ii->ih,
			 &intrcnt[INTRCNT_ISA_IRQ + irq->r_start]);
	if (error) {
		free(ii, M_DEVBUF);
		return error;
	}
	isa_intr_enable(irq->r_start);

	*cookiep = ii;

	if (child)
		device_printf(child, "interrupting at ISA irq %d\n",
			      (int)irq->r_start);

	return 0;
}

int
isa_teardown_intr(device_t dev, device_t child,
		  struct resource *irq, void *cookie)
{
	struct isa_intr *ii = cookie;

	alpha_teardown_intr(ii->ih);
	isa_intr_disable(irq->r_start);

	return 0;
}

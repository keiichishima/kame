/*	$NetBSD: optiide.c,v 1.5 2004/01/03 22:56:53 thorpej Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/param.h>
#include <sys/systm.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciidereg.h>
#include <dev/pci/pciidevar.h>
#include <dev/pci/pciide_opti_reg.h>

static void opti_chip_map(struct pciide_softc*, struct pci_attach_args*);
static void opti_setup_channel(struct wdc_channel*);

static int  optiide_match(struct device *, struct cfdata *, void *);
static void optiide_attach(struct device *, struct device *, void *);

CFATTACH_DECL(optiide, sizeof(struct pciide_softc),
    optiide_match, optiide_attach, NULL, NULL);

static const struct pciide_product_desc pciide_opti_products[] =  {
	{ PCI_PRODUCT_OPTI_82C621,
	  0,
	  "OPTi 82c621 PCI IDE controller",
	  opti_chip_map,
	},
	{ PCI_PRODUCT_OPTI_82C568,
	  0,
	  "OPTi 82c568 (82c621 compatible) PCI IDE controller",
	  opti_chip_map,
	},
	{ PCI_PRODUCT_OPTI_82D568,
	  0,
	  "OPTi 82d568 (82c621 compatible) PCI IDE controller",
	  opti_chip_map,
	},
	{ 0,
	  0,
	  NULL,
	  NULL
	}
};

static int
optiide_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_OPTI &&
	    PCI_CLASS(pa->pa_class) == PCI_CLASS_MASS_STORAGE &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_MASS_STORAGE_IDE) {
		if (pciide_lookup_product(pa->pa_id, pciide_opti_products))
			return (2);
	}
	return (0);
}

static void
optiide_attach(struct device *parent, struct device *self, void *aux)
{
	struct pci_attach_args *pa = aux;
	struct pciide_softc *sc = (struct pciide_softc *)self;

	pciide_common_attach(sc, pa,
	    pciide_lookup_product(pa->pa_id, pciide_opti_products));

}

static void
opti_chip_map(struct pciide_softc *sc, struct pci_attach_args *pa)
{
	struct pciide_channel *cp;
	bus_size_t cmdsize, ctlsize;
	pcireg_t interface;
	u_int8_t init_ctrl;
	int channel;

	if (pciide_chipen(sc, pa) == 0)
		return;

	aprint_normal("%s: bus-master DMA support present",
	    sc->sc_wdcdev.sc_dev.dv_xname);

	/*
	 * XXXSCW:
	 * There seem to be a couple of buggy revisions/implementations
	 * of the OPTi pciide chipset. This kludge seems to fix one of
	 * the reported problems (PR/11644) but still fails for the
	 * other (PR/13151), although the latter may be due to other
	 * issues too...
	 */
	if (PCI_REVISION(pa->pa_class) <= 0x12) {
		aprint_normal(" but disabled due to chip rev. <= 0x12");
		sc->sc_dma_ok = 0;
	} else
		pciide_mapreg_dma(sc, pa);

	aprint_normal("\n");

	sc->sc_wdcdev.cap = WDC_CAPABILITY_DATA32 | WDC_CAPABILITY_DATA16 |
		WDC_CAPABILITY_MODE;
	sc->sc_wdcdev.PIO_cap = 4;
	if (sc->sc_dma_ok) {
		sc->sc_wdcdev.cap |= WDC_CAPABILITY_DMA | WDC_CAPABILITY_IRQACK;
		sc->sc_wdcdev.irqack = pciide_irqack;
		sc->sc_wdcdev.DMA_cap = 2;
	}
	sc->sc_wdcdev.set_modes = opti_setup_channel;

	sc->sc_wdcdev.channels = sc->wdc_chanarray;
	sc->sc_wdcdev.nchannels = PCIIDE_NUM_CHANNELS;

	init_ctrl = pciide_pci_read(sc->sc_pc, sc->sc_tag,
	    OPTI_REG_INIT_CONTROL);

	interface = PCI_INTERFACE(pa->pa_class);

	for (channel = 0; channel < sc->sc_wdcdev.nchannels; channel++) {
		cp = &sc->pciide_channels[channel];
		if (pciide_chansetup(sc, channel, interface) == 0)
			continue;
		if (channel == 1 &&
		    (init_ctrl & OPTI_INIT_CONTROL_CH2_DISABLE) != 0) {
			aprint_normal("%s: %s channel ignored (disabled)\n",
			    sc->sc_wdcdev.sc_dev.dv_xname, cp->name);
			cp->wdc_channel.ch_flags |= WDCF_DISABLED;
			continue;
		}
		pciide_mapchan(pa, cp, interface, &cmdsize, &ctlsize,
		    pciide_pci_intr);
	}
}

static void
opti_setup_channel(struct wdc_channel *chp)
{
	struct ata_drive_datas *drvp;
	struct pciide_channel *cp = (struct pciide_channel*)chp;
	struct pciide_softc *sc = (struct pciide_softc *)cp->wdc_channel.ch_wdc;
	int drive, spd;
	int mode[2];
	u_int8_t rv, mr;

	/*
	 * The `Delay' and `Address Setup Time' fields of the
	 * Miscellaneous Register are always zero initially.
	 */
	mr = opti_read_config(chp, OPTI_REG_MISC) & ~OPTI_MISC_INDEX_MASK;
	mr &= ~(OPTI_MISC_DELAY_MASK |
		OPTI_MISC_ADDR_SETUP_MASK |
		OPTI_MISC_INDEX_MASK);

	/* Prime the control register before setting timing values */
	opti_write_config(chp, OPTI_REG_CONTROL, OPTI_CONTROL_DISABLE);

	/* Determine the clockrate of the PCIbus the chip is attached to */
	spd = (int) opti_read_config(chp, OPTI_REG_STRAP);
	spd &= OPTI_STRAP_PCI_SPEED_MASK;

	/* setup DMA if needed */
	pciide_channel_dma_setup(cp);

	for (drive = 0; drive < 2; drive++) {
		drvp = &chp->ch_drive[drive];
		/* If no drive, skip */
		if ((drvp->drive_flags & DRIVE) == 0) {
			mode[drive] = -1;
			continue;
		}

		if ((drvp->drive_flags & DRIVE_DMA)) {
			/*
			 * Timings will be used for both PIO and DMA,
			 * so adjust DMA mode if needed
			 */
			if (drvp->PIO_mode > (drvp->DMA_mode + 2))
				drvp->PIO_mode = drvp->DMA_mode + 2;
			if (drvp->DMA_mode + 2 > (drvp->PIO_mode))
				drvp->DMA_mode = (drvp->PIO_mode > 2) ?
				    drvp->PIO_mode - 2 : 0;
			if (drvp->DMA_mode == 0)
				drvp->PIO_mode = 0;

			mode[drive] = drvp->DMA_mode + 5;
		} else
			mode[drive] = drvp->PIO_mode;

		if (drive && mode[0] >= 0 &&
		    (opti_tim_as[spd][mode[0]] != opti_tim_as[spd][mode[1]])) {
			/*
			 * Can't have two drives using different values
			 * for `Address Setup Time'.
			 * Slow down the faster drive to compensate.
			 */
			int d = (opti_tim_as[spd][mode[0]] >
				 opti_tim_as[spd][mode[1]]) ?  0 : 1;

			mode[d] = mode[1-d];
			chp->ch_drive[d].PIO_mode = chp->ch_drive[1-d].PIO_mode;
			chp->ch_drive[d].DMA_mode = 0;
			chp->ch_drive[d].drive_flags &= ~DRIVE_DMA;
		}
	}

	for (drive = 0; drive < 2; drive++) {
		int m;
		if ((m = mode[drive]) < 0)
			continue;

		/* Set the Address Setup Time and select appropriate index */
		rv = opti_tim_as[spd][m] << OPTI_MISC_ADDR_SETUP_SHIFT;
		rv |= OPTI_MISC_INDEX(drive);
		opti_write_config(chp, OPTI_REG_MISC, mr | rv);

		/* Set the pulse width and recovery timing parameters */
		rv  = opti_tim_cp[spd][m] << OPTI_PULSE_WIDTH_SHIFT;
		rv |= opti_tim_rt[spd][m] << OPTI_RECOVERY_TIME_SHIFT;
		opti_write_config(chp, OPTI_REG_READ_CYCLE_TIMING, rv);
		opti_write_config(chp, OPTI_REG_WRITE_CYCLE_TIMING, rv);

		/* Set the Enhanced Mode register appropriately */
	    	rv = pciide_pci_read(sc->sc_pc, sc->sc_tag, OPTI_REG_ENH_MODE);
		rv &= ~OPTI_ENH_MODE_MASK(chp->ch_channel, drive);
		rv |= OPTI_ENH_MODE(chp->ch_channel, drive, opti_tim_em[m]);
		pciide_pci_write(sc->sc_pc, sc->sc_tag, OPTI_REG_ENH_MODE, rv);
	}

	/* Finally, enable the timings */
	opti_write_config(chp, OPTI_REG_CONTROL, OPTI_CONTROL_ENABLE);
}

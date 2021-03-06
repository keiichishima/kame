/*
 * Copyright (c) 1997, 1998, 1999
 *	Bill Paul <wpaul@ctr.columbia.edu>.  All rights reserved.
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
 *	This product includes software developed by Bill Paul.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/pci/if_ste.c,v 1.13.2.1 1999/10/10 23:04:54 wpaul Exp $
 */


#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <net/ethernet.h>
#include <net/if_dl.h>
#include <net/if_media.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#include "opt_bdg.h"
#ifdef BRIDGE
#include <net/bridge.h>
#endif

#include <vm/vm.h>              /* for vtophys */
#include <vm/pmap.h>            /* for vtophys */
#include <machine/clock.h>      /* for DELAY */
#include <machine/bus_memio.h>
#include <machine/bus_pio.h>
#include <machine/bus.h>

#include <pci/pcireg.h>
#include <pci/pcivar.h>

#define STE_USEIOSPACE

/*#define STE_BACKGROUND_AUTONEG*/

#include <pci/if_stereg.h>

#if !defined(lint)
static const char rcsid[] =
  "$FreeBSD: src/sys/pci/if_ste.c,v 1.13.2.1 1999/10/10 23:04:54 wpaul Exp $";
#endif

/*
 * Various supported device vendors/types and their names.
 */
static struct ste_type ste_devs[] = {
	{ ST_VENDORID, ST_DEVICEID_ST201, "Sundance ST201 10/100BaseTX" },
	{ DL_VENDORID, DL_DEVICEID_550TX, "D-Link DFE-550TX 10/100BaseTX" },
	{ 0, 0, NULL }
};

static struct ste_type ste_phys[] = {
	{ 0, 0, "<MII-compliant physical interface>" }
};

static unsigned long ste_count = 0;
static const char *ste_probe	__P((pcici_t, pcidi_t));
static void ste_attach		__P((pcici_t, int));
static void ste_init		__P((void *));
static void ste_intr		__P((void *));
static void ste_rxeof		__P((struct ste_softc *));
static void ste_txeoc		__P((struct ste_softc *));
static void ste_txeof		__P((struct ste_softc *));
static void ste_stats_update	__P((void *));
static void ste_stop		__P((struct ste_softc *));
static void ste_reset		__P((struct ste_softc *));
static int ste_ioctl		__P((struct ifnet *, u_long, caddr_t));
static int ste_encap		__P((struct ste_softc *, struct ste_chain *,
					struct mbuf *));
static void ste_start		__P((struct ifnet *));
static void ste_watchdog	__P((struct ifnet *));
static void ste_shutdown	__P((int, void *));
static int ste_newbuf		__P((struct ste_softc *,
					struct ste_chain_onefrag *,
					struct mbuf *));
static int ste_ifmedia_upd	__P((struct ifnet *));
static void ste_ifmedia_sts	__P((struct ifnet *, struct ifmediareq *));

static void ste_mii_sync		__P((struct ste_softc *));
static void ste_mii_send		__P((struct ste_softc *, u_int32_t, int));
static int ste_mii_readreg	__P((struct ste_softc *, struct ste_mii_frame *));
static int ste_mii_writereg	__P((struct ste_softc *, struct ste_mii_frame *));
static u_int16_t ste_phy_readreg	__P((struct ste_softc *, int));
static void ste_phy_writereg	__P((struct ste_softc *, int, int));

static void ste_autoneg_xmit	__P((struct ste_softc *));
static void ste_autoneg_mii	__P((struct ste_softc *, int, int));
static void ste_setmode_mii	__P((struct ste_softc *, int));
static void ste_getmode_mii	__P((struct ste_softc *));

static int ste_eeprom_wait	__P((struct ste_softc *));
static int ste_read_eeprom	__P((struct ste_softc *, caddr_t, int,
							int, int));
static void ste_wait		__P((struct ste_softc *));
static u_int8_t ste_calchash	__P((caddr_t));
static void ste_setmulti	__P((struct ste_softc *));
static int ste_init_rx_list	__P((struct ste_softc *));
static void ste_init_tx_list	__P((struct ste_softc *));

#define STE_SETBIT4(sc, reg, x)				\
	CSR_WRITE_4(sc, reg, CSR_READ_4(sc, reg) | x)

#define STE_CLRBIT4(sc, reg, x)				\
	CSR_WRITE_4(sc, reg, CSR_READ_4(sc, reg) & ~x)

#define STE_SETBIT2(sc, reg, x)				\
	CSR_WRITE_2(sc, reg, CSR_READ_2(sc, reg) | x)

#define STE_CLRBIT2(sc, reg, x)				\
	CSR_WRITE_2(sc, reg, CSR_READ_2(sc, reg) & ~x)

#define STE_SETBIT1(sc, reg, x)				\
	CSR_WRITE_1(sc, reg, CSR_READ_1(sc, reg) | x)

#define STE_CLRBIT1(sc, reg, x)				\
	CSR_WRITE_1(sc, reg, CSR_READ_1(sc, reg) & ~x)


#ifdef __i386__
#define STE_BUS_SPACE_IO	I386_BUS_SPACE_IO
#define STE_BUS_SPACE_MEM	I386_BUS_SPACE_MEM
#endif
#ifdef __alpha__
#define STE_BUS_SPACE_IO	ALPHA_BUS_SPACE_IO
#define STE_BUS_SPACE_MEM	ALPHA_BUS_SPACE_MEM
#endif

#define MII_SET(x)		STE_SETBIT1(sc, STE_PHYCTL, x)
#define MII_CLR(x)		STE_CLRBIT1(sc, STE_PHYCTL, x) 

/*
 * Sync the PHYs by setting data bit and strobing the clock 32 times.
 */
static void ste_mii_sync(sc)
	struct ste_softc		*sc;
{
	register int		i;

	MII_SET(STE_PHYCTL_MDIR|STE_PHYCTL_MDATA);

	for (i = 0; i < 32; i++) {
		MII_SET(STE_PHYCTL_MCLK);
		DELAY(1);
		MII_CLR(STE_PHYCTL_MCLK);
		DELAY(1);
	}

	return;
}

/*
 * Clock a series of bits through the MII.
 */
static void ste_mii_send(sc, bits, cnt)
	struct ste_softc		*sc;
	u_int32_t		bits;
	int			cnt;
{
	int			i;

	MII_CLR(STE_PHYCTL_MCLK);

	for (i = (0x1 << (cnt - 1)); i; i >>= 1) {
                if (bits & i) {
			MII_SET(STE_PHYCTL_MDATA);
                } else {
			MII_CLR(STE_PHYCTL_MDATA);
                }
		DELAY(1);
		MII_CLR(STE_PHYCTL_MCLK);
		DELAY(1);
		MII_SET(STE_PHYCTL_MCLK);
	}
}

/*
 * Read an PHY register through the MII.
 */
static int ste_mii_readreg(sc, frame)
	struct ste_softc		*sc;
	struct ste_mii_frame	*frame;
	
{
	int			i, ack, s;

	s = splimp();

	/*
	 * Set up frame for RX.
	 */
	frame->mii_stdelim = STE_MII_STARTDELIM;
	frame->mii_opcode = STE_MII_READOP;
	frame->mii_turnaround = 0;
	frame->mii_data = 0;
	
	CSR_WRITE_2(sc, STE_PHYCTL, 0);
	/*
 	 * Turn on data xmit.
	 */
	MII_SET(STE_PHYCTL_MDIR);

	ste_mii_sync(sc);

	/*
	 * Send command/address info.
	 */
	ste_mii_send(sc, frame->mii_stdelim, 2);
	ste_mii_send(sc, frame->mii_opcode, 2);
	ste_mii_send(sc, frame->mii_phyaddr, 5);
	ste_mii_send(sc, frame->mii_regaddr, 5);

	/* Turn off xmit. */
	MII_CLR(STE_PHYCTL_MDIR);

	/* Idle bit */
	MII_CLR((STE_PHYCTL_MCLK|STE_PHYCTL_MDATA));
	DELAY(1);
	MII_SET(STE_PHYCTL_MCLK);
	DELAY(1);

	/* Check for ack */
	MII_CLR(STE_PHYCTL_MCLK);
	DELAY(1);
	MII_SET(STE_PHYCTL_MCLK);
	DELAY(1);
	ack = CSR_READ_2(sc, STE_PHYCTL) & STE_PHYCTL_MDATA;

	/*
	 * Now try reading data bits. If the ack failed, we still
	 * need to clock through 16 cycles to keep the PHY(s) in sync.
	 */
	if (ack) {
		for(i = 0; i < 16; i++) {
			MII_CLR(STE_PHYCTL_MCLK);
			DELAY(1);
			MII_SET(STE_PHYCTL_MCLK);
			DELAY(1);
		}
		goto fail;
	}

	for (i = 0x8000; i; i >>= 1) {
		MII_CLR(STE_PHYCTL_MCLK);
		DELAY(1);
		if (!ack) {
			if (CSR_READ_2(sc, STE_PHYCTL) & STE_PHYCTL_MDATA)
				frame->mii_data |= i;
			DELAY(1);
		}
		MII_SET(STE_PHYCTL_MCLK);
		DELAY(1);
	}

fail:

	MII_CLR(STE_PHYCTL_MCLK);
	DELAY(1);
	MII_SET(STE_PHYCTL_MCLK);
	DELAY(1);

	splx(s);

	if (ack)
		return(1);
	return(0);
}

/*
 * Write to a PHY register through the MII.
 */
static int ste_mii_writereg(sc, frame)
	struct ste_softc		*sc;
	struct ste_mii_frame	*frame;
	
{
	int			s;

	s = splimp();
	/*
	 * Set up frame for TX.
	 */

	frame->mii_stdelim = STE_MII_STARTDELIM;
	frame->mii_opcode = STE_MII_WRITEOP;
	frame->mii_turnaround = STE_MII_TURNAROUND;
	
	/*
 	 * Turn on data output.
	 */
	MII_SET(STE_PHYCTL_MDIR);

	ste_mii_sync(sc);

	ste_mii_send(sc, frame->mii_stdelim, 2);
	ste_mii_send(sc, frame->mii_opcode, 2);
	ste_mii_send(sc, frame->mii_phyaddr, 5);
	ste_mii_send(sc, frame->mii_regaddr, 5);
	ste_mii_send(sc, frame->mii_turnaround, 2);
	ste_mii_send(sc, frame->mii_data, 16);

	/* Idle bit. */
	MII_SET(STE_PHYCTL_MCLK);
	DELAY(1);
	MII_CLR(STE_PHYCTL_MCLK);
	DELAY(1);

	/*
	 * Turn off xmit.
	 */
	MII_CLR(STE_PHYCTL_MDIR);

	splx(s);

	return(0);
}

static u_int16_t ste_phy_readreg(sc, reg)
	struct ste_softc		*sc;
	int			reg;
{
	struct ste_mii_frame	frame;

	bzero((char *)&frame, sizeof(frame));

	frame.mii_phyaddr = sc->ste_phy_addr;
	frame.mii_regaddr = reg;
	ste_mii_readreg(sc, &frame);

	return(frame.mii_data);
}

static void ste_phy_writereg(sc, reg, data)
	struct ste_softc		*sc;
	int			reg;
	int			data;
{
	struct ste_mii_frame	frame;

	bzero((char *)&frame, sizeof(frame));

	frame.mii_phyaddr = sc->ste_phy_addr;
	frame.mii_regaddr = reg;
	frame.mii_data = data;

	ste_mii_writereg(sc, &frame);

	return;
}


/*
 * Initiate an autonegotiation session.
 */
static void ste_autoneg_xmit(sc)
	struct ste_softc		*sc;
{
	u_int16_t		phy_sts;

	ste_phy_writereg(sc, PHY_BMCR, PHY_BMCR_RESET);
	DELAY(500);
	while(ste_phy_readreg(sc, STE_PHY_GENCTL)
			& PHY_BMCR_RESET);

	phy_sts = ste_phy_readreg(sc, PHY_BMCR);
	phy_sts |= PHY_BMCR_AUTONEGENBL|PHY_BMCR_AUTONEGRSTR;
	ste_phy_writereg(sc, PHY_BMCR, phy_sts);

	return;
}

/*
 * Invoke autonegotiation on a PHY. Also used with the 3Com internal
 * autoneg logic which is mapped onto the MII.
 */
static void ste_autoneg_mii(sc, flag, verbose)
	struct ste_softc		*sc;
	int			flag;
	int			verbose;
{
	u_int16_t		phy_sts = 0, media, advert, ability;
	struct ifnet		*ifp;
	struct ifmedia		*ifm;

	ifm = &sc->ifmedia;
	ifp = &sc->arpcom.ac_if;

	ifm->ifm_media = IFM_ETHER | IFM_AUTO;

	/*
	 * The 100baseT4 PHY on the 3c905-T4 has the 'autoneg supported'
	 * bit cleared in the status register, but has the 'autoneg enabled'
	 * bit set in the control register. This is a contradiction, and
	 * I'm not sure how to handle it. If you want to force an attempt
	 * to autoneg for 100baseT4 PHYs, #define FORCE_AUTONEG_TFOUR
	 * and see what happens.
	 */
#ifndef FORCE_AUTONEG_TFOUR
	/*
	 * First, see if autoneg is supported. If not, there's
	 * no point in continuing.
	 */
	phy_sts = ste_phy_readreg(sc, PHY_BMSR);
	if (!(phy_sts & PHY_BMSR_CANAUTONEG)) {
		if (verbose)
			printf("ste%d: autonegotiation not supported\n",
							sc->ste_unit);
		ifm->ifm_media = IFM_ETHER|IFM_10_T|IFM_HDX;	
		media = ste_phy_readreg(sc, PHY_BMCR);
		media &= ~PHY_BMCR_SPEEDSEL;
		media &= ~PHY_BMCR_DUPLEX;
		ste_phy_writereg(sc, PHY_BMCR, media);
		STE_CLRBIT2(sc, STE_MACCTL0, STE_MACCTL0_FULLDUPLEX);
		return;
	}
#endif

	switch (flag) {
	case STE_FLAG_FORCEDELAY:
		/*
	 	 * XXX Never use this option anywhere but in the probe
	 	 * routine: making the kernel stop dead in its tracks
 		 * for three whole seconds after we've gone multi-user
		 * is really bad manners.
	 	 */
		ste_autoneg_xmit(sc);
		DELAY(5000000);
		break;
	case STE_FLAG_SCHEDDELAY:
		/*
		 * Wait for the transmitter to go idle before starting
		 * an autoneg session, otherwise ste_start() may clobber
	 	 * our timeout, and we don't want to allow transmission
		 * during an autoneg session since that can screw it up.
	 	 */
		if (sc->ste_cdata.ste_tx_head != NULL) {
			sc->ste_want_auto = 1;
			return;
		}
		ste_autoneg_xmit(sc);
		ifp->if_timer = 5;
		sc->ste_autoneg = 1;
		sc->ste_want_auto = 0;
		return;
		break;
	case STE_FLAG_DELAYTIMEO:
		ifp->if_timer = 0;
		sc->ste_autoneg = 0;
		break;
	default:
		printf("ste%d: invalid autoneg flag: %d\n", sc->ste_unit, flag);
		return;
	}

	if (ste_phy_readreg(sc, PHY_BMSR) & PHY_BMSR_AUTONEGCOMP) {
		if (verbose)
			printf("ste%d: autoneg complete, ", sc->ste_unit);
		phy_sts = ste_phy_readreg(sc, PHY_BMSR);
	} else {
		if (verbose)
			printf("ste%d: autoneg not complete, ", sc->ste_unit);
	}

	media = ste_phy_readreg(sc, PHY_BMCR);

	/* Link is good. Report modes and set duplex mode. */
	if (ste_phy_readreg(sc, PHY_BMSR) & PHY_BMSR_LINKSTAT) {
		if (verbose)
			printf("link status good ");
		advert = ste_phy_readreg(sc, STE_PHY_ANAR);
		ability = ste_phy_readreg(sc, STE_PHY_LPAR);

		if (advert & PHY_ANAR_100BT4 && ability & PHY_ANAR_100BT4) {
			ifm->ifm_media = IFM_ETHER|IFM_100_T4;
			media |= PHY_BMCR_SPEEDSEL;
			media &= ~PHY_BMCR_DUPLEX;
			printf("(100baseT4)\n");
		} else if (advert & PHY_ANAR_100BTXFULL &&
			ability & PHY_ANAR_100BTXFULL) {
			ifm->ifm_media = IFM_ETHER|IFM_100_TX|IFM_FDX;
			media |= PHY_BMCR_SPEEDSEL;
			media |= PHY_BMCR_DUPLEX;
			printf("(full-duplex, 100Mbps)\n");
		} else if (advert & PHY_ANAR_100BTXHALF &&
			ability & PHY_ANAR_100BTXHALF) {
			ifm->ifm_media = IFM_ETHER|IFM_100_TX|IFM_HDX;
			media |= PHY_BMCR_SPEEDSEL;
			media &= ~PHY_BMCR_DUPLEX;
			printf("(half-duplex, 100Mbps)\n");
		} else if (advert & PHY_ANAR_10BTFULL &&
			ability & PHY_ANAR_10BTFULL) {
			ifm->ifm_media = IFM_ETHER|IFM_10_T|IFM_FDX;
			media &= ~PHY_BMCR_SPEEDSEL;
			media |= PHY_BMCR_DUPLEX;
			printf("(full-duplex, 10Mbps)\n");
		} else if (advert & PHY_ANAR_10BTHALF &&
			ability & PHY_ANAR_10BTHALF) {
			ifm->ifm_media = IFM_ETHER|IFM_10_T|IFM_HDX;
			media &= ~PHY_BMCR_SPEEDSEL;
			media &= ~PHY_BMCR_DUPLEX;
			printf("(half-duplex, 10Mbps)\n");
		}

		/* Set ASIC's duplex mode to match the PHY. */
		if (media & PHY_BMCR_DUPLEX)
			STE_SETBIT2(sc, STE_MACCTL0, STE_MACCTL0_FULLDUPLEX);
		else
			STE_CLRBIT2(sc, STE_MACCTL0, STE_MACCTL0_FULLDUPLEX);
		ste_phy_writereg(sc, PHY_BMCR, media);
	} else {
		if (verbose)
			printf("no carrier (forcing half-duplex, 10Mbps)\n");
		ifm->ifm_media = IFM_ETHER|IFM_10_T|IFM_HDX;
		media &= ~PHY_BMCR_SPEEDSEL;
		media &= ~PHY_BMCR_DUPLEX;
		ste_phy_writereg(sc, PHY_BMCR, media);
		STE_CLRBIT2(sc, STE_MACCTL0, STE_MACCTL0_FULLDUPLEX);
	}

	ste_init(sc);

	if (sc->ste_tx_pend) {
		sc->ste_autoneg = 0;
		sc->ste_tx_pend = 0;
		ste_start(ifp);
	}

	return;
}

static void ste_getmode_mii(sc)
	struct ste_softc		*sc;
{
	u_int16_t		bmsr;
	struct ifnet		*ifp;

	ifp = &sc->arpcom.ac_if;

	bmsr = ste_phy_readreg(sc, PHY_BMSR);
	if (bootverbose)
		printf("ste%d: PHY status word: %x\n", sc->ste_unit, bmsr);

	/* fallback */
	sc->ifmedia.ifm_media = IFM_ETHER|IFM_10_T|IFM_HDX;

	if (bmsr & PHY_BMSR_10BTHALF) {
		if (bootverbose)
			printf("ste%d: 10Mbps half-duplex mode supported\n",
								sc->ste_unit);
		ifmedia_add(&sc->ifmedia,
			IFM_ETHER|IFM_10_T|IFM_HDX, 0, NULL);
		ifmedia_add(&sc->ifmedia, IFM_ETHER|IFM_10_T, 0, NULL);
	}

	if (bmsr & PHY_BMSR_10BTFULL) {
		if (bootverbose)
			printf("ste%d: 10Mbps full-duplex mode supported\n",
								sc->ste_unit);
		ifmedia_add(&sc->ifmedia,
			IFM_ETHER|IFM_10_T|IFM_FDX, 0, NULL);
		sc->ifmedia.ifm_media = IFM_ETHER|IFM_10_T|IFM_FDX;
	}

	if (bmsr & PHY_BMSR_100BTXHALF) {
		if (bootverbose)
			printf("ste%d: 100Mbps half-duplex mode supported\n",
								sc->ste_unit);
		ifp->if_baudrate = 100000000;
		ifmedia_add(&sc->ifmedia, IFM_ETHER|IFM_100_TX, 0, NULL);
		ifmedia_add(&sc->ifmedia,
			IFM_ETHER|IFM_100_TX|IFM_HDX, 0, NULL);
		sc->ifmedia.ifm_media = IFM_ETHER|IFM_100_TX|IFM_HDX;
	}

	if (bmsr & PHY_BMSR_100BTXFULL) {
		if (bootverbose)
			printf("ste%d: 100Mbps full-duplex mode supported\n",
								sc->ste_unit);
		ifp->if_baudrate = 100000000;
		ifmedia_add(&sc->ifmedia,
			IFM_ETHER|IFM_100_TX|IFM_FDX, 0, NULL);
		sc->ifmedia.ifm_media = IFM_ETHER|IFM_100_TX|IFM_FDX;
	}

	/* Some also support 100BaseT4. */
	if (bmsr & PHY_BMSR_100BT4) {
		if (bootverbose)
			printf("ste%d: 100baseT4 mode supported\n", sc->ste_unit);
		ifp->if_baudrate = 100000000;
		ifmedia_add(&sc->ifmedia, IFM_ETHER|IFM_100_T4, 0, NULL);
		sc->ifmedia.ifm_media = IFM_ETHER|IFM_100_T4;
#ifdef FORCE_AUTONEG_TFOUR
		if (bootverbose)
			printf("ste%d: forcing on autoneg support for BT4\n",
							 sc->ste_unit);
		ifmedia_add(&sc->ifmedia, IFM_ETHER|IFM_AUTO, 0, NULL);
		sc->ifmedia.ifm_media = IFM_ETHER|IFM_AUTO;
#endif
	}

	if (bmsr & PHY_BMSR_CANAUTONEG) {
		if (bootverbose)
			printf("ste%d: autoneg supported\n", sc->ste_unit);
		ifmedia_add(&sc->ifmedia, IFM_ETHER|IFM_AUTO, 0, NULL);
		sc->ifmedia.ifm_media = IFM_ETHER|IFM_AUTO;
	}

	return;
}

/*
 * Set speed and duplex mode.
 */
static void ste_setmode_mii(sc, media)
	struct ste_softc		*sc;
	int			media;
{
	u_int16_t		bmcr;
	struct ifnet		*ifp;

	ifp = &sc->arpcom.ac_if;

	/*
	 * If an autoneg session is in progress, stop it.
	 */
	if (sc->ste_autoneg) {
		printf("ste%d: canceling autoneg session\n", sc->ste_unit);
		ifp->if_timer = sc->ste_autoneg = sc->ste_want_auto = 0;
		bmcr = ste_phy_readreg(sc, PHY_BMCR);
		bmcr &= ~PHY_BMCR_AUTONEGENBL;
		ste_phy_writereg(sc, PHY_BMCR, bmcr);
	}

	printf("ste%d: selecting MII, ", sc->ste_unit);

	bmcr = ste_phy_readreg(sc, PHY_BMCR);

	bmcr &= ~(PHY_BMCR_AUTONEGENBL|PHY_BMCR_SPEEDSEL|
			PHY_BMCR_DUPLEX|PHY_BMCR_LOOPBK);

	if (IFM_SUBTYPE(media) == IFM_100_T4) {
		printf("100Mbps/T4, half-duplex\n");
		bmcr |= PHY_BMCR_SPEEDSEL;
		bmcr &= ~PHY_BMCR_DUPLEX;
	}

	if (IFM_SUBTYPE(media) == IFM_100_TX) {
		printf("100Mbps, ");
		bmcr |= PHY_BMCR_SPEEDSEL;
	}

	if (IFM_SUBTYPE(media) == IFM_10_T) {
		printf("10Mbps, ");
		bmcr &= ~PHY_BMCR_SPEEDSEL;
	}

	if ((media & IFM_GMASK) == IFM_FDX) {
		printf("full duplex\n");
		bmcr |= PHY_BMCR_DUPLEX;
		STE_SETBIT2(sc, STE_MACCTL0, STE_MACCTL0_FULLDUPLEX);
	} else {
		printf("half duplex\n");
		bmcr &= ~PHY_BMCR_DUPLEX;
		STE_CLRBIT2(sc, STE_MACCTL0, STE_MACCTL0_FULLDUPLEX);
	}

	ste_phy_writereg(sc, PHY_BMCR, bmcr);

	return;
}

static int ste_ifmedia_upd(ifp)
	struct ifnet		*ifp;
{
	struct ste_softc	*sc;
	struct ifmedia		*ifm;

	sc = ifp->if_softc;
	ifm = &sc->ifmedia;

	if (IFM_TYPE(ifm->ifm_media) != IFM_ETHER)
		return(EINVAL);

	if (IFM_SUBTYPE(ifm->ifm_media) == IFM_AUTO)
		ste_autoneg_mii(sc, STE_FLAG_SCHEDDELAY, 1);
	else
		ste_setmode_mii(sc, ifm->ifm_media);

	return(0);
}

static void ste_ifmedia_sts(ifp, ifmr)
	struct ifnet		*ifp;
	struct ifmediareq	*ifmr;
{
	struct ste_softc		*sc;
	u_int16_t		advert = 0, ability = 0;

	sc = ifp->if_softc;

	ifmr->ifm_active = IFM_ETHER;

	if (!(ste_phy_readreg(sc, PHY_BMCR) & PHY_BMCR_AUTONEGENBL)) {
		if (ste_phy_readreg(sc, PHY_BMCR) & PHY_BMCR_SPEEDSEL)
			ifmr->ifm_active = IFM_ETHER|IFM_100_TX;
		else
			ifmr->ifm_active = IFM_ETHER|IFM_10_T;
		if (ste_phy_readreg(sc, PHY_BMCR) & PHY_BMCR_DUPLEX)
			ifmr->ifm_active |= IFM_FDX;
		else
			ifmr->ifm_active |= IFM_HDX;
		return;
	}

	ability = ste_phy_readreg(sc, STE_PHY_LPAR);
	advert = ste_phy_readreg(sc, STE_PHY_ANAR);
	if (advert & PHY_ANAR_100BT4 &&
		ability & PHY_ANAR_100BT4) {
		ifmr->ifm_active = IFM_ETHER|IFM_100_T4;
	} else if (advert & PHY_ANAR_100BTXFULL &&
		ability & PHY_ANAR_100BTXFULL) {
		ifmr->ifm_active = IFM_ETHER|IFM_100_TX|IFM_FDX;
	} else if (advert & PHY_ANAR_100BTXHALF &&
		ability & PHY_ANAR_100BTXHALF) {
		ifmr->ifm_active = IFM_ETHER|IFM_100_TX|IFM_HDX;
	} else if (advert & PHY_ANAR_10BTFULL &&
		ability & PHY_ANAR_10BTFULL) {
		ifmr->ifm_active = IFM_ETHER|IFM_10_T|IFM_FDX;
	} else if (advert & PHY_ANAR_10BTHALF &&
		ability & PHY_ANAR_10BTHALF) {
		ifmr->ifm_active = IFM_ETHER|IFM_10_T|IFM_HDX;
	}

	return;
}

static void ste_wait(sc)
	struct ste_softc		*sc;
{
	register int		i;

	for (i = 0; i < STE_TIMEOUT; i++) {
		if (!(CSR_READ_4(sc, STE_DMACTL) & STE_DMACTL_DMA_HALTINPROG))
			break;
	}

	if (i == STE_TIMEOUT)
		printf("ste%d: command never completed!\n", sc->ste_unit);

	return;
}

/*
 * The EEPROM is slow: give it time to come ready after issuing
 * it a command.
 */
static int ste_eeprom_wait(sc)
	struct ste_softc		*sc;
{
	int			i;

	DELAY(1000);

	for (i = 0; i < 100; i++) {
		if (CSR_READ_2(sc, STE_EEPROM_CTL) & STE_EECTL_BUSY)
			DELAY(1000);
		else
			break;
	}

	if (i == 100) {
		printf("ste%d: eeprom failed to come ready\n", sc->ste_unit);
		return(1);
	}

	return(0);
}

/*
 * Read a sequence of words from the EEPROM. Note that ethernet address
 * data is stored in the EEPROM in network byte order.
 */
static int ste_read_eeprom(sc, dest, off, cnt, swap)
	struct ste_softc		*sc;
	caddr_t			dest;
	int			off;
	int			cnt;
	int			swap;
{
	int			err = 0, i;
	u_int16_t		word = 0, *ptr;

	if (ste_eeprom_wait(sc))
		return(1);

	for (i = 0; i < cnt; i++) {
		CSR_WRITE_2(sc, STE_EEPROM_CTL, STE_EEOPCODE_READ | (off + i));
		err = ste_eeprom_wait(sc);
		if (err)
			break;
		word = CSR_READ_2(sc, STE_EEPROM_DATA);
		ptr = (u_int16_t *)(dest + (i * 2));
		if (swap)
			*ptr = ntohs(word);
		else
			*ptr = word;	
	}

	return(err ? 1 : 0);
}

static u_int8_t ste_calchash(addr)
	caddr_t			addr;
{

	u_int32_t		crc, carry;
	int			i, j;
	u_int8_t		c;

	/* Compute CRC for the address value. */
	crc = 0xFFFFFFFF; /* initial value */

	for (i = 0; i < 6; i++) {
		c = *(addr + i);
		for (j = 0; j < 8; j++) {
			carry = ((crc & 0x80000000) ? 1 : 0) ^ (c & 0x01);
			crc <<= 1;
			c >>= 1;
			if (carry)
				crc = (crc ^ 0x04c11db6) | carry;
		}
	}

	/* return the filter bit position */
	return(crc & 0x0000003F);
}

static void ste_setmulti(sc)
	struct ste_softc	*sc;
{
	struct ifnet		*ifp;
	int			h = 0;
	u_int32_t		hashes[2] = { 0, 0 };
	struct ifmultiaddr	*ifma;

	ifp = &sc->arpcom.ac_if;
	if (ifp->if_flags & IFF_ALLMULTI || ifp->if_flags & IFF_PROMISC) {
		STE_SETBIT1(sc, STE_RX_MODE, STE_RXMODE_ALLMULTI);
		STE_CLRBIT1(sc, STE_RX_MODE, STE_RXMODE_MULTIHASH);
		return;
	}

	/* first, zot all the existing hash bits */
	CSR_WRITE_4(sc, STE_MAR0, 0);
	CSR_WRITE_4(sc, STE_MAR1, 0);

	/* now program new ones */
	for (ifma = ifp->if_multiaddrs.lh_first; ifma != NULL;
	    ifma = ifma->ifma_link.le_next) {
		if (ifma->ifma_addr->sa_family != AF_LINK)
			continue;
		h = ste_calchash(LLADDR((struct sockaddr_dl *)ifma->ifma_addr));
		if (h < 32)
			hashes[0] |= (1 << h);
		else
			hashes[1] |= (1 << (h - 32));
	}

	CSR_WRITE_4(sc, STE_MAR0, hashes[0]);
	CSR_WRITE_4(sc, STE_MAR1, hashes[1]);
	STE_CLRBIT1(sc, STE_RX_MODE, STE_RXMODE_ALLMULTI);
	STE_SETBIT1(sc, STE_RX_MODE, STE_RXMODE_MULTIHASH);

	return;
}

static void ste_intr(xsc)
	void			*xsc;
{
	struct ste_softc	*sc;
	struct ifnet		*ifp;
	u_int16_t		status;

	sc = xsc;
	ifp = &sc->arpcom.ac_if;

	/* See if this is really our interrupt. */
	if (!(CSR_READ_2(sc, STE_ISR) & STE_ISR_INTLATCH))
		return;

	for (;;) {
		status = CSR_READ_2(sc, STE_ISR_ACK);

		if (!(status & STE_INTRS))
			break;

		if (status & STE_ISR_RX_DMADONE)
			ste_rxeof(sc);

		if (status & STE_ISR_TX_DMADONE)
			ste_txeof(sc);

		if (status & STE_ISR_TX_DONE)
			ste_txeoc(sc);

		if (status & STE_ISR_STATS_OFLOW) {
			untimeout(ste_stats_update, sc, sc->ste_stat_ch);
			ste_stats_update(sc);
		}

		if (status & STE_ISR_HOSTERR) {
			ste_reset(sc);
			ste_init(sc);
		}
	}

	/* Re-enable interrupts */
	CSR_WRITE_2(sc, STE_IMR, STE_INTRS);

	if (ifp->if_snd.ifq_head != NULL)
		ste_start(ifp);

	return;
}

/*
 * A frame has been uploaded: pass the resulting mbuf chain up to
 * the higher level protocols.
 */
static void ste_rxeof(sc)
	struct ste_softc		*sc;
{
        struct ether_header	*eh;
        struct mbuf		*m;
        struct ifnet		*ifp;
	struct ste_chain_onefrag	*cur_rx;
	int			total_len = 0;
	u_int32_t		rxstat;

	ifp = &sc->arpcom.ac_if;

again:

	while((rxstat = sc->ste_cdata.ste_rx_head->ste_ptr->ste_status)) {
		cur_rx = sc->ste_cdata.ste_rx_head;
		sc->ste_cdata.ste_rx_head = cur_rx->ste_next;

		/*
		 * If an error occurs, update stats, clear the
		 * status word and leave the mbuf cluster in place:
		 * it should simply get re-used next time this descriptor
	 	 * comes up in the ring.
		 */
		if (rxstat & STE_RXSTAT_FRAME_ERR) {
			ifp->if_ierrors++;
			cur_rx->ste_ptr->ste_status = 0;
			continue;
		}

		/*
		 * If there error bit was not set, the upload complete
		 * bit should be set which means we have a valid packet.
		 * If not, something truly strange has happened.
		 */
		if (!(rxstat & STE_RXSTAT_DMADONE)) {
			printf("ste%d: bad receive status -- packet dropped",
							sc->ste_unit);
			ifp->if_ierrors++;
			cur_rx->ste_ptr->ste_status = 0;
			continue;
		}

		/* No errors; receive the packet. */	
		m = cur_rx->ste_mbuf;
		total_len = cur_rx->ste_ptr->ste_status & STE_RXSTAT_FRAMELEN;

		/*
		 * Try to conjure up a new mbuf cluster. If that
		 * fails, it means we have an out of memory condition and
		 * should leave the buffer in place and continue. This will
		 * result in a lost packet, but there's little else we
		 * can do in this situation.
		 */
		if (ste_newbuf(sc, cur_rx, NULL) == ENOBUFS) {
			ifp->if_ierrors++;
			cur_rx->ste_ptr->ste_status = 0;
			continue;
		}

		ifp->if_ipackets++;
		eh = mtod(m, struct ether_header *);
		m->m_pkthdr.rcvif = ifp;
		m->m_pkthdr.len = m->m_len = total_len;

#if NBPFILTER > 0
		/* Handle BPF listeners. Let the BPF user see the packet. */
		if (ifp->if_bpf)
			bpf_mtap(ifp, m);
#endif

#ifdef BRIDGE
		if (do_bridge) {
			struct ifnet *bdg_ifp ;
			bdg_ifp = bridge_in(m);
			if (bdg_ifp != BDG_LOCAL && bdg_ifp != BDG_DROP)
				bdg_forward(&m, bdg_ifp);
			if (((bdg_ifp != BDG_LOCAL) && (bdg_ifp != BDG_BCAST) &&
			    (bdg_ifp != BDG_MCAST)) || bdg_ifp == BDG_DROP) {
				m_freem(m);
				continue;
			}
		}
#endif

#if NBPFILTER > 0
		/*
		 * Don't pass packet up to the ether_input() layer unless it's
		 * a broadcast packet, multicast packet, matches our ethernet
		 * address or the interface is in promiscuous mode.
		 */
		if (ifp->if_bpf) {
			if (ifp->if_flags & IFF_PROMISC &&
			    (bcmp(eh->ether_dhost, sc->arpcom.ac_enaddr,
			    ETHER_ADDR_LEN) && (eh->ether_dhost[0] & 1) == 0)){
				m_freem(m);
				continue;
			}
		}
#endif

		/* Remove header from mbuf and pass it on. */
		m_adj(m, sizeof(struct ether_header));
		ether_input(ifp, eh, m);
	}

	/*
	 * Handle the 'end of channel' condition. When the upload
	 * engine hits the end of the RX ring, it will stall. This
	 * is our cue to flush the RX ring, reload the uplist pointer
	 * register and unstall the engine.
	 * XXX This is actually a little goofy. With the ThunderLAN
	 * chip, you get an interrupt when the receiver hits the end
	 * of the receive ring, which tells you exactly when you
	 * you need to reload the ring pointer. Here we have to
	 * fake it. I'm mad at myself for not being clever enough
	 * to avoid the use of a goto here.
	 */
	if (CSR_READ_4(sc, STE_RX_DMALIST_PTR) == 0 ||
		CSR_READ_4(sc, STE_DMACTL) & STE_DMACTL_RXDMA_STOPPED) {
		STE_SETBIT4(sc, STE_DMACTL, STE_DMACTL_RXDMA_STALL);
		ste_wait(sc);
		CSR_WRITE_4(sc, STE_RX_DMALIST_PTR,
			vtophys(&sc->ste_ldata->ste_rx_list[0]));
		sc->ste_cdata.ste_rx_head = &sc->ste_cdata.ste_rx_chain[0];
		STE_SETBIT4(sc, STE_DMACTL, STE_DMACTL_RXDMA_UNSTALL);
		goto again;
	}

	return;
}

static void ste_txeoc(sc)
	struct ste_softc	*sc;
{
	u_int8_t		txstat;
	struct ifnet		*ifp;

	ifp = &sc->arpcom.ac_if;

	while ((txstat = CSR_READ_1(sc, STE_TX_STATUS)) &
	    STE_TXSTATUS_TXDONE) {
		if (txstat & STE_TXSTATUS_UNDERRUN ||
		    txstat & STE_TXSTATUS_EXCESSCOLLS ||
		    txstat & STE_TXSTATUS_RECLAIMERR) {
			ifp->if_oerrors++;
			printf("ste%d: transmission error: %x\n",
			    sc->ste_unit, txstat);
			STE_SETBIT4(sc, STE_ASICCTL, STE_ASICCTL_TX_RESET);

			if (sc->ste_cdata.ste_tx_head != NULL)
				CSR_WRITE_4(sc, STE_TX_DMALIST_PTR,
				    vtophys(sc->ste_cdata.ste_tx_head->ste_ptr));
			if (txstat & STE_TXSTATUS_UNDERRUN &&
			    sc->ste_tx_thresh < STE_PACKET_SIZE) {
				sc->ste_tx_thresh += STE_MIN_FRAMELEN;
				printf("ste%d: tx underrun, increasing tx"
				    " start threshold to %d bytes\n",
				    sc->ste_unit, sc->ste_tx_thresh);
			}
			CSR_WRITE_2(sc, STE_TX_STARTTHRESH, sc->ste_tx_thresh);
			CSR_WRITE_2(sc, STE_TX_RECLAIM_THRESH,
			    (STE_PACKET_SIZE >> 4));
		}
		STE_SETBIT2(sc, STE_MACCTL1, STE_MACCTL1_TX_ENABLE);
		if (CSR_READ_4(sc, STE_TX_DMALIST_PTR))
			CSR_WRITE_4(sc, STE_DMACTL, STE_DMACTL_TXDMA_UNSTALL);
		CSR_WRITE_2(sc, STE_TX_STATUS, txstat);
	}

	return;
}

static void ste_txeof(sc)
	struct ste_softc	*sc;
{
	struct ste_chain	*cur_tx;
	struct ifnet		*ifp;

	ifp = &sc->arpcom.ac_if;

	/* Clear the timeout timer. */
	ifp->if_timer = 0;

	while(sc->ste_cdata.ste_tx_head != NULL) {
		cur_tx = sc->ste_cdata.ste_tx_head;
		if (!(cur_tx->ste_ptr->ste_ctl & STE_TXCTL_DMADONE))
			break;
		sc->ste_cdata.ste_tx_head = cur_tx->ste_next;

		m_freem(cur_tx->ste_mbuf);
		cur_tx->ste_mbuf = NULL;
		ifp->if_opackets++;

		cur_tx->ste_next = sc->ste_cdata.ste_tx_free;
		sc->ste_cdata.ste_tx_free = cur_tx;
	}

	if (sc->ste_cdata.ste_tx_head == NULL) {
		ifp->if_flags &= ~IFF_OACTIVE;
		sc->ste_cdata.ste_tx_tail = NULL;
		if (sc->ste_want_auto)
			ste_autoneg_mii(sc, STE_FLAG_SCHEDDELAY, 1);
	} else {
		if (CSR_READ_4(sc, STE_DMACTL) & STE_DMACTL_TXDMA_STOPPED ||
		    !CSR_READ_4(sc, STE_TX_DMALIST_PTR)) {
			CSR_WRITE_4(sc, STE_TX_DMALIST_PTR,
			    vtophys(sc->ste_cdata.ste_tx_head->ste_ptr));
			CSR_WRITE_4(sc, STE_DMACTL, STE_DMACTL_TXDMA_UNSTALL);
		}
	}

	return;
}

static void ste_stats_update(xsc)
	void			*xsc;
{
	struct ste_softc	*sc;
	struct ste_stats	stats;
	struct ifnet		*ifp;
	int			i, s;
	u_int8_t		*p;

	s = splimp();

	sc = xsc;
	ifp = &sc->arpcom.ac_if;
	p = (u_int8_t *)&stats;

	for (i = 0; i < sizeof(stats); i++) {
		*p = CSR_READ_1(sc, STE_STATS + i);
		p++;
	}

	ifp->if_collisions += stats.ste_single_colls +
	    stats.ste_multi_colls + stats.ste_late_colls;

	sc->ste_stat_ch = timeout(ste_stats_update, sc, hz);
	splx(s);

	return;
}


/*
 * Probe for a Sundance ST201 chip. Check the PCI vendor and device
 * IDs against our list and return a device name if we find a match.
 */
static const char *ste_probe(config_id, device_id)
	pcici_t			config_id;
	pcidi_t			device_id;
{
	struct ste_type		*t;

	t = ste_devs;

	while(t->ste_name != NULL) {
		if ((device_id & 0xFFFF) == t->ste_vid &&
		    ((device_id >> 16) & 0xFFFF) == t->ste_did) {
			return(t->ste_name);
		}
		t++;
	}

	return(NULL);
}

/*
 * Attach the interface. Allocate softc structures, do ifmedia
 * setup and ethernet/BPF attach.
 */
static void
ste_attach(config_id, unit)
	pcici_t			config_id;
	int			unit;
{
	int			s, i;
#ifndef STE_USEIOSPACE
	vm_offset_t		pbase, vbase;
#endif
	u_int32_t		command;
	struct ste_softc	*sc;
	struct ifnet		*ifp;
	int			media = IFM_ETHER|IFM_100_TX|IFM_FDX;
	struct ste_type		*p;
	u_int16_t		phy_vid, phy_did, phy_sts;

	s = splimp();

	sc = malloc(sizeof(struct ste_softc), M_DEVBUF, M_NOWAIT);
	if (sc == NULL) {
		printf("ste%d: no memory for softc struct!\n", unit);
		goto fail;
	}
	bzero(sc, sizeof(struct ste_softc));

	/*
	 * Handle power management nonsense.
	 */
	command = pci_conf_read(config_id, STE_PCI_CAPID) & 0x000000FF;
	if (command == 0x01) {

		command = pci_conf_read(config_id, STE_PCI_PWRMGMTCTRL);
		if (command & STE_PSTATE_MASK) {
			u_int32_t		iobase, membase, irq;

			/* Save important PCI config data. */
			iobase = pci_conf_read(config_id, STE_PCI_LOIO);
			membase = pci_conf_read(config_id, STE_PCI_LOMEM);
			irq = pci_conf_read(config_id, STE_PCI_INTLINE);

			/* Reset the power state. */
			printf("ste%d: chip is in D%d power mode "
			"-- setting to D0\n", unit, command & STE_PSTATE_MASK);
			command &= 0xFFFFFFFC;
			pci_conf_write(config_id, STE_PCI_PWRMGMTCTRL, command);

			/* Restore PCI config data. */
			pci_conf_write(config_id, STE_PCI_LOIO, iobase);
			pci_conf_write(config_id, STE_PCI_LOMEM, membase);
			pci_conf_write(config_id, STE_PCI_INTLINE, irq);
		}
	}

	/*
	 * Map control/status registers.
	 */
	command = pci_conf_read(config_id, PCI_COMMAND_STATUS_REG);
	command |= (PCIM_CMD_PORTEN|PCIM_CMD_MEMEN|PCIM_CMD_BUSMASTEREN);
	pci_conf_write(config_id, PCI_COMMAND_STATUS_REG, command);
	command = pci_conf_read(config_id, PCI_COMMAND_STATUS_REG);

#ifdef STE_USEIOSPACE
	if (!(command & PCIM_CMD_PORTEN)) {
		printf("ste%d: failed to enable I/O ports!\n", unit);
		free(sc, M_DEVBUF);
		goto fail;
	}

	if (!pci_map_port(config_id, STE_PCI_LOIO,
	    (pci_port_t *)&(sc->ste_bhandle))) {
		printf ("ste%d: couldn't map ports\n", unit);
		goto fail;
        }

	sc->ste_btag = STE_BUS_SPACE_IO;
#else
	if (!(command & PCIM_CMD_MEMEN)) {
		printf("ste%d: failed to enable memory mapping!\n", unit);
		goto fail;
	}

	if (!pci_map_mem(config_id, STE_PCI_LOMEM, &vbase, &pbase)) {
		printf ("ste%d: couldn't map memory\n", unit);
		goto fail;
	}
	sc->ste_btag = STE_BUS_SPACE_MEM;
	sc->ste_bhandle = vbase;
#endif

	/* Allocate interrupt */
	if (!pci_map_int(config_id, ste_intr, sc, &net_imask)) {
		printf("ste%d: couldn't map interrupt\n", unit);
		goto fail;
	}

	callout_handle_init(&sc->ste_stat_ch);

	/* Reset the adapter. */
	ste_reset(sc);

	/*
	 * Get station address from the EEPROM.
	 */
	if (ste_read_eeprom(sc, (caddr_t)&sc->arpcom.ac_enaddr,
	    STE_EEADDR_NODE0, 3, 0)) {
		printf("ste%d: failed to read station address\n", unit);
		free(sc, M_DEVBUF);
		goto fail;
	}

	/*
	 * A Sundance chip was detected. Inform the world.
	 */
	printf("ste%d: Ethernet address: %6D\n", unit,
	    sc->arpcom.ac_enaddr, ":");

	sc->ste_unit = unit;

	/* Allocate the descriptor queues. */
	sc->ste_ldata = contigmalloc(sizeof(struct ste_list_data), M_DEVBUF,
	    M_NOWAIT, 0x100000, 0xffffffff, PAGE_SIZE, 0);

	if (sc->ste_ldata == NULL) {
		free(sc, M_DEVBUF);
		printf("ste%d: no memory for list buffers!\n", unit);
		goto fail;
	}

	bzero(sc->ste_ldata, sizeof(struct ste_list_data));

	if (bootverbose)
		printf("ste%d: probing for a PHY\n", sc->ste_unit);
	for (i = STE_PHYADDR_MIN; i < STE_PHYADDR_MAX + 1; i++) {
		if (bootverbose)
			printf("ste%d: checking address: %d\n",
						sc->ste_unit, i);
		sc->ste_phy_addr = i;
		ste_phy_writereg(sc, PHY_BMCR, PHY_BMCR_RESET);
		DELAY(500);
		while(ste_phy_readreg(sc, PHY_BMCR)
				& PHY_BMCR_RESET);
		if ((phy_sts = ste_phy_readreg(sc, PHY_BMSR)))
			break;
	}
	if (phy_sts) {
		phy_vid = ste_phy_readreg(sc, STE_PHY_VENID);
		phy_did = ste_phy_readreg(sc, STE_PHY_DEVID);
		if (bootverbose)
			printf("ste%d: found PHY at address %d, ",
				sc->ste_unit, sc->ste_phy_addr);
		if (bootverbose)
			printf("vendor id: %x device id: %x\n",
			phy_vid, phy_did);
		p = ste_phys;
		while(p->ste_vid) {
			if (phy_vid == p->ste_vid &&
				(phy_did | 0x000F) == p->ste_did) {
				sc->ste_pinfo = p;
				break;
			}
			p++;
		}
		if (sc->ste_pinfo == NULL)
			sc->ste_pinfo = &ste_phys[PHY_UNKNOWN];
		if (bootverbose)
			printf("ste%d: PHY type: %s\n",
				sc->ste_unit, sc->ste_pinfo->ste_name);
	} else {
		printf("ste%d: MII without any phy!\n", sc->ste_unit);
		free(sc->ste_ldata, M_DEVBUF);
		free(sc, M_DEVBUF);
		goto fail;
	}

	ifp = &sc->arpcom.ac_if;
	ifp->if_softc = sc;
	ifp->if_unit = unit;
	ifp->if_name = "ste";
	ifp->if_mtu = ETHERMTU;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = ste_ioctl;
	ifp->if_output = ether_output;
	ifp->if_start = ste_start;
	ifp->if_watchdog = ste_watchdog;
	ifp->if_init = ste_init;
	ifp->if_baudrate = 10000000;
	ifp->if_snd.ifq_maxlen = STE_TX_LIST_CNT - 1;

	/*
	 * Do ifmedia setup.
	 */
	ifmedia_init(&sc->ifmedia, 0, ste_ifmedia_upd, ste_ifmedia_sts);

	ste_getmode_mii(sc);
	ste_autoneg_mii(sc, STE_FLAG_FORCEDELAY, 1);
	media = sc->ifmedia.ifm_media;
	ste_stop(sc);

	ifmedia_set(&sc->ifmedia, media);

	/*
	 * Call MI attach routines.
	 */

	if_attach(ifp);
	ether_ifattach(ifp);

#if NBPFILTER > 0
	bpfattach(ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif

	at_shutdown(ste_shutdown, sc, SHUTDOWN_POST_SYNC);

fail:
	splx(s);
	return;
}

static int ste_newbuf(sc, c, m)
	struct ste_softc	*sc;
	struct ste_chain_onefrag	*c;
	struct mbuf		*m;
{
	struct mbuf		*m_new = NULL;

	if (m == NULL) {
		MGETHDR(m_new, M_DONTWAIT, MT_DATA);
		if (m_new == NULL) {
			printf("ste%d: no memory for rx list -- "
			    "packet dropped\n", sc->ste_unit);
			return(ENOBUFS);
		}
		MCLGET(m_new, M_DONTWAIT);
		if (!(m_new->m_flags & M_EXT)) {
			printf("ste%d: no memory for rx list -- "
			    "packet dropped\n", sc->ste_unit);
			m_freem(m_new);
			return(ENOBUFS);
		}
		m_new->m_len = m_new->m_pkthdr.len = MCLBYTES;
	} else {
		m_new = m;
		m_new->m_len = m_new->m_pkthdr.len = MCLBYTES;
		m_new->m_data = m_new->m_ext.ext_buf;
	}

	m_adj(m_new, ETHER_ALIGN);

	c->ste_mbuf = m_new;
	c->ste_ptr->ste_status = 0;
	c->ste_ptr->ste_frag.ste_addr = vtophys(mtod(m_new, caddr_t));
	c->ste_ptr->ste_frag.ste_len = 1536 | STE_FRAG_LAST;

	return(0);
}

static int ste_init_rx_list(sc)
	struct ste_softc	*sc;
{
	struct ste_chain_data	*cd;
	struct ste_list_data	*ld;
	int			i;

	cd = &sc->ste_cdata;
	ld = sc->ste_ldata;

	for (i = 0; i < STE_RX_LIST_CNT; i++) {
		cd->ste_rx_chain[i].ste_ptr = &ld->ste_rx_list[i];
		if (ste_newbuf(sc, &cd->ste_rx_chain[i], NULL) == ENOBUFS)
			return(ENOBUFS);
		if (i == (STE_RX_LIST_CNT - 1)) {
			cd->ste_rx_chain[i].ste_next =
			    &cd->ste_rx_chain[0];
			ld->ste_rx_list[i].ste_next =
			    vtophys(&ld->ste_rx_list[0]);
		} else {
			cd->ste_rx_chain[i].ste_next =
			    &cd->ste_rx_chain[i + 1];
			ld->ste_rx_list[i].ste_next =
			    vtophys(&ld->ste_rx_list[i + 1]);
		}

	}

	cd->ste_rx_head = &cd->ste_rx_chain[0];

	return(0);
}

static void ste_init_tx_list(sc)
	struct ste_softc	*sc;
{
	struct ste_chain_data	*cd;
	struct ste_list_data	*ld;
	int			i;

	cd = &sc->ste_cdata;
	ld = sc->ste_ldata;
	for (i = 0; i < STE_TX_LIST_CNT; i++) {
		cd->ste_tx_chain[i].ste_ptr = &ld->ste_tx_list[i];
		if (i == (STE_TX_LIST_CNT - 1))
			cd->ste_tx_chain[i].ste_next = NULL;
		else
			cd->ste_tx_chain[i].ste_next =
			    &cd->ste_tx_chain[i + 1];
	}

	cd->ste_tx_free = &cd->ste_tx_chain[0];
	cd->ste_tx_tail = cd->ste_tx_head = NULL;

	return;
}

static void ste_init(xsc)
	void			*xsc;
{
	struct ste_softc	*sc;
	int			i, s;
	u_int16_t		phy_bmcr = 0;
	struct ifnet		*ifp;

	sc = xsc;
	ifp = &sc->arpcom.ac_if;

	if (sc->ste_autoneg)
		return;

	s = splimp();

	if (sc->ste_pinfo != NULL)
		phy_bmcr = ste_phy_readreg(sc, PHY_BMCR);

	ste_stop(sc);

	/* Init our MAC address */
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		CSR_WRITE_1(sc, STE_PAR0 + i, sc->arpcom.ac_enaddr[i]);
	}

	/* Init RX list */
	if (ste_init_rx_list(sc) == ENOBUFS) {
		printf("ste%d: initialization failed: no "
		    "memory for RX buffers\n", sc->ste_unit);
		ste_stop(sc);
		splx(s);
		return;
	}

	/* Init TX descriptors */
	ste_init_tx_list(sc);

	/* Set the TX freethresh value */
	CSR_WRITE_1(sc, STE_TX_DMABURST_THRESH, STE_PACKET_SIZE >> 8);

	/* Set the TX start threshold for best performance. */
	sc->ste_tx_thresh = STE_MIN_FRAMELEN;
	CSR_WRITE_2(sc, STE_TX_STARTTHRESH, sc->ste_tx_thresh);

	/* Set the TX reclaim threshold. */
	CSR_WRITE_1(sc, STE_TX_RECLAIM_THRESH, (STE_PACKET_SIZE >> 4));

	/* Set up the RX filter. */
	CSR_WRITE_1(sc, STE_RX_MODE, STE_RXMODE_UNICAST);

	/* If we want promiscuous mode, set the allframes bit. */
	if (ifp->if_flags & IFF_PROMISC) {
		STE_SETBIT1(sc, STE_RX_MODE, STE_RXMODE_PROMISC);
	} else {
		STE_CLRBIT1(sc, STE_RX_MODE, STE_RXMODE_PROMISC);
	}

	/* Set capture broadcast bit to accept broadcast frames. */
	if (ifp->if_flags & IFF_BROADCAST) {
		STE_SETBIT1(sc, STE_RX_MODE, STE_RXMODE_BROADCAST);
	} else {
		STE_CLRBIT1(sc, STE_RX_MODE, STE_RXMODE_BROADCAST);
	}

	ste_setmulti(sc);

	/* Load the address of the RX list. */
	STE_SETBIT4(sc, STE_DMACTL, STE_DMACTL_RXDMA_STALL);
	ste_wait(sc);
	CSR_WRITE_4(sc, STE_RX_DMALIST_PTR,
	    vtophys(&sc->ste_ldata->ste_rx_list[0]));
	STE_SETBIT4(sc, STE_DMACTL, STE_DMACTL_RXDMA_UNSTALL);
	STE_SETBIT4(sc, STE_DMACTL, STE_DMACTL_RXDMA_UNSTALL);

	/* Enable receiver and transmitter */
	CSR_WRITE_2(sc, STE_MACCTL0, 0);
	STE_SETBIT2(sc, STE_MACCTL1, STE_MACCTL1_TX_ENABLE);
	STE_SETBIT2(sc, STE_MACCTL1, STE_MACCTL1_RX_ENABLE);

	/* Enable stats counters. */
	STE_SETBIT2(sc, STE_MACCTL1, STE_MACCTL1_STATS_ENABLE);

	/* Enable interrupts. */
	CSR_WRITE_2(sc, STE_ISR, 0xFFFF);
	CSR_WRITE_2(sc, STE_IMR, STE_INTRS);

	if (sc->ste_pinfo != NULL)
		ste_phy_writereg(sc, PHY_BMCR, phy_bmcr);
	if (phy_bmcr & PHY_BMCR_DUPLEX)
		STE_SETBIT2(sc, STE_MACCTL0, STE_MACCTL0_FULLDUPLEX);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	splx(s);

	sc->ste_stat_ch = timeout(ste_stats_update, sc, hz);

	return;
}

static void ste_stop(sc)
	struct ste_softc	*sc;
{
	int			i;
	struct ifnet		*ifp;

	ifp = &sc->arpcom.ac_if;

	untimeout(ste_stats_update, sc, sc->ste_stat_ch);

	CSR_WRITE_2(sc, STE_IMR, 0);
	STE_SETBIT2(sc, STE_MACCTL1, STE_MACCTL1_TX_DISABLE);
	STE_SETBIT2(sc, STE_MACCTL1, STE_MACCTL1_RX_DISABLE);
	STE_SETBIT2(sc, STE_MACCTL1, STE_MACCTL1_STATS_DISABLE);
	STE_SETBIT2(sc, STE_DMACTL, STE_DMACTL_TXDMA_STALL);
	STE_SETBIT2(sc, STE_DMACTL, STE_DMACTL_RXDMA_STALL);
	ste_wait(sc);

	for (i = 0; i < STE_RX_LIST_CNT; i++) {
		if (sc->ste_cdata.ste_rx_chain[i].ste_mbuf != NULL) {
			m_freem(sc->ste_cdata.ste_rx_chain[i].ste_mbuf);
			sc->ste_cdata.ste_rx_chain[i].ste_mbuf = NULL;
		}
	}

	for (i = 0; i < STE_TX_LIST_CNT; i++) {
		if (sc->ste_cdata.ste_tx_chain[i].ste_mbuf != NULL) {
			m_freem(sc->ste_cdata.ste_tx_chain[i].ste_mbuf);
			sc->ste_cdata.ste_tx_chain[i].ste_mbuf = NULL;
		}
	}

	ifp->if_flags &= ~(IFF_RUNNING|IFF_OACTIVE);

	return;
}

static void ste_reset(sc)
	struct ste_softc	*sc;
{
	int			i;

	STE_SETBIT4(sc, STE_ASICCTL,
	    STE_ASICCTL_GLOBAL_RESET|STE_ASICCTL_RX_RESET|
	    STE_ASICCTL_TX_RESET|STE_ASICCTL_DMA_RESET|
	    STE_ASICCTL_FIFO_RESET|STE_ASICCTL_NETWORK_RESET|
	    STE_ASICCTL_AUTOINIT_RESET|STE_ASICCTL_HOST_RESET|
	    STE_ASICCTL_EXTRESET_RESET);

	DELAY(100000);

	for (i = 0; i < STE_TIMEOUT; i++) {
		if (!(CSR_READ_4(sc, STE_ASICCTL) & STE_ASICCTL_RESET_BUSY))
			break;
	}

	if (i == STE_TIMEOUT)
		printf("ste%d: global reset never completed\n", sc->ste_unit);

#ifdef foo
	STE_SETBIT4(sc, STE_ASICCTL, STE_ASICCTL_RX_RESET);
	for (i = 0; i < STE_TIMEOUT; i++) {
		if (!(CSR_READ_4(sc, STE_ASICCTL) & STE_ASICCTL_RX_RESET))
			break;
	}

	if (i == STE_TIMEOUT)
		printf("ste%d: RX reset never completed\n", sc->ste_unit);

	DELAY(100000);

	STE_SETBIT4(sc, STE_ASICCTL, STE_ASICCTL_TX_RESET);
	for (i = 0; i < STE_TIMEOUT; i++) {
		if (!(CSR_READ_4(sc, STE_ASICCTL) & STE_ASICCTL_TX_RESET))
			break;
	}

	if (i == STE_TIMEOUT)
		printf("ste%d: TX reset never completed\n", sc->ste_unit);

	DELAY(100000);
#endif

	return;
}

static int ste_ioctl(ifp, command, data)
	struct ifnet		*ifp;
	u_long			command;
	caddr_t			data;
{
	struct ste_softc	*sc;
	struct ifreq		*ifr;
	int			error = 0, s;

	s = splimp();

	sc = ifp->if_softc;
	ifr = (struct ifreq *)data;

	switch(command) {
	case SIOCSIFADDR:
	case SIOCGIFADDR:
	case SIOCSIFMTU:
		error = ether_ioctl(ifp, command, data);
		break;
	case SIOCSIFFLAGS:
		if (ifp->if_flags & IFF_UP) {
			ste_init(sc);
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				ste_stop(sc);
		}
		error = 0;
		break;
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		ste_setmulti(sc);
		error = 0;
		break;
	case SIOCGIFMEDIA:
	case SIOCSIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->ifmedia, command);
		break;
	default:
		error = EINVAL;
		break;
	}

	splx(s);

	return(error);
}

static int ste_encap(sc, c, m_head)
	struct ste_softc	*sc;
	struct ste_chain	*c;
	struct mbuf		*m_head;
{
	int			frag = 0;
	struct ste_frag		*f = NULL;
	int			total_len;
	struct mbuf		*m;

	m = m_head;
	total_len = 0;

	for (m = m_head, frag = 0; m != NULL; m = m->m_next) {
		if (m->m_len != 0) {
			if (frag == STE_MAXFRAGS)
				break;
			total_len += m->m_len;
			f = &c->ste_ptr->ste_frags[frag];
			f->ste_addr = vtophys(mtod(m, vm_offset_t));
			f->ste_len = m->m_len;
			frag++;
		}
	}

	if (m != NULL) {
		struct mbuf		*m_new = NULL;

		MGETHDR(m_new, M_DONTWAIT, MT_DATA);
		if (m_new == NULL) {
			printf("ste%d: no memory for "
			   "tx list", sc->ste_unit);
			return(1);
		}
		if (m_head->m_pkthdr.len > MHLEN) {
			MCLGET(m_new, M_DONTWAIT);
			if (!(m_new->m_flags & M_EXT)) {
				m_freem(m_new);
				printf("ste%d: no memory for "
			   	    "tx list", sc->ste_unit);
				return(1);
			}
		}
		m_copydata(m_head, 0, m_head->m_pkthdr.len,
		    mtod(m_new, caddr_t));
		m_new->m_pkthdr.len = m_new->m_len = m_head->m_pkthdr.len;
		m_freem(m_head);
		m_head = m_new;
		f = &c->ste_ptr->ste_frags[0];
		f->ste_addr = vtophys(mtod(m_new, caddr_t));
		f->ste_len = total_len = m_new->m_len;
		frag = 1;
	}

	c->ste_mbuf = m_head;
	c->ste_ptr->ste_frags[frag - 1].ste_len |= STE_FRAG_LAST;
	c->ste_ptr->ste_ctl = total_len;
	c->ste_ptr->ste_next = 0;

	return(0);
}

static void ste_start(ifp)
	struct ifnet		*ifp;
{
	struct ste_softc	*sc;
	struct mbuf		*m_head = NULL;
	struct ste_chain	*prev = NULL, *cur_tx = NULL, *start_tx;

	sc = ifp->if_softc;

	if (sc->ste_autoneg) {
		sc->ste_tx_pend = 1;
		return;
	}

	if (sc->ste_cdata.ste_tx_free == NULL) {
		ifp->if_flags |= IFF_OACTIVE;
		return;
	}

	start_tx = sc->ste_cdata.ste_tx_free;

	while(sc->ste_cdata.ste_tx_free != NULL) {
		IF_DEQUEUE(&ifp->if_snd, m_head);
		if (m_head == NULL)
			break;

		cur_tx = sc->ste_cdata.ste_tx_free;
		sc->ste_cdata.ste_tx_free = cur_tx->ste_next;

		cur_tx->ste_next = NULL;

		ste_encap(sc, cur_tx, m_head);

		if (prev != NULL) {
			prev->ste_next = cur_tx;
			prev->ste_ptr->ste_next = vtophys(cur_tx->ste_ptr);
		}
		prev = cur_tx;

#if NBPFILTER > 0
		/*
		 * If there's a BPF listener, bounce a copt of this frame
		 * to him.
	 	 */
		if (ifp->if_bpf)
			bpf_mtap(ifp, cur_tx->ste_mbuf);
#endif
	}

	if (cur_tx == NULL)
		return;

	cur_tx->ste_ptr->ste_ctl |= STE_TXCTL_DMAINTR;

	STE_SETBIT4(sc, STE_DMACTL, STE_DMACTL_TXDMA_STALL);
	ste_wait(sc);

	if (sc->ste_cdata.ste_tx_head != NULL) {
		sc->ste_cdata.ste_tx_tail->ste_next = start_tx;
		sc->ste_cdata.ste_tx_tail->ste_ptr->ste_next =
		    vtophys(start_tx->ste_ptr);
		sc->ste_cdata.ste_tx_tail->ste_ptr->ste_ctl &=
		    ~STE_TXCTL_DMAINTR;
		sc->ste_cdata.ste_tx_tail = cur_tx;
	} else {
		sc->ste_cdata.ste_tx_head = start_tx;
		sc->ste_cdata.ste_tx_tail = cur_tx;
	}

	if (!CSR_READ_4(sc, STE_TX_DMALIST_PTR))
		CSR_WRITE_4(sc, STE_TX_DMALIST_PTR,
		    vtophys(start_tx->ste_ptr));
	STE_SETBIT4(sc, STE_DMACTL, STE_DMACTL_TXDMA_UNSTALL);

	ifp->if_timer = 5;

	return;
}

static void ste_watchdog(ifp)
	struct ifnet		*ifp;
{
	struct ste_softc	*sc;

	sc = ifp->if_softc;

	if (sc->ste_autoneg) {
		ste_autoneg_mii(sc, STE_FLAG_DELAYTIMEO, 1);
		if (!(ifp->if_flags & IFF_UP))
			ste_stop(sc);
		return;
	}

	ifp->if_oerrors++;
	printf("ste%d: watchdog timeout\n", sc->ste_unit);

	if (sc->ste_pinfo != NULL) {
		if (!(ste_phy_readreg(sc, PHY_BMSR) & PHY_BMSR_LINKSTAT))
			printf("ste%d: no carrier - transceiver "
			    "cable problem?\n", sc->ste_unit);
	}

	ste_txeoc(sc);
	ste_txeof(sc);
	ste_rxeof(sc);
	ste_reset(sc);
	ste_init(sc);

	if (ifp->if_snd.ifq_head != NULL)
		ste_start(ifp);

	return;
}

static void ste_shutdown(howto, arg)
	int			howto;
	void			*arg;
{
	struct ste_softc	*sc;

	sc = arg;
	ste_stop(sc);

	return;
}

static struct pci_device ste_device = {
	"ste",
	ste_probe,
	ste_attach,
	&ste_count,
	NULL
};
#ifdef COMPAT_PCI_DRIVER
COMPAT_PCI_DRIVER(ste, ste_device);
#else
DATA_SET(pcidevice_set, ste_device);
#endif /* COMPAT_PCI_DRIVER */

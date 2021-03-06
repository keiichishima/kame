/*	$OpenBSD: if_wi_pcmcia.c,v 1.8 2001/08/17 21:52:16 deraadt Exp $	*/

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
 *	From: if_wi.c,v 1.7 1999/07/04 14:40:22 wpaul Exp $
 */

/*
 * Lucent WaveLAN/IEEE 802.11 PCMCIA driver for OpenBSD.
 *
 * Originally written by Bill Paul <wpaul@ctr.columbia.edu>
 * Electrical Engineering Department
 * Columbia University, New York City
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/timeout.h>
#include <sys/socket.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_ether.h>
#endif

#include <net/if_ieee80211.h>

#include <machine/bus.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciadevs.h>

#include <dev/ic/if_wireg.h>
#include <dev/ic/if_wi_ieee.h>
#include <dev/ic/if_wivar.h>

int	wi_pcmcia_match		__P((struct device *, void *, void *));
void	wi_pcmcia_attach	__P((struct device *, struct device *, void *));
int	wi_pcmcia_detach	__P((struct device *, int));
int	wi_pcmcia_activate	__P((struct device *, enum devact));
void	wi_pcmcia_attach	__P((struct device *, struct device *, void *));

int	wi_intr			__P((void *));
int	wi_attach		__P((struct wi_softc *, int));
void	wi_init			__P((void *));
void	wi_stop			__P((struct wi_softc *));

struct wi_pcmcia_softc {
	struct wi_softc sc_wi;

	struct pcmcia_io_handle	sc_pcioh;
	int			sc_io_window;
	struct pcmcia_function	*sc_pf;
};

struct cfattach wi_pcmcia_ca = {
	sizeof (struct wi_pcmcia_softc), wi_pcmcia_match, wi_pcmcia_attach,
	wi_pcmcia_detach, wi_pcmcia_activate
};

static const struct wi_pcmcia_product {
	u_int16_t	pp_vendor;
	u_int16_t	pp_product;
	const char	*pp_cisinfo[4];
	const char	*pp_name;
} wi_pcmcia_products[] = {
	{ PCMCIA_VENDOR_LUCENT,
	  PCMCIA_PRODUCT_LUCENT_WAVELAN_IEEE,
	  PCMCIA_CIS_LUCENT_WAVELAN_IEEE,
	  "WaveLAN/IEEE"
	},
	{ PCMCIA_VENDOR_3COM,
	  PCMCIA_PRODUCT_3COM_3CRWE737A,
	  PCMCIA_CIS_3COM_3CRWE737A,
	  "3Com AirConnect Wireless LAN"
	},
	{ PCMCIA_VENDOR_COREGA,
	  PCMCIA_PRODUCT_COREGA_WIRELESS_LAN_PCC_11,
	  PCMCIA_CIS_COREGA_WIRELESS_LAN_PCC_11,
	  "Corega Wireless LAN PCC-11"
	},
	{ PCMCIA_VENDOR_COREGA,
	  PCMCIA_PRODUCT_COREGA_WIRELESS_LAN_PCCA_11,
	  PCMCIA_CIS_COREGA_WIRELESS_LAN_PCCA_11,
	  "Corega Wireless LAN PCCA-11",
	},
	{ PCMCIA_VENDOR_INTERSIL,
	  PCMCIA_PRODUCT_INTERSIL_PRISM2,
	  PCMCIA_CIS_INTERSIL_PRISM2,
	  "Intersil Prism II",
	},
	{ PCMCIA_VENDOR_SAMSUNG,
	  PCMCIA_PRODUCT_SAMSUNG_SWL_2000N,
	  PCMCIA_CIS_SAMSUNG_SWL_2000N,
	  "Samsung MagicLAN SWL-2000N",
	},
	{ PCMCIA_VENDOR_LUCENT,
	  PCMCIA_PRODUCT_LUCENT_WAVELAN_IEEE,
	  PCMCIA_CIS_SMC_2632W,
	  "SMC 2632 EZ Connect Wireless PC Card",
	},
	{ PCMCIA_VENDOR_LUCENT,
	  PCMCIA_PRODUCT_LUCENT_WAVELAN_IEEE,
	  PCMCIA_CIS_NANOSPEED_PRISM2,
	  "NANOSPEED ROOT-RZ2000 WLAN Card",
	},
	{ PCMCIA_VENDOR_ELSA,
	  PCMCIA_PRODUCT_ELSA_XI300_IEEE,
	  PCMCIA_CIS_ELSA_XI300_IEEE,
	  "XI300 Wireless LAN",
	},
	{ PCMCIA_VENDOR_COMPAQ,
	  PCMCIA_PRODUCT_COMPAQ_NC5004,
	  PCMCIA_CIS_COMPAQ_NC5004,
	  "Compaq Agency NC5004 Wireless Card",
	},
	{ PCMCIA_VENDOR_CONTEC,
	  PCMCIA_PRODUCT_CONTEC_FX_DS110_PCC,
	  PCMCIA_CIS_CONTEC_FX_DS110_PCC,
	  "Contec FLEXLAN/FX-DS110-PCC",
	},
	{ PCMCIA_VENDOR_TDK,
	  PCMCIA_PRODUCT_TDK_LAK_CD011WL,
	  PCMCIA_CIS_TDK_LAK_CD011WL,
	  "TDK LAK-CD011WL",
	},
	{ PCMCIA_VENDOR_LUCENT,
	  PCMCIA_PRODUCT_LUCENT_WAVELAN_IEEE,
	  PCMCIA_CIS_NEC_CMZ_RT_WP,
	  "NEC Wireless Card CMZ-RT-WP",
	},
	{ PCMCIA_VENDOR_LUCENT,
	  PCMCIA_PRODUCT_LUCENT_WAVELAN_IEEE,
	  PCMCIA_CIS_NTT_ME_WLAN,
	  "NTT-ME 11Mbps Wireless LAN PC Card",
	},
	{ PCMCIA_VENDOR_ADDTRON,
	  PCMCIA_PRODUCT_ADDTRON_AWP100,
	  PCMCIA_CIS_ADDTRON_AWP100,
	  "Addtron AWP-100",
	},
	{ PCMCIA_VENDOR_LUCENT,
	  PCMCIA_PRODUCT_LUCENT_WAVELAN_IEEE,
	  PCMCIA_CIS_CABLETRON_ROAMABOUT,
	  "Cabletron RoamAbout",
	},
	{ PCMCIA_VENDOR_IODATA2,
	  PCMCIA_PRODUCT_IODATA2_WNB11PCM,
	  PCMCIA_CIS_IODATA2_WNB11PCM,
	  "I-O DATA WN-B11/PCM",
	},
	{ 0,
	  0,
	  { NULL, NULL, NULL, NULL },
	  NULL,
	}
};

static const struct wi_pcmcia_product *wi_lookup __P((struct pcmcia_attach_args *pa));

const struct wi_pcmcia_product *
wi_lookup(pa)
	struct pcmcia_attach_args *pa;
{
	const struct wi_pcmcia_product *pp;

	/*
	 * Several PRISM II-based cards use the Lucent WaveLAN vendor
	 * and product IDs so we match by CIS information first.
	 */
	for (pp = wi_pcmcia_products; pp->pp_name != NULL; pp++) {
		if (pa->card->cis1_info[0] != NULL &&
		    pp->pp_cisinfo[0] != NULL &&
		    strcmp(pa->card->cis1_info[0], pp->pp_cisinfo[0]) == 0 &&
		    pa->card->cis1_info[1] != NULL &&
		    pp->pp_cisinfo[1] != NULL &&
		    strcmp(pa->card->cis1_info[1], pp->pp_cisinfo[1]) == 0)
			return (pp);
	}

	/* Match by vendor/product ID. */
	for (pp = wi_pcmcia_products; pp->pp_name != NULL; pp++) {
		if (pa->manufacturer != PCMCIA_VENDOR_INVALID &&
		    pa->manufacturer == pp->pp_vendor &&
		    pa->product != PCMCIA_PRODUCT_INVALID &&
		    pa->product == pp->pp_product)
			return (pp);
	}

	return (NULL);
}

int
wi_pcmcia_match(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct pcmcia_attach_args *pa = aux;

	if (wi_lookup(pa) != NULL)
		return (1);
	return (0);
}

void
wi_pcmcia_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct wi_pcmcia_softc	*psc = (struct wi_pcmcia_softc *)self;
	struct wi_softc		*sc = &psc->sc_wi;
	struct pcmcia_attach_args *pa = aux;
	struct pcmcia_function	*pf = pa->pf;
	struct pcmcia_config_entry *cfe = pf->cfe_head.sqh_first;
	int			state = 0;

	psc->sc_pf = pf;

	/* Enable the card. */
	pcmcia_function_init(pf, cfe);
	if (pcmcia_function_enable(pf)) {
		printf(": function enable failed\n");
		goto bad;
	}
	state++;

	if (pcmcia_io_alloc(pf, 0, WI_IOSIZ, WI_IOSIZ, &psc->sc_pcioh)) {
		printf(": can't alloc i/o space\n");
		goto bad;
	}
	state++;

	if (pcmcia_io_map(pf, PCMCIA_WIDTH_IO16, 0, WI_IOSIZ,
	    &psc->sc_pcioh, &psc->sc_io_window)) {
		printf(": can't map io space\n");
		goto bad;
	}
	state++;

	sc->wi_btag = psc->sc_pcioh.iot;
	sc->wi_bhandle = psc->sc_pcioh.ioh;

	/* Make sure interrupts are disabled. */
	CSR_WRITE_2(sc, WI_INT_EN, 0);
	CSR_WRITE_2(sc, WI_EVENT_ACK, 0xffff);

	/* Establish the interrupt. */
	sc->sc_ih = pcmcia_intr_establish(pa->pf, IPL_NET, wi_intr, psc, "");
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt\n",
		    sc->sc_dev.dv_xname);
		goto bad;
	}

	wi_attach(sc, 0);
	return;

bad:
	if (state > 2)
		pcmcia_io_unmap(pf, psc->sc_io_window);
	if (state > 1)
		pcmcia_io_free(pf, &psc->sc_pcioh);
	if (state > 0)
		pcmcia_function_disable(pf);
}

int
wi_pcmcia_detach(dev, flags)
	struct device *dev;
	int flags;
{
	struct wi_pcmcia_softc *psc = (struct wi_pcmcia_softc *)dev;
	struct wi_softc *sc = &psc->sc_wi;
	struct ifnet *ifp = &sc->arpcom.ac_if;

	pcmcia_io_unmap(psc->sc_pf, psc->sc_io_window);
	pcmcia_io_free(psc->sc_pf, &psc->sc_pcioh);

	ether_ifdetach(ifp);
	if_detach(ifp);

	return (0);
}

int
wi_pcmcia_activate(dev, act)
	struct device *dev;
	enum devact act;
{
	struct wi_pcmcia_softc *psc = (struct wi_pcmcia_softc *)dev;
	struct wi_softc *sc = &psc->sc_wi;
	struct ifnet *ifp = &sc->arpcom.ac_if;
	int s;

	s = splnet();
	switch (act) {
	case DVACT_ACTIVATE:
		pcmcia_function_enable(psc->sc_pf);
		sc->sc_ih = pcmcia_intr_establish(psc->sc_pf, IPL_NET,
		    wi_intr, sc, sc->sc_dev.dv_xname);
		wi_init(sc);
		break;

	case DVACT_DEACTIVATE:
		ifp->if_timer = 0;
		if (ifp->if_flags & IFF_RUNNING)
			wi_stop(sc);
		pcmcia_intr_disestablish(psc->sc_pf, sc->sc_ih);
		pcmcia_function_disable(psc->sc_pf);
		break;
	}
	splx(s);
	return (0);
}

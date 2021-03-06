/*	$NetBSD: if_tlvar.h,v 1.3 1999/01/11 22:45:42 tron Exp $	*/

/*
 * Copyright (c) 1997 Manuel Bouyer.  All rights reserved.
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
 *  This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Texas Instruments ThunderLAN ethernet controller
 * ThunderLAN Programmer's Guide (TI Literature Number SPWU013A)
 * available from www.ti.com
 */

struct tl_product_desc {
	u_int32_t tp_product;
	int tp_tlphymedia;
	const char *tp_desc;
};

struct tl_softc {
	struct device sc_dev;		/* base device */
	bus_space_tag_t tl_bustag;
	bus_space_handle_t tl_bushandle; /* CSR region handle */
	const struct tl_product_desc *tl_product;
	void* tl_ih;
	struct ethercom tl_ec;
	u_int8_t tl_enaddr[ETHER_ADDR_LEN];	/* hardware adress */
	u_int16_t tl_flags;
#define TL_IFACT 0x0001 /* chip has interface activity */
	u_int8_t tl_lasttx; /* we were without input this many seconds */
	i2c_adapter_t i2cbus;		/* i2c bus, for eeprom */
	mii_data_t tl_mii;		/* mii bus */
	struct Rx_list *Rx_list;	/* Receive and transmit lists */
	struct Tx_list *Tx_list;
	struct Rx_list *active_Rx, *last_Rx;
	struct Tx_list *active_Tx, *last_Tx;
	struct Tx_list *Free_Tx;
	int opkt;		/* used to detect link up/down for AUI/BNC */
	int stats_exesscoll;	/* idem */
#ifdef TL_PRIV_STATS
	int ierr_overr;
	int ierr_code;
	int ierr_crc;
	int ierr_nomem;
	int oerr_underr;
	int oerr_deffered;
	int oerr_coll;
	int oerr_multicoll;
	int oerr_latecoll;
	int oerr_exesscoll;
	int oerr_carrloss;
	int oerr_mcopy;
#endif
};
#define tl_if            tl_ec.ec_if
#define tl_bpf   tl_if.if_bpf

typedef struct tl_softc tl_softc_t;
typedef u_long ioctl_cmd_t;

#define TL_HR_READ(sc, reg) \
	bus_space_read_4(sc->tl_bustag, sc->tl_bushandle, (reg))
#define TL_HR_READ_BYTE(sc, reg) \
	bus_space_read_1(sc->tl_bustag, sc->tl_bushandle, (reg))
#define TL_HR_WRITE(sc, reg, data) \
	bus_space_write_4(sc->tl_bustag, sc->tl_bushandle, (reg), (data))
#define TL_HR_WRITE_BYTE(sc, reg, data) \
	bus_space_write_1(sc->tl_bustag, sc->tl_bushandle, (reg), (data))
#define ETHER_MIN_TX (ETHERMIN + sizeof(struct ether_header))

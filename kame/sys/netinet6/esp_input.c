/*	$KAME: esp_input.c,v 1.91 2004/11/11 22:34:45 suz Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * RFC1827/2406 Encapsulated Security Payload.
 */

#ifdef __FreeBSD__
#include "opt_inet.h"
#include "opt_inet6.h"
#endif
#ifdef __NetBSD__
#include "opt_inet.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#ifndef __FreeBSD__
#include <sys/malloc.h>
#endif
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/time.h>
#ifndef __FreeBSD__
#include <sys/kernel.h>
#endif
#include <sys/syslog.h>

#include <net/if.h>
#include <net/route.h>
#include <net/netisr.h>
#if defined(__FreeBSD__) && __FreeBSD_version >= 502010
#include <net/pfil.h>
#endif
#include <machine/cpu.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/in_var.h>
#include <netinet/ip_ecn.h>
#if defined(__NetBSD__) && __NetBSD_Version__ >= 105080000	/* 1.5H */
#include <netinet/ip_icmp.h>
#endif

#ifdef INET6
#include <netinet/ip6.h>
#if defined(__OpenBSD__) || defined(__FreeBSD__)
#include <netinet/in_pcb.h>
#else
#include <netinet6/in6_pcb.h>
#endif
#include <netinet6/ip6_var.h>
#include <netinet/icmp6.h>
#include <netinet6/ip6protosw.h>
#endif

#include <netinet6/ipsec.h>
#include <netinet6/ah.h>
#include <netinet6/esp.h>
#include <netkey/key.h>
#include <netkey/keydb.h>
#include <netkey/key_debug.h>

#include <machine/stdarg.h>

#include <net/net_osdep.h>

#define IPLEN_FLIPPED

#define ESPMAXLEN \
	(sizeof(struct esp) < sizeof(struct newesp) \
		? sizeof(struct newesp) : sizeof(struct esp))

#ifdef INET
extern struct protosw inetsw[];
#ifdef __NetBSD__
extern u_char ip_protox[];
#endif

void
#ifdef __FreeBSD__
esp4_input(m, off)
	struct mbuf *m;
	int off;
#else
#if __STDC__
esp4_input(struct mbuf *m, ...)
#else
esp4_input(m, va_alist)
	struct mbuf *m;
	va_dcl
#endif
#endif
{
	struct ip *ip;
	struct esp *esp;
	struct esptail esptail;
	u_int32_t spi;
	struct secasvar *sav = NULL;
	size_t taillen;
	u_int16_t nxt;
	const struct esp_algorithm *algo;
	int ivlen;
	size_t hlen;
	size_t esplen;
#if !(defined(__FreeBSD__) && __FreeBSD_version >= 500000)
	int s;
#endif
#if !(defined(__FreeBSD__) && __FreeBSD__ >= 4)
	va_list ap;
	int off;
#endif

#ifndef __FreeBSD__
	va_start(ap, m);
	off = va_arg(ap, int);
	(void)va_arg(ap, int);		/* ignore value, advance ap */
	va_end(ap);
#endif

	/* sanity check for alignment. */
	if (off % 4 != 0 || m->m_pkthdr.len % 4 != 0) {
		ipseclog((LOG_ERR, "IPv4 ESP input: packet alignment problem "
			"(off=%d, pktlen=%d)\n", off, m->m_pkthdr.len));
		ipsecstat.in_inval++;
		goto bad;
	}

	if (m->m_len < off + ESPMAXLEN) {
		m = m_pullup(m, off + ESPMAXLEN);
		if (!m) {
			ipseclog((LOG_DEBUG,
			    "IPv4 ESP input: can't pullup in esp4_input\n"));
			ipsecstat.in_inval++;
			goto bad;
		}
	}

	ip = mtod(m, struct ip *);
	esp = (struct esp *)(((u_int8_t *)ip) + off);
#ifdef _IP_VHL
	hlen = IP_VHL_HL(ip->ip_vhl) << 2;
#else
	hlen = ip->ip_hl << 2;
#endif

	/* find the sassoc. */
	spi = esp->esp_spi;

	if ((sav = key_allocsa(AF_INET, (caddr_t)&ip->ip_src,
	    (caddr_t)&ip->ip_dst, IPPROTO_ESP, spi)) == 0) {
		ipseclog((LOG_WARNING,
		    "IPv4 ESP input: no key association found for spi %u\n",
		    (u_int32_t)ntohl(spi)));
		ipsecstat.in_nosa++;
		goto bad;
	}
	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		printf("DP esp4_input called to allocate SA:%p\n", sav));
	if (sav->state != SADB_SASTATE_MATURE &&
	    sav->state != SADB_SASTATE_DYING) {
		ipseclog((LOG_DEBUG,
		    "IPv4 ESP input: non-mature/dying SA found for spi %u\n",
		    (u_int32_t)ntohl(spi)));
		ipsecstat.in_badspi++;
		goto bad;
	}
	algo = esp_algorithm_lookup(sav->alg_enc);
	if (!algo) {
		ipseclog((LOG_DEBUG, "IPv4 ESP input: "
		    "unsupported encryption algorithm for spi %u\n",
		    (u_int32_t)ntohl(spi)));
		ipsecstat.in_badspi++;
		goto bad;
	}

	/* check if we have proper ivlen information */
	ivlen = sav->ivlen;
	if (ivlen < 0) {
		ipseclog((LOG_ERR, "inproper ivlen in IPv4 ESP input: %s %s\n",
		    ipsec4_logpacketstr(ip, spi), ipsec_logsastr(sav)));
		ipsecstat.in_inval++;
		goto bad;
	}

	if (!((sav->flags & SADB_X_EXT_OLD) == 0 && sav->replay &&
	    sav->alg_auth && sav->key_auth))
		goto noreplaycheck;

	if (sav->alg_auth == SADB_X_AALG_NULL ||
	    sav->alg_auth == SADB_AALG_NONE)
		goto noreplaycheck;

	/*
	 * check for sequence number.
	 */
	if (ipsec_chkreplay(ntohl(((struct newesp *)esp)->esp_seq), sav))
		; /* okey */
	else {
		ipsecstat.in_espreplay++;
		ipseclog((LOG_WARNING,
		    "replay packet in IPv4 ESP input: %s %s\n",
		    ipsec4_logpacketstr(ip, spi), ipsec_logsastr(sav)));
		goto bad;
	}

	/* check ICV */
    {
	u_int8_t sum0[AH_MAXSUMSIZE];
	u_int8_t sum[AH_MAXSUMSIZE];
	const struct ah_algorithm *sumalgo;
	size_t siz;

	sumalgo = ah_algorithm_lookup(sav->alg_auth);
	if (!sumalgo)
		goto noreplaycheck;
	siz = (((*sumalgo->sumsiz)(sav) + 3) & ~(4 - 1));
	if (m->m_pkthdr.len < off + ESPMAXLEN + siz) {
		ipsecstat.in_inval++;
		goto bad;
	}
	if (AH_MAXSUMSIZE < siz) {
		ipseclog((LOG_DEBUG,
		    "internal error: AH_MAXSUMSIZE must be larger than %lu\n",
		    (u_long)siz));
		ipsecstat.in_inval++;
		goto bad;
	}

	m_copydata(m, m->m_pkthdr.len - siz, siz, (caddr_t)&sum0[0]);

	if (esp_auth(m, off, m->m_pkthdr.len - off - siz, sav, sum)) {
		ipseclog((LOG_WARNING, "auth fail in IPv4 ESP input: %s %s\n",
		    ipsec4_logpacketstr(ip, spi), ipsec_logsastr(sav)));
		ipsecstat.in_espauthfail++;
		goto bad;
	}

	if (bcmp(sum0, sum, siz) != 0) {
		ipseclog((LOG_WARNING, "auth fail in IPv4 ESP input: %s %s\n",
		    ipsec4_logpacketstr(ip, spi), ipsec_logsastr(sav)));
		ipsecstat.in_espauthfail++;
		goto bad;
	}

	/* strip off the authentication data */
	m_adj(m, -siz);
	ip = mtod(m, struct ip *);
#ifdef IPLEN_FLIPPED
	ip->ip_len = ip->ip_len - siz;
#else
	ip->ip_len = htons(ntohs(ip->ip_len) - siz);
#endif
	m->m_flags |= M_AUTHIPDGM;
	ipsecstat.in_espauthsucc++;
    }

	/*
	 * update sequence number.
	 */
	if ((sav->flags & SADB_X_EXT_OLD) == 0 && sav->replay) {
		if (ipsec_updatereplay(ntohl(((struct newesp *)esp)->esp_seq), sav)) {
			ipsecstat.in_espreplay++;
			goto bad;
		}
	}

noreplaycheck:

	/* process main esp header. */
	if (sav->flags & SADB_X_EXT_OLD) {
		/* RFC 1827 */
		esplen = sizeof(struct esp);
	} else {
		/* RFC 2406 */
		if (sav->flags & SADB_X_EXT_DERIV)
			esplen = sizeof(struct esp);
		else
			esplen = sizeof(struct newesp);
	}

	if (m->m_pkthdr.len < off + esplen + ivlen + sizeof(esptail)) {
		ipseclog((LOG_WARNING,
		    "IPv4 ESP input: packet too short\n"));
		ipsecstat.in_inval++;
		goto bad;
	}

	if (m->m_len < off + esplen + ivlen) {
		m = m_pullup(m, off + esplen + ivlen);
		if (!m) {
			ipseclog((LOG_DEBUG,
			    "IPv4 ESP input: can't pullup in esp4_input\n"));
			ipsecstat.in_inval++;
			goto bad;
		}
	}

	/*
	 * pre-compute and cache intermediate key
	 */
	if (esp_schedule(algo, sav) != 0) {
		ipsecstat.in_inval++;
		goto bad;
	}

	/*
	 * decrypt the packet.
	 */
	if (!algo->decrypt)
		panic("internal error: no decrypt function");
	if ((*algo->decrypt)(m, off, sav, algo, ivlen)) {
		/* m is already freed */
		m = NULL;
		ipseclog((LOG_ERR, "decrypt fail in IPv4 ESP input: %s\n",
		    ipsec_logsastr(sav)));
		ipsecstat.in_inval++;
		goto bad;
	}
	ipsecstat.in_esphist[sav->alg_enc]++;

	m->m_flags |= M_DECRYPTED;

	/*
	 * find the trailer of the ESP.
	 */
	m_copydata(m, m->m_pkthdr.len - sizeof(esptail), sizeof(esptail),
	     (caddr_t)&esptail);
	nxt = esptail.esp_nxt;
	taillen = esptail.esp_padlen + sizeof(esptail);

	if (m->m_pkthdr.len < taillen ||
	    m->m_pkthdr.len - taillen < off + esplen + ivlen + sizeof(esptail)) {
		ipseclog((LOG_WARNING,
		    "bad pad length in IPv4 ESP input: %s %s\n",
		    ipsec4_logpacketstr(ip, spi), ipsec_logsastr(sav)));
		ipsecstat.in_inval++;
		goto bad;
	}

	/* strip off the trailing pad area. */
	m_adj(m, -taillen);

#ifdef IPLEN_FLIPPED
	ip->ip_len = ip->ip_len - taillen;
#else
	ip->ip_len = htons(ntohs(ip->ip_len) - taillen);
#endif

	/* was it transmitted over the IPsec tunnel SA? */
	if (ipsec4_tunnel_validate(m, off + esplen + ivlen, nxt, sav)) {
		/*
		 * strip off all the headers that precedes ESP header.
		 *	IP4 xx ESP IP4' payload -> IP4' payload
		 *
		 * XXX more sanity checks
		 * XXX relationship with gif?
		 */
		u_int8_t tos;

		tos = ip->ip_tos;
		m_adj(m, off + esplen + ivlen);
		if (m->m_len < sizeof(*ip)) {
			m = m_pullup(m, sizeof(*ip));
			if (!m) {
				ipsecstat.in_inval++;
				goto bad;
			}
		}
		ip = mtod(m, struct ip *);
		/* ECN consideration. */
		if (!ip_ecn_egress(ip4_ipsec_ecn, &tos, &ip->ip_tos)) {
			ipsecstat.in_inval++;
			goto bad;
		}
		if (!key_checktunnelsanity(sav, AF_INET,
			    (caddr_t)&ip->ip_src, (caddr_t)&ip->ip_dst)) {
			ipseclog((LOG_ERR, "ipsec tunnel address mismatch "
			    "in IPv4 ESP input: %s %s\n",
			    ipsec4_logpacketstr(ip, spi), ipsec_logsastr(sav)));
			ipsecstat.in_inval++;
			goto bad;
		}

		key_sa_recordxfer(sav, m);
		if (ipsec_addhist(m, IPPROTO_ESP, spi) != 0 ||
		    ipsec_addhist(m, IPPROTO_IPV4, 0) != 0) {
			ipsecstat.in_nomem++;
			goto bad;
		}

#ifdef __NetBSD__
		s = splnet();
#elif !(defined(__FreeBSD__) && __FreeBSD_version >= 500000)
		s = splimp();
#endif
#if defined(__FreeBSD__) && __FreeBSD_version >= 500000
		if (netisr_queue(NETISR_IP, m)) {	/* (0) on success. */
			ipsecstat.in_inval++;
			m = NULL;
			goto bad;
		}
#else
		if (IF_QFULL(&ipintrq)) {
			ipsecstat.in_inval++;
			splx(s);
			goto bad;
		}
		IF_ENQUEUE(&ipintrq, m);
		schednetisr(NETISR_IP); /* can be skipped but to make sure */
		splx(s);
#endif
		m = NULL;
		nxt = IPPROTO_DONE;
	} else {
		/*
		 * strip off ESP header and IV.
		 * even in m_pulldown case, we need to strip off ESP so that
		 * we can always compute checksum for AH correctly.
		 */
		size_t stripsiz;

		stripsiz = esplen + ivlen;

		ip = mtod(m, struct ip *);
		ovbcopy((caddr_t)ip, (caddr_t)(((u_char *)ip) + stripsiz), off);
		m->m_data += stripsiz;
		m->m_len -= stripsiz;
		m->m_pkthdr.len -= stripsiz;

		ip = mtod(m, struct ip *);
#ifdef IPLEN_FLIPPED
		ip->ip_len = ip->ip_len - stripsiz;
#else
		ip->ip_len = htons(ntohs(ip->ip_len) - stripsiz);
#endif
		ip->ip_p = nxt;

		key_sa_recordxfer(sav, m);
		if (ipsec_addhist(m, IPPROTO_ESP, spi) != 0) {
			ipsecstat.in_nomem++;
			goto bad;
		}

		if (nxt != IPPROTO_DONE) {
			if ((inetsw[ip_protox[nxt]].pr_flags & PR_LASTHDR) != 0 &&
			    ipsec4_in_reject(m, NULL)) {
				ipsecstat.in_polvio++;
				goto bad;
			}
#ifdef __FreeBSD__
			(*inetsw[ip_protox[nxt]].pr_input)(m, off);
#else
			(*inetsw[ip_protox[nxt]].pr_input)(m, off, nxt);
#endif
		} else
			m_freem(m);
		m = NULL;
	}

	if (sav) {
		KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
			printf("DP esp4_input call free SA:%p\n", sav));
		key_freesav(sav);
	}
	ipsecstat.in_success++;
	return;

bad:
	if (sav) {
		KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
			printf("DP esp4_input call free SA:%p\n", sav));
		key_freesav(sav);
	}
	if (m)
		m_freem(m);
	return;
}

#if defined(__NetBSD__) && __NetBSD_Version__ >= 105080000	/* 1.5H */
/* assumes that ip header and esp header are contiguous on mbuf */
void *
esp4_ctlinput(cmd, sa, v)
	int cmd;
	struct sockaddr *sa;
	void *v;
{
	struct ip *ip = v;
	struct esp *esp;
	struct icmp *icp;
	struct secasvar *sav;

	if (sa->sa_family != AF_INET ||
	    sa->sa_len != sizeof(struct sockaddr_in))
		return NULL;
	if ((unsigned)cmd >= PRC_NCMDS)
		return NULL;
	if (cmd == PRC_MSGSIZE && ip_mtudisc && ip && ip->ip_v == 4) {
		/*
		 * Check to see if we have a valid SA corresponding to
		 * the address in the ICMP message payload.
		 */
		esp = (struct esp *)((caddr_t)ip + (ip->ip_hl << 2));
		if ((sav = key_allocsa(AF_INET, (caddr_t) &ip->ip_src,
		    (caddr_t) &ip->ip_dst, IPPROTO_ESP, esp->esp_spi)) == NULL)
			return NULL;
		if (sav->state != SADB_SASTATE_MATURE &&
		    sav->state != SADB_SASTATE_DYING) {
			key_freesav(sav);
			return NULL;
		}

		/* XXX Further validation? */

		key_freesav(sav);

		/*
		 * Now that we've validated that we are actually communicating
		 * with the host indicated in the ICMP message, locate the
		 * ICMP header, recalculate the new MTU, and create the
		 * corresponding routing entry.
		 */
		icp = (struct icmp *)((caddr_t)ip -
		    offsetof(struct icmp, icmp_ip));
		icmp_mtudisc(icp, ip->ip_dst);

		return NULL;
	}

	return NULL;
}
#endif /* NetBSD 1.5H or later */
#endif /* INET */

#ifdef INET6
int
esp6_input(mp, offp, proto)
	struct mbuf **mp;
	int *offp, proto;
{
	struct mbuf *m = *mp;
	int off = *offp;
	struct ip6_hdr *ip6;
	struct sockaddr_in6 src_sa, dst_sa;
	struct esp *esp;
	struct esptail esptail;
	u_int32_t spi;
	struct secasvar *sav = NULL;
	size_t taillen;
	u_int16_t nxt;
	const struct esp_algorithm *algo;
	int ivlen;
	size_t esplen;
#if !(defined(__FreeBSD__) && __FreeBSD_version >= 500000)
	int s;
#endif

	/* sanity check for alignment. */
	if (off % 4 != 0 || m->m_pkthdr.len % 4 != 0) {
		ipseclog((LOG_ERR, "IPv6 ESP input: packet alignment problem "
			"(off=%d, pktlen=%d)\n", off, m->m_pkthdr.len));
		ipsec6stat.in_inval++;
		goto bad;
	}

#ifndef PULLDOWN_TEST
	IP6_EXTHDR_CHECK(m, off, ESPMAXLEN, IPPROTO_DONE);
	esp = (struct esp *)(mtod(m, caddr_t) + off);
#else
	IP6_EXTHDR_GET(esp, struct esp *, m, off, ESPMAXLEN);
	if (esp == NULL) {
		ipsec6stat.in_inval++;
		return IPPROTO_DONE;
	}
#endif
	ip6 = mtod(m, struct ip6_hdr *);

	if (ntohs(ip6->ip6_plen) == 0) {
		ipseclog((LOG_ERR, "IPv6 ESP input: "
		    "ESP with IPv6 jumbogram is not supported.\n"));
		ipsec6stat.in_inval++;
		goto bad;
	}

	/* find the sassoc. */
	spi = esp->esp_spi;

	if ((sav = key_allocsa(AF_INET6, (caddr_t)&ip6->ip6_src,
	    (caddr_t)&ip6->ip6_dst, IPPROTO_ESP, spi)) == 0) {
		ipseclog((LOG_WARNING,
		    "IPv6 ESP input: no key association found for spi %u\n",
		    (u_int32_t)ntohl(spi)));
		ipsec6stat.in_nosa++;
		goto bad;
	}
	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		printf("DP esp6_input called to allocate SA:%p\n", sav));
	if (sav->state != SADB_SASTATE_MATURE &&
	    sav->state != SADB_SASTATE_DYING) {
		ipseclog((LOG_DEBUG,
		    "IPv6 ESP input: non-mature/dying SA found for spi %u\n",
		    (u_int32_t)ntohl(spi)));
		ipsec6stat.in_badspi++;
		goto bad;
	}
	algo = esp_algorithm_lookup(sav->alg_enc);
	if (!algo) {
		ipseclog((LOG_DEBUG, "IPv6 ESP input: "
		    "unsupported encryption algorithm for spi %u\n",
		    (u_int32_t)ntohl(spi)));
		ipsec6stat.in_badspi++;
		goto bad;
	}

	/* check if we have proper ivlen information */
	ivlen = sav->ivlen;
	if (ivlen < 0) {
		ipseclog((LOG_ERR, "inproper ivlen in IPv6 ESP input: %s %s\n",
		    ipsec6_logpacketstr(ip6, spi), ipsec_logsastr(sav)));
		ipsec6stat.in_badspi++;
		goto bad;
	}

	if (!((sav->flags & SADB_X_EXT_OLD) == 0 && sav->replay &&
	    sav->alg_auth && sav->key_auth))
		goto noreplaycheck;

	if (sav->alg_auth == SADB_X_AALG_NULL ||
	    sav->alg_auth == SADB_AALG_NONE)
		goto noreplaycheck;

	/*
	 * check for sequence number.
	 */
	if (ipsec_chkreplay(ntohl(((struct newesp *)esp)->esp_seq), sav))
		; /* okey */
	else {
		ipsec6stat.in_espreplay++;
		ipseclog((LOG_WARNING,
		    "replay packet in IPv6 ESP input: %s %s\n",
		    ipsec6_logpacketstr(ip6, spi), ipsec_logsastr(sav)));
		goto bad;
	}

	/* check ICV */
    {
	u_char sum0[AH_MAXSUMSIZE];
	u_char sum[AH_MAXSUMSIZE];
	const struct ah_algorithm *sumalgo;
	size_t siz;

	sumalgo = ah_algorithm_lookup(sav->alg_auth);
	if (!sumalgo)
		goto noreplaycheck;
	siz = (((*sumalgo->sumsiz)(sav) + 3) & ~(4 - 1));
	if (m->m_pkthdr.len < off + ESPMAXLEN + siz) {
		ipsec6stat.in_inval++;
		goto bad;
	}
	if (AH_MAXSUMSIZE < siz) {
		ipseclog((LOG_DEBUG,
		    "internal error: AH_MAXSUMSIZE must be larger than %lu\n",
		    (u_long)siz));
		ipsec6stat.in_inval++;
		goto bad;
	}

	m_copydata(m, m->m_pkthdr.len - siz, siz, (caddr_t)&sum0[0]);

	if (esp_auth(m, off, m->m_pkthdr.len - off - siz, sav, sum)) {
		ipseclog((LOG_WARNING, "auth fail in IPv6 ESP input: %s %s\n",
		    ipsec6_logpacketstr(ip6, spi), ipsec_logsastr(sav)));
		ipsec6stat.in_espauthfail++;
		goto bad;
	}

	if (bcmp(sum0, sum, siz) != 0) {
		ipseclog((LOG_WARNING, "auth fail in IPv6 ESP input: %s %s\n",
		    ipsec6_logpacketstr(ip6, spi), ipsec_logsastr(sav)));
		ipsec6stat.in_espauthfail++;
		goto bad;
	}

	/* strip off the authentication data */
	m_adj(m, -siz);
	ip6 = mtod(m, struct ip6_hdr *);
	ip6->ip6_plen = htons(ntohs(ip6->ip6_plen) - siz);

	m->m_flags |= M_AUTHIPDGM;
	ipsec6stat.in_espauthsucc++;
    }

	/*
	 * update sequence number.
	 */
	if ((sav->flags & SADB_X_EXT_OLD) == 0 && sav->replay) {
		if (ipsec_updatereplay(ntohl(((struct newesp *)esp)->esp_seq), sav)) {
			ipsec6stat.in_espreplay++;
			goto bad;
		}
	}

noreplaycheck:

	/* process main esp header. */
	if (sav->flags & SADB_X_EXT_OLD) {
		/* RFC 1827 */
		esplen = sizeof(struct esp);
	} else {
		/* RFC 2406 */
		if (sav->flags & SADB_X_EXT_DERIV)
			esplen = sizeof(struct esp);
		else
			esplen = sizeof(struct newesp);
	}

	if (m->m_pkthdr.len < off + esplen + ivlen + sizeof(esptail)) {
		ipseclog((LOG_WARNING,
		    "IPv6 ESP input: packet too short\n"));
		ipsec6stat.in_inval++;
		goto bad;
	}

#ifndef PULLDOWN_TEST
	IP6_EXTHDR_CHECK(m, off, esplen + ivlen, IPPROTO_DONE);	/* XXX */
#else
	IP6_EXTHDR_GET(esp, struct esp *, m, off, esplen + ivlen);
	if (esp == NULL) {
		ipsec6stat.in_inval++;
		m = NULL;
		goto bad;
	}
#endif
	ip6 = mtod(m, struct ip6_hdr *);	/* set it again just in case */

	/*
	 * pre-compute and cache intermediate key
	 */
	if (esp_schedule(algo, sav) != 0) {
		ipsec6stat.in_inval++;
		goto bad;
	}

	/*
	 * decrypt the packet.
	 */
	if (!algo->decrypt)
		panic("internal error: no decrypt function");
	if ((*algo->decrypt)(m, off, sav, algo, ivlen)) {
		/* m is already freed */
		m = NULL;
		ipseclog((LOG_ERR, "decrypt fail in IPv6 ESP input: %s\n",
		    ipsec_logsastr(sav)));
		ipsec6stat.in_inval++;
		goto bad;
	}
	ipsec6stat.in_esphist[sav->alg_enc]++;

	m->m_flags |= M_DECRYPTED;

	/*
	 * find the trailer of the ESP.
	 */
	m_copydata(m, m->m_pkthdr.len - sizeof(esptail), sizeof(esptail),
	     (caddr_t)&esptail);
	nxt = esptail.esp_nxt;
	taillen = esptail.esp_padlen + sizeof(esptail);

	if (m->m_pkthdr.len < taillen ||
	    m->m_pkthdr.len - taillen < sizeof(struct ip6_hdr)) {	/* ? */
		ipseclog((LOG_WARNING,
		    "bad pad length in IPv6 ESP input: %s %s\n",
		    ipsec6_logpacketstr(ip6, spi), ipsec_logsastr(sav)));
		ipsec6stat.in_inval++;
		goto bad;
	}

	/* strip off the trailing pad area. */
	m_adj(m, -taillen);

	ip6->ip6_plen = htons(ntohs(ip6->ip6_plen) - taillen);

	/* was it transmitted over the IPsec tunnel SA? */
	if (ipsec6_tunnel_validate(m, off + esplen + ivlen, nxt, sav)) {
		/*
		 * strip off all the headers that precedes ESP header.
		 *	IP6 xx ESP IP6' payload -> IP6' payload
		 *
		 * XXX more sanity checks
		 * XXX relationship with gif?
		 */
		u_int32_t flowinfo;	/* net endian */
		flowinfo = ip6->ip6_flow;
		m_adj(m, off + esplen + ivlen);
		if (m->m_len < sizeof(*ip6)) {
			m = m_pullup(m, sizeof(*ip6));
			if (!m) {
				ipsec6stat.in_inval++;
				goto bad;
			}
		}
		ip6 = mtod(m, struct ip6_hdr *);
		/* ECN consideration. */
		if (!ip6_ecn_egress(ip6_ipsec_ecn, &flowinfo, &ip6->ip6_flow)) {
			ipsec6stat.in_inval++;
			goto bad;
		}
		if (!key_checktunnelsanity(sav, AF_INET6, (caddr_t)&src_sa,
					   (caddr_t)&dst_sa)) {
			ipseclog((LOG_ERR, "ipsec tunnel address mismatch "
			    "in IPv6 ESP input: %s %s\n",
			    ipsec6_logpacketstr(ip6, spi),
			    ipsec_logsastr(sav)));
			ipsec6stat.in_inval++;
			goto bad;
		}

		key_sa_recordxfer(sav, m);
		if (ipsec_addhist(m, IPPROTO_ESP, spi) != 0 ||
		    ipsec_addhist(m, IPPROTO_IPV6, 0) != 0) {
			ipsec6stat.in_nomem++;
			goto bad;
		}

#ifdef __NetBSD__
		s = splnet();
#elif !(defined(__FreeBSD__) && __FreeBSD_version >= 500000)
		s = splimp();
#endif

#if defined(__FreeBSD__) && __FreeBSD_version >= 500000
		if (netisr_queue(NETISR_IPV6, m)) {	/* (0) on success. */
			ipsec6stat.in_inval++;
			m = NULL;
			goto bad;
		}
#else
		if (IF_QFULL(&ip6intrq)) {
			ipsec6stat.in_inval++;
			splx(s);
			goto bad;
		}
		IF_ENQUEUE(&ip6intrq, m);
		schednetisr(NETISR_IPV6); /* can be skipped but to make sure */
		splx(s);
#endif
		m = NULL;
		nxt = IPPROTO_DONE;
	} else {
		/*
		 * strip off ESP header and IV.
		 * even in m_pulldown case, we need to strip off ESP so that
		 * we can always compute checksum for AH correctly.
		 */
		size_t stripsiz;
		u_int8_t *prvnxtp;

		/*
		 * Set the next header field of the previous header correctly.
		 */
		prvnxtp = ip6_get_prevhdr(m, off); /* XXX */
		*prvnxtp = nxt;

		stripsiz = esplen + ivlen;

		ip6 = mtod(m, struct ip6_hdr *);
		if (m->m_len >= stripsiz + off) {
			ovbcopy((caddr_t)ip6, ((caddr_t)ip6) + stripsiz, off);
			m->m_data += stripsiz;
			m->m_len -= stripsiz;
			m->m_pkthdr.len -= stripsiz;
		} else {
			/*
			 * this comes with no copy if the boundary is on
			 * cluster
			 */
			struct mbuf *n;

			n = m_split(m, off, M_DONTWAIT);
			if (n == NULL) {
				/* m is retained by m_split */
				goto bad;
			}
			m_adj(n, stripsiz);
			/* m_cat does not update m_pkthdr.len */
			m->m_pkthdr.len += n->m_pkthdr.len;
			m_cat(m, n);
		}

#ifndef PULLDOWN_TEST
		/*
		 * KAME requires that the packet to be contiguous on the
		 * mbuf.  We need to make that sure.
		 * this kind of code should be avoided.
		 * XXX other conditions to avoid running this part?
		 */
		if (m->m_len != m->m_pkthdr.len) {
			struct mbuf *n = NULL;
			int maxlen;

			MGETHDR(n, M_DONTWAIT, MT_HEADER);
			maxlen = MHLEN;
			if (n)
#ifdef __FreeBSD__
				M_MOVE_PKTHDR(n, m);
#else
				M_COPY_PKTHDR(n, m);
#endif
			if (n && m->m_pkthdr.len > maxlen) {
				MCLGET(n, M_DONTWAIT);
				maxlen = MCLBYTES;
				if ((n->m_flags & M_EXT) == 0) {
					m_free(n);
					n = NULL;
				}
			}
			if (!n) {
				printf("esp6_input: mbuf allocation failed\n");
				goto bad;
			}

			if (m->m_pkthdr.len <= maxlen) {
				m_copydata(m, 0, m->m_pkthdr.len, mtod(n, caddr_t));
				n->m_len = m->m_pkthdr.len;
				n->m_pkthdr.len = m->m_pkthdr.len;
				n->m_next = NULL;
				m_freem(m);
			} else {
				m_copydata(m, 0, maxlen, mtod(n, caddr_t));
				n->m_len = maxlen;
				n->m_pkthdr.len = m->m_pkthdr.len;
				n->m_next = m;
				m_adj(m, maxlen);
			}
			m = n;
		}
#endif

		ip6 = mtod(m, struct ip6_hdr *);
		ip6->ip6_plen = htons(ntohs(ip6->ip6_plen) - stripsiz);

		key_sa_recordxfer(sav, m);
		if (ipsec_addhist(m, IPPROTO_ESP, spi) != 0) {
			ipsec6stat.in_nomem++;
			goto bad;
		}
	}

	*offp = off;
	*mp = m;

	if (sav) {
		KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
			printf("DP esp6_input call free SA:%p\n", sav));
		key_freesav(sav);
	}
	ipsec6stat.in_success++;
	return nxt;

bad:
	if (sav) {
		KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
			printf("DP esp6_input call free SA:%p\n", sav));
		key_freesav(sav);
	}
	if (m)
		m_freem(m);
	return IPPROTO_DONE;
}

void
esp6_ctlinput(cmd, sa, d)
	int cmd;
	struct sockaddr *sa;
	void *d;
{
	const struct newesp *espp;
	struct newesp esp;
	struct ip6ctlparam *ip6cp = NULL, ip6cp1;
	struct secasvar *sav;
	struct ip6_hdr *ip6;
	struct mbuf *m;
	int off;
	struct in6_addr ip6c_srcin, in6;

	if (sa->sa_family != AF_INET6 ||
	    sa->sa_len != sizeof(struct sockaddr_in6))
		return;
	if ((unsigned)cmd >= PRC_NCMDS)
		return;

	/* if the parameter is from icmp6, decode it. */
	if (d != NULL) {
		ip6cp = (struct ip6ctlparam *)d;
		m = ip6cp->ip6c_m;
		ip6 = ip6cp->ip6c_ip6;
		off = ip6cp->ip6c_off;
	} else {
		m = NULL;
		ip6 = NULL;
		off = 0;
	}

	if (ip6) {
		/*
		 * Notify the error to all possible sockets via pfctlinput2.
		 * Since the upper layer information (such as protocol type,
		 * source and destination ports) is embedded in the encrypted
		 * data and might have been cut, we can't directly call
		 * an upper layer ctlinput function. However, the pcbnotify
		 * function will consider source and destination addresses
		 * as well as the flow info value, and may be able to find
		 * some PCB that should be notified.
		 * Although pfctlinput2 will call esp6_ctlinput(), there is
		 * no possibility of an infinite loop of function calls,
		 * because we don't pass the inner IPv6 header.
		 */
		bzero(&ip6cp1, sizeof(ip6cp1));
		ip6cp1.ip6c_src = ip6cp->ip6c_src;
		pfctlinput2(cmd, sa, (void *)&ip6cp1);

		/*
		 * Then go to special cases that need ESP header information.
		 * XXX: We assume that when ip6 is non NULL,
		 * M and OFF are valid.
		 */

		/* check if we can safely examine src and dst ports */
		if (m->m_pkthdr.len < off + sizeof(esp))
			return;

		if (m->m_len < off + sizeof(esp)) {
			/*
			 * this should be rare case,
			 * so we compromise on this copy...
			 */
			m_copydata(m, off, sizeof(esp), (caddr_t)&esp);
			espp = &esp;
		} else
			espp = (struct newesp*)(mtod(m, caddr_t) + off);

		if (cmd == PRC_MSGSIZE) {
			int valid = 0;

			/*
			 * Check to see if we have a valid SA corresponding to
			 * the address in the ICMP message payload.
			 */
			if (in6_embedscope(&ip6c_srcin, ip6cp->ip6c_src) ||
			    in6_embedscope(&in6, (struct sockaddr_in6 *)sa))
				return;
			sav = key_allocsa(AF_INET6, (caddr_t)&ip6c_srcin,
			    (caddr_t)&in6, IPPROTO_ESP, espp->esp_spi);
			if (sav) {
				if (sav->state == SADB_SASTATE_MATURE ||
				    sav->state == SADB_SASTATE_DYING)
					valid++;
				key_freesav(sav);
			}

			/* XXX Further validation? */

			/*
			 * Depending on the value of "valid" and routing table
			 * size (mtudisc_{hi,lo}wat), we will:
			 * - recalcurate the new MTU and create the
			 *   corresponding routing entry, or
			 * - ignore the MTU change notification.
			 */
			icmp6_mtudisc_update((struct ip6ctlparam *)d,
					     (struct sockaddr_in6 *)sa, valid);
		}
	} else {
		/* we normally notify any pcb here */
	}
}
#endif /* INET6 */

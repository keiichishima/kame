/*	$KAME: halist.c,v 1.6 2001/05/16 06:41:48 jinmei Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, 1998, 1999 and 2000 WIDE Project.
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
 * Copyright (c) 1999, 2000 and 2001 Ericsson Radio Systems AB
 * All rights reserved.
 *
 * Author:  Magnus Braathen <magnus.braathen@era.ericsson.se>
 */

#include <sys/types.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <netinet/in.h>
#include <net/if.h>
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
#include <net/if_var.h>
#endif
#include <net/route.h>
#include <netdb.h>
#include <netinet6/in6_var.h>
#include <netinet6/ip6_var.h>
#include <netinet6/nd6.h>
#include <netinet6/mip6.h>
#include "mip6stat.h"

void pr_halhdr __P((void));
void pr_halentry __P((struct mip6_halst));

void
halistpr(u_long halist)
{
	u_long             hal_ptr;
	struct mip6_halst  halentry;

	if (halist == 0) {
		printf("symbol not in namelist\n");
		return;
	}

	kget(halist, hal_ptr);

	printf("HA list:\n");
	pr_halhdr();

	for (; hal_ptr; hal_ptr = (u_long)halentry.next) {
		kget(hal_ptr, halentry);
		pr_halentry(halentry);
	}
}

/*
 * Print header for the table columns.
 */
void
pr_halhdr()
{
	if (lflag)
		printf("%-*.*s %6.6s %10.10s %8.8s\n",
			WID_IP6, WID_IP6, "Home agent",
		       "Netif", "Lifetime", "Preference");
	else
		printf("%-*.*s %6.6s %8.8s\n",
			WID_IP6, WID_IP6, "Home agent",
			"Netif", "Lifetime");
}

void
pr_halentry(struct mip6_halst halentry)
{
	char          *cp = "::";
	char           ifname[IFNAMSIZ+1];
	struct ifnet   ifp;

	kget(halentry.ifp, ifp);
#if defined(__FreeBSD__) || defined(__bsdi__)
	kget(ifp.if_name, ifname);
	printf("%4s%2d ", ifname, ifp.if_unit);
#else
	kget(ifp.if_xname, ifname);
	printf("%6s ", ifname);
#endif

	cp = ip6addr_print(&halentry.ll_addr, -1, ifname);

	if (nflag)
		printf("%-*s ", WID_IP6, cp);
	else
		printf("%-*.*s ", WID_IP6, WID_IP6, cp);

	printf("%8u ", halentry.lifetime);

	if(lflag)
		printf("%10d ", halentry.pref);

	printf("\n");
}

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
 * glue for kernel code programming differences.
 */

/*
 * OS dependencies:
 *
 * - privileged process
 *	NetBSD, FreeBSD 3
 *		struct proc *p;
 *		if (p && !suser(p->p_ucred, &p->p_acflag)
 *			privileged;
 *	OpenBSD, BSDI 3, FreeBSD 2
 *		struct socket *so;
 *		if (so->so_state & SS_PRIV)
 *			privileged;
 * - foo_control
 *	NetBSD, FreeBSD 3
 *		needs to give struct proc * as argument
 *	OpenBSD, BSDI 3, FreeBSD 2
 *		do not need struct proc *
 * - bpf:
 *	OpenBSD, NetBSD, BSDI 3
 *		need caddr_t * (= if_bpf **) and struct ifnet *
 *	FreeBSD 2, FreeBSD 3
 *		need only struct ifnet * as argument
 * - struct ifnet
 *			use queue.h?	member names	if name
 *			---		---		---
 *	FreeBSD 2	no		?		if_name+unit
 *	FreeBSD 3	yes		strange		if_name+unit
 *	OpenBSD		yes		standard	if_xname
 *	NetBSD		yes		standard	if_xname
 *	BSDI 3		no		---		if_name+unit
 * - usrreq
 *	NetBSD, OpenBSD, BSDI 3, FreeBSD 2
 *		single function with PRU_xx, arguments are mbuf 
 *	FreeBSD 3
 *		separates functions, non-mbuf arguments
 * - {set,get}sockopt
 *	NetBSD, OpenBSD, BSDI 3, FreeBSD 2
 *		manipulation based on mbuf
 *	FreeBSD 3
 *		non-mbuf manipulation using sooptcopy{in,out}()
 * - timeout() and untimeout()
 *	NetBSD, OpenBSD, BSDI 3, FreeBSD 2
 *		timeout() is a void function
 *	FreeBSD 3
 *		timeout() is non-void, must keep returned value for untimeuot()
 * - sysctl
 *	NetBSD, OpenBSD
 *		foo_sysctl()
 *	BSDI 3
 *		foo_sysctl() but with different style
 *	FreeBSD 2, FreeBSD 3
 *		linker hack
 *
 * - ovbcopy()
 *	in NetBSD 1.4 or later, ovbcopy() is not supplied in the kernel.
 *	bcopy() is safe against overwrites.
 */

#ifndef __NET_NET_OSDEP_H_DEFINED_
#define __NET_NET_OSDEP_H_DEFINED_
#ifdef _KERNEL

#if defined(__NetBSD__) || defined(__OpenBSD__)
#define if_name(ifp)	((ifp)->if_xname)
#else
struct ifnet;
extern char *if_name __P((struct ifnet *));
#endif

#ifdef __FreeBSD__
#define HAVE_OLD_BPF
#endif

#if defined(__FreeBSD__) && __FreeBSD__ >= 3
#define ifa_list	ifa_link
#define if_addrlist	if_addrhead
#define if_list		if_link
#endif

#if defined(__NetBSD__) && __NetBSD_Version__ >= 104000000
#define ovbcopy		bcopy
#endif

#endif /*_KERNEL*/
#endif /*__NET_NET_OSDEP_H_DEFINED_ */

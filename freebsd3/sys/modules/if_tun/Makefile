# $FreeBSD: src/sys/modules/if_tun/Makefile,v 1.8.2.1 1999/08/29 16:27:18 peter Exp $

.PATH:  ${.CURDIR}/../../net
KMOD=   if_tun
SRCS=   if_tun.c bpfilter.h opt_devfs.h opt_inet.h opt_inet6.h tun.h vnode_if.h
NOMAN=
CLEANFILES+=	vnode_if.h vnode_if.c

NBPFILTER?=	0
NTUN?=		2

CFLAGS+= ${PROTOS} -D_KERNEL
CLEANFILES+=	bpfilter.h opt_devfs.h opt_inet.h opt_inet6.h tun.h

bpfilter.h:
	echo "#define NBPFILTER ${NBPFILTER}" > bpfilter.h

opt_devfs.h:
	touch opt_devfs.h

opt_inet.h:
	echo "#define INET 1" > opt_inet.h

opt_inet6.h:
	echo "#define INET6 1" > opt_inet6.h

tun.h:
	echo "#define NTUN ${NTUN}" > tun.h

.include <bsd.kmod.mk>

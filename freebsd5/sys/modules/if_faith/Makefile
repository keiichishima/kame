# $FreeBSD: src/sys/modules/if_faith/Makefile,v 1.2 2002/01/11 15:48:46 ru Exp $

.PATH: ${.CURDIR}/../../net

KMOD=	if_faith
SRCS=	if_faith.c opt_inet.h opt_inet6.h faith.h bpf.h

opt_inet.h:
	echo "#define INET 1" > ${.TARGET}

opt_inet6.h:
	echo "#define INET6 1" > ${.TARGET}

faith.h:
	echo "#define NFAITH 1" > ${.TARGET}

bpf.h:
	echo "#define NBPF 1" > ${.TARGET}

.include <bsd.kmod.mk>

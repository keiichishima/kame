# $Whistle: Makefile,v 1.2 1999/01/19 19:39:21 archie Exp $
# $FreeBSD: src/sys/modules/netgraph/iface/Makefile,v 1.2.2.2 1999/11/17 07:46:40 julian Exp $

KMOD=		ng_iface
SRCS= 		ng_iface.c bpfilter.h opt_inet.h opt_atalk.h opt_ipx.h
MAN8=		ng_iface.8
KMODDEPS=	netgraph

IFACE_FILTER?=		0	# 0/1 - requires bpf configured in kernel
IFACE_INET?=		1	# 0/1 - requires INET configured in kernel
IFACE_NETATALK?=	0	# 0/1 - requires NETATALK configured in kernel
IFACE_IPX?=		0	# 0/1 - requires IPX configured in kernel

CFLAGS+=	${PROTOS}

CLEANFILES+=	bpfilter.h opt_inet.h opt_atalk.h opt_ipx.h

bpfilter.h:
	echo "#define NBPFILTER ${IFACE_FILTER}" > bpfilter.h

opt_inet.h:
	touch opt_inet.h
.if ${IFACE_INET} > 0
	echo "#define INET 1" > opt_inet.h
.endif

opt_atalk.h:
	touch opt_atalk.h
.if ${IFACE_NETATALK} > 0
	echo "#define NETATALK ${IFACE_NETATALK}" > opt_atalk.h
.endif

opt_ipx.h:
	touch opt_ipx.h
.if ${IFACE_IPX} > 0
	echo "#define IPX ${IFACE_IPX}" > opt_ipx.h
.endif

.include <bsd.kmod.mk>

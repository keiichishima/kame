# $FreeBSD: src/sys/modules/lnc/Makefile,v 1.6 2002/11/06 13:46:58 nyan Exp $

.PATH: ${.CURDIR}/../../dev/lnc

KMOD=	if_lnc
SRCS=	if_lnc.c if_lnc_pci.c
.if ${MACHINE} == "pc98"
SRCS+=	if_lnc_cbus.c
.else
SRCS+=	if_lnc_isa.c
.endif
SRCS+=	opt_inet.h device_if.h bus_if.h isa_if.h pci_if.h
#SRCS+=	miibus_if.h

CFLAGS+= -g -I${.CURDIR}/../../dev/lnc

.include <bsd.kmod.mk>

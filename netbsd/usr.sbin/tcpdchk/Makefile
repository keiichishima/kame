#	$NetBSD: Makefile,v 1.10 2000/10/11 20:23:57 is Exp $

PROG=	tcpdchk
SRCS=	tcpdchk.c fakelog.c inetcf.c scaffold.c percent_m.c
MAN=	tcpdchk.8
LDADD+=	-L${.CURDIR}/../../lib/libwrap \
	-L${.CURDIR}/../../lib/libwrap/obj \
	-L/usr/local/v6/lib
LDADD+=	-lwrap
DPADD+=	${.CURDIR}/../../lib/libwrap/libwrap.a \
	${.CURDIR}/../../lib/libwrap/obj/libwrap.a \
	/usr/local/v6/lib/libwrap.a
#DPADD=	${LIBWRAP}

CPPFLAGS+= -I${.CURDIR}/../../lib/libwrap

.include "${.CURDIR}/../../lib/libwrap/Makefile.cflags"

.include <bsd.prog.mk>

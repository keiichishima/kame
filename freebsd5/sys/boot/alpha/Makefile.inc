# $FreeBSD: src/sys/boot/alpha/Makefile.inc,v 1.8 2000/10/27 09:36:34 dfr Exp $
# Options used when building app-specific libalpha components
PRIMARY_LOAD_ADDRESS=	0x20000000	# "Region 1 start"
SECONDARY_LOAD_ADDRESS=	0x2000c000	# "Region 1 start" + 48k
HEAP_LIMIT=		0x20040000	# "Region 1 start" + 256k
DPADD+=			${DESTDIR}/${LIBDIR}/libstand.a
LIBSTANDDIR=		${.CURDIR}/../../../../lib/libstand
LIBSTAND=		-lstand
LIBALPHA=		${.OBJDIR}/../libalpha/libalpha.a

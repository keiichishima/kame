#	$NetBSD: Makefile,v 1.2 2002/12/23 17:52:04 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/cd9660 ${.CURDIR}/../..

KMOD=	cd9660

SRCS=	lkminit_vfs.c
SRCS+=	cd9660_bmap.c cd9660_lookup.c cd9660_rrip.c cd9660_vfsops.c
SRCS+=	cd9660_node.c cd9660_vnops.c cd9660_util.c

.include <bsd.kmod.mk>

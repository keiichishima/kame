#	$NetBSD: Makefile,v 1.4 2001/12/12 11:49:05 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/compat/freebsd

CPPFLAGS+=	-nostdinc -I$S -D_KERNEL -DEXEC_ELF32

KMOD=	exec_freebsd_elf

SRCS=	lkminit_exec.c
SRCS+=	freebsd_exec_elf32.c

# compat_util.c symbol redifinitions
CPPFLAGS+=	-Demul_find=freebsd_emul_find				\
		-Dstackgap_init=freebsd_stackgap_init			\
		-Demul_flags_translate=freebsd_emul_flags_translate	\
		-Dstackgap_alloc=freebsd_stackgap_alloc		 	\
		-Dcompat_offseterr=freebsd_compat_offseterr
.PATH: $S/compat/common
SRCS+=	compat_util.c

.include <bsd.kmod.mk>

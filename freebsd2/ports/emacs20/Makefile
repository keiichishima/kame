# New ports collection makefile for:	GNU emacs
# Version required:	20.6
# Date created:		29 October 1994
# Whom:			jkh
#
# $Id: Makefile,v 1.3 2000/03/30 17:05:36 sumikawa Exp $
#

DISTNAME=	emacs-20.6
CATEGORIES=	editors
MASTER_SITES=	${MASTER_SITE_GNU}
MASTER_SITE_SUBDIR=	emacs

MAINTAINER=	sumikawa@kame.net

Y2K=            http://www.gnu.org/software/year2000.html

WRKSRC=		${WRKDIR}/emacs-20.6
USE_AUTOCONF=	yes
USE_GMAKE=	yes
CONFIGURE_TARGET=	${MACHINE_ARCH}--freebsd
.if !defined(NO_X11)
CONFIGURE_ARGS=	--with-x-toolkit --with-pop
USE_XLIB=	yes
.else
CONFIGURE_ARGS=	--with-x=no --with-pop
.endif

MAKE_ENV=	INSTALL_SCRIPT="${INSTALL_SCRIPT}"
MAN1=		emacs.1 etags.1 ctags.1
PLIST_SUB=	EMACS_VER=20.5 EMACS_ARCH=${CONFIGURE_TARGET}

.if defined(USE_XPG4)
.if (${USE_XPG4} == "YES" || ${USE_XPG4} == "yes")
CONFIGURE_ARGS+=	--with-xpg4
.endif
.endif

pre-fetch:
.if !defined(USE_XPG4)
	@${ECHO_MSG} "NOTE: You can have international locale library support "
	@${ECHO_MSG} "by adding \"USE_XPG4=YES\" to argument of make."
.endif

pre-build:
	${RM} -rf ${WRKSRC}/info/*

.include <bsd.port.mk>

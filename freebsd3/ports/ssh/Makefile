# New ports collection makefile for:	ssh
# Date created:		30 Jul 1995
# Whom:			torstenb@FreeBSD.org
#
# $FreeBSD: ports/security/ssh/Makefile,v 1.90 2000/02/20 17:30:53 sumikawa Exp $
# $KAME: Makefile,v 1.6 2001/02/09 06:00:59 itojun Exp $
#
# Maximal ssh package requires YES values for
# USE_PERL, USE_TCPWRAP
#

PORTNAME=	ssh
PORTVERSION=	1.2.27
CATEGORIES=	security net #ipv6
MASTER_SITES=	ftp://ftp.cs.hut.fi/pub/ssh/

MAINTAINER=	sumikawa@kame.net

BROKEN=		"obsolete, security issues"

USE_AUTOCONF=	YES	# unfortunately... see comments in patch-xa for details

# You can set USA_RESIDENT appropriately in /etc/make.conf if this bugs you..

.if defined(USA_RESIDENT) && ${USA_RESIDENT} == YES
MASTER_SITES+=	\
	ftp://ftp.replay.com/pub/replay/crypto/SSH/ \
	ftp://nic.funet.fi/pub/crypt/mirrors/ftp.dsi.unimi.it/applied-crypto/ \
	ftp://rzsun2.informatik.uni-hamburg.de/pub/virus/crypt/ripem/ \
	ftp://idea.sec.dsi.unimi.it/pub/security/crypt/math/ \
	ftp://ftp.univie.ac.at/security/crypt/cryptography/asymmetric/rsa/ \
	ftp://isdec.vc.cvut.cz/pub/security/unimi/crypt/applied-crypto/

CONFIGURE_ARGS+= --with-rsaref
LIB_DEPENDS+=	rsaref.2:${PORTSDIR}/security/rsaref
BUILD_DEPENDS+= /nonexistent:${PORTSDIR}/security/rsaref:extract
.endif

RESTRICTED=	"Crypto; export-controlled"
IS_INTERACTIVE=	YES

GNU_CONFIGURE=	YES

CONFIGURE_ARGS+= --with-etcdir=${PREFIX}/etc

#Uncomment if all your users are in their own group and their homedir
#is writeable by that group.  Beware the security implications!
#CONFIGURE_ARGS+= --enable-group-writeability

#Uncomment if you want to allow ssh to emulate an unencrypted rsh connection
#over a secure medium.  This is normally dangerous since it can lead to the
#disclosure keys and passwords.
#CONFIGURE_ARGS+= --with-none

.if defined(KRB5_HOME) && exists(${KRB5_HOME})
CONFIGURE_ARGS+=--with-kerberos5=${KRB5_HOME} --enable-kerberos-tgt-passing \
	--disable-suid-ssh
.endif

# Include support for the SecureID card
# Warning: untested !
.if defined(USE_SECUREID) && ${USE_SECUREID} == YES
CONFIGURE_ARGS+= --with-secureid
.endif

# Don't use IDEA. IDEA can be freely used for non-commercial use. However,
# commercial use may require a licence in a number of countries
# Warning: untested !
.if defined(DONT_USE_IDEA) && ${DONT_USE_IDEA} == YES
CONFIGURE_ARGS+= --without-idea
.endif

.if !exists(${X11BASE}/bin/xauth)
CONFIGURE_ARGS+= --without-x
.endif

MAN1=		scp1.1 ssh-add1.1 ssh-agent1.1 ssh-keygen1.1 ssh1.1 \
		make-ssh-known-hosts1.1
MAN8=		sshd1.8
MLINKS=		make-ssh-known-hosts1.1 make-ssh-known-hosts.1 \
		scp1.1 scp.1 \
		ssh-add1.1 ssh-add.1 \
		ssh-agent1.1 ssh-agent.1 \
		ssh-keygen1.1 ssh-keygen.1 \
		ssh1.1 ssh.1 \
		ssh.1 slogin.1 \
		ssh1.1 slogin1.1 \
		sshd1.8 sshd.8

pre-patch:
	@${MV} -f ${WRKSRC}/make-ssh-known-hosts.pl \
	    ${WRKSRC}/make-ssh-known-hosts.pl.in

fetch-depends:
.if !defined(USA_RESIDENT) || ${USA_RESIDENT} != YES && ${USA_RESIDENT} != NO
	@ ${ECHO}
	@ ${ECHO} You must set the variable USA_RESIDENT to YES if you are a
	@ ${ECHO} United States resident, otherwise NO.
	@ ${ECHO} If you are a US resident then this port must also fetch
	@ ${ECHO} the RSAREF2 library from sources abroad \(RSA Inc. holds a
	@ ${ECHO} patent on RSA and public key crypto in general in the United
	@ ${ECHO} States so using RSA implementations other than RSAREF there
	@ ${ECHO} may violate US patent law\).
	@ ${FALSE}
.endif

post-install:
	@if [ ! -f ${PREFIX}/etc/ssh_host_key ]; then \
		${ECHO} "Generating a secret host key..."; \
		${PREFIX}/bin/ssh-keygen -f ${PREFIX}/etc/ssh_host_key -N ""; \
	fi
	@if [ ! -f ${PREFIX}/etc/rc.d/sshd.sh ]; then \
		${ECHO} "Installing ${PREFIX}/etc/rc.d/sshd.sh startup file."; \
		${ECHO} "#!/bin/sh" > ${PREFIX}/etc/rc.d/sshd.sh; \
		${ECHO} "[ -x ${PREFIX}/sbin/sshd ] && ${PREFIX}/sbin/sshd && ${ECHO} -n ' sshd'" >> ${PREFIX}/etc/rc.d/sshd.sh; \
		${CHMOD} 751 ${PREFIX}/etc/rc.d/sshd.sh; \
	fi

.include <bsd.port.pre.mk>

.if defined(USE_PERL) && ${USE_PERL} == YES || \
    exists(${PERL5}) && (!defined(USE_PERL) || ${USE_PERL} != NO)
USE_PERL5=	yes
CONFIGURE_ENV+= PERL=${PERL5}
.else
CONFIGURE_ENV+= PERL=/replace_it_with_PERL_path
.endif

# Include tcp-wrapper support (call remote identd)
.if exists(/usr/include/tcpd.h)
CONFIGURE_ARGS+= --with-libwrap
.else
.if defined(USE_TCPWRAP) && ${USE_TCPWRAP} == YES || \
    exists(${PREFIX}/lib/libwrap.a) && \
    (!defined(USE_TCPWRAP) || ${USE_TCPWRAP} != NO)
CONFIGURE_ENV+= LDFLAGS=-L${PREFIX}/lib CFLAGS="${CFLAGS} -I${PREFIX}/include"
CONFIGURE_ARGS+= --with-libwrap
LIB_DEPENDS+=   wrap.7:${PORTSDIR}/security/tcp_wrapper
.endif
.endif

# Original IPv6 patches were obtained from ftp://ftp.kyoto.wide.ad.jp/IPv6/ssh/
# ssh-1.2.27-IPv6-1.5-patch.gz
# We still use USE_INET6 here and try to support pre 4.0 machines with kame
# IPv6 stack
#.if ${OSVERSION} >= 400014 || ( ${OSVERSION} < 400014 && defined(USE_INET6) )
CONFIGURE_ARGS+=	--enable-ipv6
#.else
#CONFIGURE_ARGS+=	--disable-ipv6
#.endif

# Include SOCKS firewall support
.if defined(USE_SOCKS) && ${USE_SOCKS} == YES
CONFIGURE_ARGS+= --with-socks="-L${PREFIX}/lib -lsocks5" --with-socks5
.endif

# Include extra files if X11 is installed
.if exists(${X11BASE}/lib/libX11.a)
PLIST:=         ${WRKDIR}/PLIST
pre-install:
	@${CAT} ${PKGDIR}/PLIST.X11 > ${PLIST}
	@${CAT} ${PKGDIR}/PLIST >> ${PLIST}
.endif

.include <bsd.port.post.mk>

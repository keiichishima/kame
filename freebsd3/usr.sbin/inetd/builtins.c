/*-
 * Copyright (c) 1983, 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/usr.sbin/inetd/builtins.c,v 1.14.2.1 1999/11/18 09:33:47 des Exp $
 *
 */

#include <sys/filio.h>
#include <sys/ioccom.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/ucred.h>
#include <sys/uio.h>
#include <sys/utsname.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <syslog.h>
#include <unistd.h>

#include "inetd.h"

extern int	 debug;
extern struct servtab *servtab;

char ring[128];
char *endring;

int check_loop __P((struct sockaddr_in6 *, struct servtab *sep));
void inetd_setproctitle __P((char *, int));

struct biltin biltins[] = {
	/* Echo received data */
	{ "echo",	SOCK_STREAM,	1, -1,	echo_stream },
	{ "echo",	SOCK_DGRAM,	0, 1,	echo_dg },

	/* Internet /dev/null */
	{ "discard",	SOCK_STREAM,	1, -1,	discard_stream },
	{ "discard",	SOCK_DGRAM,	0, 1,	discard_dg },

	/* Return 32 bit time since 1970 */
	{ "time",	SOCK_STREAM,	0, -1,	machtime_stream },
	{ "time",	SOCK_DGRAM,	0, 1,	machtime_dg },

	/* Return human-readable time */
	{ "daytime",	SOCK_STREAM,	0, -1,	daytime_stream },
	{ "daytime",	SOCK_DGRAM,	0, 1,	daytime_dg },

	/* Familiar character generator */
	{ "chargen",	SOCK_STREAM,	1, -1,	chargen_stream },
	{ "chargen",	SOCK_DGRAM,	0, 1,	chargen_dg },

	{ "tcpmux",	SOCK_STREAM,	1, -1,	(void (*)())tcpmux },

	{ "auth",	SOCK_STREAM,	1, -1,	ident_stream },

	{ NULL }
};

/*
 * RFC864 Character Generator Protocol. Generates character data without
 * any regard for input.
 */

void
initring()
{
	int i;

	endring = ring;

	for (i = 0; i <= 128; ++i)
		if (isprint(i))
			*endring++ = i;
}

/* ARGSUSED */
void
chargen_dg(s, sep)		/* Character generator */
	int s;
	struct servtab *sep;
{
	struct sockaddr_in6 sin;
	static char *rs;
	int len, size;
	char text[LINESIZ+2];

	if (endring == 0) {
		initring();
		rs = ring;
	}

	size = sizeof(sin);
	if (recvfrom(s, text, sizeof(text), 0, SA(&sin), &size) < 0)
		return;

	if (check_loop(&sin, sep))
		return;

	if ((len = endring - rs) >= LINESIZ)
		memmove(text, rs, LINESIZ);
	else {
		memmove(text, rs, len);
		memmove(text + len, ring, LINESIZ - len);
	}
	if (++rs == endring)
		rs = ring;
	text[LINESIZ] = '\r';
	text[LINESIZ + 1] = '\n';
	(void) sendto(s, text, sizeof(text), 0, SA(&sin), sizeof(sin));
}

/* ARGSUSED */
void
chargen_stream(s, sep)		/* Character generator */
	int s;
	struct servtab *sep;
{
	int len;
	char *rs, text[LINESIZ+2];

	inetd_setproctitle(sep->se_service, s);

	if (!endring) {
		initring();
		rs = ring;
	}

	text[LINESIZ] = '\r';
	text[LINESIZ + 1] = '\n';
	for (rs = ring;;) {
		if ((len = endring - rs) >= LINESIZ)
			memmove(text, rs, LINESIZ);
		else {
			memmove(text, rs, len);
			memmove(text + len, ring, LINESIZ - len);
		}
		if (++rs == endring)
			rs = ring;
		if (write(s, text, sizeof(text)) != sizeof(text))
			break;
	}
	exit(0);
}

/*
 * RFC867 Daytime Protocol. Sends the current date and time as an ascii
 * character string without any regard for input.
 */

/* ARGSUSED */
void
daytime_dg(s, sep)		/* Return human-readable time of day */
	int s;
	struct servtab *sep;
{
	char buffer[256];
	time_t clock;
	struct sockaddr_in6 sin;
	int size;

	clock = time((time_t *) 0);

	size = sizeof(sin);
	if (recvfrom(s, buffer, sizeof(buffer), 0, SA(&sin), &size) < 0)
		return;

	if (check_loop(&sin, sep))
		return;

	(void) sprintf(buffer, "%.24s\r\n", ctime(&clock));
	(void) sendto(s, buffer, strlen(buffer), 0, SA(&sin), sizeof(sin));
}

/* ARGSUSED */
void
daytime_stream(s, sep)		/* Return human-readable time of day */
	int s;
	struct servtab *sep;
{
	char buffer[256];
	time_t clock;

	clock = time((time_t *) 0);

	(void) sprintf(buffer, "%.24s\r\n", ctime(&clock));
	(void) write(s, buffer, strlen(buffer));
}

/*
 * RFC863 Discard Protocol. Any data received is thrown away and no response
 * is sent.
 */

/* ARGSUSED */
void
discard_dg(s, sep)		/* Discard service -- ignore data */
	int s;
	struct servtab *sep;
{
	char buffer[BUFSIZE];

	(void) read(s, buffer, sizeof(buffer));
}

/* ARGSUSED */
void
discard_stream(s, sep)		/* Discard service -- ignore data */
	int s;
	struct servtab *sep;
{
	int ret;
	char buffer[BUFSIZE];

	inetd_setproctitle(sep->se_service, s);
	while (1) {
		while ((ret = read(s, buffer, sizeof(buffer))) > 0)
			;
		if (ret == 0 || errno != EINTR)
			break;
	}
	exit(0);
}

/*
 * RFC862 Echo Protocol. Any data received is sent back to the sender as
 * received.
 */

/* ARGSUSED */
void
echo_dg(s, sep)			/* Echo service -- echo data back */
	int s;
	struct servtab *sep;
{
	char buffer[BUFSIZE];
	int i, size;
	struct sockaddr_in6 sin;

	size = sizeof(sin);
	if ((i = recvfrom(s, buffer, sizeof(buffer), 0, SA(&sin), &size)) < 0)
		return;

	if (check_loop(&sin, sep))
		return;

	(void) sendto(s, buffer, i, 0, SA(&sin), size);
}

/* ARGSUSED */
void
echo_stream(s, sep)		/* Echo service -- echo data back */
	int s;
	struct servtab *sep;
{
	char buffer[BUFSIZE];
	int i;

	inetd_setproctitle(sep->se_service, s);
	while ((i = read(s, buffer, sizeof(buffer))) > 0 &&
	    write(s, buffer, i) > 0)
		;
	exit(0);
}

/*
 * RFC1413 Identification Protocol. Given a TCP port number pair, return a
 * character string which identifies the owner of that connection on the
 * server's system. Extended to allow for ~/.fakeid support and ~/.noident
 * support.
 */

/* ARGSUSED */
void
iderror(lport, fport, s, er)	/* Generic ident_stream error-sending func */
	int lport, fport, s, er;
{
	char *p;

	asprintf(&p, "%d , %d : ERROR : %s\r\n", lport, fport, 
	    er == -1 ? "HIDDEN-USER" : er ? strerror(er) : "UNKNOWN-ERROR");
	if (p == NULL) {
		syslog(LOG_ERR, "asprintf: %m");
		exit(EX_OSERR);
	}
	write(s, p, strlen(p));
	free(p);

	exit(0);
}

/* ARGSUSED */
void
ident_stream(s, sep)		/* Ident service (AKA "auth") */
	int s;
	struct servtab *sep;
{
	struct utsname un;
	struct stat sb;
	struct sockaddr_in6 sin[2];
	struct sockaddr_in sin4[2];
	struct ucred uc;
	struct timeval tv = {
		10,
		0
	};
	struct passwd *pw;
	fd_set fdset;
	char buf[BUFSIZE], *cp = NULL, *p, **av, *osname = NULL, *ctlname;
	int len, c, fflag = 0, nflag = 0, rflag = 0, argc = 0;
	u_short lport, fport;
	struct sockaddr *sa;
	int salen;

	inetd_setproctitle(sep->se_service, s);
	/*
	 * Reset getopt() since we are a fork() but not an exec() from
	 * a parent which used getopt() already.
	 */
	optind = 1;
	optreset = 1;
	/*
	 * Take the internal argument vector and count it out to make an
	 * argument count for getopt. This can be used for any internal
	 * service to read arguments and use getopt() easily.
	 */
	for (av = sep->se_argv; *av; av++)
		argc++;
	if (argc) {
		int sec, usec;

		while ((c = getopt(argc, sep->se_argv, "fno:rt:")) != -1)
			switch (c) {
			case 'f':
				fflag = 1;
				break;
			case 'n':
				nflag = 1;
				break;
			case 'o':
				osname = optarg;
				break;
			case 'r':
				rflag = 1;
				break;
			case 't':
				switch (sscanf(optarg, "%d.%d", &sec, &usec)) {
				case 2:
					tv.tv_usec = usec;
				case 1:
					tv.tv_sec = sec;
					break;
				default:
					if (debug)
						warnx("bad -t argument");
					break;
				}
				break;
			default:
				break;
			}
	}
	if (osname == NULL) {
		if (uname(&un) == -1)
			iderror(0, 0, s, errno);
		osname = un.sysname;
	}
	len = sizeof(sin[0]);
	if (getsockname(s, (struct sockaddr *)&sin[0], &len) == -1)
		iderror(0, 0, s, errno);
	len = sizeof(sin[1]);
	if (getpeername(s, (struct sockaddr *)&sin[1], &len) == -1)
		iderror(0, 0, s, errno);
	/*
	 * We're going to prepare for and execute reception of a
	 * packet of data from the user. The data is in the format
	 * "local_port , foreign_port\r\n" (with local being the
	 * server's port and foreign being the client's.)
	 */
	FD_ZERO(&fdset);
	FD_SET(s, &fdset);
	if (select(s + 1, &fdset, NULL, NULL, &tv) == -1)
		iderror(0, 0, s, errno);
	if (ioctl(s, FIONREAD, &len) == -1)
		iderror(0, 0, s, errno);
	if (len >= sizeof(buf))
		len = sizeof(buf) - 1;
	len = read(s, buf, len);
	if (len == -1)
		iderror(0, 0, s, errno);
	buf[len] = '\0';
	if (sscanf(buf, "%hu , %hu", &lport, &fport) != 2)
		iderror(0, 0, s, 0);
	if (!rflag) /* Send HIDDEN-USER immediately if not "real" */
		iderror(lport, fport, s, -1);
	/*
	 * We take the input and construct an array of two sockaddr_ins
	 * which contain the local address information and foreign
	 * address information, respectively, used to look up the
	 * credentials for the socket (which are returned by the
	 * sysctl "net.inet.tcp.getcred" when we call it.) The
	 * arrays have been filled in above via get{peer,sock}name(),
	 * so right here we are only setting the ports.
	 */
	if (sin[0].sin6_family == AF_INET) {
		sin4[0].sin_family = AF_INET;
		sin4[0].sin_len = sizeof(struct sockaddr_in);
		memcpy(&sin4[0].sin_addr,
		       &((struct sockaddr_in *)&sin[0])->sin_addr.s_addr,
		       sizeof(struct in_addr));
		sin4[0].sin_port = htons(lport);
		sin4[1].sin_family = AF_INET;
		sin4[1].sin_len = sizeof(struct sockaddr_in);
		memcpy(&(sin4[1].sin_addr),
		       &((struct sockaddr_in *)&sin[1])->sin_addr.s_addr,
		       sizeof(struct in_addr));
		sin4[1].sin_port = htons(fport);
		sa = (struct sockaddr *)sin4;
		salen = sizeof(sin4);
		ctlname = "net.inet.tcp.getcred";
	} else if (IN6_IS_ADDR_V4MAPPED(&sin[0].sin6_addr)) {
		sin4[0].sin_family = AF_INET;
		sin4[0].sin_len = sizeof(struct sockaddr_in);
		memcpy(&sin4[0].sin_addr, &sin[0].sin6_addr.s6_addr[12],
		       sizeof(struct in_addr));
		sin4[0].sin_port = htons(lport);
		sin4[1].sin_family = AF_INET;
		sin4[1].sin_len = sizeof(struct sockaddr_in);
		memcpy(&(sin4[1].sin_addr), &sin[1].sin6_addr.s6_addr[12],
		       sizeof(struct in_addr));
		sin4[1].sin_port = htons(fport);
		sa = (struct sockaddr *)sin4;
		salen = sizeof(sin4);
		ctlname = "net.inet.tcp.getcred";
	} else {
		sin[0].sin6_port = htons(lport);
		sin[1].sin6_port = htons(fport);
		sa = (struct sockaddr *)sin;
		salen = sizeof(sin);
		ctlname = "net.inet6.tcp6.getcred";
	}
	len = sizeof(uc);
	if (sysctlbyname(ctlname, &uc, &len, sa, salen) == -1)
		iderror(lport, fport, s, errno);
	pw = getpwuid(uc.cr_uid);	/* Look up the pw to get the username */
	if (pw == NULL)
		iderror(lport, fport, s, errno);
	/*
	 * If enabled, we check for a file named ".noident" in the user's
	 * home directory. If found, we return HIDDEN-USER.
	 */
	if (nflag) {
		if (asprintf(&p, "%s/.noident", pw->pw_dir) == -1)
			iderror(lport, fport, s, errno);
		if (lstat(p, &sb) == 0) {
			free(p);
			iderror(lport, fport, s, -1);
		}
		free(p);
	}
	/*
	 * Here, if enabled, we read a user's ".fakeid" file in their
	 * home directory. It consists of a line containing the name
	 * they want.
	 */
	if (fflag) {
		FILE *fakeid = NULL;

		if (asprintf(&p, "%s/.fakeid", pw->pw_dir) == -1)
			iderror(lport, fport, s, errno);
		/*
		 * Here we set ourself to effectively be the user, so we don't
		 * open any files we have no permission to open, especially
		 * symbolic links to sensitive root-owned files or devices.
		 */
		seteuid(pw->pw_uid);
		setegid(pw->pw_gid);
		/*
		 * If we were to lstat() here, it would do no good, since it
		 * would introduce a race condition and could be defeated.
		 * Therefore, we open the file we have permissions to open
		 * and if it's not a regular file, we close it and end up
		 * returning the user's real username.
		 */
		fakeid = fopen(p, "r");
		free(p);
		if (fakeid != NULL &&
		    fstat(fileno(fakeid), &sb) != -1 && S_ISREG(sb.st_mode)) {
			buf[sizeof(buf) - 1] = '\0';
			if (fgets(buf, sizeof(buf), fakeid) == NULL) {
				cp = pw->pw_name;
				fclose(fakeid);
				goto printit;
			}
			fclose(fakeid);
			/*
			 * Usually, the file will have the desired identity
			 * in the form "identity\n", so we use strtok() to
			 * end the string (which fgets() doesn't do.)
			 */
			strtok(buf, "\r\n");
			/* User names of >16 characters are invalid */
			if (strlen(buf) > 16)
				buf[16] = '\0';
			cp = buf;
			/* Allow for beginning white space... */
			while (isspace(*cp))
				cp++;
			/* ...and ending white space. */
			strtok(cp, " \t");
			/*
			 * If the name is a zero-length string or matches
			 * the name of another user, it's invalid, so
			 * we will return their real identity instead.
			 */
			
			if (!*cp || getpwnam(cp))
				cp = getpwuid(uc.cr_uid)->pw_name;
		} else
			cp = pw->pw_name;
	} else
		cp = pw->pw_name;
printit:
	/* Finally, we make and send the reply. */
	if (asprintf(&p, "%d , %d : USERID : %s : %s\r\n", lport, fport, osname,
	    cp) == -1) {
		syslog(LOG_ERR, "asprintf: %m");
		exit(EX_OSERR);
	}
	write(s, p, strlen(p));
	free(p);
	
	exit(0);
}

/*
 * RFC738 Time Server.
 * Return a machine readable date and time, in the form of the
 * number of seconds since midnight, Jan 1, 1900.  Since gettimeofday
 * returns the number of seconds since midnight, Jan 1, 1970,
 * we must add 2208988800 seconds to this figure to make up for
 * some seventy years Bell Labs was asleep.
 */

long
machtime()
{
	struct timeval tv;

	if (gettimeofday(&tv, (struct timezone *)NULL) < 0) {
		if (debug)
			warnx("unable to get time of day");
		return (0L);
	}
#define	OFFSET ((u_long)25567 * 24*60*60)
	return (htonl((long)(tv.tv_sec + OFFSET)));
#undef OFFSET
}

/* ARGSUSED */
void
machtime_dg(s, sep)
	int s;
	struct servtab *sep;
{
	unsigned long result;
	struct sockaddr_in6 sin;
	int size;

	size = sizeof(sin);
	if (recvfrom(s, (char *)&result, sizeof(result), 0,
		     SA(&sin), &size) < 0)
		return;

	if (check_loop(&sin, sep))
		return;

	result = machtime();
	(void) sendto(s, (char *) &result, sizeof(result), 0,
		      (struct sockaddr *)&sin, sizeof(sin));
}

/* ARGSUSED */
void
machtime_stream(s, sep)
	int s;
	struct servtab *sep;
{
	long result;

	result = machtime();
	(void) write(s, (char *) &result, sizeof(result));
}

/*
 * RFC1078 TCP Port Service Multiplexer (TCPMUX). Service connections to
 * services based on the service name sent.
 *
 *  Based on TCPMUX.C by Mark K. Lottor November 1988
 *  sri-nic::ps:<mkl>tcpmux.c
 */

#define MAX_SERV_LEN	(256+2)		/* 2 bytes for \r\n */
#define strwrite(fd, buf)	(void) write(fd, buf, sizeof(buf)-1)

static int		/* # of characters upto \r,\n or \0 */
getline(fd, buf, len)
	int fd;
	char *buf;
	int len;
{
	int count = 0, n;
	struct sigaction sa;

	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = SIG_DFL;
	sigaction(SIGALRM, &sa, (struct sigaction *)0);
	do {
		alarm(10);
		n = read(fd, buf, len-count);
		alarm(0);
		if (n == 0)
			return (count);
		if (n < 0)
			return (-1);
		while (--n >= 0) {
			if (*buf == '\r' || *buf == '\n' || *buf == '\0')
				return (count);
			count++;
			buf++;
		}
	} while (count < len);
	return (count);
}

struct servtab *
tcpmux(s)
	int s;
{
	struct servtab *sep;
	char service[MAX_SERV_LEN+1];
	int len;

	/* Get requested service name */
	if ((len = getline(s, service, MAX_SERV_LEN)) < 0) {
		strwrite(s, "-Error reading service name\r\n");
		return (NULL);
	}
	service[len] = '\0';

	if (debug)
		warnx("tcpmux: someone wants %s", service);

	/*
	 * Help is a required command, and lists available services,
	 * one per line.
	 */
	if (!strcasecmp(service, "help")) {
		for (sep = servtab; sep; sep = sep->se_next) {
			if (!ISMUX(sep))
				continue;
			(void)write(s,sep->se_service,strlen(sep->se_service));
			strwrite(s, "\r\n");
		}
		return (NULL);
	}

	/* Try matching a service in inetd.conf with the request */
	for (sep = servtab; sep; sep = sep->se_next) {
		if (!ISMUX(sep))
			continue;
		if (!strcasecmp(service, sep->se_service)) {
			if (ISMUXPLUS(sep)) {
				strwrite(s, "+Go\r\n");
			}
			return (sep);
		}
	}
	strwrite(s, "-Service not available\r\n");
	return (NULL);
}

/*
 *	          System configuration routines
 *
 *	    Written by Toshiharu OHNO (tony-o@iij.ad.jp)
 *
 *   Copyright (C) 1993, Internet Initiative Japan, Inc. All rights reserverd.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the Internet Initiative Japan, Inc.  The name of the
 * IIJ may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $FreeBSD: src/usr.sbin/ppp/systems.c,v 1.58.2.3 2000/04/03 22:58:00 brian Exp $
 *
 *  TODO:
 */
#include <sys/param.h>

#include <ctype.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "defs.h"
#include "command.h"
#include "log.h"
#include "id.h"
#include "systems.h"

#define issep(ch) ((ch) == ' ' || (ch) == '\t')

FILE *
OpenSecret(const char *file)
{
  FILE *fp;
  char line[100];

  snprintf(line, sizeof line, "%s/%s", _PATH_PPP, file);
  fp = ID0fopen(line, "r");
  if (fp == NULL)
    log_Printf(LogWARN, "OpenSecret: Can't open %s.\n", line);
  return (fp);
}

void
CloseSecret(FILE *fp)
{
  fclose(fp);
}

/* Move string from ``from'' to ``to'', interpreting ``~'' and $.... */
const char *
InterpretArg(const char *from, char *to)
{
  char *ptr, *startto, *endto;
  struct passwd *pwd;
  int len, instring;
  const char *env;

  instring = 0;
  startto = to;
  endto = to + LINE_LEN - 1;

  while(issep(*from))
    from++;

  while (*from != '\0') {
    switch (*from) {
      case '"':
        instring = !instring;
        *to++ = *from++;
        break;
      case '\\':
        switch (*++from) {
          case '$':
          case '~':
            break;		/* Swallow the escapes */

          default:
            *to++ = '\\';	/* Pass the escapes on, maybe skipping \# */
            break;
        }
        *to++ = *from++;
        break;
      case '$':
        if (from[1] == '$') {
          *to = '\0';	/* For an empty var name below */
          from += 2;
        } else if (from[1] == '{') {
          ptr = strchr(from+2, '}');
          if (ptr) {
            len = ptr - from - 2;
            if (endto - to < len )
              len = endto - to;
            if (len) {
              strncpy(to, from+2, len);
              to[len] = '\0';
              from = ptr+1;
            } else {
              *to++ = *from++;
              continue;
            }
          } else {
            *to++ = *from++;
            continue;
          }
        } else {
          ptr = to;
          for (from++; (isalnum(*from) || *from == '_') && ptr < endto; from++)
            *ptr++ = *from;
          *ptr = '\0';
        }
        if (*to == '\0')
          *to++ = '$';
        else if ((env = getenv(to)) != NULL) {
          strncpy(to, env, endto - to);
          *endto = '\0';
          to += strlen(to);
        }
        break;

      case '~':
        ptr = strchr(++from, '/');
        len = ptr ? ptr - from : strlen(from);
        if (len == 0)
          pwd = getpwuid(ID0realuid());
        else {
          strncpy(to, from, len);
          to[len] = '\0';
          pwd = getpwnam(to);
        }
        if (pwd == NULL)
          *to++ = '~';
        else {
          strncpy(to, pwd->pw_dir, endto - to);
          *endto = '\0';
          to += strlen(to);
          from += len;
        }
        endpwent();
        break;

      default:
        *to++ = *from++;
        break;
    }
  }

  while (to > startto) {
    to--;
    if (!issep(*to)) {
      to++;
      break;
    }
  }
  *to = '\0';

  return from;
}

#define CTRL_UNKNOWN (0)
#define CTRL_INCLUDE (1)

static int
DecodeCtrlCommand(char *line, char *arg)
{
  const char *end;

  if (!strncasecmp(line, "include", 7) && issep(line[7])) {
    end = InterpretArg(line+8, arg);
    if (*end && *end != '#')
      log_Printf(LogWARN, "Usage: !include filename\n");
    else
      return CTRL_INCLUDE;
  }
  return CTRL_UNKNOWN;
}

/*
 * Initialised in system_IsValid(), set in ReadSystem(),
 * used by system_IsValid()
 */
static int modeok;
static int userok;
static int modereq;

int
AllowUsers(struct cmdargs const *arg)
{
  /* arg->bundle may be NULL (see system_IsValid()) ! */
  int f;
  struct passwd *pwd;

  userok = 0;
  pwd = getpwuid(ID0realuid());
  if (pwd != NULL)
    for (f = arg->argn; f < arg->argc; f++)
      if (!strcmp("*", arg->argv[f]) || !strcmp(pwd->pw_name, arg->argv[f])) {
        userok = 1;
        break;
      }
  endpwent();

  return 0;
}

int
AllowModes(struct cmdargs const *arg)
{
  /* arg->bundle may be NULL (see system_IsValid()) ! */
  int f, mode, allowed;

  allowed = 0;
  for (f = arg->argn; f < arg->argc; f++) {
    mode = Nam2mode(arg->argv[f]);
    if (mode == PHYS_NONE || mode == PHYS_ALL)
      log_Printf(LogWARN, "allow modes: %s: Invalid mode\n", arg->argv[f]);
    else
      allowed |= mode;
  }

  modeok = modereq & allowed ? 1 : 0;
  return 0;
}

static char *
strip(char *line)
{
  int len;

  len = strlen(line);
  while (len && (line[len-1] == '\n' || line[len-1] == '\r' ||
                 issep(line[len-1])))
    line[--len] = '\0';

  while (issep(*line))
    line++;

  if (*line == '#')
    *line = '\0';

  return line;
}

static int
xgets(char *buf, int buflen, FILE *fp)
{
  int len, n;

  n = 0;
  while (fgets(buf, buflen-1, fp)) {
    n++;
    buf[buflen-1] = '\0';
    len = strlen(buf);
    while (len && (buf[len-1] == '\n' || buf[len-1] == '\r'))
      buf[--len] = '\0';
    if (len && buf[len-1] == '\\') {
      buf += len - 1;
      buflen -= len - 1;
      if (!buflen)        /* No buffer space */
        break;
    } else
      break;
  }
  return n;
}

/* Values for ``how'' in ReadSystem */
#define SYSTEM_EXISTS	1
#define SYSTEM_VALIDATE	2
#define SYSTEM_EXEC	3

static char *
GetLabel(char *line, const char *filename, int linenum)
{
  char *argv[MAXARGS];
  int argc, len;

  argc = MakeArgs(line, argv, MAXARGS, PARSE_REDUCE);

  if (argc == 2 && !strcmp(argv[1], ":"))
    return argv[0];

  if (argc != 1 || (len = strlen(argv[0])) < 2 || argv[0][len-1] != ':') {
      log_Printf(LogWARN, "Bad label in %s (line %d) - missing colon\n",
                 filename, linenum);
      return NULL;
  }
  argv[0][len-1] = '\0';	/* Lose the ':' */

  return argv[0];
}

/* Returns -2 for ``file not found'' and -1 for ``label not found'' */

static int
ReadSystem(struct bundle *bundle, const char *name, const char *file,
           struct prompt *prompt, struct datalink *cx, int how)
{
  FILE *fp;
  char *cp;
  int n, len;
  char line[LINE_LEN];
  char filename[MAXPATHLEN];
  int linenum;
  int argc;
  char *argv[MAXARGS];
  int allowcmd;
  int indent;
  char arg[LINE_LEN];
  struct prompt *op;

  if (*file == '/')
    snprintf(filename, sizeof filename, "%s", file);
  else
    snprintf(filename, sizeof filename, "%s/%s", _PATH_PPP, file);
  fp = ID0fopen(filename, "r");
  if (fp == NULL) {
    log_Printf(LogDEBUG, "ReadSystem: Can't open %s.\n", filename);
    return -2;
  }
  log_Printf(LogDEBUG, "ReadSystem: Checking %s (%s).\n", name, filename);

  linenum = 0;
  while ((n = xgets(line, sizeof line, fp))) {
    linenum += n;
    if (issep(*line))
      continue;

    cp = strip(line);

    switch (*cp) {
    case '\0':			/* empty/comment */
      break;

    case '!':
      switch (DecodeCtrlCommand(cp+1, arg)) {
      case CTRL_INCLUDE:
        log_Printf(LogCOMMAND, "%s: Including \"%s\"\n", filename, arg);
        n = ReadSystem(bundle, name, arg, prompt, cx, how);
        log_Printf(LogCOMMAND, "%s: Done include of \"%s\"\n", filename, arg);
        if (!n) {
          fclose(fp);
          return 0;	/* got it */
        }
        break;
      default:
        log_Printf(LogWARN, "%s: %s: Invalid command\n", filename, cp);
        break;
      }
      break;

    default:
      if ((cp = GetLabel(cp, filename, linenum)) == NULL)
        continue;

      if (strcmp(cp, name) == 0) {
        /* We're in business */
        if (how == SYSTEM_EXISTS) {
          fclose(fp);
	  return 0;
	}
	while ((n = xgets(line, sizeof line, fp))) {
          linenum += n;
          indent = issep(*line);
          cp = strip(line);

          if (*cp == '\0')			/* empty / comment */
            continue;

          if (!indent) {			/* start of next section */
            if (*cp != '!' && how == SYSTEM_EXEC)
              cp = GetLabel(cp, filename, linenum);
            break;
          }

          len = strlen(cp);
          if ((argc = command_Expand_Interpret(cp, len, argv, cp - line)) < 0)
            log_Printf(LogWARN, "%s: %d: Syntax error\n", filename, linenum);
          else {
            allowcmd = argc > 0 && !strcasecmp(argv[0], "allow");
            if ((how != SYSTEM_EXEC && allowcmd) ||
                (how == SYSTEM_EXEC && !allowcmd)) {
              /*
               * Disable any context so that warnings are given to everyone,
               * including syslog.
               */
              op = log_PromptContext;
              log_PromptContext = NULL;
	      command_Run(bundle, argc, (char const *const *)argv, prompt,
                          name, cx);
              log_PromptContext = op;
            }
          }
        }

	fclose(fp);  /* everything read - get out */
	return 0;
      }
      break;
    }
  }
  fclose(fp);
  return -1;
}

const char *
system_IsValid(const char *name, struct prompt *prompt, int mode)
{
  /*
   * Note:  The ReadSystem() calls only result in calls to the Allow*
   * functions.  arg->bundle will be set to NULL for these commands !
   */
  int def, how, rs;

  def = !strcmp(name, "default");
  how = ID0realuid() == 0 ? SYSTEM_EXISTS : SYSTEM_VALIDATE;
  userok = 0;
  modeok = 1;
  modereq = mode;

  rs = ReadSystem(NULL, "default", CONFFILE, prompt, NULL, how);

  if (!def) {
    if (rs == -1)
      rs = 0;		/* we don't care that ``default'' doesn't exist */

    if (rs == 0)
      rs = ReadSystem(NULL, name, CONFFILE, prompt, NULL, how);

    if (rs == -1)
      return "Configuration label not found";

    if (rs == -2)
      return _PATH_PPP "/" CONFFILE ": File not found";
  }

  if (how == SYSTEM_EXISTS)
    userok = modeok = 1;

  if (!userok)
    return "User access denied";

  if (!modeok)
    return "Mode denied for this label";

  return NULL;
}

int
system_Select(struct bundle *bundle, const char *name, const char *file,
             struct prompt *prompt, struct datalink *cx)
{
  userok = modeok = 1;
  modereq = PHYS_ALL;
  return ReadSystem(bundle, name, file, prompt, cx, SYSTEM_EXEC);
}

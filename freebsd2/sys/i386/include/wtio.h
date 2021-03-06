#ifndef _MACHINE_WTIO_H
#define _MACHINE_WTIO_H

/*
 * Streamer tape driver for 386bsd and FreeBSD.
 * Supports Archive and Wangtek compatible QIC-02/QIC-36 boards.
 *
 * Copyright (C) 1993 by:
 *      Sergey Ryzhkov       <sir@kiae.su>
 *      Serge Vakulenko      <vak@zebub.msk.su>
 *
 * This software is distributed with NO WARRANTIES, not even the implied
 * warranties for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Authors grant any other persons or organizations permission to use
 * or modify this software as long as this message is kept with the software,
 * all derivative works or modified versions.
 *
 * This driver is derived from the old 386bsd Wangtek streamer tape driver,
 * made by Robert Baron at CMU, based on Intel sources.
 *
 * $Id: wtio.h,v 1.1 1996/02/22 00:31:35 joerg Exp $
 *
 */

/* formats for printing flags and error values */
#define WTDS_BITS "\20\1inuse\2read\3write\4start\5rmark\6wmark\7rew\10excep\11vol\12wo\13ro\14wany\15rany\16wp\17timer\20active"
#define WTER_BITS "\20\1eof\2bnl\3uda\4eom\5wrp\6usl\7cni\11por\12erm\13bpe\14bom\15mbd\16ndt\17ill"

#endif /* _MACHINE_WTIO_H */

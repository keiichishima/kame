/*
 * Mach Operating System
 * Copyright (c) 1992, 1991 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 *
 *	from: Mach, Revision 2.2  92/04/04  11:34:26  rpd
 *	$Id: serial.S,v 1.1.1.1.2.2 1997/08/17 04:31:42 kato Exp $
 */

/*
  Copyright 1988, 1989, 1990, 1991, 1992 
   by Intel Corporation, Santa Clara, California.

                All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appears in all
copies and that both the copyright notice and this permission notice
appear in supporting documentation, and that the name of Intel
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.

INTEL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL INTEL BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/*
 * Serial bootblock interface routines
 * Copyright (c) 1994, J"org Wunsch
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * THE AUTHOR ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  THE AUTHOR DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 */ 

/*
 * modified for PC-98 by KATO T. of Nagoya University
 */

	.file	"serial.s"

#include "asm.h"
#ifdef PC98
#include <pc98/pc98/sioreg.h>
#else
#include <i386/isa/sioreg.h>
#endif
	.text

/*
 * The serial port interface routines implement a simple polled i/o
 * interface to a standard serial port.  Due to the space restrictions
 * for the boot blocks, no BIOS support is used (since BIOS requires
 * expensive real/protected mode switches), instead the rudimentary
 * BIOS support is duplicated here.
 *
 * The base address for the i/o port is passed from the Makefile in
 * the COMCONSOLE preprocessor macro.  Console parameters are currently
 * hard-coded to 9600 Bd, 8 bit.  This can be changed in the
 * init_serial() function.
 */

/*
 * void serial_putc(char ch)
 *	send ch to serial port
 *
 */

ENTRY(serial_putc)
	push	%ebp
	mov	%esp, %ebp

	mov	$COMCONSOLE + 2, %edx	# line status reg
1:	inb	%dx, %al
	test	$0x01, %al
	jz	1b		# TX buffer not empty

	movb	0x8(%ebp), %al

	sub	$2, %edx	# TX output reg
	outb	%al, %dx	# send this one

	pop	%ebp
	ret

/*
 * int serial_getc(void)
 *	read a character from serial port
 */

ENTRY(serial_getc)
	push	%ebp
	mov	%esp, %ebp

	mov	$COMCONSOLE + 2, %edx	# line status reg
1:
	inb	%dx, %al
	testb	$0x02, %al
	jz	1b		# no RX char available

	xor	%eax, %eax
	sub	$2, %edx	# RX buffer reg
	inb	%dx, %al	# fetch (first) character

	cmp	$0x7F, %eax	# make DEL...
	jne	2f
	mov	$0x08, %eax	# look like BS
2:
	pop	%ebp
	ret

/*
 * int serial_ischar(void)
 *       if there is a character pending, return true; otherwise return 0
 */
ENTRY(serial_ischar)
	push	%ebp
	mov	%esp, %ebp

	xorl	%eax, %eax
	mov	$COMCONSOLE + 2, %edx	# line status reg
	inb	%dx, %al
	andb	$0x02, %al		# RX char available?

	pop	%ebp
	ret

/*
 * void init_serial(void)
 * 	initialize the serial console port to 9600 Bd, 8 bpc
 */
ENTRY(init_serial)
	push	%ebp
	mov	%esp, %ebp

	/* set 8253 */
	movb	0xb6, %al
	outb	%al, $0x77
	movl	$COMCONSOLE_CLK, %eax
	outb	%al, $0x75
	inb	$0x5f, %al
	movb	%ah, %al
	outb	%al, $0x75

	/* inhibit com int */
	inb	$0x35, %al
	andb	$0xf8, %al
	movb	%al, %ah
	inb	$0x5f, %al
	movb	%ah, %al
	outb	%al, $0x35

	inb	$0x02, %al
	orb	$0x10, %al
	outb	%al, $0x02
	
	/* dummy command */
	xor	%al,%al
	mov	$COMCONSOLE + 2, %edx
	outb	%al, %dx
	inb	$0x5f, %al
	xor	%al,%al
	outb	%al, %dx
	inb	$0x5f, %al
	xor	%al,%al
	outb	%al, %dx
	inb	$0x5f, %al

	/* RESET 8251 */
	movb	$0x40, %al
	outb	%al, %dx

	movb	$COMCONSOLE_MODE , %al
	andb	$0xfc, %al
	orb	$0x02, %al	/* factor = 1/16 */
	outb	%al, %dx
	inb	$0x5f, %al

	/* start RS-232C */
	movb	$0x37, %al
	outb	%al, %dx

	pop	%ebp
	ret

/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Berkeley Software Design Inc's name may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BERKELEY SOFTWARE DESIGN INC ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL BERKELEY SOFTWARE DESIGN INC BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from BSDI $Id: mutex.h,v 2.7.2.35 2000/04/27 03:10:26 cp Exp $
 * $FreeBSD: src/sys/amd64/include/mutex.h,v 1.37 2003/05/01 01:05:23 peter Exp $
 */

#ifndef _MACHINE_MUTEX_H_
#define _MACHINE_MUTEX_H_

#ifndef LOCORE

#ifdef _KERNEL

/* Global locks */
extern struct mtx	clock_lock;

#endif	/* _KERNEL */

#else	/* !LOCORE */

/*
 * Simple assembly macros to get and release mutexes.
 *
 * Note: All of these macros accept a "flags" argument and are analoguous
 *	 to the mtx_lock_flags and mtx_unlock_flags general macros. If one
 *	 desires to not pass a flag, the value 0 may be passed as second
 *	 argument.
 *
 * XXX: We only have MTX_LOCK_SPIN and MTX_UNLOCK_SPIN for now, since that's
 *	all we use right now. We should add MTX_LOCK and MTX_UNLOCK (for sleep
 *	locks) in the near future, however.
 */
#define MTX_LOCK_SPIN(lck, flags)					\
	pushq $0 ;							\
	pushq $0 ;							\
	pushq $flags ;							\
	pushq $lck ;							\
	call _mtx_lock_spin_flags ;					\
	addq $0x20, %rsp ;						\

#define MTX_UNLOCK_SPIN(lck)						\
	pushq $0 ;							\
	pushq $0 ;							\
	pushq $0 ;							\
	pushq $lck ;							\
	call _mtx_unlock_spin_flags ;					\
	addq $0x20, %rsp ;						\

#endif	/* !LOCORE */
#endif	/* __MACHINE_MUTEX_H */

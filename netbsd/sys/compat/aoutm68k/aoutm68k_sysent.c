/* $NetBSD: aoutm68k_sysent.c,v 1.9 2002/05/03 00:24:23 eeh Exp $ */

/*
 * System call switch table.
 *
 * DO NOT EDIT-- this file is automatically generated.
 * created from	NetBSD: syscalls.master,v 1.7 2002/05/03 00:20:56 eeh Exp 
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: aoutm68k_sysent.c,v 1.9 2002/05/03 00:24:23 eeh Exp $");

#if defined(_KERNEL_OPT)
#include "opt_ktrace.h"
#include "opt_nfsserver.h"
#include "opt_ntp.h"
#include "opt_compat_netbsd.h"
#include "opt_sysv.h"
#include "opt_compat_43.h"
#include "fs_lfs.h"
#include "fs_nfs.h"
#endif
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <compat/aoutm68k/aoutm68k_syscallargs.h>

#define	s(type)	sizeof(type)

struct sysent aoutm68k_sysent[] = {
	{ 0, 0, 0,
	    sys_nosys },			/* 0 = syscall (indir) */
	{ 1, s(struct sys_exit_args), 0,
	    sys_exit },				/* 1 = exit */
	{ 0, 0, 0,
	    sys_fork },				/* 2 = fork */
	{ 3, s(struct sys_read_args), 0,
	    sys_read },				/* 3 = read */
	{ 3, s(struct sys_write_args), 0,
	    sys_write },			/* 4 = write */
#ifdef COMPAT_AOUT_ALTPATH
	{ 3, s(struct aoutm68k_sys_open_args), 0,
	    aoutm68k_sys_open },		/* 5 = open */
#else
	{ 3, s(struct sys_open_args), 0,
	    sys_open },				/* 5 = open */
#endif
	{ 1, s(struct sys_close_args), 0,
	    sys_close },			/* 6 = close */
	{ 4, s(struct sys_wait4_args), 0,
	    sys_wait4 },			/* 7 = wait4 */
#ifdef COMPAT_43
	{ 2, s(struct compat_43_sys_creat_args), 0,
	    compat_43_sys_creat },		/* 8 = ocreat */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 8 = excluded compat_43_sys_creat */
#endif
#ifdef COMPAT_AOUT_ALTPATH
	{ 2, s(struct aoutm68k_sys_link_args), 0,
	    aoutm68k_sys_link },		/* 9 = link */
	{ 1, s(struct aoutm68k_sys_unlink_args), 0,
	    aoutm68k_sys_unlink },		/* 10 = unlink */
#else
	{ 2, s(struct sys_link_args), 0,
	    sys_link },				/* 9 = link */
	{ 1, s(struct sys_unlink_args), 0,
	    sys_unlink },			/* 10 = unlink */
#endif
	{ 0, 0, 0,
	    sys_nosys },			/* 11 = obsolete execv */
#ifdef COMPAT_AOUT_ALTPATH
	{ 1, s(struct aoutm68k_sys_chdir_args), 0,
	    aoutm68k_sys_chdir },		/* 12 = chdir */
#else
	{ 1, s(struct sys_chdir_args), 0,
	    sys_chdir },			/* 12 = chdir */
#endif
	{ 1, s(struct sys_fchdir_args), 0,
	    sys_fchdir },			/* 13 = fchdir */
	{ 3, s(struct sys_mknod_args), 0,
	    sys_mknod },			/* 14 = mknod */
#ifdef COMPAT_AOUT_ALTPATH
	{ 2, s(struct aoutm68k_sys_chmod_args), 0,
	    aoutm68k_sys_chmod },		/* 15 = chmod */
	{ 3, s(struct aoutm68k_sys_chown_args), 0,
	    aoutm68k_sys_chown },		/* 16 = chown */
#else
	{ 2, s(struct sys_chmod_args), 0,
	    sys_chmod },			/* 15 = chmod */
	{ 3, s(struct sys_chown_args), 0,
	    sys_chown },			/* 16 = chown */
#endif
	{ 1, s(struct sys_obreak_args), 0,
	    sys_obreak },			/* 17 = break */
	{ 3, s(struct sys_getfsstat_args), 0,
	    sys_getfsstat },			/* 18 = getfsstat */
#ifdef COMPAT_43
	{ 3, s(struct compat_43_sys_lseek_args), 0,
	    compat_43_sys_lseek },		/* 19 = olseek */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 19 = excluded compat_43_sys_lseek */
#endif
	{ 0, 0, SYCALL_MPSAFE | 0,
	    sys_getpid },			/* 20 = getpid */
	{ 4, s(struct sys_mount_args), 0,
	    sys_mount },			/* 21 = mount */
	{ 2, s(struct sys_unmount_args), 0,
	    sys_unmount },			/* 22 = unmount */
	{ 1, s(struct sys_setuid_args), 0,
	    sys_setuid },			/* 23 = setuid */
	{ 0, 0, 0,
	    sys_getuid },			/* 24 = getuid */
	{ 0, 0, 0,
	    sys_geteuid },			/* 25 = geteuid */
	{ 4, s(struct sys_ptrace_args), 0,
	    sys_ptrace },			/* 26 = ptrace */
	{ 3, s(struct sys_recvmsg_args), 0,
	    sys_recvmsg },			/* 27 = recvmsg */
	{ 3, s(struct sys_sendmsg_args), 0,
	    sys_sendmsg },			/* 28 = sendmsg */
	{ 6, s(struct sys_recvfrom_args), 0,
	    sys_recvfrom },			/* 29 = recvfrom */
	{ 3, s(struct sys_accept_args), 0,
	    sys_accept },			/* 30 = accept */
	{ 3, s(struct sys_getpeername_args), 0,
	    sys_getpeername },			/* 31 = getpeername */
	{ 3, s(struct sys_getsockname_args), 0,
	    sys_getsockname },			/* 32 = getsockname */
#ifdef COMPAT_AOUT_ALTPATH
	{ 2, s(struct aoutm68k_sys_access_args), 0,
	    aoutm68k_sys_access },		/* 33 = access */
	{ 2, s(struct aoutm68k_sys_chflags_args), 0,
	    aoutm68k_sys_chflags },		/* 34 = chflags */
#else
	{ 2, s(struct sys_access_args), 0,
	    sys_access },			/* 33 = access */
	{ 2, s(struct sys_chflags_args), 0,
	    sys_chflags },			/* 34 = chflags */
#endif
	{ 2, s(struct sys_fchflags_args), 0,
	    sys_fchflags },			/* 35 = fchflags */
	{ 0, 0, 0,
	    sys_sync },				/* 36 = sync */
	{ 2, s(struct sys_kill_args), 0,
	    sys_kill },				/* 37 = kill */
#ifdef COMPAT_43
	{ 2, s(struct aoutm68k_compat_43_sys_stat_args), 0,
	    aoutm68k_compat_43_sys_stat },	/* 38 = stat43 */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 38 = excluded aoutm68k_compat_43_sys_stat */
#endif
	{ 0, 0, 0,
	    sys_getppid },			/* 39 = getppid */
#ifdef COMPAT_43
	{ 2, s(struct aoutm68k_compat_43_sys_lstat_args), 0,
	    aoutm68k_compat_43_sys_lstat },	/* 40 = lstat43 */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 40 = excluded aoutm68k_compat_43_sys_lstat */
#endif
	{ 1, s(struct sys_dup_args), 0,
	    sys_dup },				/* 41 = dup */
	{ 0, 0, 0,
	    sys_pipe },				/* 42 = pipe */
	{ 0, 0, 0,
	    sys_getegid },			/* 43 = getegid */
	{ 4, s(struct sys_profil_args), 0,
	    sys_profil },			/* 44 = profil */
#if defined(KTRACE) || !defined(_KERNEL)
	{ 4, s(struct sys_ktrace_args), 0,
	    sys_ktrace },			/* 45 = ktrace */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 45 = excluded ktrace */
#endif
#ifdef COMPAT_13
	{ 3, s(struct compat_13_sys_sigaction_args), 0,
	    compat_13_sys_sigaction },		/* 46 = sigaction13 */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 46 = excluded compat_13_sys_sigaction */
#endif
	{ 0, 0, 0,
	    sys_getgid },			/* 47 = getgid */
#ifdef COMPAT_13
	{ 2, s(struct compat_13_sys_sigprocmask_args), 0,
	    compat_13_sys_sigprocmask },	/* 48 = sigprocmask13 */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 48 = excluded compat_13_sys_sigprocmask */
#endif
	{ 2, s(struct sys___getlogin_args), 0,
	    sys___getlogin },			/* 49 = __getlogin */
	{ 1, s(struct sys_setlogin_args), 0,
	    sys_setlogin },			/* 50 = setlogin */
	{ 1, s(struct sys_acct_args), 0,
	    sys_acct },				/* 51 = acct */
#ifdef COMPAT_13
	{ 0, 0, 0,
	    compat_13_sys_sigpending },		/* 52 = sigpending13 */
	{ 2, s(struct compat_13_sys_sigaltstack_args), 0,
	    compat_13_sys_sigaltstack },	/* 53 = sigaltstack13 */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 52 = excluded compat_13_sys_sigpending */
	{ 0, 0, 0,
	    sys_nosys },			/* 53 = excluded compat_13_sys_sigaltstack */
#endif
	{ 3, s(struct aoutm68k_sys_ioctl_args), 0,
	    aoutm68k_sys_ioctl },		/* 54 = ioctl */
#ifdef COMPAT_12
	{ 1, s(struct compat_12_sys_reboot_args), 0,
	    compat_12_sys_reboot },		/* 55 = oreboot */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 55 = excluded compat_12_sys_reboot */
#endif
#ifdef COMPAT_AOUT_ALTPATH
	{ 1, s(struct aoutm68k_sys_revoke_args), 0,
	    aoutm68k_sys_revoke },		/* 56 = revoke */
	{ 2, s(struct aoutm68k_sys_symlink_args), 0,
	    aoutm68k_sys_symlink },		/* 57 = symlink */
	{ 3, s(struct aoutm68k_sys_readlink_args), 0,
	    aoutm68k_sys_readlink },		/* 58 = readlink */
	{ 3, s(struct aoutm68k_sys_execve_args), 0,
	    aoutm68k_sys_execve },		/* 59 = execve */
#else
	{ 1, s(struct sys_revoke_args), 0,
	    sys_revoke },			/* 56 = revoke */
	{ 2, s(struct sys_symlink_args), 0,
	    sys_symlink },			/* 57 = symlink */
	{ 3, s(struct sys_readlink_args), 0,
	    sys_readlink },			/* 58 = readlink */
	{ 3, s(struct sys_execve_args), 0,
	    sys_execve },			/* 59 = execve */
#endif
	{ 1, s(struct sys_umask_args), 0,
	    sys_umask },			/* 60 = umask */
#ifdef COMPAT_AOUT_ALTPATH
	{ 1, s(struct aoutm68k_sys_chroot_args), 0,
	    aoutm68k_sys_chroot },		/* 61 = chroot */
#else
	{ 1, s(struct sys_chroot_args), 0,
	    sys_chroot },			/* 61 = chroot */
#endif
#ifdef COMPAT_43
	{ 2, s(struct aoutm68k_compat_43_sys_fstat_args), 0,
	    aoutm68k_compat_43_sys_fstat },	/* 62 = fstat43 */
	{ 4, s(struct compat_43_sys_getkerninfo_args), 0,
	    compat_43_sys_getkerninfo },	/* 63 = ogetkerninfo */
	{ 0, 0, 0,
	    compat_43_sys_getpagesize },	/* 64 = ogetpagesize */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 62 = excluded aoutm68k_compat_43_sys_fstat */
	{ 0, 0, 0,
	    sys_nosys },			/* 63 = excluded compat_43_sys_getkerninfo */
	{ 0, 0, 0,
	    sys_nosys },			/* 64 = excluded compat_43_sys_getpagesize */
#endif
#ifdef COMPAT_12
	{ 2, s(struct compat_12_sys_msync_args), 0,
	    compat_12_sys_msync },		/* 65 = msync */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 65 = excluded compat_12_sys_msync */
#endif
	{ 0, 0, 0,
	    sys_vfork },			/* 66 = vfork */
	{ 0, 0, 0,
	    sys_nosys },			/* 67 = obsolete vread */
	{ 0, 0, 0,
	    sys_nosys },			/* 68 = obsolete vwrite */
	{ 1, s(struct sys_sbrk_args), 0,
	    sys_sbrk },				/* 69 = sbrk */
	{ 1, s(struct sys_sstk_args), 0,
	    sys_sstk },				/* 70 = sstk */
#ifdef COMPAT_43
	{ 6, s(struct compat_43_sys_mmap_args), 0,
	    compat_43_sys_mmap },		/* 71 = ommap */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 71 = excluded compat_43_sys_mmap */
#endif
	{ 1, s(struct sys_ovadvise_args), 0,
	    sys_ovadvise },			/* 72 = vadvise */
	{ 2, s(struct sys_munmap_args), 0,
	    sys_munmap },			/* 73 = munmap */
	{ 3, s(struct sys_mprotect_args), 0,
	    sys_mprotect },			/* 74 = mprotect */
	{ 3, s(struct sys_madvise_args), 0,
	    sys_madvise },			/* 75 = madvise */
	{ 0, 0, 0,
	    sys_nosys },			/* 76 = obsolete vhangup */
	{ 0, 0, 0,
	    sys_nosys },			/* 77 = obsolete vlimit */
	{ 3, s(struct sys_mincore_args), 0,
	    sys_mincore },			/* 78 = mincore */
	{ 2, s(struct sys_getgroups_args), 0,
	    sys_getgroups },			/* 79 = getgroups */
	{ 2, s(struct sys_setgroups_args), 0,
	    sys_setgroups },			/* 80 = setgroups */
	{ 0, 0, 0,
	    sys_getpgrp },			/* 81 = getpgrp */
	{ 2, s(struct sys_setpgid_args), 0,
	    sys_setpgid },			/* 82 = setpgid */
	{ 3, s(struct sys_setitimer_args), 0,
	    sys_setitimer },			/* 83 = setitimer */
#ifdef COMPAT_43
	{ 0, 0, 0,
	    compat_43_sys_wait },		/* 84 = owait */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 84 = excluded compat_43_sys_wait */
#endif
#ifdef COMPAT_12
	{ 1, s(struct compat_12_sys_swapon_args), 0,
	    compat_12_sys_swapon },		/* 85 = oswapon */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 85 = excluded compat_12_sys_swapon */
#endif
	{ 2, s(struct sys_getitimer_args), 0,
	    sys_getitimer },			/* 86 = getitimer */
#ifdef COMPAT_43
	{ 2, s(struct compat_43_sys_gethostname_args), 0,
	    compat_43_sys_gethostname },	/* 87 = ogethostname */
	{ 2, s(struct compat_43_sys_sethostname_args), 0,
	    compat_43_sys_sethostname },	/* 88 = osethostname */
	{ 0, 0, 0,
	    compat_43_sys_getdtablesize },	/* 89 = ogetdtablesize */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 87 = excluded compat_43_sys_gethostname */
	{ 0, 0, 0,
	    sys_nosys },			/* 88 = excluded compat_43_sys_sethostname */
	{ 0, 0, 0,
	    sys_nosys },			/* 89 = excluded compat_43_sys_getdtablesize */
#endif
	{ 2, s(struct sys_dup2_args), 0,
	    sys_dup2 },				/* 90 = dup2 */
	{ 0, 0, 0,
	    sys_nosys },			/* 91 = unimplemented getdopt */
	{ 3, s(struct sys_fcntl_args), 0,
	    sys_fcntl },			/* 92 = fcntl */
	{ 5, s(struct sys_select_args), 0,
	    sys_select },			/* 93 = select */
	{ 0, 0, 0,
	    sys_nosys },			/* 94 = unimplemented setdopt */
	{ 1, s(struct sys_fsync_args), 0,
	    sys_fsync },			/* 95 = fsync */
	{ 3, s(struct sys_setpriority_args), 0,
	    sys_setpriority },			/* 96 = setpriority */
	{ 3, s(struct sys_socket_args), 0,
	    sys_socket },			/* 97 = socket */
	{ 3, s(struct sys_connect_args), 0,
	    sys_connect },			/* 98 = connect */
#ifdef COMPAT_43
	{ 3, s(struct compat_43_sys_accept_args), 0,
	    compat_43_sys_accept },		/* 99 = oaccept */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 99 = excluded compat_43_sys_accept */
#endif
	{ 2, s(struct sys_getpriority_args), 0,
	    sys_getpriority },			/* 100 = getpriority */
#ifdef COMPAT_43
	{ 4, s(struct compat_43_sys_send_args), 0,
	    compat_43_sys_send },		/* 101 = osend */
	{ 4, s(struct compat_43_sys_recv_args), 0,
	    compat_43_sys_recv },		/* 102 = orecv */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 101 = excluded compat_43_sys_send */
	{ 0, 0, 0,
	    sys_nosys },			/* 102 = excluded compat_43_sys_recv */
#endif
#ifdef COMPAT_13
	{ 1, s(struct compat_13_sys_sigreturn_args), 0,
	    compat_13_sys_sigreturn },		/* 103 = sigreturn13 */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 103 = excluded compat_13_sys_sigreturn */
#endif
	{ 3, s(struct sys_bind_args), 0,
	    sys_bind },				/* 104 = bind */
	{ 5, s(struct sys_setsockopt_args), 0,
	    sys_setsockopt },			/* 105 = setsockopt */
	{ 2, s(struct sys_listen_args), 0,
	    sys_listen },			/* 106 = listen */
	{ 0, 0, 0,
	    sys_nosys },			/* 107 = obsolete vtimes */
#ifdef COMPAT_43
	{ 3, s(struct compat_43_sys_sigvec_args), 0,
	    compat_43_sys_sigvec },		/* 108 = osigvec */
	{ 1, s(struct compat_43_sys_sigblock_args), 0,
	    compat_43_sys_sigblock },		/* 109 = osigblock */
	{ 1, s(struct compat_43_sys_sigsetmask_args), 0,
	    compat_43_sys_sigsetmask },		/* 110 = osigsetmask */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 108 = excluded compat_43_sys_sigvec */
	{ 0, 0, 0,
	    sys_nosys },			/* 109 = excluded compat_43_sys_sigblock */
	{ 0, 0, 0,
	    sys_nosys },			/* 110 = excluded compat_43_sys_sigsetmask */
#endif
#ifdef COMPAT_13
	{ 1, s(struct compat_13_sys_sigsuspend_args), 0,
	    compat_13_sys_sigsuspend },		/* 111 = sigsuspend13 */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 111 = excluded compat_13_sys_sigsuspend */
#endif
#ifdef COMPAT_43
	{ 2, s(struct compat_43_sys_sigstack_args), 0,
	    compat_43_sys_sigstack },		/* 112 = osigstack */
	{ 3, s(struct compat_43_sys_recvmsg_args), 0,
	    compat_43_sys_recvmsg },		/* 113 = orecvmsg */
	{ 3, s(struct compat_43_sys_sendmsg_args), 0,
	    compat_43_sys_sendmsg },		/* 114 = osendmsg */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 112 = excluded compat_43_sys_sigstack */
	{ 0, 0, 0,
	    sys_nosys },			/* 113 = excluded compat_43_sys_recvmesg */
	{ 0, 0, 0,
	    sys_nosys },			/* 114 = excluded compat_43_sys_sendmesg */
#endif
	{ 0, 0, 0,
	    sys_nosys },			/* 115 = obsolete vtrace */
	{ 2, s(struct sys_gettimeofday_args), 0,
	    sys_gettimeofday },			/* 116 = gettimeofday */
	{ 2, s(struct sys_getrusage_args), 0,
	    sys_getrusage },			/* 117 = getrusage */
	{ 5, s(struct sys_getsockopt_args), 0,
	    sys_getsockopt },			/* 118 = getsockopt */
	{ 0, 0, 0,
	    sys_nosys },			/* 119 = obsolete resuba */
	{ 3, s(struct sys_readv_args), 0,
	    sys_readv },			/* 120 = readv */
	{ 3, s(struct sys_writev_args), 0,
	    sys_writev },			/* 121 = writev */
	{ 2, s(struct sys_settimeofday_args), 0,
	    sys_settimeofday },			/* 122 = settimeofday */
	{ 3, s(struct sys_fchown_args), 0,
	    sys_fchown },			/* 123 = fchown */
	{ 2, s(struct sys_fchmod_args), 0,
	    sys_fchmod },			/* 124 = fchmod */
#ifdef COMPAT_43
	{ 6, s(struct compat_43_sys_recvfrom_args), 0,
	    compat_43_sys_recvfrom },		/* 125 = orecvfrom */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 125 = excluded compat_43_sys_recvfrom */
#endif
	{ 2, s(struct sys_setreuid_args), 0,
	    sys_setreuid },			/* 126 = setreuid */
	{ 2, s(struct sys_setregid_args), 0,
	    sys_setregid },			/* 127 = setregid */
#ifdef COMPAT_AOUT_ALTPATH
	{ 2, s(struct aoutm68k_sys_rename_args), 0,
	    aoutm68k_sys_rename },		/* 128 = rename */
#else
	{ 2, s(struct sys_rename_args), 0,
	    sys_rename },			/* 128 = rename */
#endif
#ifdef COMPAT_43
#ifdef COMPAT_AOUT_ALTPATH
	{ 2, s(struct aoutm68k_compat_43_sys_truncate_args), 0,
	    aoutm68k_compat_43_sys_truncate },	/* 129 = otruncate */
#else
	{ 2, s(struct compat_43_sys_truncate_args), 0,
	    compat_43_sys_truncate },		/* 129 = otruncate */
#endif
	{ 2, s(struct compat_43_sys_ftruncate_args), 0,
	    compat_43_sys_ftruncate },		/* 130 = oftruncate */
#else
#ifdef COMPAT_AOUT_ALTPATH
	{ 0, 0, 0,
	    sys_nosys },			/* 129 = excluded aoutm68k_compat_43_sys_truncate */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 129 = excluded compat_43_sys_truncate */
#endif
	{ 0, 0, 0,
	    sys_nosys },			/* 130 = excluded compat_43_sys_ftruncate */
#endif
	{ 2, s(struct sys_flock_args), 0,
	    sys_flock },			/* 131 = flock */
	{ 2, s(struct sys_mkfifo_args), 0,
	    sys_mkfifo },			/* 132 = mkfifo */
	{ 6, s(struct sys_sendto_args), 0,
	    sys_sendto },			/* 133 = sendto */
	{ 2, s(struct sys_shutdown_args), 0,
	    sys_shutdown },			/* 134 = shutdown */
	{ 4, s(struct sys_socketpair_args), 0,
	    sys_socketpair },			/* 135 = socketpair */
	{ 2, s(struct sys_mkdir_args), 0,
	    sys_mkdir },			/* 136 = mkdir */
#ifdef COMPAT_AOUT_ALTPATH
	{ 1, s(struct aoutm68k_sys_rmdir_args), 0,
	    aoutm68k_sys_rmdir },		/* 137 = rmdir */
	{ 2, s(struct aoutm68k_sys_utimes_args), 0,
	    aoutm68k_sys_utimes },		/* 138 = utimes */
#else
	{ 1, s(struct sys_rmdir_args), 0,
	    sys_rmdir },			/* 137 = rmdir */
	{ 2, s(struct sys_utimes_args), 0,
	    sys_utimes },			/* 138 = utimes */
#endif
	{ 0, 0, 0,
	    sys_nosys },			/* 139 = obsolete 4.2 sigreturn */
	{ 2, s(struct sys_adjtime_args), 0,
	    sys_adjtime },			/* 140 = adjtime */
#ifdef COMPAT_43
	{ 3, s(struct compat_43_sys_getpeername_args), 0,
	    compat_43_sys_getpeername },	/* 141 = ogetpeername */
	{ 0, 0, 0,
	    compat_43_sys_gethostid },		/* 142 = ogethostid */
	{ 1, s(struct compat_43_sys_sethostid_args), 0,
	    compat_43_sys_sethostid },		/* 143 = osethostid */
	{ 2, s(struct compat_43_sys_getrlimit_args), 0,
	    compat_43_sys_getrlimit },		/* 144 = ogetrlimit */
	{ 2, s(struct compat_43_sys_setrlimit_args), 0,
	    compat_43_sys_setrlimit },		/* 145 = osetrlimit */
	{ 2, s(struct compat_43_sys_killpg_args), 0,
	    compat_43_sys_killpg },		/* 146 = okillpg */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 141 = excluded compat_43_sys_getpeername */
	{ 0, 0, 0,
	    sys_nosys },			/* 142 = excluded compat_43_sys_gethostid */
	{ 0, 0, 0,
	    sys_nosys },			/* 143 = excluded compat_43_sys_sethostid */
	{ 0, 0, 0,
	    sys_nosys },			/* 144 = excluded compat_43_sys_getrlimit */
	{ 0, 0, 0,
	    sys_nosys },			/* 145 = excluded compat_43_sys_setrlimit */
	{ 0, 0, 0,
	    sys_nosys },			/* 146 = excluded compat_43_sys_killpg */
#endif
	{ 0, 0, 0,
	    sys_setsid },			/* 147 = setsid */
	{ 4, s(struct sys_quotactl_args), 0,
	    sys_quotactl },			/* 148 = quotactl */
#ifdef COMPAT_43
	{ 0, 0, 0,
	    compat_43_sys_quota },		/* 149 = oquota */
	{ 3, s(struct compat_43_sys_getsockname_args), 0,
	    compat_43_sys_getsockname },	/* 150 = ogetsockname */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 149 = excluded compat_43_sys_quota */
	{ 0, 0, 0,
	    sys_nosys },			/* 150 = excluded compat_43_sys_getsockname */
#endif
	{ 0, 0, 0,
	    sys_nosys },			/* 151 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 152 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 153 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 154 = unimplemented */
#if defined(NFS) || defined(NFSSERVER) || !defined(_KERNEL)
	{ 2, s(struct sys_nfssvc_args), 0,
	    sys_nfssvc },			/* 155 = nfssvc */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 155 = excluded nfssvc */
#endif
#ifdef COMPAT_43
	{ 4, s(struct compat_43_sys_getdirentries_args), 0,
	    compat_43_sys_getdirentries },	/* 156 = ogetdirentries */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 156 = excluded compat_43_sys_getdirentries */
#endif
#ifdef COMPAT_AOUT_ALTPATH
	{ 2, s(struct aoutm68k_sys_statfs_args), 0,
	    aoutm68k_sys_statfs },		/* 157 = statfs */
#else
	{ 2, s(struct sys_statfs_args), 0,
	    sys_statfs },			/* 157 = statfs */
#endif
	{ 2, s(struct sys_fstatfs_args), 0,
	    sys_fstatfs },			/* 158 = fstatfs */
	{ 0, 0, 0,
	    sys_nosys },			/* 159 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 160 = unimplemented */
#if defined(NFS) || defined(NFSSERVER) || !defined(_KERNEL)
#ifdef COMPAT_AOUT_ALTPATH
	{ 2, s(struct aoutm68k_sys_getfh_args), 0,
	    aoutm68k_sys_getfh },		/* 161 = getfh */
#else
	{ 2, s(struct sys_getfh_args), 0,
	    sys_getfh },			/* 161 = getfh */
#endif
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 161 = excluded getfh */
#endif
#ifdef COMPAT_09
	{ 2, s(struct compat_09_sys_getdomainname_args), 0,
	    compat_09_sys_getdomainname },	/* 162 = ogetdomainname */
	{ 2, s(struct compat_09_sys_setdomainname_args), 0,
	    compat_09_sys_setdomainname },	/* 163 = osetdomainname */
	{ 1, s(struct compat_09_sys_uname_args), 0,
	    compat_09_sys_uname },		/* 164 = ouname */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 162 = excluded compat_09_sys_getdomainname */
	{ 0, 0, 0,
	    sys_nosys },			/* 163 = excluded compat_09_sys_setdomainname */
	{ 0, 0, 0,
	    sys_nosys },			/* 164 = excluded compat_09_sys_uname */
#endif
	{ 2, s(struct sys_sysarch_args), 0,
	    sys_sysarch },			/* 165 = sysarch */
	{ 0, 0, 0,
	    sys_nosys },			/* 166 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 167 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 168 = unimplemented */
#if (defined(SYSVSEM) || !defined(_KERNEL)) && !defined(_LP64) && defined(COMPAT_10)
	{ 5, s(struct compat_10_sys_semsys_args), 0,
	    compat_10_sys_semsys },		/* 169 = osemsys */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 169 = excluded 1.0 semsys */
#endif
#if (defined(SYSVMSG) || !defined(_KERNEL)) && !defined(_LP64) && defined(COMPAT_10)
	{ 6, s(struct compat_10_sys_msgsys_args), 0,
	    compat_10_sys_msgsys },		/* 170 = omsgsys */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 170 = excluded 1.0 msgsys */
#endif
#if (defined(SYSVSHM) || !defined(_KERNEL)) && !defined(_LP64) && defined(COMPAT_10)
	{ 4, s(struct compat_10_sys_shmsys_args), 0,
	    compat_10_sys_shmsys },		/* 171 = oshmsys */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 171 = excluded 1.0 shmsys */
#endif
	{ 0, 0, 0,
	    sys_nosys },			/* 172 = unimplemented */
	{ 5, s(struct sys_pread_args), 0,
	    sys_pread },			/* 173 = pread */
	{ 5, s(struct sys_pwrite_args), 0,
	    sys_pwrite },			/* 174 = pwrite */
	{ 1, s(struct sys_ntp_gettime_args), 0,
	    sys_ntp_gettime },			/* 175 = ntp_gettime */
#if defined(NTP) || !defined(_KERNEL)
	{ 1, s(struct sys_ntp_adjtime_args), 0,
	    sys_ntp_adjtime },			/* 176 = ntp_adjtime */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 176 = excluded ntp_adjtime */
#endif
	{ 0, 0, 0,
	    sys_nosys },			/* 177 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 178 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 179 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 180 = unimplemented */
	{ 1, s(struct sys_setgid_args), 0,
	    sys_setgid },			/* 181 = setgid */
	{ 1, s(struct sys_setegid_args), 0,
	    sys_setegid },			/* 182 = setegid */
	{ 1, s(struct sys_seteuid_args), 0,
	    sys_seteuid },			/* 183 = seteuid */
#if defined(LFS) || !defined(_KERNEL)
	{ 3, s(struct sys_lfs_bmapv_args), 0,
	    sys_lfs_bmapv },			/* 184 = lfs_bmapv */
	{ 3, s(struct sys_lfs_markv_args), 0,
	    sys_lfs_markv },			/* 185 = lfs_markv */
	{ 2, s(struct sys_lfs_segclean_args), 0,
	    sys_lfs_segclean },			/* 186 = lfs_segclean */
	{ 2, s(struct sys_lfs_segwait_args), 0,
	    sys_lfs_segwait },			/* 187 = lfs_segwait */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 184 = excluded lfs_bmapv */
	{ 0, 0, 0,
	    sys_nosys },			/* 185 = excluded lfs_markv */
	{ 0, 0, 0,
	    sys_nosys },			/* 186 = excluded lfs_segclean */
	{ 0, 0, 0,
	    sys_nosys },			/* 187 = excluded lfs_segwait */
#endif
#ifdef COMPAT_12
	{ 2, s(struct aoutm68k_compat_12_sys_stat_args), 0,
	    aoutm68k_compat_12_sys_stat },	/* 188 = stat12 */
	{ 2, s(struct aoutm68k_compat_12_sys_fstat_args), 0,
	    aoutm68k_compat_12_sys_fstat },	/* 189 = fstat12 */
	{ 2, s(struct aoutm68k_compat_12_sys_lstat_args), 0,
	    aoutm68k_compat_12_sys_lstat },	/* 190 = lstat12 */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 188 = excluded aoutm68k_compat_12_sys_stat */
	{ 0, 0, 0,
	    sys_nosys },			/* 189 = excluded aoutm68k_compat_12_sys_fstat */
	{ 0, 0, 0,
	    sys_nosys },			/* 190 = excluded aoutm68k_compat_12_sys_lstat */
#endif
#ifdef COMPAT_AOUT_ALTPATH
	{ 2, s(struct aoutm68k_sys_pathconf_args), 0,
	    aoutm68k_sys_pathconf },		/* 191 = pathconf */
#else
	{ 2, s(struct sys_pathconf_args), 0,
	    sys_pathconf },			/* 191 = pathconf */
#endif
	{ 2, s(struct sys_fpathconf_args), 0,
	    sys_fpathconf },			/* 192 = fpathconf */
	{ 0, 0, 0,
	    sys_nosys },			/* 193 = unimplemented */
	{ 2, s(struct sys_getrlimit_args), 0,
	    sys_getrlimit },			/* 194 = getrlimit */
	{ 2, s(struct sys_setrlimit_args), 0,
	    sys_setrlimit },			/* 195 = setrlimit */
#ifdef COMPAT_12
	{ 4, s(struct compat_12_sys_getdirentries_args), 0,
	    compat_12_sys_getdirentries },	/* 196 = getdirentries */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 196 = excluded compat_12_sys_getdirentries */
#endif
	{ 7, s(struct sys_mmap_args), 0,
	    sys_mmap },				/* 197 = mmap */
	{ 0, 0, 0,
	    sys_nosys },			/* 198 = __syscall (indir) */
	{ 4, s(struct sys_lseek_args), 0,
	    sys_lseek },			/* 199 = lseek */
#ifdef COMPAT_AOUT_ALTPATH
	{ 3, s(struct aoutm68k_sys_truncate_args), 0,
	    aoutm68k_sys_truncate },		/* 200 = truncate */
#else
	{ 3, s(struct sys_truncate_args), 0,
	    sys_truncate },			/* 200 = truncate */
#endif
	{ 3, s(struct sys_ftruncate_args), 0,
	    sys_ftruncate },			/* 201 = ftruncate */
	{ 6, s(struct sys___sysctl_args), 0,
	    sys___sysctl },			/* 202 = __sysctl */
	{ 2, s(struct sys_mlock_args), 0,
	    sys_mlock },			/* 203 = mlock */
	{ 2, s(struct sys_munlock_args), 0,
	    sys_munlock },			/* 204 = munlock */
#ifdef COMPAT_AOUT_ALTPATH
	{ 1, s(struct aoutm68k_sys_undelete_args), 0,
	    aoutm68k_sys_undelete },		/* 205 = undelete */
#else
	{ 1, s(struct sys_undelete_args), 0,
	    sys_undelete },			/* 205 = undelete */
#endif
	{ 2, s(struct sys_futimes_args), 0,
	    sys_futimes },			/* 206 = futimes */
	{ 1, s(struct sys_getpgid_args), 0,
	    sys_getpgid },			/* 207 = getpgid */
	{ 2, s(struct sys_reboot_args), 0,
	    sys_reboot },			/* 208 = reboot */
	{ 3, s(struct sys_poll_args), 0,
	    sys_poll },				/* 209 = poll */
#if defined(LKM) || !defined(_KERNEL)
	{ 0, 0, 0,
	    sys_lkmnosys },			/* 210 = lkmnosys */
	{ 0, 0, 0,
	    sys_lkmnosys },			/* 211 = lkmnosys */
	{ 0, 0, 0,
	    sys_lkmnosys },			/* 212 = lkmnosys */
	{ 0, 0, 0,
	    sys_lkmnosys },			/* 213 = lkmnosys */
	{ 0, 0, 0,
	    sys_lkmnosys },			/* 214 = lkmnosys */
	{ 0, 0, 0,
	    sys_lkmnosys },			/* 215 = lkmnosys */
	{ 0, 0, 0,
	    sys_lkmnosys },			/* 216 = lkmnosys */
	{ 0, 0, 0,
	    sys_lkmnosys },			/* 217 = lkmnosys */
	{ 0, 0, 0,
	    sys_lkmnosys },			/* 218 = lkmnosys */
	{ 0, 0, 0,
	    sys_lkmnosys },			/* 219 = lkmnosys */
#else	/* !LKM */
	{ 0, 0, 0,
	    sys_nosys },			/* 210 = excluded lkmnosys */
	{ 0, 0, 0,
	    sys_nosys },			/* 211 = excluded lkmnosys */
	{ 0, 0, 0,
	    sys_nosys },			/* 212 = excluded lkmnosys */
	{ 0, 0, 0,
	    sys_nosys },			/* 213 = excluded lkmnosys */
	{ 0, 0, 0,
	    sys_nosys },			/* 214 = excluded lkmnosys */
	{ 0, 0, 0,
	    sys_nosys },			/* 215 = excluded lkmnosys */
	{ 0, 0, 0,
	    sys_nosys },			/* 216 = excluded lkmnosys */
	{ 0, 0, 0,
	    sys_nosys },			/* 217 = excluded lkmnosys */
	{ 0, 0, 0,
	    sys_nosys },			/* 218 = excluded lkmnosys */
	{ 0, 0, 0,
	    sys_nosys },			/* 219 = excluded lkmnosys */
#endif	/* !LKM */
#if defined(SYSVSEM) || !defined(_KERNEL)
#ifdef COMPAT_14
	{ 4, s(struct compat_14_sys___semctl_args), 0,
	    compat_14_sys___semctl },		/* 220 = __semctl */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 220 = excluded compat_14_semctl */
#endif
	{ 3, s(struct sys_semget_args), 0,
	    sys_semget },			/* 221 = semget */
	{ 3, s(struct sys_semop_args), 0,
	    sys_semop },			/* 222 = semop */
	{ 1, s(struct sys_semconfig_args), 0,
	    sys_semconfig },			/* 223 = semconfig */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 220 = excluded compat_14_semctl */
	{ 0, 0, 0,
	    sys_nosys },			/* 221 = excluded semget */
	{ 0, 0, 0,
	    sys_nosys },			/* 222 = excluded semop */
	{ 0, 0, 0,
	    sys_nosys },			/* 223 = excluded semconfig */
#endif
#if defined(SYSVMSG) || !defined(_KERNEL)
#ifdef COMPAT_14
	{ 3, s(struct compat_14_sys_msgctl_args), 0,
	    compat_14_sys_msgctl },		/* 224 = msgctl */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 224 = excluded compat_14_sys_msgctl */
#endif
	{ 2, s(struct sys_msgget_args), 0,
	    sys_msgget },			/* 225 = msgget */
	{ 4, s(struct sys_msgsnd_args), 0,
	    sys_msgsnd },			/* 226 = msgsnd */
	{ 5, s(struct sys_msgrcv_args), 0,
	    sys_msgrcv },			/* 227 = msgrcv */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 224 = excluded compat_14_msgctl */
	{ 0, 0, 0,
	    sys_nosys },			/* 225 = excluded msgget */
	{ 0, 0, 0,
	    sys_nosys },			/* 226 = excluded msgsnd */
	{ 0, 0, 0,
	    sys_nosys },			/* 227 = excluded msgrcv */
#endif
#if defined(SYSVSHM) || !defined(_KERNEL)
	{ 3, s(struct sys_shmat_args), 0,
	    sys_shmat },			/* 228 = shmat */
#ifdef COMPAT_14
	{ 3, s(struct compat_14_sys_shmctl_args), 0,
	    compat_14_sys_shmctl },		/* 229 = shmctl */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 229 = excluded compat_14_sys_shmctl */
#endif
	{ 1, s(struct sys_shmdt_args), 0,
	    sys_shmdt },			/* 230 = shmdt */
	{ 3, s(struct sys_shmget_args), 0,
	    sys_shmget },			/* 231 = shmget */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 228 = excluded shmat */
	{ 0, 0, 0,
	    sys_nosys },			/* 229 = excluded compat_14_shmctl */
	{ 0, 0, 0,
	    sys_nosys },			/* 230 = excluded shmdt */
	{ 0, 0, 0,
	    sys_nosys },			/* 231 = excluded shmget */
#endif
	{ 2, s(struct sys_clock_gettime_args), 0,
	    sys_clock_gettime },		/* 232 = clock_gettime */
	{ 2, s(struct sys_clock_settime_args), 0,
	    sys_clock_settime },		/* 233 = clock_settime */
	{ 2, s(struct sys_clock_getres_args), 0,
	    sys_clock_getres },			/* 234 = clock_getres */
	{ 0, 0, 0,
	    sys_nosys },			/* 235 = unimplemented timer_create */
	{ 0, 0, 0,
	    sys_nosys },			/* 236 = unimplemented timer_delete */
	{ 0, 0, 0,
	    sys_nosys },			/* 237 = unimplemented timer_settime */
	{ 0, 0, 0,
	    sys_nosys },			/* 238 = unimplemented timer_gettime */
	{ 0, 0, 0,
	    sys_nosys },			/* 239 = unimplemented timer_getoverrun */
	{ 2, s(struct sys_nanosleep_args), 0,
	    sys_nanosleep },			/* 240 = nanosleep */
	{ 1, s(struct sys_fdatasync_args), 0,
	    sys_fdatasync },			/* 241 = fdatasync */
	{ 1, s(struct sys_mlockall_args), 0,
	    sys_mlockall },			/* 242 = mlockall */
	{ 0, 0, 0,
	    sys_munlockall },			/* 243 = munlockall */
	{ 0, 0, 0,
	    sys_nosys },			/* 244 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 245 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 246 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 247 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 248 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 249 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 250 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 251 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 252 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 253 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 254 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 255 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 256 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 257 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 258 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 259 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 260 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 261 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 262 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 263 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 264 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 265 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 266 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 267 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 268 = unimplemented */
	{ 0, 0, 0,
	    sys_nosys },			/* 269 = unimplemented */
#ifdef COMPAT_AOUT_ALTPATH
	{ 2, s(struct aoutm68k_sys___posix_rename_args), 0,
	    aoutm68k_sys___posix_rename },	/* 270 = __posix_rename */
#else
	{ 2, s(struct sys___posix_rename_args), 0,
	    sys___posix_rename },		/* 270 = __posix_rename */
#endif
	{ 3, s(struct sys_swapctl_args), 0,
	    sys_swapctl },			/* 271 = swapctl */
	{ 3, s(struct sys_getdents_args), 0,
	    sys_getdents },			/* 272 = getdents */
	{ 3, s(struct sys_minherit_args), 0,
	    sys_minherit },			/* 273 = minherit */
#ifdef COMPAT_AOUT_ALTPATH
	{ 2, s(struct aoutm68k_sys_lchmod_args), 0,
	    aoutm68k_sys_lchmod },		/* 274 = lchmod */
	{ 3, s(struct aoutm68k_sys_lchown_args), 0,
	    aoutm68k_sys_lchown },		/* 275 = lchown */
	{ 2, s(struct aoutm68k_sys_lutimes_args), 0,
	    aoutm68k_sys_lutimes },		/* 276 = lutimes */
#else
	{ 2, s(struct sys_lchmod_args), 0,
	    sys_lchmod },			/* 274 = lchmod */
	{ 3, s(struct sys_lchown_args), 0,
	    sys_lchown },			/* 275 = lchown */
	{ 2, s(struct sys_lutimes_args), 0,
	    sys_lutimes },			/* 276 = lutimes */
#endif
	{ 3, s(struct sys___msync13_args), 0,
	    sys___msync13 },			/* 277 = __msync13 */
	{ 2, s(struct aoutm68k_sys___stat13_args), 0,
	    aoutm68k_sys___stat13 },		/* 278 = __stat13 */
	{ 2, s(struct aoutm68k_sys___fstat13_args), 0,
	    aoutm68k_sys___fstat13 },		/* 279 = __fstat13 */
	{ 2, s(struct aoutm68k_sys___lstat13_args), 0,
	    aoutm68k_sys___lstat13 },		/* 280 = __lstat13 */
	{ 2, s(struct sys___sigaltstack14_args), 0,
	    sys___sigaltstack14 },		/* 281 = __sigaltstack14 */
	{ 0, 0, 0,
	    sys___vfork14 },			/* 282 = __vfork14 */
#ifdef COMPAT_AOUT_ALTPATH
	{ 3, s(struct aoutm68k_sys___posix_chown_args), 0,
	    aoutm68k_sys___posix_chown },	/* 283 = __posix_chown */
#else
	{ 3, s(struct sys___posix_chown_args), 0,
	    sys___posix_chown },		/* 283 = __posix_chown */
#endif
	{ 3, s(struct sys___posix_fchown_args), 0,
	    sys___posix_fchown },		/* 284 = __posix_fchown */
	{ 3, s(struct sys___posix_lchown_args), 0,
	    sys___posix_lchown },		/* 285 = __posix_lchown */
	{ 1, s(struct sys_getsid_args), 0,
	    sys_getsid },			/* 286 = getsid */
	{ 0, 0, 0,
	    sys_nosys },			/* 287 = unimplemented */
#if defined(KTRACE) || !defined(_KERNEL)
	{ 4, s(struct sys_fktrace_args), 0,
	    sys_fktrace },			/* 288 = fktrace */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 288 = excluded ktrace */
#endif
	{ 5, s(struct sys_preadv_args), 0,
	    sys_preadv },			/* 289 = preadv */
	{ 5, s(struct sys_pwritev_args), 0,
	    sys_pwritev },			/* 290 = pwritev */
	{ 3, s(struct sys___sigaction14_args), 0,
	    sys___sigaction14 },		/* 291 = __sigaction14 */
	{ 1, s(struct sys___sigpending14_args), 0,
	    sys___sigpending14 },		/* 292 = __sigpending14 */
	{ 3, s(struct sys___sigprocmask14_args), 0,
	    sys___sigprocmask14 },		/* 293 = __sigprocmask14 */
	{ 1, s(struct sys___sigsuspend14_args), 0,
	    sys___sigsuspend14 },		/* 294 = __sigsuspend14 */
	{ 1, s(struct sys___sigreturn14_args), 0,
	    sys___sigreturn14 },		/* 295 = __sigreturn14 */
	{ 2, s(struct sys___getcwd_args), 0,
	    sys___getcwd },			/* 296 = __getcwd */
	{ 1, s(struct sys_fchroot_args), 0,
	    sys_fchroot },			/* 297 = fchroot */
	{ 2, s(struct sys_fhopen_args), 0,
	    sys_fhopen },			/* 298 = fhopen */
	{ 2, s(struct aoutm68k_sys_fhstat_args), 0,
	    aoutm68k_sys_fhstat },		/* 299 = fhstat */
	{ 2, s(struct sys_fhstatfs_args), 0,
	    sys_fhstatfs },			/* 300 = fhstatfs */
#if defined(SYSVSEM) || !defined(_KERNEL)
	{ 4, s(struct sys_____semctl13_args), 0,
	    sys_____semctl13 },			/* 301 = ____semctl13 */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 301 = excluded ____semctl13 */
#endif
#if defined(SYSVMSG) || !defined(_KERNEL)
	{ 3, s(struct sys___msgctl13_args), 0,
	    sys___msgctl13 },			/* 302 = __msgctl13 */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 302 = excluded __msgctl13 */
#endif
#if defined(SYSVSHM) || !defined(_KERNEL)
	{ 3, s(struct sys___shmctl13_args), 0,
	    sys___shmctl13 },			/* 303 = __shmctl13 */
#else
	{ 0, 0, 0,
	    sys_nosys },			/* 303 = excluded __shmctl13 */
#endif
	{ 2, s(struct sys_lchflags_args), 0,
	    sys_lchflags },			/* 304 = lchflags */
	{ 0, 0, 0,
	    sys_issetugid },			/* 305 = issetugid */
};


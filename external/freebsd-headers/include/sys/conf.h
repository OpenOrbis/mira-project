/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 2000
 *	Poul-Henning Kamp.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)conf.h	8.5 (Berkeley) 1/9/95
 * $FreeBSD: release/9.0.0/sys/sys/conf.h 226577 2011-10-20 18:47:58Z kib $
 */

#ifndef _SYS_CONF_H_
#define	_SYS_CONF_H_

#ifdef _KERNEL
#include <sys/eventhandler.h>
#else
#include <sys/queue.h>
#endif

struct snapdata;
struct devfs_dirent;
struct cdevsw;
struct file;

struct cdev {
	void		*__si_reserved;
	u_int		si_flags;
#define	SI_ETERNAL	0x0001	/* never destroyed */
#define SI_ALIAS	0x0002	/* carrier of alias name */
#define SI_NAMED	0x0004	/* make_dev{_alias} has been called */
#define SI_CHEAPCLONE	0x0008	/* can be removed_dev'ed when vnode reclaims */
#define SI_CHILD	0x0010	/* child of another struct cdev **/
#define SI_DEVOPEN	0x0020	/* opened by device */
#define SI_CONSOPEN	0x0040	/* opened by console */
#define SI_DUMPDEV	0x0080	/* is kernel dumpdev */
#define SI_CANDELETE	0x0100	/* can do BIO_DELETE */
#define SI_CLONELIST	0x0200	/* on a clone list */
	struct timespec	si_atime;
	struct timespec	si_ctime;
	struct timespec	si_mtime;
	uid_t		si_uid;
	gid_t		si_gid;
	mode_t		si_mode;
	struct ucred	*si_cred;	/* cached clone-time credential */
	int		si_drv0;
	int		si_refcount;
	LIST_ENTRY(cdev)	si_list;
	LIST_ENTRY(cdev)	si_clone;
	LIST_HEAD(, cdev)	si_children;
	LIST_ENTRY(cdev)	si_siblings;
	struct cdev *si_parent;
	char		*si_name;
	void		*si_drv1, *si_drv2;
	struct cdevsw	*si_devsw;
	int		si_iosize_max;	/* maximum I/O size (for physio &al) */
	u_long		si_usecount;
	u_long		si_threadcount;
	union {
		struct snapdata *__sid_snapdata;
	} __si_u;
	char		__si_namebuf[SPECNAMELEN + 1];
};

#define si_snapdata	__si_u.__sid_snapdata

#ifdef _KERNEL

/*
 * Definitions of device driver entry switches
 */

struct bio;
struct buf;
struct thread;
struct uio;
struct knote;
struct clonedevs;
struct vm_object;
struct vnode;

/*
 * Note: d_thread_t is provided as a transition aid for those drivers
 * that treat struct proc/struct thread as an opaque data type and
 * exist in substantially the same form in both 4.x and 5.x.  Writers
 * of drivers that dips into the d_thread_t structure should use
 * struct thread or struct proc as appropriate for the version of the
 * OS they are using.  It is provided in lieu of each device driver
 * inventing its own way of doing this.  While it does violate style(9)
 * in a number of ways, this violation is deemed to be less
 * important than the benefits that a uniform API between releases
 * gives.
 *
 * Users of struct thread/struct proc that aren't device drivers should
 * not use d_thread_t.
 */

typedef struct thread d_thread_t;

typedef int d_open_t(struct cdev *dev, int oflags, int devtype, struct thread *td);
typedef int d_fdopen_t(struct cdev *dev, int oflags, struct thread *td, struct file *fp);
typedef int d_close_t(struct cdev *dev, int fflag, int devtype, struct thread *td);
typedef void d_strategy_t(struct bio *bp);
typedef int d_ioctl_t(struct cdev *dev, u_long cmd, caddr_t data,
		      int fflag, struct thread *td);

typedef int d_read_t(struct cdev *dev, struct uio *uio, int ioflag);
typedef int d_write_t(struct cdev *dev, struct uio *uio, int ioflag);
typedef int d_poll_t(struct cdev *dev, int events, struct thread *td);
typedef int d_kqfilter_t(struct cdev *dev, struct knote *kn);
typedef int d_mmap_t(struct cdev *dev, vm_ooffset_t offset, vm_paddr_t *paddr,
		     int nprot, vm_memattr_t *memattr);
typedef int d_mmap_single_t(struct cdev *cdev, vm_ooffset_t *offset,
    vm_size_t size, struct vm_object **object, int nprot);
typedef void d_purge_t(struct cdev *dev);

// PlayStation 4 addition, Credits: ChendoChap
typedef int d_mmap_single_ext_t(struct cdev *dev, vm_ooffset_t *offset, vm_size_t size, struct vm_object **object, vm_prot_t nprot, void* unk0, void* unk1);

typedef int dumper_t(
	void *_priv,		/* Private to the driver. */
	void *_virtual,		/* Virtual (mapped) address. */
	vm_offset_t _physical,	/* Physical address of virtual. */
	off_t _offset,		/* Byte-offset to write at. */
	size_t _length);	/* Number of bytes to dump. */

#endif /* _KERNEL */

/*
 * Types for d_flags.
 */
#define	D_TAPE	0x0001
#define	D_DISK	0x0002
#define	D_TTY	0x0004
#define	D_MEM	0x0008

#ifdef _KERNEL 

#define	D_TYPEMASK	0xffff

/*
 * Flags for d_flags which the drivers can set.
 */
#define	D_TRACKCLOSE	0x00080000	/* track all closes */
#define D_MMAP_ANON	0x00100000	/* special treatment in vm_mmap.c */
#define D_PSEUDO	0x00200000	/* make_dev() can return NULL */
#define D_NEEDGIANT	0x00400000	/* driver want Giant */
#define	D_NEEDMINOR	0x00800000	/* driver uses clone_create() */

/*
 * Version numbers.
 */
#define D_VERSION_00	0x20011966
#define D_VERSION_01	0x17032005	/* Add d_uid,gid,mode & kind */
#define D_VERSION_02	0x28042009	/* Add d_mmap_single */
#define D_VERSION_03	0x17122009	/* d_mmap takes memattr,vm_ooffset_t */
#define D_VERSION	D_VERSION_03

/*
 * Flags used for internal housekeeping
 */
#define D_INIT		0x80000000	/* cdevsw initialized */

/*
 * Character device switch table
 */
struct cdevsw {
	int			d_version;
	u_int			d_flags;
	const char		*d_name;
	d_open_t		*d_open;
	d_fdopen_t		*d_fdopen;
	d_close_t		*d_close;
	d_read_t		*d_read;
	d_write_t		*d_write;
	d_ioctl_t		*d_ioctl;
	d_poll_t		*d_poll;
	d_mmap_t		*d_mmap;
	d_strategy_t		*d_strategy;
	dumper_t		*d_dump;
	d_kqfilter_t		*d_kqfilter;
	d_purge_t		*d_purge;
	d_mmap_single_t		*d_mmap_single;

	// PlayStation 4 Addition, credits: ChendoChap
#if defined(MIRA_PLATFORM)
	d_mmap_single_ext_t *d_mmap_single_ext;
#endif

	int32_t			d_spare0[3];
	void			*d_spare1[3];

	/* These fields should not be messed with by drivers */
	LIST_HEAD(, cdev)	d_devs;
	int			d_spare2;
	union {
		struct cdevsw		*gianttrick;
		SLIST_ENTRY(cdevsw)	postfree_list;
	} __d_giant;
};

#if defined(MIRA_CHECKS)
static_assert(sizeof(struct cdevsw) == 0xC0, "devsw size invalid.");
#endif

#define	d_gianttrick		__d_giant.gianttrick
#define	d_postfree_list		__d_giant.postfree_list

struct module;

struct devsw_module_data {
	int	(*chainevh)(struct module *, int, void *); /* next handler */
	void	*chainarg;	/* arg for next event handler */
	/* Do not initialize fields hereafter */
};

#define DEV_MODULE(name, evh, arg)					\
static moduledata_t name##_mod = {					\
    #name,								\
    evh,								\
    arg									\
};									\
DECLARE_MODULE(name, name##_mod, SI_SUB_DRIVERS, SI_ORDER_MIDDLE)


void clone_setup(struct clonedevs **cdp);
void clone_cleanup(struct clonedevs **);
#define CLONE_UNITMASK 0xfffff
#define CLONE_FLAG0 (CLONE_UNITMASK + 1)
int clone_create(struct clonedevs **, struct cdevsw *, int *unit, struct cdev **dev, int extra);

int	count_dev(struct cdev *_dev);
void	destroy_dev(struct cdev *_dev);
int	destroy_dev_sched(struct cdev *dev);
int	destroy_dev_sched_cb(struct cdev *dev, void (*cb)(void *), void *arg);
void	destroy_dev_drain(struct cdevsw *csw);
void	drain_dev_clone_events(void);
struct cdevsw *dev_refthread(struct cdev *_dev, int *_ref);
struct cdevsw *devvn_refthread(struct vnode *vp, struct cdev **devp, int *_ref);
void	dev_relthread(struct cdev *_dev, int _ref);
void	dev_depends(struct cdev *_pdev, struct cdev *_cdev);
void	dev_ref(struct cdev *dev);
void	dev_refl(struct cdev *dev);
void	dev_rel(struct cdev *dev);
void	dev_strategy(struct cdev *dev, struct buf *bp);
struct cdev *make_dev(struct cdevsw *_devsw, int _unit, uid_t _uid, gid_t _gid,
		int _perms, const char *_fmt, ...) __printflike(6, 7);
struct cdev *make_dev_cred(struct cdevsw *_devsw, int _unit,
		struct ucred *_cr, uid_t _uid, gid_t _gid, int _perms,
		const char *_fmt, ...) __printflike(7, 8);
#define	MAKEDEV_REF		0x01
#define	MAKEDEV_WHTOUT		0x02
#define	MAKEDEV_NOWAIT		0x04
#define	MAKEDEV_WAITOK		0x08
#define	MAKEDEV_ETERNAL		0x10
#define	MAKEDEV_CHECKNAME	0x20
struct cdev *make_dev_credf(int _flags,
		struct cdevsw *_devsw, int _unit,
		struct ucred *_cr, uid_t _uid, gid_t _gid, int _mode,
		const char *_fmt, ...) __printflike(8, 9);
int	make_dev_p(int _flags, struct cdev **_cdev, struct cdevsw *_devsw,
		struct ucred *_cr, uid_t _uid, gid_t _gid, int _mode,
		const char *_fmt, ...) __printflike(8, 9);
struct cdev *make_dev_alias(struct cdev *_pdev, const char *_fmt, ...)
		__printflike(2, 3);
int	make_dev_alias_p(int _flags, struct cdev **_cdev, struct cdev *_pdev,
		const char *_fmt, ...) __printflike(4, 5);
int	make_dev_physpath_alias(int _flags, struct cdev **_cdev,
	        struct cdev *_pdev, struct cdev *_old_alias,
                const char *_physpath);
void	dev_lock(void);
void	dev_unlock(void);
void	setconf(void);

#ifdef KLD_MODULE
#define	MAKEDEV_ETERNAL_KLD	0
#else
#define	MAKEDEV_ETERNAL_KLD	MAKEDEV_ETERNAL
#endif

#define	dev2unit(d)	((d)->si_drv0)

typedef	void (*cdevpriv_dtr_t)(void *data);
int	devfs_get_cdevpriv(void **datap);
int	devfs_set_cdevpriv(void *priv, cdevpriv_dtr_t dtr);
void	devfs_clear_cdevpriv(void);
void	devfs_fpdrop(struct file *fp);	/* XXX This is not public KPI */

ino_t	devfs_alloc_cdp_inode(void);
void	devfs_free_cdp_inode(ino_t ino);

#define		UID_ROOT	0
#define		UID_BIN		3
#define		UID_UUCP	66
#define		UID_NOBODY	65534

#define		GID_WHEEL	0
#define		GID_KMEM	2
#define		GID_TTY		4
#define		GID_OPERATOR	5
#define		GID_BIN		7
#define		GID_GAMES	13
#define		GID_DIALER	68
#define		GID_NOBODY	65534

typedef void (*dev_clone_fn)(void *arg, struct ucred *cred, char *name,
	    int namelen, struct cdev **result);

int dev_stdclone(char *_name, char **_namep, const char *_stem, int *_unit);
EVENTHANDLER_DECLARE(dev_clone, dev_clone_fn);

/* Stuff relating to kernel-dump */

struct dumperinfo {
	dumper_t *dumper;	/* Dumping function. */
	void    *priv;		/* Private parts. */
	u_int   blocksize;	/* Size of block in bytes. */
	u_int	maxiosize;	/* Max size allowed for an individual I/O */
	off_t   mediaoffset;	/* Initial offset in bytes. */
	off_t   mediasize;	/* Space available in bytes. */
};

int set_dumper(struct dumperinfo *);
int dump_write(struct dumperinfo *, void *, vm_offset_t, off_t, size_t);
void dumpsys(struct dumperinfo *);
int doadump(boolean_t);
extern int dumping;		/* system is dumping */

#endif /* _KERNEL */

#endif /* !_SYS_CONF_H_ */

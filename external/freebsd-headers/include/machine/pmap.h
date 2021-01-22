/*-
 * Copyright (c) 2003 Peter Wemm.
 * Copyright (c) 1991 Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and William Jolitz of UUNET Technologies Inc.
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
 * Derived from hp300 version by Mike Hibler, this version by William
 * Jolitz uses a recursive map [a pde points to the page directory] to
 * map the page tables using the pagetables themselves. This is done to
 * reduce the impact on kernel virtual memory for lots of sparse address
 * space, and to reduce the cost of memory to each process.
 *
 *	from: hp300: @(#)pmap.h	7.2 (Berkeley) 12/16/90
 *	from: @(#)pmap.h	7.4 (Berkeley) 5/12/91
 * $FreeBSD: release/9.0.0/sys/amd64/include/pmap.h 222813 2011-06-07 08:46:13Z attilio $
 */

#ifndef _MACHINE_PMAP_H_
#define	_MACHINE_PMAP_H_

/*
 * Page-directory and page-table entries follow this format, with a few
 * of the fields not present here and there, depending on a lot of things.
 */
				/* ---- Intel Nomenclature ---- */
#define	PG_V		0x001	/* P	Valid			*/
#define PG_RW		0x002	/* R/W	Read/Write		*/
#define PG_U		0x004	/* U/S  User/Supervisor		*/
#define	PG_NC_PWT	0x008	/* PWT	Write through		*/
#define	PG_NC_PCD	0x010	/* PCD	Cache disable		*/
#define PG_A		0x020	/* A	Accessed		*/
#define	PG_M		0x040	/* D	Dirty			*/
#define	PG_PS		0x080	/* PS	Page size (0=4k,1=2M)	*/
#define	PG_PTE_PAT	0x080	/* PAT	PAT index		*/
#define	PG_G		0x100	/* G	Global			*/
#define	PG_AVAIL1	0x200	/*    /	Available for system	*/
#define	PG_AVAIL2	0x400	/*   <	programmers use		*/
#define	PG_AVAIL3	0x800	/*    \				*/
#define	PG_PDE_PAT	0x1000	/* PAT	PAT index		*/
#define	PG_NX		(1ul<<63) /* No-execute */


/* Our various interpretations of the above */
#define PG_W		PG_AVAIL1	/* "Wired" pseudoflag */
#define	PG_MANAGED	PG_AVAIL2
#define	PG_FRAME	(0x000ffffffffff000ul)
#define	PG_PS_FRAME	(0x000fffffffe00000ul)
#define	PG_PROT		(PG_RW|PG_U)	/* all protection bits . */
#define PG_N		(PG_NC_PWT|PG_NC_PCD)	/* Non-cacheable */

/* Page level cache control fields used to determine the PAT type */
#define PG_PDE_CACHE	(PG_PDE_PAT | PG_NC_PWT | PG_NC_PCD)
#define PG_PTE_CACHE	(PG_PTE_PAT | PG_NC_PWT | PG_NC_PCD)

/*
 * Promotion to a 2MB (PDE) page mapping requires that the corresponding 4KB
 * (PTE) page mappings have identical settings for the following fields:
 */
#define	PG_PTE_PROMOTE	(PG_NX | PG_MANAGED | PG_W | PG_G | PG_PTE_PAT | \
	    PG_M | PG_A | PG_NC_PCD | PG_NC_PWT | PG_U | PG_RW | PG_V)

/*
 * Page Protection Exception bits
 */

#define PGEX_P		0x01	/* Protection violation vs. not present */
#define PGEX_W		0x02	/* during a Write cycle */
#define PGEX_U		0x04	/* access from User mode (UPL) */
#define PGEX_RSV	0x08	/* reserved PTE field is non-zero */
#define PGEX_I		0x10	/* during an instruction fetch */

/*
 * Pte related macros.  This is complicated by having to deal with
 * the sign extension of the 48th bit.
 */
#define KVADDR(l4, l3, l2, l1) ( \
	((unsigned long)-1 << 47) | \
	((unsigned long)(l4) << PML4SHIFT) | \
	((unsigned long)(l3) << PDPSHIFT) | \
	((unsigned long)(l2) << PDRSHIFT) | \
	((unsigned long)(l1) << PAGE_SHIFT))

#define UVADDR(l4, l3, l2, l1) ( \
	((unsigned long)(l4) << PML4SHIFT) | \
	((unsigned long)(l3) << PDPSHIFT) | \
	((unsigned long)(l2) << PDRSHIFT) | \
	((unsigned long)(l1) << PAGE_SHIFT))

/* Initial number of kernel page tables. */
#ifndef NKPT
#define	NKPT		32
#endif

#define NKPML4E		1		/* number of kernel PML4 slots */
#define NKPDPE		howmany(NKPT, NPDEPG)/* number of kernel PDP slots */

#define	NUPML4E		(NPML4EPG/2)	/* number of userland PML4 pages */
#define	NUPDPE		(NUPML4E*NPDPEPG)/* number of userland PDP pages */
#define	NUPDE		(NUPDPE*NPDEPG)	/* number of userland PD entries */

/*
 * NDMPML4E is the number of PML4 entries that are used to implement the
 * direct map.  It must be a power of two.
 */
#define	NDMPML4E	2

/*
 * The *PDI values control the layout of virtual memory.  The starting address
 * of the direct map, which is controlled by DMPML4I, must be a multiple of
 * its size.  (See the PHYS_TO_DMAP() and DMAP_TO_PHYS() macros.)
 */
#define	PML4PML4I	(NPML4EPG/2)	/* Index of recursive pml4 mapping */

#define	KPML4I		(NPML4EPG-1)	/* Top 512GB for KVM */
#define	DMPML4I		rounddown(KPML4I - NDMPML4E, NDMPML4E) /* Below KVM */

#define	KPDPI		(NPDPEPG-2)	/* kernbase at -2GB */

/*
 * XXX doesn't really belong here I guess...
 */
#define ISA_HOLE_START    0xa0000
#define ISA_HOLE_LENGTH (0x100000-ISA_HOLE_START)

#ifndef LOCORE

#include <sys/queue.h>
#include <sys/_cpuset.h>
#include <sys/_lock.h>
#include <sys/_mutex.h>

typedef u_int64_t pd_entry_t;
typedef u_int64_t pt_entry_t;
typedef u_int64_t pdp_entry_t;
typedef u_int64_t pml4_entry_t;

#define	PML4ESHIFT	(3)
#define	PDPESHIFT	(3)
#define	PTESHIFT	(3)
#define	PDESHIFT	(3)

/*
 * Address of current address space page table maps and directories.
 */
#ifdef _KERNEL
#define	addr_PTmap	(KVADDR(PML4PML4I, 0, 0, 0))
#define	addr_PDmap	(KVADDR(PML4PML4I, PML4PML4I, 0, 0))
#define	addr_PDPmap	(KVADDR(PML4PML4I, PML4PML4I, PML4PML4I, 0))
#define	addr_PML4map	(KVADDR(PML4PML4I, PML4PML4I, PML4PML4I, PML4PML4I))
#define	addr_PML4pml4e	(addr_PML4map + (PML4PML4I * sizeof(pml4_entry_t)))
#define	PTmap		((pt_entry_t *)(addr_PTmap))
#define	PDmap		((pd_entry_t *)(addr_PDmap))
#define	PDPmap		((pd_entry_t *)(addr_PDPmap))
#define	PML4map		((pd_entry_t *)(addr_PML4map))
#define	PML4pml4e	((pd_entry_t *)(addr_PML4pml4e))

extern u_int64_t KPDPphys;	/* physical address of kernel level 3 */
extern u_int64_t KPML4phys;	/* physical address of kernel level 4 */

/*
 * virtual address to page table entry and
 * to physical address.
 * Note: these work recursively, thus vtopte of a pte will give
 * the corresponding pde that in turn maps it.
 */
pt_entry_t *vtopte(vm_offset_t);
#define	vtophys(va)	pmap_kextract(((vm_offset_t) (va)))

static __inline pt_entry_t
pte_load(pt_entry_t *ptep)
{
	pt_entry_t r;

	r = *ptep;
	return (r);
}

static __inline pt_entry_t
pte_load_store(pt_entry_t *ptep, pt_entry_t pte)
{
	pt_entry_t r;

	__asm __volatile(
	    "xchgq %0,%1"
	    : "=m" (*ptep),
	      "=r" (r)
	    : "1" (pte),
	      "m" (*ptep));
	return (r);
}

#define	pte_load_clear(pte)	atomic_readandclear_long(pte)

static __inline void
pte_store(pt_entry_t *ptep, pt_entry_t pte)
{

	*ptep = pte;
}

#define	pte_clear(ptep)		pte_store((ptep), (pt_entry_t)0ULL)

#define	pde_store(pdep, pde)	pte_store((pdep), (pde))

extern pt_entry_t pg_nx;

#endif /* _KERNEL */

/*
 * Pmap stuff
 */
struct	pv_entry;
struct	pv_chunk;

struct md_page {
	TAILQ_HEAD(,pv_entry)	pv_list;
	int			pat_mode;
};

/*
 * The kernel virtual address (KVA) of the level 4 page table page is always
 * within the direct map (DMAP) region.
 */
struct pmap {
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_750
	uint64_t pm_unk;
#endif
	struct mtx		pm_mtx;
	pml4_entry_t		*pm_pml4;	/* KVA of level 4 page table */
	TAILQ_HEAD(,pv_chunk)	pm_pvchunk;	/* list of mappings in pmap */
	cpuset_t		pm_active;	/* active on cpus */
	/* spare u_int here due to padding */
	struct pmap_statistics	pm_stats;	/* pmap statistics */
	vm_page_t		pm_root;	/* spare page table pages */
};

typedef struct pmap	*pmap_t;

#ifdef _KERNEL
extern struct pmap	kernel_pmap_store;
#define kernel_pmap	(&kernel_pmap_store)

#define	PMAP_LOCK(pmap)		mtx_lock(&(pmap)->pm_mtx)
#define	PMAP_LOCK_ASSERT(pmap, type) \
				mtx_assert(&(pmap)->pm_mtx, (type))
#define	PMAP_LOCK_DESTROY(pmap)	mtx_destroy(&(pmap)->pm_mtx)
#define	PMAP_LOCK_INIT(pmap)	mtx_init(&(pmap)->pm_mtx, "pmap", \
				    NULL, MTX_DEF | MTX_DUPOK)
#define	PMAP_LOCKED(pmap)	mtx_owned(&(pmap)->pm_mtx)
#define	PMAP_MTX(pmap)		(&(pmap)->pm_mtx)
#define	PMAP_TRYLOCK(pmap)	mtx_trylock(&(pmap)->pm_mtx)
#define	PMAP_UNLOCK(pmap)	mtx_unlock(&(pmap)->pm_mtx)
#endif

/*
 * For each vm_page_t, there is a list of all currently valid virtual
 * mappings of that page.  An entry is a pv_entry_t, the list is pv_list.
 */
typedef struct pv_entry {
	vm_offset_t	pv_va;		/* virtual address for mapping */
	TAILQ_ENTRY(pv_entry)	pv_list;
} *pv_entry_t;

/*
 * pv_entries are allocated in chunks per-process.  This avoids the
 * need to track per-pmap assignments.
 */
#define	_NPCM	3
#define	_NPCPV	168
struct pv_chunk {
	pmap_t			pc_pmap;
	TAILQ_ENTRY(pv_chunk)	pc_list;
	uint64_t		pc_map[_NPCM];	/* bitmap; 1 = free */
	uint64_t		pc_spare[2];
	struct pv_entry		pc_pventry[_NPCPV];
};

#ifdef	_KERNEL

extern caddr_t	CADDR1;
extern pt_entry_t *CMAP1;
extern vm_paddr_t phys_avail[];
extern vm_paddr_t dump_avail[];
extern vm_offset_t virtual_avail;
extern vm_offset_t virtual_end;

#define	pmap_page_get_memattr(m)	((vm_memattr_t)(m)->md.pat_mode)
#define	pmap_unmapbios(va, sz)	pmap_unmapdev((va), (sz))

void	pmap_bootstrap(vm_paddr_t *);
int	pmap_change_attr(vm_offset_t, vm_size_t, int);
void	pmap_demote_DMAP(vm_paddr_t base, vm_size_t len, boolean_t invalidate);
void	pmap_init_pat(void);
void	pmap_kenter(vm_offset_t va, vm_paddr_t pa);
void	*pmap_kenter_temporary(vm_paddr_t pa, int i);
vm_paddr_t pmap_kextract(vm_offset_t);
void	pmap_kremove(vm_offset_t);
void	*pmap_mapbios(vm_paddr_t, vm_size_t);
void	*pmap_mapdev(vm_paddr_t, vm_size_t);
void	*pmap_mapdev_attr(vm_paddr_t, vm_size_t, int);
boolean_t pmap_page_is_mapped(vm_page_t m);
void	pmap_page_set_memattr(vm_page_t m, vm_memattr_t ma);
void	pmap_unmapdev(vm_offset_t, vm_size_t);
void	pmap_invalidate_page(pmap_t, vm_offset_t);
void	pmap_invalidate_range(pmap_t, vm_offset_t, vm_offset_t);
void	pmap_invalidate_all(pmap_t);
void	pmap_invalidate_cache(void);
void	pmap_invalidate_cache_pages(vm_page_t *pages, int count);
void	pmap_invalidate_cache_range(vm_offset_t sva, vm_offset_t eva);

#endif /* _KERNEL */

#endif /* !LOCORE */

#endif /* !_MACHINE_PMAP_H_ */

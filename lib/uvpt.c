/* User virtual page table helpers */

#include <inc/lib.h>
#include <inc/mmu.h>

extern volatile pte_t uvpt[];     /* VA of "virtual page table" */
extern volatile pde_t uvpd[];     /* VA of current page directory */
extern volatile pdpe_t uvpdp[];   /* VA of current page directory pointer */
extern volatile pml4e_t uvpml4[]; /* VA of current page map level 4 */

pte_t
get_uvpt_entry(void *va) {
    if (!(uvpml4[VPML4(va)] & PTE_P)) return uvpml4[VPML4(va)];
    if (!(uvpdp[VPDP(va)] & PTE_P) || (uvpdp[VPDP(va)] & PTE_PS)) return uvpdp[VPDP(va)];
    if (!(uvpd[VPD(va)] & PTE_P) || (uvpd[VPD(va)] & PTE_PS)) return uvpd[VPD(va)];
    return uvpt[VPT(va)];
}

uintptr_t
get_phys_addr(void *va) {
    if (!(uvpml4[VPML4(va)] & PTE_P))
        return -1;
    if (!(uvpdp[VPDP(va)] & PTE_P))
        return -1;
    if (uvpdp[VPDP(va)] & PTE_PS)
        return PTE_ADDR(uvpdp[VPDP(va)]) + ((uintptr_t)va & ((1ULL << PDP_SHIFT) - 1));
    if (!(uvpd[VPD(va)] & PTE_P))
        return -1;
    if ((uvpd[VPD(va)] & PTE_PS))
        return PTE_ADDR(uvpd[VPD(va)]) + ((uintptr_t)va & ((1ULL << PD_SHIFT) - 1));
    if (!(uvpt[VPT(va)] & PTE_P))
        return -1;
    return PTE_ADDR(uvpt[VPT(va)]) + PAGE_OFFSET(va);
}

int
get_prot(void *va) {
    pte_t pte = get_uvpt_entry(va);
    int prot = pte & PTE_AVAIL & ~PTE_SHARE;
    if (pte & PTE_P) prot |= PROT_R;
    if (pte & PTE_W) prot |= PROT_W;
    if (!(pte & PTE_NX)) prot |= PROT_X;
    if (pte & PTE_SHARE) prot |= PROT_SHARE;
    return prot;
}

bool
is_page_dirty(void *va) {
    pte_t pte = get_uvpt_entry(va);
    return pte & PTE_D;
}

bool
is_page_present(void *va) {
    return get_uvpt_entry(va) & PTE_P;
}

int
foreach_shared_region(int (*fun)(void *start, void *end, void *arg), void *arg) {
    /* Calls fun() for every shared region.
     * NOTE: Skip over larger pages/page directories for efficiency */
    // LAB 11: Your code here:

    int res = 0;
    // (void)fun, (void)arg;

    // Traversal through virtual addresses
    // for (uintptr_t addr = 0; addr < MAX_USER_ADDRESS; addr += PAGE_SIZE) {
    //     if
    //     (
    //         !(uvpml4[VPML4(addr)] & PTE_P) || 
    //         !(uvpdp[VPDP(addr)] & PTE_P) || 
    //         !(uvpd[VPD(addr)] & PTE_P)
    //     ) {
    //         continue;
    //     }
    //     if
    //     (
    //         (uvpt[VPT(addr)] & PTE_P) &&
    //         (uvpt[VPT(addr)] & PTE_SHARE)
    //     ) {
    //         res = fun((void*) addr, (void *) (addr + PAGE_SIZE), arg);
    //     }
    //     if (res != 0) {
    //         return res;
    //     }
    // }

    // Traversal using index tables
    for (size_t i4 = 0; i4 < PML4_ENTRY_COUNT; ++i4) {
        if (!(uvpml4[i4] & PTE_P)) {
            continue;
        }

        for (size_t i3 = 0; i3 < PDP_ENTRY_COUNT; ++i3) {
            const size_t idx_pdp =
                (i4 << PDP_ENTRY_SHIFT) |
                i3;
            const pte_t pdpe = uvpdp[idx_pdp];
            if (!(pdpe & PTE_P)) {
                continue;
            }

            // skip 1GB huge page
            if (pdpe & PTE_PS) {
                continue;
            }

            for (size_t i2 = 0; i2 < PD_ENTRY_COUNT; ++i2) {
                const size_t idx_pd =
                    (i4 << (PDP_ENTRY_SHIFT + PD_ENTRY_SHIFT)) |
                    (i3 << PD_ENTRY_SHIFT) |
                    i2;
                const pte_t pde = uvpd[idx_pd];
                if (!(pde & PTE_P)) {
                    continue;
                }

                // skip 2MB huge page
                if (pde & PTE_PS) {
                    continue;
                }

                for (size_t i1 = 0; i1 < PT_ENTRY_COUNT; ++i1) {
                    const size_t idx_pt =
                        (i4 << (PDP_ENTRY_SHIFT + PD_ENTRY_SHIFT + PT_ENTRY_SHIFT)) |
                        (i3 << (PD_ENTRY_SHIFT + PT_ENTRY_SHIFT)) |
                        (i2 << PT_ENTRY_SHIFT) |
                        i1;
                    const pte_t pte = uvpt[idx_pt];

                    if (!(pte & PTE_P) || !(pte & PTE_SHARE)) {
                        continue;
                    }

                    uintptr_t addr =
                        ((uintptr_t) i4 << PML4_SHIFT) |
                        ((uintptr_t) i3 << PDP_SHIFT)  |
                        ((uintptr_t) i2 << PD_SHIFT)   |
                        ((uintptr_t) i1 << PT_SHIFT);

                    // from the condition in the original loop
                    if (addr >= MAX_USER_ADDRESS) {
                        continue;
                    }

                    const int res = fun((void*) addr, (void*) (addr + PAGE_SIZE), arg);
                    if (res != 0) {
                        return res;
                    }
                }
            }
        }
    }

    return res;
}


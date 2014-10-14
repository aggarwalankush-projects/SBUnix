// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t error = utf->utf_err;
	int r;
	envid_t envid = sys_getenvid();
	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	if(!(error & FEC_WR))
		panic("\npgfault: no write access at addr = 0x%x, rip = %x!\n", addr, utf->utf_rip);
	if(!(uvpt[VPN(addr)] & PTE_COW))
		panic("\npgfault: COW bit not set at addr: 0x%0x!\n", addr);

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.

	// LAB 4: Your code here.
	if(uvpt[VPN((uint64_t)addr)] & PTE_COW){
		if ((r = sys_page_alloc(0, PFTEMP, PTE_P|PTE_U|PTE_W)) < 0)
			panic("\npgfault: Panic at sys_page_alloc: %e\n", r);

		memmove(PFTEMP, (void *)(ROUNDDOWN(addr, PGSIZE)), PGSIZE);
	}
	if((r = sys_page_map(0, (void *)PFTEMP, envid,(void *)ROUNDDOWN(addr, PGSIZE), PTE_P|PTE_U|PTE_W))<0)
		panic("\npgfault: Panic at sys_page_map: %e\n", r);
	//unmap temp page
	if ((r = sys_page_unmap(envid,(void *)PFTEMP)) < 0)
		panic("\npgfault: Panic at sys_page_unmap for PFTEMP: %e\n", r);
	return;

//	panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;
	
	// LAB 4: Your code here.
	uint64_t va = pn * PGSIZE;
        envid_t envid_parent = sys_getenvid();


        if(((uvpt[pn] & PTE_W)) || ((uvpt[pn] & PTE_COW))) {
                if(sys_page_map(envid_parent,(void *) va, envid, (void *)va, PTE_COW | PTE_P | PTE_U) < 0)
                        panic(" duppage: Sys page Map Failed for child\n");
                if(sys_page_map(envid_parent,(void *)va,envid_parent, (void *)va, PTE_COW | PTE_P | PTE_U) < 0)
                        panic(" duppage :Syspage Page Map Failed for parent\n");

        }
        else
        {
                if(sys_page_map(envid_parent, (void *)va, envid,(void *) va,  PGOFF(uvpt[pn])) < 0)
                        panic(" duppage : Page Map Failed as it is not writable or Copy on write\n");
        }
        return 0;
//      panic("duppage not implemented");
//      return 0;

}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
        int ret =0;
        envid_t env_id ;
extern unsigned char end[];
        envid_t pid = sys_getenvid();

        set_pgfault_handler(pgfault);
        env_id =  sys_exofork();

        if(env_id < 0 )
                panic("fork: Fork Failed\n");
        if(env_id == 0) {

                thisenv = &envs[ENVX(sys_getenvid())];
                return 0;
        }
        /*copy address space */
        uintptr_t addr;
        for (addr = (uintptr_t)UTEXT; addr < (uintptr_t)end; addr += PGSIZE)
        {
                if( (uvpml4e[VPML4E(addr)] & PTE_P) && (uvpde[VPDPE(addr)] & PTE_P) && (uvpd[VPD(addr)] & PTE_P) && (uvpt[VPN(addr)]&PTE_P) )
                {

                        duppage(env_id, VPN(addr));
                }
        }
        duppage(env_id, VPN(USTACKTOP-PGSIZE));
	if( (ret = sys_page_alloc(env_id,(void *) UXSTACKTOP-PGSIZE, PTE_U | PTE_P | PTE_W))<0)
                panic(" fork: child page allocation fails in  exception stack\n");
        ret = sys_env_set_pgfault_upcall(env_id, thisenv->env_pgfault_upcall);

        /*USTACKTOP*/
        if((ret = sys_env_set_status(env_id,ENV_RUNNABLE))<0)
           panic(" fork: Unable to make child runnable\n");
        return env_id ;

	//panic("fork not implemented");
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}

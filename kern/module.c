#include <inc/stab.h>

#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/elf.h>
#include <kern/pmap.h>
#include <kern/env.h>
#include <kern/kernsym.h>
#include <kern/module.h>

#define MODTEMP 0x8005400000/*modules are loaded in here. 10 pages*/

//module list
struct Module *modules = NULL;
//current ELF variables
struct Secthdr *sectHdr;
struct Symhdr *symtab;
char *str_tab, *sectHdr_strtab;
int num_sym, num_sectHdr;
struct Elf *elf;

//initialization from kern/init
void module_init(void) {

	int c;
	if (modules == NULL) {
		panic("modules have not been initialized yet\n");
	}
	for (c = 0; c < NLKM - 1; c++) {
		modules[c].init = 0;
		modules[c].clean = 0;
		modules[c].startAddr = (uint64_t *) (MODTEMP + c * (uint64_t) PGSIZE);
		modules[c].isInMemory = false;
	}
}

int list_module() {
	int find = false;
	int c;
	cprintf("..List of loaded modules..\n");
	for (c = 0; c < NLKM - 1; c++) {
		if (modules[c].isInMemory) {
			find = true;
			cprintf(
					"Module : %s(version = %s) is loaded at kernel load Address : %x\n",
					modules[c].name, modules[c].version, modules[c].startAddr);
		}

	}
	if (!find) {
		cprintf("..No modules are currently installed.. \n");
	}
	return 0;
}

int load_module(char *buffer, char *path) {

	struct PageInfo *page = NULL;
	int perm = PTE_P | PTE_W;
	elf = (struct Elf *) buffer;
	void (*ptr_init_module)();
	int err, c;
	int upgrade = -1;

	//TODO : Sanity check.
	if (elf->e_magic == ELF_MAGIC) {
		cprintf("\n");
		cprintf("...Loading Module %s\n", path);
	} else {
		cprintf("\nUnable to LOAD: Bad ELF\n");
		return -E_BAD_ELF;
	}

	//check if module already loaded
	if (((err = lookup_module(path)) >= 0)) {
		upgrade = err; // this is already loaded old module
		return -E_LKM_FAIL;

	}

	int i;
	struct Secthdr *sym_tab_entry, *str_tab_entry, *sectHdr_strtab_entry;

	//section header
	sectHdr = (struct Secthdr *) ((char *) elf + elf->e_shoff);
	num_sectHdr = elf->e_shnum;

	// string table for the sectHdr.
	sectHdr_strtab_entry = &sectHdr[elf->e_shstrndx];
	sectHdr_strtab = (char *) ((char *) elf + sectHdr_strtab_entry->sh_offset);

	for (i = 0; i < num_sectHdr; i++) {
		if (sectHdr[i].sh_type == ELF_SHT_SYMTAB) {
			// symbol table
			sym_tab_entry = sectHdr + i;
			symtab =
					(struct Symhdr *) ((char *) elf + sym_tab_entry->sh_offset);
			num_sym = sym_tab_entry->sh_size / sizeof(struct Symhdr);

		} else if (sectHdr[i].sh_type == ELF_SHT_STRTAB
				&& (sectHdr + i) != sectHdr_strtab_entry) {
			//string table for symbol
			str_tab_entry = sectHdr + i;
			str_tab = (char *) (str_tab_entry->sh_offset + (char *) elf);
		}
	}

//creating module
	if ((err = put_module()) < 0) {
		cprintf("failed to create modules %e", err);
		return err;
	}

	c = err; // indicate module no
	cprintf("module %d created\n", c);

	load_program(c);
//	cprintf("init : %x, clean %x\n", modules[c].init, modules[c].clean);

//accessing kern symbols
	if (get_symbols() < 0) {
		cprintf("failed to load module. could not resolve references\n");
		remove_module(c);
		return -E_BAD_SYMBOL;
	}

//apply relocation of symbols
	relocate((uint64_t *) modules[c].startAddr, elf);

//look for version and author
	get_version_author(c);
//fixing name
	strncpy(modules[c].name, path, MOD_NAME_LEN);
//check if new module has an upgrade
	if (upgrade >= 0) {
		cprintf("module already loaded..checking version\n");
		if (!strcmp(modules[upgrade].version, modules[c].version)) {
			//clear the module
			cprintf("No upgrade implemented\n... exiting\n");
			remove_module(c);
			return E_LKM_FAIL;
		} else { //an upgrade.
			cprintf("upgrade found\n");
			cprintf("upgrading module %s, from %s to %s\n", modules[c].name,
					modules[upgrade].version, modules[c].version);
			remove_module(upgrade);
		}
	}

	lcr3(boot_cr3);
	((void (*)(void)) (modules[c].init))();
	lcr3(curenv->env_cr3);

	return 0;
}

int load_program(int c) {
	size_t size;
	int i;
	if (!modules[c].isInMemory || modules[c].startAddr == NULL) {
		return -E_LKM_FAIL;
	}
	//copy in the actual location
	uint64_t sect_start_addr = (uint64_t) modules[c].startAddr;
	pte_t *pte_store;
	struct PageInfo *pg = page_lookup(boot_pml4e, (void *) MODTEMP, &pte_store);
	for (i = 0; i < num_sectHdr; i++) {
		if (sectHdr[i].sh_type == ELF_SHT_PROGBITS
				&& sectHdr[i].sh_flags & ELF_SHF_ALLOC) {
			size = sectHdr[i].sh_size;
			char *name = (char *) (sectHdr_strtab + sectHdr[i].sh_name);
			//cprintf("type : %s, size :%d, elf %x\n", name, size, elf);
			uint64_t src = (uint64_t) (sectHdr[i].sh_offset + (char *) elf);
			(sectHdr + i)->sh_addr = sect_start_addr; //dest
			memcpy((void *) sect_start_addr, (void *) src, size);
			sect_start_addr += size;
		}

	}
	return 0;
}

int relocate(uint64_t *startAddr, struct Elf *elf) {
	int i, err, j;
	uint64_t value, *rel_dest;
	struct Symhdr *symbol;
	struct Rela *rela;
	uint64_t sh_type;
	for (i = 0; i < num_sectHdr; i++) {
		uint32_t infosec = sectHdr[i].sh_info;

		if (infosec >= num_sym) {
			continue;
		}
		if (!(sectHdr[infosec].sh_flags & ELF_SHF_ALLOC)) { //if not a memory related section
			continue;
		}

		sh_type = sectHdr[i].sh_type;

		if (sh_type == ELF_SHT_REL) {
			cprintf("this is elf sht rel\n");
		} else if (sh_type == ELF_SHT_RELA) {
			rela = (void *) (sectHdr[i].sh_offset + (char *) elf);
			for (j = 0; j < sectHdr[i].sh_size / (sizeof(struct Rela)); j++) {
				rel_dest = (void *) ((char *) (sectHdr[infosec].sh_addr)
						+ rela[j].r_offset);

				symbol = (void *) (symtab + ELF_R_SYM(rela[j].r_info));
				value = symbol->st_value + rela[j].r_addend;
				//cprintf("relocation destination %x, value %x\n", rel_dest,
				//		value);
				switch (ELF_R_TYPE(rela[j].r_info)) {
				case R_X86_64_64:
					*(uint64_t *) rel_dest = value;
					break;
				case R_X86_64_32:
					*(uint32_t *) rel_dest = value;
					break;
				case R_X86_64_PC32:
					value -= (uint64_t) rel_dest;
					*(uint32_t *) rel_dest = value;
					break;
				}
			}
		}

	}

	return 0;
}

/*adds a module to the module list
 * and returns the index to the array*/
int put_module() {
//find a free module.
	int c, err, i;
	for (c = 0; c < NLKM; c++) {
		if (!modules[c].isInMemory) {
			break; //found a free module
		}
	}

	if (c == NLKM)
		return -NO_FREE_MODULE;

	modules[c].init = 0;
	modules[c].clean = 0;
//allocate space in k-memory.
	if ((err = give_module_memory(c)) < 0) {
		return err;
	}
//create a module at module[i]
//look up init and clean
//	cprintf("%d", num_sym);
	for (i = 0; i < num_sym; i++) {
		char *name = (char *) (str_tab + symtab[i].st_name);
//		cprintf("%s\n", name);
		if (!(strcmp(name, "init_module"))) {

			modules[c].init = (uint64_t *) ((char *) (modules[c].startAddr)
					+ symtab[i].st_value);
		} else if (!(strcmp(name, "clean_module"))) {

			modules[c].clean = (uint64_t *) ((char *) (modules[c].startAddr)
					+ symtab[i].st_value);
		}
	}
	if (modules[c].init == 0 || modules[c].clean == 0) {
		return -E_BAD_MODULE;
	}

	modules[c].isInMemory = true; //module in memory
	return c;
}

/*call this after fixing relocation
 * the version and name would have been
 * resolved by then*/
int get_version_author(int c) {
	int i;
	if (c > NLKM) {
		cprintf("failed to fix meta data.. invalid module\n");
		return -E_BAD_MODULE;
	}
	if (!modules[c].isInMemory) {
		cprintf("failed to load module\n");
		return -E_BAD_MODULE;
	}
	for (i = 0; i < num_sym; i++) {
		char *name = (char *) (str_tab + symtab[i].st_name);
		if (!(strcmp(name, "version"))) {
			//cprintf("version found\n");
			strcpy(modules[c].version,
					(char *) (*(uint64_t *) symtab[i].st_value));
			cprintf("version : %s\n", modules[c].version, modules[c].version);
		} else if (!(strcmp(name, "author"))) {
			//cprintf("author found\n");
			strcpy(modules[c].author,
					(char *) (*(uint64_t *) symtab[i].st_value));
			cprintf("author : %s\n", modules[c].author);
		}
	}

	return 0;
}

/*loads actual physical region.Returns 0 on success, <0 on error*/
int give_module_memory(int c) {
	int i;
	struct PageInfo *page;
	int perm = PTE_P | PTE_W;
	if (c > NLKM) {
		cprintf("invalid module number");
		return -E_LKM_FAIL;
	}

//find the size of the loadable modules
	size_t size = 0;
	for (i = 0; i < num_sectHdr; i++) {
		if (sectHdr[i].sh_type == ELF_SHT_PROGBITS
				&& sectHdr[i].sh_flags & ELF_SHF_ALLOC) {
			size += sectHdr[i].sh_size;
		}

	}
// we dont support modules bigger than PGSIZE
	if (size > (uint64_t) PGSIZE) {
		cprintf("too big module\n");
		return -E_LKM_FAIL;
	}

//now allocate memory
	uint64_t *startAddr = (uint64_t *) modules[c].startAddr;

//allocate in kernel space
	page = page_alloc(ALLOC_ZERO);
	if (!page) {
		panic("no page");
		return -E_NO_MEM;
	}
//into the kernel.
	if ((page_insert(boot_pml4e, page, startAddr, perm))
			!= 0) {
		page_free(page);
		return -E_NO_MEM;
	}
//into current user memory
	if ((page_insert(curenv->env_pml4e, page, startAddr, perm)) != 0) {
		page_free(page);
		return -E_NO_MEM;
	}
	return 0;

}


int get_symbols() {
	int err, i;
	///cprintf("kernel symbol address=%x\n", kernel_table);
	for (i = 1; i < num_sym; i++) {

		if (symtab[i].st_shndx == SHN_UNDEF) {
			char *name = (char *) (str_tab + symtab[i].st_name);
			//fetch kernel value
			
			uint64_t sym_value;

			if ((sym_value = get_kern_sym(kernel_table, name)) < 0) {
				cprintf("could not find the symbol\n");
				return -E_BAD_SYMBOL;
			}
			(symtab + i)->st_value = (uint64_t) sym_value;
		//	cprintf("External symbol %s found, sym rel addr=%llx \n", name,
			//		sym_value);

		} else { //local variables
			uint64_t infosec = symtab[i].st_shndx;
			(symtab + i)->st_value += sectHdr[infosec].sh_addr;
		}
	}

	return 0;

}

int lookup_module(char *mname) {
	int c;
	for (c = 0; c < NLKM; c++) {
		if (modules[c].isInMemory && !strcmp(mname, modules[c].name)) {
			return c;
		}
	}
	return -E_BAD_MODULE;
}

int remove_module(int c) {
//disallocate the page
	uint64_t *startAddr = (uint64_t *) modules[c].startAddr;
	page_remove(boot_pml4e, startAddr);
	page_remove(curenv->env_pml4e, startAddr);

//remove entry from the module list
	modules[c].init = 0;
	modules[c].clean = 0;
	memset(modules[c].version, 0, MOD_NAME_LEN);
	memset(modules[c].name, 0, MOD_NAME_LEN);
	memset(modules[c].author, 0, MOD_NAME_LEN);
	modules[c].startAddr = (uint64_t *) (MODTEMP + c * (uint64_t) PGSIZE);
	modules[c].isInMemory = false; //initially everything is free
	return 0;
}

int unload_module(char *mname) {
	int err, c;
	cprintf("----------------------------\n");
	pte_t *pte_store;
	struct PageInfo *page = page_lookup(boot_pml4e, (void *) MODTEMP,
			&pte_store);
//lookup the module
	if ((err = lookup_module(mname)) < 0) {
		cprintf("Module is not loaded\n");
		return err;
	}
//else its module
	c = err;

	lcr3(boot_cr3);
	((void (*)(void)) (modules[c].clean))();
	lcr3(curenv->env_cr3);
	remove_module(c);
	return 0;

}


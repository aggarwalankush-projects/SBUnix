#include <inc/lib.h>	
#include <inc/elf.h>
#include <inc/types.h>

int read_file_into_memory(char * filename);

uint64_t file_size;

void umain(int argc, char **argv) {
	uint64_t r;

	r = read_file_into_memory("kernel.sym");
	if (r < 0) {
		cprintf("Could not open file\n");
		return;
	}

	if ((r = sys_load_kernel_symbol(UTEMP, file_size)) < 0) {
		panic("loading kernel symbol failed: error = %d\n", r);
	}

}

int read_file_into_memory(char * filename) {
	//open the given file.
	int r = 0;
	struct Stat filestat;
	
	int fdnum;
	int npages;
	int bytes_to_read = 0, i = 0;
	void * addr;

	fdnum = open(filename, O_RDONLY);

	if (fdnum < 0)
		return fdnum;

	r = fstat(fdnum, &filestat);

	if (r < 0)
		return r;

	file_size = filestat.st_size;

	npages = file_size / PGSIZE + 1;

	//	cprintf("file size %d pages needed %d\n", file_size, npages);

	//load the file at UTEMP
	//i = 0;
	addr = (uint8_t *) UTEMP;
	bytes_to_read = file_size;
	while (bytes_to_read > 0) {
	//	cprintf("Reading page %d\n", i++);

		//Allocate a page . Map it at addr.
		if ((r = sys_page_alloc(0, (void*) addr, PTE_P | PTE_U | PTE_W)) < 0)
			return r;

		//read the contents from file , one page at a time .
		if ((r = read(fdnum, addr, MIN(PGSIZE,bytes_to_read ))) < 0)
			return r;

		bytes_to_read -= r;
		addr += r;
	}

//	cprintf("File read done\n");
	return (uint64_t) UTEMP;
}


#include<inc/lib.h>
#include<inc/elf.h>

#define MAX_PAGES 20

void insmod(char *path){
	int f, n, fd;
	struct Elf *elf;
	int r = 0;
	int i = 0;

	f = open(path, O_RDONLY);
	if(f < 0){
		cprintf("can't open file : %s\n ", path);
	}
	else{
		for(i=0; ; i++){
			if(i == MAX_PAGES){
				cprintf("%s: elf file too large to be read\n",path);
				return ;
			}
			if ((r = sys_page_alloc(thisenv->env_id, UTEMP + i * PGSIZE, PTE_P|PTE_U|PTE_W)) < 0)
				panic("sys_page_alloc: %e", r);

			seek(f, i * PGSIZE);
			if ((n = readn(f, UTEMP + i * PGSIZE, PGSIZE )) < PGSIZE ){
				if(n < 0){
					cprintf("failed to read elf file: not an elf file\n");
					return;
				}
				else
					break;
			}

		}
		sys_load_module(UTEMP, path);
	}

}


void umain(int argc, char **argv){
	binaryname = "insmod";
	int f;

	if (argc < 2){
		cprintf("Usage: insmod module_name.o\n");
	}else {
		insmod(argv[1]);
	}

}

#include<inc/lib.h>
#include<inc/elf.h>

void rmmod(char *path){
	sys_unload_module(path);
}

void umain(int argc, char **argv){
	binaryname = "rmmod";

	if (argc < 2){
		cprintf("USAGE: rmmod module_name.o\n");
	}else {
		rmmod(argv[1]);
	}

}

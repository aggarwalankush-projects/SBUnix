#include<inc/lib.h>
#include<inc/elf.h>

void lsmod() {
	sys_list_module();
}

void umain(int argc, char **argv) {
	binaryname = "lsmod";
	if (argc >= 2) {
		cprintf("USAGE: lsmod\n");
	} else {
		lsmod();
	}
}

#include<inc/module.h>

MODULE_VERSION("1.0")
MODULE_AUTHOR("m.paul")

int
init_module(){
	cprintf("in testsyscall init module\n");
	sys_hello_me(); // this is the newly added syscall
	return 0;
}

int
clean_module(){
	cprintf("in testsyscall clean module\n");
	return 0;
}
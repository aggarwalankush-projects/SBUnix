#include<inc/module.h>

MODULE_VERSION("2.0")
MODULE_AUTHOR("mpaul")

int
init_module(){
	cprintf("hello word. I am a hello module version2\n");
	return 0;
}

int
clean_module(){
	cprintf("bye world. I am a dead second module version2\n");
	return 0;
}


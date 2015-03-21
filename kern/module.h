#ifndef JOS_KERN_LKM_H
#define JOS_KERN_LKM_H
#define MOD_NAME_LEN 32
#define MAX_MODULES	10
#define MODULE_DATA_LOAD_START 0x8005400000

#define NLKM 10 /*support only 10 LKMs*/

#include <inc/elf.h>

struct Module{
	uint64_t *init;
	uint64_t *clean;
	uint64_t *startAddr;
	char name[MOD_NAME_LEN];
	char version[MOD_NAME_LEN];
	char author[MOD_NAME_LEN];
	struct Module *link;
	int isInMemory;
	//TODO state
};

void module_init(void);
int load_module(char *buffer, char *path);
int unload_module(char *path);
int get_symbols();
int put_module();
int relocate(uint64_t *startAddr,struct Elf *elf);
int give_module_memory(int i);
int load_program(int c);
int get_version_author(int c);
int list_module();
int lookup_module(char *mname);
int remove_module(int c);
struct Module *modules;

#endif

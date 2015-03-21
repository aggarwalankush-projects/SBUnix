#ifndef JOS_KERN_SYMBOL_TABLE_H
#define JOS_KERN_SYMBOL_TABLE_H

#define SYMBOL_LENGTH 64
#define MAX_KERNEL_SYMBOLS   1024

struct kernel_symbol 
{
    char name[SYMBOL_LENGTH];
    uint64_t addr;
    uint64_t hash;
};

extern struct kernel_symbol kernel_table[MAX_KERNEL_SYMBOLS];

extern uint64_t kern_ksym;
void set_kern_sym(struct kernel_symbol *table, char *symbol_name, uint64_t symbol_value);
uint64_t get_kern_sym(struct kernel_symbol *table, char *name);

#endif // !JOS_KERN_SYMBOL_TABLE_H

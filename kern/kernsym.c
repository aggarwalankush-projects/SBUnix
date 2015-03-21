#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include<kern/kernsym.h>

static unsigned kernel_symbol_index = 0;
struct kernel_symbol kernel_table[MAX_KERNEL_SYMBOLS];

uint64_t kern_ksym =0;

uint64_t hash_fn(char *symbol_name)
{
    unsigned long long hash = 0;
    int c;
    while ((c = *symbol_name++))
        hash = c + (hash << 6) + (hash << 16) - hash;
    return hash;
}
void set_kern_sym(struct kernel_symbol *table, char *symbol_name, uint64_t symbol_value)
{

    uint64_t hash_value = hash_fn(symbol_name);
    
    if ( kernel_symbol_index < MAX_KERNEL_SYMBOLS)
    {
        table[kernel_symbol_index].addr = symbol_value;
        strncpy(table[kernel_symbol_index].name, symbol_name, 
                sizeof(table[kernel_symbol_index].name));
        table[kernel_symbol_index].hash = hash_value;
    }
    else
    {
        panic("set_kern_sym: NO more space to add more kernel symbols\n");
    }
    kernel_symbol_index++;
}
uint64_t get_kern_sym(struct kernel_symbol *table, char *name)
{
    uint64_t hash_value = hash_fn(name);
    unsigned int i =0;
    
    while ( i < kernel_symbol_index )
    {
        if ( table[i].hash == hash_value && (strcmp(table[i].name, name) == 0))
        {
            return table[i].addr;
        }
        i++;
    }
    return (uint64_t)~0;
}


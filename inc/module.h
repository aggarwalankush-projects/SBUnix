#ifndef JOS_INC_MODULE_H
#define JOS_INC_MODULE_H

int	cprintf(const char *fmt, ...);

// this shud be exposed by the newsyscall apis.
int sys_hello_me();

#define MODULE_VERSION(v) char *version = v;
#define MODULE_AUTHOR(n) char *author = n;

#endif /*JOS_INC_MODULE_H*/

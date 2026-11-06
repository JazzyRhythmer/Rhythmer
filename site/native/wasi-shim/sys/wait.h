/* Only hostsim's unused fork-based command-line harness needs this header.
   Those functions are discarded by wasm-ld. The browser data engine never forks. */
#include <sys/types.h>
#ifndef __pid_t_defined
typedef int pid_t;
#endif
int waitpid(pid_t,int*,int);
int fork(void);
#define WIFEXITED(s) (((s)&127)==0)
#define WEXITSTATUS(s) (((s)>>8)&255)

#ifndef COSMOPOLITAN_LIBC_RUNTIME_MAPS_H_
#define COSMOPOLITAN_LIBC_RUNTIME_MAPS_H_
#include "libc/intrin/atomic.h"
#include "libc/intrin/dll.h"
#include "libc/thread/tls2.internal.h"
COSMOPOLITAN_C_START_

#define MAP_CONTAINER(e) DLL_CONTAINER(struct Map, elem, e)

struct Map {
  char *addr;        /* granule aligned */
  size_t size;       /* must be nonzero */
  struct Dll elem;   /* for __maps.free */
  int64_t off;       /* -1 if anonymous */
  int prot;          /* memory protects */
  int flags;         /* memory map flag */
  bool iscow;        /* windows nt only */
  bool readonlyfile; /* windows nt only */
  unsigned visited;  /* used for checks */
  intptr_t hand;     /* windows nt only */
};

struct Maps {
  unsigned mono;
  atomic_int lock;
  struct Dll *free;
  struct Dll *used;
  size_t count;
  size_t pages;
  struct Map stack;
  struct Map guard;
  bool once;
  atomic_ulong rollo;
};

struct AddrSize {
  char *addr;
  size_t size;
};

extern struct Maps __maps;

void __maps_init(void);
bool __maps_lock(void);
void __maps_check(void);
void __maps_unlock(void);
void __maps_add(struct Map *);
struct Map *__maps_alloc(void);
void __maps_free(struct Map *);
int __munmap(char *, size_t, bool);
void *__mmap(char *, size_t, int, int, int, int64_t);
void __maps_stack(char *, int, int, size_t, int, intptr_t);
struct AddrSize __get_main_stack(void);

COSMOPOLITAN_C_END_
#endif /* COSMOPOLITAN_LIBC_RUNTIME_MAPS_H_ */

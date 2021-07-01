#include "heap.h"
#include "assert.h"
#include <malloc.h>
#include <memory.h>

void *halloc(const size_t size) {
  assert(size);
  return malloc(size);
}

void *halloc_clear(const size_t size) {
  assert(size);
  void *mem = halloc(size);
  memset(mem, 0, size);
  return mem;
}

void hfree(void *mem) {
  assert(mem);
  free(mem);
}

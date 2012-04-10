#ifndef SHELF_H_
#define SHELF_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  const uint8_t *bytes;
  uint64_t len;
} SHELF_SLICE;


typedef struct shelf SHELF;

SHELF *shelf_open(const char* path, bool create);
void shelf_close(SHELF *store);




#endif

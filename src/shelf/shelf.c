
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include "shelf.h"

typedef struct shelf {
  const char* path;
  // current file
  // current offset
} shelf;

SHELF *shelf_open(const char* path, bool create){
  // try to open the directory pointed to by path.
  // if it doesn't exist and create is true, create the dir.
  // error: SSTORE = null
  struct stat sb;

  if (stat(path, &sb) != 0) {
    // doesn't exist. create it.
    if (!create){
      return NULL;
    }
    if(mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0){
      return NULL;
    }
  }else if(!S_ISDIR(sb.st_mode)){
    return NULL;
  }
  // ok, directory exists.
  SHELF *s = malloc(sizeof(*s));
  if (s != NULL){
    s->path = path;
  }
  return s;
}

bool shelf_set(SHELF_SLICE* key, SHELF_SLICE* value) {
  return false;
}

void shelf_close(SHELF *store){
  free(store);
}



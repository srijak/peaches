file(GLOB_RECURSE SHELF_SOURCES ./ *.c *.h)

add_library(shelf ${SHELF_SOURCES})


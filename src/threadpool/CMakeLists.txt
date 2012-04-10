file(GLOB_RECURSE LIB_SOURCES ./ *.c *.h)

add_library(threadpool ${LIB_SOURCES})


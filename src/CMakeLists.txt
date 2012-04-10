add_subdirectory(shelf)
add_subdirectory(threadpool)

#--- executable
file(GLOB SRC_SOURCES ./ *.c *.h)
add_executable(peaches ${SRC_SOURCES})

target_link_libraries(peaches shelf threadpool)


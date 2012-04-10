include_directories(../src)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/../src)
#set(SHELF_OBJ ${PROJECT_BINARY_DIR}/src/shelf)
#link_directories(${SHELF_OBJ})

#--- Libs
find_package(PkgConfig)

pkg_check_modules(CHECK REQUIRED check)
include_directories(${CHECK_INCLUDE_DIRS})
link_directories(${CHECK_LIBRARY_DIRS})

#--- add tests. Anything that starts with test_ is a test.
file(GLOB TESTS "test_*.c")
foreach(T_FILE ${TESTS})
  get_filename_component(T_NAME ${T_FILE} NAME)
  add_executable(${T_NAME} ${T_FILE})
  target_link_libraries(${T_NAME} ${CHECK_LIBRARIES} shelf threadpool)
  add_test(NAME ${T_NAME} COMMAND ${T_NAME})
endforeach(T_FILE ${TESTS})


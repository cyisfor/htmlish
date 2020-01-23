cmake_minimum_required(VERSION 3.0)
project(htmlish VERSION 0.1)

add_executable(parse
	src/main.c)

add_test(NAME parse
	COMMAND "${CMAKE_CURRENT_BINARY_DIR}/test_parse"
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
add_executable(test_parse
	src/test_parse.c)
enable_testing()

target_link_libraries(parse PRIVATE htmlish)

add_library(htmlish
	src/htmlish.h src/input.h src/parse_chat.h
	src/htmlish.c
	src/parse_chat.c
	src/input.c
	src/error.c)

set(includes
	"${CMAKE_CURRENT_SOURCE_DIR}"
	"${CMAKE_CURRENT_BINARY_DIR}/module_bin"
	"${CMAKE_CURRENT_BINARY_DIR}/modules"
	"${CMAKE_CURRENT_BINARY_DIR}/module_bin/libxmlfixes/libxml2/include"
	"${CMAKE_CURRENT_BINARY_DIR}/modules/libxmlfixes/libxml2/include")
target_include_directories(htmlish PUBLIC
	${includes})

target_include_directories(test_parse PUBLIC
	${includes})
target_link_libraries(htmlish PUBLIC xmlfixes)
target_link_libraries(test_parse PUBLIC htmlish)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/add_module")
include(AddModule)

add_module(libxmlfixes
  GIT 5ea906a419464d246dba4acf026bcd029894b6e1
  /home/repo/libxmlfixes
  git@github.com:cyisfor/libxmlfixes.git
  https://github.com/cyisfor/libxmlfixes.git)


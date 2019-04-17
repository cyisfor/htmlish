cmake_minimum_required(VERSION 3.0)
project(htmlish VERSION 0.1)

add_executable(parse
	src/main.c)

add_test(parse test_parse)
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

target_include_directories(htmlish PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}"
	"${CMAKE_CURRENT_BINARY_DIR}"
	"libxmlfixes/libxml2/include"
	"${CMAKE_CURRENT_BINARY_DIR}/libxmlfixes/libxml2/include")

target_include_directories(test_parse PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}"
	"${CMAKE_CURRENT_BINARY_DIR}"
	"libxmlfixes/libxml2/include"
	"${CMAKE_CURRENT_BINARY_DIR}/libxmlfixes/libxml2/include")
target_link_libraries(htmlish PUBLIC xmlfixes)
target_link_libraries(test_parse PUBLIC htmlish)
add_subdirectory(libxmlfixes)


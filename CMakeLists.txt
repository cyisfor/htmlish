cmake_minimum_required(VERSION 3.0)
project(htmlish VERSION 0.1)

add_executable(parse
	src/main.c)

target_link_libraries(parse PRIVATE htmlish)

add_library(htmlish
	src/htmlish.h src/input.h src/parse_chat.h
	src/htmlish.c
	src/parse_chat.c
	src/input.c
	src/error.c)

target_include_directories(htmlish PUBLIC
	"libxmlfixes/src"
	"${CMAKE_CURRENT_BINARY_DIR}/libxmlfixes/src"
	"libxmlfixes/libxml2/include"
	"${CMAKE_CURRENT_BINARY_DIR}/libxmlfixes/libxml2/include")

add_subdirectory(libxmlfixes)

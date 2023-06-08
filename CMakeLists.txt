cmake_minimum_required(VERSION 3.18)
project(ex4 VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)

add_library(VirtualMemory
  			pdebug.cpp
  			bits.cpp
			VirtualMemory.cpp
			PhysicalMemory.cpp)

set_property(TARGET VirtualMemory PROPERTY CXX_STANDARD 11)
target_compile_options(VirtualMemory PUBLIC -Wall -Wextra -pedantic)


add_executable(ex4
			SimpleTest.cpp)

set_property(TARGET ex4 PROPERTY CXX_STANDARD 11)
target_compile_options(ex4 PUBLIC -Wall -Wextra -Werror -pedantic)
target_include_directories(ex4 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ex4 PUBLIC VirtualMemory)

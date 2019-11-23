cmake_minimum_required(VERSION 3.15)
project(elf2rel)

set(CMAKE_CXX_STANDARD 17)

include_directories(elfio)
include_directories(clipp)
include_directories(.)

add_executable(elf2rel
    elf2rel.cpp)
    
target_link_libraries(elf2rel)


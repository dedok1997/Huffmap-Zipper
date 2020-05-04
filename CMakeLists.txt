cmake_minimum_required (VERSION 3.8)

project(huffman)

set(CMAKE_CXX_STANDARD 11)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX") # CMake already contains W3 in its flags
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")
endif()


add_executable(huffman main.cpp huffman.cpp)


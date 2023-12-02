cmake_minimum_required(VERSION 3.16.0)
#set(CMAKE_C_COMPILER /usr/bin/arm-linux-gnueabihf-gcc)
#set(CMAKE_CXX_COMPILER /usr/bin/arm-linux-gnueabihf-g++)
add_compile_options(-g)
project(MCPIRelay)

set (CMAKE_CXX_STANDARD 11)
    
add_subdirectory(RakNet-MCPI)
add_subdirectory(MCPIRelay)

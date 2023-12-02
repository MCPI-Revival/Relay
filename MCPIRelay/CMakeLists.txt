project(mcpi-relay)

file(GLOB_RECURSE SOURCE_FILES *.cpp)

include_directories(.)
include_directories(${RAKNET_INCLUDE_DIRS})
#include_directories(./RakNet-MCPI/Source)

add_executable(mcpi-relay ${SOURCE_FILES})
target_link_libraries(mcpi-relay RakNetLibStatic)

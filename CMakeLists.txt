cmake_minimum_required(VERSION 3.12)
project(nbody LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_COMPILER /usr/bin/g++)

set(SFML_ROOT "$ENV{HOME}/SFML")
include_directories(${SFML_ROOT}/include include)
link_directories(${SFML_ROOT}/lib64)

# add your sources here:
add_executable(nbody 
         NBody.cpp
         src/Body.cpp
         src/utils.cpp
)

target_link_libraries(nbody
    OpenCL
    sfml-graphics
    sfml-window
    sfml-system
)

set_target_properties(nbody PROPERTIES
    BUILD_RPATH ${SFML_ROOT}/lib64
)
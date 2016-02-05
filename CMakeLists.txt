cmake_minimum_required(VERSION 2.8)
project( box-label )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall -O2")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --std=c99 -Wall -O2")

set(OpenCV_DIR /usr/local/share/OpenCV/)
find_package( OpenCV REQUIRED )

add_executable( box-label box-label.cpp )
target_link_libraries( box-label ${OpenCV_LIBS} )

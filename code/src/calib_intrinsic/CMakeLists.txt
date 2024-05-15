cmake_minimum_required(VERSION 3.5)
project(calib_intrinsic C CXX)
find_package(OpenCV REQUIRED)

include_directories("../../include")
include_directories(${OpenCV_INCLUDE_DIRS})
link_directories(${OpenCV_LIBRARY_DIRS})

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})

add_definitions(-DNUM_CAMERA=6)
add_definitions(-DNUM_IMAGES=20)
add_definitions(-DCHESSBOARD_WIDTH=11)
add_definitions(-DCHESSBOARD_HEIGHT=9)
add_definitions(-DDATAPATH="../../../../data/")
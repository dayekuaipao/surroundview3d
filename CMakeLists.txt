cmake_minimum_required(VERSION 3.16)
project(surround3D)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
include_directories(${OpenCV_INCLUDE_DIRS}) 
include_directories(${PROJECT_SOURCE_DIR}/include)
set(OpenCV_DIR "/usr/local/lib/cmake/opencv4")
find_package(OpenCV REQUIRED)
aux_source_directory(${PROJECT_SOURCE_DIR}/source SRC_FILES)
add_executable(${PROJECT_NAME} ${SRC_FILES})
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS} ${CMAKE_DL_LIBS} pthread glfw X11 Xrandr Xi dl GL)
add_definitions(-D__OMNIDIR__)
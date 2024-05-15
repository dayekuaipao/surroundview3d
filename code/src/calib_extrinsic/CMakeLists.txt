cmake_minimum_required(VERSION 3.5)
project(calib_extrinsic C CXX)
find_package(OpenCV REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(Ceres REQUIRED) 


include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${EIGEN3_INCLUDE_DIRS})
include_directories(${CERES_INCLUDE_DIRS})
include_directories("../../include")
link_directories(${OpenCV_LIBRARY_DIRS})

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS} ${EIGEN3_LIBS} ${CERES_LIBRARIES})
add_definitions(-DNUM_CAMERA=6)
add_definitions(-DNUM_POINT=8)
add_definitions(-DDATAPATH="../../../../data/")
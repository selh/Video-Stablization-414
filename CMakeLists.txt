cmake_minimum_required(VERSION 2.8)
set (CMAKE_CXX_STANDARD 11)
project( DisplayImage )
find_package( OpenCV REQUIRED )
add_compile_options(-std=c++11)
include_directories( ${OpenCV_INCLUDE_DIRS} )

file(GLOB DisplayImage_src
    "*.h"
    "*.cpp"
)

add_executable(DisplayImage ${DisplayImage_src})
target_link_libraries( DisplayImage ${OpenCV_LIBS} )
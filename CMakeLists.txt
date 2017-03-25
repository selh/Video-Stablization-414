cmake_minimum_required(VERSION 2.8)
project( DisplayImage )
find_package( OpenCV REQUIRED )

set(SOURCES SIFT.cpp scale.cpp scale.h )

add_executable( DisplayImage ${SOURCES} )
target_link_libraries( DisplayImage ${OpenCV_LIBS} )
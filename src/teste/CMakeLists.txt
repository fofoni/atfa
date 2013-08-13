cmake_minimum_required( VERSION 2.8 )
project( BlaBle )

execute_process( COMMAND git describe
                 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                 OUTPUT_VARIABLE BlaBle_VERSION
                 OUTPUT_STRIP_TRAILING_WHITESPACE )
add_definitions( "-DBlaBle_VERSION=\"${BlaBle_VERSION}\"" )

find_package( OpenCV REQUIRED )

add_executable( displayblabli teste-opencv.cpp )

target_link_libraries( displayblabli ${OpenCV_LIBS} )

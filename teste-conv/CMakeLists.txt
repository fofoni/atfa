cmake_minimum_required( VERSION 2.8 )
project( TesteConv )

execute_process( COMMAND git describe
                 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                 OUTPUT_VARIABLE BlaBle_VERSION
                 OUTPUT_STRIP_TRAILING_WHITESPACE )
add_definitions( "-DBlaBle_VERSION=\"${BlaBle_VERSION}\"" )

find_package( OpenCV REQUIRED )
find_package( PortAudio REQUIRED )
# TODO: write a working FindLibSndFile.cmake

include_directories(${LIBSNDFILE_INCLUDE_DIR})

set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -O2")

add_executable( teste teste-opencv.cpp )

target_link_libraries( teste ${OpenCV_LIBS} ${PortAudio_LIBS} -lsndfile )

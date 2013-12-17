cmake_minimum_required( VERSION 2.8 )
project( ATFA )

execute_process( COMMAND git describe
                 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                 OUTPUT_VARIABLE ATFA_VERSION
                 OUTPUT_STRIP_TRAILING_WHITESPACE )
add_definitions( "-DATFA_VERSION=\"${ATFA_VERSION}\"" )

find_package( OpenCV REQUIRED )
find_package( PortAudio REQUIRED )
# TODO: write a working FindLibSndFile.cmake

set( CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -O2" )

add_executable( atfa main.cpp )

target_link_libraries( atfa ${OpenCV_LIBS} ${PortAudio_LIBS} -lsndfile )

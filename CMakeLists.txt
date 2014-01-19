cmake_minimum_required( VERSION 2.8 )
project( ATFA )

execute_process( COMMAND git describe
                 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                 OUTPUT_VARIABLE ATFA_VERSION
                 OUTPUT_STRIP_TRAILING_WHITESPACE )
add_definitions( "-DATFA_VERSION=\"${ATFA_VERSION}\"" )
add_definitions( "-DATFA_DIR=\"${PROJECT_SOURCE_DIR}\"" )

find_package( OpenCV REQUIRED )
find_package( PortAudio REQUIRED )
# TODO: write a working FindLibSndFile.cmake

set( USUAL_FLAGS "-Wall -Wextra -pedantic" )
set( DBG_FLAGS "${USUAL_FLAGS} -O0 -g3 -fno-inline -DATFA_DEBUG  " )
set( RLS_FLAGS "${USUAL_FLAGS} -O2                 -DATFA_RELEASE" )
set( CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}   ${DBG_FLAGS}" )
set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${RLS_FLAGS}" )

add_executable( atfa src/main.cpp src/Signal.cpp )

target_link_libraries( atfa ${OpenCV_LIBS} ${PortAudio_LIBS} -lsndfile )

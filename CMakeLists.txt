cmake_minimum_required( VERSION 2.8 )
project( ATFA )

set( LOCAL_MOD_DIR "${PROJECT_SOURCE_DIR}/cmake/Modules/" )
set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${LOCAL_MOD_DIR} )

#find_package( OpenCV REQUIRED )
find_package( PortAudio REQUIRED )
find_package( LibSndFile REQUIRED )

execute_process( COMMAND git describe
                 WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                 OUTPUT_VARIABLE ATFA_VERSION
                 OUTPUT_STRIP_TRAILING_WHITESPACE )
add_definitions( "-DATFA_VERSION=\"${ATFA_VERSION}\"" )
add_definitions( "-DATFA_DIR=\"${PROJECT_SOURCE_DIR}\"" )

set( USUAL_FLAGS "-ansi -Wall -Wextra -pedantic" )
set( DBG_FLAGS "${USUAL_FLAGS} -DATFA_DEBUG   -O0 -g3 -fno-inline" )
set( RLS_FLAGS "${USUAL_FLAGS} -DATFA_RELEASE -O2" )
set( CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}   ${DBG_FLAGS}" )
set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${RLS_FLAGS}" )

add_executable( atfa src/main.cpp src/Signal.cpp )

target_link_libraries( atfa ${OpenCV_LIBS} ${PortAudio_LIBS}
                            ${LIBSNDFILE_LIBRARIES} )

# TODO: write a CPack file for creating .rpms, and etc?

# TODO: find a way to separate documentation and code, so that we can find
#       things easier in code. Consider Doxygen structural commands.

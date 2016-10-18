cmake_minimum_required( VERSION 3.3.2 )
project( ATFA )
set( PROJ "Ambiente de Testes para Filtros Adaptativos" )

if( NOT CMAKE_BUILD_TYPE )
    message(WARNING "No build type set: using \"Release\"")
    set( CMAKE_BUILD_TYPE "Release" )
endif()

# set( CMAKE_VERBOSE_MAKEFILE ON )

message(STATUS "=============================================================")
message(STATUS "${PROJ} -- ${CMAKE_BUILD_TYPE} build")
message(STATUS "")
message(STATUS "CMAKE GENERATOR : ${CMAKE_GENERATOR}")
message(STATUS "SYSTEM NAME     : ${CMAKE_SYSTEM_NAME}")
message(STATUS "=============================================================")

set( LOCAL_MOD_DIR "${PROJECT_SOURCE_DIR}/cmake/Modules/" )
set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${LOCAL_MOD_DIR} )

find_package( PortAudio REQUIRED )
find_package( LibSndFile REQUIRED )
find_package( Qt5Core REQUIRED )
find_package( Qt5Gui REQUIRED )
find_package( Qt5Widgets REQUIRED )

set( USUAL_FLAGS "" )

### ### ### Setup Qt
set( CMAKE_AUTOMOC ON )
set( CMAKE_INCLUDE_CURRENT_DIR ON )

### ### ### Setup PortAudio
if(PortAudio_INCLUDE_DIRS)
    include_directories( "${PortAudio_INCLUDE_DIRS}" )
endif(PortAudio_INCLUDE_DIRS)

### ### ### Setup MATLAB
# http://www.mathworks.com/help/matlab/matlab_external/ ...
# ... mat-file-library-and-include-files.html
set( LINK_MATLAB YES )
set( MATLAB_ROOT "/usr/local/MATLAB/MATLAB_Production_Server/R2015a" )
set( MATLAB_ARCH "glnxa64" )
set( MATLAB_INCLUDE_DIRS "${MATLAB_ROOT}/extern/include" )
set( MATLAB_LINK_DIRS "${MATLAB_ROOT}/bin/${MATLAB_ARCH}" )
if(LINK_MATLAB)
    include_directories( "${MATLAB_INCLUDE_DIRS}" )
    link_directories( "${MATLAB_LINK_DIRS}" )
    set( MATLAB_LIBS "-lmat -lmx" )
else(LINK_MATLAB)
    set( MATLAB_LIBS "" )
endif(LINK_MATLAB)

### ### ### Setup ATFA
execute_process( COMMAND git describe
                 WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                 OUTPUT_VARIABLE ATFA_VERSION
                 OUTPUT_STRIP_TRAILING_WHITESPACE )
add_definitions( "-DATFA_VERSION=\"${ATFA_VERSION}\"" )
add_definitions( "-DATFA_DIR=\"${PROJECT_SOURCE_DIR}\"" )

set( USUAL_FLAGS "${USUAL_FLAGS} -std=c++11 -Wall -Wextra -pedantic" )
set( DBG_FLAGS "${USUAL_FLAGS} -DATFA_DEBUG -O0 -g3 -fno-inline" )
set( RLS_FLAGS "${USUAL_FLAGS} -DATFA_RELEASE -O3 -march=native -mtune=native" )
set( CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}   ${DBG_FLAGS}" )
set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${RLS_FLAGS}" )

### ### ### Setup compiler
add_executable( atfa
    src/main.cpp
    src/Signal.cpp
    src/Stream.cpp
    src/VAD.cpp
    src/AdaptiveFilter.cpp
    src/utils.cpp
    src/ATFA.cpp
    src/dialogs/ChangeAlgorithmDialog.cpp
    src/dialogs/ChangeRIRDialog.cpp
    src/dialogs/ShowTextDialog.cpp
    src/widgets/FileSelectWidget.cpp
    src/widgets/LEDIndicatorWidget.cpp
)

qt5_use_modules(atfa Core Gui Widgets)

target_link_libraries( atfa
    ${PortAudio_LIBS}
    ${LIBSNDFILE_LIBRARIES}
    ${QT_QTMAIN_LIBRARY} ${QT_LIBRARIES}
    ${MATLAB_LIBS}
    -ldl
)

# TODO: Fix the ill-positioning of latex formula images using CSS

# TODO: write a CPack file for creating .rpms, and etc?

# TODO: find a way to separate documentation and code, so that we can find
#       things easier in code. Consider Doxygen structural commands. Consider
#       Doxygen tag files.

cmake_minimum_required( VERSION 3.3.2 )
project( ATFA )

set( LOCAL_MOD_DIR "${PROJECT_SOURCE_DIR}/cmake/Modules/" )
set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${LOCAL_MOD_DIR} )

find_package( PortAudio REQUIRED )
find_package( LibSndFile REQUIRED )
find_package( Qt4 REQUIRED )

set( USUAL_FLAGS "" )

set( ATFA_Qt_headers
    src/ATFA.h src/dialogs/ChangeAlgorithmDialog.h
    src/dialogs/ChangeRIRDialog.h src/dialogs/ShowTextDialog.h
    src/widgets/FileSelectWidget.h
)

# See the Qt tutorial on writing cmake files for Qt5

### ### ### Setup Qt
QT4_WRAP_CPP( ATFA_MOC_sources ${ATFA_Qt_headers} )
include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )
# Qt4 uses `long long` a lot
set( USUAL_FLAGS "${USUAL_FLAGS} -Wno-long-long" )

### ### ### Setup PortAudio
include_directories( "${PortAudio_INCLUDE_DIRS}" )

### ### ### Setup ATFA
execute_process( COMMAND git describe
                 WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                 OUTPUT_VARIABLE ATFA_VERSION
                 OUTPUT_STRIP_TRAILING_WHITESPACE )
add_definitions( "-DATFA_VERSION=\"${ATFA_VERSION}\"" )
add_definitions( "-DATFA_DIR=\"${PROJECT_SOURCE_DIR}\"" )

set( USUAL_FLAGS "${USUAL_FLAGS} -ansi -Wall -Wextra -pedantic" )
set( DBG_FLAGS "${USUAL_FLAGS} -DATFA_DEBUG -O0 -g3 -fno-inline" )
set( RLS_FLAGS "${USUAL_FLAGS} -DATFA_RELEASE -O2" )
set( CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}   ${DBG_FLAGS}" )
set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${RLS_FLAGS}" )

### ### ### Setup compiler
add_executable( atfa
    src/main.cpp src/Signal.cpp src/Stream.cpp src/utils.cpp src/ATFA.cpp
    src/dialogs/ChangeAlgorithmDialog.cpp src/dialogs/ChangeRIRDialog.cpp
    src/dialogs/ShowTextDialog.cpp
    src/widgets/FileSelectWidget.cpp
    ${ATFA_MOC_sources}
)

target_link_libraries( atfa
    ${PortAudio_LIBS}
    ${LIBSNDFILE_LIBRARIES}
    ${QT_QTMAIN_LIBRARY} ${QT_LIBRARIES}
)

if( WIN32 )
    if( MSVC ) # Check if we are using the Visual Studio compiler
        set_target_properties( atfa PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS" )
    elseif( CMAKE_COMPILER_IS_GNUCXX )
        # SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows" ) # Not tested
    else()
        message( SEND_ERROR
            "You are using an unsupported Windows compiler! (Not MSVC or GCC)"
        )
    endif()
elseif( UNIX )
    # Nothing special required
else()
    message( SEND_ERROR
        "You are on an unsupported platform! (Not Win32 or Unix)"
    )
endif()

# TODO: Fix the ill-positioning of latex formula images using CSS

# TODO: write a CPack file for creating .rpms, and etc?

# TODO: find a way to separate documentation and code, so that we can find
#       things easier in code. Consider Doxygen structural commands. Consider
#       Doxygen tag files.

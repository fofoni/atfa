cmake_minimum_required( VERSION 3.3.2 )
project( ATFA )
set( PROJ "Ambiente de Testes para Filtros Adaptativos" )

if( NOT CMAKE_BUILD_TYPE )
    message(WARNING "No build type set: using \"Release\"")
    set( CMAKE_BUILD_TYPE "Release" )
endif()

message(STATUS "=============================================================")
message(STATUS "${PROJ} -- ${CMAKE_BUILD_TYPE} build")
message(STATUS "")
message(STATUS "CMAKE GENERATOR : ${CMAKE_GENERATOR}")
message(STATUS "SYSTEM NAME     : ${CMAKE_SYSTEM_NAME}")
message(STATUS "=============================================================")

###########
########### Configurações
###########

# Gerar uma Makefile que imprime ou não os comandos de compilação e linkagem
#set( CMAKE_VERBOSE_MAKEFILE YES )

# Gerar um executável que chama ou não a função adapf_getw dos DSO's de
# filtros adaptativos, e, ao final de cada simulação, gera um arquivo
# .mat com todos os sinais presentes em todos os buffers
#set( ATFA_LOG_MATLAB YES )

# Caso o build-type seja DEBUG, compilar os objetos com ou sem as flags
# "-march=native -mtune=native" (o modo RELEASE sempre é compilado com estas
# flags)
set( ATFA_TUNE_DEBUGREL YES )

###########
########### Find libraries
###########
set( LOCAL_MOD_DIR "${PROJECT_SOURCE_DIR}/cmake/Modules/" )
set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${LOCAL_MOD_DIR} )
find_package( PortAudio REQUIRED )
find_package( LibSndFile REQUIRED )
find_package( Qt5Core REQUIRED )
find_package( Qt5Gui REQUIRED )
find_package( Qt5Widgets REQUIRED )

set( USUAL_FLAGS "" )

###########
########### Setup Qt
###########
set( CMAKE_AUTOMOC ON )
set( CMAKE_INCLUDE_CURRENT_DIR ON )

###########
########### Setup PortAudio
###########
if(PortAudio_INCLUDE_DIRS)
    include_directories( "${PortAudio_INCLUDE_DIRS}" )
endif(PortAudio_INCLUDE_DIRS)

###########
########### Setup MATLAB
###########
# http://www.mathworks.com/help/matlab/matlab_external/mat-file-library-and-include-files.html
set( MATLAB_ROOT "/usr/local/MATLAB/MATLAB_Production_Server/R2015a" )
set( MATLAB_ARCH "glnxa64" )
set( MATLAB_INCLUDE_DIRS "${MATLAB_ROOT}/extern/include" )
set( MATLAB_LINK_DIRS "${MATLAB_ROOT}/bin/${MATLAB_ARCH}" )
if(ATFA_LOG_MATLAB)
    include_directories( "${MATLAB_INCLUDE_DIRS}" )
    link_directories( "${MATLAB_LINK_DIRS}" )
    set( MATLAB_LIBS "-lmat -lmx" )
else(ATFA_LOG_MATLAB)
    set( MATLAB_LIBS "" )
endif(ATFA_LOG_MATLAB)

###########
########### Setup ATFA
###########
execute_process( COMMAND git describe
                 WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                 OUTPUT_VARIABLE ATFA_VERSION
                 OUTPUT_STRIP_TRAILING_WHITESPACE )
add_definitions( "-DATFA_VERSION=\"${ATFA_VERSION}\"" )
add_definitions( "-DATFA_DIR=\"${PROJECT_SOURCE_DIR}\"" )
if(ATFA_LOG_MATLAB)
    add_definitions( "-DATFA_LOG_MATLAB" )
endif(ATFA_LOG_MATLAB)

###########
########### Setup compiler
###########
set( WARN_FLAGS "${WARN_FLAGS} -Wall -Wextra -Wsign-conversion" )
set( USUAL_FLAGS "${USUAL_FLAGS} -std=c++11 ${WARN_FLAGS} -pedantic" )
set( DBG_FLAGS "${USUAL_FLAGS} -DATFA_DEBUG -O0 -g3 -fno-inline" )
if(ATFA_TUNE_DEBUGREL)
    set( DBG_FLAGS "${DBG_FLAGS} -march=native -mtune=native" )
endif(ATFA_TUNE_DEBUGREL)
set( RLS_FLAGS "${USUAL_FLAGS} -DATFA_RELEASE -O3 -march=native -mtune=native" )
set( CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}   ${DBG_FLAGS}" )
set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${RLS_FLAGS}" )
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

###########
########### Setup linker
###########
target_link_libraries( atfa
    ${PortAudio_LIBS}
    ${LIBSNDFILE_LIBRARIES}
    ${QT_QTMAIN_LIBRARY} ${QT_LIBRARIES}
    ${MATLAB_LIBS}
    -ldl
)

# TODO: write a CPack file for creating .rpms, and etc?

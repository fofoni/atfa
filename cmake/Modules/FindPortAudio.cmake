# - Try to find PortAudio
# Once done this will define
#
#  PortAudio_FOUND - system has PortAudio
#  PortAudio_INCLUDE_DIRS - the PortAudio include directory
#  PortAudio_LIBS - Link these to use PortAudio
#  PortAudio_DEFINITIONS - Compiler switches required for using PortAudio
#  PortAudio_VERSION - PortAudio version
#
#  Copyright (c) 2006 Andreas Schneider <mail@cynapses.org>
#
# Redistribution and use is allowed according to the terms of the New BSD
#  license. For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if (PortAudio_LIBS AND PortAudio_INCLUDE_DIRS)
  # in cache already
  set(PortAudio_FOUND TRUE)
else (PortAudio_LIBS AND PortAudio_INCLUDE_DIRS)
  if (NOT WIN32)
   include(FindPkgConfig)
   pkg_check_modules(PORTAUDIO2 portaudio-2.0)
  endif (NOT WIN32)

  if (PORTAUDIO2_FOUND)
    set(PortAudio_INCLUDE_DIRS
      ${PORTAUDIO2_INCLUDE_DIRS}
    )
    set(PORTAUDIO_INCLUDE_DIR
      ${PORTAUDIO2_INCLUDE_DIRS}
    )
    if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      set(PortAudio_LIBS "${PORTAUDIO2_LIBRARY_DIRS}/lib${PORTAUDIO2_LIBRARIES}.dylib")
    else (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      set(PortAudio_LIBS
        ${PORTAUDIO2_LIBRARIES}
      )
      set(PORTAUDIO_LIBRARIES
        ${PORTAUDIO2_LIBRARIES}
      )
    endif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(PortAudio_VERSION
      19
    )
    set(PortAudio_FOUND TRUE)
  else (PORTAUDIO2_FOUND)
    find_path(PortAudio_INCLUDE_DIR
      NAMES
        portaudio.h
      PATHS
        /usr/include
        /usr/local/include
        /opt/local/include
        /sw/include
    )

    find_library(PORTAUDIO_LIBRARY
      NAMES
        portaudio
      PATHS
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        /sw/lib
    )

    find_path(PORTAUDIO_LIBRARY_DIR
      NAMES
        portaudio
      PATHS
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        /sw/lib
    )

    set(PortAudio_INCLUDE_DIRS
      ${PortAudio_INCLUDE_DIR}
    )
    set(PortAudio_LIBS
      ${PORTAUDIO_LIBRARY}
    )

    set(PORTAUDIO_LIBRARY_DIRS
      ${PORTAUDIO_LIBRARY_DIR}
    )

    set(PortAudio_VERSION
      19
    )

    if (PortAudio_INCLUDE_DIRS AND PortAudio_LIBS)
       set(PortAudio_FOUND TRUE)
    endif (PortAudio_INCLUDE_DIRS AND PortAudio_LIBS)

    if (PortAudio_FOUND)
      if (NOT Portaudio_FIND_QUIETLY)
        message(STATUS "Found PortAudio: ${PortAudio_LIBS}")
      endif (NOT Portaudio_FIND_QUIETLY)
    else (PortAudio_FOUND)
      if (Portaudio_FIND_REQUIRED)
        message(FATAL_ERROR "Could not find PortAudio")
      endif (Portaudio_FIND_REQUIRED)
    endif (PortAudio_FOUND)
  endif (PORTAUDIO2_FOUND)


  # show the PortAudio_INCLUDE_DIRS and PortAudio_LIBS variables only in the
  # advanced view
  mark_as_advanced(PortAudio_INCLUDE_DIRS PortAudio_LIBS)

endif (PortAudio_LIBS AND PortAudio_INCLUDE_DIRS)

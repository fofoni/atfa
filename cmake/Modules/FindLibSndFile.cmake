# - Try to find LibSndFile
# Once done this will define
#  LIBSNDFILE_FOUND - System has LibSndFile
#  LIBSNDFILE_INCLUDE_DIRS - The LibSndFile include directories
#  LIBSNDFILE_LIBRARIES - The libraries needed to use LibSndFile
#  LIBSNDFILE_DEFINITIONS - Compiler switches required for using LibSndFile

find_package( PkgConfig )
pkg_check_modules( PC_LIBSNDFILE libsndfile )
#set(LIBXML2_DEFINITIONS ${PC_LIBXML_CFLAGS_OTHER})

find_path( LIBSNDFILE_INCLUDE_DIR sndfile.hh
           HINTS ${PC_LIBSNDFILE_INCLUDEDIR} ${PC_LIBSNDFILE_INCLUDE_DIRS} )

find_library( LIBSNDFILE_LIBRARY NAMES sndfile
              HINTS ${PC_LIBSNDFILE_LIBDIR} ${PC_LIBSNDFILE_LIBRARY_DIRS} )

set( LIBSNDFILE_LIBRARIES ${LIBSNDFILE_LIBRARY} )
set( LIBSNDFILE_INCLUDE_DIRS ${LIBSNDFILE_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBSNDFILE_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(LibSndFile  DEFAULT_MSG
                                  LIBSNDFILE_LIBRARY LIBSNDFILE_INCLUDE_DIR)

mark_as_advanced(LIBXML2_INCLUDE_DIR LIBXML2_LIBRARY)

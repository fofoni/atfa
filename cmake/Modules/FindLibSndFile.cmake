# - Try to find LibSndFile
# Once done this will define
#  LIBSNDFILE_FOUND - System has LibSndFile
#  LIBSNDFILE_INCLUDE_DIRS - The LibSndFile include directories
#  LIBSNDFILE_LIBRARIES - The libraries needed to use LibSndFile
#  LIBSNDFILE_DEFINITIONS - Compiler switches required for using LibSndFile

find_package( PkgConfig )
pkg_check_modules( PC_LIBSNDFILE QUIET libsndfile )

find_path( LIBSNDFILE_INCLUDE_DIR sndfile.hh
           HINTS ${PC_LIBSNDFILE_INCLUDEDIR} ${PC_LIBSNDFILE_INCLUDE_DIRS} )

find_library( LIBSNDFILE_LIBRARY NAMES sndfile
              HINTS ${PC_LIBSNDFILE_LIBDIR} ${PC_LIBSNDFILE_LIBRARY_DIRS} )

set( LIBSNDFILE_LIBRARIES ${LIBSNDFILE_LIBRARY} )
set( LIBSNDFILE_INCLUDE_DIRS ${LIBSNDFILE_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibSndFile  DEFAULT_MSG
                                  LIBSNDFILE_LIBRARY LIBSNDFILE_INCLUDE_DIR)

mark_as_advanced(LIBSNDFILE_INCLUDE_DIR LIBSNDFILE_LIBRARY)

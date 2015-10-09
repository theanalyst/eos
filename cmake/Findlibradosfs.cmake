# Try to find kinetic io library
# Once done, this will define
#
# LIBRADOSFS_FOUND        - system has kinetic io library
# LIBRADOSFS_INCLUDE_DIRS - the kinetic io include directories
# LIBRADOSFS_LIBRARIES    - kinetic io libraries

if(LIBRADOSFS_INCLUDE_DIR AND LIBRADOSFS_LIBRARIES)
  set(LIBRADOSFS_FIND_QUIETLY TRUE)
endif(LIBRADOSFS_INCLUDE_DIR AND LIBRADOSFS_LIBRARIES)

find_path(LIBRADOSFS_INCLUDE_DIR libradosfs.hh
          HINTS
          /usr/
          /usr/include
          /opt/radosfs/
          /usr/local/include/
          PATH_SUFFIXES include/radosfs
          PATHS /opt/radosfs/
          )

find_library(LIBRADOSFS_LIBRARIES radosfs
             HINTS /usr/ /opt/radosfs/
             PATH_SUFFIXES lib lib64
             /usr/lib64
             lib/radosfs/
             lib64/radosfs/
             )

# handle the QUIETLY and REQUIRED arguments and set LIBRADOSFS_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libradosfs DEFAULT_MSG LIBRADOSFS_INCLUDE_DIR LIBRADOSFS_LIBRARIES)

mark_as_advanced(LIBRADOSFS_INCLUDE_DIR LIBRADOSFS_LIBRARIES)

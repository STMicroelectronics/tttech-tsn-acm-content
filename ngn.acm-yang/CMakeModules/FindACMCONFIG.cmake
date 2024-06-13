#  ACMCONFIG_FOUND - System has libacmconfig
#  ACMCONFIG_INCLUDE_DIRS - The libacmconfig include directories
#  ACMCONFIG_LIBRARIES - The libraries needed to use libacmconfig
#  ACMCONFIG_DEFINITIONS - Compiler switches required for using libacmconfig

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_ACMCONFIG QUIET acmconfig)
    set(ACMCONFIG_DEFINITIONS ${PC_ACMCONFIG_CFLAGS_OTHER})
endif()

find_path(ACMCONFIG_INCLUDE_DIR libacmconfig/libacmconfig.h
          HINTS ${PC_ACMCONFIG_INCLUDEDIR} ${PC_ACMCONFIG_INCLUDE_DIRS}
          PATH_SUFFIXES acmconfig)

find_library(ACMCONFIG_LIBRARY NAMES acmconfig 
             HINTS ${PC_ACMCONFIG_LIBDIR} ${PC_ACMCONFIG_LIBRARY_DIRS})

set(ACMCONFIG_LIBRARIES ${ACMCONFIG_LIBRARY} )
set(ACMCONFIG_INCLUDE_DIRS ${ACMCONFIG_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(acmconfig DEFAULT_MSG
                                  ACMCONFIG_LIBRARY ACMCONFIG_INCLUDE_DIR)

mark_as_advanced(ACMCONFIG_INCLUDE_DIR ACMCONFIG_LIBRARY)

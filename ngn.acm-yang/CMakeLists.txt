# acm plugin
cmake_minimum_required(VERSION 3.5)
include(GNUInstallDirs)

# include custom Modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules/")

# get version information from git tag (derived from git describe)
include(GetGitVersion)
get_git_version(GIT_VERSION "[0-9]*.[0-9]*.[0-9]*")

add_library(acm SHARED acm.c ${CMAKE_SOURCE_DIR}/common/common.c source/parse_functions)

# dependencies - sysrepo
find_package(SYSREPO REQUIRED)
target_link_libraries(acm ${SYSREPO_LIBRARIES})
include_directories(${SYSREPO_INCLUDE_DIRS})

# set sysrepo plugins directory
set(SR_PLUGINS_DIR /usr/lib/sysrepo/plugins)

# dependencies - libacmconfig
find_library(LIBACMCONFIG acmconfig)
target_link_libraries(acm ${LIBACMCONFIG})

# dependencies - libbase
find_library(LIBBASE base)
target_link_libraries(acm ${LIBBASE})

# include common directory
include_directories(${CMAKE_SOURCE_DIR}/common)

# include source directory
include_directories(${CMAKE_SOURCE_DIR}/source)

target_compile_options(acm PRIVATE -DGIT_VERSION="${GIT_VERSION}" -Werror -Wall -Wextra)

install(TARGETS acm DESTINATION ${SR_PLUGINS_DIR})
#install(FILES ${CMAKE_SOURCE_DIR}/model/acm.yang DESTINATION ${SR_CUSTOM_YANG_DIR})

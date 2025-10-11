###############################################################################
#  Copyright (c) 2016-2023 Joel de Guzman
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
cmake_minimum_required(VERSION 3.5.1)

if (TARGET infra)
   return()
endif()

project(infra)

option(INFRA_FORCE_STD_FS             "force use of std::filesystem in infra"     OFF)
option(INFRA_FORCE_GHC_FS             "force use of ghc::filesystem in infra"     OFF)
option(INFRA_FORCE_STD_STRING_VIEW    "force use of std::string_view in infra"    OFF)
option(INFRA_FORCE_NONSTD_STRING_VIEW "force use of nonstd::string_view in infra" OFF)
option(INFRA_FORCE_STD_OPTIONAL       "force use of std::optional in infra"       OFF)
option(INFRA_FORCE_NONSTD_OPTIONAL    "force use of nonstd::optional in infra"    OFF)

# some ranges of GCC and Clang versions require linker flag
option(INFRA_ADD_FS_LINK "add linker command for standard filesystem library" OFF)

set(INFRA_HEADER             ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(INFRA_FILESYSTEM_HEADER  ${CMAKE_CURRENT_SOURCE_DIR}/external/filesystem/include)
set(INFRA_OPTIONAL_HEADER    ${CMAKE_CURRENT_SOURCE_DIR}/external/optional-lite/include)
set(INFRA_STRING_VIEW_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/external/string-view-lite/include)

# projects common options

# Add our CMake modules to path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

option(CYCFI_ENABLE_GIT_SUBMODULE_CHECK             "check and clone submodules when not available." ON)
option(CYCFI_ENABLE_LTO                             "enable link time optimization for Elements targets" OFF)
set   (CYCFI_USE_EMPTY_SOURCE_GROUPS OFF CACHE BOOL "if to use virtual directories")

add_library(infra INTERFACE)
target_include_directories(infra INTERFACE
   ${INFRA_HEADER}
   ${INFRA_FILESYSTEM_HEADER}
   ${INFRA_OPTIONAL_HEADER}
   ${INFRA_STRING_VIEW_HEADER}
)
target_compile_features(infra INTERFACE cxx_std_14)

if(INFRA_FORCE_STD_FS)
   target_compile_definitions(infra INTERFACE INFRA_FORCE_STD_FS)
endif()

if(INFRA_FORCE_GHC_FS)
   target_compile_definitions(infra INTERFACE INFRA_FORCE_GHC_FS)
endif()

if(INFRA_FORCE_STD_STRING_VIEW)
   target_compile_definitions(infra INTERFACE INFRA_FORCE_STD_STRING_VIEW)
endif()

if(INFRA_FORCE_NONSTD_STRING_VIEW)
   target_compile_definitions(infra INTERFACE INFRA_FORCE_NONSTD_STRING_VIEW)
endif()

if(INFRA_FORCE_STD_OPTIONAL)
   target_compile_definitions(infra INTERFACE INFRA_FORCE_STD_OPTIONAL)
endif()

if(INFRA_FORCE_NONSTD_OPTIONAL)
   target_compile_definitions(infra INTERFACE INFRA_FORCE_NONSTD_OPTIONAL)
endif()

if(MSVC)
   # MSVC does not correctly report __cplusplus by default, so we need to enable it
   # see https://docs.microsoft.com/bs-cyrl-ba/cpp/build/reference/zc-cplusplus?view=msvc-160 for details
   target_compile_options(infra INTERFACE /Zc:__cplusplus)
endif()

if(INFRA_ADD_FS_LINK)
   include(CheckCXXSymbolExists)
   # ciso646 is empty on C++, which makes it a convenient header for checking
   # the currently used libc++.
   # Note that it has been removed in C++20, however:
   # https://en.cppreference.com/w/cpp/header/ciso646
   check_cxx_symbol_exists(_LIBCPP_VERSION "ciso646" HAVE_LIBCPP)
   if(HAVE_LIBCPP)
      target_link_libraries(infra INTERFACE c++fs)
   else()
      target_link_libraries(infra INTERFACE stdc++fs)
   endif()
endif()

add_library(cycfi::infra ALIAS infra)

# Add Infra file list to IDE

file(GLOB_RECURSE PROJECT_FILES "${INFRA_HEADER}/**")
if(CYCFI_USE_EMPTY_SOURCE_GROUPS)
   source_group("" FILES ${PROJECT_FILES})
endif()
add_custom_target(infra_files SOURCES ${PROJECT_FILES})

file(GLOB_RECURSE CMAKE_MODULE_FILES "${CMAKE_SOURCE_DIR}/cmake/**")
if(CYCFI_USE_EMPTY_SOURCE_GROUPS)
   source_group("" FILES ${CMAKE_MODULE_FILES})
endif()
add_custom_target(cmake_modules SOURCES ${CMAKE_MODULE_FILES})

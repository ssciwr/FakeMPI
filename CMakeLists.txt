cmake_minimum_required(VERSION 3.15)

project(
  FakeMPI
  VERSION 3.1.0
  LANGUAGES C)

# Maybe about our partial support for multi config generators (MSVC!)
get_cmake_property(MULTI GENERATOR_IS_MULTI_CONFIG)
if(MULTI)
  message(WARNING "FakeMPI does not properly support multi-config generators."
                  "Only Release builds will be configured.")
  set(CMAKE_CONFIGURATION_TYPES "Release")
endif()

# Initialize some default paths
include(GNUInstallDirs)

# Add the library
add_library(MPI_C lib/unimpi.c lib/experimental_unimpi.c)
target_include_directories(
  MPI_C PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/>
               $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_definitions(MPI_C PUBLIC MPI_IS_FAKEMPI=1)
set_property(TARGET MPI_C PROPERTY C_STANDARD 99)

# Testing stuff
include(CTest)
add_subdirectory(test)

# Version exporting stuff - Modern CMake to the rescue:
# https://cliutils.gitlab.io/modern-cmake/chapters/install/installing.html
include(CMakePackageConfigHelpers)
write_basic_package_version_file(MPIConfigVersion.cmake
                                 COMPATIBILITY AnyNewerVersion)

# Configuration file defining variables for compatibility with FindMPI. This is
# only necessary because of projects that have not switched to using imported
# targets yet. Unfortunately, the results written in this file point to *the
# build directory* instead of the *installation directory*. This is a known
# CMake issue, see https://gitlab.kitware.com/cmake/cmake/-/issues/17984. For
# now, we live with it but should be aware of the fact.
file(
  GENERATE
  OUTPUT ${CMAKE_BINARY_DIR}/MPIConfig-compatibility.cmake
  CONTENT
    "
    set(MPI_C_FOUND 1)
    set(MPI_C_LIBRARIES $<TARGET_FILE:MPI_C>)
    set(MPI_C_INCLUDE_DIRS
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )
  ")

# Installation rules
install(
  TARGETS MPI_C
  EXPORT MPIConfig
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(
  EXPORT MPIConfig
  NAMESPACE MPI::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MPI)

install(FILES ${CMAKE_BINARY_DIR}/MPIConfigVersion.cmake
              ${CMAKE_BINARY_DIR}/MPIConfig-compatibility.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MPI)

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

cmake_minimum_required(VERSION 3.15)

project(
  FakeMPI
  VERSION 3.1.0
  LANGUAGES C)

# Initialize some default paths
include(GNUInstallDirs)

# Add the library
add_library(MPI_C lib/mpi.c)
target_include_directories(
  MPI_C PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/>
               $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# Testing stuff
include(CTest)
add_subdirectory(test)

# Version exporting stuff - Modern CMake to the rescue:
# https://cliutils.gitlab.io/modern-cmake/chapters/install/installing.html

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    MPIConfigVersion.cmake
    COMPATIBILITY AnyNewerVersion
    )

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

install(
  FILES ${CMAKE_BINARY_DIR}/MPIConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MPI)


install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

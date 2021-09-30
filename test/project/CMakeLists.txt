cmake_minimum_required(VERSION 3.15)
project(TestDummy LANGUAGES C)

find_package(
  MPI 3.0
  COMPONENTS C
  REQUIRED)

# Add C Hello World executable
add_executable(helloworld helloworld.c)
target_link_libraries(helloworld PUBLIC MPI::MPI_C)

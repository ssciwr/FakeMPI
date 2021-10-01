# FakeMPI - A sequential MPI stub

*I am still experimenting with this - try at your own risk*

`FakeMPI` is a sequential MPI stub. It implements the MPI C interface with an implementation that is limited to work on one rank.

## What is this good for?

There are some use cases for this type of library. Here are some:

* Compilation of libraries that make use of MPI on systems that do not have MPI or where it is hard to set up MPI. Using `FakeMPI` as a fallback will give you a sequential, but functional version of the library.
* Building of statically linked binaries from libraries that rely on MPI e.g. for the purpose of publishing Python wheels for a sequential version of such library. Normally, this would be quite hard, because building static MPI applications "is not for the weak, and it is not recommended" (quoting [the OpenMPI FAQ](https://www.open-mpi.org/faq/?category=mpi-apps#static-mpi-apps)).

There are also plenty of use cases where you should *not* use this library and instead use a regular MPI implementation and run your executables sequentially.

## How to use it

`FakeMPI` has a modern CMake build system (a.k.a. it exports targets). You can build and install it with:

```
git clone https://github.com/dokempf/FakeMPI.git
cd FakeMPI
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=<path/to/fakempi/install> ..
make
make install
```

Then, in an unrelated CMake build of a library that uses MPI, you can do:

```
cmake -DCMAKE_PREFIX_PATH=<path/to/fakempi/install> -DCMAKE_FIND_PACKAGE_PREFER_CONFIG=ON <path/to/mylib/src>
```

Please note that the `CMAKE_FIND_PACKAGE_PREFER_CONFIG` option was only introduced in CMake 3.15!
It is necessary to have the `FakeMPI` installation take precedence over other MPI installations
found by CMake's `FindMPI.cmake` script.

Within your libraries `CMakeLists.txt`, you can use it just like any other MPI implementation:

```
find_package(MPI REQUIRED)
target_link_libraries(mytarget PUBLIC MPI::MPI_C)
```

## Caveats

There are severe drawbacks and limitations to the approach:

* **FakeMPI is not a fully standard conforming MPI implementation**. Instead, it implements the methods needed to run most applications.
* If your library depends on other libraries that depend on MPI you need to compile these libraries with `FakeMPI` as well - potentially leading to *build from source creep*.
* There is no performance guarantee whatsoever.
* Globally installing `FakeMPI` might shadow your actual MPI for CMake - don't do it.
* Only the C interface is provided.

## Features compared to other such implementations

Several implementations of this idea are floating around. In fact, many HPC libraries ship similar functionality to provide sequential fallbacks (LAMMPS - we borrowed their implementation, PetSc, Dune etc.). This is what is distinct for this distribution:

* Stand-alone distribution that is independent of a library. This allows separation of concern in the sense that a user can compile a library with `FakeMPI` without the library being aware of the fact.
* CMake integration: CMake-based libraries (assuming they follow certain CMake conventions) will pick up `FakeMPI` without changes to the code base.
* `FakeMPI` is a *functional* fallback: It will not only make applications compile, but (hopefully) return correct results.
* Some libraries use similar functionality in a sequential fallback mode with separate code branches. With `FakeMPI` the parallel library code is used, only it is turned into a sequential stub.

## Acknowledgments

The actual implementation is taken from [LAMMPS](https://www.lammps.org/) which uses this functionality internally.
LAMMPS is copyrighted by Sandia Corporation and published under the GPLv2. For more information, see [our Copyright notice](COPYING.md)

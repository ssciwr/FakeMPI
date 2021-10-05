#include "experimental_unimpi.h"

int
MPI_Comm_create_errhandler(MPI_Comm_errhandler_function* comm_errhandler_fn,

                           MPI_Errhandler* errhandler)
{
  return MPI_SUCCESS;
}

int
MPI_Comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler)
{
  return MPI_SUCCESS;
}
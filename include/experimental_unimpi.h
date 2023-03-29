#if !defined(EXPERIMENTAL_MPIUNI_H)
#define EXPERIMENTAL_MPIUNI_H

#include "unimpi.h"

#if defined(__cplusplus)
extern "C"
{
#endif

#define MPI_VERSION 3

#define MPI_ERR_ARG 13
#define MPI_MAX_INFO_KEY 2056
#define MPI_MAX_INFO_VAL 2056

  typedef void MPI_Comm_errhandler_function(MPI_Comm*, int*, ...);
  int MPI_Comm_create_errhandler(
    MPI_Comm_errhandler_function* comm_errhandler_fn,
    MPI_Errhandler* errhandler);
  int MPI_Comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler);

  typedef int MPI_Message;
  typedef int MPI_Count;

#if defined(__cplusplus)
}
#endif

#endif

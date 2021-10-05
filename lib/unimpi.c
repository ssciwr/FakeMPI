/*
      This provides a few of the MPI-uni functions that cannot be implemented
    with C macros
*/
#include "mpi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(MPIUNI_H)
#error "Wrong mpi.h included! require mpi.h from MPIUNI"
#endif

#define MPI_SUCCESS 0
#define MPI_FAILURE 1

void* MPIUNI_TMP = NULL;

/*
       With MPI Uni there are exactly four distinct communicators:
    MPI_COMM_SELF, MPI_COMM_WORLD, and a MPI_Comm_dup() of each of these
   (duplicates of duplicates return the same communictor)

    MPI_COMM_SELF and MPI_COMM_WORLD are MPI_Comm_free() in MPI_Finalize() but
   in general with PETSc, the other communicators are freed once the last PETSc
   object is freed (before MPI_Finalize()).

*/
#define MAX_ATTR 256
#define MAX_COMM 128

typedef struct
{
  void* attribute_val;
  int active;
} MPI_Attr;

typedef struct
{
  void* extra_state;
  MPI_Delete_function* del;
  int active; /* Is this keyval in use by some comm? */
} MPI_Attr_keyval;

static MPI_Attr_keyval attr_keyval[MAX_ATTR];
static MPI_Attr attr[MAX_COMM][MAX_ATTR];
static int
  comm_active[MAX_COMM]; /* Boolean array indicating which comms are in use */
static int mpi_tag_ub = 100000000;
static int num_attr = 1; /* Maximal number of keyvals/attributes ever created,
                            including the predefined MPI_TAG_UB attribute. */
static int MaxComm =
  2; /* Maximal number of communicators ever created, including comm_self(1),
        comm_world(2), but not comm_null(0) */
static void* MPIUNIF_mpi_in_place = 0;

#define CommIdx(comm)                                                          \
  ((comm)-1) /* the communicator's internal index used in attr[idx][] and      \
                comm_active[idx]. comm_null does not occupy slots in attr[][]  \
              */

#if defined(__cplusplus)
extern "C"
{
#endif

  /*
     To avoid problems with prototypes to the system memcpy() it is duplicated
     here
  */
  int MPIUNI_Memcpy(void* dst, const void* src, int n)
  {
    if (dst == MPI_IN_PLACE || dst == MPIUNIF_mpi_in_place)
      return MPI_SUCCESS;
    if (src == MPI_IN_PLACE || src == MPIUNIF_mpi_in_place)
      return MPI_SUCCESS;
    if (!n)
      return MPI_SUCCESS;

    /* GPU-aware MPIUNI. Use synchronous copy per MPI semantics */
    {
      memcpy(dst, src, n);
    }
    return MPI_SUCCESS;
  }

  static int classcnt = 0;
  static int codecnt = 0;

  int MPI_Add_error_class(int* cl)
  {
    *cl = classcnt++;
    return MPI_SUCCESS;
  }

  int MPI_Add_error_code(int cl, int* co)
  {
    if (cl >= classcnt)
      return MPI_FAILURE;
    *co = codecnt++;
    return MPI_SUCCESS;
  }

  int MPI_Type_get_envelope(MPI_Datatype datatype,
                            int* num_integers,
                            int* num_addresses,
                            int* num_datatypes,
                            int* combiner)
  {
    int comb = datatype >> 28;
    switch (comb) {
      case MPI_COMBINER_NAMED:
        *num_integers = 0;
        *num_addresses = 0;
        *num_datatypes = 0;
        *combiner = comb;
        break;
      case MPI_COMBINER_DUP:
        *num_integers = 0;
        *num_addresses = 0;
        *num_datatypes = 1;
        *combiner = comb;
        break;
      case MPI_COMBINER_CONTIGUOUS:
        *num_integers = 1;
        *num_addresses = 0;
        *num_datatypes = 1;
        *combiner = comb;
        break;
      default:
        return MPIUni_Abort(MPI_COMM_SELF, 1);
    }
    return MPI_SUCCESS;
  }

  int MPI_Type_get_contents(MPI_Datatype datatype,
                            int max_integers,
                            int max_addresses,
                            int max_datatypes,
                            int* array_of_integers,
                            MPI_Aint* array_of_addresses,
                            MPI_Datatype* array_of_datatypes)
  {
    int comb = datatype >> 28;
    switch (comb) {
      case MPI_COMBINER_NAMED:
        return MPIUni_Abort(MPI_COMM_SELF, 1);
      case MPI_COMBINER_DUP:
        if (max_datatypes < 1)
          return MPIUni_Abort(MPI_COMM_SELF, 1);
        array_of_datatypes[0] = datatype & 0x0fffffff;
        break;
      case MPI_COMBINER_CONTIGUOUS:
        if (max_integers < 1 || max_datatypes < 1)
          return MPIUni_Abort(MPI_COMM_SELF, 1);
        array_of_integers[0] = (datatype >> 8) & 0xfff; /* count */
        array_of_datatypes[0] = (datatype & 0x0ff000ff) |
                                0x100; /* basic named type (count=1) from which
                                          the contiguous type is derived */
        break;
      default:
        return MPIUni_Abort(MPI_COMM_SELF, 1);
    }
    return MPI_SUCCESS;
  }

  /*
     Used to set the built-in MPI_TAG_UB attribute
  */
  static int Keyval_setup(void)
  {
    attr[CommIdx(MPI_COMM_WORLD)][0].active = 1;
    attr[CommIdx(MPI_COMM_WORLD)][0].attribute_val = &mpi_tag_ub;
    attr[CommIdx(MPI_COMM_SELF)][0].active = 1;
    attr[CommIdx(MPI_COMM_SELF)][0].attribute_val = &mpi_tag_ub;
    attr_keyval[0].active = 1;
    return MPI_SUCCESS;
  }

  int MPI_Comm_create_keyval(MPI_Copy_function* copy_fn,
                             MPI_Delete_function* delete_fn,
                             int* keyval,
                             void* extra_state)
  {
    int i, keyid;
    for (i = 1; i < num_attr; i++) { /* the first attribute is always in use */
      if (!attr_keyval[i].active) {
        keyid = i;
        goto found;
      }
    }
    if (num_attr >= MAX_ATTR)
      return MPIUni_Abort(MPI_COMM_WORLD, 1);
    keyid = num_attr++;

  found:
    attr_keyval[keyid].extra_state = extra_state;
    attr_keyval[keyid].del = delete_fn;
    attr_keyval[keyid].active = 1;
    *keyval = keyid;
    return MPI_SUCCESS;
  }

  int MPI_Comm_free_keyval(int* keyval)
  {
    attr_keyval[*keyval].extra_state = 0;
    attr_keyval[*keyval].del = 0;
    attr_keyval[*keyval].active = 0;
    *keyval = 0;
    return MPI_SUCCESS;
  }

  int MPI_Comm_set_attr(MPI_Comm comm, int keyval, void* attribute_val)
  {
    int idx = CommIdx(comm);
    if (comm < 1 || comm > MaxComm)
      return MPI_FAILURE;
    attr[idx][keyval].active = 1;
    attr[idx][keyval].attribute_val = attribute_val;
    return MPI_SUCCESS;
  }

  int MPI_Comm_delete_attr(MPI_Comm comm, int keyval)
  {
    int idx = CommIdx(comm);
    if (comm < 1 || comm > MaxComm)
      return MPI_FAILURE;
    if (attr[idx][keyval].active && attr_keyval[keyval].del) {
      void* save_attribute_val = attr[idx][keyval].attribute_val;
      attr[idx][keyval].active = 0;
      attr[idx][keyval].attribute_val = 0;
      (*(attr_keyval[keyval].del))(
        comm, keyval, save_attribute_val, attr_keyval[keyval].extra_state);
    }
    return MPI_SUCCESS;
  }

  int MPI_Comm_get_attr(MPI_Comm comm,
                        int keyval,
                        void* attribute_val,
                        int* flag)
  {
    int idx = CommIdx(comm);
    if (comm < 1 || comm > MaxComm)
      return MPI_FAILURE;
    if (!keyval)
      Keyval_setup();
    *flag = attr[idx][keyval].active;
    *(void**)attribute_val = attr[idx][keyval].attribute_val;
    return MPI_SUCCESS;
  }

  int MPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm* newcomm)
  {
    int j;
    if (comm < 1 || comm > MaxComm)
      return MPI_FAILURE;
    for (j = 3; j <= MaxComm; j++) {
      if (!comm_active[CommIdx(j)]) {
        comm_active[CommIdx(j)] = 1;
        *newcomm = j;
        return MPI_SUCCESS;
      }
    }
    if (MaxComm >= MAX_COMM)
      return MPI_FAILURE;
    *newcomm = ++MaxComm;
    comm_active[CommIdx(*newcomm)] = 1;
    return MPI_SUCCESS;
  }

  int MPI_Comm_dup(MPI_Comm comm, MPI_Comm* out)
  {
    int j;
    if (comm < 1 || comm > MaxComm)
      return MPI_FAILURE;
    for (j = 3; j <= MaxComm; j++) {
      if (!comm_active[CommIdx(j)]) {
        comm_active[CommIdx(j)] = 1;
        *out = j;
        return MPI_SUCCESS;
      }
    }
    if (MaxComm >= MAX_COMM)
      return MPI_FAILURE;
    *out = ++MaxComm;
    comm_active[CommIdx(*out)] = 1;
    return MPI_SUCCESS;
  }

  int MPI_Comm_free(MPI_Comm* comm)
  {
    int i;
    int idx = CommIdx(*comm);

    if (*comm < 1 || *comm > MaxComm)
      return MPI_FAILURE;
    for (i = 0; i < num_attr; i++) {
      if (attr[idx][i].active && attr_keyval[i].del)
        (*attr_keyval[i].del)(
          *comm, i, attr[idx][i].attribute_val, attr_keyval[i].extra_state);
      attr[idx][i].active = 0;
      attr[idx][i].attribute_val = 0;
    }
    if (*comm >= 3)
      comm_active[idx] = 0;
    *comm = 0;
    return MPI_SUCCESS;
  }

  int MPI_Comm_size(MPI_Comm comm, int* size)
  {
    if (comm < 1 || comm > MaxComm)
      return MPI_FAILURE;
    *size = 1;
    return MPI_SUCCESS;
  }

  int MPI_Comm_rank(MPI_Comm comm, int* rank)
  {
    if (comm < 1 || comm > MaxComm)
      return MPI_FAILURE;
    *rank = 0;
    return MPI_SUCCESS;
  }

  int MPIUni_Abort(MPI_Comm comm, int errorcode)
  {
    printf("MPI operation not supported by PETSc's sequential MPI wrappers\n");
    return MPI_ERR_NOSUPPORT;
  }

  int MPI_Abort(MPI_Comm comm, int errorcode)
  {
    abort();
    return MPI_SUCCESS;
  }

  /* --------------------------------------------------------------------------*/

  static int MPI_was_initialized = 0;
  static int MPI_was_finalized = 0;

  int MPI_Init(int* argc, char*** argv)
  {
    if (MPI_was_initialized)
      return MPI_FAILURE;
    /* MPI standard says "once MPI_Finalize returns, no MPI routine (not even
       MPI_Init) may be called", so an MPI standard compliant MPIU should have
       this 'if (MPI_was_finalized) return MPI_FAILURE;' check. We relax it here
       to make life easier for users of MPIU so that they can do multiple
       PetscInitialize/Finalize().
    */
    /* if (MPI_was_finalized) return MPI_FAILURE; */
    MPI_was_initialized = 1;
    MPI_was_finalized = 0;
    return MPI_SUCCESS;
  }

  int MPI_Init_thread(int* argc, char*** argv, int required, int* provided)
  {
    MPI_Query_thread(provided);
    return MPI_Init(argc, argv);
  }

  int MPI_Query_thread(int* provided)
  {
    *provided = MPI_THREAD_FUNNELED;
    return MPI_SUCCESS;
  }

  int MPI_Finalize(void)
  {
    MPI_Comm comm;
    if (MPI_was_finalized)
      return MPI_FAILURE;
    if (!MPI_was_initialized)
      return MPI_FAILURE;
    comm = MPI_COMM_WORLD;
    MPI_Comm_free(&comm);
    comm = MPI_COMM_SELF;
    MPI_Comm_free(&comm);

    /* reset counters */
    MaxComm = 2;
    num_attr = 1;
    MPI_was_finalized = 1;
    MPI_was_initialized = 0;
    return MPI_SUCCESS;
  }

  int MPI_Initialized(int* flag)
  {
    *flag = MPI_was_initialized;
    return MPI_SUCCESS;
  }

  int MPI_Finalized(int* flag)
  {
    *flag = MPI_was_finalized;
    return MPI_SUCCESS;
  }

#if defined(__cplusplus)
}
#endif

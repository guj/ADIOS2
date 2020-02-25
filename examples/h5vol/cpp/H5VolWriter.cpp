//#define H5VL_FRIEND // junmin added

#include "H5VLpublic.h"
//#include "H5Dprivate.h" // H5D_t
//#include "H5Iprivate.h" // H5I_object_verify
#include "hdf5.h"
#define H5S_FRIEND  // suppress error for H5Spkg
//#include "H5Spkg.h" // H5S_hyper_dim
#define H5O_FRIEND  // suppress error for H5Opkg
//#include "H5Opkg.h"
//#include "mpi.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "H5Epublic.h"
#include "H5VolWriter.h"
#include "error.h"

// these are in h5private.h
#define H5_ATTR_UNUSED /**/
#define SUCCEED 1
#define FAIL 0

static hid_t H5VL_ADIOS_g = -1;
static MPI_Comm H5VL_MPI_COMM = MPI_COMM_SELF;
static MPI_Info H5VL_MPI_INFO = MPI_INFO_NULL;
static int HDF5_MAX_STR_SIZE_IN_ARRAY = 30;

static int h5i_search_func(void *obj, hid_t id, void *key) {
  if (key == obj) {
    printf("id = %llu\n", id);
    return 1;
  } else
    return 0;
}

//
// either "ts_number" or ".../ts_number"
// is a valid dataset name
//
static int isDatasetName(const char *name) {
  int len = strlen(name);
  while (true) {
    char c = name[len - 1];
    if (c >= '0' && c <= '9') {
      len--;
      if (len == 0) {
        return 1;
      }
    } else if ((len < strlen(name)) && (c == '/')) {
      return 1;
    } else {
      break;
    }
  }
  return 0;
}

//
// returns numOfDigits of timestep
//
static int getDatasetTimeStep(const char *name, int *ts) {
  /*
  int power = -1;
  int len = strlen(name);
  while (true) {
    char c = name[len - 1];
    if (c >= '0' && c <= '9') {
      if (*ts < 0)
        *ts = 0; // initialize

      power++;
      int digit = c - '0';
      *ts += digit * pow(10, power);
      len--;
      if (len == 0) {
        return (power + 1);
      }
    } else if ((len < strlen(name)) && (c == '/')) {
      return (power + 1);
    } else {
      break;
    }
  }

  return power;
  */
}

static void *assignDim(struct adios_index_comp_struct_v1 *varInAdios /* in */,
                       H5VL_adios_var_t *var /* out */) {
#ifdef NEVER
  int idx = adios_get_ch_idx(varInAdios, var->curr_timestep);

  // ADIOS_VOL_LOG_ERR("out of bound timestep: %d\n", val->curr_timestep);

  REQUIRE_SUCC_MSG((idx >= 0), NULL, "out of bound timestep: %d\n",
                   var->curr_timestep);

  int ndim = adios_get_var_ndim(var->fileReader, idx, varInAdios,
                                &(var->is_global_dim));
  if (ndim < 0) {
    ADIOS_VOL_LOG_ERR("Not a file or file object");
    return NULL;
  }

  var->dimInfo->ndim = ndim;

  // var->is_global_dim = 1;
  if (var->dimInfo->ndim > 0) {
    var->dimInfo->dims =
        (hsize_t *)SAFE_CALLOC(var->dimInfo->ndim, sizeof(hsize_t));

    int i;
    for (i = 0; i < var->dimInfo->ndim; i++) {
      // dims order: local/global/offset for each dimension
      uint64_t temp = varInAdios->characteristics[idx].dims.charDims[i * 3 + 1];
      if (temp > 0) {
        var->dimInfo->dims[i] = temp;
      } else {
        var->dimInfo->dims[i] =
            varInAdios->characteristics[idx].dims.charDims[i * 3];
      }
      // printf(" [%dth: %llu] ", i, var->dimInfo->dims[i]);
    }

    if (ADIOS_YES == var->fileReader->adios_host_language_fortran) {
      // file was in fortran order
      for (i = 0; i < ndim / 2; i++) {
        uint64_t temp = var->dimInfo->dims[i];
        var->dimInfo->dims[i] = var->dimInfo->dims[ndim - i - 1];
        var->dimInfo->dims[ndim - i - 1] = temp;
      }
    }
  }
  return var;
#else
  return var;
#endif
}

static void *createVar(H5_bp_file_t *fileReader, const char *pathWithTimeStep) {
  /*
  */
}

//
// static void adios_vol_not_supported(const char* msg) {printf("%s is not yet
// supported in ADIOS VOL\n", msg);}

// static hid_t toHDF5type(enum ADIOS_DATATYPES adiosType) {
static hid_t toHDF5type(struct adios_index_comp_struct_v1 *adiosVar) {
#ifdef NEVER
  enum ADIOS_DATATYPES adiosType = adiosVar->type;
  //
  // this is here to make sure H5Tget_class(type) returns H5T_STRING
  // actual size here is not determined here
  //

  int size_of_type =
      adios_get_type_size(adiosType, adiosVar->characteristics[0].value);

  switch (adiosType) {
  case adios_string: {
    hid_t atype = H5Tcopy(H5T_C_S1);
    H5Tset_size(atype, size_of_type);
    return atype;
  }
  case adios_byte:
    return H5T_NATIVE_CHAR;

  case adios_unsigned_byte:
    return H5T_NATIVE_UCHAR;

  case adios_string_array: {
    int maxStrSize = HDF5_MAX_STR_SIZE_IN_ARRAY;
    hid_t atype = H5Tcopy(H5T_C_S1);
    herr_t ret = H5Tset_size(atype, maxStrSize);
    ret = H5Tset_strpad(atype, H5T_STR_NULLTERM);
    return atype;
    // return H5T_STRING;
  }
  case adios_short:
    return H5T_NATIVE_SHORT;

  case adios_unsigned_short:
    return H5T_NATIVE_USHORT;

  case adios_integer:
    return H5T_NATIVE_INT;

  case adios_unsigned_integer:
    return H5T_NATIVE_UINT;

  case adios_long:
    return H5T_NATIVE_LONG;

  case adios_unsigned_long:
    return H5T_NATIVE_ULONG;

  case adios_real:
    return H5T_NATIVE_FLOAT;

  case adios_double:
    return H5T_NATIVE_DOUBLE;

  case adios_long_double:
    return H5T_NATIVE_LDOUBLE;

  case adios_complex: {
    size_t h5float_size = H5Tget_size(H5T_NATIVE_FLOAT);
    hid_t m_DefH5TypeComplexFloat = H5Tcreate(H5T_COMPOUND, 2 * h5float_size);

    H5Tinsert(m_DefH5TypeComplexFloat, "freal", 0, H5T_NATIVE_FLOAT);
    H5Tinsert(m_DefH5TypeComplexFloat, "fimg", h5float_size, H5T_NATIVE_FLOAT);

    return m_DefH5TypeComplexFloat;
  }
  case adios_double_complex: {
    size_t h5double_size = H5Tget_size(H5T_NATIVE_DOUBLE);
    hid_t m_DefH5TypeComplexDouble = H5Tcreate(H5T_COMPOUND, 2 * h5double_size);

    H5Tinsert(m_DefH5TypeComplexDouble, "dreal", 0, H5T_NATIVE_DOUBLE);
    H5Tinsert(m_DefH5TypeComplexDouble, "dimg", h5double_size,
              H5T_NATIVE_DOUBLE);

    return m_DefH5TypeComplexDouble;
  }
  // return mH5T_COMPLEX_DOUBLE;
  default:
    SHOW_ERROR_MSG("unknown adios type %d found.", adiosType);
    return 0;
  }
#else
  return 0;
#endif
}



int call_adios_get_num_vars(H5_bp_file_t *f) {
  /*
  int num = 0;

  struct adios_index_comp_struct_v1 *v = f->vars_root;
  while (v) {
    num++;
    v = v->next;
  }
  return num;
*/
}

int call_adios_get_num_attributes(H5_bp_file_t *f) {
  /*
  int num = 0;

  struct adios_index_comp_struct_v1 *v = f->attrs_root;
  while (v) {
    num++;
    v = v->next;
  }
  return num;
  */
}

int call_is_valid_bpfile(const char *name, H5_bp_file_t *bpFile) {
  /*
  REQUIRE_NOT_NULL(bpFile);
  // printf("  check adios validity here.\n");

  int isBpFile = 1;
  int err, flag = 0;

  MPI_File fh = 0;

#ifdef NEVER
  err = MPI_File_open(MPI_COMM_SELF, (char *)name, MPI_MODE_RDONLY,
                      (MPI_Info)MPI_INFO_NULL, &fh);
#else
  err = MPI_File_open(H5VL_MPI_COMM, (char *)name, MPI_MODE_RDONLY,
                      H5VL_MPI_INFO, &fh);

#endif
  // H5Adios_list_MPI_Error(err);
  REQUIRE_MPI_SUCC(err);

  REQUIRE_SUCC((0 == call_init(bpFile, fh)), -1);

  REQUIRE_SUCC((0 == call_seek_back(bpFile, ADIOS_BP_MINIFOOTER_SIZE)), -1);

  int offset = ADIOS_BP_MINIFOOTER_SIZE - 4;

  REQUIRE_SUCC((0 == call_adios_get_version(bpFile, offset)), -1);

  // printf("version=%d\n", bpFile->bp_version);

  if (bpFile->bp_version < 3) {
    printf("bp version is not up to date. \n");
    return 0;
  }

  bpFile->file_name = SAFE_MALLOC(strlen(name) + 1);

  REQUIRE_NOT_NULL(bpFile->file_name);

  sprintf(bpFile->file_name, "%s", name);
  bpFile->file_name[strlen(name)] = '\0';

  return isBpFile;
  */
}




static herr_t H5VL_adios_file_get(void *file, H5VL_file_get_t get_type,
                                  hid_t dxpl_id, void **req,
                                  va_list arguments) {
#ifdef NEVER
  herr_t ret_value = SUCCEED;

  H5VL_adios_t *f = (H5VL_adios_t *)file;

  switch (get_type) {
  /* H5Fget_access_plist */
  case H5VL_FILE_GET_FAPL:
    ADIOS_VOL_NOT_SUPPORTED_ERR("H5VL_FILE_GET_FAPL\n");
    break;
  case H5VL_FILE_GET_FCPL:
    ADIOS_VOL_NOT_SUPPORTED_ERR("H5VL_FILE_GET_FCPL\n");
    break;
  case H5VL_FILE_GET_OBJ_COUNT: {
    int type = va_arg(arguments, unsigned int);
    ssize_t *ret = va_arg(arguments, ssize_t *);

    if (type == H5F_OBJ_DATASET) {
      ssize_t result = call_adios_get_num_vars(f->bpFileReader);
      *ret = (ssize_t)result;
    } else if (type == H5F_OBJ_ATTR) {
      ssize_t result = call_adios_get_num_attributes(f->bpFileReader);
      *ret = (ssize_t)result;
    } else {
      ADIOS_VOL_NOT_SUPPORTED_ERR("count types other than dataset\n");
    }
    break;
  }
  case H5VL_FILE_GET_OBJ_IDS: {
    unsigned type = va_arg(arguments, unsigned int);
    size_t max_objs = va_arg(arguments, size_t);
    hid_t *oid_list = va_arg(arguments, hid_t *);
    ssize_t *ret = va_arg(arguments, ssize_t *);
    size_t obj_count = 0; /* Number of opened objects */

    if (type == H5F_OBJ_DATASET) {
      printf("todo: return var ids \n");
      // temp set to obj_count
      int i;
      for (i = 0; i < obj_count; i++) {
        oid_list[i] = i;
      }
      *ret = (ssize_t)max_objs;
    } else {
      ADIOS_VOL_NOT_SUPPORTED_ERR("get ids of types  other than dataset\n");
    }
    break;
  }
  case H5VL_FILE_GET_INTENT:
    ADIOS_VOL_NOT_SUPPORTED_ERR("H5VL_FILE_GET_INTENT\n");
    break;
  case H5VL_FILE_GET_NAME: {
    H5I_type_t type = va_arg(arguments, H5I_type_t);
    size_t size = va_arg(arguments, size_t);
    char *name = va_arg(arguments, char *);
    ssize_t *ret = va_arg(arguments, ssize_t *);

    size_t len = strlen(f->bpFileReader->file_name);
    if (name) {
      sprintf(name, "%s", f->bpFileReader->file_name);
      name[len] = '\0';
    }

    // Set the return value for the API call
    *ret = (ssize_t)len;

    break;
  }
  case H5VL_OBJECT_GET_FILE: {
    break;
  }
  default:
    ADIOS_VOL_NOT_SUPPORTED_ERR("H5VL_FILE_GET flag");
  }

  return ret_value;
#endif
}

hid_t
H5VL_adios_register(void)
{
  if(H5I_VOL != H5Iget_type(H5VL_ADIOS_g)) {

    H5VL_ADIOS_g = H5VLregister_connector(&H5VL_adios2_def, H5P_DEFAULT);
    if (H5VL_ADIOS_g <= 0) {
      printf("  [ECP ADIOS VOL ERROR] with H5VLregister_connector\n");
      return -1;
    }    
  }

  return H5VL_ADIOS_g;
}

herr_t H5Pset_fapl_adios(hid_t *acc_tpl, hid_t *fapl_id) {
  //hid_t plugin_id = H5VLget_driver_id(VOLNAME);
  //H5VLinitialize(plugin_id, H5P_DEFAULT);
  //H5VLclose(plugin_id);

  if (H5VL_adios_register() < 0) 
    return -1;

  /*
  */

#ifdef NEVER
  hid_t ret_value = H5Pset_vol(*acc_tpl, H5VL_ADIOS_g, fapl_id);
  if (H5Pget_driver(*acc_tpl) == H5FD_MPIO) {
    H5Pget_fapl_mpio(*acc_tpl, &H5VL_MPI_COMM, &H5VL_MPI_INFO);
    int rank, size;
    MPI_Comm_rank(H5VL_MPI_COMM, &rank);
    MPI_Comm_size(H5VL_MPI_COMM, &size);
    if (rank == 0)
      printf(" ... parallel ..size=%d\n", size);
  } else {
    // printf(" ... serial ..\n");
  }
#endif

  return -1;
}

herr_t H5P_unset_adios() { return H5VLunregister_connector(H5VL_ADIOS_g); }

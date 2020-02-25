#ifndef ADIOS_VOL_WRITER_H
#define ADIOS_VOL_WRITER_H

//#define H5VL_FRIEND // junmin added
#define H5F_FRIEND /*suppress error about including H5Fpkg   */

//#include "H5Fpkg.h"
#include "hdf5.h"

#include "mpi.h"
//#include "reader.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ADIOS2 511
#define ADIOS2_VOL_WRITER_NAME "ADIOS-VOL-WRITER"

#define ADIOS_TIMESTEP "ADIOS_TIMESTEP"

extern hid_t GLOBAL_FILE_ID;

typedef struct {
  int to_be_done;
} H5_bp_file_t;

typedef struct H5VL_complex_float_t {
  float real;
  float img;
} H5VLT_complex_float_t;

typedef struct H5VL_complex_double_t {
  double real;
  double img;
} H5VLT_complex_double_t;

typedef struct H5VL_adios_dim_t {
  hsize_t ndim;
  hsize_t *dims;
} H5VL_adios_dim_t;

typedef struct H5VL_adios_var_t {
  char *name;
  unsigned int ntimestep;

  H5VL_adios_dim_t *dimInfo;

  unsigned int curr_timestep;
  unsigned int is_global_dim;

  H5_bp_file_t *fileReader;

  // hid_t assigned_gid;
} H5VL_adios_var_t;

typedef struct H5VL_adios_attr_t {
  char *name;
  // H5VL_adios_dim_t *dimInfo;
  int ts; // only needed forthe ADIOS_TIMESTEP
  H5_bp_file_t *fileReader;
} H5VL_adios_attr_t;

typedef struct H5VL_adios2_t {
  H5_bp_file_t *bpFileReader;
} H5VL_adios2_t;

typedef struct H5VL_link_adios_varnames_t {
  hid_t fid;
  int rank;
  int size;
  char **varNames;
} H5VL_adios_varnames_t;

typedef struct {
  hsize_t posCompact;  // e.g. 0,1,2,3
  hsize_t posInSource; // e.g. 3,5,8.9
} H5_posMap;

void GetSelOrder(hid_t space_id, H5S_sel_type space_type, H5_posMap **result);

void assignToMemSpace(H5_posMap *sourceSelOrderInC,
                      H5_posMap *targetSelOrderInC, hsize_t total,
                      size_t dataTypeSize, char *adiosData, char *buf);

herr_t H5Pset_fapl_adios(hid_t *acc_tpl, hid_t *fapl_id);
herr_t H5P_unset_adios();

//
// VL definition 
//
static herr_t H5VL_adios2_init(hid_t vipl_id)
{
  return  0;
}

static herr_t H5VL_adios2_term(void)
{
  return 0;
}
static void *H5VL_adios2_file_create(const char *name, unsigned flags, hid_t fcpl_id, hid_t fapl_id, hid_t dxpl_id, void **req)
{
  printf(" creating  file: %s\n", name);
  
  
}
static void *H5VL_adios2_file_open(const char *name, unsigned flags, hid_t fapl_id, hid_t dxpl_id, void **req)
{
  printf(" openinging  file: %s\n", name);
}
				   

static const H5VL_class_t H5VL_adios2_def = {
    0,
    ADIOS2,
    ADIOS2_VOL_WRITER_NAME,         /* name */
    0,
    H5VL_adios2_init, /* initialize */
    H5VL_adios2_term, /* terminate */
    //sizeof(hid_t),   /* size of vol info */
    //NULL,            /* copy vol info */
    //NULL,            /* free vol info copy*/
    {   /* info_cls */
      (size_t)0,                                  /* info size    */
      NULL,                                       /* info copy    */
      NULL,                                       /* info compare */
      NULL,                                       /* info free    */
      NULL,                                       /* info to str  */
      NULL                                        /* str to info  */
    },
    {   /* wrap_cls */
      NULL,                                       /* get_object   */
      NULL,                                       /* get_wrap_ctx */
      NULL,                                       /* wrap_object  */
      NULL,                                       /* unwrap_object */
      NULL                                        /* free_wrap_ctx */
    },

    {
        /* attribute_cls */
        NULL, // H5VL_adios_attr_create,                /* create */
        NULL, // H5VL_adios_attr_open, /* open */
        NULL, // H5VL_adios_attr_read, /* read */
        NULL, // H5VL_adios_attr_write,                 /* write */
        NULL, // H5VL_adios_attr_get,      /* get */
        NULL, // H5VL_adios_attr_specific, /* specific */
        NULL,                     // H5VL_adios_attr_optional,  /*optional*/
        NULL //H5VL_adios_attr_close     /* close */
    },
    {
        /* dataset_cls */
        NULL, // H5VL_adios_dataset_create, /* create */
        NULL, // H5VL_adios_dataset_open, /* open */
        NULL, // H5VL_adios_dataset_read, /* read */
        NULL, // H5VL_adios_dataset_write,  /* write */
        NULL, // H5VL_adios_dataset_get,  /* get properties*/
        NULL,                    // H5VL_adios_dataset_specific
        NULL,                    // optional
        NULL // H5VL_adios_dataset_close /* close */
    },
    {
        /* datatype_cls */
        NULL, // H5VL_adios_datatype_commit,           /* commit */
        NULL, // H5VL_adios_datatype_open,             /* open */
        NULL, //H5VL_adios_datatype_get, /* get_size */
        NULL                    // H5VL_adios_datatype_close /* close */
    },
    {
        /* file_cls */
        H5VL_adios2_file_create, /* create file */
                              // NULL,
        H5VL_adios2_file_open, /* open */
        NULL, //H5VL_adios_file_get,  /* get */
        NULL, // H5VL_adios_file_misc,                  /* misc */
        NULL, // H5VL_adios_file_optional, /* optional */
        NULL //H5VL_adios_file_close     /* close */
    },
    {
        /* group_cls */
        NULL,                  // H5VL_adios_group_create, /* create */
        NULL, // H5VL_adios_group_open, /* open */
        NULL, // H5VL_adios_group_get,  /* get  */
        NULL,                  /* specific */
        NULL,                  /* optional */
        NULL  //H5VL_adios_group_close /* close */
    },
    {
        /* link_cls */
        NULL, // H5VL_adios_link_create,                /* create */
        NULL, // H5VL_adios_link_copy,                  /* copy   */
        NULL, // H5VL_adios_link_move,                  /* move   */
        NULL, // H5VL_adios_link_get,      /* get    */
        NULL, // H5VL_adios_link_specific, /* iterate etc*/
        NULL // H5VL_adios_link_remove                 /* remove */
    },
    {
        /* object_cls */
        NULL, //H5VL_adios_object_open, /* open */
        NULL, // H5VL_adios_object_copy,                /* copy */
        NULL, // H5VL_adios_object_get,                 /* get */
        NULL, // H5VL_adios_object_specific, /* specific */
        NULL  //H5VL_adios_object_optional, /* optional */
    }};

#endif  // ADIOS_VOL_WRITER_H

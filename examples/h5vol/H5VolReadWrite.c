//#define H5VL_FRIEND // junmin added
#include "H5VLpublic.h"
#include "hdf5.h"
#define H5S_FRIEND // suppress error for H5Spkg
//#include "H5Spkg.h" // H5S_hyper_dim
#define H5O_FRIEND // suppress error for H5Opkg

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "H5Epublic.h"
#include "H5VolReadWrite.h"
//#include "error.h"
#include "H5VolUtil.h"
#include <unistd.h> // sleep
// these are in h5private.h
#define H5_ATTR_UNUSED /**/
#define SUCCEED 1
#define FAIL 0

static hid_t H5VL_ADIOS_g = -1;

static adios2_adios *m_ADIOS2 = NULL;
static adios2_io *m_IO = NULL;
static const char *m_ADIOS2Separator = "/";
static int m_MPIRank = 0;

#define RANK_ZERO_MSG(...)                                                     \
    {                                                                          \
        if (0 == m_MPIRank)                                                    \
        {                                                                      \
            fprintf(stderr, "## VOL info:");                                   \
            fprintf(stderr, __VA_ARGS__);                                      \
            fflush(stderr);                                                    \
        }                                                                      \
    }

herr_t H5VL_adios2_begin_read_step(const char *filename)
{
    return H5VL_adios2_beginstep(filename, adios2_step_mode_read);
}

herr_t H5VL_adios2_begin_write_step(const char *filename)
{
    return H5VL_adios2_beginstep(filename, adios2_step_mode_append);
}

herr_t H5VL_adios2_beginstep(const char *filename, adios2_step_mode m)
{
    adios2_engine *engine = adios2_get_engine(m_IO, filename);
    if (NULL == engine)
        return -1;

    adios2_step_status status;
    adios2_begin_step(engine, m, 0.0, &status);

    if (adios2_step_status_end_of_stream == status)
    {
        RANK_ZERO_MSG("..end_of_stream \n");
        return -1;
    }
    else if (adios2_step_status_not_ready == status)
    {
        RANK_ZERO_MSG(".. not ready \n");
        while (adios2_step_status_not_ready == status)
        {
            sleep(1);
            adios2_begin_step(engine, m, 0.0, &status);
        }
        RANK_ZERO_MSG("... other status \n");
        if (adios2_step_status_ok == status)
        {
            return 0;
        }
        return -1;
    }
    else if (adios2_step_status_ok == status)
    {
        RANK_ZERO_MSG(".. stream ready \n");
        return 0;
    }
    return -1;
}

herr_t H5VL_adios2_endstep(const char *filename)
{
    adios2_engine *engine = adios2_get_engine(m_IO, filename);
    if (NULL == engine)
        return -1;

    adios2_end_step(engine);
    return 0;
}

void gInitADIOS2(hid_t acc_tpl)
{
#ifdef ADIOS2_HAVE_MPI
    MPI_Comm comm = MPI_COMM_WORLD;
    if (H5Pget_driver(acc_tpl) == H5FD_MPIO)
    {
        MPI_Info info;
        H5Pget_fapl_mpio(acc_tpl, &comm, &info);
        // int rank;
        MPI_Comm_rank(comm, &m_MPIRank);
    }
    m_ADIOS2 = adios2_init(comm, adios2_debug_mode_on);
#else
    printf(" ... serial ..\n");
    m_ADIOS2 = adios2_init(adios2_debug_mode_on);
#endif
    REQUIRE_NOT_NULL(m_ADIOS2);

    m_IO = adios2_declare_io(m_ADIOS2, "THROUGH_HDF5_VOL");
    REQUIRE_NOT_NULL(m_IO);
}

void gExitADIOS2()
{
    if (NULL == m_ADIOS2)
        return;
    adios2_finalize(m_ADIOS2);
}

H5VL_AttrDef_t *gCreateAttrDef(const char *name, hid_t type_id, hid_t space_id)
{
    H5VL_AttrDef_t *attrDef =
        (H5VL_AttrDef_t *)SAFE_CALLOC(1, sizeof(H5VL_AttrDef_t));
    attrDef->m_Name = (char *)SAFE_CALLOC(strlen(name) + 1, sizeof(char));
    sprintf(attrDef->m_Name, "%s", name);

    attrDef->m_Owner = NULL;

    attrDef->m_TypeID = type_id;

    if (space_id != -1) // most likely from H5Dcreate
        attrDef->m_SpaceID = H5Scopy(space_id);
    else
        printf(".. do something when attr is opened \n");
    return attrDef;
}

H5VL_VarDef_t *gCreateVarDef(const char *name, adios2_engine *engine,
                             adios2_variable *var, hid_t space_id)
{
    H5VL_VarDef_t *varDef =
        (H5VL_VarDef_t *)SAFE_CALLOC(1, sizeof(H5VL_VarDef_t));
    varDef->m_Name = (char *)SAFE_CALLOC(strlen(name) + 1, sizeof(char));
    sprintf(varDef->m_Name, "%s", name);

    varDef->m_Engine = engine;
    varDef->m_Variable = var;
    varDef->m_DimCount = -1; // default: unknown

    if (space_id != -1) // most likely from H5Dcreate
    {
        varDef->m_ShapeID = H5Scopy(space_id);
    }
    else
    { // likely from H5Dopen, so get space info from adios var:
        REQUIRE_NOT_NULL(var);
        size_t nDims;
        if (adios2_error_none != adios2_variable_ndims(&nDims, var))
        {
            free(varDef);
            return NULL;
        }

        varDef->m_DimCount = nDims;

        size_t shape[nDims];
        if (adios2_error_none != adios2_variable_shape(shape, var))
        {
            free(varDef);
            return NULL;
        }

        hid_t filespace = H5Screate_simple(nDims, (hsize_t *)shape, NULL);
        varDef->m_ShapeID = filespace;
    }

    return varDef;
}

void gChooseEngine()
{
    const char *engineType = getenv("ADIOS2_ENGINE");

    if (engineType != NULL)
    {
        if (0 == m_MPIRank)
            printf("  ADIOS2 will apply engine: %s\n", engineType);
        adios2_set_engine(m_IO, engineType);
    }
}
adios2_engine *gADIOS2CreateFile(const char *name)
{
    gChooseEngine();
    return adios2_open(m_IO, name, adios2_mode_write);
}

adios2_engine *gADIOS2OpenFile(const char *name)
{
    gChooseEngine();
    return adios2_open(m_IO, name, adios2_mode_read);
}

void gADIOS2CloseFile(H5VL_ADIOS2_t *handle)
{
    if (NULL == handle)
        return;

    if (NULL != handle->m_Engine)
        adios2_close(handle->m_Engine);

    free(handle);
    handle = NULL;
}

adios2_variable *gADIOS2InqVar(const char *name)
{
    return adios2_inquire_variable(m_IO, name);
}

herr_t gADIOS2ReadVar(H5VL_VarDef_t *varDef)
{
    REQUIRE_NOT_NULL(varDef);
    REQUIRE_NOT_NULL(varDef->m_Variable);

    int varDim = varDef->m_DimCount;
    if (varDim < 0)
        return -1;

    size_t start[varDim], count[varDim];
    if (H5VL_CODE_FAIL ==
        gUtilADIOS2GetBlockInfo(varDef->m_HyperSlabID, start, count, varDim))
        return -1;

    adios2_set_selection(varDef->m_Variable, varDef->m_DimCount, start, count);

    adios2_get(varDef->m_Engine, varDef->m_Variable, varDef->m_Data,
               adios2_mode_sync);

    return 0;
}

adios2_variable *gADIOS2CreateVar(H5VL_VarDef_t *varDef)
{
    adios2_variable *variable = adios2_inquire_variable(m_IO, varDef->m_Name);
    if (NULL == variable)
    {
        adios2_type varType = gUtilADIOS2Type(varDef->m_TypeID);

        size_t varDim = 1; // gUtilGetSpaceInfo(;

        varDim = gUtilADIOS2GetDim(varDef->m_ShapeID);
        if (0 == varDim)
        { //  scalar
            variable = adios2_define_variable(m_IO, varDef->m_Name, varType,
                                              varDim, NULL, NULL, NULL,
                                              adios2_constant_dims_true);
        }
        else
        {
            varDef->m_DimCount = varDim;

            size_t shape[varDim];
            gUtilADIOS2GetShape(varDef->m_ShapeID, shape, varDim);

            size_t start[varDim], count[varDim];
            if (H5VL_CODE_FAIL == gUtilADIOS2GetBlockInfo(varDef->m_HyperSlabID,
                                                          start, count, varDim))
                return NULL;

            variable = adios2_define_variable(m_IO, varDef->m_Name, varType,
                                              varDim, shape, start, count,
                                              adios2_constant_dims_true);
        }
    }
    adios2_put(varDef->m_Engine, variable, varDef->m_Data,
               adios2_mode_deferred);

    return variable;
}

adios2_attribute *gADIOS2CreateAttr(H5VL_AttrDef_t *input)
{
    adios2_type attrType = gUtilADIOS2Type(input->m_TypeID);

    size_t attrDim = 0;

    if (gUtilADIOS2IsScalar(input->m_SpaceID))
    {
        if (NULL == input->m_Owner)
            return adios2_define_attribute(m_IO, input->m_Name, attrType,
                                           input->m_Data);
        else
            return adios2_define_variable_attribute(
                m_IO, input->m_Name, attrType, input->m_Data,
                input->m_Owner->m_Name, m_ADIOS2Separator);
    }
    else
    {
        attrDim = gUtilADIOS2GetDim(input->m_SpaceID);

        if (1 != attrDim)
        {
            printf("Unable to support 2+D arrays  in ADIOS2 attributes. Use "
                   "Vars instead.");
            return NULL;
        }

        size_t shape[attrDim];
        gUtilADIOS2GetShape(input->m_SpaceID, shape, attrDim);

        if (NULL == input->m_Owner)
            return adios2_define_attribute_array(m_IO, input->m_Name, attrType,
                                                 input->m_Data, shape[0]);
        else
            return adios2_define_variable_attribute_array(
                m_IO, input->m_Name, attrType, input->m_Data, shape[0],
                input->m_Owner->m_Name, m_ADIOS2Separator);
    }

    return NULL;
}

/*
static int h5i_search_func(void *obj, hid_t id, void *key) {
  if (key == obj) {
    printf("id = %llu\n", id);
    return 1;
  } else
    return 0;
}
*/
//
// either "ts_number" or ".../ts_number"
// is a valid dataset name
//
static int isDatasetName(const char *name)
{
    int len = strlen(name);
    while (true)
    {
        char c = name[len - 1];
        if (c >= '0' && c <= '9')
        {
            len--;
            if (len == 0)
            {
                return 1;
            }
        }
        else if ((len < strlen(name)) && (c == '/'))
        {
            return 1;
        }
        else
        {
            break;
        }
    }
    return 0;
}

hid_t H5VL_adios_register(void)
{
    if (H5I_VOL != H5Iget_type(H5VL_ADIOS_g))
    {

        H5VL_ADIOS_g = H5VLregister_connector(&H5VL_adios2_def, H5P_DEFAULT);
        if (H5VL_ADIOS_g <= 0)
        {
            printf("  [ECP ADIOS VOL ERROR] with H5VLregister_connector\n");
            return -1;
        }
    }

    return H5VL_ADIOS_g;
}

/*
herr_t H5Pset_fapl_adios(hid_t *acc_tpl, hid_t *fapl_id) {
  //hid_t plugin_id = H5VLget_driver_id(VOLNAME);
  //H5VLinitialize(plugin_id, H5P_DEFAULT);
  //H5VLclose(plugin_id);

  if (H5VL_adios_register() < 0)
    return -1;


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
*/
herr_t H5P_unset_adios() { return H5VLunregister_connector(H5VL_ADIOS_g); }

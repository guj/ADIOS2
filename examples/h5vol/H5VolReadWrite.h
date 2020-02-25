#ifndef ADIOS_VOL_WRITER_H
#define ADIOS_VOL_WRITER_H

//#define H5VL_FRIEND // junmin added
#define H5F_FRIEND /*suppress error about including H5Fpkg   */

//#include "H5Fpkg.h"
#include "hdf5.h"

#include "H5VolError.h"
#include "mpi.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <adios2_c.h>

// Define the values for the first entries of
// VOL struct H5VL_class_t
#define H5VL_ADIOS2_NAME "ADIOS_VOL"
#define H5VL_ADIOS2_VALUE 511 /* VOL connector ID */
#define H5VL_ADIOS2_VERSION 0

typedef struct H5VL_ADIOS2_t
{
    adios2_engine *m_Engine;
} H5VL_ADIOS2_t;

typedef struct H5VL_VARDef_t
{
    char *m_Name;
    hid_t m_ShapeID;
    hid_t m_TypeID;
    hid_t m_MemSpaceID;
    hid_t m_HyperSlabID;
    hid_t m_PropertyID;
    void *m_Data; // used for both write & read so not going to be const
    adios2_engine *m_Engine;
    adios2_variable *m_Variable; // mainly for read reuse
    size_t m_DimCount;
} H5VL_VarDef_t;

typedef struct H5VL_ATTRDef_t
{
    char *m_Name;
    hid_t m_SpaceID;
    hid_t m_TypeID;
    H5VL_VarDef_t *m_Owner;
    const void *m_Data;
    adios2_attribute *m_Attribute; // for read reuse
} H5VL_AttrDef_t;

typedef struct H5VL_GROUPDef_t
{
    char *m_FullPath;
} H5VL_GroupDef_t;

H5VL_VarDef_t *gCreateVarDef(const char *name, adios2_engine *engine,
                             adios2_variable *var, hid_t space_id);

H5VL_AttrDef_t *gCreateAttrDef(const char *name, hid_t typeid, hid_t space_id);
/*
 */

extern void gInitADIOS2(hid_t acc_tpl);
extern void gExitADIOS2();

extern adios2_attribute *gADIOS2CreateAttr(H5VL_AttrDef_t *input);
extern adios2_engine *gADIOS2CreateFile(const char *name);
extern adios2_engine *gADIOS2OpenFile(const char *name);
extern void gADIOS2CloseFile(H5VL_ADIOS2_t *handle);
extern adios2_variable *gADIOS2CreateVar(H5VL_VarDef_t *var);
extern adios2_variable *gADIOS2InqVar(const char *name);
extern herr_t gADIOS2ReadVar(H5VL_VarDef_t *var);

//
// VL definition
//
static herr_t H5VL_adios2_init(hid_t vipl_id) { return 0; }

static herr_t H5VL_adios2_term(void)
{
    gExitADIOS2();
    return 0;
}

extern herr_t H5VL_adios2_begin_read_step(const char *);
extern herr_t H5VL_adios2_begin_write_step(const char *);

extern herr_t H5VL_adios2_beginstep(const char *engine_name,
                                    adios2_step_mode m);

extern herr_t H5VL_adios2_endstep(const char *engine_nane);

//
// attribute handlers
//
static void *H5VL_adios2_attr_create(void *obj,
                                     const H5VL_loc_params_t *loc_params,
                                     const char *name, hid_t type_id,
                                     hid_t space_id, hid_t acpl_id,
                                     hid_t aapl_id, hid_t dxpl_id, void **req)
{
    REQUIRE_NOT_NULL(loc_params);
    REQUIRE_NOT_NULL(obj);

    // just a none null pointer to return; need to actaully use var handle
    if (H5I_FILE == loc_params->obj_type)
    {
        H5VL_ADIOS2_t *handle = (H5VL_ADIOS2_t *)obj;
        H5VL_AttrDef_t *attrDef = gCreateAttrDef(name, type_id, space_id);
        attrDef->m_Owner = NULL;
        return attrDef;
    }
    else if (H5I_DATASET == loc_params->obj_type)
    {
        H5VL_VarDef_t *varDef = (H5VL_VarDef_t *)obj;
        H5VL_AttrDef_t *attrDef = gCreateAttrDef(name, type_id, space_id);
        attrDef->m_Owner = varDef;
        return attrDef;
    }
    else if (H5I_GROUP == loc_params->obj_type)
    {
        printf(
            "TODO: Not clear whether to support creating attr for a group \n");
        return NULL;
    }
    return NULL;
}

static void *H5VL_adios2_attr_open(void *obj,
                                   const H5VL_loc_params_t *loc_params,
                                   const char *name, hid_t aapl_id,
                                   hid_t dxpl_id, void **req)
{
    printf("  need to think it over about reading attr from file/group/var");
    return NULL;
}

static herr_t H5VL_adios2_attr_read(void *attr, hid_t mem_type_id, void *buf,
                                    hid_t dxpl_id, void **req)
{
    printf("  need to think it over about reading attr from file/group/var");
    return -1;
}

static herr_t H5VL_adios2_attr_write(void *attr, hid_t mem_type_id,
                                     const void *buf, hid_t dxpl_id, void **req)
{
    REQUIRE_NOT_NULL(attr);
    H5VL_AttrDef_t *attrDef = (H5VL_AttrDef_t *)attr;
    attrDef->m_Data = (void *)buf;
    gADIOS2CreateAttr(attrDef);
    return 0;
}

static herr_t H5VL_adios2_attr_get(void *obj, H5VL_attr_get_t get_type,
                                   hid_t dxpl_id, void **req, va_list arguments)
{
    return 0;
}

// static herr_t H5VL_adios2_attr_optional(void* obj, hid_t dxpl_id, void **req,
// va_list arguments);

static herr_t H5VL_adios2_attr_close(void *attr, hid_t dxpl_id, void **req)
{
    if (NULL == attr)
        return 0;

    H5VL_AttrDef_t *attrDef = (H5VL_AttrDef_t *)attr;
    free(attrDef->m_Name);
    H5Sclose(attrDef->m_SpaceID);
    free(attrDef);
    attrDef = NULL;

    return 0;
}

static herr_t H5VL_adios2_attr_specific(void *obj,
                                        const H5VL_loc_params_t *loc_params,
                                        H5VL_attr_specific_t specific_type,
                                        hid_t dxpl_id, void **req,
                                        va_list arguments)
{
    printf(" doing nothing at  attr_spec/");
    return 0;
}

//
// NOTE: this is called from H5F.c when a new file or  trunc file is asked
//       so no need to check flags here. if do need to, use & not ==
//
static void *H5VL_adios2_file_create(const char *name, unsigned flags,
                                     hid_t fcpl_id, hid_t fapl_id,
                                     hid_t dxpl_id, void **req)
{
    H5VL_ADIOS2_t *handle = NULL;

    if (flags & H5F_ACC_TRUNC)
    {
        handle = (H5VL_ADIOS2_t *)SAFE_CALLOC(1, sizeof(H5VL_ADIOS2_t));
        handle->m_Engine = gADIOS2CreateFile(name);
    }

    return (void *)handle;
}

static void *H5VL_adios2_file_open(const char *name, unsigned flags,
                                   hid_t fapl_id, hid_t dxpl_id, void **req)
{
    H5VL_ADIOS2_t *handle = NULL;
    handle = (H5VL_ADIOS2_t *)SAFE_CALLOC(1, sizeof(H5VL_ADIOS2_t));
    handle->m_Engine = gADIOS2OpenFile(name);
    return handle;
}

static herr_t H5VL_adios2_file_specific(void *file,
                                        H5VL_file_specific_t specific_type,
                                        hid_t dxpl_id, void **req,
                                        va_list arguments)
{
    //
    // This function is called after H5Fopen/create. Do not remove
    //
    return 0;
}

static herr_t H5VL_adios2_file_close(void *file, hid_t dxpl_id, void **req)
{
    H5VL_ADIOS2_t *handle = (H5VL_ADIOS2_t *)file;
    gADIOS2CloseFile(handle);
    handle = NULL;
    return 0;
}

static void *H5VL_adios2_group_create(void *obj,
                                      const H5VL_loc_params_t *loc_params,
                                      const char *name, hid_t lcpl_id,
                                      hid_t gcpl_id, hid_t gapl_id,
                                      hid_t dxpl_id, void **req)
{
    if (H5I_GROUP == loc_params->obj_type)
    {
        H5VL_GroupDef_t *parent = (H5VL_GroupDef_t *)obj;
        H5VL_GroupDef_t *grp =
            (H5VL_GroupDef_t *)SAFE_CALLOC(1, sizeof(H5VL_GroupDef_t));
        grp->m_FullPath = (char *)SAFE_CALLOC(
            strlen(name) + 1 + strlen(parent->m_FullPath), sizeof(char));
        sprintf(grp->m_FullPath, "%s/%s", parent->m_FullPath, name);
        return grp;
    }
    else if (H5I_FILE == loc_params->obj_type)
    {
        // H5VL_ADIOS2_t *handle = (H5VL_ADIOS2_t*)obj;
        H5VL_GroupDef_t *grp =
            (H5VL_GroupDef_t *)SAFE_CALLOC(1, sizeof(H5VL_GroupDef_t));
        grp->m_FullPath = (char *)SAFE_CALLOC(strlen(name) + 1, sizeof(char));
        sprintf(grp->m_FullPath, "%s", name);
        return grp;
    }

    return NULL;
}

static herr_t H5VL_adios2_group_close(void *obj, hid_t dxpl_id, void **req)
{
    if (NULL == obj)
        return 0;

    H5VL_GroupDef_t *grp = (H5VL_GroupDef_t *)obj;
    free(grp->m_FullPath);
    free(grp);
    grp = NULL;
    return 0;
}

static void *H5VL_adios2_group_open(void *obj,
                                    const H5VL_loc_params_t *loc_params,
                                    const char *varName, hid_t gapl_id,
                                    hid_t dxpl_id, void **req)
{
    if (H5I_GROUP == loc_params->obj_type)
    {
        printf("... open head is group. ");
    }
    else if (H5I_FILE == loc_params->obj_type)
    {
        printf("... open head is file. ");
    }

    return NULL;
}

static void *H5VL_adios2_dataset_create(void *obj,
                                        const H5VL_loc_params_t *loc_params,
                                        const char *name, hid_t lcpl_id,
                                        hid_t type_id, hid_t space_id,
                                        hid_t dcpl_id, hid_t dapl_id,
                                        hid_t dxpl_id, void **req)
{
    REQUIRE_NOT_NULL(loc_params);

    // just a none null pointer to return; need to actaully use var handle
    if (H5I_FILE == loc_params->obj_type)
    {
        H5VL_ADIOS2_t *handle = (H5VL_ADIOS2_t *)obj;
        REQUIRE_NOT_NULL(handle);

        H5VL_VarDef_t *varDef =
            gCreateVarDef(name, handle->m_Engine, NULL, space_id);
        varDef->m_TypeID = type_id;

        return varDef;
    }
    else if (H5I_GROUP == loc_params->obj_type)
    {
        printf("TODO: create var in a group \n");
        return NULL;
    }
    return NULL;
}

static void *H5VL_adios2_dataset_open(void *obj,
                                      const H5VL_loc_params_t *loc_params,
                                      const char *name, hid_t dapl_id,
                                      hid_t dxpl_id, void **req)
{
    REQUIRE_NOT_NULL(loc_params);

    // just a none null pointer to return; need to actaully use var handle
    if (H5I_FILE == loc_params->obj_type)
    {
        H5VL_ADIOS2_t *handle = (H5VL_ADIOS2_t *)obj;
        REQUIRE_NOT_NULL(handle);

        adios2_variable *var = gADIOS2InqVar(name);
        if (NULL == var)
        {
            printf("H5VL_ADIOS2: Error: No such variable: %s in file\n ", name);
            return NULL;
        }

        H5VL_VarDef_t *varDef = gCreateVarDef(name, handle->m_Engine, var, -1);

        return varDef;
    }
    else if (H5I_GROUP == loc_params->obj_type)
    {
        printf("TODO: open var in a group \n");
        return NULL;
    }
    return NULL;
}

static herr_t H5VL_adios2_dataset_read(void *dset, hid_t mem_type_id,
                                       hid_t mem_space_id, hid_t file_space_id,
                                       hid_t plist_id, void *buf, void **req)
{
    H5VL_VarDef_t *var = (H5VL_VarDef_t *)dset;
    var->m_HyperSlabID = file_space_id;
    var->m_MemSpaceID = mem_space_id;
    var->m_Data = buf;
    return gADIOS2ReadVar(var);
}

static herr_t H5VL_adios2_dataset_get(void *dset, H5VL_dataset_get_t get_type,
                                      hid_t dxpl_id, void **req,
                                      va_list arguments)
{
    H5VL_VarDef_t *varDef = (H5VL_VarDef_t *)dset;
    REQUIRE_NOT_NULL(varDef);

    switch (get_type)
    {
    case H5VL_DATASET_GET_SPACE:
    {
        hid_t *ret_id = va_arg(arguments, hid_t *);
        *ret_id = H5Scopy(varDef->m_ShapeID);
        REQUIRE_SUCC_MSG((*ret_id >= 0), -1,
                         "H5VOL-ADIOS2: Unable to get space id.");
        break;
    }
    default:
    {
        printf("... todo: handle dataset get properties \n");
        return -1;
    }
    }
    return 0;
}

static herr_t H5VL_adios2_dataset_write(void *dset, hid_t mem_type_id,
                                        hid_t mem_space_id, hid_t file_space_id,
                                        hid_t plist_id, const void *buf,
                                        void **req)
{
    H5VL_VarDef_t *varDef = (H5VL_VarDef_t *)dset;
    varDef->m_Data = (void *)buf;
    varDef->m_MemSpaceID = mem_space_id;
    varDef->m_HyperSlabID = file_space_id;
    varDef->m_PropertyID = plist_id;

    gADIOS2CreateVar(varDef);
    return 0;
}

static herr_t H5VL_adios2_dataset_close(void *dset, hid_t dxpl_id, void **req)
{
    if (NULL == dset)
        return 0;

    H5VL_VarDef_t *varDef = (H5VL_VarDef_t *)dset;
    free(varDef->m_Name);
    H5Sclose(varDef->m_ShapeID);
    free(varDef);
    varDef = NULL;
    return 0;
}

static const H5VL_class_t H5VL_adios2_def = {
    H5VL_ADIOS2_VERSION,
    (H5VL_class_value_t)H5VL_ADIOS2_VALUE,
    H5VL_ADIOS2_NAME, /* name */
    0,
    H5VL_adios2_init, /* initialize */
    H5VL_adios2_term, /* terminate */
    {
        /* info_cls */
        (size_t)0, /* info size    */
        NULL,      /* info copy    */
        NULL,      /* info compare */
        NULL,      /* info free    */
        NULL,      /* info to str  */
        NULL       /* str to info  */
    },
    {
        /* wrap_cls */
        NULL, /* get_object   */
        NULL, /* get_wrap_ctx */
        NULL, /* wrap_object  */
        NULL, /* unwrap_object */
        NULL  /* free_wrap_ctx */
    },
    {/* attribute_cls */
     H5VL_adios2_attr_create, H5VL_adios2_attr_open, H5VL_adios2_attr_read,
     H5VL_adios2_attr_write, H5VL_adios2_attr_get, H5VL_adios2_attr_specific,
     NULL, // H5VL_adios2_attr_optional,  /*optional*/
     H5VL_adios2_attr_close},
    {
        /* dataset_cls */
        H5VL_adios2_dataset_create, H5VL_adios2_dataset_open,
        H5VL_adios2_dataset_read, H5VL_adios2_dataset_write,
        H5VL_adios2_dataset_get,  /* get properties*/
        NULL,                     // H5VL_adios_dataset_specific
        NULL,                     // optional
        H5VL_adios2_dataset_close /* close */
    },
    {
        /* datatype_cls */
        NULL, // H5VL_adios_datatype_commit,           /* commit */
        NULL, // H5VL_adios_datatype_open,             /* open */
        NULL, // H5VL_adios_datatype_get, /* get_size */
        NULL  // H5VL_adios_datatype_close /* close */
    },
    {/* file_cls */
     H5VL_adios2_file_create, H5VL_adios2_file_open,
     NULL, // H5VL_adios_file_get,  /* get properties, objs etc*/
     H5VL_adios2_file_specific,
     NULL, // H5VL_adios_file_optional, /* get file size etc */
     H5VL_adios2_file_close},
    {
        /* group_cls */
        H5VL_adios2_group_create, H5VL_adios2_group_open,
        NULL,                   // H5VL_adios_group_get,  /* get  */
        NULL,                   /* specific */
        NULL,                   /* optional */
        H5VL_adios2_group_close /* close */
    },
    {
        /* link_cls */
        NULL, // H5VL_adios_link_create,                /* create */
        NULL, // H5VL_adios_link_copy,                  /* copy   */
        NULL, // H5VL_adios_link_move,                  /* move   */
        NULL, // H5VL_adios_link_get,      /* get    */
        NULL, // H5VL_adios_link_specific, /* iterate etc*/
        NULL  // H5VL_adios_link_remove                 /* remove */
    },
    {
        /* object_cls */
        NULL, // H5VL_adios_object_open, /* open */
        NULL, // H5VL_adios_object_copy,                /* copy */
        NULL, // H5VL_adios_object_get,                 /* get */
        NULL, // H5VL_adios_object_specific, /* specific */
        NULL  // H5VL_adios_object_optional, /* optional */
    }};

#endif // ADIOS_VOL_WRITER_H

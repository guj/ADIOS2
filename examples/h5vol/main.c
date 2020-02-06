/*
 */

//#include "hdf5.h"
//#include "H5VolWriter.h"
#include "H5Vol.h"

#include "stdlib.h"

#define VARNAME1 "Var1"
#define VARNAME2 "Var2"
#define NX 32 /* dataset dimensions */
#define NY 32
#define DIM 2
#define LAZYRANK 3
#define NUM_STEPS 3

void AttrTests(hid_t owner_id)
{
    { // test attributes
        printf("\n\t=> Now write a 1D data array attr \n");
        hid_t a1_sid = H5Screate(H5S_SIMPLE);
        hsize_t a1_dim[] = {4};
        hid_t ret = H5Sset_extent_simple(a1_sid, 1, a1_dim, NULL);
        hid_t attr1 = H5Acreate(owner_id, "TopAttr1DFloat", H5T_NATIVE_FLOAT,
                                a1_sid, H5P_DEFAULT, H5P_DEFAULT);
        float a1Value[4] = {0.1, 0.2, 0.3, 0.04};
        H5Awrite(attr1, H5T_NATIVE_FLOAT, a1Value);
        H5Sclose(a1_sid);
        H5Aclose(attr1);

        printf("\n\t=> Now write a scalar attr \n");
        int iValue = 9;
        hid_t a2_sid = H5Screate(H5S_SCALAR);
        hid_t attr2 = H5Acreate(owner_id, "TopAttrInt", H5T_NATIVE_INT, a2_sid,
                                H5P_DEFAULT, H5P_DEFAULT);
        H5Awrite(attr2, H5T_NATIVE_INT, &iValue);
        H5Sclose(a2_sid);
        H5Aclose(attr2);

        printf("\nAttempt to create a 2D attr,  and expect to FAIL\n");
        hid_t a3_sid = H5Screate(H5S_SIMPLE);
        hsize_t a3_dim[] = {2, 2};
        ret = H5Sset_extent_simple(a3_sid, 2, a1_dim, NULL);
        hid_t attr3 = H5Acreate(owner_id, "TopAttr2DFloat", H5T_NATIVE_FLOAT,
                                a3_sid, H5P_DEFAULT, H5P_DEFAULT);
        float a3Value[2][2] = {0.1, 0.2, 0.3, 0.05};
        H5Awrite(attr3, H5T_NATIVE_FLOAT, a3Value);
        H5Sclose(a3_sid);
        H5Aclose(attr3);

        printf("\n\t=> Now write a string attr \n");
        hid_t a4_sid = H5Screate(H5S_SCALAR);
        hid_t atype = H5Tcopy(H5T_C_S1);
        H5Tset_size(atype, 7);
        H5Tset_strpad(atype, H5T_STR_NULLTERM);
        hid_t attr4 = H5Acreate(owner_id, "TopAttrString", atype, a4_sid,
                                H5P_DEFAULT, H5P_DEFAULT);
        H5Awrite(attr4, atype, "string");
        H5Sclose(a4_sid);
        H5Aclose(attr4);

        printf("\n\t=> Now write a char attr \n");
        hid_t a5_sid = H5Screate(H5S_SCALAR);
        hid_t attr5 = H5Acreate(owner_id, "TopAttrChar", H5T_NATIVE_CHAR,
                                a5_sid, H5P_DEFAULT, H5P_DEFAULT);
        H5Awrite(attr5, H5T_NATIVE_CHAR, "a");
        H5Sclose(a5_sid);
        H5Aclose(attr5);

        printf("\n\t=> Now write a string array attr \n");
        hid_t a6_sid = H5Screate(H5S_SIMPLE);
        hsize_t a6_dim[] = {4};
        ret = H5Sset_extent_simple(a6_sid, 1, a6_dim, NULL);
        hid_t str = H5Tcopy(H5T_C_S1);
        H5Tset_size(str, H5T_VARIABLE);
        H5Tset_strpad(str, H5T_STR_NULLTERM);
        hid_t attr6 = H5Acreate(owner_id, "TopAttrStringArray", str, a6_sid,
                                H5P_DEFAULT, H5P_DEFAULT);
        char *strArray[4] = {"Never", "let", "passion go", "away"};

        H5Awrite(attr6, str, strArray);
        H5Sclose(a6_sid);
        H5Aclose(attr6);
    }
}

int TestW_1(const char *filename, MPI_Comm comm, MPI_Info info, int mpi_size,
            int mpi_rank)
{
    if (mpi_rank == 0)
        printf("===> Testing one var write <=== %s\n", filename);
    /*
     * HDF5 APIs definitions
     */
    hid_t file_id, dset_id;
    hid_t filespace, memspace;
    hsize_t dimsf[2];
    int *data;

    hsize_t count[2]; /* hyperslab selection parameters */
    hsize_t offset[2];

    hid_t plist_id; /* property list identifier */
    int i;
    herr_t status;

    /*
     * Set up file access property list with parallel I/O access
     */
    plist_id = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_fapl_mpio(plist_id, comm, info);
#ifdef NATIVE
#else
    H5VL_ADIOS2_set(plist_id);
#endif
    file_id = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, plist_id);
    H5Pclose(plist_id);

    if (file_id < 0)
        return -1;

    AttrTests(file_id);

    //
    // define dataset, will be used in the timesteps next
    //
    dimsf[0] = NX;
    dimsf[1] = NY;
    filespace = H5Screate_simple(DIM, dimsf, NULL);
    dset_id = H5Dcreate(file_id, VARNAME1, H5T_NATIVE_INT, filespace,
                        H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Sclose(filespace);

    //
    // loop of timesteps
    //
    int k = 0;
    for (k = 0; k < NUM_STEPS; k++)
    {
        H5VL_adios2_begin_write_step(filename);
        //
        // empty groups
        //
        hid_t gid =
            H5Gcreate(file_id, "Group1", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        hid_t gid2 =
            H5Gcreate(gid, "SubG1", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        H5Gclose(gid2);
        H5Gclose(gid);

        // Each process defines dataset in memory and writes it to the hyperslab
        // in the file.
        count[0] = dimsf[0] / mpi_size;
        count[1] = dimsf[1];
        offset[0] = mpi_rank * count[0];
        offset[1] = 0;

        memspace = H5Screate_simple(DIM, count, NULL);
        if (mpi_rank == LAZYRANK)
            H5Sselect_none(memspace);

        // Select hyperslab in the file.
        filespace = H5Dget_space(dset_id);
        H5Sselect_hyperslab(filespace, H5S_SELECT_SET, offset, NULL, count,
                            NULL);
        if (mpi_rank == LAZYRANK)
            H5Sselect_none(filespace);

        // Initialize data buffer
        if (mpi_rank == LAZYRANK)
        {
            data = NULL;
        }
        else
        {
            data = (int *)malloc(sizeof(int) * count[0] * count[1]);
            for (i = 0; i < count[0] * count[1]; i++)
            {
                data[i] = mpi_rank + 10 + 100 * k;
            }
        }

        // Create property list for collective dataset write.
        plist_id = H5Pcreate(H5P_DATASET_XFER);
        H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);

        status = H5Dwrite(dset_id, H5T_NATIVE_INT, memspace, filespace,
                          plist_id, data);
        if (mpi_rank != LAZYRANK)
            free(data);

        H5VL_adios2_endstep(filename);

        if (0 == mpi_rank)
            printf("...wrote step %d for .. %s\n", k, VARNAME1);

        // adios2 attributes are global. so only need to write attr of var once
        if (k == 0)
            AttrTests(dset_id);
    } // timestep finished

    /*
     * Close/release resources.
     */
    H5Dclose(dset_id);
    H5Sclose(filespace);
    H5Sclose(memspace);
    H5Pclose(plist_id);
    H5Fclose(file_id);

    return 0;
}

int TestW_2(const char *filename, MPI_Comm comm, MPI_Info info, int mpi_size,
            int mpi_rank)
{
    if (mpi_rank == 0)
        printf("===> Testing two vars write <=== %s\n", filename);

    /*
     * HDF5 APIs definitions
     */
    hid_t file_id;
    hid_t dset_id_1, dset_id_2;
    hid_t filespace1, filespace2;
    hid_t memspace;                // memspace2;
    hsize_t dimsf[2];              /* dataset dimensions */
    int *data;                     /* pointer to data buffer to write */
    hsize_t count1[2], offset1[2]; /* hyperslab selection parameters */
    hsize_t count2[1], offset2[1]; /* hyperslab selection parameters */
    hid_t plist_id;                /* property list identifier */
    int i;
    herr_t status;

    /*
     * Set up file access property list with parallel I/O access
     */
    plist_id = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_fapl_mpio(plist_id, comm, info);
#ifdef NATIVE
#else
    H5VL_ADIOS2_set(plist_id);
#endif
    file_id = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, plist_id);
    H5Pclose(plist_id);

    if (file_id < 0)
        return -1;

    {
        hid_t scalarSpace = H5Screate(H5S_SCALAR);
        hid_t scalar_did =
            H5Dcreate(file_id, "scalarVar", H5T_NATIVE_INT, scalarSpace,
                      H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        int scalarVal = 919;
        status = H5Dwrite(scalar_did, H5T_NATIVE_INT, H5P_DEFAULT, scalar_did,
                          H5P_DEFAULT, &scalarVal);
        H5Sclose(scalarSpace);
        H5Dclose(scalar_did);
    }
    dimsf[0] = NX;
    dimsf[1] = NY;
    filespace1 = H5Screate_simple(DIM, dimsf, NULL);
    hsize_t dims1D[1];
    dims1D[0] = NX * NY;
    filespace2 = H5Screate_simple(1, dims1D, NULL);

    /*
     * Create the dataset with default properties and close filespace.
     */
    dset_id_1 = H5Dcreate(file_id, VARNAME1, H5T_NATIVE_INT, filespace1,
                          H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    dset_id_2 = H5Dcreate(file_id, VARNAME2, H5T_NATIVE_INT, filespace2,
                          H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Sclose(filespace1);
    H5Sclose(filespace2);

    /*
     * Each process defines dataset in memory and writes it to the hyperslab
     * in the file.
     */
    count1[0] = dimsf[0] / mpi_size;
    count1[1] = dimsf[1];
    offset1[0] = mpi_rank * count1[0];
    offset1[1] = 0;
    memspace = H5Screate_simple(DIM, count1, NULL);
    if (mpi_rank == LAZYRANK)
        H5Sselect_none(memspace);

    /*
     * Select hyperslab in the file.
     */
    filespace1 = H5Dget_space(dset_id_1);
    H5Sselect_hyperslab(filespace1, H5S_SELECT_SET, offset1, NULL, count1,
                        NULL);
    if (mpi_rank == LAZYRANK)
        H5Sselect_none(filespace1);

    count2[0] = dims1D[0] / mpi_size;
    offset2[0] = mpi_rank * count2[0];
    filespace2 = H5Dget_space(dset_id_2);
    H5Sselect_hyperslab(filespace2, H5S_SELECT_SET, offset2, NULL, count2,
                        NULL);
    if (mpi_rank == LAZYRANK)
        H5Sselect_none(filespace2);

    /*
     * Initialize data buffer
     */
    if (mpi_rank == LAZYRANK)
    {
        data = NULL;
    }
    else
    {
        data = (int *)malloc(sizeof(int) * count1[0] * count1[1]);
        for (i = 0; i < count1[0] * count1[1]; i++)
        {
            data[i] = mpi_rank + 10;
        }
    } /*else*/

    /*
     * Create property list for collective dataset write.
     */
    plist_id = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);

    status = H5Dwrite(dset_id_1, H5T_NATIVE_INT, memspace, filespace1, plist_id,
                      data);
    status = H5Dwrite(dset_id_2, H5T_NATIVE_INT, memspace, filespace2, plist_id,
                      data);
    if (mpi_rank != LAZYRANK)
        free(data);

    /*
     * Close/release resources.
     */
    H5Dclose(dset_id_1);
    H5Sclose(filespace1);
    H5Sclose(memspace);
    H5Pclose(plist_id);
    H5Fclose(file_id);

    return 0;
}

int TestR_1(const char *filename, MPI_Comm comm, MPI_Info info, int mpi_size,
            int mpi_rank)
{
    if (mpi_rank == 0)
        printf("===> Testing read <===  %s\n", filename);

    hid_t plist_id = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_fapl_mpio(plist_id, comm, info);
#ifdef NATIVE
#else
    H5VL_ADIOS2_set(plist_id);
#endif

    /* open the file collectively */
    hid_t fid = H5Fopen(filename, H5F_ACC_RDWR, plist_id);
    H5Pclose(plist_id);
    if (fid == -1)
        return -1;

    while (true)
    {
        herr_t status = H5VL_adios2_begin_read_step(filename);
        if (0 != status)
            break;

        /* open the dataset1 collectively */
        hid_t dataset1 = H5Dopen2(fid, VARNAME1, H5P_DEFAULT);
        if (dataset1 == -1)
            return -1;

        /* set up dimensions of the slab this process accesses */
        hsize_t start[2], count[2], stride[2];

        start[0] = mpi_rank * NX / mpi_size;
        start[1] = 0;
        count[0] = NX / mpi_size;
        count[1] = NY;
        stride[0] = 1;
        stride[1] = 1;

        /* create a file dataspace independently */
        hid_t file_dataspace = H5Dget_space(dataset1);
        if (file_dataspace == -1)
            return -1;

        H5Sselect_hyperslab(file_dataspace, H5S_SELECT_SET, start, stride,
                            count, NULL);

        /* create a memory dataspace independently */
        hid_t mem_dataspace = H5Screate_simple(DIM, count, NULL);

        hid_t xfer_plist = H5P_DEFAULT;
#ifdef MPIO
        xfer_plist = H5Pcreate(H5P_DATASET_XFER);
        assert(xfer_plist != -1);
        ret = H5Pset_dxpl_mpio(xfer_plist, H5FD_MPIO_COLLECTIVE);
        assert(ret != -1);
#endif
        int data_array1[NX / mpi_size]
                       [NY]; /* data buffer, we knew it was integer */
        int i, j;
        for (i = 0; i < NX / mpi_size; i++)
            for (j = 0; j < NY; j++)
                data_array1[i][j] = -1; // init

        /* read data independently or collectively */
        hid_t ret = H5Dread(dataset1, H5T_NATIVE_INT, mem_dataspace,
                            file_dataspace, xfer_plist, data_array1);
        if (ret == -1)
            return -1;

#ifndef PRINTING
        for (i = 0; i < NX / mpi_size; i++)
        {
            printf("rank:%d row:%d[", mpi_rank, i);

            for (j = 0; j < NY; j++)
                if ((j < 3) || (j > NY - 3))
                    printf("%d ", data_array1[i][j]);
                else
                    printf(".");

            printf("]\n");
        }
#endif
        H5VL_adios2_endstep(filename);
        if (H5P_DEFAULT != xfer_plist)
            H5Pclose(xfer_plist);

        H5Sclose(mem_dataspace);
        H5Sclose(file_dataspace);

        H5Dclose(dataset1);
    } // while
    H5Fclose(fid);

    return 0;
}
int main(int argc, char **argv)
{
    /*
     * MPI variables
     */
    int mpi_size, mpi_rank;
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Info info = MPI_INFO_NULL;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(comm, &mpi_size);
    MPI_Comm_rank(comm, &mpi_rank);

    const char *file1Name = "t1";
    const char *file2Name = "t2";

    if (argc == 1)
    {
        TestW_1(file1Name, comm, info, mpi_size, mpi_rank);
        TestW_2(file2Name, comm, info, mpi_size, mpi_rank);

        TestR_1(file1Name, comm, info, mpi_size, mpi_rank);
        TestR_1(file2Name, comm, info, mpi_size, mpi_rank);
    }
    if (argc > 1)
    {
        if (argv[1][0] == 'w')
            TestW_1(file2Name, comm, info, mpi_size, mpi_rank);
        else if (argv[1][0] == 'r')
            TestR_1(file2Name, comm, info, mpi_size, mpi_rank);
        else
            TestW_2(file1Name, comm, info, mpi_size, mpi_rank);
    }
    MPI_Finalize();
}

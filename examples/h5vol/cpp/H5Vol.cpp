#include "H5Vol.h"

#include <iostream>

H5VolWriterHandle::H5VolWriterHandle(MPI_Comm& comm, MPI_Info& info)
{
  htri_t  is_registered   = -1; // FAIL by default

  if((is_registered = H5VLis_connector_registered(ADIOS2_VOL_WRITER_NAME)) < 0)
    return;

  // to register by name it looks like one needs to put in in some plugin directory
  //if((m_VID = H5VLregister_connector_by_name(ADIOS2_VOL_WRITER_NAME, H5P_DEFAULT)) < 0)
  if((m_VID = H5VLregister_connector(&H5VL_adios2_def, H5P_DEFAULT)) < 0)
    return;

  Init(comm, info); 
}


H5VolWriterHandle::~H5VolWriterHandle()
{
  if (!IsValid())
    return;

  H5VLunregister_connector(m_VID);
  H5Pclose(m_VOLAccessProperty);
}


bool H5VolWriterHandle::IsValid() const
{
  return (H5I_INVALID_HID != m_VID);
}


void H5VolWriterHandle::Init(MPI_Comm&  comm, MPI_Info& info) 
{
  if (!IsValid())
    return;

  m_VOLAccessProperty = H5Pcreate (H5P_FILE_ACCESS);

  void* defaultVolInfo;
  herr_t status = H5Pget_vol_info(m_VOLAccessProperty, &defaultVolInfo);
  if (status < 0) {
    std::cout<<"Unable to get vol info "<<std::endl;
    return;
  }

  H5Pset_fapl_mpio(m_VOLAccessProperty, comm,  info);
  H5Pset_vol(m_VOLAccessProperty, m_VID, defaultVolInfo);
}


hid_t H5VolWriterHandle::CreateFile(const char* file_name)
{
  if (!IsValid())
    return -1;

  hid_t file_id = H5Fcreate(file_name, H5F_ACC_TRUNC, H5P_DEFAULT, m_VOLAccessProperty);
  if (file_id < 0) {
    fprintf(stderr, "Error creating file %s\n", file_name);
    return -1;
  }
  return file_id;
}

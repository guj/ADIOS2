#include "H5VolWriter.h"

class H5VolWriterHandle
{
 public:
  H5VolWriterHandle(MPI_Comm&, MPI_Info&);
  ~H5VolWriterHandle();

  bool  IsValid() const;
  void Init(MPI_Comm&,  MPI_Info&);

  hid_t CreateFile(const char* filename);
 private:
  // the associated VOL ID
  hid_t  m_VID = H5I_INVALID_HID; 
  hid_t  m_VOLAccessProperty = H5I_INVALID_HID;
};


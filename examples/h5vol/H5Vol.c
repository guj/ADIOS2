#include "H5Vol.h"

static hid_t m_VID = -1;

htri_t H5VL_ADIOS2_isRegistered()
{
    htri_t is_registered = -1; // FAIL by default
    is_registered = H5VLis_connector_registered(H5VL_ADIOS2_NAME);
    return is_registered;
}

void H5VL_ADIOS2_register()
{
    if (H5VL_ADIOS2_isRegistered() > 0)
        return;
    m_VID = H5VLregister_connector(&H5VL_adios2_def, H5P_DEFAULT);
}

void H5VL_ADIOS2_clean()
{
    if (H5I_INVALID_HID != m_VID)
        return;

    H5VLunregister_connector(m_VID);
    m_VID = H5I_INVALID_HID;
}

// extern
void H5VL_ADIOS2_set(hid_t fapl)
{
    H5VL_ADIOS2_register();

    void *defaultVolInfo;
    herr_t status = H5Pget_vol_info(fapl, &defaultVolInfo);
    if (status < 0)
    {
        printf("Unable to get vol info \n");
        return;
    }
    H5Pset_vol(fapl, m_VID, defaultVolInfo);

    gInitADIOS2(fapl);
}

#pragma once

#ifdef NO_DATA_SOURCE
    #include "fake_data.h"
#else
    #ifdef ARTERY_PROJECT
        #include "etsi_cam_artery_wrpayloader.h"
    #endif
#endif

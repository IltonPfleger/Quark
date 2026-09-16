#pragma once

#ifdef NO_DATA_SOURCE
    #include <transducers/av/fake_data.h>
#else
    #ifdef CARLA_V2_PROJECT
        #include "gps_carla_wrpayloader.h"
    #endif

    #ifdef GNSS_HARDWARE
        #include "gnss_lpms_ig1p.h"
    #endif
#endif

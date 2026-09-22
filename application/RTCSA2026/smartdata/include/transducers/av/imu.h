#pragma once

#ifdef NO_DATA_SOURCE
    #include "fake_data.h"
#else
    #ifdef CARLA_V2_PROJECT
        #include "imu_carla_wrpayloader.h"
    #endif

    #ifdef IMU_HARDWARE
        #include "imu_lpms_ig1p.h"
    #endif
#endif
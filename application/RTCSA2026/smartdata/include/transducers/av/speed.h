#pragma once

#ifdef NO_DATA_SOURCE
    #include "fake_data.h"
#else
    #ifdef CARLA_V2_PROJECT
        #include "speed_socket_wrpayloader.h"
    #endif

    #ifdef GPS_HARDWARE
        #include "gps_carla_wrpayloader.h"
    #endif
#endif
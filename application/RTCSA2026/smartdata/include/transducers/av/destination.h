#pragma once

#ifdef NO_DATA_SOURCE
    #include "fake_data.h"
#else
    #ifdef CARLA_V2_PROJECT
        #include "destination_carla_wrpayloader.h"
    #endif
#endif
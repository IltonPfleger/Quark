#pragma once

#ifdef NO_DATA_SOURCE
    #include "fake_data.h"
#else
    #include "battery_socket_wrpayloader.h" // not used in current CARLA V2 simulation
#endif
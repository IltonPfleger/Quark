#pragma once

#ifdef CARLA_V2_PROJECT
    #include "camera_carla_wrpayloader.h"
#endif

#ifdef CAMERA_HARDWARE
    #include "camera_luxonois_oakd.h"
    #include "camera_luxonois_oakd_shared_memory.h"
#endif

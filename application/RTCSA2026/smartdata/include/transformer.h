//#pragma once
//
//// EPOS Smart Transformer Declarations
//
//#include <transducer.h>
//
//#define _UTIL
//
//#ifdef BASIC_EXAMPLE
//    // Transformer
//    #include "transformers/example/temperature_adjust.h"
//#endif
//
//#ifdef CARLA_PROJECT
//    #include "transformers/old_carla_project.h"
//#endif
//
//#ifdef AV_PROJECT
#include "transformers/av/dynamics_state.h"
//    #include "transformers/av/object_recognition_and_tracking.h"
#include "transformers/av/object_recognition_and_tracking_fuser.h"
//    #include "transformers/av/path_planning.h"
//    #include "transformers/av/motion_planning.h"
//    #include "transformers/av/high_level_control.h"
//    #include "transformers/av/pdm_suspension.h"
//    #include "transformers/av/visual_vibration_detection.h"
//    #include "transformers/av/road_surface_condition_detection.h"
//#endif

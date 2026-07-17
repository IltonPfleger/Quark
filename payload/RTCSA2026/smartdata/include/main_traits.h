#pragma once
#include <system/traits.h>

// Build
template <> struct Traits<Build> : public Traits_Tokens {
    // Basic configuration
    static const unsigned int MODE                     = LIBRARY;
    static const unsigned int ARCHITECTURE             = IA32;
    static const unsigned int MACHINE                  = PC;
    static const unsigned int MODEL                    = Legacy_PC;
    static const unsigned int CPUS                     = 1;
    static const unsigned int NODES                    = -1U; // (> 1 => NETWORKING)
    static const unsigned int EXPECTED_SIMULATION_TIME = 60;  // s (0 => not simulated)

    // Default flags
    static const bool enabled               = true;
    static const bool monitored             = false;
    static const bool verified              = false;
    static const bool debugged              = true;
    static const bool hysterically_debugged = false;
#ifdef CLOUD_INTEGRATION
    static const bool cloud = true; // change as you need
#else
    static const bool cloud = false; // fixed
#endif

#ifdef NO_DATA_SOURCE
    static const bool no_connection_test = true;
#else
    static const bool no_connection_test = false;
#endif

    // Default aspects
    typedef ALIST<> ASPECTS;
};

#include <architecture/traits.h>

class Machine_Common;
template <> struct Traits<Machine_Common> : public Traits<Build> {};

template <> struct Traits<Machine> : public Traits<Machine_Common> {
    static const bool cpus_use_local_timer = false;

    static const unsigned int NOT_USED = 0xffffffff;
    static const unsigned int CPUS     = Traits<Build>::CPUS;

    // Boot Image
    static const unsigned int BOOT_LENGTH_MIN = 512;
    static const unsigned int BOOT_LENGTH_MAX = 512;
    static const unsigned int BOOT_IMAGE_ADDR = 0x00008000;
    static const unsigned int RAMDISK         = 0x0fa28000; // MEMDISK-dependent
    static const unsigned int RAMDISK_SIZE    = 0x003c0000;

    // Physical Memory
    static const unsigned int MEM_BASE   = 0x00000000;
    static const unsigned int MEM_TOP    = 0x10000000; // 256 MB (MAX for 32-bit is 0x70000000 / 1792 MB)
    static const unsigned int BOOT_STACK = NOT_USED;   // not used (defined by BOOT and by SETUP)

    // Logical Memory Map
    static const unsigned int BOOT  = 0x00007c00;
    static const unsigned int SETUP = 0x00100000; // 1 MB
    static const unsigned int INIT  = 0x00200000; // 2 MB

    static const unsigned int PAYLOAD_LOW  = 0x00000000;
    static const unsigned int PAYLOAD_CODE = 0x00000000;
    static const unsigned int PAYLOAD_DATA = 0x00400000; // 4 MB
    static const unsigned int PAYLOAD_HIGH = 0x0fffffff; // 256 MB

    static const unsigned int PHY_MEM = 0x80000000; // 2 GB
    static const unsigned int IO_BASE = 0xf0000000; // 4 GB - 256 MB
    static const unsigned int IO_TOP  = 0xff400000; // 4 GB - 12 MB

    static const unsigned int SYS      = IO_TOP; // 4 GB - 12 MB
    static const unsigned int SYS_CODE = 0xff700000;
    static const unsigned int SYS_DATA = 0xff740000;

    // Default Sizes and Quantities
    static const unsigned int STACK_SIZE  = 16 * 1024;
    static const unsigned int HEAP_SIZE   = 16 * 1024 * 1024;
    static const unsigned int MAX_THREADS = 16;
};

template <> struct Traits<Project> : public Traits<Build> {
    static const bool yaw_pitch_roll_derived               = true;
    static const bool speed_derived                        = true;
    static const bool CAM_IS_TRANSFORMER                   = true;
    static const bool LIDAR_IS_TRANSFORMER                 = true;
    static const bool RADAR_IS_ON                          = false;
    static const bool ETSI_CAM_IS_ON                       = false;
    static const bool RADAR_IS_TRANSFORMER                 = false;
    static const bool FUSER_IS_GROUND_TRUTH                = false;
    static const unsigned int STEERING_ANGLE_ACTUATION_DEV = 31;
    static const unsigned int AV_STATE_LIST_COMPLETE_DEV   = 30;
    static const unsigned int MOTION_VECTOR_LOCAL_DEV      = 23;
    static const unsigned int AV_STATE_LIST_SIZE           = 150;
    static const unsigned int MOTION_VECTOR_LOCAL_SIZE     = 53;
    static const int VEHICLE_LENGTH                        = 35;
    static const int VEHICLE_WIDTH                         = 21;
    static const int VEHICLE_HEIGHT                        = 18; // m^-1
    static const unsigned int VEHICLE_CLASS                = 5;
    static const unsigned int MAX_MAP_POINTS               = 150;

    // Needed For DM-KF-Hardware
    static const unsigned long long EGO_ID          = 120;
    static const bool DYNAMIC_STATE_SENSOR          = false;
    static const unsigned int FLOAT_INT_PRECISION   = 100000000;
    static const unsigned int FLOAT_INT_PRECISION_M = 1000;
};

template <> struct Traits<Vehicle_Models_Parameters> : public Traits<Build> {
    static const unsigned int AUDI_A2_mass        = 1080;
    static constexpr float AUDI_A2_wheel_base     = 2.405;
    static constexpr float AUDI_A2_front_overhang = 1.445;
    static constexpr float AUDI_A2_rear_overhang  = 0.962;

    // this parameters have been extracted from CARLA blueprint library, please, change this to the
    // respective vehicle before starting a simulation
    static const unsigned int mass        = AUDI_A2_mass;
    static constexpr float wheel_base     = AUDI_A2_wheel_base;
    static constexpr float front_overhang = AUDI_A2_front_overhang;
    static constexpr float rear_overhang  = AUDI_A2_rear_overhang;
};

template <> struct Traits<Object_Classes> : public Traits<Build> {

    static const int UNKNOWN_LENGTH         = 0;
    static const int UNKNOWN_WIDTH          = 0;
    static const int UNKNOWN_HEIGHT         = 0; // cm
    static const unsigned int UNKNOWN_CLASS = 0;

    static const int MOPED_LENGTH         = 235;
    static const int MOPED_WIDTH          = 88;
    static const int MOPED_HEIGHT         = 150; // cm
    static const unsigned int MOPED_CLASS = 1;

    static const int MOTORCYCLE_LENGTH         = 230;
    static const int MOTORCYCLE_WIDTH          = 90;
    static const int MOTORCYCLE_HEIGHT         = 60; // cm
    static const unsigned int MOTORCYCLE_CLASS = 2;

    static const int PASSENGER_CAR_LENGTH         = 442;
    static const int PASSENGER_CAR_WIDTH          = 198;
    static const int PASSENGER_CAR_HEIGHT         = 155; // cm
    static const unsigned int PASSENGER_CAR_CLASS = 3;

    static const int BUS_LENGTH         = 1020;
    static const int BUS_WIDTH          = 246;
    static const int BUS_HEIGHT         = 323; // cm
    static const unsigned int BUS_CLASS = 4;

    static const int LIGHT_TRUCK_LENGTH         = 690;
    static const int LIGHT_TRUCK_WIDTH          = 290;
    static const int LIGHT_TRUCK_HEIGHT         = 340; // cm
    static const unsigned int LIGHT_TRUCK_CLASS = 5;

    static const int HEAVY_TRUCK_LENGTH         = 1500;
    static const int HEAVY_TRUCK_WIDTH          = 245;
    static const int HEAVY_TRUCK_HEIGHT         = 400; // cm
    static const unsigned int HEAVY_TRUCK_CLASS = 6;

    static const int TRAILER_LENGTH         = 1200;
    static const int TRAILER_WIDTH          = 250;
    static const int TRAILER_HEIGHT         = 430; // cm
    static const unsigned int TRAILER_CLASS = 7;

    static const int SPECIAL_LENGTH         = 0;
    static const int SPECIAL_WIDTH          = 0;
    static const int SPECIAL_HEIGHT         = 0; // cm
    static const unsigned int SPECIAL_CLASS = 8;

    static const int TRAM_LENGTH         = 1600;
    static const int TRAM_WIDTH          = 300;
    static const int TRAM_HEIGHT         = 400; // cm
    static const unsigned int TRAM_CLASS = 9;

    static const int EMERGENCY_LENGTH         = 520;
    static const int EMERGENCY_WIDTH          = 230;
    static const int EMERGENCY_HEIGHT         = 274; // cm
    static const unsigned int EMERGENCY_CLASS = 10;

    static const int AGRICULTURAL_LENGTH         = 436;
    static const int AGRICULTURAL_WIDTH          = 201;
    static const int AGRICULTURAL_HEIGHT         = 274; // cm
    static const unsigned int AGRICULTURAL_CLASS = 11;

    static const unsigned int REFERENCE_CLASS       = 12;
    static const unsigned int EGO_CLASS             = 13;
    static const unsigned int EGO_FUTURE_CLASS      = 14;
    static const unsigned int EGO_DESTINATION_CLASS = 15;
};

// API Components
template <> struct Traits<Payload> : public Traits<Build> {
    static const unsigned int STACK_SIZE  = Traits<Machine>::STACK_SIZE;
    static const unsigned int HEAP_SIZE   = Traits<Machine>::HEAP_SIZE;
    static const unsigned int MAX_THREADS = Traits<Machine>::MAX_THREADS;
};

template <> struct Traits<System> : public Traits<Build> {
    static const unsigned int mode = Traits<Build>::MODE;
    static const bool multithread  = (Traits<Build>::CPUS > 1) || (Traits<Payload>::MAX_THREADS > 1);
    static const bool multitask    = (mode != Traits<Build>::LIBRARY);
    static const bool multicore    = (Traits<Build>::CPUS > 1) && multithread;
    static const bool multiheap    = multitask || Traits<Scratchpad>::enabled;

    static const unsigned int LIFE_SPAN  = 1 * YEAR; // s
    static const unsigned int DUTY_CYCLE = 1000000;  // ppm

    static const bool reboot = false;

    static const unsigned int STACK_SIZE = Traits<Machine>::STACK_SIZE;
    static const unsigned int HEAP_SIZE  = (Traits<Payload>::MAX_THREADS + 1) * Traits<Payload>::STACK_SIZE;
};

template <> struct Traits<Thread> : public Traits<Build> {
    static const bool enabled           = Traits<System>::multithread;
    static const bool smp               = Traits<System>::multicore;
    static const bool simulate_capacity = false;
    static const bool trace_idle        = hysterically_debugged;

    typedef Priority Criterion;
    static const unsigned int QUANTUM = 10000; // us
};

template <> struct Traits<SmartData> : public Traits<Build> {
    static const unsigned char PREDICTOR = NONE;
};

// Utilities
template <> struct Traits<Debug> : public Traits<Build> {
    static const bool error   = false;
    static const bool warning = false;
    static const bool info    = false;
    static const bool trace   = false;
    static const bool logger  = true;
};

template <> struct Traits<Network> : public Traits<Build> {
    typedef LIST<TSTP> NETWORKS;

    static const unsigned int RETRIES = 3;
    static const unsigned int TIMEOUT = 10; // s

    static const bool routing = false;
    static const bool enabled = (Traits<Build>::NODES > 1) && (NETWORKS::Length > 0);
};

template <> struct Traits<Only_Data_UDP_Wrpayloader> : public Traits<Build> {
    static const unsigned short UNDERLYING_PROTOCOL = 0x8401; // TSTP
};

template <> struct Traits<TSTP> : public Traits<Network> {
    typedef Only_Data_UDP_Wrpayloader NIC_Family;
    static constexpr unsigned int NICS[] = {0}; // relative to NIC_Family (i.e. Traits<Ethernet>::DEVICES[NICS[i]]
    static const unsigned int UNITS      = COUNTOF(NICS);

    static const unsigned int KEY_SIZE    = 16;
    static const unsigned int RADIO_RANGE = 8000; // payloadroximated radio range in centimeters

    static const bool enabled        = Traits<Network>::enabled && (NETWORKS::Count<TSTP>::Result > 0);
    static const bool use_encryption = false;
};

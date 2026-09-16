#pragma once

#include <OpenZen.h>
#include <math.h>
#include <smartdata.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <iostream>

namespace lpms
{
constexpr double PI      = 3.14159265359;
constexpr double GRAVITY = 9.80664999999998;

class Client;

enum INSDevice {
    INS_LONGITUDINAL_ACCELERATION = 0,
    INS_LATERAL_ACCELERATION,
    INS_VERTICAL_ACCELERATION,
    INS_YAW_RATE,
    INS_PITCH_RATE,
    INS_ROLL_RATE,
    INS_YAW,
    GNSS_LATITUDE,
    GNSS_LONGITUDE,
    GNSS_ALTITUDE,
    GNSS_VELOCITY,
    GNSS_Timestamp,
    INSDeviceLastValue
};

union DataUnion {
    float fp;
    float fp64;
    float ll;
};

};  // namespace lpms

class lpms::Client
{
   protected:
    ZenClientHandle_t _client;
    ZenSensorHandle_t _sensor;
    pthread_t _thread;
    pthread_mutex_t _imu_data_mutex;
    pthread_mutex_t _gnss_data_mutex;
    pthread_mutex_t _reconnect_mutex;
    volatile union DataUnion _values[lpms::INSDeviceLastValue];

    Client()
    {
        if (ZenInit(&_client) != ZenError_None)
            db<SmartData>(ERR) << "INS Client Error: Can't Start OpenZen Client." << endl;
        else {
            connect();
            _values[INS_LONGITUDINAL_ACCELERATION].fp = NAN;
            _values[INS_LATERAL_ACCELERATION].fp      = NAN;
            _values[INS_VERTICAL_ACCELERATION].fp     = NAN;
            _values[INS_YAW_RATE].fp                  = NAN;
            _values[INS_PITCH_RATE].fp                = NAN;
            _values[INS_ROLL_RATE].fp                 = NAN;
            _values[INS_YAW].fp                       = NAN;
            _values[GNSS_LONGITUDE].fp64              = NAN;
            _values[GNSS_LATITUDE].fp64               = NAN;
            _values[GNSS_ALTITUDE].fp64               = NAN;
            _values[GNSS_VELOCITY].fp64               = NAN;
            _values[GNSS_Timestamp].ll                = -1;
            // setGnssTimestampToNanoseconds();
            pthread_mutex_init(&_imu_data_mutex, NULL);
            pthread_mutex_init(&_gnss_data_mutex, NULL);
            pthread_mutex_init(&_reconnect_mutex, NULL);
            pthread_create(&_thread, NULL, &lpms::Client::event_thread, (void*)this);
        }
    }

   public:
    ~Client() { ZenShutdown(_client); }

    static Client& GetInstance()
    {
        static Client client;
        return client;
    }

    void connect()
    {
        if (ZenListSensorsAsync(_client) != ZenError_None) db<SmartData>(ERR) << "INS Client Error: Can't Search For Sensors." << endl;
        ZenEvent zenEvent;
        do {
            ZenWaitForNextEvent(_client, &zenEvent);
            if (zenEvent.eventType != ZenEventType_SensorFound) continue;
            if (ZenObtainSensor(_client, &zenEvent.data.sensorFound, &_sensor) == ZenSensorInitError_None) break;
        } while (!zenEvent.data.sensorListingProgress.complete);
    }

    void reconnect()
    {
        ZenShutdown(_client);
        sleep(1);
        if (ZenInit(&_client) == ZenError_None) connect();
        sleep(1);
    }

    void set_gnss_timestamp_to_nanoseconds()
    {
        ZenComponentHandle_t* handles = NULL;
        size_t nComponents;
        if (ZenSensorComponents(_client, _sensor, g_zenSensorType_Gnss, &handles, &nComponents) != ZenError_None)
            db<SmartData>(ERR) << "INS Client Error: Can't Find GNSS Component." << endl;
        ZenSensorComponentSetBoolProperty(_client, _sensor, handles[0], ZenGnssProperty_OutputNavPvtNano, true);
    }

    float convert_heading(ZenImuData data)
    {
        float raw = atan2(data.b[1], data.b[0]) * (180 / lpms::PI);
        return (raw < 0 ? fmod(raw, 360) + 360 : raw) * lpms::PI / 180;
    }

    Int64 convert_timestamp(ZenGnssData data)
    {
        struct tm now;
        now.tm_sec   = data.second;
        now.tm_min   = data.minute;
        now.tm_hour  = data.hour;
        now.tm_mday  = data.day;
        now.tm_mon   = data.month - 1;
        now.tm_year  = data.year - 1900;
        time_t epoch = mktime(&now);
        return ((Int64)epoch) * 1'000'000'000 + (Int64)(data.nanoSecondCorrection);
    }

    static void* event_thread(void* ptr)
    {
        lpms::Client* self    = (lpms::Client*)ptr;
        double imu_timestamp  = -1;
        double gnss_timestamp = -1;
        clock_t imu_interval  = clock();
        clock_t gnss_interval = clock();
        while (true) {
            ZenEvent event;
            ZenWaitForNextEvent(self->_client, &event);
            if (event.eventType == ZenEventType_ImuData && imu_timestamp != event.data.imuData.timestamp) {
                pthread_mutex_lock(&self->_imu_data_mutex);
                imu_timestamp                                         = event.data.imuData.timestamp;
                imu_interval                                          = clock();
                self->_values[lpms::INS_LONGITUDINAL_ACCELERATION].fp = event.data.imuData.a[0] * GRAVITY;
                self->_values[lpms::INS_LATERAL_ACCELERATION].fp      = event.data.imuData.a[1] * GRAVITY;
                self->_values[lpms::INS_VERTICAL_ACCELERATION].fp     = event.data.imuData.a[2] * GRAVITY;
                self->_values[lpms::INS_YAW_RATE].fp                  = event.data.imuData.g1[0] * lpms::PI / 180;
                self->_values[lpms::INS_PITCH_RATE].fp                = event.data.imuData.g1[1] * lpms::PI / 180;
                self->_values[lpms::INS_ROLL_RATE].fp                 = event.data.imuData.g1[2] * lpms::PI / 180;
                self->_values[lpms::INS_YAW].fp                       = self->convert_heading(event.data.imuData);
                // self->_values[lpms::GNSS_ALTITUDE].fp      = event.data.imuData.altitude;
                pthread_mutex_unlock(&self->_imu_data_mutex);
                continue;
            };
            if (event.eventType == ZenEventType_GnssData && gnss_timestamp != event.data.gnssData.timestamp) {
                pthread_mutex_lock(&self->_gnss_data_mutex);
                gnss_timestamp                           = event.data.gnssData.timestamp;
                gnss_interval                            = clock();
                self->_values[lpms::GNSS_LONGITUDE].fp64 = event.data.gnssData.longitude * lpms::PI / 180;
                self->_values[lpms::GNSS_LATITUDE].fp64  = event.data.gnssData.latitude * lpms::PI / 180;
                self->_values[lpms::GNSS_ALTITUDE].fp64  = event.data.gnssData.height;
                self->_values[lpms::GNSS_VELOCITY].fp64  = event.data.gnssData.velocity;
                self->_values[lpms::GNSS_Timestamp].ll   = self->convert_timestamp(event.data.gnssData);
                pthread_mutex_unlock(&self->_gnss_data_mutex);
                continue;
            };
            if ((clock() - imu_interval) / CLOCKS_PER_SEC > 1 || (gnss_interval - clock()) / CLOCKS_PER_SEC > 5) {
                pthread_mutex_lock(&self->_reconnect_mutex);
                self->reconnect();
                imu_interval  = clock();
                gnss_interval = clock();
                pthread_mutex_unlock(&self->_reconnect_mutex);
            };
        };
    }

    bool get_data(lpms::INSDevice target, float* dst)
    {
        pthread_mutex_lock(&_imu_data_mutex);
        if (!isnan(_values[target].fp)) {
            *dst               = _values[target].fp;
            _values[target].fp = NAN;
            pthread_mutex_unlock(&_imu_data_mutex);
            return true;
        }
        pthread_mutex_unlock(&_imu_data_mutex);
        return false;
    }

    bool get_data(lpms::INSDevice target, double* dst)
    {
        pthread_mutex_lock(&_gnss_data_mutex);
        if (!isnan(_values[target].fp64)) {
            *dst                 = _values[target].fp64;
            _values[target].fp64 = NAN;
            pthread_mutex_unlock(&_gnss_data_mutex);
            return true;
        }
        pthread_mutex_unlock(&_gnss_data_mutex);
        return false;
    }

    bool get_data(lpms::INSDevice target, Int64* dst)
    {
        pthread_mutex_lock(&_gnss_data_mutex);
        if (_values[target].ll == -1) {
            *dst               = _values[target].ll;
            _values[target].ll = -1;
            pthread_mutex_unlock(&_gnss_data_mutex);
            return true;
        }
        pthread_mutex_unlock(&_gnss_data_mutex);
        return false;
    }
};

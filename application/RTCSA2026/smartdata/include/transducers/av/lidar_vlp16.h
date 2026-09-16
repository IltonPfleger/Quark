#pragma once

#include <transducer.h>
#include <math.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <chrono>

class VLP16_LIDAR_Transducer;

namespace vlp
{
    static constexpr int VLP16_N_LASERS = 16;
    static constexpr int VLP16_FIRINGS_PER_BLOCK = 2;
    static constexpr int VLP16_POINTS_PER_BLOCK = VLP16_FIRINGS_PER_BLOCK * VLP16_N_LASERS;
    static constexpr int VLP16_DATA_BLOCK_SIZE = 100;
    static constexpr int VLP16_STREAM_SIZE = 1206;
    static constexpr int VLP16_BLOCKS_PER_PACKET = 12;
    static constexpr int VLP16_POINTS_PER_PACKET = VLP16_POINTS_PER_BLOCK * VLP16_BLOCKS_PER_PACKET;
    static constexpr float VLP16_TIME_PER_FIRING = 2.304f;// [µs] * pow(10, -6);
    static constexpr float VLP16_TIME_PER_FIRING_SEQUENCE = 55.296f;// [µs] * pow(10, -6);
    static constexpr float PI = 3.14159265359;
    static constexpr float DEG_TO_RADIANS = PI/180.f;
    static constexpr float DISTANCE_TO_METERS = 2.0/1000.f;
    static constexpr std::array<int, VLP16_N_LASERS> omegas = {-15, 1, -13, 3, -11, 5, -9, 7, -7, 9, -5, 11, -3, 13, -1, 15};


    static const float deg_sin(float x)
    {
        return sin(x*DEG_TO_RADIANS);
    };

    static const float deg_cos(float x)
    {
        return cos(x*DEG_TO_RADIANS);
    };


    static const std::array<float, VLP16_N_LASERS> get_sin_omegas()
    {
        static std::array<float, VLP16_N_LASERS> internal_sins_buffer;
        for(int i = 0; i < VLP16_N_LASERS; i++)
            internal_sins_buffer[i] = deg_sin(omegas[i]);
        return internal_sins_buffer;
    }

    static const std::array<float, VLP16_N_LASERS> get_cos_omegas()
    {
        static std::array<float, VLP16_N_LASERS> internal_coss_buffer;
        for(int i = 0; i < VLP16_N_LASERS; i++)
            internal_coss_buffer[i] = deg_cos(omegas[i]);
        return internal_coss_buffer;
    }


//static constexpr std::array<float, VLP16_N_LASERS> sin_omegas = []() {std::array<float, VLP16_N_LASERS> tmp{};for(int i = 0; i < VLP16_N_LASERS; i++)tmp[i] = deg_sin(omegas[i]);return tmp;}();
//static constexpr std::array<float, VLP16_N_LASERS> cos_omegas = []() {std::array<float, VLP16_N_LASERS> tmp{};for(int i = 0; i < VLP16_N_LASERS; i++)tmp[i] = deg_cos(omegas[i]);return tmp;}();
    static const std::array<float, VLP16_N_LASERS> sin_omegas = get_sin_omegas();
    static const std::array<float, VLP16_N_LASERS> cos_omegas = get_cos_omegas();


    typedef float LIDAR_DistanceDataType;
    typedef unsigned char LIDAR_IntensityDataType;
    typedef struct __attribute__((packed))
    {
        LIDAR_DistanceDataType x;
        LIDAR_DistanceDataType y;
        LIDAR_DistanceDataType z;
        LIDAR_IntensityDataType i;
    } LIDAR_Point;

    typedef struct {
        unsigned char raw_distance[2];
        unsigned char reflexibility;
    } VLP16_DataPoint;

    typedef struct {
        unsigned char flag[2];
        unsigned char raw_azimuth[2];
        VLP16_DataPoint points[VLP16_POINTS_PER_BLOCK];
    } VLP16_DataBlock;

    typedef struct {
        VLP16_DataBlock blocks[VLP16_BLOCKS_PER_PACKET];
        unsigned char time_stmap[4];
        unsigned char factory[2];
    } VLP16_Packet;

    class LIDAR;

};

class vlp::LIDAR
{
        friend VLP16_LIDAR_Transducer;

    public:
        LIDAR(const int16_t _port = 2368)
        {
            memset(&this->_address, 0, sizeof(this->_address));
            this->_address_len = sizeof(this->_address);
            this->_n_socket = socket(AF_INET, SOCK_DGRAM, 0);
            this->_address.sin_family = AF_INET;
            this->_address.sin_addr.s_addr = INADDR_ANY;
            this->_address.sin_port = htons(_port);
            if (_n_socket < 0)
                db<SmartData>(ERR) << "LIDAR Constructor Error: Can't create main socket" << endl;
            if(bind(_n_socket, (const struct sockaddr*)&this->_address, sizeof(this->_address)) == -1)
                db<SmartData>(ERR) << "LIDAR Constructor Error: Can't connect main socket" << endl;
        }

        bool sense()
        {
            unsigned char stream[vlp::VLP16_STREAM_SIZE];
            int n_bytes = recvfrom(this->_n_socket, stream, sizeof(stream), 0, (struct sockaddr *)&this->_address, &this->_address_len);
            if(n_bytes < 0) {
                db<SmartData>(ERR) << "LIDAR Sense Error: Can't capture sensor data" << endl;
                return false;
            } else if(n_bytes != 1206) {
                db<SmartData>(ERR) << "LIDAR Sense Warnning: UDP package missing bytes"<< endl;
                return false;
            }
            this->unpack_data(stream);
            return true;
        }

        void get_precise_azimuths(vlp::VLP16_Packet * packet)
        {
            double azimuths[vlp::VLP16_BLOCKS_PER_PACKET];
            float azimuth_gap;
            for(int i = 0; i < vlp::VLP16_BLOCKS_PER_PACKET; i++)
                azimuths[i] = (((uint8_t)(packet->blocks[i].raw_azimuth[1]) << 8) | uint8_t(packet->blocks[i].raw_azimuth[0]))/100;
            for(int i = 0, l = 0; i < vlp::VLP16_BLOCKS_PER_PACKET; i++, l += vlp::VLP16_POINTS_PER_BLOCK) {
                azimuth_gap = 0;
                if( i != vlp::VLP16_BLOCKS_PER_PACKET - 1) {
                    if(azimuths[i + 1] < azimuths[i])
                        azimuths[i + 1] += 360;
                    azimuth_gap = azimuths[i + 1] - azimuths[i];
                }
                for(int k = 0; k < vlp::VLP16_POINTS_PER_BLOCK; k++)
                    if(k < 16)
                        _precise_azimuths[l + k]  = azimuths[i] + ((azimuth_gap * vlp::VLP16_TIME_PER_FIRING * k) / vlp::VLP16_TIME_PER_FIRING_SEQUENCE);
                    else
                        _precise_azimuths[l + k] = azimuths[i] + (azimuth_gap * vlp::VLP16_TIME_PER_FIRING * ((k-16) + vlp::VLP16_TIME_PER_FIRING_SEQUENCE)) / (2 * vlp::VLP16_TIME_PER_FIRING_SEQUENCE);
            }
        }


        void unpack_data(unsigned char * stream)
        {
            vlp::VLP16_Packet * packet = reinterpret_cast<vlp::VLP16_Packet*>(stream);
            get_precise_azimuths(packet);
            for(int i = 0, l = 0; i < vlp::VLP16_BLOCKS_PER_PACKET; i++, l += vlp::VLP16_POINTS_PER_BLOCK) {
                vlp::VLP16_DataBlock block = packet->blocks[i];
                for(int j = 0, k = 0; j < vlp::VLP16_POINTS_PER_BLOCK; j++, k++) {
                    if(k == vlp::VLP16_N_LASERS)
                        k = 0;
                    vlp::VLP16_DataPoint point = block.points[j];
                    float distance = (((uint8_t)(point.raw_distance[1]) << 8) | uint8_t(point.raw_distance[0])) * vlp::DISTANCE_TO_METERS;
                    float cos_distance = distance * vlp::cos_omegas[k];
                    this->_buffer[l + j].x = cos_distance * vlp::deg_sin(_precise_azimuths[l + j]);
                    this->_buffer[l + j].y = cos_distance * vlp::deg_cos(_precise_azimuths[l + j]);
                    this->_buffer[l + j].z = distance * vlp::sin_omegas[k];
                    this->_buffer[l + j].i = point.reflexibility;
                }
            }
        }
    protected:
        int16_t _port;
        float _precise_azimuths[vlp::VLP16_POINTS_PER_PACKET];
        int _n_socket;
        struct sockaddr_in _address;
        socklen_t _address_len;

    private:
        vlp::LIDAR_Point _buffer[vlp::VLP16_POINTS_PER_PACKET];
};

class VLP16_LIDAR_Transducer : public Transducer<SmartData::Unit::PCD_MONOCROMATIC|2>
{
        friend Responsive_SmartData<VLP16_LIDAR_Transducer>;

    public:
        static const bool active = true;
        static const Uncertainty UNCERTAINTY = UNKNOWN;
        static const Type TYPE = SENSOR;

        VLP16_LIDAR_Transducer(UInt32 dev): _value()
        {
            _uncertainty = UNKNOWN;
            _vlp16 = new vlp::LIDAR();
            _receive_thread = new Thread(&lidar_reader_thread, this);
            db<SmartData>(TRC) << "VLP16 LIDAR Transducer Created" << endl;
        }

        ~VLP16_LIDAR_Transducer()
        {
            delete _vlp16;
            delete _receive_thread;
        }

        virtual Value sense() { return _value; }
        virtual Uncertainty uncertainty() { return _uncertainty; }
        virtual SmartData::Signature signature() { return 0; }

        static void* lidar_reader_thread(void * p)
        {
            Thread::assignhandler();
            VLP16_LIDAR_Transducer *transducer = reinterpret_cast<VLP16_LIDAR_Transducer *>(p);
            char BASE_FILE_NAME[] = "./bin/LIDAR/LIDAR_CLOUD_POINT-";
            char FILE_NAME[128];
            FILE * file;
            while (true) {
                if (transducer->_vlp16->sense()) { //blocking
                    auto now = std::chrono::system_clock::now();
                    UInt64 t = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
                    sprintf(FILE_NAME, "%s%llu.pcd", BASE_FILE_NAME, t);
                    file = fopen(FILE_NAME, "wb");
                    fwrite(transducer->_vlp16->_buffer, 1, sizeof(transducer->_vlp16->_buffer), file);
                    fclose(file);
                    transducer->notify();
                    memcpy((void*)&transducer->_value, &transducer->_vlp16->_buffer, sizeof(vlp::LIDAR_Point)*vlp::VLP16_POINTS_PER_PACKET);
                };
            };
        };

    private:
        Value _value;
        Uncertainty _uncertainty;
        vlp::LIDAR *_vlp16;
        Thread * _receive_thread;
};

using VLP16_LIDAR = Responsive_SmartData<VLP16_LIDAR_Transducer>;

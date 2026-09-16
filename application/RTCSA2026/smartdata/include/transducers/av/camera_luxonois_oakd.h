#pragma once
#include <transducer.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <depthai/depthai.hpp>

class OAK_D_Camera_Transducer : public Transducer<SmartData::Unit::RAW_BGR | 2>
{
    friend Responsive_SmartData<OAK_D_Camera_Transducer>;

   public:
    static constexpr UInt32 WIDTH      = 1920;
    static constexpr UInt32 HEIGHT     = 1200;
    static constexpr UInt32 IMAGE_SIZE = WIDTH * HEIGHT * 3;
    static constexpr const char *STREAM_NAME = "BGR";
    static constexpr const char *IMAGES_PATH = "./bin/CAMERA";

    // SmartData
    static constexpr UInt32 PERIOD = 0;
    static constexpr bool active         = true;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE               = SENSOR;

    OAK_D_Camera_Transducer(UInt32 dev) : _value(1), _uncertainty(UNKNOWN)
    {
        connect();
        start_reader_thread();
        db<SmartData>(TRC) << "OAKD Transducer Created" << endl;
    };

    void start_reader_thread() { _thread = new Periodic_Thread(Microsecond(PERIOD), &camera_reader_thread, (void *)this); }

    void connect()
    {
        dai::Pipeline pipeline = get_pipeline();
        db<SmartData>(TRC) << "Connecting OAKD Transducer..." << endl;
        try {
            _device = new dai::Device(pipeline);
        } catch (...) {
            db<SmartData>(ERR) << "Connection fail, trying again..." << endl;
            connect();
        };
    };

    void reconnect()
    {
        db<SmartData>(TRC) << "Disconnecting OAKD Transducer..." << endl;
        delete _device;
        db<SmartData>(TRC) << "Disconnected!" << endl;
        sleep(5);
        db<SmartData>(TRC) << "Reconnecting OAKD Transducer..." << endl;
        connect();
        db<SmartData>(TRC) << "Reconnected!" << endl;
    };

    dai::Pipeline get_pipeline()
    {
        // Create Pipeline
        dai::Pipeline pipeline;
        std::shared_ptr<dai::node::ColorCamera> camBGR = pipeline.create<dai::node::ColorCamera>();
        std::shared_ptr<dai::node::XLinkOut> xout      = pipeline.create<dai::node::XLinkOut>();
        xout->setStreamName(STREAM_NAME);

        // Setup Input
        camBGR->setBoardSocket(dai::CameraBoardSocket::CAM_A);
        camBGR->setResolution(dai::ColorCameraProperties::SensorResolution::THE_1200_P);
        camBGR->setVideoSize(WIDTH, HEIGHT);
        camBGR->setColorOrder(dai::ColorCameraProperties::ColorOrder::BGR);

        // Setup Output
        xout->input.setBlocking(false);
        xout->input.setQueueSize(1);
        camBGR->video.link(xout->input);
        return pipeline;
    }

    static void *camera_reader_thread(void *ptr)
    {
        Thread::assignhandler();
        OAK_D_Camera_Transducer *transducer         = (OAK_D_Camera_Transducer *)ptr;
        std::shared_ptr<dai::DataOutputQueue> queue = transducer->_device->getOutputQueue(STREAM_NAME, 1, false);
        UInt64 last_image_timestamp     = get_nanoseconds_timestamp();
        while (true) {
            std::shared_ptr<dai::ImgFrame> img = queue->tryGet<dai::ImgFrame>();
            if (img == nullptr) {
                UInt64 image_timestamp = get_nanoseconds_timestamp();
                UInt64 image_delay     = image_timestamp - last_image_timestamp;
                if (image_delay > 1e9) {
                    transducer->reconnect();
                }
                continue;
            };
            last_image_timestamp = get_nanoseconds_timestamp();
            memcpy((void *)&transducer->_value, img->getCvFrame().data, IMAGE_SIZE);
            transducer->save_image();
            transducer->notify();
            Periodic_Thread::wait_next();
        }
    }

    void save_image()
    {
        char file_name[128];
        get_new_image_file_name(file_name);
        FILE *file = fopen(file_name, "wb");
        fwrite(&_value, IMAGE_SIZE, 1, file);
        fclose(file);
    }

    static void get_new_image_file_name(char *dst) { sprintf(dst, "%s/CAMERA_IMAGE-%llu.raw", IMAGES_PATH, get_nanoseconds_timestamp()); }

    static UInt64 get_nanoseconds_timestamp()
    {
        auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    }

    ~OAK_D_Camera_Transducer() { delete _thread; }
    virtual Value sense() { return _value; }
    virtual Uncertainty uncertainty() { return _uncertainty; }
    virtual SmartData::Signature signature() { return 0; }

   private:
    Value _value;
    dai::Device *_device;
    Uncertainty _uncertainty;
    Periodic_Thread *_thread;
};

using OAK_D_Camera = Responsive_SmartData<OAK_D_Camera_Transducer>;

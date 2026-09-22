#pragma once
#include <transformer.h>
#include "vibration_detection.hpp"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>


using Camera_AV_Proxy =       Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::RAW_BGR|1)>>;
using Dynamics_State_Proxy =  Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::MOTION_VECTOR_LOCAL|12<<16|1)>>;

template<unsigned int source_dev>
class Visual_Vibration_Detection_Transformer: public Transducer<SmartData::Unit::Road_Condition_Class| SmartData::Unit::I32>, private Observer
{
    friend Responsive_SmartData<Visual_Vibration_Detection_Transformer<17>>;
    friend Responsive_SmartData<Visual_Vibration_Detection_Transformer<52>>; // Unit is Ignored -- Sensor/Transformer

public:
    static const bool active = true;
    static const unsigned long long EXPIRY = 100000;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR;
    static const unsigned int DYNAMICS_DEV = 16;

    typedef __UTIL::Observer Observer;

public:
    Visual_Vibration_Detection_Transformer(const Device_Id & dev) : _value(), _dev(dev) {
        // 5013 --> (11+1) (SmartData <-> Model in Python)
        detector = new VibrationDetection();
        
        _sp = new Dynamics_State_Proxy(Dynamics_State_Proxy::Region(0, 0, 0, 100, Dynamics_State_Proxy::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, DYNAMICS_DEV);
        _sp->attach(this);
        db<SmartData>(TRC) << "Visual_Vibration_Detection_Transformer Interest in dynamic state created!" << DYNAMICS_DEV << ",u=" << _input->UNIT << endl;
    
        if (source_dev != _dev) { // A regular Transformer (relies on other sensor)
            // inputs
            _input = new Camera_AV_Proxy(Camera_AV_Proxy::Region(0, 0, 0, 100, Camera_AV_Proxy::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, source_dev);
            db<SmartData>(INF) << "Visual_Vibration_Detection_Transformer Interest created!" << source_dev << ",u=" << _input->UNIT << endl;
            // attach my inputs to trigger my update
            _input->attach(this);
        }
        _last_consumption = 0;
    }

    ~Visual_Vibration_Detection_Transformer() { db<SmartData>(TRC) << "~Visual_Vibration_Detection - d=" << _dev << endl; delete _sp; if (source_dev != _dev) { delete _input;  } }

    virtual Value sense() { return _value; }
    virtual SmartData::Signature signature() { return 0; }

    void update(typename __UTIL::Observed *obs) {
        if (source_dev != _dev) {
            db<SmartData>(TRC) << "Visual_Vibration_Detection update! - d=" << _dev << "," << _last_consumption << _sp->when() << "," << _input->when() << endl;
            if (_input->when() > _last_consumption &&
                _sp->when() > _last_consumption) {
                transform();
                _last_consumption = _sp->now();
                notify();
            }
        } else {
            db<SmartData>(TRC) << "Visual_Vibration_Detection update! - d=" << _dev << "," << _last_consumption << "," << _sp->when() << endl;
            if (_sp->when() > _last_consumption) {
                transform();
                _last_consumption = _sp->now();
                notify();   
            }
        }
    }

private:

    uint32_t pack_data(float data)
    {
        uint8_t v = static_cast<uint8_t>(
            std::clamp(data * 100.0f, 0.0f, 255.0f)
        );

        uint32_t packed = 0;
        packed |= (v & 0xFF) << 24;
        return packed;
    }

    

    bool transform() {
        std::map<std::string, float> classes = {
            {"Unknown", 0.0f}, {"Normal", 1.0f}, {"Pothole", 2.0f}, {"Bump", 3.0f}
        };
        Dynamics_State_Proxy::Value sp = *_sp;
        cv::Mat img;

        if (source_dev != _dev) {
            typename Camera_AV_Proxy::Value in = *_input;
            img = convertSmartDataToMat(in);
        }
        else {
            std::cout << "Loading image from file..." << std::endl;
            img = convertRawToMat(reinterpret_cast<const uint8_t*>(std::ifstream(TEMP_IMAGE_SOURCE, std::ios::binary).rdbuf()), 1920, 1080);            
        }

        if (img.empty()) {
            std::cerr << "Error: Image load failed!" << std::endl;
            return 1;
        }

        detector->initialize(img);
        
        CarMovement mv;
        // Unpack Dynamics_State_Proxy
        const unsigned char *raw = sp._data;
        if (!raw) {
            std::cerr << "Dynamics state has no data!" << std::endl;
        } else {
            const unsigned char *p = raw + 1; // skip first control byte

            int32_t loc[3] = {0,0,0};
            float speed_f = 0.f, heading_f = 0.f, yawr_f = 0.f, accel_f = 0.f;
            char id_buf[9] = {0};
            int32_t uncertainty_i = 0, obj_class_i = 0;
            int64_t ts_i = 0;

            std::memcpy(&loc[0], p + 0,  4);
            std::memcpy(&loc[1], p + 4,  4);
            std::memcpy(&loc[2], p + 8,  4);
            std::memcpy(&speed_f, p + 12, 4);
            std::memcpy(&heading_f, p + 16, 4);
            std::memcpy(&yawr_f, p + 20, 4);
            std::memcpy(&accel_f, p + 24, 4);
            std::memcpy(id_buf,      p + 28, 8);
            std::memcpy(&uncertainty_i, p + 36, 4);
            std::memcpy(&obj_class_i,   p + 40, 4);
            std::memcpy(&ts_i,       p + 44, 8);

            mv.speed = speed_f;
            mv.timestamp = static_cast<double>(ts_i); // adjust units if ts is in microseconds/nanoseconds
        }

        DetectionResult result = detector->detect(mv, img);
        std::string classification = result.classification;

        auto out = static_cast<float>(classes[classification]);
        std::cout << "Vibration Detection Result: " << classification << " - " << out << std::endl;

        db<SmartData>(TRC) << "Visual_Vibration_Detection Awaiting response from Algorithm..." << endl;
        _value = pack_data(static_cast<float>(out));
        return true;
    }

    cv::Mat convertRawToMat(const uint8_t* raw_data, int width, int height) {
        // Converte o buffer raw BGR para cv::Mat
        cv::Mat mat(height, width, CV_8UC3, (void*)raw_data);

        // Converte de BGR para RGB
        cv::Mat mat_rgb;
        cv::cvtColor(mat, mat_rgb, cv::COLOR_BGR2RGB);
        return mat_rgb;
    }

    cv::Mat convertSmartDataToMat(const typename Camera_AV_Proxy::Value& in) {
        constexpr int width  = 1920;
        constexpr int height = 1080;

        // Converte o buffer raw BGR para cv::Mat
        cv::Mat mat(height, width, CV_8UC3, (void*)in._data);

        // Verifica o tamanho do buffer
        size_t buffer_size = width * height * 3;
        std::cout << "Buffer size matches cv::Mat? " 
                << ((mat.total() * mat.elemSize() == buffer_size) ? "Yes" : "No") 
                << std::endl;

        // Converte de BGR para RGB
        cv::Mat mat_rgb;
        cv::cvtColor(mat, mat_rgb, cv::COLOR_BGR2RGB);

        return mat_rgb;
    }

private:
    Camera_AV_Proxy *_input;
    Dynamics_State_Proxy *_sp;
    Value _value;
    Device_Id _dev;
    SmartData::Time _last_consumption;
    VibrationDetection *detector;
    double timestamp = 0.0;
};

template<unsigned int T_dev>
using Visual_Vibration_Detection = Responsive_SmartData<Visual_Vibration_Detection_Transformer<T_dev>>;
using Visual_Vibration_Detection_Camera = Visual_Vibration_Detection<17>;
using Visual_Vibration_Detection_Camera_File = Visual_Vibration_Detection<52>;

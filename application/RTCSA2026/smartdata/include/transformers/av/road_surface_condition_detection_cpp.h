#pragma once
#include <transformer.h>
#include "road_surface_condition_detector.hpp"
#include <cstdint>
#include <array>
#include <tuple>
#include <map>
#include <algorithm> // for std::clamp


using Camera_AV_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::RAW_BGR|1)>>;
using Dynamics_State_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::MOTION_VECTOR_LOCAL|12<<16|1)>>;
using Steer_Source_Proxy = Interested_SmartData<SmartData::Unit::Wrap<(SmartData::Unit::Angle | SmartData::Unit::F32)>>;

template<unsigned int source_dev>
class Road_Surface_Condition_Detection_Transformer : public Transducer<SmartData::Unit::ROAD_SURFACE_CONDITION>, private Observer
{
    friend Responsive_SmartData<Road_Surface_Condition_Detection_Transformer<17>>;
    friend Responsive_SmartData<Road_Surface_Condition_Detection_Transformer<55>>;

public:
    static const bool active = true;
    static const unsigned long long EXPIRY = 100000;
    static const Uncertainty UNCERTAINTY = UNKNOWN;
    static const Type TYPE = SENSOR;
    static const unsigned int DYNAMICS_DEV = 16;
    static const unsigned int STEER_DEV = 31;

    typedef __UTIL::Observer Observer;


public:
    Road_Surface_Condition_Detection_Transformer(const Device_Id & dev) 
        : _value(), _dev(dev)// inicializa detector
    {
        detector = new RoadSurfaceConditionDetector();
        _steer = new Steer_Source_Proxy(Steer_Source_Proxy::Region(0, 0, 0, 100, Dynamics_State_Proxy::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, STEER_DEV);
        _sp = new Dynamics_State_Proxy(Dynamics_State_Proxy::Region(0, 0, 0, 100, Dynamics_State_Proxy::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, DYNAMICS_DEV);
        _sp->attach(this);
        _steer->attach(this);

        db<SmartData>(TRC) << "Road_Surface_Condition_Detection_Transformer Interest in dynamic state created!" << DYNAMICS_DEV << ",u=" << _input->UNIT << endl;
    
        if (source_dev != _dev) { // A regular Transformer (relies on other sensor)
            // inputs
            _input = new Camera_AV_Proxy(Camera_AV_Proxy::Region(0, 0, 0, 100, Camera_AV_Proxy::now(), INFINITE), EXPIRY, 0, SmartData::SINGLE, SmartData::ANY, source_dev);
            db<SmartData>(INF) << "Road_Surface_Condition_Detection_Transformer Interest created!" << source_dev << ",u=" << _input->UNIT << endl;
            // attach my inputs to trigger my update
            _input->attach(this);
        }
        _last_consumption = 0;
    }

    ~Road_Surface_Condition_Detection_Transformer() { db<SmartData>(TRC) << "~Road_Surface_Condition_Detection - d=" << _dev << endl; delete _sp; if (source_dev != _dev) { delete _input;  } }

    virtual Value sense() { return _value; }
    virtual SmartData::Signature signature() { return 0; }

    void update(typename __UTIL::Observed *obs) {
        if (source_dev != _dev) {
            db<SmartData>(TRC) << "Road_Surface_Condition_Detection update! - d=" << _dev << "," << _last_consumption << _sp->when() << "," << _input->when() << endl;
            if (_input->when() > _last_consumption &&
                _sp->when() > _last_consumption &&
                _steer->when() > _last_consumption) {
                transform();
                _last_consumption = _sp->now();
                notify();
            }
        } else {
            db<SmartData>(TRC) << "Road_Surface_Condition_Detection update! - d=" << _dev << "," << _last_consumption << "," << _sp->when() << endl;
            if (_sp->when() > _last_consumption &&
                _steer->when() > _last_consumption) {
                transform();
                _last_consumption = _sp->now();
                notify();   
            }
        }
    }

private:


    // Define enums to match Python classes
    enum Friction {
        F_UNKNOWN = 0, F_DRY, F_WET, F_WATER, F_FRESH_SNOW, F_MELTED_SNOW, F_ICE
    };

    enum Material {
        M_UNKNOWN = 0, M_ASPHALT, M_CONCRETE, M_GRAVEL, M_MUD
    };

    enum Unevenness {
        U_UNKNOWN = 0, U_SMOOTH, U_SLIGHT, U_SEVERE
    };

    // Map tuple<friction, material, unevenness> -> index
    std::map<std::tuple<Friction, Material, Unevenness>, uint8_t> ENUM_ROAD_SURFACE_CONDITION;

    // Initialize the map (like Python nested loops)
    void init_enum_map() {
        uint8_t idx = 0;
        for(int f = F_UNKNOWN; f <= F_ICE; f++) {
            for(int m = M_UNKNOWN; m <= M_MUD; m++) {
                for(int u = U_UNKNOWN; u <= U_SEVERE; u++) {
                    ENUM_ROAD_SURFACE_CONDITION[{static_cast<Friction>(f),
                                                static_cast<Material>(m),
                                                static_cast<Unevenness>(u)}] = idx++;
                }
            }
        }
    }

    // Helper to pack classification data into 4 bytes
    uint32_t pack_data(const std::array<std::map<std::string, float>, 2>& classification) {
        // Left wheel
        Friction friction_l = static_cast<Friction>(classification[0].at("friction_class"));
        Material material_l = static_cast<Material>(classification[0].at("material_class"));
        Unevenness unevenness_l = static_cast<Unevenness>(classification[0].at("unevenness_class"));
        uint8_t class_left = ENUM_ROAD_SURFACE_CONDITION[{friction_l, material_l, unevenness_l}];

        // Right wheel
        Friction friction_r = static_cast<Friction>(classification[1].at("friction_class"));
        Material material_r = static_cast<Material>(classification[1].at("material_class"));
        Unevenness unevenness_r = static_cast<Unevenness>(classification[1].at("unevenness_class"));
        uint8_t class_right = ENUM_ROAD_SURFACE_CONDITION[{friction_r, material_r, unevenness_r}];

        // Friction coefficient * 100 (clamped to 0-255)
        uint8_t fc_left  = static_cast<uint8_t>(std::clamp(classification[0].at("friction_coefficient") * 100, 0.0f, 255.0f));
        uint8_t fc_right = static_cast<uint8_t>(std::clamp(classification[1].at("friction_coefficient") * 100, 0.0f, 255.0f));

        // Compacta em 32 bits (mesma ordem que no Python: fc_left, fc_right, class_left, class_right)
        uint32_t packed = 0;
        packed |= (fc_left   & 0xFF) << 24;
        packed |= (fc_right  & 0xFF) << 16;
        packed |= (class_left & 0xFF) << 8;
        packed |= (class_right & 0xFF);

        return packed;
    }
    bool transform() {
        double ego_steer_angle = *_steer;
        cv::Mat img;
        std::cout << "RUNNING DETECTOR ON TRANSFORM!" << std::endl;
        if(source_dev != _dev) {
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
        auto predictions = detector->detect(img, ego_steer_angle);

        std::array<std::map<std::string, float>, 2> classification;
        std::map<std::string, float> friction_map = {
            {"Unknown", 0.0f}, {"dry", 1.0f}, {"wet", 2.0f}, {"water", 3.0f},
            {"fresh_snow", 4.0f}, {"melted_snow", 5.0f}, {"ice", 6.0f}
        };

        std::map<std::string, float> material_map = {
            {"Unknown", 0.0f}, {"asphalt", 1.0f}, {"concrete", 2.0f},
            {"gravel", 3.0f}, {"mud", 4.0f}
        };

        std::map<std::string, float> unevenness_map = {
            {"Unknown", 0.0f}, {"smooth", 1.0f}, {"slight", 2.0f}, {"severe", 3.0f}
        };

        std::map<std::string, float> class_map = {{"dry", 0.0}, {"wet", 1.0}, {"snow", 2.0}};
        
        classification[0] = {
            {"friction_class",   static_cast<float>(friction_map[predictions.first.friction_class])},
            {"material_class",   static_cast<float>(material_map[predictions.first.material_class])},
            {"unevenness_class", static_cast<float>(unevenness_map[predictions.first.unevenness_class])},
            {"friction_coefficient", predictions.first.friction_coefficient}
        };

        classification[1] = {
            {"friction_class",   static_cast<float>(friction_map[predictions.second.friction_class])},
            {"material_class",   static_cast<float>(material_map[predictions.second.material_class])},
            {"unevenness_class", static_cast<float>(unevenness_map[predictions.second.unevenness_class])},
            {"friction_coefficient", predictions.second.friction_coefficient}
        };


        db<SmartData>(TRC) << "Road_Surface_Condition_Detection Awaiting response from Algorithm..." << endl;
        std::cout << "Left Wheel - Friction: " << classification[0]["friction_class"]
                  << ", Material: " << classification[0]["material_class"]
                  << ", Unevenness: " << classification[0]["unevenness_class"]
                  << ", Coefficient: " << classification[0]["friction_coefficient"] << std::endl;
        std::cout << "Right Wheel - Friction: " << classification[1]["friction_class"]
                  << ", Material: " << classification[1]["material_class"]
                  << ", Unevenness: " << classification[1]["unevenness_class"]
                  << ", Coefficient: " << classification[1]["friction_coefficient"] << std::endl;
        _value = pack_data(classification);

        return true;
    }

    cv::Mat convertRawToMat(const uint8_t* raw_data, int width, int height) {
        // Converte o buffer raw BGR para cv::Mat
        cv::Mat mat(height, width, CV_8UC3, (void*)raw_data);
        return mat;
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
    Steer_Source_Proxy *_steer;
    Value _value;
    Device_Id _dev;
    SmartData::Time _last_consumption;
    RoadSurfaceConditionDetector *detector;

};

template<unsigned int T_dev>
using Road_Surface_Condition_Detection = Responsive_SmartData<Road_Surface_Condition_Detection_Transformer<T_dev>>;
using Road_Surface_Condition_Detection_Camera = Road_Surface_Condition_Detection<17>;
using Road_Surface_Condition_Detection_Camera_File = Road_Surface_Condition_Detection<55>;

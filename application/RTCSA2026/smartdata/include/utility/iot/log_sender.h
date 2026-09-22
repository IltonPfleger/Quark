#pragma once
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <lib/cpp-httplib/httplib.h>
#undef CPPHTTPLIB_OPENSSL_SUPPORT
#include <pthread.h>

namespace IoTLogs
{

struct IoTSmartData {
    static constexpr const char* version = "1.2";
    char value[64];
    UInt32 unit;
    UInt32 dev;
    UInt32 error;
    UInt32 confidence;
    UInt32 signature;
    UInt32 workflow;
    UInt32 r;
    UInt64 t;
    Int32 x;
    Int32 y;
    Int32 z;

    void get(char* ptr)
    {
        sprintf(ptr,
                "{\"version\": %s,"
                "\"unit\": %lu,"
                "\"value\": %s,"
                "\"error\": %lu,"
                "\"confidence\": %lu,"
                "\"signature\": %lu,"
                "\"x\": %ld,"
                "\"y\": %ld,"
                "\"z\": %ld,"
                "\"r\": %lu,"
                "\"t\": %llu,"
                "\"dev\": %lu,"
                "\"workflow\": %lu}",
                version, unit, value, error, confidence, signature, x, y, z, r, t, dev, workflow);
    }
};

struct IoTSeries {
    static constexpr const char* version = "1.2";
    UInt32 unit;
    UInt32 dev;
    UInt32 signature;
    UInt64 t0;
    UInt64 t1;
    UInt32 r;
    Int32 x;
    Int32 y;
    Int32 z;

    void get(char* ptr)
    {
        sprintf(ptr,
                "{\"series\": {"
                "\"version\": %s,"
                "\"unit\": %lu,"
                "\"x\": %ld,"
                "\"y\": %ld,"
                "\"z\": %ld,"
                "\"r\": %lu,"
                "\"t0\": %llu,"
                "\"t1\": %llu,"
                "\"dev\": %lu,"
                "\"signature\": %lu}}",
                version, unit, x, y, z, r, t0, t1, dev, signature);
    }
};

class IoTBuffer
{
   public:
    static constexpr int MAX_SIZE = 1000;
    IoTBuffer()                   = default;
    ~IoTBuffer()                  = default;

    int with_location_size() { return _with_location_size; }

    int add_to_waiting_for_location(IoTSmartData sd)
    {
        int full = 0;
        pthread_mutex_lock(&_waiting_for_location_mutex);
        if (_waiting_for_location_size < MAX_SIZE)
            _waiting_for_location[_waiting_for_location_size++] = sd;
        else
            full = 1;
        pthread_mutex_unlock(&_waiting_for_location_mutex);
        return full;
    }

    int add_to_with_location(IoTSmartData SD)
    {
        int full = 0;
        pthread_mutex_lock(&_with_location_mutex);
        if (_with_location_size < MAX_SIZE)
            _with_location[_with_location_size++] = SD;
        else
            full = 1;
        pthread_mutex_unlock(&_with_location_mutex);
        return full;
    }

    void clear_with_location()
    {
        pthread_mutex_lock(&_with_location_mutex);
        _with_location_size = 0;
        pthread_mutex_unlock(&_with_location_mutex);
    }

    void set_location(double x, double y, double z)
    {
        pthread_mutex_lock(&_waiting_for_location_mutex);
        for (int i = 0; i < _waiting_for_location_size; i++) {
            IoTSmartData SD = _waiting_for_location[i];
            SD.x            = x;
            SD.y            = y;
            SD.z            = z;
            add_to_with_location(SD);
        }
        _waiting_for_location_size = 0;
        pthread_mutex_unlock(&_waiting_for_location_mutex);
    }

    void get_smartdata(char* SDs)
    {
        sprintf(SDs, "%s", "{\"smartdata\": [");
        char SD[1024];
        pthread_mutex_lock(&_with_location_mutex);
        for (int i = 0; i < _with_location_size - 1; i++) {
            _with_location[i].get(SD);
            strcat(SDs, SD);
            strcat(SDs, ", ");
        }
        _with_location[_with_location_size - 1].get(SD);
        strcat(SDs, SD);
        pthread_mutex_unlock(&_with_location_mutex);
        strcat(SDs, "]}");
    }

   private:
    pthread_mutex_t _with_location_mutex        = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t _waiting_for_location_mutex = PTHREAD_MUTEX_INITIALIZER;
    volatile int _waiting_for_location_size     = 0;
    volatile int _with_location_size            = 0;
    struct IoTSmartData _waiting_for_location[MAX_SIZE];
    struct IoTSmartData _with_location[MAX_SIZE];
};

class IoTSender
{
   public:
    IoTSender()
    {
        _client = new httplib::SSLClient(HOST, HTTPS_PORT);
        _buffer = new IoTBuffer();
        _client->enable_server_certificate_verification(false);
        SSL_CTX* ctx = _client->ssl_context();
        if (ctx) {
            if (!SSL_CTX_use_certificate_file(ctx, PEM_PATH, SSL_FILETYPE_PEM)) {
                db<SmartData>(ERR) << "IotSender: Failed To Load Client .pem Certificate!" << endl;
            }
            if (!SSL_CTX_use_PrivateKey_file(ctx, KEY_PATH, SSL_FILETYPE_PEM)) {
                db<SmartData>(ERR) << "IoTSender: Failed To Load Client .key Private Key!" << endl;
            }
        } else {
            db<SmartData>(ERR) << "IoTSender: Cannot Get SSL Context!" << endl;
        }
        pthread_create(&_asynchronous_sender, NULL, &IoTSender::asynchronous_sender_thread, (void*)this);
    }

    ~IoTSender()
    {
        exit = true;
        pthread_join(_asynchronous_sender, NULL);
        delete _client;
        delete _buffer;
    }

    void add_to_queue(IoTSmartData SD) { _buffer->add_to_waiting_for_location(SD); }

    int create_series(IoTSeries series)
    {
        char str[512];
        series.get(str);
        return send(CREATE_API.c_str(), str);
    }

    void new_location(double x, double y, double z) { _buffer->set_location(x, y, z); }

   private:
    static void* asynchronous_sender_thread(void* ptr)
    {
        IoTSender* sender = static_cast<IoTSender*>(ptr);
        char* SDs         = new char[IoTBuffer::MAX_SIZE * 256];
        while (!sender->exit) {
            if (sender->_buffer->with_location_size() < IoTBuffer::MAX_SIZE / 4) continue;
            sender->_buffer->get_smartdata(SDs);
            int error = sender->send(PUT_API.c_str(), SDs);
            if (!error) sender->_buffer->clear_with_location();
        }
        delete SDs;
        return nullptr;
    }

    int send(const char* api, const char* data)
    {
        db<SmartData>(LOGGER) << "Sending Data... >> ";
        httplib::Result result = _client->Post(api, data, "payloadlication/json");
        if (result) {
            db<SmartData>(LOGGER) << result->status;
            if (result->status < 300) {
                db<SmartData>(LOGGER) << " Done!" << endl;
                return 0;
            }
        }
        db<SmartData>(ERR) << " Failed!" << endl;
        return 1;
    }

    static const std::string HOST;
    static const std::string CREATE_API;
    static const std::string PUT_API;
    static const std::string GET_PATH;
    static const char KEY_PATH[];
    static const char PEM_PATH[];
    static const int HTTPS_PORT;
    volatile bool exit = false;
    pthread_t _asynchronous_sender;
    IoTBuffer* _buffer;
    httplib::SSLClient* _client;
};

const std::string IoTSender::HOST       = "deviot.setic.ufsc.br";
const std::string IoTSender::CREATE_API = "/api/create.php";
const std::string IoTSender::PUT_API    = "/api/put.php";
const std::string IoTSender::GET_PATH   = "/api/create.php";
const char IoTSender::KEY_PATH[]        = "./bin/cloud/certificates/sdav.key";
const char IoTSender::PEM_PATH[]        = "./bin/cloud/certificates/sdav.pem";
const int IoTSender::HTTPS_PORT         = 443;
};  // namespace IoTLogs

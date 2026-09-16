#pragma once

// EPOS Trustful SpaceTime Protocol Manager Declarations

#define __tstp__ 1

#ifdef __tstp__


class TSTP::Manager: private SmartData, private Data_Observer<Buffer>
{
    friend class TSTP;

private:
    static const bool forwarder = true;
    static const bool drop_expired = true;

    static const UInt32 RANGE = Traits<TSTP>::RADIO_RANGE;

    // Model Control Message
    class Model: public Control
    {
    protected:
        typedef unsigned char Data[MTU - sizeof(Header)];

    public:
        template<typename M>
        Model(const Region & dst, const M & model)
        : Control(dst, 0, 0, 0, EPOCH), _model(model) {}

        Region destination() const { return Region(_origin, _radius, _t1); }

        template<typename T>
        T * model() { return reinterpret_cast<T *>(&_model); }
        template<typename T>
        void model(const T & m) { *reinterpret_cast<T *>(&_model) = m; }

        friend Debug & operator<<(Debug & db, const Model & m) {
            db << reinterpret_cast<const Control &>(m) << ",d=" << m.region() << ",e=" << m._model;
            return db;
        }

    private:
        Data _model;
    } __attribute__((packed));

//    // TSTP Smart Data bindings
//    // Predictive (binder between Model messages and Smart Data)
//    class Predictive: public Model
//    {
//    public:
//        template<typename M>
//        Predictive(const M & model, const Unit & unit, const Error & error = 0, const Time & expiry = 0)
//        : Model(model, unit, error, expiry), _size(sizeof(Model) - sizeof(Model::Data) + sizeof(M)) { }
//
//        void t0(const Time & t) { _t0 = t; }
//        void t1(const Time & t) { _t1 = t; }
//        Time t0() const { return _t0; }
//        Time t1() const { return _t1; }
//
//        void respond(const Time & expiry) { send(expiry); }
//
//    private:
//        void send(const Time & expiry) {
//            if((_origin.time() >= _t0) && (_origin.time() <= _t1)) {
//                assert(expiry > now());
//                Buffer * buf = alloc(_size);
//                Model * model = buf->frame()->data<Model>();
//                memcpy(model, this, _size);
//                model->expiry(expiry);
//                marshal(buf);
//                _nic->send(buf);
//            }
//        }
//    private:
//        UInt32 _size;
//        Time _t0;
//        Time _t1;
//    };

public:
    Manager();
    ~Manager();

    bool synchronized();

    static Region destination(Buffer * buf);

private:
    void update(Data_Observed<Buffer> * obs, Buffer * buf);

    static void marshal(Buffer * buf);
};

#endif

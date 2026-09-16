#pragma once

// EPOS Trustful SpaceTime Protocol Locator Declarations

#define __tstp__ 1

#ifdef __tstp__

#include <network/hecops.h>

class TSTP::Locator: private SmartData, private Data_Observer<Buffer>
{
    friend class TSTP;
private:
    typedef HeCoPS<Space, 3> Engine;

public:
    Locator();
    ~Locator();

    static const Space & here() { return _engine.here(); }
    static const Percent & confidence() { return _engine.confidence(); }
    static const Global_Space & reference() { return _reference; }

    static Global_Space absolute(const Space & s) { return _reference + Global_Space(s); }
    static Global_Space absolute(const Spacetime & st) { return _reference + Global_Space(st.space); }
    static Space relative(const Global_Space & s) { Global_Space tmp = s; tmp -= _reference; return tmp; } // note that Point - Point returns the distance, that's why the -= is necessary here

private:
    void update(Data_Observed<Buffer> * obs, Buffer * buf);

    static void marshal(Buffer * buf);

private:
    static Global_Space _reference;
    static Engine _engine;

};

#endif

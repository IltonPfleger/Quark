// EPOS Trustful SpaceTime Protocol Initialization

#define __tstp__ 1

#ifdef __tstp__

#include <machine/udpnic.h>
#include <main_traits.h>
#include <network/tstp/tstp.h>

TSTP *TSTP::_tstp;
// UDP_Socket* UDPNIC::soc;
UDPNIC::_AES UDPNIC::_aes;
unsigned char UDPNIC::GRP_KEY[16];

TSTP::TSTP(NIC<NIC_Family> *nic) {
    db<Init, TSTP>(TRC) << "TSTP(nic=" << nic << ")" << endl;

    _nic = nic;
    _nic->attach(this, PROTO_TSTP);

    // The order parts are created defines the order they get notified when packets arrive:
    // mac->security(decrypt)->locator->timekeeper->router->manager->security(encrypt)->mac
    _security   = new /*(SYSTEM)*/ Security;
    _locator    = new /*(SYSTEM)*/ Locator;
    _timekeeper = new /*(SYSTEM)*/ Timekeeper; // here() reports (0,0,0) if _locator wasn't created first!
    _router     = new /*(SYSTEM)*/ Router;
    _manager    = new /*(SYSTEM)*/ Manager;
}

TSTP::Security::Security() {
    db<TSTP>(TRC) << "TSTP::Security() -- NO SECURITY BOOTSTRAP ENABLED, USING FIXED UUID config" << endl;

    // unsigned char uuid[8] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x05, 0x07, 0x08 };

    // _id = new /*(&_id)*/ Node_Id(/*Machine::*/uuid/*()*/, sizeof(UUID));

    db<TSTP>(INF) << "TSTP::Security:uuid=" << _id << endl;

    // TODO Hardcoded MasterSecret, disabling OTP Poly
    // _aes.encrypt(_id, _id, _auth);
    // Peer *peer = new Peer(_id, Region(here(), 0, now(), Time(-1UL)));
    // peer->master_secret(_DH::Shared_Key(reinterpret_cast<void *>(&_auth), KEY_SIZE));
    // _trusted_peers.insert(peer->link());

    attach_part(this);

    // initialization continues through update as relevant packets are received

    /*
    if((TSTP::here() != TSTP::sink()) && (!Traits<Radio>::promiscuous)) {
        Peer * peer = new (SYSTEM) Peer(_id, Region(TSTP::sink(), 0, 0, -1));
        _pending_peers.insert(peer->link());

        // Wait for key establishment
        while(_trusted_peers.size() == 0)
            Thread::self()->yield();
    }
    */
}

TSTP::Locator::Locator() {
    db<TSTP>(TRC) << "TSTP::Locator()" << endl;

    // System_Info::Boot_Map * bm = &System::info()->bm;
    // if(bm->space_x != Space::UNKNOWN) {
    //     _engine.here(Space(bm->space_x, bm->space_y, bm->space_z));
    //     _engine.confidence(100);
    // } else {

    // TCB - usar valores diferentes para instancias diferentes.
    _engine.here(Space(0, 0, 0));
    _engine.confidence(100);
    //}

    attach_part(this);

    db<TSTP>(INF) << "TSTP::Locator:here=" << here();
    if (here() == sink())
        db<TSTP>(INF) << "[sink]" << endl;
    else
        db<TSTP>(INF) << "[node]" << endl;

    // Wait for spatial localization
    // TCB - I commented this code because confidence didn't get higher than 80 and initialization was stuck here.
    // while(confidence() < 80)
    //    Thread::/*self()->*/yield();

    // _absolute_location is initialized later through an Epoch message
}

TSTP::Timekeeper::Timekeeper() {
    db<TSTP>(TRC) << "TSTP::Timekeeper()" << endl;
    db<TSTP>(INF) << "TSTP::Timekeeper:timer accuracy = " << timer_accuracy() << " ppb" << endl;
    db<TSTP>(INF) << "TSTP::Timekeeper:timer frequency = " << timer_frequency() << " Hz" << endl;
    db<TSTP>(INF) << "TSTP::Timekeeper:maximum drift = " << MAX_DRIFT << " us" << endl;
    db<TSTP>(INF) << "TSTP::Timekeeper:sync period = " << sync_period() << " us" << endl;

    attach_part(this);

    _skew = 0;

    if (here() == sink())
        _next_sync = INFINITE; // just so that the sink will always have synchronized() returning true
    else {
        _next_sync = INFINITE;
        // TODO this thread creation introduces an error to time synchronization
        // 0;
        //  keep_alive();
        //  Microsecond period = static_cast<Microsecond>(sync_period());
        //  _life_keeper_handler = new /*(SYSTEM)*/ Function_Handler(&keep_alive);
        //  _life_keeper = new /*(SYSTEM)*/ Alarm(period, _life_keeper_handler, INFINITE);
        //  while(!synchronized())
        //      Thread::/*self()->*/yield();
    }
}

TSTP::Router::Router() {
    db<TSTP>(TRC) << "TSTP::Router()" << endl;

    attach_part(this);
}

TSTP::Manager::Manager() {
    db<TSTP>(TRC) << "TSTP::Manager()" << endl;

    attach_part(this);
}

void TSTP::init() {
    db<Init, TSTP>(TRC) << "TSTP::init()" << endl;

    _nic  = new UDPNIC();
    _tstp = new /*(SYSTEM)*/ TSTP(_nic); // leaked memory if we do not attribute tstp to a variable for later deletion
}

void TSTP::finish() {
    delete _tstp;
    delete _nic;
}

// template <typename Engine>
// TSTP::MAC<Engine, true> TSTP::MAC<Engine, true>::_instance;

// template <typename Engine>
// TSTP::MAC<Engine, false> TSTP::MAC<Engine, false>::_instance;

// template <typename Engine>
// TSTP::MAC<Engine, true> TSTP::MAC<Engine, true>::instance() {
//     return _instance;
// }

// template <typename Engine>
// TSTP::MAC<Engine, false> TSTP::MAC<Engine, false>::instance() {
//     return _instance;
// }

#endif

// EPOS Trustful Space-Time Protocol Implementation

// #include <system/config.h>

#define __tstp__ 1

#ifdef __tstp__

#include <main_traits.h>
#include <utility/math.h>
// #include <utility/string.h>
#include <machine/nic.h>
#include <network/tstp/tstp.h>

// __BEGIN_SYS

TSTP::Security *TSTP::_security;
TSTP::Timekeeper *TSTP::_timekeeper;
TSTP::Locator *TSTP::_locator;
TSTP::Router *TSTP::_router;
TSTP::Manager *TSTP::_manager;

NIC<TSTP::NIC_Family> *TSTP::_nic;
Data_Observed<TSTP::Buffer> TSTP::_parts;
Data_Observed<TSTP::Buffer> TSTP::_clients;

Debug &operator<<(Debug &db, const TSTP::Packet &p) {
    switch (p.type()) {
    case TSTP::INTEREST:
        db << reinterpret_cast<const TSTP::Interest &>(p);
        break;
    case TSTP::RESPONSE:
        db << reinterpret_cast<const TSTP::Response &>(p);
        break;
    case TSTP::COMMAND:
        db << reinterpret_cast<const TSTP::Command &>(p);
        break;
    case TSTP::CONTROL:
        switch (p.subtype()) {
        case TSTP::DH_RESPONSE:
            db << reinterpret_cast<const TSTP::Security::DH_Response &>(p);
            break;
        case TSTP::AUTH_REQUEST:
            db << reinterpret_cast<const TSTP::Security::Auth_Request &>(p);
            break;
        case TSTP::DH_REQUEST:
            db << reinterpret_cast<const TSTP::Security::DH_Request &>(p);
            break;
        case TSTP::AUTH_GRANTED:
            db << reinterpret_cast<const TSTP::Security::Auth_Granted &>(p);
            break;
        case TSTP::REPORT:
            db << reinterpret_cast<const TSTP::Security::Report &>(p);
            break;
        case TSTP::KEEP_ALIVE:
            db << reinterpret_cast<const TSTP::Timekeeper::Keep_Alive &>(p);
            break;
        case TSTP::EPOCH:
            db << reinterpret_cast<const TSTP::Timekeeper::Epoch &>(p);
            break;
            //        case TSTP::MODEL:
            //            db << reinterpret_cast<const TSTP::Model &>(p);
            //            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    db << "}";
    return db;
};

void TSTP::update(NIC_Family::Observed *obs, const Protocol &prot, Buffer *buf) {

    // Packet *packet = buf->frame()->data<Packet>();
    // db<TSTP>(ERR) << "TSTP::update:packet=" << *packet << endl;

    // db<TSTP>(TRC) << "TSTP::update(nic=" << obs << ",prot=" << hex << prot << ",buf=" << buf << ")" << endl;
    Packet *packet = buf->frame()->data<Packet>();
    //db<TSTP>(ERR) << "SMARTDATA RECEIVED: " << *packet << endl;
    // db<TSTP>(INF) << "TSTP::update:packet=" << packet->header()->unit() << "," << packet->header()->device() <<
    // ",will notify=" << buf->destined_to_me << "," << buf->trusted << endl; db<TSTP>(INF) << "TSTP::sizeof(packet)="
    // << sizeof(*packet->header()) << ",buf-size()=" << buf->size() << endl;

    _parts.notify(buf);

    if (buf->destined_to_me && buf->trusted && packet->type() != CONTROL)
        _clients.notify(buf); // why was tstp bothering with unit here? --> space time seems ok, but unit is like
                              // opening SmartData casquet

    _nic->free(buf);
}

//    if(buf->is_microframe || !buf->trusted)
//        return;
//
//    Packet * packet = buf->frame()->data<Packet>();
//
//    bool is_model = (packet->type() == CONTROL && buf->frame()->data<Control>()->subtype() == MODEL);
//
//    if(is_model && buf->frame()->data<Control>()->origin() != here()) {
//        Model * model = reinterpret_cast<Model *>(packet);
//        if(model->time() < now()) {
//            // Check region inclusion and notify interested observers
//            Interests::List * list = _interested[model->unit()];
//            if(list && !list->empty()) {
//                for(Interests::Element * el = list->head(); el; el = el->next()) {
//                    Interested * interested = el->object();
//                    if(interested->region().contains(model->origin(), model->time()))
//                        notify(interested, buf);
//                }
//            } else {
//                Responsives::List * list_responsive = _responsives[model->unit()];
//                if(list_responsive && !list_responsive->empty()) {
//                    for(Responsives::Element * el = list_responsive->head(); el; el = el->next()) {
//                        Responsive * responsive = el->object();
//                        if(responsive->interest() && responsive->model_listener() &&
//                        responsive->interest()->region().contains(model->origin(), model->time())){
//                            notify(responsive, buf);
//                        }
//                    }
//                }
//            }
//        }
//    }
//
//    if(packet->time() > now() || !buf->destined_to_me)
//        return;
//
//    switch(packet->type()) {
//    case INTEREST: {
//        Interest * interest = reinterpret_cast<Interest *>(packet);
//        db<TSTP>(INF) << "TSTP::update:interest=" << interest << " => " << *interest << endl;
//        // Check for local capability to respond and notify interested observers
//        Responsives::List * list = _responsives[interest->unit()]; // TODO: What if sensor can answer multiple formats
//        (e.g. int and float) if(list)
//            for(Responsives::Element * el = list->head(); el; el = el->next()) {
//                Responsive * responsive = el->object();
//                if((now() < interest->region().t1) && interest->region().contains(responsive->origin(),
//                interest->region().t1))
//                    notify(responsive, buf);
//            }
//    } break;
//    case RESPONSE: {
//        Response * response = reinterpret_cast<Response *>(packet);
//        db<TSTP>(INF) << "TSTP::update:response=" << response << " => " << *response << endl;
//        if(response->time() < now()) {
//            // Check region inclusion and notify interested observers
//            Interests::List * list = _interested[response->unit()];
//            if(list)
//                for(Interests::Element * el = list->head(); el; el = el->next()) {
//                    Interested * interested = el->object();
//                    if(interested->region().contains(response->origin(), response->time()))
//                        notify(interested, buf);
//                }
//        }
//    } break;
//    case COMMAND: {
//        Command * command = reinterpret_cast<Command *>(packet);
//        db<TSTP>(INF) << "TSTP::update:command=" << command << " => " << *command << endl;
//        // Check for local capability to respond and notify interested observers
//        Responsives::List * list = _responsives[command->unit()]; // TODO: What if sensor can answer multiple formats
//        (e.g. int and float) if(list)
//            for(Responsives::Element * el = list->head(); el; el = el->next()) {
//                Responsive * responsive = el->object();
//                if(command->region().contains(responsive->origin(), now()))
//                    notify(responsive, buf);
//            }
//    } break;
//    case CONTROL: {
//        switch(buf->frame()->data<Control>()->subtype()) {
//            case DH_REQUEST:
//                db<TSTP>(INF) << "TSTP::update: DH_Request: " << *buf->frame()->data<DH_Request>() << endl;
//                break;
//            case DH_RESPONSE:
//                db<TSTP>(INF) << "TSTP::update: DH_Response: " << *buf->frame()->data<DH_Response>() << endl;
//                break;
//            case AUTH_REQUEST:
//                db<TSTP>(INF) << "TSTP::update: Auth_Request: " << *buf->frame()->data<Auth_Request>() << endl;
//                break;
//            case AUTH_GRANTED:
//                db<TSTP>(INF) << "TSTP::update: Auth_Granted: " << *buf->frame()->data<Auth_Granted>() << endl;
//                break;
//            case REPORT: {
//                db<TSTP>(INF) << "TSTP::update: Report: " << *buf->frame()->data<Report>() << endl;
//                Report * report = reinterpret_cast<Report *>(packet);
//                if(report->time() < now()) {
//                    // Check region inclusion and advertise interested observers
//                    Interests::List * list = _interested[report->unit()];
//                    if(list)
//                        for(Interests::Element * el = list->head(); el; el = el->next()) {
//                            Interested * interested = el->object();
//                            if(interested->region().contains(report->origin(), report->time()))
//                                interested->advertise();
//                        }
//                    if(report->epoch_request() && (here() == sink())) {
//                        db<TSTP>(TRC) << "TSTP::update: responding to Epoch request" << endl;
//                        Buffer * buf = alloc(sizeof(Epoch));
//                        Epoch * epoch = new (buf->frame()->data<Epoch>()) Epoch(Region(report->origin(), 0, now(),
//                        now() + 10000000)); // TODO: what's the expiry? marshal(buf); db<TSTP>(INF) <<
//                        "TSTP::update:epoch = " << epoch << " => " << (*epoch) << endl; _nic->send(buf);
//                    }
//                }
//            } break;
//            case KEEP_ALIVE:
//                db<TSTP>(INF) << "TSTP::update: Keep_Alive: " << *buf->frame()->data<Keep_Alive>() << endl;
//                break;
//            case EPOCH: {
//                db<TSTP>(INF) << "TSTP::update: Epoch: " << *buf->frame()->data<Epoch>() << endl;
//                Epoch * epoch = reinterpret_cast<Epoch *>(packet);
//                if(here() != sink()) {
//                    _global_coordinates = epoch->coordinates();
//                    _reference = epoch->epoch();
//                    db<TSTP>(INF) << "TSTP::update: Epoch: adjusted epoch Space-Time to: " << _global_coordinates <<
//                    ", " << _reference << endl;
//                }
//            } break;
//            default:
//                db<TSTP>(WRN) << "TSTP::update: Unrecognized Control subtype: " <<
//                buf->frame()->data<Control>()->subtype() << endl; break;
//        }
//    } break;
//    case MODEL: {
//        // Already treated
//    } break;
//    default:
//        db<TSTP>(WRN) << "TSTP::update: Unrecognized packet type: " << packet->type() << endl;
//        break;
//    }

void TSTP::marshal(Buffer *buf) {
    db<TSTP>(TRC) << "TSTP::marshal(buf=" << buf << ")" << endl;

    Manager::marshal(buf);
    Router::marshal(buf);
    Locator::marshal(buf);
    Timekeeper::marshal(buf);
    Security::marshal(buf);

    Packet *packet = buf->frame()->data<Packet>();
    db<TSTP>(INF) << "TSTP::marshal:packet=" << *packet << ",size=" << buf->size() << endl;
}

// __END_SYS

#endif

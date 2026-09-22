#ifndef __SDS__
#define __SDS__

#define ARTERY_PROJECT
#define NO_DATA_SOURCE

#include <Thread.hpp>
#include <boolean_filters.h>
#include <main_traits.h>
#include <seu.h>
#include <smartdata.h>
#include <transducer.h>
#include <transformer.h>

typedef void (*constructor_t)();

extern constructor_t __init_array_start[];
extern constructor_t __init_array_end[];

void call_global_constructors() {
    for (constructor_t *ctor = __init_array_start; ctor != __init_array_end; ++ctor) {
        (*ctor)();
    }
}

using DS          = Dynamics_State;
using DS_Proxy    = Interested_SmartData<DS::Unit::Wrap<DS::UNIT>>;
using OBRTF       = Object_Recognition_And_Tracking_Fuser;
using OBRTF_Proxy = Interested_SmartData<OBRTF::Unit::Wrap<(SmartData::Unit::MOTION_VECTOR_LOCAL | 10)>>;
using Device      = QUARK::Meta::GetFromTypeList<QUARK::Traits<QUARK::Ethernet>::Devices, 0>::Result;

class SDs {

  public:
    static void init() {
        call_global_constructors();
        TSTP::init();
        new QUARK::Thread(node, 0, QUARK::Thread::Criterion(QUARK::Thread::Criterion::NORMAL, 3));
    }

    static void *node(void *) {
        SEU_SmartData *seu = new SEU_SmartData();
        Road_Parameters rp = Road_Parameters(0, 0, 0, 0, 0);
        rp.set_default();
        Unit_Dev_Expiry::List *ud_list = new Unit_Dev_Expiry::List();
        ud_list->insert((new Unit_Dev_Expiry(Dynamics_State::UNIT, 16, 100000))->link());
        ud_list->insert((new Unit_Dev_Expiry(Object_Recognition_And_Tracking_Fuser::UNIT, 23, 100000))->link());
        RSS_Safe_Distance *rss = new RSS_Safe_Distance(ud_list, &rp, &rp, 100000);
        seu->add_boolean_filter(rss);

        QUARK::Alarm::udelay(5'000'000);

        OBRTF_Proxy b(OBRTF_Proxy::Region(0, 0, 0, 100, OBRTF_Proxy::now(), INFINITE), 300'000);
        DS_Proxy a(DS_Proxy::Region(0, 0, 0, 100, DS_Proxy::now(), INFINITE), 5'000);

        while (1)
            QUARK::Alarm::udelay(100'000'000);

        return nullptr;
    }
};

#endif

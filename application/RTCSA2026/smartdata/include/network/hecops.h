#pragma once

// EPOS Heuristic Cooperative Positioning System Declarations

#include <smartdata.h>

template<typename Space, UInt32 PEERS>
class HeCoPS
{
    friend class TSTP;

protected:
    static const Percent CONFIDENCE_TRASHOLD = 80;

public:
    typedef char RSSI;

    struct Peer {
        Space coordinates;
        Percent confidence;
        RSSI rssi;
    };

public:
    HeCoPS(const Space & h = Space(-1, -1, -1), const Percent & c = 0): _here(h), _confidence(c), _n_peers(0) {
        db<TSTP>(TRC) << "HeCoPS::HeCoPS()" << endl;
    }
    ~HeCoPS() {
        db<TSTP>(TRC) << "HeCoPS::~HeCoPS()" << endl;
    }

    const Space & here() const { return _here; }
    const Percent & confidence() const { return _confidence; }

    void learn(const Space & coordinates, const Percent & confidence, const RSSI & rssi) {
        db<TSTP>(INF) << "HeCoPS::learn(c=" << coordinates << ",conf=" << confidence << ",rssi=" << static_cast<int>(rssi) << ")" << endl;
        if(confidence < CONFIDENCE_TRASHOLD)
            return;
        UInt32 idx = -1u;

        for(UInt32 i = 0; i < _n_peers; i++) {
            if(_peers[i].coordinates == coordinates) {
                if(_peers[i].confidence > confidence)
                    return;
                else {
                    idx = i;
                    break;
                }
            }
        }

        if(idx == -1u) {
            if(_n_peers < 3)
                idx = _n_peers++;
            else
                for(UInt32 i = 0; i < _n_peers; i++)
                    if((_peers[i].confidence <= confidence) && ((idx == -1u) || (confidence >= _peers[i].confidence)))
                        idx = i;
        }

        if(idx != -1u) {
            _peers[idx].coordinates = coordinates;
            _peers[idx].confidence = confidence;
            _peers[idx].rssi = rssi;

            if(_n_peers >= 3) {
                _here.trilaterate(_peers[0].coordinates, _peers[0].rssi + 128, _peers[1].coordinates, _peers[1].rssi + 128, _peers[2].coordinates, _peers[2].rssi + 128);
                _confidence = (_peers[0].confidence + _peers[1].confidence + _peers[2].confidence) * CONFIDENCE_TRASHOLD / 100 / 3;
                db<TSTP>(INF) << "TSTP::Locator: Location updated: " << _here << ", confidence = " << _confidence << "%" << endl;
            }
        }
    }

    void forget(const Space & coordinates);

    bool synchronized() { return _confidence >= CONFIDENCE_TRASHOLD; }
    bool neigbor_synchronized(const Percent & confidence) { return confidence >= CONFIDENCE_TRASHOLD; }

private:
    void here(const Space & h) { _here = h; }
    void confidence(const Percent & c) { _confidence = c; }

private:
    Space _here;
    Percent _confidence;

    UInt32 _n_peers;
    Peer _peers[PEERS];
};

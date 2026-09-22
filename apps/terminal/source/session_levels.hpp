#pragma once

#include <string>

struct SessionTriplet {
    bool valid;
    double open;
    double high;
    double low;
};

struct SessionInstrumentLevels {
    bool ready;
    std::string dataDate;
    std::string lastBarEt;

    double pdh;
    double pdl;
    double pdc;

    double dayOpen;
    double dayHigh;
    double dayLow;

    SessionTriplet asia;
    SessionTriplet london;
    SessionTriplet newYork;

    bool sessionRangeValid;
    double sessionHigh;
    double sessionLow;
};

struct SessionLevelsData {
    int version;
    std::string status;
    std::string generatedUtc;
    std::string nowEt;
    std::string session;
    int sessionProgress;

    SessionInstrumentLevels nas100;
    SessionInstrumentLevels us30;
    SessionInstrumentLevels gold;
};

bool fetchSessionLevels(
    const std::string &host,
    int port,
    SessionLevelsData &levels,
    std::string &error
);

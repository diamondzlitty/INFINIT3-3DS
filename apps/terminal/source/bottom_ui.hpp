#pragma once

#include <3ds.h>

enum T6cBottomTarget {
    T6C_TARGET_NONE = -1,
    T6C_TARGET_NAS100 = 0,
    T6C_TARGET_US30 = 1,
    T6C_TARGET_GOLD = 2,
    T6C_TARGET_15M = 3,
    T6C_TARGET_30M = 4,
    T6C_TARGET_1H = 5,
    T6C_TARGET_ALERT = 6,
    T6C_TARGET_REFRESH = 7,
    T6C_TARGET_START = 8
};

struct T6bBottomModel {
    int market;
    int timeframe;
    int pressedTarget;

    bool dataConnected;
    bool candleConnected;
    bool macroConnected;
    bool levelsConnected;
    bool newsReady;
    bool selectedCandleValid;
    bool syncActive;
    bool degraded;
    bool cursorLatest;
    bool levelsPage;
    bool levelInstrumentReady;
    bool activeSessionValid;

    int cursorPosition;
    int candleCount;
    int marketPulseFrames;
    int faultSource;
    int nasDirection;
    int us30Direction;
    int goldDirection;
    int sessionProgress;
    const char *sessionName;

    double nas100;
    double us30;
    double gold;

    double candleOpen;
    double candleHigh;
    double candleLow;
    double candleClose;

    double dxy;
    double us2y;
    double us10y;
    double wti;
    double vix;
    double curve2s10s;

    double pdh;
    double pdl;
    double pdc;

    double dayOpen;
    double dayHigh;
    double dayLow;

    double sessionOpen;
    double sessionHigh;
    double sessionLow;
};

bool t6aBottomInit();
int t6cBottomHitTest(int x, int y);
void t6bBottomRender(const T6bBottomModel &model);

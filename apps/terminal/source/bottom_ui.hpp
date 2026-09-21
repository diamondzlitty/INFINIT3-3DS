#pragma once

#include <3ds.h>

struct T6bBottomModel {
    int market;
    int timeframe;
    bool dataConnected;
    double nas100;
    double us30;
    double gold;
};

bool t6aBottomInit();
void t6bBottomRender(const T6bBottomModel &model);

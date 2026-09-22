#include <3ds.h>
#include "bottom_ui.hpp"

#include <cstdio>
#include <cstring>

namespace {

constexpr int WIDTH = 320;
constexpr int HEIGHT = 240;
constexpr int STRIDE = 240;
constexpr int PIXELS = WIDTH * HEIGHT;

static u16 g_backbuffer[PIXELS];

static inline u16 gray(u8 value)
{
    return RGB8_to_565(value, value, value);
}

static inline u16 *framebuffer()
{
    return reinterpret_cast<u16 *>(
        gfxGetFramebuffer(
            GFX_BOTTOM,
            GFX_LEFT,
            nullptr,
            nullptr
        )
    );
}

static inline size_t pixelIndex(int x, int y)
{
    return
        static_cast<size_t>(x) * STRIDE +
        static_cast<size_t>(STRIDE - 1 - y);
}

static inline void putPixel(
    int x,
    int y,
    u16 color
)
{
    if (
        x < 0 || x >= WIDTH ||
        y < 0 || y >= HEIGHT
    ) {
        return;
    }

    g_backbuffer[pixelIndex(x, y)] = color;
}

static void fillScreen(u16 color)
{
    for (int i = 0; i < PIXELS; ++i) {
        g_backbuffer[i] = color;
    }
}

static void fillRect(
    int x,
    int y,
    int w,
    int h,
    u16 color
)
{
    for (int py = y; py < y + h; ++py) {
        for (int px = x; px < x + w; ++px) {
            putPixel(px, py, color);
        }
    }
}

static void drawHLine(
    int x,
    int y,
    int length,
    u16 color
)
{
    for (int px = x; px < x + length; ++px) {
        putPixel(px, y, color);
    }
}

static void drawVLine(
    int x,
    int y,
    int length,
    u16 color
)
{
    for (int py = y; py < y + length; ++py) {
        putPixel(x, py, color);
    }
}

static void drawRect(
    int x,
    int y,
    int w,
    int h,
    u16 color
)
{
    drawHLine(x, y, w, color);
    drawHLine(x, y + h - 1, w, color);
    drawVLine(x, y, h, color);
    drawVLine(x + w - 1, y, h, color);
}

// Original DSi 5x7 glyph set used by INFINIT3 Terminal.
static const u8 FONT_A[7] = {14,17,17,31,17,17,17};
static const u8 FONT_D[7] = {30,17,17,17,17,17,30};
static const u8 FONT_E[7] = {31,16,16,30,16,16,31};
static const u8 FONT_F[7] = {31,16,16,30,16,16,16};
static const u8 FONT_G[7] = {14,17,16,23,17,17,14};
static const u8 FONT_H[7] = {17,17,17,31,17,17,17};
static const u8 FONT_I[7] = {14,4,4,4,4,4,14};
static const u8 FONT_L[7] = {16,16,16,16,16,16,31};
static const u8 FONT_M[7] = {17,27,21,21,17,17,17};
static const u8 FONT_N[7] = {17,25,21,19,17,17,17};
static const u8 FONT_O[7] = {14,17,17,17,17,17,14};
static const u8 FONT_R[7] = {30,17,17,30,20,18,17};
static const u8 FONT_S[7] = {15,16,16,14,1,1,30};
static const u8 FONT_T[7] = {31,4,4,4,4,4,4};
static const u8 FONT_U[7] = {17,17,17,17,17,17,14};
static const u8 FONT_V[7] = {17,17,17,17,17,10,4};
static const u8 FONT_C[7] = {14,17,16,16,16,17,14};
static const u8 FONT_K[7] = {17,18,20,24,20,18,17};
static const u8 FONT_W[7] = {17,17,17,21,21,21,10};
static const u8 FONT_X[7] = {17,17,10,4,10,17,17};
static const u8 FONT_Y[7] = {17,17,10,4,4,4,4};

static const u8 FONT_0[7] = {14,17,19,21,25,17,14};
static const u8 FONT_1[7] = {4,12,4,4,4,4,14};
static const u8 FONT_2[7] = {14,17,1,2,4,8,31};
static const u8 FONT_3[7] = {30,1,1,14,1,1,30};
static const u8 FONT_4[7] = {2,6,10,18,31,2,2};
static const u8 FONT_5[7] = {31,16,16,30,1,1,30};
static const u8 FONT_6[7] = {14,16,16,30,17,17,14};
static const u8 FONT_7[7] = {31,1,2,4,8,8,8};
static const u8 FONT_8[7] = {14,17,17,14,17,17,14};
static const u8 FONT_9[7] = {14,17,17,15,1,1,14};

static const u8 FONT_DASH[7] = {0,0,0,31,0,0,0};
static const u8 FONT_DOT[7] = {0,0,0,0,0,6,6};

static const u8 *getGlyph(char c)
{
    switch (c) {
        case 'A': return FONT_A;
        case 'D': return FONT_D;
        case 'E': return FONT_E;
        case 'F': return FONT_F;
        case 'G': return FONT_G;
        case 'H': return FONT_H;
        case 'I': return FONT_I;
        case 'L': return FONT_L;
        case 'M': return FONT_M;
        case 'N': return FONT_N;
        case 'O': return FONT_O;
        case 'R': return FONT_R;
        case 'S': return FONT_S;
        case 'T': return FONT_T;
        case 'U': return FONT_U;
        case 'V': return FONT_V;
        case 'C': return FONT_C;
        case 'K': return FONT_K;
        case 'W': return FONT_W;
        case 'X': return FONT_X;
        case 'Y': return FONT_Y;

        case '0': return FONT_0;
        case '1': return FONT_1;
        case '2': return FONT_2;
        case '3': return FONT_3;
        case '4': return FONT_4;
        case '5': return FONT_5;
        case '6': return FONT_6;
        case '7': return FONT_7;
        case '8': return FONT_8;
        case '9': return FONT_9;

        case '-': return FONT_DASH;
        case '.': return FONT_DOT;
        default: return nullptr;
    }
}

static void drawGlyph(
    int x,
    int y,
    const u8 *glyph,
    int scale,
    u16 color
)
{
    if (!glyph) {
        return;
    }

    for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 5; ++col) {
            if (glyph[row] & (1 << (4 - col))) {
                fillRect(
                    x + col * scale,
                    y + row * scale,
                    scale,
                    scale,
                    color
                );
            }
        }
    }
}

static void drawText(
    int x,
    int y,
    const char *text,
    int scale,
    u16 color
)
{
    int cursorX = x;

    while (*text) {
        if (*text == ' ') {
            cursorX += 4 * scale;
        } else {
            drawGlyph(
                cursorX,
                y,
                getGlyph(*text),
                scale,
                color
            );
            cursorX += 6 * scale;
        }

        ++text;
    }
}

static int sx(int value)
{
    return (value * 5 + 2) / 4;
}

static int sy(int value)
{
    return (value * 5 + 2) / 4;
}

static void present()
{
    std::memcpy(
        framebuffer(),
        g_backbuffer,
        sizeof(g_backbuffer)
    );
}

static void formatPrice(
    char *out,
    size_t outSize,
    bool ready,
    double value
)
{
    if (!ready) {
        std::snprintf(out, outSize, "----");
        return;
    }

    std::snprintf(
        out,
        outSize,
        "%.2f",
        value
    );
}

}

bool t6aBottomInit()
{
    gfxSetDoubleBuffering(
        GFX_BOTTOM,
        false
    );

    return
        gfxGetScreenFormat(
            GFX_BOTTOM
        ) == GSP_RGB565_OES;
}

static bool t6cInside(
    int x,
    int y,
    int left,
    int top,
    int width,
    int height
)
{
    return
        x >= left &&
        x < left + width &&
        y >= top &&
        y < top + height;
}

static void formatMetric(
    char *buffer,
    size_t size,
    bool ready,
    double value
)
{
    if (!ready) {
        snprintf(buffer, size, "--");
        return;
    }

    snprintf(buffer, size, "%.2f", value);
}

static void formatLabeledMetric(
    char *buffer,
    size_t size,
    const char *label,
    bool ready,
    double value
)
{
    char valueText[20];

    formatMetric(
        valueText,
        sizeof(valueText),
        ready,
        value
    );

    snprintf(
        buffer,
        size,
        "%s %s",
        label,
        valueText
    );
}

int t6cBottomHitTest(int x, int y)
{
    if (t6cInside(x, y, sx(4),   sy(45),  sx(77), sy(30))) return T6C_TARGET_NAS100;
    if (t6cInside(x, y, sx(4),   sy(87),  sx(77), sy(30))) return T6C_TARGET_US30;
    if (t6cInside(x, y, sx(4),   sy(129), sx(77), sy(30))) return T6C_TARGET_GOLD;

    if (t6cInside(x, y, sx(200), sy(45),  sx(52), sy(30))) return T6C_TARGET_15M;
    if (t6cInside(x, y, sx(200), sy(87),  sx(52), sy(30))) return T6C_TARGET_30M;
    if (t6cInside(x, y, sx(200), sy(129), sx(52), sy(30))) return T6C_TARGET_1H;

    if (t6cInside(x, y, sx(8),   sy(169), sx(64), sy(18))) return T6C_TARGET_ALERT;
    if (t6cInside(x, y, sx(96),  sy(169), sx(64), sy(18))) return T6C_TARGET_REFRESH;
    if (t6cInside(x, y, sx(184), sy(169), sx(64), sy(18))) return T6C_TARGET_START;

    return T6C_TARGET_NONE;
}

void t6bBottomRender(
    const T6bBottomModel &model
)
{
    const u16 background = gray(24);
    const u16 normal = gray(49);
    const u16 selected = gray(99);
    const u16 pressed = gray(140);
    const u16 secondary = gray(170);
    const u16 white = gray(255);

    fillScreen(background);

    const u16 nasColor =
        model.pressedTarget == T6C_TARGET_NAS100
        ? pressed
        : (model.market == 0 ? selected : normal);

    const u16 us30Color =
        model.pressedTarget == T6C_TARGET_US30
        ? pressed
        : (model.market == 1 ? selected : normal);

    const u16 goldColor =
        model.pressedTarget == T6C_TARGET_GOLD
        ? pressed
        : (model.market == 2 ? selected : normal);

    const u16 fifteenColor =
        model.pressedTarget == T6C_TARGET_15M
        ? pressed
        : (model.timeframe == 0 ? selected : normal);

    const u16 thirtyColor =
        model.pressedTarget == T6C_TARGET_30M
        ? pressed
        : (model.timeframe == 1 ? selected : normal);

    const u16 oneHourColor =
        model.pressedTarget == T6C_TARGET_1H
        ? pressed
        : (model.timeframe == 2 ? selected : normal);

    const u16 alertColor =
        model.pressedTarget == T6C_TARGET_ALERT
        ? pressed
        : normal;

    const u16 refreshColor =
        model.pressedTarget == T6C_TARGET_REFRESH
        ? pressed
        : normal;

    const u16 startColor =
        model.pressedTarget == T6C_TARGET_START
        ? pressed
        : normal;

    // Header
    drawText(
        sx(77), sy(8),
        "INFINIT3 TERMINAL",
        1,
        white
    );

    drawHLine(
        sx(63), sy(23),
        sx(130),
        white
    );

    const char *stateLabel = "ONLINE";

    if (model.syncActive) {
        stateLabel = "SYNC";
    } else if (model.degraded) {
        stateLabel = "DEGRADED";
    } else if (model.marketPulseFrames > 12) {
        stateLabel = "FRESH";
    }

    const char *cursorLabel = "----";
    if (model.candleCount > 0 && model.cursorPosition > 0) {
        cursorLabel = model.cursorLatest ? "LIVE" : "HIST";
    }

    const char *faultLabel = "";
    if (model.faultSource == 1) {
        faultLabel = " CND";
    } else if (model.faultSource == 2) {
        faultLabel = " MKT";
    } else if (model.faultSource == 3) {
        faultLabel = " MAC";
    } else if (model.faultSource == 4) {
        faultLabel = " LVL";
    }

    char statusLine[80];
    snprintf(
        statusLine,
        sizeof(statusLine),
        "%s%s CND%d MKT%d MAC%d LVL%d NWS%d %s C%d-%d",
        stateLabel,
        faultLabel,
        model.candleConnected ? 1 : 0,
        model.dataConnected ? 1 : 0,
        model.macroConnected ? 1 : 0,
        model.levelsConnected ? 1 : 0,
        model.newsReady ? 1 : 0,
        cursorLabel,
        model.cursorPosition,
        model.candleCount
    );

    const u16 statusColor =
        (model.syncActive ||
         model.degraded ||
         model.marketPulseFrames > 12)
        ? white
        : secondary;

    drawText(
        sx(4), sy(29),
        statusLine,
        1,
        statusColor
    );

    // Preserve the proven DSi three-column geometry.
    drawVLine(
        sx(86), sy(39),
        sy(126),
        white
    );

    drawVLine(
        sx(196), sy(39),
        sy(126),
        white
    );

    // Market selectors.
    fillRect(
        sx(4), sy(45),
        sx(77), sy(30),
        nasColor
    );

    fillRect(
        sx(4), sy(87),
        sx(77), sy(30),
        us30Color
    );

    fillRect(
        sx(4), sy(129),
        sx(77), sy(30),
        goldColor
    );

    // Timeframe selectors.
    fillRect(
        sx(201), sy(45),
        sx(51), sy(30),
        fifteenColor
    );

    fillRect(
        sx(201), sy(87),
        sx(51), sy(30),
        thirtyColor
    );

    fillRect(
        sx(201), sy(129),
        sx(51), sy(30),
        oneHourColor
    );

    char nasPrice[24];
    char us30Price[24];
    char goldPrice[24];

    formatPrice(
        nasPrice,
        sizeof(nasPrice),
        model.dataConnected,
        model.nas100
    );

    formatPrice(
        us30Price,
        sizeof(us30Price),
        model.dataConnected,
        model.us30
    );

    formatPrice(
        goldPrice,
        sizeof(goldPrice),
        model.dataConnected,
        model.gold
    );

    // Stack each live price inside its own market selector.
    drawText(
        sx(8), sy(50),
        "NAS100",
        1,
        white
    );

    drawText(
        sx(8), sy(63),
        nasPrice,
        1,
        white
    );

    drawText(
        sx(8), sy(92),
        "US30",
        1,
        white
    );

    drawText(
        sx(8), sy(105),
        us30Price,
        1,
        white
    );

    drawText(
        sx(8), sy(134),
        "GOLD",
        1,
        white
    );

    drawText(
        sx(8), sy(147),
        goldPrice,
        1,
        white
    );

    drawText(
        sx(217), sy(56),
        "15M",
        1,
        white
    );

    drawText(
        sx(217), sy(98),
        "30M",
        1,
        white
    );

    drawText(
        sx(220), sy(140),
        "1H",
        1,
        white
    );

    // Selected-candle OHLC occupies the upper center column.
    drawText(
        sx(121), sy(42),
        "OHLC",
        1,
        secondary
    );

    char metric[40];

    formatLabeledMetric(
        metric,
        sizeof(metric),
        "O",
        model.selectedCandleValid,
        model.candleOpen
    );
    drawText(sx(91), sy(51), metric, 1, white);

    formatLabeledMetric(
        metric,
        sizeof(metric),
        "H",
        model.selectedCandleValid,
        model.candleHigh
    );
    drawText(sx(91), sy(63), metric, 1, white);

    formatLabeledMetric(
        metric,
        sizeof(metric),
        "L",
        model.selectedCandleValid,
        model.candleLow
    );
    drawText(sx(91), sy(92), metric, 1, white);

    formatLabeledMetric(
        metric,
        sizeof(metric),
        "C",
        model.selectedCandleValid,
        model.candleClose
    );
    drawText(sx(91), sy(104), metric, 1, white);

    // Compact macro dashboard in the lower center column.
    drawText(
        sx(119), sy(121),
        "MACRO",
        1,
        secondary
    );

    formatLabeledMetric(
        metric,
        sizeof(metric),
        "DXY",
        model.macroConnected,
        model.dxy
    );
    drawText(sx(90), sy(132), metric, 1, white);

    formatLabeledMetric(
        metric,
        sizeof(metric),
        "2Y",
        model.macroConnected,
        model.us2y
    );
    drawText(sx(143), sy(132), metric, 1, white);

    formatLabeledMetric(
        metric,
        sizeof(metric),
        "VIX",
        model.macroConnected,
        model.vix
    );
    drawText(sx(90), sy(142), metric, 1, white);

    formatLabeledMetric(
        metric,
        sizeof(metric),
        "10Y",
        model.macroConnected,
        model.us10y
    );
    drawText(sx(143), sy(142), metric, 1, white);

    formatLabeledMetric(
        metric,
        sizeof(metric),
        "WTI",
        model.macroConnected,
        model.wti
    );
    drawText(sx(90), sy(152), metric, 1, white);

    formatLabeledMetric(
        metric,
        sizeof(metric),
        "2S10S",
        model.macroConnected,
        model.curve2s10s
    );
    drawText(sx(143), sy(152), metric, 1, white);

    // Bottom action row remains exactly where T6C touch expects it.
    fillRect(
        sx(8), sy(169),
        sx(64), sy(18),
        alertColor
    );
    drawRect(
        sx(8), sy(169),
        sx(64), sy(18),
        white
    );
    drawText(
        sx(27), sy(175),
        "ALERT",
        1,
        white
    );

    fillRect(
        sx(96), sy(169),
        sx(64), sy(18),
        refreshColor
    );
    drawRect(
        sx(96), sy(169),
        sx(64), sy(18),
        white
    );
    drawText(
        sx(107), sy(175),
        "REFRESH",
        1,
        white
    );

    fillRect(
        sx(184), sy(169),
        sx(64), sy(18),
        startColor
    );
    drawRect(
        sx(184), sy(169),
        sx(64), sy(18),
        white
    );
    drawText(
        sx(200), sy(175),
        "START",
        1,
        white
    );

    present();
}

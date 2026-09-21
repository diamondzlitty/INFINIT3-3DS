#include <3ds.h>
#include "bottom_ui.hpp"
#include <3ds/services/news.h>

#include <arpa/inet.h>
#include <errno.h>
#include <malloc.h>
#include <math.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <sstream>
#include <string>
#include <vector>

#define SOC_ALIGN 0x1000
#define SOC_BUFFERSIZE 0x100000

static const char *CONFIG_PATH = "sdmc:/3ds/INFINIT3/config.ini";
static u32 *g_socBuffer = NULL;

struct ServerConfig {
    std::string host;
    int port;
};
struct MarketData {
    double nas100;
    double us30;
    double gold;
};

static bool parseMarketData(
    const std::string &body,
    MarketData &market,
    std::string &error
)
{
    bool haveNas100 = false;
    bool haveUs30 = false;
    bool haveGold = false;

    std::istringstream input(body);
    std::string line;

    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == 13) {
            line.pop_back();
        }

        size_t equals = line.find("=");

        if (equals == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, equals);
        std::string value = line.substr(equals + 1);

        char *end = NULL;
        double parsed = strtod(value.c_str(), &end);

        if (end == value.c_str()) {
            continue;
        }

        if (key == "NAS100") {
            market.nas100 = parsed;
            haveNas100 = true;
        } else if (key == "US30") {
            market.us30 = parsed;
            haveUs30 = true;
        } else if (key == "GOLD") {
            market.gold = parsed;
            haveGold = true;
        }
    }

    if (!haveNas100 || !haveUs30 || !haveGold) {
        error = "missing Big-3 field:";

        if (!haveNas100) {
            error += " NAS100";
        }

        if (!haveUs30) {
            error += " US30";
        }

        if (!haveGold) {
            error += " GOLD";
        }

        return false;
    }

    return true;
}



struct MacroData {
    double dxy;
    double us2y;
    double us10y;
    double wti;
    double vix;
    double curve2s10s;
};

static bool parseMacroData(
    const std::string &body,
    MacroData &macro,
    std::string &error
)
{
    bool haveDxy = false;
    bool haveUs2y = false;
    bool haveUs10y = false;
    bool haveWti = false;
    bool haveVix = false;
    bool haveCurve = false;

    std::istringstream input(body);
    std::string line;

    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == 13) {
            line.pop_back();
        }

        size_t equals = line.find("=");

        if (equals == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, equals);
        std::string value = line.substr(equals + 1);

        char *end = NULL;
        double parsed = strtod(value.c_str(), &end);

        if (end == value.c_str()) {
            continue;
        }

        if (key == "DXY") {
            macro.dxy = parsed;
            haveDxy = true;
        } else if (key == "US2Y") {
            macro.us2y = parsed;
            haveUs2y = true;
        } else if (key == "US10Y") {
            macro.us10y = parsed;
            haveUs10y = true;
        } else if (key == "WTI") {
            macro.wti = parsed;
            haveWti = true;
        } else if (key == "VIX") {
            macro.vix = parsed;
            haveVix = true;
        } else if (key == "2S10S") {
            macro.curve2s10s = parsed;
            haveCurve = true;
        }
    }

    if (!haveDxy || !haveUs2y || !haveUs10y || !haveWti || !haveVix || !haveCurve) {
        error = "missing macro field:";

        if (!haveDxy) error += " DXY";
        if (!haveUs2y) error += " US2Y";
        if (!haveUs10y) error += " US10Y";
        if (!haveWti) error += " WTI";
        if (!haveVix) error += " VIX";
        if (!haveCurve) error += " 2S10S";

        return false;
    }

    return true;
}

struct Candle {
    double open;
    double high;
    double low;
    double close;
};

enum Instrument {
    INSTRUMENT_NAS100 = 0,
    INSTRUMENT_US30,
    INSTRUMENT_GOLD,
    INSTRUMENT_COUNT
};

enum Timeframe {
    TIMEFRAME_15M = 0,
    TIMEFRAME_30M,
    TIMEFRAME_1H,
    TIMEFRAME_COUNT
};

struct ChartSelection {
    Instrument instrument;
    Timeframe timeframe;
};

struct ChartView {
    int cursor;
    int windowStart;
    int visibleCount;
};

enum ChartTransition {
    TRANSITION_NONE = 0,
    TRANSITION_ENTRANCE,
    TRANSITION_SELECTION,
    TRANSITION_REFRESH
};

static const char *instrumentName(Instrument instrument)
{
    static const char *names[] = { "NAS100", "US30", "GOLD" };
    return names[(int)instrument];
}

static const char *timeframeName(Timeframe timeframe)
{
    static const char *names[] = { "15m", "30m", "1h" };
    return names[(int)timeframe];
}

static const char *candlePath(const ChartSelection &selection)
{
    static const char *paths[INSTRUMENT_COUNT][TIMEFRAME_COUNT] = {
        { "/nas100_15m.txt", "/nas100_30m.txt", "/nas100_1h.txt" },
        { "/us30_15m.txt",   "/us30_30m.txt",   "/us30_1h.txt" },
        { "/gold_15m.txt",   "/gold_30m.txt",   "/gold_1h.txt" }
    };
    return paths[(int)selection.instrument][(int)selection.timeframe];
}

static void cycleInstrument(ChartSelection &selection, int direction)
{
    int next = (int)selection.instrument + direction;
    if (next < 0) next = INSTRUMENT_COUNT - 1;
    if (next >= INSTRUMENT_COUNT) next = 0;
    selection.instrument = (Instrument)next;
}

static void cycleTimeframe(ChartSelection &selection, int direction)
{
    int next = (int)selection.timeframe + direction;
    if (next < 0) next = TIMEFRAME_COUNT - 1;
    if (next >= TIMEFRAME_COUNT) next = 0;
    selection.timeframe = (Timeframe)next;
}

static bool parseCandles(
    const std::string &body,
    std::vector<Candle> &candles,
    std::string &error
)
{
    candles.clear();

    std::istringstream input(body);
    std::string line;

    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == 13) {
            line.pop_back();
        }

        if (line.empty()) {
            continue;
        }

        long o = 0;
        long h = 0;
        long l = 0;
        long c = 0;

        if (sscanf(
            line.c_str(),
            "%ld,%ld,%ld,%ld",
            &o,
            &h,
            &l,
            &c
        ) != 4) {
            error = "invalid OHLC row";
            return false;
        }

        if (h < l) {
            error = "candle high below low";
            return false;
        }

        Candle candle;
        candle.open  = (double)o / 100.0;
        candle.high  = (double)h / 100.0;
        candle.low   = (double)l / 100.0;
        candle.close = (double)c / 100.0;

        candles.push_back(candle);

        if (candles.size() > 240) {
            error = "too many candles";
            return false;
        }
    }

    if (candles.empty()) {
        error = "no candles";
        return false;
    }

    return true;
}

static inline void putTopPixel(
    int x,
    int y,
    u8 shade
)
{
    if (x < 0 || x >= 400 || y < 0 || y >= 240) {
        return;
    }

    u8 *fb = gfxGetFramebuffer(
        GFX_TOP,
        GFX_LEFT,
        NULL,
        NULL
    );

    size_t index =
        3 * (x * 240 + (239 - y));

    fb[index + 0] = shade;
    fb[index + 1] = shade;
    fb[index + 2] = shade;
}

static void clearTopScreen(u8 shade)
{
    for (int x = 0; x < 400; x++) {
        for (int y = 0; y < 240; y++) {
            putTopPixel(x, y, shade);
        }
    }
}

static void drawTopLine(
    int x0,
    int y0,
    int x1,
    int y1,
    u8 shade
)
{
    int dx = x1 - x0;
    if (dx < 0) dx = -dx;

    int sx = x0 < x1 ? 1 : -1;

    int dy = y1 - y0;
    if (dy < 0) dy = -dy;
    dy = -dy;

    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        putTopPixel(x0, y0, shade);

        if (x0 == x1 && y0 == y1) {
            break;
        }

        int e2 = 2 * err;

        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

static void fillTopRect(
    int x,
    int y,
    int width,
    int height,
    u8 shade
)
{
    for (int px = x; px < x + width; px++) {
        for (int py = y; py < y + height; py++) {
            putTopPixel(px, py, shade);
        }
    }
}

static int priceToChartY(
    double price,
    double minPrice,
    double maxPrice,
    int top,
    int bottom
)
{
    if (maxPrice <= minPrice) {
        return (top + bottom) / 2;
    }

    double normalized =
        (maxPrice - price) /
        (maxPrice - minPrice);

    int y =
        top +
        (int)(
            normalized *
            (double)(bottom - top)
        );

    if (y < top) y = top;
    if (y > bottom) y = bottom;

    return y;
}

static u8 scaledShade(u8 shade, float scale)
{
    int value = (int)((float)shade * scale);
    if (value < 0) value = 0;
    if (value > 255) value = 255;
    return (u8)value;
}

static Candle interpolateCandle(
    const Candle &from,
    const Candle &to,
    float amount
)
{
    Candle result;
    result.open = from.open + (to.open - from.open) * amount;
    result.high = from.high + (to.high - from.high) * amount;
    result.low = from.low + (to.low - from.low) * amount;
    result.close = from.close + (to.close - from.close) * amount;
    return result;
}

static void drawCandleSeries(
    const std::vector<Candle> &candles,
    int start,
    int count,
    int cursor,
    float reveal,
    int xOffset,
    float shadeScale,
    const Candle *latestFrom,
    float latestAmount
)
{
    if (candles.empty() || count <= 0 || shadeScale < 0.03f) {
        return;
    }

    const int left = 8;
    const int right = 391;
    const int top = 8;
    const int bottom = 231;

    if (start < 0) start = 0;
    if (start + count > (int)candles.size()) {
        count = (int)candles.size() - start;
    }

    double minPrice = candles[start].low;
    double maxPrice = candles[start].high;

    for (int i = start; i < start + count; i++) {
        if (candles[i].low < minPrice) minPrice = candles[i].low;
        if (candles[i].high > maxPrice) maxPrice = candles[i].high;
    }

    double range = maxPrice - minPrice;
    if (range < 0.01) range = 1.0;
    minPrice -= range * 0.05;
    maxPrice += range * 0.05;

    const int chartWidth = right - left - 8;
    const double step = (double)chartWidth / (double)count;
    int bodyWidth = step >= 7.0 ? 5 : (step >= 4.0 ? 3 : 2);
    int drawCount = (int)((float)count * reveal + 0.999f);
    if (drawCount < 1) drawCount = 1;
    if (drawCount > count) drawCount = count;

    for (int n = 0; n < drawCount; n++) {
        int candleIndex = start + n;
        Candle candle = candles[candleIndex];

        if (
            latestFrom &&
            candleIndex == (int)candles.size() - 1
        ) {
            candle = interpolateCandle(
                *latestFrom,
                candle,
                latestAmount
            );
        }

        int x =
            left + 4 +
            (int)(((double)n + 0.5) * step) +
            xOffset;

        int yHigh = priceToChartY(candle.high, minPrice, maxPrice, top + 3, bottom - 3);
        int yLow = priceToChartY(candle.low, minPrice, maxPrice, top + 3, bottom - 3);
        int yOpen = priceToChartY(candle.open, minPrice, maxPrice, top + 3, bottom - 3);
        int yClose = priceToChartY(candle.close, minPrice, maxPrice, top + 3, bottom - 3);

        bool selected = candleIndex == cursor;
        if (selected) {
            drawTopLine(
                x,
                top + 2,
                x,
                bottom - 2,
                scaledShade(72, shadeScale)
            );
        }

        drawTopLine(
            x,
            yHigh,
            x,
            yLow,
            scaledShade(selected ? 255 : 175, shadeScale)
        );

        int bodyTop = yOpen < yClose ? yOpen : yClose;
        int bodyBottom = yOpen > yClose ? yOpen : yClose;
        int bodyHeight = bodyBottom - bodyTop + 1;
        if (bodyHeight < 2) bodyHeight = 2;

        u8 bodyShade = candle.close >= candle.open ? 245 : 95;
        if (selected) bodyShade = 255;

        fillTopRect(
            x - bodyWidth / 2,
            bodyTop,
            bodyWidth,
            bodyHeight,
            scaledShade(bodyShade, shadeScale)
        );

        if (selected) {
            int half = bodyWidth / 2 + 2;
            u8 highlight = scaledShade(255, shadeScale);
            drawTopLine(x - half, bodyTop - 2, x + half, bodyTop - 2, highlight);
            drawTopLine(x - half, bodyBottom + 2, x + half, bodyBottom + 2, highlight);
        }

        if (candleIndex == (int)candles.size() - 1) {
            drawTopLine(
                right - 22,
                yClose,
                right,
                yClose,
                scaledShade(255, shadeScale)
            );
        }
    }
}

static void drawCandleChart(
    const std::vector<Candle> &candles,
    const std::vector<Candle> &previousCandles,
    const ChartView &view,
    ChartTransition transition,
    int animationFrame,
    int animationLength,
    int transitionDirection
)
{
    clearTopScreen(0);

    const int left = 8;
    const int right = 391;
    const int top = 8;
    const int bottom = 231;

    drawTopLine(left, top, right, top, 90);
    drawTopLine(left, bottom, right, bottom, 90);
    drawTopLine(left, top, left, bottom, 90);
    drawTopLine(right, top, right, bottom, 90);

    for (int i = 1; i < 5; i++) {
        int y = top + ((bottom - top) * i / 5);
        drawTopLine(left + 1, y, right - 1, y, 28);
    }

    if (candles.empty()) return;

    float amount = 1.0f;
    if (animationLength > 0 && animationFrame < animationLength) {
        amount = (float)animationFrame / (float)animationLength;
    }
    if (amount < 0.0f) amount = 0.0f;
    if (amount > 1.0f) amount = 1.0f;
    amount = amount * amount * (3.0f - 2.0f * amount);

    int count = view.visibleCount;
    int start = view.windowStart;

    if (transition == TRANSITION_SELECTION && !previousCandles.empty()) {
        int previousCount = count;
        if (previousCount > (int)previousCandles.size()) {
            previousCount = (int)previousCandles.size();
        }
        int previousStart = (int)previousCandles.size() - previousCount;
        int direction = transitionDirection == 0 ? 1 : transitionDirection;

        drawCandleSeries(
            previousCandles,
            previousStart,
            previousCount,
            -1,
            1.0f,
            (int)(-direction * amount * 28.0f),
            1.0f - amount,
            NULL,
            1.0f
        );

        drawCandleSeries(
            candles,
            start,
            count,
            view.cursor,
            1.0f,
            (int)(direction * (1.0f - amount) * 28.0f),
            amount,
            NULL,
            1.0f
        );
        return;
    }

    const Candle *latestFrom = NULL;
    if (transition == TRANSITION_REFRESH && !previousCandles.empty()) {
        latestFrom = &previousCandles.back();
    }

    drawCandleSeries(
        candles,
        start,
        count,
        view.cursor,
        transition == TRANSITION_ENTRANCE ? amount : 1.0f,
        0,
        1.0f,
        latestFrom,
        amount
    );
}

static bool loadServerConfig(ServerConfig &cfg, std::string &error)
{
    FILE *fp = fopen(CONFIG_PATH, "r");
    if (!fp) {
        error = "config.ini not found";
        return false;
    }

    char line[256];
    std::string serverValue;

    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }

        if (strncmp(line, "server=", 7) == 0) {
            serverValue = line + 7;
            break;
        }
    }

    fclose(fp);

    if (serverValue.empty()) {
        error = "missing server= entry";
        return false;
    }

    const std::string prefix = "http://";
    if (serverValue.compare(0, prefix.size(), prefix) != 0) {
        error = "server must begin http://";
        return false;
    }

    std::string authority = serverValue.substr(prefix.size());
    size_t slash = authority.find('/');
    if (slash != std::string::npos) {
        authority.erase(slash);
    }

    size_t colon = authority.rfind(':');
    if (colon == std::string::npos) {
        cfg.host = authority;
        cfg.port = 80;
    } else {
        cfg.host = authority.substr(0, colon);
        cfg.port = atoi(authority.substr(colon + 1).c_str());
    }

    if (cfg.host.empty() || cfg.port < 1 || cfg.port > 65535) {
        error = "invalid server host/port";
        return false;
    }

    return true;
}

static bool httpGetMarket(const ServerConfig &cfg, std::string &body, std::string &error)
{
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0) {
        error = "socket: " + std::string(strerror(errno));
        return false;
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons((u16)cfg.port);

    in_addr_t addr = inet_addr(cfg.host.c_str());
    if (addr == INADDR_NONE) {
        close(sock);
        error = "T1 config requires IPv4 address";
        return false;
    }

    server.sin_addr.s_addr = addr;

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        error = "connect: " + std::string(strerror(errno));
        close(sock);
        return false;
    }

    char request[512];
    snprintf(
        request,
        sizeof(request),
        "GET /market.txt HTTP/1.0\r\n"
        "Host: %s:%d\r\n"
        "Connection: close\r\n"
        "\r\n",
        cfg.host.c_str(),
        cfg.port
    );

    size_t requestLen = strlen(request);
    ssize_t sent = send(sock, request, requestLen, 0);
    if (sent < 0 || (size_t)sent != requestLen) {
        error = "send: " + std::string(strerror(errno));
        close(sock);
        return false;
    }

    std::string response;
    char buffer[1024];

    for (;;) {
        ssize_t received = recv(sock, buffer, sizeof(buffer), 0);

        if (received == 0) {
            break;
        }

        if (received < 0) {
            error = "recv: " + std::string(strerror(errno));
            close(sock);
            return false;
        }

        response.append(buffer, (size_t)received);

        if (response.size() > 16384) {
            error = "HTTP response too large";
            close(sock);
            return false;
        }
    }

    close(sock);

    size_t firstLineEnd = response.find("\r\n");
    if (firstLineEnd == std::string::npos) {
        error = "invalid HTTP response";
        return false;
    }

    std::string statusLine = response.substr(0, firstLineEnd);
    if (statusLine.find(" 200 ") == std::string::npos) {
        error = "HTTP status: " + statusLine;
        return false;
    }

    size_t bodyStart = response.find("\r\n\r\n");
    if (bodyStart == std::string::npos) {
        error = "HTTP headers incomplete";
        return false;
    }

    body = response.substr(bodyStart + 4);
    return true;
}

static bool httpGetCandles(
    const ServerConfig &cfg,
    const ChartSelection &selection,
    std::string &body,
    std::string &error
)
{
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0) {
        error = "socket: " + std::string(strerror(errno));
        return false;
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons((u16)cfg.port);

    in_addr_t addr = inet_addr(cfg.host.c_str());
    if (addr == INADDR_NONE) {
        close(sock);
        error = "T1 config requires IPv4 address";
        return false;
    }

    server.sin_addr.s_addr = addr;

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        error = "connect: " + std::string(strerror(errno));
        close(sock);
        return false;
    }

    const char *path = candlePath(selection);

    char request[512];
    snprintf(
        request,
        sizeof(request),
        "GET %s HTTP/1.0\r\n"
        "Host: %s:%d\r\n"
        "Connection: close\r\n"
        "\r\n",
        path,
        cfg.host.c_str(),
        cfg.port
    );

    size_t requestLen = strlen(request);
    ssize_t sent = send(sock, request, requestLen, 0);
    if (sent < 0 || (size_t)sent != requestLen) {
        error = "send: " + std::string(strerror(errno));
        close(sock);
        return false;
    }

    std::string response;
    char buffer[1024];

    for (;;) {
        ssize_t received = recv(sock, buffer, sizeof(buffer), 0);

        if (received == 0) {
            break;
        }

        if (received < 0) {
            error = "recv: " + std::string(strerror(errno));
            close(sock);
            return false;
        }

        response.append(buffer, (size_t)received);

        if (response.size() > 16384) {
            error = "HTTP response too large";
            close(sock);
            return false;
        }
    }

    close(sock);

    size_t firstLineEnd = response.find("\r\n");
    if (firstLineEnd == std::string::npos) {
        error = "invalid HTTP response";
        return false;
    }

    std::string statusLine = response.substr(0, firstLineEnd);
    if (statusLine.find(" 200 ") == std::string::npos) {
        error = "HTTP status: " + statusLine;
        return false;
    }

    size_t bodyStart = response.find("\r\n\r\n");
    if (bodyStart == std::string::npos) {
        error = "HTTP headers incomplete";
        return false;
    }

    body = response.substr(bodyStart + 4);
    return true;
}

static bool httpGetMacro(const ServerConfig &cfg, std::string &body, std::string &error)
{
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0) {
        error = "socket: " + std::string(strerror(errno));
        return false;
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons((u16)cfg.port);

    in_addr_t addr = inet_addr(cfg.host.c_str());
    if (addr == INADDR_NONE) {
        close(sock);
        error = "T1 config requires IPv4 address";
        return false;
    }

    server.sin_addr.s_addr = addr;

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        error = "connect: " + std::string(strerror(errno));
        close(sock);
        return false;
    }

    char request[512];
    snprintf(
        request,
        sizeof(request),
        "GET /macro.txt HTTP/1.0\r\n"
        "Host: %s:%d\r\n"
        "Connection: close\r\n"
        "\r\n",
        cfg.host.c_str(),
        cfg.port
    );

    size_t requestLen = strlen(request);
    ssize_t sent = send(sock, request, requestLen, 0);
    if (sent < 0 || (size_t)sent != requestLen) {
        error = "send: " + std::string(strerror(errno));
        close(sock);
        return false;
    }

    std::string response;
    char buffer[1024];

    for (;;) {
        ssize_t received = recv(sock, buffer, sizeof(buffer), 0);

        if (received == 0) {
            break;
        }

        if (received < 0) {
            error = "recv: " + std::string(strerror(errno));
            close(sock);
            return false;
        }

        response.append(buffer, (size_t)received);

        if (response.size() > 16384) {
            error = "HTTP response too large";
            close(sock);
            return false;
        }
    }

    close(sock);

    size_t firstLineEnd = response.find("\r\n");
    if (firstLineEnd == std::string::npos) {
        error = "invalid HTTP response";
        return false;
    }

    std::string statusLine = response.substr(0, firstLineEnd);
    if (statusLine.find(" 200 ") == std::string::npos) {
        error = "HTTP status: " + statusLine;
        return false;
    }

    size_t bodyStart = response.find("\r\n\r\n");
    if (bodyStart == std::string::npos) {
        error = "HTTP headers incomplete";
        return false;
    }

    body = response.substr(bodyStart + 4);
    return true;
}

static Result addNotificationTest(void)
{
    static const u16 title[] = {
        'I', 'N', 'F', 'I', 'N', 'I', 'T', '3', ' ', 'T', 'E', 'R', 'M', 'I', 'N', 'A', 'L'
    };
    static const u16 message[] = {
        'N', 'E', 'W', 'S', ' ', 's', 'e', 'r', 'v', 'i', 'c', 'e', ' ', 'r', 'e', 'a', 'd', 'y', '.'
    };

    return NEWS_AddNotification(
        title,
        sizeof(title) / sizeof(title[0]),
        message,
        sizeof(message) / sizeof(message[0]),
        NULL,
        0,
        false
    );
}

struct AppState {
    ChartSelection selection;
    ChartView view;
    std::vector<Candle> candles;
    std::vector<Candle> previousCandles;
    MarketData market;
    MacroData macro;
    bool candleReady;
    bool marketReady;
    bool macroReady;
    bool newsReady;
    ChartTransition transition;
    int transitionFrame;
    int transitionLength;
    int transitionDirection;
    int uiPulseFrames;
    int marketPulseFrames;
    int inputRepeatCooldown;
    std::string status;
    std::string detail;
};

static void clampChartView(AppState &state)
{
    int candleCount = (int)state.candles.size();
    if (candleCount <= 0) {
        state.view.cursor = -1;
        state.view.windowStart = 0;
        state.view.visibleCount = 0;
        return;
    }

    int minVisible = candleCount < 12 ? candleCount : 12;
    int maxVisible = candleCount < 96 ? candleCount : 96;

    if (state.view.visibleCount < minVisible) {
        state.view.visibleCount = minVisible;
    }
    if (state.view.visibleCount > maxVisible) {
        state.view.visibleCount = maxVisible;
    }
    if (state.view.cursor < 0) state.view.cursor = 0;
    if (state.view.cursor >= candleCount) state.view.cursor = candleCount - 1;

    int maxStart = candleCount - state.view.visibleCount;
    if (state.view.windowStart < 0) state.view.windowStart = 0;
    if (state.view.windowStart > maxStart) state.view.windowStart = maxStart;

    if (state.view.cursor < state.view.windowStart) {
        state.view.windowStart = state.view.cursor;
    }
    if (state.view.cursor >= state.view.windowStart + state.view.visibleCount) {
        state.view.windowStart =
            state.view.cursor - state.view.visibleCount + 1;
    }
}

static void moveCursor(AppState &state, int amount)
{
    if (state.candles.empty()) return;

    int previous = state.view.cursor;
    state.view.cursor += amount;
    clampChartView(state);

    if (state.view.cursor == previous) {
        state.detail = amount < 0 ? "HISTORY START" : "LATEST CANDLE";
    } else {
        char text[40];
        snprintf(
            text,
            sizeof(text),
            "CURSOR %d / %lu",
            state.view.cursor + 1,
            (unsigned long)state.candles.size()
        );
        state.detail = text;
    }
    state.uiPulseFrames = 6;
}

static void zoomChart(AppState &state, int amount)
{
    if (state.candles.empty()) return;

    int oldVisible = state.view.visibleCount;
    state.view.visibleCount += amount;
    clampChartView(state);

    if (state.view.visibleCount != oldVisible) {
        state.view.windowStart =
            state.view.cursor - state.view.visibleCount / 2;
        clampChartView(state);
    }

    char text[40];
    snprintf(
        text,
        sizeof(text),
        "ZOOM %d CANDLES",
        state.view.visibleCount
    );
    state.detail = text;
    state.uiPulseFrames = 6;
}


// T5_FINAL_STABLE_BOTTOM

static void printStableBottomRow(
    int row,
    const char *text
)
{
    printf(
        "\x1b[%d;1H%-40.40s",
        row,
        text ? text : ""
    );
}

static const char *stableInstrumentName(
    Instrument instrument
)
{
    if (instrument == INSTRUMENT_NAS100) {
        return "NAS100";
    }

    if (instrument == INSTRUMENT_US30) {
        return "US30";
    }

    return "GOLD";
}

static const char *stableTimeframeName(
    Timeframe timeframe
)
{
    if (timeframe == TIMEFRAME_15M) {
        return "15M";
    }

    if (timeframe == TIMEFRAME_30M) {
        return "30M";
    }

    return "1H";
}

static void renderBottomScreen(
    const AppState &state
)
{
    // T6A graphics-only foundation test.
    // No libctru console text is allowed to touch
    // the bottom framebuffer during normal runtime.
    (void)state;
}

static void renderTopFrame(
    const AppState &state
)
{
    drawCandleChart(
        state.candles,
        state.previousCandles,
        state.view,
        state.transition,
        state.transitionFrame,
        state.transitionLength,
        state.transitionDirection
    );

    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();
}

static void renderFrame(
    const AppState &state
)
{
    drawCandleChart(
        state.candles,
        state.previousCandles,
        state.view,
        state.transition,
        state.transitionFrame,
        state.transitionLength,
        state.transitionDirection
    );

    renderBottomScreen(state);

    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();
}

static void advanceAnimations(AppState &state)
{
    if (state.transitionFrame < state.transitionLength) {
        state.transitionFrame++;
    } else if (state.transition != TRANSITION_NONE) {
        state.transition = TRANSITION_NONE;
        state.previousCandles.clear();
    }

    if (state.uiPulseFrames > 0) state.uiPulseFrames--;
    if (state.marketPulseFrames > 0) state.marketPulseFrames--;
    if (state.inputRepeatCooldown > 0) state.inputRepeatCooldown--;
}

static void showLoading(
    AppState &state
)
{
    state.status = "SYNC";
    state.detail =
        "FETCHING MARKET + MACRO + OHLC";
    state.uiPulseFrames = 0;
}

static bool valueChanged(double before, double after)
{
    return fabs(before - after) > 0.0001;
}

static void refreshData(
    const ServerConfig &cfg,
    AppState &state,
    ChartTransition transition,
    int transitionDirection
)
{
    showLoading(state);

    bool wasAtLatest =
        !state.candles.empty() &&
        state.view.cursor == (int)state.candles.size() - 1;

    std::string candleBody;
    std::string marketBody;
    std::string macroBody;
    std::string candleError;
    std::string marketError;
    std::string macroError;
    std::vector<Candle> newCandles;
    MarketData newMarket = {};
    MacroData newMacro = {};

    bool candleOk =
        httpGetCandles(cfg, state.selection, candleBody, candleError) &&
        parseCandles(candleBody, newCandles, candleError);

    bool marketOk =
        httpGetMarket(cfg, marketBody, marketError) &&
        parseMarketData(marketBody, newMarket, marketError);

    bool macroOk =
        httpGetMacro(cfg, macroBody, macroError) &&
        parseMacroData(macroBody, newMacro, macroError);

    if (candleOk) {
        state.previousCandles = state.candles;
        state.candles = newCandles;
        state.candleReady = true;

        if (state.view.visibleCount <= 0) state.view.visibleCount = 60;

        if (transition == TRANSITION_SELECTION || state.previousCandles.empty()) {
            state.view.cursor = (int)state.candles.size() - 1;
            state.view.windowStart =
                (int)state.candles.size() - state.view.visibleCount;
            transition = state.previousCandles.empty()
                ? TRANSITION_ENTRANCE
                : TRANSITION_SELECTION;
        } else if (wasAtLatest) {
            state.view.cursor = (int)state.candles.size() - 1;
            state.view.windowStart =
                (int)state.candles.size() - state.view.visibleCount;
        }

        clampChartView(state);
        state.transition = transition;
        state.transitionFrame = 0;
        state.transitionLength = transition == TRANSITION_ENTRANCE ? 18 : 14;
        state.transitionDirection = transitionDirection;
    } else if (transition == TRANSITION_SELECTION) {
        state.previousCandles = state.candles;
        state.candles.clear();
        state.candleReady = false;
        clampChartView(state);
        state.transition = TRANSITION_NONE;
    }

    bool changed = false;
    if (marketOk) {
        changed =
            state.marketReady &&
            (valueChanged(state.market.nas100, newMarket.nas100) ||
             valueChanged(state.market.us30, newMarket.us30) ||
             valueChanged(state.market.gold, newMarket.gold));
        state.market = newMarket;
        state.marketReady = true;
    }

    if (macroOk) {
        changed = changed ||
            (state.macroReady &&
             (valueChanged(state.macro.dxy, newMacro.dxy) ||
              valueChanged(state.macro.us2y, newMacro.us2y) ||
              valueChanged(state.macro.us10y, newMacro.us10y) ||
              valueChanged(state.macro.wti, newMacro.wti) ||
              valueChanged(state.macro.vix, newMacro.vix) ||
              valueChanged(state.macro.curve2s10s, newMacro.curve2s10s)));
        state.macro = newMacro;
        state.macroReady = true;
    }

    if (changed) state.marketPulseFrames = 24;

    if (candleOk && marketOk && macroOk) {
        state.status = "ONLINE // SYNC OK";
        char detail[40];
        snprintf(
            detail,
            sizeof(detail),
            "%s %s // %lu CANDLES",
            instrumentName(state.selection.instrument),
            timeframeName(state.selection.timeframe),
            (unsigned long)state.candles.size()
        );
        state.detail = detail;
    } else {
        state.status = "DEGRADED // HELD";
        if (!candleOk) state.detail = "CANDLE: " + candleError;
        else if (!marketOk) state.detail = "MARKET: " + marketError;
        else state.detail = "MACRO: " + macroError;
    }
    state.uiPulseFrames = 18;
}

int main(int argc, char *argv[])
{
    gfxInitDefault();
    gfxSetDoubleBuffering(GFX_TOP, true);
    gfxSetDoubleBuffering(GFX_BOTTOM, false);
    consoleInit(GFX_BOTTOM, NULL);
    t6aBottomInit();

    printf("INFINIT3 TERMINAL\n");
    printf("NEW 3DS NATIVE T5 RC\n\n");
    printf("Loading:\n%s\n\n", CONFIG_PATH);

    ServerConfig cfg;
    std::string configError;

    if (!loadServerConfig(cfg, configError)) {
        printf("CONFIG: FAIL\n%s\n", configError.c_str());
        printf("\nSTART = exit\n");

        while (aptMainLoop()) {
            hidScanInput();
            if (hidKeysDown() & KEY_START) {
                break;
            }
            gfxFlushBuffers();
            gfxSwapBuffers();
            gspWaitForVBlank();
        }

        gfxExit();
        return 0;
    }

    printf("CONFIG: PASS\n");
    printf("Server: %s:%d\n\n", cfg.host.c_str(), cfg.port);
    printf("Initializing SOC...\n");

    g_socBuffer = (u32 *)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
    if (!g_socBuffer) {
        printf("SOC BUFFER: FAIL\n");
        printf("\nSTART = exit\n");

        while (aptMainLoop()) {
            hidScanInput();
            if (hidKeysDown() & KEY_START) {
                break;
            }
            gfxFlushBuffers();
            gfxSwapBuffers();
            gspWaitForVBlank();
        }

        gfxExit();
        return 0;
    }

    int rc = socInit(g_socBuffer, SOC_BUFFERSIZE);
    if (rc != 0) {
        printf("socInit: FAIL 0x%08X\n", (unsigned int)rc);
        free(g_socBuffer);
        g_socBuffer = NULL;
        printf("\nSTART = exit\n");

        while (aptMainLoop()) {
            hidScanInput();
            if (hidKeysDown() & KEY_START) {
                break;
            }
            gfxFlushBuffers();
            gfxSwapBuffers();
            gspWaitForVBlank();
        }

        gfxExit();
        return 0;
    }

    Result newsResult = newsInit();
    bool newsReady = R_SUCCEEDED(newsResult);

    AppState state = {};
    state.selection.instrument = INSTRUMENT_NAS100;
    state.selection.timeframe = TIMEFRAME_15M;
    state.view.cursor = -1;
    state.view.windowStart = 0;
    state.view.visibleCount = 60;
    state.newsReady = newsReady;
    state.transition = TRANSITION_NONE;
    state.status = "BOOTING";
    state.detail = "INITIALIZING T5 TERMINAL";

    refreshData(cfg, state, TRANSITION_ENTRANCE, 1);

    printf("\x1b[2J\x1b[H");

    // T6A: establish the persistent RGB565 bottom UI once.
    t6aBottomDrawFoundation();
    renderFrame(state);

    while (aptMainLoop()) {
        hidScanInput();
        u32 down = hidKeysDown();
        bool needsFrame = down != 0;
        bool bottomDirty = down != 0;

        if (down & KEY_L) {
            cycleInstrument(state.selection, -1);
            refreshData(cfg, state, TRANSITION_SELECTION, -1);
        } else if (down & KEY_R) {
            cycleInstrument(state.selection, 1);
            refreshData(cfg, state, TRANSITION_SELECTION, 1);
        } else if (down & KEY_ZL) {
            cycleTimeframe(state.selection, -1);
            refreshData(cfg, state, TRANSITION_SELECTION, -1);
        } else if (down & KEY_ZR) {
            cycleTimeframe(state.selection, 1);
            refreshData(cfg, state, TRANSITION_SELECTION, 1);
        } else if (down & KEY_X) {
            refreshData(cfg, state, TRANSITION_REFRESH, 0);
        }

        if (down & KEY_DLEFT) moveCursor(state, -1);
        if (down & KEY_DRIGHT) moveCursor(state, 1);
        if (down & KEY_DUP) zoomChart(state, -4);
        if (down & KEY_DDOWN) zoomChart(state, 4);

        circlePosition circle;
        circlePosition cstick;
        hidCircleRead(&circle);
        hidCstickRead(&cstick);

        if (state.inputRepeatCooldown <= 0) {
            bool repeated = false;
            if (circle.dx < -70) {
                moveCursor(state, -4);
                repeated = true;
            } else if (circle.dx > 70) {
                moveCursor(state, 4);
                repeated = true;
            } else if (cstick.dx < -45) {
                moveCursor(state, -8);
                repeated = true;
            } else if (cstick.dx > 45) {
                moveCursor(state, 8);
                repeated = true;
            } else if (cstick.dy > 45) {
                zoomChart(state, -4);
                repeated = true;
            } else if (cstick.dy < -45) {
                zoomChart(state, 4);
                repeated = true;
            }

            if (repeated) {
                state.inputRepeatCooldown = 5;
                needsFrame = true;
                bottomDirty = true;
            }
        }

        if (down & KEY_Y) {
            Result notificationResult = newsReady
                ? addNotificationTest()
                : (Result)-1;
            char detail[40];
            snprintf(
                detail,
                sizeof(detail),
                "NEWS RESULT 0x%08lX",
                (unsigned long)notificationResult
            );
            state.status = R_SUCCEEDED(notificationResult)
                ? "NOTIFICATION SENT"
                : "NOTIFICATION FAIL";
            state.detail = detail;
            state.uiPulseFrames = 20;
        }

        if (bottomDirty) {
            renderFrame(state);
        } else if (
            needsFrame ||
            state.transition != TRANSITION_NONE
        ) {
            renderTopFrame(state);
        } else {
            gspWaitForVBlank();
        }
        advanceAnimations(state);

        if (down & KEY_START) {
            break;
        }
    }

    if (newsReady) {
        newsExit();
    }

    socExit();

    if (g_socBuffer) {
        free(g_socBuffer);
        g_socBuffer = NULL;
    }

    gfxExit();
    return 0;
}

#include <3ds.h>

#include <arpa/inet.h>
#include <errno.h>
#include <malloc.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <sstream>
#include <string>

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

static void printNetworkResult(const ServerConfig &cfg)
{
    std::string marketBody;
    std::string macroBody;
    std::string error;

    MarketData market = {};
    MacroData macro = {};

    printf("\x1b[2J\x1b[H");
    printf("INFINIT3 TERMINAL\n");
    printf("NEW 3DS NATIVE T3\n\n");

    printf("SERVER %s:%d\n", cfg.host.c_str(), cfg.port);

    if (!httpGetMarket(cfg, marketBody, error)) {
        printf("\nMARKET HTTP: FAIL\n");
        printf("%s\n", error.c_str());
        printf("\nX = retry    START = exit\n");
        return;
    }

    printf("MARKET HTTP: 200\n");

    if (!parseMarketData(marketBody, market, error)) {
        printf("MARKET PARSE: FAIL\n");
        printf("%s\n", error.c_str());
        printf("\nX = retry    START = exit\n");
        return;
    }

    printf("MARKET PARSE: PASS\n");

    if (!httpGetMacro(cfg, macroBody, error)) {
        printf("MACRO HTTP: FAIL\n");
        printf("%s\n", error.c_str());
        printf("\nX = retry    START = exit\n");
        return;
    }

    printf("MACRO HTTP: 200\n");

    if (!parseMacroData(macroBody, macro, error)) {
        printf("MACRO PARSE: FAIL\n");
        printf("%s\n", error.c_str());
        printf("\nX = retry    START = exit\n");
        return;
    }

    printf("MACRO PARSE: PASS\n\n");

    printf("BIG-3\n");
    printf("NAS100 %10.2f\n", market.nas100);
    printf("US30   %10.2f\n", market.us30);
    printf("GOLD   %10.2f\n", market.gold);

    printf("\nMACRO\n");
    printf("DXY    %10.4f\n", macro.dxy);
    printf("US2Y   %9.4f%%\n", macro.us2y);
    printf("US10Y  %9.4f%%\n", macro.us10y);
    printf("WTI    %10.2f\n", macro.wti);
    printf("VIX    %10.2f\n", macro.vix);
    printf("2S10S  %+9.4f\n", macro.curve2s10s);

    printf("\nT3 structured state online.\n");
    printf("X = refresh    START = exit\n");
}

int main(int argc, char *argv[])
{
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

    printf("INFINIT3 TERMINAL\n");
    printf("NEW 3DS NATIVE T1\n\n");
    printf("Loading:\n%s\n\n", CONFIG_PATH);

    ServerConfig cfg;
    std::string configError;

    if (!loadServerConfig(cfg, configError)) {
        printf("CONFIG: FAIL\n%s\n", configError.c_str());
        printf("\nSTART = exit\n");

        while (aptMainLoop()) {
            gspWaitForVBlank();
            gfxSwapBuffers();
            hidScanInput();

            if (hidKeysDown() & KEY_START) {
                break;
            }
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
            gspWaitForVBlank();
            gfxSwapBuffers();
            hidScanInput();

            if (hidKeysDown() & KEY_START) {
                break;
            }
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
            gspWaitForVBlank();
            gfxSwapBuffers();
            hidScanInput();

            if (hidKeysDown() & KEY_START) {
                break;
            }
        }

        gfxExit();
        return 0;
    }

    printNetworkResult(cfg);

    while (aptMainLoop()) {
        gspWaitForVBlank();
        gfxSwapBuffers();
        hidScanInput();

        u32 down = hidKeysDown();

        if (down & KEY_X) {
            printNetworkResult(cfg);
        }

        if (down & KEY_START) {
            break;
        }
    }

    socExit();

    if (g_socBuffer) {
        free(g_socBuffer);
        g_socBuffer = NULL;
    }

    gfxExit();
    return 0;
}

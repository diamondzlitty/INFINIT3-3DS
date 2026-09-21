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

#include <string>

#define SOC_ALIGN 0x1000
#define SOC_BUFFERSIZE 0x100000

static const char *CONFIG_PATH = "sdmc:/3ds/INFINIT3/config.ini";
static u32 *g_socBuffer = NULL;

struct ServerConfig {
    std::string host;
    int port;
};

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

static void printNetworkResult(const ServerConfig &cfg)
{
    std::string body;
    std::string error;

    printf("\x1b[2J\x1b[H");
    printf("INFINIT3 TERMINAL\n");
    printf("NEW 3DS NATIVE T1\n\n");
    printf("SERVER\n");
    printf("%s:%d\n\n", cfg.host.c_str(), cfg.port);
    printf("GET /market.txt\n");

    if (httpGetMarket(cfg, body, error)) {
        printf("\nNETWORK: PASS\n");
        printf("HTTP: 200\n\n");
        printf("LIVE BACKEND DATA\n");
        printf("------------------------------\n");
        printf("%s", body.c_str());

        if (body.empty() || body[body.size() - 1] != '\n') {
            printf("\n");
        }
    } else {
        printf("\nNETWORK: FAIL\n");
        printf("%s\n", error.c_str());
    }

    printf("\nX = retry    START = exit\n");
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

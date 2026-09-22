#include "session_levels.hpp"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <map>
#include <sstream>
#include <string>

typedef std::map<std::string, std::string> LevelFields;

static bool httpGetSessionLevels(
    const std::string &host,
    int port,
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
    server.sin_port = htons((uint16_t)port);

    in_addr_t addr = inet_addr(host.c_str());
    if (addr == INADDR_NONE) {
        close(sock);
        error = "levels config requires IPv4";
        return false;
    }

    server.sin_addr.s_addr = addr;

    if (
        connect(
            sock,
            (struct sockaddr *)&server,
            sizeof(server)
        ) < 0
    ) {
        error = "connect: " + std::string(strerror(errno));
        close(sock);
        return false;
    }

    char request[512];
    snprintf(
        request,
        sizeof(request),
        "GET /session_levels.txt HTTP/1.0\r\n"
        "Host: %s:%d\r\n"
        "Connection: close\r\n"
        "\r\n",
        host.c_str(),
        port
    );

    size_t requestLen = strlen(request);
    ssize_t sent = send(sock, request, requestLen, 0);

    if (
        sent < 0 ||
        (size_t)sent != requestLen
    ) {
        error = "send: " + std::string(strerror(errno));
        close(sock);
        return false;
    }

    std::string response;
    char buffer[1024];

    for (;;) {
        ssize_t received =
            recv(sock, buffer, sizeof(buffer), 0);

        if (received == 0) break;

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

    std::string statusLine =
        response.substr(0, firstLineEnd);

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

static bool parseFields(
    const std::string &body,
    LevelFields &fields,
    std::string &error
)
{
    fields.clear();

    std::istringstream input(body);
    std::string line;

    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == 13) {
            line.pop_back();
        }

        if (line.empty()) continue;

        size_t equals = line.find("=");
        if (equals == std::string::npos) {
            error = "invalid levels row";
            return false;
        }

        std::string key = line.substr(0, equals);
        std::string value = line.substr(equals + 1);

        if (key.empty()) {
            error = "empty levels key";
            return false;
        }

        fields[key] = value;
    }

    return true;
}

static bool getString(
    const LevelFields &fields,
    const std::string &key,
    std::string &value,
    std::string &error
)
{
    LevelFields::const_iterator it = fields.find(key);

    if (
        it == fields.end() ||
        it->second.empty()
    ) {
        error = "missing levels field: " + key;
        return false;
    }

    value = it->second;
    return true;
}

static bool getInt(
    const LevelFields &fields,
    const std::string &key,
    int &value,
    std::string &error
)
{
    std::string text;

    if (!getString(fields, key, text, error)) {
        return false;
    }

    char *end = NULL;
    long parsed = strtol(text.c_str(), &end, 10);

    if (
        end == text.c_str() ||
        *end != '\0'
    ) {
        error = "invalid levels integer: " + key;
        return false;
    }

    value = (int)parsed;
    return true;
}

static bool getDouble(
    const LevelFields &fields,
    const std::string &key,
    double &value,
    std::string &error
)
{
    std::string text;

    if (!getString(fields, key, text, error)) {
        return false;
    }

    if (text == "NA") {
        error = "required levels value is NA: " + key;
        return false;
    }

    char *end = NULL;
    double parsed = strtod(text.c_str(), &end);

    if (
        end == text.c_str() ||
        *end != '\0'
    ) {
        error = "invalid levels number: " + key;
        return false;
    }

    value = parsed;
    return true;
}

static bool getOptionalDouble(
    const LevelFields &fields,
    const std::string &key,
    bool &valid,
    double &value,
    std::string &error
)
{
    std::string text;

    if (!getString(fields, key, text, error)) {
        return false;
    }

    if (text == "NA") {
        valid = false;
        value = 0.0;
        return true;
    }

    char *end = NULL;
    double parsed = strtod(text.c_str(), &end);

    if (
        end == text.c_str() ||
        *end != '\0'
    ) {
        error = "invalid optional number: " + key;
        return false;
    }

    valid = true;
    value = parsed;
    return true;
}

static bool parseTriplet(
    const LevelFields &fields,
    const std::string &prefix,
    SessionTriplet &triplet,
    std::string &error
)
{
    bool openValid = false;
    bool highValid = false;
    bool lowValid = false;

    if (
        !getOptionalDouble(
            fields,
            prefix + "_OPEN",
            openValid,
            triplet.open,
            error
        ) ||
        !getOptionalDouble(
            fields,
            prefix + "_HIGH",
            highValid,
            triplet.high,
            error
        ) ||
        !getOptionalDouble(
            fields,
            prefix + "_LOW",
            lowValid,
            triplet.low,
            error
        )
    ) {
        return false;
    }

    if (
        openValid != highValid ||
        openValid != lowValid
    ) {
        error = "partial session triplet: " + prefix;
        return false;
    }

    triplet.valid = openValid;

    if (
        triplet.valid &&
        triplet.high < triplet.low
    ) {
        error = "session high below low: " + prefix;
        return false;
    }

    return true;
}

static bool parseInstrument(
    const LevelFields &fields,
    const std::string &prefix,
    SessionInstrumentLevels &levels,
    std::string &error
)
{
    int ready = 0;

    if (
        !getInt(
            fields,
            prefix + "_READY",
            ready,
            error
        )
    ) {
        return false;
    }

    if (ready != 1) {
        error = prefix + " levels not ready";
        return false;
    }

    levels.ready = true;

    if (
        !getString(
            fields,
            prefix + "_DATA_DATE",
            levels.dataDate,
            error
        ) ||
        !getString(
            fields,
            prefix + "_LAST_BAR_ET",
            levels.lastBarEt,
            error
        ) ||
        !getDouble(
            fields,
            prefix + "_PDH",
            levels.pdh,
            error
        ) ||
        !getDouble(
            fields,
            prefix + "_PDL",
            levels.pdl,
            error
        ) ||
        !getDouble(
            fields,
            prefix + "_PDC",
            levels.pdc,
            error
        ) ||
        !getDouble(
            fields,
            prefix + "_DAY_OPEN",
            levels.dayOpen,
            error
        ) ||
        !getDouble(
            fields,
            prefix + "_DAY_HIGH",
            levels.dayHigh,
            error
        ) ||
        !getDouble(
            fields,
            prefix + "_DAY_LOW",
            levels.dayLow,
            error
        )
    ) {
        return false;
    }

    if (levels.pdh < levels.pdl) {
        error = prefix + " PDH below PDL";
        return false;
    }

    if (levels.dayHigh < levels.dayLow) {
        error = prefix + " day high below low";
        return false;
    }

    if (
        !parseTriplet(
            fields,
            prefix + "_ASIA",
            levels.asia,
            error
        ) ||
        !parseTriplet(
            fields,
            prefix + "_LONDON",
            levels.london,
            error
        ) ||
        !parseTriplet(
            fields,
            prefix + "_NY",
            levels.newYork,
            error
        )
    ) {
        return false;
    }

    bool highValid = false;
    bool lowValid = false;

    if (
        !getOptionalDouble(
            fields,
            prefix + "_SESSION_HIGH",
            highValid,
            levels.sessionHigh,
            error
        ) ||
        !getOptionalDouble(
            fields,
            prefix + "_SESSION_LOW",
            lowValid,
            levels.sessionLow,
            error
        )
    ) {
        return false;
    }

    if (highValid != lowValid) {
        error = "partial session range: " + prefix;
        return false;
    }

    levels.sessionRangeValid = highValid;

    if (
        levels.sessionRangeValid &&
        levels.sessionHigh < levels.sessionLow
    ) {
        error = prefix + " session high below low";
        return false;
    }

    return true;
}

static bool parseSessionLevels(
    const std::string &body,
    SessionLevelsData &levels,
    std::string &error
)
{
    LevelFields fields;

    if (!parseFields(body, fields, error)) {
        return false;
    }

    if (
        !getInt(
            fields,
            "VERSION",
            levels.version,
            error
        ) ||
        !getString(
            fields,
            "STATUS",
            levels.status,
            error
        ) ||
        !getString(
            fields,
            "GENERATED_UTC",
            levels.generatedUtc,
            error
        ) ||
        !getString(
            fields,
            "NOW_ET",
            levels.nowEt,
            error
        ) ||
        !getString(
            fields,
            "SESSION",
            levels.session,
            error
        ) ||
        !getInt(
            fields,
            "SESSION_PROGRESS",
            levels.sessionProgress,
            error
        )
    ) {
        return false;
    }

    if (levels.version != 1) {
        error = "unsupported levels version";
        return false;
    }

    if (levels.status != "OK") {
        error = "levels status: " + levels.status;
        return false;
    }

    if (
        levels.sessionProgress < 0 ||
        levels.sessionProgress > 100
    ) {
        error = "invalid session progress";
        return false;
    }

    if (
        levels.session != "ASIA" &&
        levels.session != "LONDON" &&
        levels.session != "NEW_YORK" &&
        levels.session != "MAINTENANCE" &&
        levels.session != "CLOSED"
    ) {
        error = "unknown session: " + levels.session;
        return false;
    }

    if (
        !parseInstrument(
            fields,
            "NAS100",
            levels.nas100,
            error
        ) ||
        !parseInstrument(
            fields,
            "US30",
            levels.us30,
            error
        ) ||
        !parseInstrument(
            fields,
            "GOLD",
            levels.gold,
            error
        )
    ) {
        return false;
    }

    return true;
}

bool fetchSessionLevels(
    const std::string &host,
    int port,
    SessionLevelsData &levels,
    std::string &error
)
{
    std::string body;

    if (
        !httpGetSessionLevels(
            host,
            port,
            body,
            error
        )
    ) {
        return false;
    }

    return parseSessionLevels(
        body,
        levels,
        error
    );
}

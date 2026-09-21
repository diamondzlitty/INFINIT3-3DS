#!/bin/zsh

BASE="$HOME/DSiMarketServer"
LOGS="$BASE/logs"

mkdir -p "$LOGS"
cd "$BASE" || exit 1

echo ""
echo "========================================"
echo "        INFINIT3 BACKEND SYSTEM"
echo "========================================"

# ==================================================
# STOP OLD COPIES
# ==================================================

for SERVICE in http market news caffeinate; do

    PIDFILE="$BASE/.$SERVICE.pid"

    if [ -f "$PIDFILE" ]; then

        PID=$(cat "$PIDFILE")

        if kill -0 "$PID" 2>/dev/null; then
            kill "$PID" 2>/dev/null
            sleep 1
        fi

        rm -f "$PIDFILE"
    fi
done

# ==================================================
# HTTP SERVER
# ==================================================

nohup python3 -u -m http.server 8080 --bind 0.0.0.0 \
    > "$LOGS/http.log" 2>&1 &

echo $! > "$BASE/.http.pid"

# ==================================================
# MARKET FEED
# ==================================================

nohup python3 -u market_feed.py \
    > "$LOGS/market.log" 2>&1 &

echo $! > "$BASE/.market.pid"

# ==================================================
# NEWS FEED
# ==================================================

nohup python3 -u news_feed.py \
    > "$LOGS/news.log" 2>&1 &

echo $! > "$BASE/.news.pid"

# ==================================================
# KEEP MAC AWAKE
# ==================================================

nohup caffeinate -dimsu \
    > "$LOGS/caffeinate.log" 2>&1 &

echo $! > "$BASE/.caffeinate.pid"

sleep 3

echo ""

# ==================================================
# STATUS CHECK
# ==================================================

check_service() {

    NAME="$1"
    PIDFILE="$2"

    printf "%-18s" "$NAME"

    if [ -f "$PIDFILE" ] && \
       kill -0 "$(cat "$PIDFILE")" 2>/dev/null; then

        echo "RUNNING ✓"

    else

        echo "FAILED ✗"
    fi
}

check_service \
    "HTTP SERVER:" \
    "$BASE/.http.pid"

check_service \
    "MARKET FEED:" \
    "$BASE/.market.pid"

check_service \
    "NEWS FEED:" \
    "$BASE/.news.pid"

check_service \
    "SLEEP PREVENTION:" \
    "$BASE/.caffeinate.pid"

# ==================================================
# NETWORK ADDRESS
# ==================================================

echo ""

IP=$(ipconfig getifaddr en0 2>/dev/null)

if [ -z "$IP" ]; then
    IP=$(ipconfig getifaddr en1 2>/dev/null)
fi

echo "MAC IP:      ${IP:-UNKNOWN}"
echo "DSi EXPECTS: 192.168.1.154"

if [ "$IP" = "192.168.1.154" ]; then

    echo "NETWORK ADDRESS MATCH ✓"

else

    echo "WARNING: MAC IP HAS CHANGED"
    echo "DSi code currently expects 192.168.1.154"
fi

# ==================================================
# LATEST MARKET STATUS
# ==================================================

echo ""
echo "MARKET FEED:"
echo "----------------------------------------"

tail -n 5 "$LOGS/market.log" 2>/dev/null

# ==================================================
# LATEST NEWS STATUS
# ==================================================

echo ""
echo "NEWS FEED:"
echo "----------------------------------------"

tail -n 8 "$LOGS/news.log" 2>/dev/null

echo ""
echo "========================================"
echo "       INFINIT3 BACKEND ONLINE"
echo "========================================"
echo ""
echo "Trading Terminal backend: READY"
echo "News Terminal backend:    READY"
echo ""
echo "You may close this Terminal window."
echo ""
# ===== INFINIT3 MACRO FEED - 3DS T3 =====
echo
echo "Starting 3DS macro feed..."
if pgrep -f "python3 -u macro_feed.py" >/dev/null 2>&1; then
    echo "MACRO FEED:      ALREADY RUNNING"
else
    nohup python3 -u macro_feed.py > logs/macro.log 2>&1 &
    echo $! > logs/macro.pid
    echo "MACRO FEED:      STARTED"
fi
# ===== END INFINIT3 MACRO FEED =====

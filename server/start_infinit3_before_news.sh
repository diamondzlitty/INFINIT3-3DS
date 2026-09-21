#!/bin/zsh

BASE="$HOME/DSiMarketServer"
LOGS="$BASE/logs"

mkdir -p "$LOGS"
cd "$BASE" || exit 1

echo ""
echo "======================================"
echo "      INFINIT3 TERMINAL BACKEND"
echo "======================================"

# Stop an old copy if this command was already run.
for SERVICE in http feed caffeinate; do
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

# --------------------------------------
# HTTP SERVER
# --------------------------------------

nohup python3 -m http.server 8080 --bind 0.0.0.0 \
    > "$LOGS/http.log" 2>&1 &

echo $! > "$BASE/.http.pid"

# --------------------------------------
# MARKET FEED
# --------------------------------------

nohup python3 market_feed.py \
    > "$LOGS/feed.log" 2>&1 &

echo $! > "$BASE/.feed.pid"

# --------------------------------------
# KEEP MAC AWAKE
# --------------------------------------

nohup caffeinate -dimsu \
    > "$LOGS/caffeinate.log" 2>&1 &

echo $! > "$BASE/.caffeinate.pid"

sleep 2

echo ""
echo "HTTP SERVER:"
if kill -0 "$(cat "$BASE/.http.pid")" 2>/dev/null; then
    echo "  RUNNING ✓"
else
    echo "  FAILED ✗"
fi

echo ""
echo "MARKET FEED:"
if kill -0 "$(cat "$BASE/.feed.pid")" 2>/dev/null; then
    echo "  RUNNING ✓"
else
    echo "  FAILED ✗"
fi

echo ""
echo "SLEEP PREVENTION:"
if kill -0 "$(cat "$BASE/.caffeinate.pid")" 2>/dev/null; then
    echo "  RUNNING ✓"
else
    echo "  FAILED ✗"
fi

echo ""

IP=$(ipconfig getifaddr en0 2>/dev/null)

if [ -z "$IP" ]; then
    IP=$(ipconfig getifaddr en1 2>/dev/null)
fi

echo "MAC IP: ${IP:-UNKNOWN}"
echo "DSi EXPECTS: 192.168.1.154"

if [ "$IP" = "192.168.1.154" ]; then
    echo "NETWORK ADDRESS MATCH ✓"
else
    echo "WARNING: MAC IP CHANGED"
    echo "The DSi currently expects 192.168.1.154"
fi

echo ""
echo "Latest feed output:"
echo "--------------------------------------"
tail -n 5 "$LOGS/feed.log" 2>/dev/null

echo ""
echo "======================================"
echo " INFINIT3 BACKEND STARTED"
echo "======================================"
echo ""
echo "You can close this Terminal window."
echo "The backend will keep running."
echo ""

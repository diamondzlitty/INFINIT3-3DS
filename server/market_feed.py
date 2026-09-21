import os
import time
import threading
import warnings

import yfinance as yf


warnings.filterwarnings(
    "ignore",
    message=".*LibreSSL.*"
)


# ==================================================
# INFINIT3 TERMINAL V4 MARKET ENGINE
#
# LIVE PRICES:
# Yahoo WebSocket
#
# CANDLES:
# yfinance history requests
# ==================================================

SYMBOLS = {
    "NAS100": "NQ=F",
    "US30": "YM=F",
    "GOLD": "GC=F",
}

REVERSE_SYMBOLS = {
    "NQ=F": "NAS100",
    "YM=F": "US30",
    "GC=F": "GOLD",
}

TIMEFRAMES = {
    "15m": "15m",
    "30m": "30m",
    "1h": "1h",
}


# ==================================================
# SHARED LIVE PRICE STATE
# ==================================================

prices = {
    "NAS100": None,
    "US30": None,
    "GOLD": None,
}

last_stream_update = {
    "NAS100": 0,
    "US30": 0,
    "GOLD": 0,
}

price_lock = threading.Lock()


# ==================================================
# ATOMIC FILE WRITE
#
# Prevents the DSi from downloading market.txt
# halfway through a rewrite.
# ==================================================

def write_market_file():

    with price_lock:

        if any(
            prices[name] is None
            for name in prices
        ):
            return

        text = (
            f"NAS100={prices['NAS100']:.2f}\n"
            f"US30={prices['US30']:.2f}\n"
            f"GOLD={prices['GOLD']:.2f}\n"
        )

    temp_file = "market.tmp"

    with open(
        temp_file,
        "w"
    ) as file:

        file.write(text)

    os.replace(
        temp_file,
        "market.txt"
    )


# ==================================================
# INITIAL PRICES
#
# Important when:
# - market is closed
# - websocket has not sent first tick yet
# ==================================================

def load_initial_prices():

    print()
    print(
        "Loading initial market prices..."
    )

    for name, symbol in SYMBOLS.items():

        try:

            ticker = yf.Ticker(
                symbol
            )

            price = float(
                ticker.fast_info[
                    "last_price"
                ]
            )

            with price_lock:

                prices[name] = price

            print(
                f"{name}: {price:.2f}"
            )

        except Exception as error:

            print(
                f"{name} initial-price error:",
                error
            )

    write_market_file()


# ==================================================
# FALLBACK QUOTE
#
# If a symbol hasn't streamed for a while,
# occasionally ask Yahoo normally.
#
# This also keeps closed-market prices available.
# ==================================================

def refresh_stale_quotes():

    now = time.time()

    for name, symbol in SYMBOLS.items():

        stale_for = (
            now -
            last_stream_update[name]
        )

        # Don't hammer Yahoo.
        # Only fallback if we haven't seen
        # a stream update for 20 seconds.
        if stale_for < 20:
            continue

        try:

            ticker = yf.Ticker(
                symbol
            )

            price = float(
                ticker.fast_info[
                    "last_price"
                ]
            )

            changed = False

            with price_lock:

                old_price = prices[name]

                if (
                    old_price is None or
                    price != old_price
                ):
                    prices[name] = price
                    changed = True

            if changed:

                write_market_file()

                print(
                    f"FALLBACK {name} "
                    f"{price:.2f}"
                )

        except Exception as error:

            print(
                f"{name} fallback error:",
                error
            )


# ==================================================
# WEBSOCKET MESSAGE HANDLER
# ==================================================

def handle_stream_message(
    message
):

    if not isinstance(
        message,
        dict
    ):
        return

    # yfinance currently supplies decoded
    # Yahoo messages containing id + price.
    symbol = (
        message.get("id")
        or
        message.get("symbol")
    )

    price = (
        message.get("price")
        or
        message.get(
            "regularMarketPrice"
        )
    )

    if (
        symbol not in
        REVERSE_SYMBOLS
    ):
        return

    if price is None:
        return

    try:

        price = float(
            price
        )

    except Exception:
        return

    name = REVERSE_SYMBOLS[
        symbol
    ]

    changed = False

    with price_lock:

        old_price = prices[name]

        prices[name] = price

        last_stream_update[name] = (
            time.time()
        )

        if (
            old_price is None or
            price != old_price
        ):
            changed = True

    if changed:

        write_market_file()

        print(
            f"LIVE {name} "
            f"{price:.2f}"
        )


# ==================================================
# WEBSOCKET THREAD
# ==================================================

def websocket_worker():

    symbols = list(
        SYMBOLS.values()
    )

    while True:

        try:

            print()
            print(
                "Connecting Yahoo live stream..."
            )

            ws = yf.WebSocket(
                verbose=False
            )

            ws.subscribe(
                symbols
            )

            print(
                "Yahoo WebSocket connected."
            )

            print(
                "Streaming:",
                ", ".join(
                    symbols
                )
            )

            ws.listen(
                handle_stream_message
            )

        except Exception as error:

            print(
                "WebSocket error:",
                error
            )

            print(
                "Retrying stream in 5 seconds..."
            )

            time.sleep(
                5
            )


# ==================================================
# CANDLE FILE GENERATOR
# ==================================================

def save_candles(
    name,
    symbol,
    timeframe_name,
    interval
):

    ticker = yf.Ticker(
        symbol
    )

    data = ticker.history(
        period="1mo",
        interval=interval,
        auto_adjust=False,
        prepost=True
    )

    data = data.dropna(
        subset=[
            "Open",
            "High",
            "Low",
            "Close"
        ]
    )

    data = data.tail(
        60
    )

    filename = (
        f"{name.lower()}_"
        f"{timeframe_name}.txt"
    )

    temp_filename = (
        filename +
        ".tmp"
    )

    with open(
        temp_filename,
        "w"
    ) as file:

        for _, candle in data.iterrows():

            o = round(
                float(
                    candle["Open"]
                ) * 100
            )

            h = round(
                float(
                    candle["High"]
                ) * 100
            )

            l = round(
                float(
                    candle["Low"]
                ) * 100
            )

            c = round(
                float(
                    candle["Close"]
                ) * 100
            )

            file.write(
                f"{o},{h},{l},{c}\n"
            )

    os.replace(
        temp_filename,
        filename
    )

    print(
        f"Updated {filename}: "
        f"{len(data)} candles"
    )


# ==================================================
# REFRESH ALL CANDLE FILES
# ==================================================

def refresh_candles():

    print()
    print(
        "Refreshing OHLC candles..."
    )

    for market_name, symbol in (
        ("nas100", "NQ=F"),
        ("us30", "YM=F"),
        ("gold", "GC=F"),
    ):

        for (
            timeframe_name,
            interval
        ) in TIMEFRAMES.items():

            try:

                save_candles(
                    market_name,
                    symbol,
                    timeframe_name,
                    interval
                )

            except Exception as error:

                print(
                    f"{market_name}_"
                    f"{timeframe_name} "
                    f"error:",
                    error
                )

    print(
        "Candle refresh complete."
    )


# ==================================================
# START
# ==================================================

print()
print(
    "======================================="
)
print(
    "   INFINIT3 V4 LIVE MARKET ENGINE"
)
print(
    "======================================="
)

print(
    "yfinance:",
    yf.__version__
)

load_initial_prices()


# Start WebSocket in the background.
stream_thread = threading.Thread(
    target=websocket_worker,
    daemon=True
)

stream_thread.start()


# Build candle files immediately.
refresh_candles()


last_candle_refresh = (
    time.time()
)

last_fallback_check = 0


# ==================================================
# MAIN ENGINE LOOP
# ==================================================

while True:

    now = time.time()

    # Check stale streaming quotes
    # every 10 seconds.
    if (
        now -
        last_fallback_check >= 10
    ):

        last_fallback_check = now

        refresh_stale_quotes()

    # Heavy OHLC refresh remains
    # once per minute.
    if (
        now -
        last_candle_refresh >= 60
    ):

        last_candle_refresh = now

        refresh_candles()

    time.sleep(
        1
    )

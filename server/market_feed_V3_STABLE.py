import time
import yfinance as yf

SYMBOLS = {
    "nas100": "^NDX",
    "us30": "^DJI",
    "gold": "GC=F",
}

TIMEFRAMES = {
    "15m": "15m",
    "30m": "30m",
    "1h": "1h",
}

print("INFINIT3 live market + candle feed starting...")


def get_price(symbol):
    ticker = yf.Ticker(symbol)
    return float(ticker.fast_info["last_price"])


def save_candles(name, symbol, timeframe_name, interval):
    ticker = yf.Ticker(symbol)

    data = ticker.history(
        period="1mo",
        interval=interval,
        auto_adjust=False,
        prepost=True
    )

    # Throw away incomplete/invalid rows.
    data = data.dropna(
        subset=["Open", "High", "Low", "Close"]
    )

    # We only need the newest 12 candles on the DSi.
    data = data.tail(60)

    filename = f"{name}_{timeframe_name}.txt"

    with open(filename, "w") as file:
        for _, candle in data.iterrows():

            # Multiply by 100 so the DSi can work with
            # whole integers instead of floating point.
            o = round(float(candle["Open"]) * 100)
            h = round(float(candle["High"]) * 100)
            l = round(float(candle["Low"]) * 100)
            c = round(float(candle["Close"]) * 100)

            file.write(
                f"{o},{h},{l},{c}\n"
            )

    print(
        f"Updated {filename}: "
        f"{len(data)} candles"
    )


while True:

    try:
        # --------------------------------------------------
        # CURRENT PRICES
        # --------------------------------------------------

        nas100 = get_price(SYMBOLS["nas100"])
        us30 = get_price(SYMBOLS["us30"])
        gold = get_price(SYMBOLS["gold"])

        with open("market.txt", "w") as file:
            file.write(f"NAS100={nas100:.2f}\n")
            file.write(f"US30={us30:.2f}\n")
            file.write(f"GOLD={gold:.2f}\n")

        print()
        print(
            f"NAS100 {nas100:.2f} | "
            f"US30 {us30:.2f} | "
            f"GOLD {gold:.2f}"
        )

        # --------------------------------------------------
        # REAL OHLC CANDLES
        # --------------------------------------------------

        for market_name, yahoo_symbol in SYMBOLS.items():

            for timeframe_name, interval in TIMEFRAMES.items():

                save_candles(
                    market_name,
                    yahoo_symbol,
                    timeframe_name,
                    interval
                )

        print("Feed cycle complete.")
        print("------------------------------")

    except Exception as error:
        print("Feed error:", error)

    # Refresh once per minute.
    time.sleep(60)

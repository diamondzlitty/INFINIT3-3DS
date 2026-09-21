#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import json
import os
import subprocess
import sys
import time
from datetime import datetime, timezone

import yfinance as yf

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
OUTPUT_PATH = os.path.join(BASE_DIR, "macro.txt")

CNBC_YIELDS_URL = (
    "https://quote.cnbc.com/quote-html-webservice/restQuote/"
    "symbolType/symbol?symbols=US2Y%7CUS10Y"
    "&requestMethod=itv&noform=1&partnerId=2"
    "&fund=1&exthrs=1&output=json&events=1"
)

YAHOO_SYMBOLS = {
    "DXY": "DX-Y.NYB",
    "WTI": "CL=F",
    "VIX": "^VIX",
}

REFRESH_SECONDS = 60


def get_yahoo_value(symbol):
    ticker = yf.Ticker(symbol)

    data = ticker.history(
        period="1d",
        interval="5m",
        auto_adjust=False,
        prepost=True,
    )

    if data.empty:
        data = ticker.history(
            period="5d",
            interval="1d",
            auto_adjust=False,
        )

    if data.empty:
        raise RuntimeError("no Yahoo data for " + symbol)

    close = float(data.iloc[-1]["Close"])
    stamp = str(data.index[-1])
    return close, stamp


def parse_percent_value(raw):
    if raw is None:
        raise ValueError("missing yield value")

    text = str(raw).strip().replace(",", "").replace("%", "")

    if not text:
        raise ValueError("empty yield value")

    return float(text)


def get_cnbc_yields():
    result = subprocess.run(
        [
            "curl",
            "--location",
            "--http1.1",
            "--connect-timeout",
            "5",
            "--max-time",
            "15",
            "--user-agent",
            "Mozilla/5.0 INFINIT3-3DS",
            "--silent",
            "--show-error",
            CNBC_YIELDS_URL,
        ],
        capture_output=True,
        text=True,
        check=False,
    )

    if result.returncode != 0:
        message = result.stderr.strip() or str(result.returncode)
        raise RuntimeError("CNBC curl failed: " + message)

    payload = json.loads(result.stdout)

    formatted = payload.get("FormattedQuoteResult", {})
    quotes = (
        formatted.get("FormattedQuote")
        or formatted.get("formattedQuote")
        or []
    )

    if isinstance(quotes, dict):
        quotes = [quotes]

    found = {}

    for quote in quotes:
        symbol = quote.get("symbol")

        if symbol not in ("US2Y", "US10Y"):
            continue

        found[symbol] = {
            "value": parse_percent_value(quote.get("last")),
            "time": quote.get("last_time")
            or quote.get("lastTime")
            or quote.get("last_timedate")
            or "",
        }

    missing = [
        name for name in ("US2Y", "US10Y")
        if name not in found
    ]

    if missing:
        raise RuntimeError(
            "CNBC missing yield quote(s): " + ", ".join(missing)
        )

    return found


def build_snapshot():
    values = {}
    stamps = {}

    for name, symbol in YAHOO_SYMBOLS.items():
        value, stamp = get_yahoo_value(symbol)
        values[name] = value
        stamps[name] = stamp

    yields = get_cnbc_yields()

    values["US2Y"] = yields["US2Y"]["value"]
    values["US10Y"] = yields["US10Y"]["value"]
    values["2S10S"] = values["US10Y"] - values["US2Y"]

    stamps["US2Y"] = yields["US2Y"]["time"]
    stamps["US10Y"] = yields["US10Y"]["time"]

    return values, stamps


def write_snapshot(values, stamps):
    generated = datetime.now(timezone.utc).isoformat()

    lines = [
        "DXY={:.4f}".format(values["DXY"]),
        "US2Y={:.4f}".format(values["US2Y"]),
        "US10Y={:.4f}".format(values["US10Y"]),
        "WTI={:.4f}".format(values["WTI"]),
        "VIX={:.4f}".format(values["VIX"]),
        "2S10S={:.4f}".format(values["2S10S"]),
        "US2Y_SOURCE=CNBC_US2Y",
        "US10Y_SOURCE=CNBC_US10Y",
        "CURVE_FREQUENCY=LIVE_QUOTES",
        "GENERATED_UTC=" + generated,
        "DXY_TIME=" + stamps["DXY"],
        "US2Y_TIME=" + stamps["US2Y"],
        "US10Y_TIME=" + stamps["US10Y"],
        "WTI_TIME=" + stamps["WTI"],
        "VIX_TIME=" + stamps["VIX"],
    ]

    temp_path = OUTPUT_PATH + ".tmp"

    with open(temp_path, "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines) + "\n")

    os.replace(temp_path, OUTPUT_PATH)


def refresh_once():
    values, stamps = build_snapshot()
    write_snapshot(values, stamps)

    print("macro.txt updated")
    print("DXY:   {:.4f}".format(values["DXY"]))
    print("US2Y:  {:.4f}".format(values["US2Y"]))
    print("US10Y: {:.4f}".format(values["US10Y"]))
    print("WTI:   {:.4f}".format(values["WTI"]))
    print("VIX:   {:.4f}".format(values["VIX"]))
    print("2s10s: {:.4f}".format(values["2S10S"]))


def main():
    once = "--once" in sys.argv

    while True:
        try:
            refresh_once()
        except Exception as exc:
            print(
                "macro feed error: {}: {}".format(
                    type(exc).__name__,
                    exc,
                ),
                flush=True,
            )

            if once:
                raise

        if once:
            return

        time.sleep(REFRESH_SECONDS)


if __name__ == "__main__":
    main()

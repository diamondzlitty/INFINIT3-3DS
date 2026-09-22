#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import time
from datetime import datetime, timedelta, timezone
from zoneinfo import ZoneInfo

import yfinance as yf

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
OUTPUT_PATH = os.path.join(BASE_DIR, "session_levels.txt")
REFRESH_SECONDS = 60

ET = ZoneInfo("America/New_York")

SYMBOLS = {
    "NAS100": "NQ=F",
    "US30": "YM=F",
    "GOLD": "GC=F",
}

SESSION_ASIA = "ASIA"
SESSION_LONDON = "LONDON"
SESSION_NEW_YORK = "NEW_YORK"
SESSION_MAINTENANCE = "MAINTENANCE"
SESSION_CLOSED = "CLOSED"


def minute_of_day(stamp):
    return stamp.hour * 60 + stamp.minute


def session_for_clock(stamp):
    local = stamp.astimezone(ET)
    weekday = local.weekday()
    minute = minute_of_day(local)

    # Weekend close for the Big-3 futures proxies.
    if weekday == 5:
        return SESSION_CLOSED, 0
    if weekday == 6 and minute < 18 * 60:
        return SESSION_CLOSED, 0
    if weekday == 4 and minute >= 17 * 60:
        return SESSION_CLOSED, 0

    if 17 * 60 <= minute < 18 * 60:
        progress = int(((minute - 17 * 60) / 60.0) * 100.0)
        return SESSION_MAINTENANCE, max(0, min(100, progress))

    if minute >= 18 * 60 or minute < 2 * 60:
        elapsed = minute - 18 * 60 if minute >= 18 * 60 else minute + 6 * 60
        progress = int((elapsed / float(8 * 60)) * 100.0)
        return SESSION_ASIA, max(0, min(100, progress))

    if minute < 8 * 60 + 30:
        elapsed = minute - 2 * 60
        duration = 6 * 60 + 30
        progress = int((elapsed / float(duration)) * 100.0)
        return SESSION_LONDON, max(0, min(100, progress))

    elapsed = minute - (8 * 60 + 30)
    duration = 8 * 60 + 30
    progress = int((elapsed / float(duration)) * 100.0)
    return SESSION_NEW_YORK, max(0, min(100, progress))


def trading_date_for_stamp(stamp):
    local = stamp.astimezone(ET)
    trading_date = local.date()
    if minute_of_day(local) >= 18 * 60:
        trading_date += timedelta(days=1)
    return trading_date


def session_for_bar(stamp):
    minute = minute_of_day(stamp)

    if minute >= 18 * 60 or minute < 2 * 60:
        return SESSION_ASIA
    if minute < 8 * 60 + 30:
        return SESSION_LONDON
    if minute < 17 * 60:
        return SESSION_NEW_YORK
    return SESSION_MAINTENANCE


def load_history(symbol):
    ticker = yf.Ticker(symbol)
    data = ticker.history(
        period="5d",
        interval="5m",
        auto_adjust=False,
        prepost=True,
    )

    if data.empty:
        raise RuntimeError("no 5m history")

    needed = ["Open", "High", "Low", "Close"]
    missing = [name for name in needed if name not in data.columns]
    if missing:
        raise RuntimeError("missing OHLC columns: " + ",".join(missing))

    data = data.dropna(subset=needed).copy()
    if data.empty:
        raise RuntimeError("5m history empty after OHLC cleanup")

    index = data.index
    if index.tz is None:
        index = index.tz_localize("UTC")

    data.index = index.tz_convert(ET)
    data = data.sort_index()

    data["_TRADING_DATE"] = [
        trading_date_for_stamp(stamp)
        for stamp in data.index
    ]
    data["_SESSION"] = [
        session_for_bar(stamp)
        for stamp in data.index
    ]

    return data


def frame_value(frame, column, method):
    if frame is None or frame.empty:
        return None

    series = frame[column]

    if method == "first":
        return float(series.iloc[0])
    if method == "last":
        return float(series.iloc[-1])
    if method == "max":
        return float(series.max())
    if method == "min":
        return float(series.min())

    raise ValueError("unknown frame method")


def session_stats(frame, name):
    rows = frame[frame["_SESSION"] == name]

    if rows.empty:
        return {"OPEN": None, "HIGH": None, "LOW": None}

    return {
        "OPEN": frame_value(rows, "Open", "first"),
        "HIGH": frame_value(rows, "High", "max"),
        "LOW": frame_value(rows, "Low", "min"),
    }


def build_instrument(symbol):
    data = load_history(symbol)

    trading_dates = sorted(set(data["_TRADING_DATE"].tolist()))
    if not trading_dates:
        raise RuntimeError("no trading dates")

    current_date = trading_dates[-1]
    current = data[data["_TRADING_DATE"] == current_date]

    if current.empty:
        raise RuntimeError("current trading day empty")

    previous = None
    if len(trading_dates) >= 2:
        previous_date = trading_dates[-2]
        previous = data[data["_TRADING_DATE"] == previous_date]

    asia = session_stats(current, SESSION_ASIA)
    london = session_stats(current, SESSION_LONDON)
    new_york = session_stats(current, SESSION_NEW_YORK)

    return {
        "DATA_DATE": current_date.isoformat(),
        "LAST_BAR_ET": data.index[-1].isoformat(),
        "PDH": frame_value(previous, "High", "max"),
        "PDL": frame_value(previous, "Low", "min"),
        "PDC": frame_value(previous, "Close", "last"),
        "DAY_OPEN": frame_value(current, "Open", "first"),
        "DAY_HIGH": frame_value(current, "High", "max"),
        "DAY_LOW": frame_value(current, "Low", "min"),
        "ASIA_OPEN": asia["OPEN"],
        "ASIA_HIGH": asia["HIGH"],
        "ASIA_LOW": asia["LOW"],
        "LONDON_OPEN": london["OPEN"],
        "LONDON_HIGH": london["HIGH"],
        "LONDON_LOW": london["LOW"],
        "NY_OPEN": new_york["OPEN"],
        "NY_HIGH": new_york["HIGH"],
        "NY_LOW": new_york["LOW"],
    }


def format_price(value):
    if value is None:
        return "NA"
    return "{:.2f}".format(value)


def safe_error(error):
    return "{}: {}".format(type(error).__name__, error).replace("\n", " ").replace("\r", " ").replace("=", ":")


def current_session_range(values, session_name):
    if session_name == SESSION_ASIA:
        return values.get("ASIA_HIGH"), values.get("ASIA_LOW")
    if session_name == SESSION_LONDON:
        return values.get("LONDON_HIGH"), values.get("LONDON_LOW")
    if session_name == SESSION_NEW_YORK:
        return values.get("NY_HIGH"), values.get("NY_LOW")
    return None, None


def build_snapshot():
    now_utc = datetime.now(timezone.utc)
    now_et = now_utc.astimezone(ET)
    session_name, session_progress = session_for_clock(now_et)

    instruments = {}
    errors = {}

    for name, symbol in SYMBOLS.items():
        try:
            instruments[name] = build_instrument(symbol)
        except Exception as error:
            instruments[name] = None
            errors[name] = safe_error(error)

    return {
        "generated_utc": now_utc.isoformat(),
        "now_et": now_et.isoformat(),
        "session": session_name,
        "session_progress": session_progress,
        "status": "OK" if not errors else "DEGRADED",
        "instruments": instruments,
        "errors": errors,
    }


def write_snapshot(snapshot):
    lines = [
        "VERSION=1",
        "STATUS=" + snapshot["status"],
        "GENERATED_UTC=" + snapshot["generated_utc"],
        "NOW_ET=" + snapshot["now_et"],
        "SESSION=" + snapshot["session"],
        "SESSION_PROGRESS={}".format(snapshot["session_progress"]),
    ]

    fields = [
        "PDH",
        "PDL",
        "PDC",
        "DAY_OPEN",
        "DAY_HIGH",
        "DAY_LOW",
        "ASIA_OPEN",
        "ASIA_HIGH",
        "ASIA_LOW",
        "LONDON_OPEN",
        "LONDON_HIGH",
        "LONDON_LOW",
        "NY_OPEN",
        "NY_HIGH",
        "NY_LOW",
    ]

    for name in ("NAS100", "US30", "GOLD"):
        values = snapshot["instruments"][name]

        if values is None:
            lines.append(name + "_READY=0")
            lines.append(name + "_ERROR=" + snapshot["errors"].get(name, "unknown"))
            lines.append(name + "_DATA_DATE=NA")
            lines.append(name + "_LAST_BAR_ET=NA")
            for field in fields:
                lines.append("{}_{}=NA".format(name, field))
            lines.append(name + "_SESSION_HIGH=NA")
            lines.append(name + "_SESSION_LOW=NA")
            continue

        lines.append(name + "_READY=1")
        lines.append(name + "_DATA_DATE=" + values["DATA_DATE"])
        lines.append(name + "_LAST_BAR_ET=" + values["LAST_BAR_ET"])

        for field in fields:
            lines.append(
                "{}_{}={}".format(
                    name,
                    field,
                    format_price(values[field]),
                )
            )

        session_high, session_low = current_session_range(
            values,
            snapshot["session"],
        )

        lines.append(
            name + "_SESSION_HIGH=" + format_price(session_high)
        )
        lines.append(
            name + "_SESSION_LOW=" + format_price(session_low)
        )

    temp_path = OUTPUT_PATH + ".tmp"

    with open(temp_path, "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines) + "\n")

    os.replace(temp_path, OUTPUT_PATH)


def refresh_once():
    snapshot = build_snapshot()
    write_snapshot(snapshot)

    print("session_levels.txt updated")
    print(
        "SESSION: {} {}%".format(
            snapshot["session"],
            snapshot["session_progress"],
        )
    )
    print("STATUS:", snapshot["status"])

    for name in ("NAS100", "US30", "GOLD"):
        values = snapshot["instruments"][name]
        if values is None:
            print(
                "{}: ERROR {}".format(
                    name,
                    snapshot["errors"].get(name, "unknown"),
                )
            )
            continue

        print(
            "{}: date={} PDH={} PDL={} DAY_H={} DAY_L={}".format(
                name,
                values["DATA_DATE"],
                format_price(values["PDH"]),
                format_price(values["PDL"]),
                format_price(values["DAY_HIGH"]),
                format_price(values["DAY_LOW"]),
            )
        )


def main():
    once = "--once" in sys.argv

    while True:
        try:
            refresh_once()
        except Exception as error:
            print(
                "session/levels feed error: {}: {}".format(
                    type(error).__name__,
                    error,
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

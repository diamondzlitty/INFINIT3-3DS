# Shared Backend

INFINIT3 TERMINAL and INFINIT3 NEWS currently rely on a lightweight external backend for heavyweight internet retrieval and preprocessing.

The existing private development backend is preserved separately.

Before public release, the reusable backend components will be audited, cleaned, documented, and migrated here without committing runtime logs, caches, local addresses, generated market files, or machine-specific state.

### T3 Macro Feed

`macro_feed.py` generates `macro.txt` for the native 3DS Terminal.

Current structured fields:

- DXY
- US2Y
- US10Y
- WTI
- VIX
- 2S10S

Yield quotes use CNBC US2Y and US10Y so both curve points share the same live source. DXY, WTI, and VIX use yfinance. The launcher starts the macro feed alongside the existing market and news services.

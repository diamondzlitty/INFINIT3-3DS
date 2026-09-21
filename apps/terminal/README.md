# template

This is a template for starting new 3DS libctru projects.


**Current milestone:** T3 HARDWARE PASS

### T1 — Native Backend Networking

Hardware verified on New Nintendo 3DS:

- Runtime server configuration loaded from `sdmc:/3ds/INFINIT3/config.ini`
- No permanent LAN/DHCP address is compiled into the application
- Native libctru socket networking initializes successfully
- HTTP GET `/market.txt` succeeds against the existing INFINIT3 backend
- Live NAS100, US30, and GOLD values render on hardware
- `X` performs a successful network retry/refetch
- `START` exits cleanly to Homebrew Launcher
- No crash, freeze, reboot, or exception observed

T1 tested `.3dsx` SHA-256:

`014c30c045f70b3e6b93c94c643a6e3e85675bafaf2091a75b178200a8be2054`



### T2 — Big-3 Structured Market State

Hardware verified on New Nintendo 3DS:

- Existing T1 runtime configuration/networking remains functional
- `/market.txt` is parsed into structured native market state
- NAS100, US30, and GOLD are stored as numeric values
- Structured Big-3 values render successfully on hardware
- `X` refreshes and reparses current backend values
- `START` exits cleanly
- No crash, freeze, reboot, or exception observed

T2 tested `.3dsx` SHA-256:

`ef48c2633e96b423c3ba78228cc633fc943971b1be581e16514b286768c97dc5`


### T3 — Expanded Macro Market State

Hardware verified on New Nintendo 3DS:

- Existing T2 Big-3 structured state remains functional
- Added a second HTTP feed at `/macro.txt`
- DXY, US2Y, US10Y, WTI, VIX, and 2S10S parse into native structured state
- US2Y and US10Y use CNBC live yield quotes from the same source
- 2S10S is calculated as US10Y minus US2Y
- DXY, WTI, and VIX use the existing Yahoo/yfinance transport
- Both market and macro feeds refresh with `X`
- `START` exits cleanly
- No crash, freeze, reboot, or exception observed

T3 tested `.3dsx` SHA-256:

`dc0072485c5980c7acfda8b7fc71e3d36fccbe0f9b371df64074faff185cd76e`

## T5 hardware pass — 2026-09-21

Terminal T5 is hardware PASS.

The native terminal now supports the complete Big-3 3x3 chart matrix, animated candlestick rendering, selected-candle OHLC inspection, cursor navigation, pan and zoom, macro and market feeds, NEWS notification testing, CIA packaging and the restored legacy INFINIT3 HOME Menu icon.

The final T5 bottom screen is intentionally a stable persistent status display. The richer DSi-inspired graphical bottom UI moves to T6.

See `docs/T5_HARDWARE_PASS.md` for the complete milestone record.

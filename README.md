# INFINIT3-3DS

Native Nintendo 3DS market-information and financial-news applications by Jaay.

## Applications

### INFINIT3 TERMINAL
Native New Nintendo 3DS market terminal for market quotes, OHLC/candlestick data, charting, market context, and related analytical tools.

**Current milestone:** T0 HARDWARE PASS

- Native libctru .3dsx launches successfully on New Nintendo 3DS hardware.
- Top-screen rendering verified.
- START exit verified.
- Clean return to Homebrew Launcher verified.
- T0 tested binary SHA-256: `8d7b38f176ac4c122d6bd5e7a35e3cf6d740b23acc3443a1cd1865a122113583`

### INFINIT3 NEWS
Native New Nintendo 3DS financial and market-news reader.

**Current milestone:** Native port pending.

The original DSi implementation and its Mac-side news pipeline already exist and are preserved separately as development references.

## Architecture

INFINIT3 follows a lightweight client/backend architecture:

`Internet / market + news sources -> backend preprocessing -> lightweight HTTP data -> Nintendo 3DS clients`

The handheld applications focus on rendering, controls, interaction, caching, and lightweight parsing rather than scraping modern websites directly.

## Repository Layout

- `apps/terminal/` - INFINIT3 TERMINAL source
- `apps/news/` - INFINIT3 NEWS source
- `server/` - shared backend source intended for public deployment/reproducibility
- `assets/` - branding, icons, screenshots, and presentation assets
- `docs/` - architecture, development, testing, and release documentation
- `packaging/universal-db/` - Universal-DB / Universal-Updater submission material
- `.github/workflows/` - future automated builds and releases

## Distribution Goal

Both applications are intended to be distributed through GitHub Releases and submitted independently to Universal-DB for installation through Universal-Updater.

Release assets will use stable names:

- `INFINIT3_TERMINAL.3dsx`
- `INFINIT3_NEWS.3dsx`

CIA builds may be added after the Homebrew Launcher versions are stable.

## Data Notice

Market information supplied by external data providers may be delayed and is intended for informational/research use. INFINIT3 is not a brokerage execution platform.

## License

License selection is pending before the first public release.

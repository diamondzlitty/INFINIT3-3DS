# INFINIT3 Staged Roadmap

## Guardrails

- The repository remains one public monorepo for **INFINIT3 TERMINAL** and
  **INFINIT3 NEWS**.
- Retrieval, classification, aggregation, scheduling, and source reconciliation live
  on the Mac/backend. Native 3DS apps render, control, cache, snapshot, replay, and
  journal prepared data.
- Each stage needs a testable, shippable boundary. No stage assumes that every planned
  feature exists before a small, useful application can run on hardware.
- T0 is locked as a hardware PASS. Do not regress its verified launch, top-screen
  rendering, START exit, or clean Homebrew Launcher return while working later stages.

## Shared foundations

Before feature growth, establish a compact versioned payload contract shared by both
apps: observed/retrieved timestamps, provider/source identity, staleness, data type,
and bounded error states. Build fixture-driven parsing and an offline cache path first.
The backend may be developed on a Mac, but public deployment/reproducibility must be
documented before a release depends on it.

## TERMINAL: T0–T10

| Stage | Scope | Exit evidence |
| --- | --- | --- |
| **T0 — Hardware baseline** | Native libctru `.3dsx`; top-screen render; START exit; clean Homebrew Launcher return. **Status: HARDWARE PASS.** | Preserve the verified binary SHA-256 `8d7b38f176ac4c122d6bd5e7a35e3cf6d740b23acc3443a1cd1865a122113583` and hardware notes. |
| **T1 — Client shell** | N3DS controls, screen layout, menu/focus model, error/offline states, local settings. | Hardware navigation and exit pass without a network dependency. |
| **T2 — Data contract and cache** | Versioned Terminal payload fixtures, lightweight HTTP client, bounded cache, timestamps/staleness/provenance. | Fixture and offline-cache tests; on-device display of known sample data. |
| **T3 — Core market board** | Watchlists, quote/OHLC display, market clock, session map, simple levels board. | On-device presentation of fresh/stale/absent states with no misleading fallback. |
| **T4 — Macro context MVP** | Regime dashboard, cross-market confirmation matrix, yield curve. | Backend calculations are documented and the 3DS shows source/time/window metadata. |
| **T5 — ICC and snapshots MVP** | Manual ICC structure tracker; `L + R` before/after snapshots and compare view. | Manual entry, persistence, compare/recovery, and invalidation/uncertainty behavior tested on hardware. |
| **T6 — Event case files** | Event mode checkpoint capture and basic event reaction summary. | One scheduled/replayed event case with a complete timestamp sequence and graceful offline behavior. |
| **T7 — Market research views** | Breadth, calendar heatmap, correlation monitor, volatility meter. | Each metric states source/window/staleness and can be hidden if data is unavailable. |
| **T8 — Replay and archive** | Replay browser and local screenshot archive. | Bounded storage, index recovery, and replay of snapshots/event cases verified. |
| **T9 — Journals and cross-app links** | Trading journal; news-to-market reaction panel integration. | Local journal integrity and a provenance-preserving linked reaction card. |
| **T10 — Experimental lab and release hardening** | Range compression; polish, accessibility, performance, docs, packaging, release checks. | Experimental labels/manual fallback are clear; release candidate passes hardware and offline/regression checks. |

## NEWS: N0–N9

| Stage | Scope | Exit evidence |
| --- | --- | --- |
| **N0 — Port baseline** | Create the native N3DS shell while preserving the DSi implementation as a behavioral reference. | Native `.3dsx` launches, renders, exits cleanly, and does not modify the reference implementation. |
| **N1 — Reader shell and controls** | N3DS controls, list/detail screens, loading/error/offline states, local settings. | Hardware navigation works with bundled fixtures. |
| **N2 — Feed contract and cache** | Versioned feed fixtures, HTTP fetch, bounded cache, source/retrieval/observed timestamps. | On-device cached/latest/stale behavior is tested. |
| **N3 — News MVP** | Ticker, topic channels, entity watchlists, primary-source mode. | Native reader presents prepared lightweight feeds with filtering and provenance. |
| **N4 — Ranking MVP** | Explainable news impact ranking and source labels. | Ranking reasons, timestamp, and manual unranked view are available. |
| **N5 — Story comparison** | Source comparison, story timeline, headline diff. | Same-story clustering has source links and updates never overwrite the original record silently. |
| **N6 — Official-release research** | FOMC/release diffs and event-aware story presentation. | Official source pairs, retrieval times, and conservative diffs are testable with fixtures. |
| **N7 — Cross-app reaction** | News-to-market reaction panel with Terminal contract. | A linked card shows story/event provenance, aligned market time, and uncertainty. |
| **N8 — Research journals and archive** | Prediction journal, replay access, optional screenshot/archive linkage. | User entries retain timestamps and original text; storage limits/recovery are verified. |
| **N9 — Experimental signals and release hardening** | Headline-frequency spike, framing disagreement, performance/docs/packaging/release checks. | Experimental labels/context are visible; release candidate passes hardware/offline/regression checks. |

## Release gates

1. Keep applications independently buildable and releasable as
   `INFINIT3_TERMINAL.3dsx` and `INFINIT3_NEWS.3dsx`.
2. Before any Universal-DB submission, provide reproducible build instructions,
   versioned release assets, license decision, attribution/data notices, screenshots,
   hardware test evidence, and a stable public release URL.
3. Do not introduce CIA distribution, background syncing, account collection,
   automated trading, or unreviewed scraping as a prerequisite for the first public
   Homebrew Launcher releases.

## Near-term sequence

The next implementation work is **T1**, in isolation: preserve T0 and establish the
Terminal client shell and controls. In parallel only after its own baseline is ready,
start **N0**. Build T2/N2 fixture contracts before live backend integration; then ship
the narrow T3–T5 and N3–N4 MVP slices before considering event/replay, journals,
cross-app panels, or Experimental analytics.

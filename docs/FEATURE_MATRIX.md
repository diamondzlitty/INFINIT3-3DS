# INFINIT3 Feature Matrix

## Product boundary

INFINIT3 is one public monorepo with two native New Nintendo 3DS applications:

- **INFINIT3 TERMINAL** is the market-context and analysis client.
- **INFINIT3 NEWS** is the financial-news and source-comparison client.

The clients are deliberately lightweight. The Mac/backend retrieves, normalizes,
timestamps, classifies, aggregates, and ranks source data. The 3DS renders prepared
data, accepts controls, keeps bounded local cache/snapshots, and supports replay and
journaling. It is not a brokerage, execution, recommendation, or direct modern-web
scraping platform. Market data can be delayed and is informational/research only.

## Classification key

- **Surface:** Terminal-only, News-only, or Shared.
- **Primary load:** Backend-heavy, 3DS-heavy, or Balanced.
- **Stage:** MVP, Later, or Experimental.

“Backend-heavy” includes the Mac development tools and deployable backend; it does
not require the 3DS to scrape, classify, or reconcile remote sources. “Experimental”
means a bounded research feature with transparent confidence and manual fallback,
not an automated trade signal.

## Requested feature inventory

| Feature | Surface | Primary load | Stage | 3DS responsibility | Backend / Mac responsibility |
| --- | --- | --- | --- | --- | --- |
| Market regime dashboard | Terminal-only | Backend-heavy | MVP | Render top/bottom-screen state and cache last payload | Normalize Big-3 and macro inputs; derive explainable regime labels |
| ICC structure tracker | Terminal-only | Balanced | MVP | Enter swing points, show phase/levels/time, retain local state | Validate optional derived structure; never represent it as a signal |
| Cross-market confirmation matrix | Terminal-only | Backend-heavy | MVP | Render directional matrix and agreement count | Align instruments/time windows and calculate transparent confirmation |
| Event mode | Terminal-only | Backend-heavy | Later | Start/view an event case file and local markers | Schedule event windows, capture checkpoints, assemble reaction timeline |
| Before / after snapshots | Terminal-only | 3DS-heavy | MVP | `L + R` capture, compare, save/replay local snapshots | Supply timestamped quote/candle/yield inputs when connected |
| Yield curve | Terminal-only | Backend-heavy | MVP | Render prepared curve and latest timestamp | Retrieve maturities, normalize units, calculate curve changes |
| Breadth | Terminal-only | Backend-heavy | Later | Render compact breadth panels | Aggregate constituents/advancers/decliners with provenance |
| Session map | Terminal-only | Balanced | MVP | Local market clock display and session overlay | Provide exchange/session calendar rules and exceptions |
| Levels board | Terminal-only | Balanced | MVP | Show manual levels and touch/button edits | Optionally distribute precomputed reference levels with source/time |
| Replay | Shared | 3DS-heavy | Later | Browse cached snapshots, event files, and journal-linked states | Package bounded historical/replay datasets |
| News impact ranking | News-only | Backend-heavy | MVP | Render ranked stories and disclosed reasons | Deduplicate, classify, score recency/entity/event relevance |
| News ticker | News-only | Backend-heavy | MVP | Render scrolling/compact headline feed and cache | Produce lightweight prioritized ticker feed |
| Source comparison | News-only | Backend-heavy | Later | Compare prepared source cards | Cluster same story and retain source/provenance links |
| Primary-source mode | News-only | Backend-heavy | MVP | Filter/render designated primary-source items | Identify/label official filings, releases, and statements |
| Headline diff | News-only | Backend-heavy | Later | Display concise changed-text indicator | Track updates and generate conservative text diffs |
| FOMC / release diffs | News-only | Backend-heavy | Later | Present changed sections and links/citations | Fetch official release pairs and generate reviewed structured diffs |
| Topic channels | News-only | Backend-heavy | MVP | Select channels and cache channel feed | Classify stories into transparent topic taxonomy |
| Entity watchlists | Shared | Balanced | MVP | Manage bounded local symbols/entities and filters | Resolve identifiers and return matching market/news payloads |
| Story timeline | News-only | Backend-heavy | Later | Navigate chronological case/story view | Cluster updates, sources, and related releases by timestamp |
| Cross-app news-to-market reaction panel | Shared | Backend-heavy | Later | Render linked headline/reaction card in both clients | Link events/stories to timestamp-aligned market reactions; show uncertainty |
| N3DS controls | Shared | 3DS-heavy | MVP | Native button/touch navigation, START exit, accessible focus states | Define stable action schema only; no input processing dependency |
| Market clock | Terminal-only | 3DS-heavy | MVP | Render local/UTC/exchange timing and stale-data indication | Supply exchange holiday/session metadata |
| Calendar heatmap | Terminal-only | Backend-heavy | Later | Render compact event/intensity calendar | Aggregate calendar, event importance, and historical context |
| Correlation monitor | Terminal-only | Backend-heavy | Later | Render rolling relation summary and warnings | Align series; calculate windows and confidence/staleness metadata |
| Volatility meter | Terminal-only | Backend-heavy | Later | Render calibrated meter and provenance | Calculate/normalize volatility inputs and threshold methodology |
| Range compression | Terminal-only | Backend-heavy | Experimental | Render observation state, never a trade prompt | Compute documented range statistics and confidence bounds |
| Headline-frequency spike | News-only | Backend-heavy | Experimental | Render activity alert with count/window | Detect volume changes against baselines; expose threshold/context |
| Framing disagreement | News-only | Backend-heavy | Experimental | Render source-framing comparison with caveats | Compare source clusters/classification; retain examples and uncertainty |
| Screenshot archive | Shared | 3DS-heavy | Later | Capture/index local screenshots and attach optional journal references | Optional export/index metadata path; no mandatory cloud upload |
| Trading journal | Terminal-only | 3DS-heavy | Later | Store user-authored observations, thesis, outcome, and snapshot links locally | Optional encrypted/exportable sync format; never execute trades |
| Prediction journal | Shared | 3DS-heavy | Later | Store timestamped user predictions, criteria, and later review | Optional evaluation/export tooling; preserve immutable original entries |

## Product rules

1. A label, score, matrix, ICC state, correlation, volatility reading, or reaction
   panel is context—not investment advice or a trade recommendation.
2. Every backend-derived item should carry as much provenance as the source permits:
   source, retrieval time, observed time, calculation window, and staleness state.
3. No feature depends on a cloud account or backend write path for basic 3DS use.
   The handheld must remain useful with its local cache, local snapshots, and journals.
4. Local journals and screenshots stay user-controlled. Any future export/sync is
   opt-in, documented, and separately reviewed.
5. Automated structure, ranking, and framing work retain a manual view and a clear
   uncertainty/experimental label wherever appropriate.

## MVP definition

The MVP is intentionally narrow: reliable native Terminal navigation and prepared
market payloads; regime, matrix, yield, session/clock, levels, ICC manual tracking,
snapshot compare, and watchlists; plus a native News reader with ticker, channels,
primary-source mode, impact ranking, and watchlists. All Later and Experimental
items remain out of the critical path.

## Terminal T5 final hardware pass — 2026-09-21

- [x] Real candle parser and feeds
- [x] Native candlestick chart renderer
- [x] NAS100 / US30 / GOLD
- [x] 15m / 30m / 1h
- [x] Nine complete chart combinations
- [x] Instrument and timeframe controls
- [x] Animated chart transitions
- [x] Candle cursor and highlight
- [x] Selected OHLC inspection
- [x] Pan and zoom
- [x] Circle Pad navigation
- [x] C-stick navigation and zoom
- [x] Manual refresh
- [x] NEWS notification test framework
- [x] Macro dashboard data
- [x] Correct legacy HOME Menu icon
- [x] CIA packaging and verified deployment
- [x] Stable persistent bottom-screen status
- [x] Hardware stability pass

Deferred to T6:

- graphical DSi-inspired bottom UI
- touch controls
- animated graphical bottom buttons

# INFINIT3 TERMINAL — T5 Hardware Pass

Date: 2026-09-21

## Final status

Terminal T5 is hardware PASS on the New Nintendo 3DS.

Final verified artifacts:

- 3DSX SHA256: `44a6cdfa3eaa09cc41636b1c146f0c94a7d27790193cc97e6b3973ec600d22b4`
- CIA SHA256: `27a3e24bf79a64826ff715ee6877bcf01399b80bb9389e9991fd2ea74d97d150`
- SMDH SHA256: `def0a9b8c1477a4a03bbf9937da1af79f3b757303981e194fe379193e6cf6a31`
- Icon SHA256: `8bf40a878d284152cb3df8cb54499889134032913ade78bc410fb1ebdbdf2295`
- Title ID: `000400000494E300`
- Product code: `CTR-H-IN3T`

The final CIA was FTP readback verified byte-for-byte before hardware installation.

## T5 progression

### T5A — candle foundation

- Native Candle structure and parser.
- Real Mac backend candle feeds.
- 60 real candles per feed.
- Native top-screen candlestick rendering.
- Wick, body, grid, autoscaling and latest-price presentation.
- T5A-2 and T5A-3 hardware passed.

### T5B — Big-3 chart matrix and CIA

Nine chart combinations are available:

- NAS100 15m
- NAS100 30m
- NAS100 1h
- US30 15m
- US30 30m
- US30 1h
- GOLD 15m
- GOLD 30m
- GOLD 1h

Controls:

- L / R: previous or next instrument.
- ZL / ZR: previous or next timeframe.
- X: manual refresh.
- Y: NEWS notification test.
- START: exit.
- D-pad Left / Right: candle cursor.
- D-pad Up / Down: zoom.
- Circle Pad: faster candle navigation.
- C-stick Left / Right: faster navigation.
- C-stick Up / Down: zoom.

### T5C — chart transitions

Implemented restrained native chart animation for:

- initial entrance
- instrument selection
- timeframe selection
- refresh and latest-candle updates

### T5D — chart inspection

Implemented:

- selected-candle cursor and highlight
- selected OHLC display
- visible-window tracking
- history navigation
- zoom boundaries
- Circle Pad and C-stick navigation

### T5E — HOME Menu identity and stable bottom status

- Restored the legacy INFINIT3 infinity icon.
- HOME Menu icon hardware verified.
- Existing selected-title banner retained.
- Final T5 bottom screen intentionally uses a stable status layout.
- Instrument and timeframe selections persist visibly.
- Bottom rendering is isolated from top-chart animation.
- Hardware testing confirmed the prior black flashing is eliminated.
- Full DSi-style graphical boxes, touch controls and animated bottom UI are deferred to T6.

### T5F — banner investigation

The current packaging path uses a static Nintendo 3DS banner model generated with bannertool.

A native-style animated or model-driven HOME Menu banner was investigated but was not made a blocker for T5. It remains a future polish item.

The current generated banner audio payload is intentionally silent.

## Networking

Runtime configuration remains external:

`sdmc:/3ds/INFINIT3/config.ini`

No LAN IP is hardcoded in the terminal source.

The Mac backend continues to provide:

- market.txt
- macro.txt
- nine candle feeds

Heavy data acquisition and preprocessing remain backend-side while the New 3DS client consumes lightweight HTTP text data.

## Final hardware result

PASS:

- HOME Menu identity
- application launch
- all nine chart combinations
- instrument selection
- timeframe selection
- cursor and OHLC inspection
- pan and zoom
- refresh
- NEWS notification test framework
- stable bottom-screen selection display
- chart animation
- clean exit
- repeated interaction stability

## Next milestone

T6 begins the dedicated New Nintendo 3DS UI and interaction layer.

Primary T6 target:

Recreate and expand the original DSi INFINIT3 bottom-screen design using a proper graphical renderer, including structured boxes, grayscale visual hierarchy, persistent selection states, touch targets and live button press animation without coupling bottom-screen rendering to top-chart animation.
